"""
Rainfall Interpolation Module (Ordinary Kriging).

This module provides the `KrigingInterpolator` class, which is responsible for 
spatially interpolating point rainfall data across a geographic grid. 
It uses Ordinary Kriging, interpolating strictly based on the distance and 
spatial correlation between rain gauges.
"""

import numpy as np
import gstools as gs
from loguru import logger
from typing import List, Tuple

from utils.types import StaticRaster
from generators.layers.rainfall.types import RainDataPoint

class KrigingInterpolator:
    """
    Performs Ordinary Kriging.
    
    This interpolator estimates rainfall across a grid using only the known 
    values from weather stations, assuming a stationary local mean.
    
    Attributes:
        model_name (str): The name of the variogram model (e.g., 'Spherical', 'Gaussian').
    """
    
    def __init__(self, variogram_model: str = "Spherical") -> None:
        """
        Initializes the KrigingInterpolator with a specific variogram model.
        
        Args:
            variogram_model (str, optional): The mathematical model for the variogram. 
                Defaults to "Spherical".
        """
        self.model_name = variogram_model.capitalize()

    def process(self, dem: StaticRaster, rain_data: List[RainDataPoint]) -> Tuple[np.ndarray, np.ndarray]:
        """
        Interpolates rainfall data over the grid using Ordinary Kriging.

        Args:
            dem (StaticRaster): Used exclusively to obtain the target grid coordinates.
            rain_data (List[RainDataPoint]): A list of point measurements containing coordinates and rainfall values.

        Returns:
            Tuple[np.ndarray, np.ndarray]: A tuple containing:
                1. final_grid (np.ndarray): The interpolated 2D rainfall grid.
                2. var_grid (np.ndarray): The 2D variance grid of the interpolation.
        """
        logger.info(f"Starting Ordinary Kriging interpolation ({self.model_name}) with {len(rain_data)} active points.")

        # 1. Extract Vectors
        x_pts = np.array([p.position.x for p in rain_data])
        y_pts = np.array([p.position.y for p in rain_data])
        z_rain = np.array([p.value for p in rain_data])
        
        # 2. Variogram Estimation
        bin_center, gamma = gs.vario_estimate((x_pts, y_pts), z_rain)
        
        try:
            model = getattr(gs, self.model_name)(dim=2)
        except AttributeError:
            logger.warning(f"Variogram model '{self.model_name}' not found in gstools. Falling back to Spherical.")
            model = gs.Spherical(dim=2)

        model.fit_variogram(bin_center, gamma, nugget=True)
        logger.debug(f"Fitted variogram model parameters: {model}")

        # 3. Ordinary Kriging on Rainfall Data
        # We ensure that the coordinates are sorted in ascending order for gstools
        # (gstools prefers strictly ascending x and y axes)
        y_coords_sorted = np.sort(dem.y_coords) 
        x_coords_sorted = dem.x_coords

        krige = gs.krige.Ordinary(
            model=model,
            cond_pos=(x_pts, y_pts),
            cond_val=z_rain
        )
        
        # GSTools returns (cols, rows), we transpose to match our (rows, cols) convention
        logger.debug("Executing Kriging on structured grid...")
        rain_grid_T, var_grid_T = krige.structured([x_coords_sorted, y_coords_sorted])
        rain_grid = rain_grid_T.T
        var_grid = var_grid_T.T

        # SINCE THE DEM HAS ROW=0 IN THE NORTH (MAX Y):
        # We need to vertically invert the kriging result to match it
        rain_grid = np.flipud(rain_grid)
        var_grid = np.flipud(var_grid)

        # 4. Clamp Negative Rainfall to 0
        final_grid = np.clip(rain_grid, a_min=0.0, a_max=None)

        logger.info("Kriging interpolation completed successfully.")
        return final_grid, var_grid