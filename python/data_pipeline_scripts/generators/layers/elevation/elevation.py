"""
Elevation layer generator for the flood simulator pipeline.

This module defines the logic required to import and process the base digital 
elevation model (DEM) for the geographic area of study. It fetches data 
from the IGN (Instituto Geográfico Nacional) Web Coverage Service (WCS) 
by dividing large geographic extents into smaller, manageable tiles.
"""

import xml.etree.ElementTree as ET
from datetime import datetime
from pathlib import Path
from typing import Any, Dict, List

import numpy as np
import requests
from loguru import logger
from pyproj import CRS, Transformer

from data_io.idrisi_io import IdrisiIO
from generators.static_layer import StaticLayerGenerator
from generators.layers.elevation.types import GeoBoundingBox, GeoPoint
from misc.types import StaticRaster, VisualConfig


class ElevationGenerator(StaticLayerGenerator):
    """
    Handles the initialization, loading, and setup of the elevation data layer.

    This generator interfaces with the IGN WCS endpoint to download elevation
    data. To avoid server-side limits on payload sizes, it automatically 
    splits the requested bounding box into smaller tiles, downloads them 
    sequentially, and delegates the merging process to the I/O handler.

    Attributes:
        SOURCE_CRS (CRS): The geographic coordinate system used for input (EPSG:4326).
        TARGET_CRS (CRS): The working projection system in meters (EPSG:25830).
        IGN_MDT_WCS_URL (str): The endpoint URL for the IGN DEM WCS.
        WCS_20_NAMESPACE (str): The XML namespace for WCS 2.0.
        WCS_NS_MAP (Dict[str, str]): Namespace mapping for XML parsing.
        MAX_PIXELS_PER_REQUEST (int): The maximum safe number of pixels per 
            axis for a single WCS request to avoid server rejections.
    """

    # --- Coordinate Systems ---
    SOURCE_CRS = CRS.from_epsg(4326)  # Geographic coordinates input (e.g., Google Maps)
    TARGET_CRS = CRS.from_epsg(25830) # Working projection for meter-based calculations (UTM 30N)

    # --- API Endpoints & Configuration ---
    IGN_MDT_WCS_URL = "https://servicios.idee.es/wcs-inspire/mdt"
    WCS_20_NAMESPACE = "http://www.opengis.net/wcs/2.0"
    WCS_NS_MAP = {'wcs': WCS_20_NAMESPACE}

    # Safe pixel limit per tile (kept below 4096 to prevent server rejection)
    MAX_PIXELS_PER_REQUEST = 4000

    def __init__(self) -> None:
        """
        Initializes the elevation layer generator with its specific visual configuration.
        """
        super().__init__("elevation", VisualConfig(cbar_unit="m", cmap="terrain"))

    def _validate_resolution_ign(self, resolution: int) -> None:
        """
        Validates if the requested spatial resolution is available in the IGN WCS catalog.

        It performs a GetCapabilities request to the WCS endpoint, parses the XML 
        response, and extracts the available resolutions for the target CRS to 
        ensure the requested DEM quality exists.

        Args:
            resolution (int): The desired spatial resolution in meters.

        Raises:
            ValueError: If the requested resolution is not available for the target CRS,
                or if the XML response cannot be parsed.
            ConnectionError: If the connection to the IGN server fails.
        """
        logger.debug(f"Validating IGN WCS capabilities for {resolution}m resolution.")

        try:
            response = requests.get(
                self.IGN_MDT_WCS_URL, 
                params={
                    "service": "WCS", 
                    "version": "2.0.1", 
                    "request": "GetCapabilities"
                },
                timeout=30
            )
            response.raise_for_status()
            root = ET.fromstring(response.content)
            
            # 1. Extract all available coverage IDs
            layers = [c.text for c in root.findall('.//wcs:CoverageId', self.WCS_NS_MAP)]
        
            # 2. Filter layers matching our target EPSG and extract resolution values
            epsg_code = self.TARGET_CRS.to_epsg()
            prefix = f"Elevacion{epsg_code}_"
            available_resolutions = []
        
            for layer in layers:
                if layer and layer.startswith(prefix):
                    res_str = layer.split('_')[-1]
                    if res_str.isdigit():
                        available_resolutions.append(int(res_str))
        
            available_resolutions.sort()

            # 3. Verification
            if resolution in available_resolutions:
                logger.info(f"Resolution {resolution}m is confirmed available for EPSG:{epsg_code}.")
            else:
                error_msg = (
                    f"Requested resolution {resolution}m is NOT available. "
                    f"Available options for EPSG:{epsg_code} are: {available_resolutions} meters."
                )
                logger.error(error_msg)
                raise ValueError(error_msg)

        except requests.exceptions.RequestException as e:
            logger.error(f"Failed to connect to IGN WCS server for validation: {e}")
            raise ConnectionError(f"IGN API validation failed: {e}") from e
        except ET.ParseError as e:
            logger.error(f"Failed to parse XML from IGN capabilities: {e}")
            raise ValueError(f"Invalid XML response from IGN: {e}") from e

    def _generate(
        self, 
        dependent_layers: Dict[str, Any], 
        start_date: datetime, 
        end_date: datetime, 
        cfg: Dict[str, Any], 
        cfg_dir: Path
    ) -> StaticRaster:
        """
        Executes the main pipeline for generating the elevation layer via tiled downloads.

        This method extracts the bounding box, transforms the coordinates to the 
        target metric CRS, and calculates a grid based on the maximum allowed 
        pixels per request. It then downloads each tile sequentially from the 
        WCS server and merges them into a single continuous raster.

        Args:
            dependent_layers (Dict[str, Any]): Dictionary of pre-calculated dependencies.
            start_date (datetime): Simulation start date (unused for static layers).
            end_date (datetime): Simulation end date (unused for static layers).
            cfg (Dict[str, Any]): Specific layer settings including resolution 
                and bounding box coordinates.
            cfg_dir (Path): Base directory for configuration.

        Returns:
            StaticRaster: The downloaded, merged, and instantiated elevation raster.
            
        Raises:
            Exception: If any individual GetCoverage tile request fails or 
                returns an HTTP error.
            requests.exceptions.RequestException: On general connection failures.
        """ 
        resolution: int = cfg['resolution']

        # Step 1: Validate capabilities
        self._validate_resolution_ign(resolution)

        # Step 2: Construct the geographic bounding box from WGS84 coordinates
        bbox_cfg = cfg['bounding_box']
        zone = GeoBoundingBox(
            south_west=GeoPoint(
                lon=bbox_cfg['south_west']['lon'],
                lat=bbox_cfg['south_west']['lat']
            ),
            north_east=GeoPoint(
                lon=bbox_cfg['north_east']['lon'],
                lat=bbox_cfg['north_east']['lat']
            )
        )

        # Step 3: Transform coordinates to the target CRS (UTM)
        logger.debug(f"Transforming bounding box from {self.SOURCE_CRS.to_epsg()} to {self.TARGET_CRS.to_epsg()}.")
        transformer = Transformer.from_crs(self.SOURCE_CRS, self.TARGET_CRS, always_xy=True)

        min_x, min_y = transformer.transform(zone.south_west.lon, zone.south_west.lat)
        max_x, max_y = transformer.transform(zone.north_east.lon, zone.north_east.lat)

        # Step 4: Prepare the WCS request parameters
        coverage_id = f"Elevacion{self.TARGET_CRS.to_epsg()}_{resolution}"
        logger.info(f"Downloading coverage '{coverage_id}' from IGN WCS...")

        # Calculate the maximum physical distance (in meters) per tile request
        step_size = self.MAX_PIXELS_PER_REQUEST * resolution

        # Step 5: Calculate the grid edges for tiled downloading
        x_edges = np.arange(min_x, max_x, step_size).tolist()
        if x_edges[-1] < max_x: 
            x_edges.append(max_x)
        
        y_edges = np.arange(min_y, max_y, step_size).tolist()
        if y_edges[-1] < max_y: 
            y_edges.append(max_y)

        total_tiles = (len(x_edges) - 1) * (len(y_edges) - 1)
        logger.info(f"Splitting request into {total_tiles} tiles...")

        try:
            data_bytes: List[bytes] = []

            # Step 6: Sequentially request each tile
            for i in range(len(x_edges) - 1):
                for j in range(len(y_edges) - 1):
                    tile_min_x, tile_max_x = x_edges[i], x_edges[i+1]
                    tile_min_y, tile_max_y = y_edges[j], y_edges[j+1]

                    logger.debug(f"Downloading Tile X:{i} Y:{j}...")
                    response = requests.get(
                        self.IGN_MDT_WCS_URL, 
                        params={
                            "service": "WCS",
                            "version": "2.0.1",
                            "request": "GetCoverage",
                            "coverageId": coverage_id,
                            "format": "image/tiff",
                            "subset": [
                                f"x({tile_min_x},{tile_max_x})",
                                f"y({tile_min_y},{tile_max_y})"
                            ]
                        }, 
                        timeout=60
                    )
                
                    if response.status_code == 200:
                        data_bytes.append(response.content)
                    else:
                        logger.error(f"Tile download failed: {response.text}")
                        raise Exception(f"Failed to download DEM tile: HTTP {response.status_code}")

            # Step 7: Delegate the merging of byte streams to the I/O handler
            return IdrisiIO.load_from_geotiff_bytes(data_bytes)
            
        except requests.exceptions.RequestException as e:
            logger.error(f"Connection error during GetCoverage request: {e}")
            raise