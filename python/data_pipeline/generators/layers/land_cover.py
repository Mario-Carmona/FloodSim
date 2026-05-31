"""
Land cover layer generator for the flood simulator pipeline.

This module defines the logic required to generate the land cover (SIOSE) data 
for the geographic area of study. It acts as an orchestrator, ensuring that 
the land cover raster is perfectly aligned with the base elevation model by 
sharing its exact spatial context.
"""

from datetime import datetime
from pathlib import Path
from typing import Any, Dict, Tuple

from loguru import logger

from downloaders.ign_client import IgnClient
from generators.static_layer import StaticLayerGenerator
from utils.types import StaticRaster, VisualConfig


class LandCoverGenerator(StaticLayerGenerator):
    """Handles the initialization and generation of the land cover data layer.

    This generator extracts the spatial context (bounding box, resolution, and CRS) 
    from the already processed topography layer. It then delegates the parallel WFS 
    data fetching, geometry parsing, and vector-to-raster conversion directly 
    to the `IgnClient`.
    """

    def __init__(self, save_layer: bool) -> None:
        """Initializes the land cover layer generator.

        Sets up the base `DataGenerator` properties, assigning the unique name 
        'land_cover' and defining the visual configuration for subsequent plotting.

        Args:
            save_layer (bool): Flag determining whether the generated land cover 
                raster should be physically saved to disk.
        """
        super().__init__("land_cover", save_layer, VisualConfig(cbar_unit="cover type", cmap="nipy_spectral"))

    def _generate(
        self, 
        dependent_layers: Dict[str, Any], 
        start_date: datetime, 
        end_date: datetime, 
        cfg: Dict[str, Any], 
        cfg_dir: Path
    ) -> StaticRaster:
        """Generates the land cover layer perfectly aligned with the elevation DEM.

        Args:
            dependent_layers (Dict[str, Any]): Dictionary of pre-calculated 
                dependencies. MUST contain the 'topography' layer to extract 
                the target spatial context.
            start_date (datetime): Simulation start time (unused for static layers).
            end_date (datetime): Simulation end time (unused for static layers).
            cfg (Dict[str, Any]): Layer-specific configuration parameters.
            cfg_dir (Path): Base directory of the configuration file.

        Returns:
            StaticRaster: The rasterized land cover matrix and its spatial context.

        Raises:
            KeyError: If the 'topography' layer is not found in `dependent_layers`.
            IgnWfsError: If the underlying IGN client fails to fetch the vector data.
        """
        logger.info("Starting land cover data generation via IGN WFS...")

        # 1. Extract the spatial context from the elevation layer to ensure 
        # perfect pixel-to-pixel alignment between both matrices.
        if 'topography' not in dependent_layers:
            error_msg = "LandCoverGenerator requires 'topography' to be generated first."
            logger.error(error_msg)
            raise KeyError(error_msg)

        ctx = dependent_layers['topography'].spatial_context

        # 2. Delegate the fetching and rasterization process to the client
        return IgnClient.fetch_land_cover(ctx)

    def _get_legal_notices(self, cfg: Dict[str, Any]) -> Dict[str, str]:
        """
        Retrieves legal notices and attributions for the land cover data sources.

        Args:
            cfg (Dict[str, Any]): Layer-specific configuration parameters.

        Returns:
            Dict[str, str]: A dictionary containing legal notices and attributions for the data sources used.
        """
        return IgnClient.get_land_cover_legal_notices()