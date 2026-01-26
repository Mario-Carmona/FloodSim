import os
import matplotlib
# Configure non-interactive backend before importing pyplot
matplotlib.use('Agg') 
import matplotlib.pyplot as plt
import numpy as np
from datetime import datetime

from common_scripts.misc.types import RasterGrid

class Visualizer:
    def __init__(self, config: dict):
        self.enabled = config.get('enabled', False)
        self.output_folder = config.get('output_folder', './results/plots')
        
        if self.enabled:
            os.makedirs(self.output_folder, exist_ok=True)
            print(f"[Visualizer] Output folder: {self.output_folder}")

    def plot_dem(self, dem: RasterGrid):
        """Generates a static image of the terrain (DEM)."""
        if not self.enabled: return

        filename = os.path.join(self.output_folder, "00_orography.png")
        
        plt.figure(figsize=(10, 8))
        # We use ‘terrain’ to give a geographical appearance
        plt.imshow(
            dem.grid, cmap='terrain', 
            extent=[
                dem.bounds.min_x, dem.bounds.max_x,
                dem.bounds.min_y, dem.bounds.max_y
            ]
        )
        plt.colorbar(label='Elevation (m)')
        plt.title("Digital Elevation Model (DEM)")
        plt.xlabel("X (UTM)")
        plt.ylabel("Y (UTM)")
        
        plt.savefig(filename, dpi=150)
        plt.close()
        print(f"[Visualizer] Saved DEM plot: {filename}")

    def plot_rain_step(self, 
                       rain_grid: np.ndarray, 
                       dem: RasterGrid, 
                       timestamp: datetime, 
                       step_index: int):
        """
        Generate a frame with rain superimposed on the terrain.
        """
        if not self.enabled: return

        filename = os.path.join(self.output_folder, f"step_{step_index:04d}.png")
        
        fig, ax = plt.subplots(figsize=(10, 8))
        
        # 1. Paint Background: Relief (MDT) in soft grayscale
        # We use alpha=0.6 so that it is visible but highlights the rain
        ax.imshow(
            dem.grid_reduced, cmap='gray', alpha=0.6, 
            extent=[
                dem.bounds.min_x, dem.bounds.max_x,
                dem.bounds.min_y, dem.bounds.max_y
            ]
        )
        
        # 2. Paint Rain
        im = ax.imshow(
            rain_grid, cmap='jet', alpha=0.7, 
            vmin=0.0, vmax=np.max(rain_grid),
            extent=[
                dem.bounds.min_x, dem.bounds.max_x,
                dem.bounds.min_y, dem.bounds.max_y
            ]
        )
        
        plt.colorbar(im, label='Rain Intensity (mm/h)')
        plt.title(f"Rain Simulation - {timestamp.strftime('%Y-%m-%d %H:%M:%S')}")
        
        plt.savefig(filename, dpi=100)
        plt.close(fig)