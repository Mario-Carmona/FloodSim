"""
Input/Output operations for IDRISI raster formats and GeoTIFF byte streams.

This module provides utilities to load raster data from in-memory GeoTIFF byte
streams and save numpy arrays into the IDRISI raster format (.doc and .img files).
"""

from contextlib import ExitStack
from pathlib import Path
from typing import List

import numpy as np
import rasterio
from loguru import logger
from rasterio.io import MemoryFile
from rasterio.merge import merge

from misc.types import SpatialContext, StaticRaster


class IdrisiIO:
    """
    Handler for IDRISI format and GeoTIFF memory operations.

    Provides static methods to interact with raster data, facilitating
    conversion from byte streams to standard data structures and exporting
    to IDRISI ASCII format.
    """

    @staticmethod
    def load_from_geotiff_bytes(data_bytes: List[bytes]) -> StaticRaster:
        """
        Loads and merges raster datasets directly from in-memory byte streams.

        This method takes multiple byte streams representing GeoTIFF files, opens
        them in memory, and merges them into a single continuous mosaic. It extracts
        the first band of the resulting mosaic and packages it alongside its
        geographic metadata.

        Args:
            data_bytes (List[bytes]): A list containing the raw binary content of
                the raster files to be merged.

        Returns:
            StaticRaster: A standardized data object containing the merged 2D data
                array and its spatial context.

        Raises:
            ValueError: If the provided list of byte streams is empty.
            rasterio.errors.RasterioIOError: If a memory file is corrupted or
                cannot be read as a valid raster.
        """
        if not data_bytes:
            raise ValueError("The provided list of byte streams is empty.")

        logger.debug("Attempting to load raster data directly from memory bytes.")

        # Using ExitStack to safely manage a dynamic number of context managers
        with ExitStack() as stack:
            datasets = []
            for elem in data_bytes:
                memfile = stack.enter_context(MemoryFile(elem))
                dataset = stack.enter_context(memfile.open())
                datasets.append(dataset)

            logger.info("Merging downloaded tiles into a single continuous raster...")
            mosaic_data, mosaic_transform = merge(datasets)

            # Extract the primary data matrix (Band 1)
            data_matrix = mosaic_data[0].astype(np.dtype(datasets[0].dtypes[0]))

            # Extract and construct the geographic metadata
            context = SpatialContext(
                crs=datasets[0].crs,
                transform=mosaic_transform,
                width=data_matrix.shape[1],
                height=data_matrix.shape[0],
                nodata_value=datasets[0].nodata if datasets[0].nodata is not None else -9999.0
            )

            logger.info(
                f"Successfully loaded raster from bytes: "
                f"Dimensions=[{context.width}x{context.height}], "
                f"CRS=[{context.crs.to_string()}]"
            )

            return StaticRaster(data=data_matrix, spatial_context=context)

    @staticmethod
    def _write_doc(path: Path, data: np.ndarray, spatial_context: SpatialContext) -> None:
        """
        Writes the IDRISI metadata (.doc) file for a given raster.

        Generates the necessary metadata fields required by the IDRISI ASCII
        format, calculating dynamic values like min/max directly from the array.

        Args:
            path (Path): The absolute or relative path where the .doc file will be saved.
            data (np.ndarray): The 2D numpy array containing the raster data.
            spatial_context (SpatialContext): The spatial metadata associated
                with the raster.
        """
        # Safely evaluate data types using numpy's built-in type hierarchy
        data_type = 'integer' if np.issubdtype(data.dtype, np.integer) else 'real'
        min_val = np.nanmin(data)
        max_val = np.nanmax(data)

        doc_content = (
            f"file format : IDRISI Raster A.1\n"
            f"file title  : \n"
            f"data type   : {data_type}\n"
            f"file type   : ascii\n"
            f"columns     : {spatial_context.width}\n"
            f"rows        : {spatial_context.height}\n"
            f"ref. system : {spatial_context.crs.to_string()}\n"
            f"ref. units  : m\n"
            f"unit dist.  : 1.0\n"
            f"min. X      : {spatial_context.bounds.min_x}\n"
            f"max. X      : {spatial_context.bounds.max_x}\n"
            f"min. Y      : {spatial_context.bounds.min_y}\n"
            f"max. Y      : {spatial_context.bounds.max_y}\n"
            f"pos'n error : unknown\n"
            f"resolution  : {spatial_context.cell_size}\n"
            f"min. value  : {min_val}\n"
            f"max. value  : {max_val}\n"
            f"display min : {min_val}\n"
            f"display max : {max_val}\n"
            f"value units : unspecified\n"
            f"nodata value: {spatial_context.nodata_value}\n"
        )

        with open(path, 'w', encoding='utf-8') as f:
            f.write(doc_content)

    @staticmethod
    def save(folder_path: Path, filename: str, data: np.ndarray, spatial_context: SpatialContext, save_metadata: bool = True) -> None:
        """
        Saves a raster array and its spatial context to IDRISI ASCII format.

        This creates two files: a .doc file containing the metadata and an .img
        file containing the raw ASCII values of the raster matrix.

        Args:
            folder_path (Path): The directory path where the files will be saved.
            filename (str): The base name for the generated files (without extension).
            data (np.ndarray): The 2D numpy array containing the raster values.
            spatial_context (SpatialContext): The spatial metadata for the raster.
            save_metadata (bool): Indicates whether an additional file should be generated with the spatial context metadata.
        """
        # Ensure output directory exists to prevent FileNotFoundError
        folder_path.mkdir(parents=True, exist_ok=True)

        img_path = folder_path / f"{filename}.img"

        logger.info(f"Saving IDRISI raster to: {folder_path / filename}")

        if(save_metadata):
            doc_path = folder_path / f"{filename}.doc"
            IdrisiIO._write_doc(doc_path, data, spatial_context)

        # Determine appropriate format specifier based on data type hierarchy
        fmt = '%f' if np.issubdtype(data.dtype, np.floating) else '%d'
        np.savetxt(img_path, data, fmt=fmt)