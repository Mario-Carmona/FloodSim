"""
Base generator implementation for dynamic 3D spatiotemporal layers.

Provides standardized methods to save and visualize time-varying geographical 
data cubes (e.g., simulated rainfall over time) generating frame-by-frame plots.
"""

from pathlib import Path

from typing import Any
import matplotlib
# Configure non-interactive backend before importing pyplot to prevent GUI crashes
matplotlib.use('Agg') 
import matplotlib.pyplot as plt
from loguru import logger
import rasterio

from drivers.hierarchical_idrisi_io import HierarchicalIdrisiIO
from generators.base import DataGenerator
from utils.types import DynamicRaster


class DynamicLayerGenerator(DataGenerator):
    """
    Generator extension specifically for handling spatiotemporal 3D data cubes.
    """

    def _read(self, output_dir: Path) -> DynamicRaster:
        """Loads a previously saved dynamic raster layer from disk.

        Uses the HierarchicalIdrisiIO driver to read the spatiotemporal raster 
        data (3D cube), timestamps, and spatial context from the specified 
        output directory.

        Args:
            output_dir (Path): The directory path where the dynamic layer's 
                data files are stored.

        Returns:
            DynamicRaster: The loaded dynamic data layer.
        """
        return HierarchicalIdrisiIO.read(output_dir, self._name)

    def _save(self, output_dir: Path, layer: DynamicRaster) -> None:
        """
        Saves the dynamic raster layer to disk.

        Args:
            output_dir (Path): The target directory for the layer.
            layer (DynamicRaster): The spatiotemporal data layer to save.
        """
        HierarchicalIdrisiIO.save(
            output_dir, self._name, 
            layer.data, layer.timestamps,
            layer.downgrade_factor, layer.spatial_context
        )

    def _visualize_step(self, step: int, visualization_dir: Path, layer: DynamicRaster) -> None:
        """
        Generates and saves a 2D map visualization for a specific time step.

        Args:
            step (int): The index of the time step to visualize.
            visualization_dir (Path): The directory to save the frame.
            layer (DynamicRaster): The complete data layer containing the time steps.
        """
        filename = visualization_dir / f"{self._name}_step_{step:04d}.png"

        plt.figure(figsize=self._visual_config.figsize)

        # Plot the 2D slice corresponding to the current time step
        plt.imshow(
            layer.data[step, :, :], 
            cmap=self._visual_config.cmap, 
            extent=[
                layer.spatial_context.bounds.min_x, layer.spatial_context.bounds.max_x,
                layer.spatial_context.bounds.min_y, layer.spatial_context.bounds.max_y
            ]
        )
        plt.colorbar(label=f"{self._name.capitalize()} ({self._visual_config.cbar_unit})")
        plt.title(f"{self._name.capitalize()} - Step {step}")

        crs = layer.spatial_context.crs
        if crs and crs.to_epsg() is not None:
            nombre_crs = f"EPSG:{crs.to_epsg()}"
        elif crs:
            nombre_crs = "Custom Projection"
        else:
            nombre_crs = "Unknown"

        try:
            unidades = crs.linear_units
        except AttributeError:
            unidades = "m"
            
        resolucion = round(layer.spatial_context.transform.a, 2)

        plt.xlabel(f"X Coordinate ({unidades})")
        plt.ylabel(f"Y Coordinate ({unidades})")
        
        texto_info = (
            f"Reference System: {nombre_crs}\n"
            f"Units: {unidades.capitalize()}\n"
            f"Spatial Resolution: {resolucion} {unidades}/px"
        )
        
        plt.figtext(
            0.05, 0.05, texto_info, 
            horizontalalignment='left',
            verticalalignment='bottom', 
            fontsize=10, color='black',
            bbox=dict(facecolor='white', alpha=0.8, edgecolor='gray', boxstyle='round,pad=0.5')
        )
        
        plt.tight_layout(rect=[0, 0.15, 1, 1])
        
        plt.savefig(filename, dpi=self._visual_config.dpi)
        plt.close()


        profile = {
            'driver': 'GTiff',
            'height': layer.spatial_context.height,
            'width': layer.spatial_context.width,
            'count': 1,
            'dtype': layer.data.dtype.name,
            'crs': layer.spatial_context.crs,
            'transform': layer.spatial_context.transform,
            'nodata': layer.spatial_context.nodata_value,
            'compress': 'deflate',
            'tiled': True
        }

        with rasterio.open(visualization_dir / f"{self._name}_step_{step:04d}.tif", 'w', **profile) as dst:
            dst.write(layer.data[step, :, :], 1)

    def _visualize(self, output_dir: Path, layer: DynamicRaster) -> None:
        """
        Iterates over all time steps in the layer and generates a visualization frame for each.

        Args:
            output_dir (Path): The base output directory.
            layer (DynamicRaster): The data layer to visualize.
        """
        visualization_dir = output_dir / "visualization"
        visualization_dir.mkdir(parents=True, exist_ok=True)
        
        num_steps = layer.data.shape[0]
        logger.info(f"Generating {num_steps} visualization frames for '{self._name}'...")

        for step in range(num_steps):
            self._visualize_step(step, visualization_dir, layer)
            
        logger.success(f"All visualization frames for '{self._name}' saved successfully.")