"""
Base definitions for data generators in the flood simulator pipeline.

This module contains the abstract base class `DataGenerator`, which standardizes
the execution flow (generate, save, visualize) for all specific data layers.
"""

from abc import ABC, abstractmethod
from datetime import datetime
from pathlib import Path
from typing import Any, Dict

from loguru import logger

# Adjusted import to reflect the user's project structure
from misc.types import SpatialContext, VisualConfig


class DataGenerator(ABC):
    """
    Abstract base class for all layer generators in the pipeline.

    It enforces a standard structure for generating, saving, and optionally
    visualizing environmental data layers.

    Attributes:
        name (str): The unique identifier/name of the data layer.
        visual_config (VisualConfig): The configuration settings for visualization.
    """

    def __init__(self, name: str, visual_config: VisualConfig) -> None:
        """
        Initializes the DataGenerator.

        Args:
            name (str): The name of the layer (e.g., 'elevation', 'rainfall').
            visual_config (VisualConfig): Configuration for plotting the layer.
        """
        self.name = name
        self.visual_config = visual_config

    def run(
        self, 
        dependent_layers: Dict[str, Any], 
        start_date: datetime, 
        end_date: datetime, 
        cfg: Dict[str, Any], 
        cfg_dir: Path, 
        output_dir: Path, 
        visualization: bool
    ) -> Any:
        """
        Executes the full lifecycle of the layer generator.

        This method orchestrates the creation of the output directory, data 
        generation, file saving, and optional visualization.

        Args:
            dependent_layers (Dict[str, Any]): Data from previously executed layers.
            start_date (datetime): Simulation start time.
            end_date (datetime): Simulation end time.
            cfg (Dict[str, Any]): Specific configuration for this layer.
            cfg_dir (Path): The directory where the main config file resides.
            output_dir (Path): The base output directory for the pipeline.
            visualization (bool): Flag indicating if the layer should be plotted.

        Returns:
            Any: The generated data layer object (e.g., StaticRaster, DynamicRaster).
        """
        logger.info(f"Starting execution lifecycle for layer: '{self.name}'")
        
        # 1. Prepare directory structure
        layer_output_dir = output_dir / self.name
        layer_output_dir.mkdir(parents=True, exist_ok=True)
        logger.debug(f"Output directory ready at: {layer_output_dir}")
    
        # 2. Generate data
        logger.debug(f"[{self.name}] Generating data...")
        layer = self._generate(dependent_layers, start_date, end_date, cfg, cfg_dir)

        # 3. Save data
        logger.debug(f"[{self.name}] Saving data to disk...")
        self._save(layer_output_dir, layer)

        # 4. Visualize if required
        if visualization:
            logger.debug(f"[{self.name}] Generating visualization...")
            self._visualize(layer_output_dir, layer)

        logger.success(f"Layer '{self.name}' processed successfully.")
        return layer

    @abstractmethod
    def _generate(
        self, 
        dependent_layers: Dict[str, Any], 
        start_date: datetime, 
        end_date: datetime, 
        cfg: Dict[str, Any], 
        cfg_dir: Path
    ) -> Any:
        """
        Core logic to generate or load the specific data layer.

        Must be implemented by subclasses.

        Args:
            dependent_layers (Dict[str, Any]): Data from dependency layers.
            start_date (datetime): Simulation start time.
            end_date (datetime): Simulation end time.
            cfg (Dict[str, Any]): Layer-specific configuration parameters.
            cfg_dir (Path): Base directory of the configuration file.

        Returns:
            Any: The generated data object (e.g., StaticRaster).
        """
        pass

    @abstractmethod
    def _save(self, output_dir: Path, layer: Any) -> None:
        """
        Saves the generated layer data to disk.

        Must be implemented by subclasses.

        Args:
            output_dir (Path): The directory where files should be saved.
            layer (Any): The data layer object to save.
        """
        pass

    @abstractmethod
    def _visualize(self, output_dir: Path, layer: Any) -> None:
        """
        Creates and saves a visual representation (e.g., a plot) of the layer.

        Must be implemented by subclasses.

        Args:
            output_dir (Path): The directory to save the visualization image.
            layer (Any): The data layer object to visualize.
        """
        pass