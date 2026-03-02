"""
This module contains the core DataPipeline class for the flood simulator.

It manages the topological execution of various geographical and meteorological
data generators (elevation, water depth, rainfall, etc.) ensuring that all
dependencies between layers are respected during the simulation data preparation.
"""

import graphlib
import yaml
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Set, Any, Optional

from loguru import logger

from generators.base import DataGenerator
from generators.layers.elevation.elevation import ElevationGenerator
from generators.layers.water_depth import WaterDepthGenerator
from generators.layers.roughness import RoughnessGenerator
from generators.layers.rainfall.rainfall import RainfallGenerator


class DataPipeline:
    """
    Manages the execution of data generation layers for the flood simulator.

    This pipeline handles the dependencies between different geographical and
    meteorological data layers, ensuring they are executed in the correct
    topological order.

    Attributes:
        MAIN_LAYER_NAME (str): The core layer that most other layers depend on.
        generators (Dict[str, DataGenerator]): A registry of instantiated layer generators.
        dependencies (Dict[str, Set[str]]): An adjacency list representing layer dependencies.
        cfg_dir (Path): The directory containing the configuration file.
        cfg (Dict[str, Any]): The loaded configuration data.
        output_dir (Path): The directory where the generated data will be saved.
        start_date (datetime): The start date for the simulation data.
        end_date (datetime): The end date for the simulation data.
        layers_to_visualize (List[str]): A list of layer names that should be visualized.
    """
    
    MAIN_LAYER_NAME: str = "elevation"

    def __init__(self, cfg_path: Path) -> None:
        """
        Initializes the data pipeline and configures the required layers.

        Args:
            cfg_path (Path): The absolute or relative path to the YAML config file.

        Raises:
            FileExistsError: If the designated output directory already exists.
        """
        self.generators: Dict[str, DataGenerator] = {}
        self.dependencies: Dict[str, Set[str]] = {}

        # Register available layers
        self._add_layer(ElevationGenerator())
        self._add_layer(WaterDepthGenerator())
        self._add_layer(RoughnessGenerator())
        self._add_layer(RainfallGenerator(), depends_on={"elevation"})

        self.cfg_dir: Path = cfg_path.parent
        self.cfg: Dict[str, Any] = self._load_config(cfg_path)

        # Output directory configuration
        output_base = Path(self.cfg.get("output_dir", "./output"))
        output_name = self.cfg.get("output_name", "default_output")
        self.output_dir: Path = (self.cfg_dir / output_base).resolve() / output_name

        if self.output_dir.exists():
            logger.error(f"Output folder already exists: {self.output_dir}")
            raise FileExistsError(f"Output folder already exists: {self.output_dir}")

        self.output_dir.mkdir(parents=True, exist_ok=True)
        logger.debug(f"Created output directory at: {self.output_dir}")

        # Configure log file
        logs_dir = self.output_dir / "logs"
        logs_dir.mkdir(parents=True, exist_ok=True)
        logger.add(
            logs_dir / "log.txt",
            format="{time:YYYY-MM-DD HH:mm:ss} | {level: <8} | {name}:{line} - {message}",
            level="TRACE", 
            rotation="10 MB"
        )

        # Date parsing
        date_format = "%Y-%m-%d %H:%M:%S"
        self.start_date: datetime = datetime.strptime(self.cfg['start_date'], date_format)
        self.end_date: datetime   = datetime.strptime(self.cfg['end_date'], date_format)

        # Visualization setup
        visualize_cfg = self.cfg.get('visualize', {})
        if visualize_cfg.get('enable', False):
            self.layers_to_visualize: List[str] = visualize_cfg.get('layers', [])
        else:
            self.layers_to_visualize: List[str] = []

    def _load_config(self, path: Path) -> Dict[str, Any]:
        """
        Loads and parses the YAML configuration file.

        Args:
            path (Path): The path to the YAML file.

        Returns:
            Dict[str, Any]: The parsed configuration dictionary.
        """
        logger.debug(f"Loading configuration from {path}")
        with open(path, 'r', encoding='utf-8') as f:
            return yaml.safe_load(f)

    def _add_layer(self, generator: DataGenerator, depends_on: Optional[Set[str]] = None):
        """
        Registers a layer generator in the pipeline and defines its dependencies.

        Args:
            generator (DataGenerator): The generator instance for the layer.
            depends_on (Optional[Set[str]]): A set of layer names this layer depends on. 
                Defaults to None.
        """
        if depends_on is None:
            depends_on = set()

        layer_name = generator.name
        self.generators[layer_name] = generator
        
        # Initialize and update dependencies
        self.dependencies[layer_name] = set()
        self.dependencies[layer_name].update(depends_on)

        # Implicit dependency on the main layer
        if layer_name != self.MAIN_LAYER_NAME:
            self.dependencies[layer_name].add(self.MAIN_LAYER_NAME)

    def _get_execution_order(self) -> List[str]:
        """
        Calculates the exact execution order of the layers using Topological Sorting.

        Returns:
            List[str]: An ordered list of layer names to execute safely.

        Raises:
            ValueError: If a circular dependency is detected between layers.
        """
        try:
            sorter = graphlib.TopologicalSorter(self.dependencies)
            return list(sorter.static_order())
        except graphlib.CycleError as e:
            logger.error(f"Circular dependency detected: {e}")
            raise ValueError(f"Circular dependency detected in layer definitions: {e}")

    def run(self) -> None:
        """
        Executes all registered layers in the resolved topological order.

        Raises:
            ValueError: If the main layer is missing from the pipeline.
        """
        # 1. Security Check
        if self.MAIN_LAYER_NAME not in self.generators:
            logger.critical(f"Missing core layer '{self.MAIN_LAYER_NAME}' in pipeline.")
            raise ValueError(f"Missing core layer '{self.MAIN_LAYER_NAME}' in the pipeline.")

        layers_output: Dict[str, Any] = {}

        # 2. Retrieve execution order
        execution_order = self._get_execution_order()
        logger.info(f"Pipeline execution order resolved: {execution_order}")
        
        # 3. Execute layer by layer
        for layer_name in execution_order:
            logger.info(f"Processing layer: {layer_name}")

            # Extract only the required dependencies for the current layer
            dependent_layers = {
                k: layers_output[k] 
                for k in self.dependencies[layer_name] 
                if k in layers_output
            }

            visualization = layer_name in self.layers_to_visualize
            layer_config = self.cfg.get('layers', {}).get(layer_name, {})

            try:
                layers_output[layer_name] = self.generators[layer_name].run(
                    dependent_layers, 
                    self.start_date, 
                    self.end_date, 
                    layer_config, 
                    self.cfg_dir,
                    self.output_dir,
                    visualization
                )
                logger.debug(f"Layer '{layer_name}' completed successfully.")
            except Exception as e:
                logger.error(f"Failed to process layer '{layer_name}': {e}")
                raise

        logger.success("Data Pipeline completed successfully!")