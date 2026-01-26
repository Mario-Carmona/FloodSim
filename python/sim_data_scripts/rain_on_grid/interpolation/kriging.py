import numpy as np
import gstools as gs
from typing import List, Tuple

from common_scripts.misc.types import RasterGrid

from sim_data_scripts.rain_on_grid.misc.types import RainDataPoint

class KrigingInterpolator:
    """
    Performs Universal Kriging (Kriging with External Drift).
    Uses elevation (DEM) as the secondary variable to guide rainfall interpolation.
    """
    
    def __init__(self, variogram_model: str = "Spherical"):
        self.model_name = variogram_model.capitalize()

    def process(self, dem: RasterGrid, rain_data: List[RainDataPoint]) -> Tuple[np.ndarray, np.ndarray]:
        """
         interpolates rainfall data over the DEM grid.

        Returns:
            Tuple containing:
            1. Interpolated rainfall grid (2D).
            2. Variance grid (2D).
        """
        # 1. Extract Vectors
        x_pts = np.array([p.x for p in rain_data])
        y_pts = np.array([p.y for p in rain_data])
        z_rain = np.array([p.value for p in rain_data])
        
        # Sample elevation at station locations for drift calculation
        z_elev = np.array([dem.get_elevation_at_point(p.x, p.y) for p in rain_data])

        # 2. Calculate Trend (Linear Drift): Rain = m * Elev + c
        # Degree 1 polyfit
        slope, intercept = np.polyfit(z_elev, z_rain, 1)
        
        trend_func = lambda h: slope * h + intercept
        
        # 3. Calculate Residuals (Data - Trend)
        residuals = z_rain - trend_func(z_elev)

        # 4. Variogram Estimation
        bin_center, gamma = gs.vario_estimate((x_pts, y_pts), residuals)
        
        model = getattr(gs, self.model_name)(dim=2)
        model.fit_variogram(bin_center, gamma, nugget=True)

        # 5. Ordinary Kriging on Residuals
        # We ensure that the coordinates are sorted in ascending order for gstools
        # (gstools prefers strictly ascending x and y axes)
        y_coords_sorted = np.sort(dem.y_coords_reduced) 
        x_coords_sorted = dem.x_coords_reduced

        krige = gs.krige.Ordinary(
            model=model,
            cond_pos=(x_pts, y_pts),
            cond_val=residuals
        )
        
        # GSTools returns (cols, rows), we transpose to match our (rows, cols) convention
        res_grid_T, var_grid_T = krige.structured([x_coords_sorted, y_coords_sorted])
        res_grid = res_grid_T.T
        var_grid = var_grid_T.T

        # SINCE YOUR DEM HAS ROW=0 IN THE NORTH (MAX Y):
        # We need to vertically invert the kriging result to match it
        res_grid = np.flipud(res_grid)
        var_grid = np.flipud(var_grid)

        # 6. Recompose: Trend(Map) + Residuals(Map)
        final_grid = trend_func(dem.grid_reduced) + res_grid

        # Forzamos que cualquier valor menor a 0.0 se convierta en 0.0
        final_grid = np.maximum(final_grid, 0.0)
        
        return final_grid, var_grid