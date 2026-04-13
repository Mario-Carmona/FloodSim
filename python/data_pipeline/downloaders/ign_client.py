"""Client for fetching DEM and Land Cover data from the IGN (Instituto Geográfico Nacional)."""

import concurrent.futures
import xml.etree.ElementTree as ET
from typing import Dict, List, Tuple

import fiona
import numpy as np
from loguru import logger
from pyproj import CRS, Transformer
from rasterio import features
from shapely.geometry import shape
from shapely.geometry.base import BaseGeometry
from skimage.morphology import reconstruction

from utils.types import Bounds, SpatialContext, StaticRaster
from utils.api_client import fetch_with_retries
from utils.raster_io import load_raster_from_bytes
from utils.spatial import calculate_grid_edges


class IgnClient:
    """A client for interacting with the IGN INSPIRE APIs.

    Attributes:
        IGN_WCS_URL (str): The endpoint URL for the IGN WCS service.
        WCS_20_NAMESPACE (str): The XML namespace for WCS 2.0.
        WCS_NS_MAP (dict): Namespace mapping for XML parsing.
        IGN_WFS_URL (str): The endpoint URL for the IGN WFS service.
        CODIIGE_TO_INDEX (dict): Mapping from SIOSE/CODIIGE string codes to internal integer IDs.
    """

    # --- API Endpoints & Configuration ---
    IGN_WCS_URL = "https://servicios.idee.es/wcs-inspire"
    WCS_20_NAMESPACE = "http://www.opengis.net/wcs/2.0"
    WCS_NS_MAP = {'wcs': WCS_20_NAMESPACE}

    IGN_WFS_URL = "https://servicios.idee.es/wfs-inspire"

    # Land Cover classes mapping (SIOSE/CODIIGE to internal numeric indices)
    CODIIGE_TO_INDEX = {
        "110": 1,  # "Mixed urban"
        "111": 2,  # "Urban core / Historic center"
        "112": 3,  # "Urban expansion"
        "113": 4,  # "Discontinuous urban fabric"
        "114": 5,  # "Urban green areas"
        "120": 6,  # "Primary sector facilities"
        "121": 7,  # "Agricultural and/or livestock facilities"
        "122": 8,  # "Forestry facilities"
        "123": 9,  # "Mining extraction"
        "130": 10, # "Industrial"
        "140": 11, # "Services and public facilities"
        "150": 12, # "Agricultural settlement and orchards"
        "160": 13, # "Transport infrastructure"
        "161": 14, # "Road and rail networks"
        "162": 15, # "Ports"
        "163": 16, # "Airports"
        "170": 17, # "Technical infrastructures"
        "171": 18, # "Supply infrastructures"
        "172": 19, # "Waste infrastructures"
        "210": 20, # "Arable land / Herbaceous crops"
        "220": 21, # "Greenhouses"
        "230": 22, # "Woody / Permanent crops"
        "231": 23, # "Citrus orchards"
        "232": 24, # "Non-citrus orchards"
        "233": 25, # "Vineyards"
        "234": 26, # "Olive groves"
        "235": 27, # "Other woody crops"
        "236": 28, # "Woody crop combinations"
        "240": 29, # "Meadows / Pastures"
        "250": 30, # "Crop combinations"
        "260": 31, # "Crop and vegetation combinations"
        "310": 32, # "Forest"
        "311": 33, # "Broad-leaved forest"
        "312": 34, # "Coniferous forest"
        "313": 35, # "Mixed forest"
        "320": 36, # "Natural grasslands"
        "330": 37, # "Scrubland / Shrubland"
        "340": 38, # "Vegetation combinations"
        "350": 39, # "Areas without vegetation / Bare areas"
        "351": 40, # "Beaches, dunes, and sand plains"
        "352": 41, # "Bare rock"
        "353": 42, # "Temporarily unstocked due to fires / Burnt areas"
        "354": 43, # "Bare soil"
        "410": 44, # "Wetlands"
        "411": 45, # "Wet and swampy areas"
        "412": 46, # "Peat bogs"
        "413": 47, # "Salt marshes"
        "414": 48, # "Salines / Salt pans"
        "510": 49, # "Water bodies"
        "511": 50, # "Water courses"
        "512": 51, # "Lakes and lagoons"
        "513": 52, # "Reservoirs"
        "514": 53, # "Artificial water bodies"
        "515": 54, # "Seas and oceans"
        "516": 55  # "Glaciers and perpetual snow"
    }

    @classmethod
    def _validate_resolution_ign(cls, resolution: int, mdt_wcs_url: str, target_crs: CRS) -> None:
        """Validates if the requested spatial resolution is available in the IGN WCS catalog.

        It performs a GetCapabilities request to the WCS endpoint, parses the XML 
        response, and extracts the available resolutions for the target CRS to 
        ensure the requested DEM quality exists.

        Args:
            resolution (int): The desired spatial resolution in meters.
            mdt_wcs_url (str): The endpoint URL for the IGN WCS service.
            target_crs (CRS): The target Coordinate Reference System.

        Raises:
            Exception: If the server connection fails or the resolution is unavailable.
        """
        logger.info(f"Validating IGN WCS capabilities for {resolution}m resolution.")

        params = {"service": "WCS", "version": "2.0.1", "request": "GetCapabilities"}
        content = fetch_with_retries(mdt_wcs_url, params=params, timeout=30)
        
        if not content:
            raise Exception("IGN API validation failed due to server connection issues.")

        root = ET.fromstring(content)
            
        # 1. Extract all available coverage IDs
        layers = [c.text for c in root.findall('.//wcs:CoverageId', cls.WCS_NS_MAP)]
        
        # 2. Filter layers matching our target EPSG and extract resolution values
        epsg_code = target_crs.to_epsg()
        prefix = f"Elevacion{epsg_code}_"

        available_resolutions = sorted([
            int(layer.split('_')[-1]) for layer in layers 
            if layer and layer.startswith(prefix) and layer.split('_')[-1].isdigit()
        ])

        # 3. Validation
        if resolution in available_resolutions:
            logger.info(f"Resolution {resolution}m is confirmed available for EPSG:{epsg_code}.")
        else:
            error_msg = (
                f"Requested resolution {resolution}m is NOT available. "
                f"Available options for EPSG:{epsg_code} are: {available_resolutions} meters."
            )
            logger.error(error_msg)
            raise Exception(error_msg)

    @classmethod
    def fetch_mdt(cls, resolution: int, target_crs: CRS, bbox_crs: CRS, bbox: Bounds) -> StaticRaster:
        """Fetches the Digital Elevation Model (MDT) from the IGN WCS service.

        Downloads the DEM tiles required to cover the bounding box at the specified 
        resolution, dynamically handling CRS transformations if necessary.

        Args:
            resolution (int): Desired spatial resolution in meters.
            target_crs (CRS): The CRS in which the data should be downloaded.
            bbox_crs (CRS): The CRS of the provided bounding box.
            bbox (Bounds): The spatial boundaries of the requested area.

        Returns:
            StaticRaster: A raster object containing the merged DEM data.

        Raises:
            Exception: If the download fails for any of the necessary tiles.
        """
        mdt_wcs_url = f"{cls.IGN_WCS_URL}/mdt"

        cls._validate_resolution_ign(resolution, mdt_wcs_url, target_crs)

        coverage_id = f"Elevacion{target_crs.to_epsg()}_{resolution}"

        # Transform BBox to the target CRS if they differ
        if bbox_crs != target_crs:
            transformer = Transformer.from_crs(bbox_crs, target_crs, always_xy=True)
            min_x, min_y = transformer.transform(bbox.min_x, bbox.min_y)
            max_x, max_y = transformer.transform(bbox.max_x, bbox.max_y)
            bbox = Bounds(min_x=min_x, max_x=max_x, min_y=min_y, max_y=max_y)

        # Safe pixel limit per tile (kept below 4096 to prevent server rejection)
        MAX_PIXELS_PER_REQUEST = 4000
        step_size = MAX_PIXELS_PER_REQUEST * resolution

        x_edges = calculate_grid_edges(bbox.min_x, bbox.max_x, step_size)
        y_edges = calculate_grid_edges(bbox.min_y, bbox.max_y, step_size)

        data_bytes = []

        # Download grid tiles
        for i in range(len(x_edges) - 1):
            for j in range(len(y_edges) - 1):
                logger.info(f"Downloading DEM Tile X:{i} Y:{j}...")
                params = {
                    "service": "WCS",
                    "version": "2.0.1",
                    "request": "GetCoverage",
                    "coverageId": coverage_id,
                    "format": "image/tiff",
                    "subset": [
                        f"x({x_edges[i]},{x_edges[i+1]})",
                        f"y({y_edges[j]},{y_edges[j+1]})"
                    ]
                }
                
                content = fetch_with_retries(mdt_wcs_url, params=params, timeout=60)
                if content:
                    data_bytes.append(content)
                else:
                    raise Exception(f"Failed to download DEM tile X:{i} Y:{j} after retries.")

        return load_raster_from_bytes(data_bytes, np.float32)

    @classmethod
    def fetch_land_cover(cls, ctx: SpatialContext) -> StaticRaster:
        """Fetches and rasterizes Land Cover (SIOSE) data from the IGN WFS service.

        This method parallelizes requests to the IGN WFS to retrieve land cover features 
        in GML format. It parses the geometries, maps them to an internal classification 
        index, rasterizes the vectors, and finally applies morphological expansion to 
        safely handle maritime zones.

        Args:
            ctx (SpatialContext): The spatial context containing bounding box, CRS, 
                and resolution parameters.

        Returns:
            StaticRaster: A rasterized land cover matrix with its spatial context.
        """
        MAX_FEATURES = 5000
        TILE_SIZE_METERS = 1000.0
        MAX_WORKERS = 4 # Safe parallelism level to avoid blocking by IGN

        epsg_code = ctx.crs.to_epsg()
        land_cover_wfs_url = f"{cls.IGN_WFS_URL}/ocupacion-suelo"
        out_shape = (ctx.height, ctx.width)

        # 1. Generate grid for parallel WFS requests
        x_edges = calculate_grid_edges(ctx.bounds.min_x, ctx.bounds.max_x, TILE_SIZE_METERS)
        y_edges = calculate_grid_edges(ctx.bounds.min_y, ctx.bounds.max_y, TILE_SIZE_METERS)

        tiles_to_download = [
            {
                'x_idx': i, 'y_idx': j,
                'min_x': x_edges[i], 'max_x': x_edges[i+1],
                'min_y': y_edges[j], 'max_y': y_edges[j+1]
            }
            for i in range(len(x_edges) - 1) for j in range(len(y_edges) - 1)
        ]

        logger.info(f"Starting parallel download of {len(tiles_to_download)} WFS tiles...")

        def _download_and_extract_tile(tile: Dict[str, float]) -> List[Tuple[str, BaseGeometry, int]]:
            """Downloads a specific bounding box tile and parses its GML features."""
            logger.info(f"Downloading Land Cover Tile X:{tile['x_idx']} Y:{tile['y_idx']}...")
            srs_uri = f"http://www.opengis.net/def/crs/EPSG/0/{epsg_code}"

            params = {
                "service": "WFS",
                "version": "2.0.0",
                "request": "GetFeature",
                "typeNames": "lcv:LandCoverUnit",
                "outputFormat": "application/gml+xml; version=3.2", 
                "srsName": srs_uri,
                "bbox": f"{tile['min_x']},{tile['min_y']},{tile['max_x']},{tile['max_y']},{srs_uri}",
                "count": MAX_FEATURES
            }
            headers = {'Accept': 'application/xml, text/xml, */*'}
            
            content = fetch_with_retries(
                land_cover_wfs_url, params=params, headers=headers, max_retries=3, timeout=120
            )
            
            if not content or b'numberReturned="0"' in content:
                return []

            try:
                root = ET.fromstring(content)
                num_matched = int(root.get('numberMatched', -1))
                num_returned = int(root.get('numberReturned', -1))

                if num_matched > -1 and num_returned > -1 and num_matched > num_returned:
                    logger.warning(f"WFS TRUNCATION DETECTED in tile: Missing {num_matched - num_returned} elements.")
        
                class_map = {}
                for unit in root.iter():
                    if not unit.tag.endswith('LandCoverUnit'): 
                        continue

                    gml_id = unit.attrib.get('{http://www.opengis.net/gml/3.2}id')
                    if not gml_id: 
                        continue
                        
                    class_value = 0
                    for elem in unit.iter():
                        if elem.tag.split('}')[-1] == 'class':
                            href = elem.attrib.get('{http://www.w3.org/1999/xlink}href')
                            if href:
                                numeric_code = href.split('/')[-1] 
                                class_value = cls.CODIIGE_TO_INDEX.get(numeric_code, 0)
                                
                    class_map[gml_id] = class_value
                    
                extracted_polygons = []
                with fiona.BytesCollection(content) as collection:
                    for feature in collection:
                        gml_id = feature['properties']['gml_id']
                        geom = shape(feature['geometry'])
                        extracted_polygons.append((gml_id, geom, class_map.get(gml_id, 0)))
                        
                return extracted_polygons

            except Exception as e:
                logger.debug(f"Error parsing tile [{tile['x_idx']},{tile['y_idx']}]: {e}")
                return []

        # 2. Concurrent Download & Deduplication
        unique_polygons = {}

        with concurrent.futures.ThreadPoolExecutor(max_workers=MAX_WORKERS) as executor:
            futures = {
                executor.submit(_download_and_extract_tile, tile): tile 
                for tile in tiles_to_download
            }
            
            for future in concurrent.futures.as_completed(futures):
                try:
                    for gml_id, geom, class_num in future.result():
                        # Using a dict inherently filters duplicate GML IDs from overlapping tiles
                        unique_polygons[gml_id] = (geom, class_num)
                except Exception as e:
                    logger.error(f"Concurrent tile processing failed: {e}")

        if not unique_polygons:
            logger.warning("No geometries extracted. Returning empty raster matrix.")
            return StaticRaster(data=np.zeros(out_shape, dtype=np.int8), spatial_context=ctx)

        # 3. Rasterization Preparation
        shapes_to_rasterize = [
            (geom, class_num) for geom, class_num in unique_polygons.values()
        ]

        logger.info(f"Rasterizing geometries into an array of shape {out_shape}...")
        
        # Burn all polygons into a single continuous raster grid
        raster_matrix = features.rasterize(
            shapes=shapes_to_rasterize,
            out_shape=out_shape,
            transform=ctx.transform,
            fill=0, 
            all_touched=False,
            dtype=np.int8
        )

        # 4. POST-PROCESSING: Morphological Expansion of the Sea
        logger.info("Extending 'Seas and oceans' class into unclassified contiguous cells...")
        
        # Step A: Seed creation - Cells definitively classified as Sea (Class 54)
        seed = (raster_matrix == 54)
        
        # Step B: Mask creation - Allowed expansion areas (Sea + Unclassified zones)
        mask = (raster_matrix == 54) | (raster_matrix == 0)
        
        # Step C: Morphological Reconstruction - Expands the seed within the mask boundaries
        expanded_sea = reconstruction(seed, mask, method='dilation')
        
        # Step D: Apply changes back to the main raster matrix
        raster_matrix[expanded_sea > 0] = 54

        logger.success("Vector-to-Raster processing completed successfully.")

        return StaticRaster(data=raster_matrix, spatial_context=ctx)