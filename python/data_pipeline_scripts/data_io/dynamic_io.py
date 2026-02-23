"""
Input/Output operations for dynamic spatiotemporal raster files.

This module handles the loading and saving of 3D dynamic raster data (time-series cubes)
using the HDF5 format, which is highly optimized for large scientific datasets.
"""

from pathlib import Path
from typing import Dict, Any

import h5py
import numpy as np
from pyproj import CRS
from affine import Affine
from loguru import logger

from misc.types import DynamicRaster, SpatialContext


class DynamicRasterIO:
    """Handles the reading and writing of spatiotemporal 3D data layers."""

    @staticmethod
    def load(folder_path: Path, filename: str) -> DynamicRaster:
        """
        Loads a 3D HDF5 file into a DynamicRaster object.

        Args:
            folder_path (Path): The directory containing the HDF5 file.
            filename (str): The name of the file (without the .h5 extension).

        Returns:
            DynamicRaster: The loaded spatiotemporal data cube.
        """
        h5_path = folder_path / f"{filename}.h5"
        logger.debug(f"Loading dynamic raster from: {h5_path}")

        with h5py.File(h5_path, 'r') as h5f:
            # 1. Load the 3D data cube
            data = np.array(h5f['data'])
            
            # 2. Reconstruct SpatialContext from HDF5 attributes
            crs_str = h5f.attrs.get('crs', '')
            crs = CRS.from_string(crs_str) if crs_str else None
            
            transform_tuple = h5f.attrs.get('transform', None)
            transform = Affine(*transform_tuple) if transform_tuple else None
            
            spatial_context = SpatialContext(
                crs=crs,
                transform=transform,
                width=h5f.attrs.get('width', data.shape[2]),
                height=h5f.attrs.get('height', data.shape[1]),
                nodata_value=h5f.attrs.get('nodata_value', -9999.0)
            )

            # Note: Timestamps list reconstruction should ideally be handled here
            # if they were stored during the save process.
            timestamps = [] 

            return DynamicRaster(data, timestamps, spatial_context)

    @staticmethod
    def save(folder_path: Path, filename: str, raster: DynamicRaster) -> None:
        """
        Saves a DynamicRaster object to disk using the HDF5 format with GZIP compression.

        Args:
            folder_path (Path): The destination directory.
            filename (str): The name of the file to save (without extension).
            raster (DynamicRaster): The spatiotemporal data object to serialize.
        """
        h5_path = folder_path / f"{filename}.h5"
        logger.info(f"Saving 3D Spatiotemporal data to {h5_path.name}...")

        # Calculate time step in seconds
        if len(raster.timestamps) > 1:
            dt_seconds = (raster.timestamps[1] - raster.timestamps[0]).total_seconds()
        else:
            dt_seconds = 3600.0  # Default to 1 hour if only 1 frame exists

        # Convert datetimes to POSIX (float) for fast reading in C++ downstream
        time_unix = [t.timestamp() for t in raster.timestamps]

        with h5py.File(h5_path, 'w') as h5f:
            # Store the 3D matrix with GZIP compression
            dset = h5f.create_dataset(
                "data", 
                data=raster.data, 
                dtype='float32', 
                compression="gzip", 
                compression_opts=4
            )
            
            # Physical Metadata Attributes
            dset.attrs["units"] = "mm/h"
            dset.attrs["nodata_value"] = -9999.0

            # Spatial Metadata Attributes
            h5f.attrs["downgrade_factor"] = raster.downgrade_factor

            # Temporal Metadata Attributes
            h5f.attrs["time_step_seconds"] = float(dt_seconds)
            h5f.attrs["start_time_unix"] = time_unix[0] if time_unix else 0.0

            # Store timestamps array
            h5f.create_dataset("time_unix", data=np.array(time_unix, dtype='float64'))
            
        logger.debug("HDF5 saving completed successfully.")