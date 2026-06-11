"""
Water depth layer generator for the flood simulator pipeline.

This module defines the logic required to calculate the initial water depth.
"""

from datetime import datetime
from pathlib import Path
from typing import Any, Dict, Tuple

import numpy as np
from loguru import logger

from generators.static_layer import StaticLayerGenerator
from utils.types import StaticRaster, VisualConfig


class WaterDepthGenerator(StaticLayerGenerator):
    """Handles the initialization and generation of the water depth data layer.

    This generator calculates the initial standing water depth.
    """

    def __init__(self, save_layer: bool) -> None:
        """Initializes the water depth layer generator.

        Sets up the base `DataGenerator` properties, assigning the unique name 
        'water_depth' and defining the visual configuration (e.g., color map 
        and units) for subsequent plotting.

        Args:
            save_layer (bool): Flag determining whether the generated water 
                depth raster should be physically saved to disk.
        """
        super().__init__(
            name="water_depth", 
            save_layer=save_layer, 
            visual_config=VisualConfig(cbar_unit="m", cmap="Blues")
        )

    def _generate(
        self, 
        dependent_layers: Dict[str, Any], 
        start_date: datetime, 
        end_date: datetime, 
        cfg: Dict[str, Any], 
        cfg_dir: Path
    ) -> StaticRaster:
        """Generates the initial water depth layer.

        Args:
            dependent_layers (Dict[str, Any]): Dictionary of pre-calculated 
                dependencies. MUST contain 'topography' layer.
            start_date (datetime): Simulation start time (unused for static layers).
            end_date (datetime): Simulation end time (unused for static layers).
            cfg (Dict[str, Any]): Layer-specific configuration parameters.
            cfg_dir (Path): Base directory of the configuration file.

        Returns:
            StaticRaster: The generated provisional water depth model.

        Raises:
            KeyError: If 'topography' are missing from `dependent_layers`.
        """
        logger.info("Calculating initial water depth from elevation models...")

        # 1. Validate required dependencies
        required_layers = ['topography']
        for req in required_layers:
            if req not in dependent_layers:
                error_msg = f"WaterDepthGenerator requires '{req}' to be generated first."
                logger.error(error_msg)
                raise KeyError(error_msg)

        # 2. Extract data arrays
        topography_array = dependent_layers['topography'].data
    
        # 3. Calculate water depth
        water_depth = np.zeros_like(topography_array)

        logger.success("Water depth calculation completed successfully.")

        return StaticRaster(
            data=water_depth,
            spatial_context=dependent_layers['topography'].spatial_context
        )

    def _get_legal_notices(self, cfg: Dict[str, Any]) -> Dict[str, str]:
        """
        Retrieves legal notices and attributions for the water depth data sources.

        Args:
            cfg (Dict[str, Any]): Layer-specific configuration parameters.

        Returns:
            Dict[str, str]: A dictionary containing legal notices and attributions for the data sources used.
        """
        return {
            "name": "Initial Standing Water Depth",
            "source": "Internal Pipeline Generation",
            "license": "CC BY 4.0",
            "attribution": "Generated as part of the flood simulation pipeline.",
            "processing": "Set to zero for initial conditions; no external data sources directly used."
        }