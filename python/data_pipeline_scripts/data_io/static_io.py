"""
Input/Output operations for static raster files.

This module handles the loading and saving of 2D static raster data (like GeoTIFFs)
into the custom StaticRaster data structures used by the pipeline.
"""

from pathlib import Path

import rasterio
from loguru import logger

from misc.types import StaticRaster, SpatialContext


class StaticRasterIO:
    """Handles the reading and writing of static spatial layers."""

    @staticmethod
    def load(folder_path: str, filename: str) -> StaticRaster:
        """
        Loads a GeoTIFF file into a StaticRaster object.

        Args:
            folder_path (Path): The directory containing the raster file.
            filename (str): The name of the file (without the .tif extension).

        Returns:
            StaticRaster: The loaded raster data and its spatial context.
        """
        tif_path = folder_path / f"{filename}.tif"
        logger.debug(f"Loading static raster from: {tif_path}")

        with rasterio.open(tif_path) as src:
            raster = StaticRaster(
                data=src.read(1),
                spatial_context=SpatialContext(
                    crs=src.crs,
                    transform=src.transform,
                    width=src.width,
                    height=src.height,
                    nodata_value=src.nodata
                )
            )
            return raster

    @staticmethod
    def save(folder_path: str, filename: str, raster: StaticRaster) -> None:
        """
        Saves a StaticRaster object to disk as a GeoTIFF.

        Args:
            folder_path (Path): The destination directory.
            filename (str): The name of the file to save (without extension).
            raster (StaticRaster): The raster data object to serialize.
        """
        tif_path = folder_path / f"{filename}.tif"
        logger.debug(f"Saving static raster to: {tif_path}")

        # Save using the standard GeoTIFF driver
        with rasterio.open(
            tif_path,
            'w',
            driver='GTiff',
            height=raster.spatial_context.height,
            width=raster.spatial_context.width,
            count=1,
            dtype=raster.data.dtype,
            crs=raster.spatial_context.crs,
            transform=raster.spatial_context.transform,
            nodata=raster.spatial_context.nodata_value,
            compress='lzw'
        ) as dst:
            dst.write(raster.data, 1)