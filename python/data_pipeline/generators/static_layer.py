"""
Base generator implementation for static 2D raster layers.

Provides standardized methods to save and visualize static geographical 
data (e.g., elevation, roughness) using matplotlib.
"""

from pathlib import Path

from typing import Any
import matplotlib
# Configure non-interactive backend before importing pyplot to prevent GUI crashes
matplotlib.use('Agg') 
import matplotlib.pyplot as plt
from loguru import logger
import rasterio

from drivers.idrisi_io import IdrisiIO
from generators.base import DataGenerator
from utils.types import StaticRaster


class StaticLayerGenerator(DataGenerator):
    """
    Generator extension specifically for handling static 2D raster layers.
    """

    def _read(self, output_dir: Path) -> StaticRaster:
        """Loads a previously saved static raster layer from disk.

        Uses the IdrisiIO driver to read the raster data and its spatial 
        context from the specified output directory.

        Args:
            output_dir (Path): The directory path where the layer's data files 
                are stored.

        Returns:
            StaticRaster: The loaded static data layer.
        """
        return IdrisiIO.read(output_dir, self._name)

    def _save(self, output_dir: Path, layer: StaticRaster) -> None:
        """
        Saves the static raster layer to disk.

        Args:
            output_dir (Path): The target directory for the layer.
            layer (StaticRaster): The data layer to save.
        """
        IdrisiIO.save(output_dir, self._name, layer.data, layer.spatial_context)

    def _visualize(self, output_dir: Path, layer: StaticRaster) -> None:
        """
        Generates and saves a 2D map visualization of the static raster.

        Args:
            output_dir (Path): The base output directory.
            layer (StaticRaster): The data layer to visualize.
        """
        visualization_dir = output_dir / "visualization"
        visualization_dir.mkdir(parents=True, exist_ok=True)

        filename = visualization_dir / f"{self._name}.png"
        logger.debug(f"Generating visualization plot for '{self._name}'...")
        
        plt.figure(figsize=self._visual_config.figsize)

        plt.imshow(
            layer.data, 
            cmap=self._visual_config.cmap, 
            extent=[
                layer.spatial_context.bounds.min_x, layer.spatial_context.bounds.max_x,
                layer.spatial_context.bounds.min_y, layer.spatial_context.bounds.max_y
            ]
        )
        plt.colorbar(label=f"{self._name.capitalize()} ({self._visual_config.cbar_unit})")
        plt.title(self._name.capitalize())

        # Extract CRS Name
        crs = layer.spatial_context.crs
        if crs and crs.to_epsg() is not None:
            crs_name = f"EPSG:{crs.to_epsg()}"
        elif crs:
            crs_text = crs.to_wkt()
            if "LOCAL_CS" in crs_text:
                crs_name = "Local System"
            else:
                crs_name = crs.to_string()
                if len(crs_name) > 30:
                    crs_name = "Custom Projection"
        else:
            crs_name = "Unknown"

        # Extract linear units
        try:
            units = crs.linear_units
        except AttributeError:
            units = "m"
            
        # Extract Spatial Resolution (Pixel size)
        resolution = round(layer.spatial_context.transform.a, 2)

        plt.xlabel(f"X Coordinate ({units})")
        plt.ylabel(f"Y Coordinate ({units})")
        
        # Metadata textbox
        metadata_textbox = (
            f"Reference System: {crs_name}\n"
            f"Units: {units.capitalize()}\n"
            f"Spatial Resolution: {resolution} {units}/px"
        )
        
        plt.figtext(
            0.05, 0.05, metadata_textbox, 
            horizontalalignment='left',
            verticalalignment='bottom', 
            fontsize=10, color='black',
            bbox=dict(facecolor='white', alpha=0.8, edgecolor='gray', boxstyle='round,pad=0.5')
        )
        
        # Compress the plot slightly upwards to leave room for the textbox
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

        with rasterio.open(visualization_dir / f"{self._name}.tif", 'w', **profile) as dst:
            dst.write(layer.data, 1)

            
        logger.success(f"Visualization saved successfully for '{self._name}'")