"""
Rainfall spatiotemporal layer generator for the flood simulator pipeline.

This module is responsible for orchestrating the extraction of meteorological data,
filtering relevant data points based on time windows, and applying spatial interpolation 
(Kriging) to generate a complete 3D dynamic raster of precipitation over time.
"""

from datetime import datetime, timedelta
from pathlib import Path
from typing import Any, Dict

import numpy as np
from affine import Affine
from loguru import logger

from generators.dynamic_layer import DynamicLayerGenerator
from utils.types import StaticRaster, DynamicRaster, VisualConfig, SpatialContext
from generators.layers.rainfall.extraction.manager import RainDataManager
from generators.layers.rainfall.interpolation.kriging import KrigingInterpolator
from generators.layers.rainfall.types import RainDataPoint


class RainfallGenerator(DynamicLayerGenerator):
    """
    Handles the generation of dynamic precipitation maps over the simulation period.
    """

    def __init__(self, save_layer: bool) -> None:
        """Initializes the rainfall spatiotemporal layer generator.

        Sets up the base `DynamicLayerGenerator` properties, assigning the 
        unique identifier 'rainfall' to this layer. It also defines the visual 
        configuration tailored for precipitation data, using a standard 'jet' 
        colormap and millimeters per hour ('mm/h') as the colorbar unit for 
        subsequent frame-by-frame plotting.

        Args:
            save_layer (bool): Flag determining whether the generated dynamic 
                rainfall data cube (and its discrete frames) should be 
                physically saved to disk.
        """
        super().__init__(
            name="rainfall", 
            save_layer=save_layer, 
            visual_config=VisualConfig(cbar_unit="mm/h", cmap="jet")
        )

    def _downsample_dem(self, layer: StaticRaster, factor: int) -> StaticRaster:
        """
        Reduces the spatial resolution of a Digital Elevation Model (DEM) by averaging pixel blocks.
        
        This is particularly useful for accelerating spatial interpolation algorithms (like Kriging)
        which scale poorly with very large grids.

        Args:
            layer (StaticRaster): The original high-resolution elevation raster.
            factor (int): The reduction factor (e.g., 2 reduces resolution by half).

        Returns:
            StaticRaster: A new raster with the downsampled data and updated spatial context.
        """
        logger.debug(f"Downsampling DEM with a factor of {factor}x")
        
        # 1. Resize the matrix (Block Averaging)
        h, w = layer.data.shape
        new_h, new_w = h // factor, w // factor
    
        # Trim edges to ensure dimensions are perfectly divisible by the factor
        data_trimmed = layer.data[:new_h * factor, :new_w * factor]
    
        # Reshape into a 4D array and calculate the mean over the blocks (axes 1 and 3)
        data_downsampled = data_trimmed.reshape(new_h, factor, new_w, factor).mean(axis=(1, 3))

        # 2. Update Geographic Metadata
        pixel_size_x = layer.spatial_context.transform.a * factor
        pixel_size_y = layer.spatial_context.transform.e * factor

        # Keep the top-left coordinate (c, f) fixed, update pixel scales
        new_transform = Affine(
            pixel_size_x, layer.spatial_context.transform.b, layer.spatial_context.transform.c,
            layer.spatial_context.transform.d, pixel_size_y, layer.spatial_context.transform.f
        )

        new_spatial_context = SpatialContext(
            crs=layer.spatial_context.crs,
            transform=new_transform,
            width=new_w,
            height=new_h
        )

        return StaticRaster(data=data_downsampled, spatial_context=new_spatial_context)

    def _generate(
        self, 
        dependent_layers: Dict[str, Any], 
        start_date: datetime, 
        end_date: datetime, 
        cfg: Dict[str, Any], 
        cfg_dir: Path
    ) -> DynamicRaster:
        """
        Executes the main pipeline for generating rainfall data.
        
        The process follows these steps:
        1.  Extracts raw meteorological data using `RainDataManager`.
        2.  Optionally downsamples the base elevation DEM to optimize computation.
        3.  Initializes a 3D spatiotemporal array (Time x Height x Width).
        4.  Iterates through the simulation timeline step-by-step.
        5.  Filters active rain gauges for the current time window.
        6.  Applies spatial interpolation (Kriging) to estimate rainfall across the entire grid.

        Args:
            dependent_layers (Dict[str, Any]): Must contain the base 'topography' StaticRaster.
            start_date (datetime): Simulation start boundary.
            end_date (datetime): Simulation end boundary.
            cfg (Dict[str, Any]): Specific settings for interpolation and time steps.
            cfg_dir (Path): Base directory for resolving external data paths.

        Returns:
            DynamicRaster: The resulting 3D data cube of rainfall intensity over time.
        """
        # --- 1. Data Extraction ---
        logger.info("Extracting meteorological rain data from sources...")
        manager = RainDataManager()
        rain_data = manager.fetch_all(
            dependent_layers['topography'].spatial_context.bounds, 
            start_date, 
            end_date, 
            dependent_layers['topography'].spatial_context.crs
        )

        # --- 2. DEM Optimization ---
        downgrade_factor = cfg.get('geography', {}).get('downgrade_factor', 1)
        if downgrade_factor > 1:
            dem = self._downsample_dem(dependent_layers['topography'], downgrade_factor)
        else:
            dem = dependent_layers['topography']

        # --- 3. Simulation Setup ---
        # Parse the simulation time step configured in the YAML (e.g., "01:00:00")
        dt_config = datetime.strptime(cfg['simulation']['time_step'], "%H:%M:%S")
        time_step = timedelta(hours=dt_config.hour, minutes=dt_config.minute, seconds=dt_config.second)

        # Calculate total required frames based on duration and step size
        total_duration = end_date - start_date
        num_steps = int(total_duration.total_seconds() // time_step.total_seconds())
        
        logger.debug(f"Simulation configured for {num_steps} steps with dt={time_step}")

        layer = DynamicRaster(
            data=np.zeros(
                (num_steps, dem.spatial_context.height, dem.spatial_context.width), 
                dtype='float32'
            ),
            timestamps=[],
            downgrade_factor=downgrade_factor,
            spatial_context=dem.spatial_context
        )

        interpolator = KrigingInterpolator(cfg['interpolation']['variogram_model'])

        # --- 4. Simulation Loop & Interpolation ---
        logger.info(f"Starting temporal interpolation loop ({num_steps} frames)...")

        for step in range(num_steps):
            step_start_time = start_date + (time_step * step)
            step_end_time = start_date + (time_step * (step + 1))

            layer.timestamps.append(step_start_time)

            # Filter gauges that have active readings strictly within this time window
            active_points = []
            for p in rain_data:
                if (p.start_timestamp <= step_start_time < p.end_timestamp) and (p.start_timestamp < step_end_time <= p.end_timestamp):
                    active_points.append(
                        RainDataPoint(position=p.station.position, value=p.mm_per_hour)
                    )

            # Perform Kriging if we have enough statistical points
            min_points = cfg['interpolation'].get('min_points_required', 3)
            if len(active_points) >= min_points:
                layer.data[step, :, :], _ = interpolator.process(dem, active_points)

                step_min = np.min(layer.data[step, :, :])
                step_max = np.max(layer.data[step, :, :])
                logger.info(f"Frame [{step:03d}] {step_start_time} | Max Intensity: {step_max:.4f} mm/h | Interpolated from {len(active_points)} points")
            else:
                logger.warning(f"Frame [{step:03d}] {step_start_time} | Not enough active stations ({len(active_points)} < {min_points}). Defaulting to 0 mm/h.")

        layer.timestamps.append(start_date + (time_step * num_steps))

        return layer

    def _get_legal_notices(self, cfg: Dict[str, Any]) -> Dict[str, str]:
        """
        Retrieves legal notices and attributions for the rainfall data sources.

        Args:
            cfg (Dict[str, Any]): Layer-specific configuration parameters.

        Returns:
            Dict[str, str]: A dictionary containing legal notices and attributions for the data sources used.
        """
        return {
            "name": "Rainfall Data (SAIH)",
            "source": "Confederación Hidrográfica del Júcar (CHJ) / Ministerio para la Transición Ecológica",
            "license": "CC BY 4.0 / Condiciones de Reutilización (Ley 37/2007)",
            "attribution": "Source of the data: Sistema Automático de Información Hidrológica (SAIH) - Confederación Hidrográfica del Júcar.",
            "processing": "The raw rain gauge data was extracted, filtered by time window, and spatially interpolated using Kriging to generate a dynamic spatiotemporal precipitation raster. The statistical meaning of the original meteorological data has not been distorted."
        }