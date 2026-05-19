"""
This module contains the core DataPipeline class for the flood simulator.

It manages the topological execution of various geographical and meteorological
data generators (topography, water depth, rainfall, etc.) ensuring that all
dependencies between layers are respected during the simulation data preparation.
"""

import graphlib
import yaml
import requests
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Set, Any, Tuple, Optional

from loguru import logger

from generators.base import DataGenerator
from generators.layers.topography import TopographyGenerator
from generators.layers.topo_bathy import TopoBathyGenerator
from generators.layers.water_depth import WaterDepthGenerator
from generators.layers.land_cover import LandCoverGenerator
from generators.layers.rainfall.rainfall import RainfallGenerator


class DataPipeline:
    """
    Manages the execution of data generation layers for the flood simulator.

    This pipeline handles the dependencies between different geographical and
    meteorological data layers, ensuring they are executed in the correct
    topological order.

    Attributes:
        generators (Dict[str, DataGenerator]): A registry of instantiated layer generators.
        dependencies (Dict[str, Set[str]]): An adjacency list representing layer dependencies.
        cfg_dir (Path): The directory containing the configuration file.
        cfg (Dict[str, Any]): The loaded configuration data.
        output_dir (Path): The directory where the generated data will be saved.
        start_date (datetime): The start date for the simulation data.
        end_date (datetime): The end date for the simulation data.
        layers_to_visualize (List[str]): A list of layer names that should be visualized.
    """

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
        self._add_layer(TopographyGenerator(save_layer = False))
        self._add_layer(LandCoverGenerator(save_layer = True), depends_on={"topography"})
        self._add_layer(TopoBathyGenerator(save_layer = True), depends_on={"topography", "land_cover"})
        self._add_layer(WaterDepthGenerator(save_layer = True), depends_on={"topography", "topo_bathy"})
        self._add_layer(RainfallGenerator(save_layer = True), depends_on={"topography"})

        self.cfg_dir: Path = cfg_path.parent
        self.cfg: Dict[str, Any] = self._load_config(cfg_path)

        # Output directory configuration
        output_base = Path(self.cfg.get("output_dir", "./output"))
        output_name = self.cfg.get("output_name", "default_output")
        self.output_dir: Path = (self.cfg_dir / output_base).resolve() / output_name

        logs_dir = self.output_dir / "logs"

        if not self.output_dir.exists():
            self.output_dir.mkdir(parents=True, exist_ok=True)
            logger.debug(f"Created output directory at: {self.output_dir}")

            logs_dir.mkdir(parents=True, exist_ok=True)

        # Configure log file
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
        layers_output: Dict[str, Any] = {}

        # 1. Retrieve execution order
        execution_order = self._get_execution_order()
        logger.info(f"Pipeline execution order resolved: {execution_order}")

        all_notices: Dict[str, Dict[str, str]] = {}
        
        # 2. Execute layer by layer
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
                layers_output[layer_name], all_notices[layer_name] = self.generators[layer_name].run(
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

        self._generate_legal_notices(all_notices)

        logger.success("Data Pipeline completed successfully!")

    def _generate_legal_notices(self, all_notices: Dict[str, Dict[str, str]]) -> None:
        """
        Generates the README.md (Dataset Card) and LICENSE files based on the legal notices of the 
        executed layers.

        Args:
            all_notices (Dict[str, Dict[str, str]]): A dictionary containing legal notices and attributions for each layer.
        """
        logger.info("Generating Dataset Card and LICENSE...")

        # 1. Base configuration for the overall dataset
        overall_license = "cc-by-4.0"

        # 2. Build README.md content
        readme_path = self.output_dir / "README.md"

        # YAML Frontmatter
        md_content = f"---\nlicense: {overall_license}\ntags:\n- simulation\n- geospatial\n---\n\n"

        # Title and Description
        md_content += "# Simulation Dataset\n\n"
        md_content += "This dataset was dynamically generated via an automated Python pipeline for use in a C++ flood simulation environment.\n\n"
        
        # Third-Party Notices Section
        md_content += "## Source Data and Third-Party Licenses\n\n"
        md_content += "This dataset is a derived work generated by processing and aggregating the following open data sources:\n\n"

        if not all_notices:
            md_content += "*No external third-party data sources with specific attribution requirements were used in this generation.*\n"
        else:
            for idx, (layer_name, notice) in enumerate(all_notices.items(), 1):
                md_content += f"**{idx}. {notice.get('name', 'Unknown Source')}**\n"
                md_content += f"* **Source:** {notice.get('source', 'N/A')}\n"
                md_content += f"* **License:** {notice.get('license', 'N/A')}\n"
                if 'attribution' in notice:
                    md_content += f"* **Attribution:** {notice['attribution']}\n"
                if 'processing' in notice:
                    md_content += f"* **Processing:** {notice['processing']}\n"
                md_content += "\n"

        # Write README.md
        with open(readme_path, "w", encoding="utf-8") as f:
            f.write(md_content)
        logger.debug(f"Created Dataset Card at: {readme_path}")

        # 3. Build LICENSE file
        license_path = self.output_dir / "LICENSE"
        
        with open(license_path, "w", encoding="utf-8") as f:
            f.write(self._fetch_license_text(overall_license))
        logger.debug(f"Created LICENSE file at: {license_path}")

    def _fetch_license_text(self, license_id: str) -> str:
        """
        Download the text directly from the official GitHub API

        Args:
            license_id (str): The license identifier (e.g., "cc-by-4.0").
        """
        api_url = f"https://api.github.com/licenses/{license_id}"

        logger.info(f"Downloading license '{license_id}' from the official choosealicense API...")
        
        try:
            # Standard header for best practices with the GitHub API
            headers = {"Accept": "application/vnd.github.v3+json"}
            
            response = requests.get(api_url, headers=headers, timeout=10)
            response.raise_for_status()
            
            # The ChooseALicense API returns a JSON object where the text is in the 'body'
            license_data = response.json()
            return license_data.get("body", "License text missing in API response.")
            
        except Exception as e:
            logger.warning(f"API request to choosealicense failed: {e}")
            return f"Dataset licensed under: {license_id}"
