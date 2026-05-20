"""
Topography layer generator for the flood simulator pipeline.

This module defines the logic required to import and process the base digital 
elevation model (DEM) for the geographic area of study. It acts as a bridge 
between the simulation pipeline and the IGN (Instituto Geográfico Nacional) client.
"""

from datetime import datetime
from pathlib import Path
from typing import Any, Dict, Tuple

from pyproj import CRS

from downloaders.ign_client import IgnClient
from generators.static_layer import StaticLayerGenerator
from utils.types import Bounds, StaticRaster, VisualConfig


class TopographyGenerator(StaticLayerGenerator):
    """Handles the initialization, loading, and setup of the topography data layer.

    This generator extracts the bounding box and resolution parameters from the 
    global configuration and delegates the actual data fetching and merging 
    processes to the `IgnClient`. The resulting digital elevation model is 
    then returned as a standard static raster.
    """

    def __init__(self, save_layer: bool) -> None:
        """Initializes the topography layer generator.

        Sets up the base `DataGenerator` properties, assigning the unique name 
        'topography' and defining the visual configuration (e.g., color map and 
        colorbar units) for subsequent plotting.

        Args:
            save_layer (bool): Flag determining whether the generated topography 
                raster should be physically saved to disk.
        """
        super().__init__("topography", save_layer, VisualConfig(cbar_unit="m", cmap="terrain"))

    def _generate(
        self, 
        dependent_layers: Dict[str, Any], 
        start_date: datetime, 
        end_date: datetime, 
        cfg: Dict[str, Any], 
        cfg_dir: Path
    ) -> StaticRaster:
        """Generates the topography layer by fetching DEM data from the IGN WCS.

        Parses the specific configuration dictionary to extract the target 
        resolution, coordinate reference systems, and spatial boundaries. It 
        then requests the corresponding elevation data.

        Args:
            dependent_layers (Dict[str, Any]): Dictionary of pre-calculated 
                dependencies (not used for this base layer).
            start_date (datetime): Simulation start date (unused for static layers).
            end_date (datetime): Simulation end date (unused for static layers).
            cfg (Dict[str, Any]): Specific layer settings. Must contain 'resolution', 
                'target_crs', and 'bounding_box' definitions.
            cfg_dir (Path): Base directory for the configuration file.

        Returns:
            StaticRaster: The downloaded, merged, and instantiated topography 
                raster matrix with its spatial context.

        Raises:
            KeyError: If the required configuration fields ('resolution', 
                'target_crs', 'bounding_box') are missing from `cfg`.
            IgnWcsError: If the IGN client fails to download the elevation tiles.
        """
        resolution: int = cfg['resolution']
        target_crs = CRS.from_epsg(cfg['target_crs'])

        bbox_cfg = cfg['bounding_box']

        # Construct the bounding box object using the configuration coordinates
        bbox = Bounds(
            min_x=bbox_cfg['south_west']['lon'],
            max_x=bbox_cfg['north_east']['lon'],
            min_y=bbox_cfg['south_west']['lat'],
            max_y=bbox_cfg['north_east']['lat']
        )

        bbox_crs = CRS.from_epsg(bbox_cfg['crs'])

        # Delegate the tile downloading and merging to the IGN client
        return IgnClient.fetch_mdt(
            resolution=resolution, 
            target_crs=target_crs, 
            bbox_crs=bbox_crs, 
            bbox=bbox
        )

    def _get_legal_notices(self, cfg: Dict[str, Any]) -> Dict[str, str]:
        """
        Retrieves legal notices and attributions for the topography data sources.

        Args:
            cfg (Dict[str, Any]): The configuration dictionary containing the resolution.

        Returns:
            Dict[str, str]: A dictionary containing legal notices and attributions for the data sources used.
        """
        return IgnClient.get_mdt_legal_notices(cfg['resolution'])