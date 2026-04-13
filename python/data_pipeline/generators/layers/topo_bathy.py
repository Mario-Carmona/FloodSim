"""
Topo-bathymetry layer generator for the flood simulator pipeline.

This module defines the logic required to merge surface elevation (topography) 
with underwater elevation (bathymetry). It fetches bathymetric data from EMODnet, 
spatially aligns it to the existing DEM, and fuses them using land cover 
classifications to identify valid water bodies.
"""

from datetime import datetime
from pathlib import Path
from typing import Any, Dict, Tuple

import numpy as np
from loguru import logger
from pyproj import CRS
from rasterio.transform import Affine
from rasterio.warp import Resampling, reproject

from downloaders.emodnet_client import EmodnetClient
from generators.static_layer import StaticLayerGenerator
from utils.types import StaticRaster, VisualConfig


class TopoBathyGenerator(StaticLayerGenerator):
    """Handles the initialization and generation of the topobathymetric data layer.

    This generator uses the base topography and land cover layers to fetch 
    and align bathymetric data. It then creates a continuous elevation model 
    by injecting bathymetry values exclusively into areas classified as water bodies.
    """

    def __init__(self, save_layer: bool) -> None:
        """Initializes the topo_bathy layer generator.

        Sets up the base `DataGenerator` properties, assigning the unique name 
        'topo_bathy' and defining the visual configuration for subsequent plotting.

        Args:
            save_layer (bool): Flag determining whether the generated topobathy 
                raster should be physically saved to disk.
        """
        super().__init__(
            name="topo_bathy", 
            save_layer=save_layer, 
            visual_config=VisualConfig(cbar_unit="m", cmap="terrain")
        )

    def _generate(
        self, 
        dependent_layers: Dict[str, Any], 
        start_date: datetime, 
        end_date: datetime, 
        cfg: Dict[str, Any], 
        cfg_dir: Path
    ) -> StaticRaster:
        """Executes the pipeline for generating the merged elevation layer.

        Args:
            dependent_layers (Dict[str, Any]): Dictionary of pre-calculated 
                dependencies. MUST contain 'topography' and 'land_cover'.
            start_date (datetime): Simulation start date (unused for static layers).
            end_date (datetime): Simulation end date (unused for static layers).
            cfg (Dict[str, Any]): Specific layer settings.
            cfg_dir (Path): Base directory for configuration.

        Returns:
            StaticRaster: The merged topobathymetric raster and its spatial context.

        Raises:
            KeyError: If 'topography' or 'land_cover' are missing from `dependent_layers`.
            EmodnetClientError: If the underlying client fails to fetch the bathymetry.
        """
        # 1. Validate dependencies
        required_layers = ['topography', 'land_cover']
        for req in required_layers:
            if req not in dependent_layers:
                error_msg = f"TopoBathyGenerator requires '{req}' to be generated first."
                logger.error(error_msg)
                raise KeyError(error_msg)

        topography_layer = dependent_layers['topography']
        land_cover_layer = dependent_layers['land_cover']

        # 2. Fetch Bathymetry
        logger.info("Downloading Bathymetry from EMODnet")
        emodnet_raster = EmodnetClient.fetch_bathymetry(topography_layer.spatial_context)

        # 3. Spatial Alignment
        logger.info("Spatial Alignment")
        emodnet_aligned = self._align_rasters(
            source_layer=emodnet_raster,
            target_shape=topography_layer.data.shape,
            target_transform=topography_layer.spatial_context.transform,
            target_crs=topography_layer.spatial_context.crs
        )

        # 4. Topobathymetric Merge
        logger.info("Topobathymetric Fusion")
        combined_array = self._combine_topo_and_bathy(
            land_cover_array=land_cover_layer.data, 
            mdt_array=topography_layer.data, 
            bathy_array=emodnet_aligned
        )

        logger.success("Topobathymetry generation completed successfully.")

        return StaticRaster(
            data=combined_array, 
            spatial_context=topography_layer.spatial_context
        )

    def _align_rasters(self, source_layer: StaticRaster, target_shape: tuple, target_transform: Affine, target_crs: CRS) -> np.ndarray:
        """Reprojects and resamples the source raster to match the target grid exactly.

        Args:
            source_layer (StaticRaster): The input raster (e.g., raw bathymetry).
            target_shape (Tuple[int, int]): The desired (height, width) of the output array.
            target_transform (Affine): The affine transformation matrix of the target.
            target_crs (CRS): The coordinate reference system of the target.

        Returns:
            np.ndarray: A new numpy array containing the aligned data.
        """
        aligned_array = np.zeros(target_shape, dtype=np.float32)
        
        reproject(
            source=source_layer.data,
            destination=aligned_array,
            src_transform=source_layer.spatial_context.transform,
            src_crs=source_layer.spatial_context.crs,
            dst_transform=target_transform,
            dst_crs=target_crs,
            resampling=Resampling.bilinear  # Bilinear smooths bathymetry transitions
        )
        return aligned_array

    def _combine_topo_and_bathy(
        self, 
        land_cover_array: np.ndarray, 
        mdt_array: np.ndarray, 
        bathy_array: np.ndarray
    ) -> np.ndarray:
        """Injects bathymetric data into the digital elevation model.

        Uses the land cover matrix to identify water bodies and replaces the 
        topography values (which are typically flat/zero over water) with the 
        aligned bathymetry values, applying the datum offset.

        Args:
            land_cover_array (np.ndarray): The categorical land cover matrix.
            mdt_array (np.ndarray): The surface digital elevation model.
            bathy_array (np.ndarray): The aligned bathymetry matrix.

        Returns:
            np.ndarray: The fused topobathymetric continuous array.
        """
        combined = np.copy(mdt_array)
        
        # SIOSE / CODIIGE indices corresponding to water bodies
        water_classes = [
            47, # Salt marshes
            48, # Salines / Salt pans
            49, # Water bodies
            50, # Water courses
            51, # Lakes and lagoons
            52, # Reservoirs
            53, # Artificial water bodies
            54  # Seas and oceans
        ]
        
        # Create a boolean mask where the land cover indicates a water body
        is_water_body = np.isin(land_cover_array, water_classes)
        
        # Inject bathymetry into the DEM, applying the vertical datum correction
        combined[is_water_body] = bathy_array[is_water_body]

        return combined
