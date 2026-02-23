"""
Elevation layer generator for the flood simulator pipeline.

This module defines the logic required to import and process the base digital 
elevation model (DEM) for the geographic area of study.
"""

from datetime import datetime
from pathlib import Path
from typing import Any, Dict

from data_io.static_io import StaticRasterIO
from generators.static_layer import StaticLayerGenerator
from misc.types import VisualConfig


class ElevationGenerator(StaticLayerGenerator):
    """
    Handles the initialization, loading, and setup of the elevation data layer.
    """

    def __init__(self) -> None:
        """
        Initializes the elevation layer generator with its specific visual configuration.
        """
        super().__init__("elevation", VisualConfig(cbar_unit="m", cmap="terrain"))

    def _generate(
        self, 
        dependent_layers: Dict[str, Any], 
        start_date: datetime, 
        end_date: datetime, 
        cfg: Dict[str, Any], 
        cfg_dir: Path
    ) -> Any:
        """
        Loads the elevation data from the specified configuration path.

        Args:
            dependent_layers (Dict[str, Any]): Dependencies (none expected for this base layer).
            start_date (datetime): Simulation start time.
            end_date (datetime): Simulation end time.
            cfg (Dict[str, Any]): Layer-specific configuration parameters.
            cfg_dir (Path): Base directory of the configuration file.

        Returns:
            StaticRaster: The loaded digital elevation model.
        """
        folder_path = (cfg_dir / Path(cfg['folder_path'])).resolve()
        return StaticRasterIO.load(folder_path, cfg['filename'])