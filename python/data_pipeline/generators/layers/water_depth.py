"""
Water depth layer generator for the flood simulator pipeline.

This module defines the logic required to calculate the initial water depth 
layer by subtracting the combined topo-bathymetry elevation from the 
surface topography.
"""

from datetime import datetime
from pathlib import Path
from typing import Any, Dict

import numpy as np
from loguru import logger

from generators.static_layer import StaticLayerGenerator
from utils.types import StaticRaster, VisualConfig


class WaterDepthGenerator(StaticLayerGenerator):
    """Handles the initialization and generation of the water depth data layer.

    This generator calculates the initial standing water depth by comparing 
    the base digital elevation model (topography) with the merged 
    topography-bathymetry model (topo_bathy).
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
    ) -> Any:
        """Generates the initial water depth layer based on elevation differences.

        Args:
            dependent_layers (Dict[str, Any]): Dictionary of pre-calculated 
                dependencies. MUST contain 'topography' and 'topo_bathy' layers.
            start_date (datetime): Simulation start time (unused for static layers).
            end_date (datetime): Simulation end time (unused for static layers).
            cfg (Dict[str, Any]): Layer-specific configuration parameters.
            cfg_dir (Path): Base directory of the configuration file.

        Returns:
            StaticRaster: The generated provisional water depth model.

        Raises:
            KeyError: If 'topography' or 'topo_bathy' are missing from `dependent_layers`.
        """
        logger.info("Calculating initial water depth from elevation models...")

        # 1. Validate required dependencies
        required_layers = ['topography', 'topo_bathy']
        for req in required_layers:
            if req not in dependent_layers:
                error_msg = f"WaterDepthGenerator requires '{req}' to be generated first."
                logger.error(error_msg)
                raise KeyError(error_msg)

        # 2. Extract data arrays
        topography_array = dependent_layers['topography'].data
        topo_bathy_array = dependent_layers['topo_bathy'].data
    
        # 3. Calculate water depth: Surface (topography) - Bottom (topo_bathy)
        water_depth = topography_array - topo_bathy_array
    
        # 4. Safety firewall: Clip negative values to zero
        # This prevents negative depths caused by satellite noise or 
        # minor vertical misalignments over solid ground.
        clean_water_depth = np.maximum(0.0, water_depth)

        logger.success("Water depth calculation completed successfully.")

        return StaticRaster(
            data=clean_water_depth,
            spatial_context=dependent_layers['topography'].spatial_context
        )