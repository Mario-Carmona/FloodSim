"""
Water depth layer generator for the flood simulator pipeline.

This module defines the provisional logic required to generate the initial
water depth layer for the simulation area.
"""

from datetime import datetime
from pathlib import Path
from typing import Any, Dict

import numpy as np
from loguru import logger

from generators.static_layer import StaticLayerGenerator
from misc.types import StaticRaster, VisualConfig


class WaterDepthGenerator(StaticLayerGenerator):
    """
    Handles the initialization and generation of the water depth data layer.
    """

    def __init__(self) -> None:
        """
        Initializes the water depth layer generator with its specific visual configuration.
        """
        super().__init__("water_depth", VisualConfig(cbar_unit="m"))

    def _generate(
        self, 
        dependent_layers: Dict[str, Any], 
        start_date: datetime, 
        end_date: datetime, 
        cfg: Dict[str, Any], 
        cfg_dir: Path
    ) -> Any:
        """
        Generates the provisional water depth data layer based on elevation.

        Args:
            dependent_layers (Dict[str, Any]): Dependencies, requiring 'elevation'.
            start_date (datetime): Simulation start time.
            end_date (datetime): Simulation end time.
            cfg (Dict[str, Any]): Layer-specific configuration parameters.
            cfg_dir (Path): Base directory of the configuration file.

        Returns:
            StaticRaster: The generated provisional water depth model.
        """
        logger.debug("Generating provisional water depth data based on elevation bounds.")
        
        height = dependent_layers['elevation'].spatial_context.height
        width = dependent_layers['elevation'].spatial_context.width

        return StaticRaster(
            data=np.zeros((height, width), dtype=dependent_layers['elevation'].data.dtype),
            spatial_context=dependent_layers['elevation'].spatial_context
        )