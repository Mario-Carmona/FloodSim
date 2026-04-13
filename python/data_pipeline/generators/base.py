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

from utils.types import VisualConfig


class DataGenerator(ABC):
    """
    Abstract base class for all layer generators in the pipeline.

    It enforces a standard structure for generating, saving, and optionally
    visualizing environmental data layers.

    Attributes:
        _name (str): The unique identifier/name of the data layer.
        _save_layer (bool): Flag determining whether the generated data layer should be physically saved to disk.
        _visual_config (VisualConfig): The configuration settings for visualization.
    """

    def __init__(self, name: str, save_layer: bool, visual_config: VisualConfig) -> None:
        """
        Initializes the DataGenerator.

        Args:
            name (str): The name of the layer (e.g., 'topography', 'rainfall').
            save_layer (bool): A boolean flag indicating if the generated data layer should be persisted to disk.
            visual_config (VisualConfig): Configuration for plotting the layer.
        """
        self._name = name
        self._save_layer = save_layer
        self._visual_config = visual_config

    @property
    def name(self) -> str:
        """Gets the unique identifier or name of the data layer.

        Returns:
            str: The name of the layer (e.g., 'topography', 'rainfall').
        """
        return self._name

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
        logger.info(f"Starting execution lifecycle for layer: '{self._name}'")
    
        layer_output_dir = output_dir / self._name

        if layer_output_dir.exists():
            # Read data
            layer = self._read(layer_output_dir)
        else:
            # Generate data
            logger.debug(f"[{self._name}] Generating data...")
            layer = self._generate(dependent_layers, start_date, end_date, cfg, cfg_dir)

            # Save data
            if self._save_layer:
                # Prepare directory structure
                layer_output_dir.mkdir(parents=True, exist_ok=True)
                logger.debug(f"Output directory ready at: {layer_output_dir}")

                logger.debug(f"[{self._name}] Saving data to disk...")
                self._save(layer_output_dir, layer)

        # Visualize if required
        if visualization:
            logger.debug(f"[{self._name}] Generating visualization...")
            self._visualize(layer_output_dir, layer)

        logger.success(f"Layer '{self._name}' processed successfully.")
        return layer

    @abstractmethod
    def _read(self, output_dir: Path) -> Any:
        """Loads previously generated layer data from disk.

        This method is called if the output directory for the layer already 
        exists. It bypasses the generation step and loads the cached data 
        directly into memory. 

        Must be implemented by subclasses.

        Args:
            output_dir (Path): The directory path where the layer's data 
                files are stored.

        Returns:
            Any: The loaded data layer object (e.g., StaticRaster or DynamicRaster).
        """
        pass

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