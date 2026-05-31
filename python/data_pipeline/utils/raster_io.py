from contextlib import ExitStack
from typing import List, Optional

import numpy as np
from loguru import logger
from rasterio.io import MemoryFile
from rasterio.merge import merge

from utils.types import SpatialContext, StaticRaster


def load_raster_from_bytes(data_bytes: List[bytes], data_type: np.typing.DTypeLike = np.float32) -> StaticRaster:
    """Loads and merges raster datasets directly from in-memory byte streams.

    This function takes a list of raw byte streams (e.g., downloaded GeoTIFF files), 
    opens them in memory without writing to disk, and merges them into a single 
    continuous raster dataset. It then packages the merged data and its spatial 
    metadata into a `StaticRaster` object.

    Args:
        data_bytes (List[bytes]): A list containing the raw byte streams of the raster files.
        data_type (np.typing.DTypeLike, optional): The target NumPy data type for the 
            merged raster array. Defaults to np.float32.

    Returns:
        StaticRaster: An object containing the merged data matrix and its associated 
            spatial context (CRS, transform, dimensions, nodata value).

    Raises:
        ValueError: If the provided `data_bytes` list is empty.
        rasterio.errors.RasterioIOError: If any of the byte streams cannot be parsed 
            as a valid raster file.
    """
    if not data_bytes:
        logger.error("The provided list of byte streams is empty.")
        raise ValueError("Cannot load raster: The provided list of byte streams is empty.")

    logger.debug(f"Attempting to load and merge {len(data_bytes)} raster(s) from memory.")

    # ExitStack is used here to dynamically manage an arbitrary number of context managers.
    # This ensures all MemoryFiles and datasets are properly closed, even if an exception occurs
    # or the list of byte streams is very large.
    with ExitStack() as stack:
        datasets = []

        for raw_bytes in data_bytes:
            # Enter context for the memory file
            memfile = stack.enter_context(MemoryFile(raw_bytes))
            # Enter context for the opened rasterio dataset
            dataset = stack.enter_context(memfile.open())
            datasets.append(dataset)

        logger.info("Merging downloaded tiles into a single continuous raster...")
        
        # Merge all opened datasets. The 'merge' function computes the overlapping 
        # bounds and creates a new mosaic array and affine transform.
        mosaic_data, mosaic_transform = merge(datasets)

        # Extract the first band (assuming single-band data like DEMs or categorical land cover)
        # and cast it to the requested numpy data type.
        data_matrix = mosaic_data[0].astype(data_type, copy=False)

        # Safely extract the nodata value, defaulting to -9999.0 if not defined in the source
        nodata_val = datasets[0].nodata if datasets[0].nodata is not None else -9999.0

        # Construct the spatial metadata object to accompany the raw numpy array
        context = SpatialContext(
            crs=datasets[0].crs,
            transform=mosaic_transform,
            width=data_matrix.shape[1],
            height=data_matrix.shape[0]
        )

        logger.info(
            f"Successfully loaded raster from bytes: Dimensions=[{context.width}x{context.height}], "
            f"CRS=[{context.crs.to_string()}]"
        )

        return StaticRaster(data=data_matrix, spatial_context=context)