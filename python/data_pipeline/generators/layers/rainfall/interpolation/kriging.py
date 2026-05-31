"""
Rainfall Interpolation Module (Kriging with External Drift).

This module provides the `KrigingInterpolator` class, which is responsible for 
spatially interpolating point rainfall data across a geographic grid. 
It uses Universal Kriging (specifically, Kriging with External Drift - KED), 
leveraging a Digital Elevation Model (DEM) as a secondary variable to account 
for the orographic effect on precipitation.

The process involves:
1. Extracting elevation at known station points.
2. Calculating a linear trend between elevation and rainfall.
3. Fitting a variogram to the residuals (actual rain - trend rain).
4. Performing Ordinary Kriging on the residuals.
5. Re-adding the elevation trend to the entire interpolated grid.
"""

import numpy as np
import gstools as gs
from loguru import logger
from typing import List, Tuple

from utils.types import StaticRaster
from generators.layers.rainfall.types import RainDataPoint

class KrigingInterpolator:
    """
    Performs Universal Kriging (Kriging with External Drift).
    
    This interpolator uses elevation (DEM) as a secondary variable (drift) to 
    guide the rainfall interpolation. It calculates a linear trend between 
    elevation and rainfall, performs Ordinary Kriging on the residuals, 
    and then adds the spatial trend back to the final grid.
    
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
        Interpolates rainfall data over the provided DEM grid using Kriging with External Drift.

        Args:
            dem (StaticRaster): The Digital Elevation Model used as the secondary drift variable.
            rain_data (List[RainDataPoint]): A list of point measurements containing coordinates and rainfall values.

        Returns:
            Tuple[np.ndarray, np.ndarray]: A tuple containing:
                1. final_grid (np.ndarray): The interpolated 2D rainfall grid.
                2. var_grid (np.ndarray): The 2D variance grid of the interpolation.
        """
        logger.info(f"Starting Kriging interpolation ({self.model_name}) with {len(rain_data)} active points.")

        # 1. Extract Vectors
        x_pts = np.array([p.position.x for p in rain_data])
        y_pts = np.array([p.position.y for p in rain_data])
        z_rain = np.array([p.value for p in rain_data])
        
        # Sample elevation at station locations for drift calculation
        z_elev = np.array([dem.get_data_value(p.position.x, p.position.y) for p in rain_data])

        # 2. Calculate Trend (Linear Drift): Rain = m * Elev + c
        trend_poly = np.polyfit(z_elev, z_rain, 1)
        trend_func = np.poly1d(trend_poly)
        logger.debug(f"Calculated elevation-rainfall trend: slope(m)={trend_poly[0]:.4e}, intercept(c)={trend_poly[1]:.4f}")
        
        # 3. Calculate Residuals (Data - Trend)
        residuals = z_rain - trend_func(z_elev)

        # 4. Variogram Estimation
        bin_center, gamma = gs.vario_estimate((x_pts, y_pts), residuals)
        
        try:
            model = getattr(gs, self.model_name)(dim=2)
        except AttributeError:
            logger.warning(f"Variogram model '{self.model_name}' not found in gstools. Falling back to Spherical.")
            model = gs.Spherical(dim=2)

        model.fit_variogram(bin_center, gamma, nugget=True)
        logger.debug(f"Fitted variogram model parameters: {model}")

        # 5. Ordinary Kriging on Residuals
        # We ensure that the coordinates are sorted in ascending order for gstools
        # (gstools prefers strictly ascending x and y axes)
        y_coords_sorted = np.sort(dem.y_coords) 
        x_coords_sorted = dem.x_coords

        krige = gs.krige.Ordinary(
            model=model,
            cond_pos=(x_pts, y_pts),
            cond_val=residuals
        )
        
        # GSTools returns (cols, rows), we transpose to match our (rows, cols) convention
        logger.debug("Executing Kriging on structured grid...")
        res_grid_T, var_grid_T = krige.structured([x_coords_sorted, y_coords_sorted])
        res_grid = res_grid_T.T
        var_grid = var_grid_T.T

        # SINCE YOUR DEM HAS ROW=0 IN THE NORTH (MAX Y):
        # We need to vertically invert the kriging result to match it
        res_grid = np.flipud(res_grid)
        var_grid = np.flipud(var_grid)

        # 6. Add Trend Back
        final_grid = res_grid + trend_func(dem.data)

        # 7. Clamp Negative Rainfall to 0 (Physical constraint)
        final_grid = np.clip(final_grid, a_min=0.0, a_max=None)

        logger.info("Kriging interpolation completed successfully.")
        return final_grid, var_grid