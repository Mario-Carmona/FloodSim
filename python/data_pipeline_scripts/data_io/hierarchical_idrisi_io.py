"""
Input/Output operations for the custom Hierarchical IDRISI Format (HIF).

This module manages a custom file structure designed to simulate 3D spatiotemporal
datasets (similar to HDF5 or NetCDF) using standard 2D IDRISI raster files. It
organizes time-series data into discrete temporal frames within a directory, 
accompanied by a global metadata file (.hif) and a JSON attributes file.
"""

import json
from datetime import datetime
from pathlib import Path
from typing import List

import numpy as np
from loguru import logger

from data_io.idrisi_io import IdrisiIO
from misc.types import SpatialContext


class HierarchicalIdrisiIO:
    """
    Handler for the custom Hierarchical IDRISI Format (HIF).

    This class provides static methods to split and save 3D spatiotemporal
    data cubes into a structured directory of individual 2D IDRISI rasters,
    allowing for easy frame-by-frame analysis while preserving global metadata.
    """

    @staticmethod
    def _write_hif(path: Path, data: np.ndarray, spatial_context: SpatialContext) -> None:
        """
        Writes the master Hierarchical IDRISI (.hif) metadata file.

        This file acts as a global descriptor for the entire 3D dataset,
        similar to a standard IDRISI .doc file, but representing the properties
        of the complete temporal stack.

        Args:
            path (Path): The absolute or relative path to save the .hif file.
            data (np.ndarray): The 3D numpy array containing the spatiotemporal data.
            spatial_context (SpatialContext): The spatial metadata associated 
                with the rasters.
        """
        # Safely determine the general data type for the IDRISI format
        data_type = 'integer' if np.issubdtype(data.dtype, np.integer) else 'real'
        min_val = np.nanmin(data)
        max_val = np.nanmax(data)
    
        hif_content = (
            f"file format : Hierarchical IDRISI\n"
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
            f.write(hif_content)

    @staticmethod
    def save(
        folder_path: Path, 
        filename: str, 
        data: np.ndarray, 
        timestamps: List[datetime],
        downgrade_factor: int,
        spatial_context: SpatialContext
    ) -> None:
        """
        Saves a 3D dataset (Time x Y x X) into the Hierarchical IDRISI format.

        This method generates a root directory containing a global `.hif` metadata
        file, a JSON file with temporal attributes, and a series of subdirectories 
        for each time step. Each subdirectory holds a standard 2D IDRISI raster 
        representing a single temporal frame.

        Args:
            folder_path (Path): The root directory where the HIF structure will be built.
            filename (str): The base name for the dataset and master metadata file.
            data (np.ndarray): The 3D numpy array representing the spatiotemporal cube.
            timestamps (List[datetime]): A list of datetime objects corresponding 
                to each step in the temporal dimension.
            downgrade_factor (int): The spatial resolution reduction factor applied 
                to the data.
            spatial_context (SpatialContext): The spatial metadata for the dataset.
            
        Raises:
            ValueError: If the length of timestamps does not match the temporal 
                dimension of the data array.
        """
        # Ensure the root output directory exists before writing any files
        folder_path.mkdir(parents=True, exist_ok=True)

        logger.info(f"Saving Hierarchical IDRISI dataset to: {folder_path}")

        # 1. Write the master .hif metadata file
        HierarchicalIdrisiIO._write_hif(folder_path / f"{filename}.hif", data, spatial_context)

        num_steps = data.shape[0]
        filenames = []

        if len(timestamps) != num_steps:
            logger.warning("Number of timestamps does not match the data's temporal dimension.")

        # 2. Save each temporal frame as an individual 2D IDRISI raster
        for t in range(num_steps):
            frame_name = f"{t:06d}"
            frame_data = data[t]

            # Create a dedicated subdirectory for the frame
            frame_path = folder_path / frame_name
            frame_path.mkdir(parents=True, exist_ok=True)

            # Delegate the saving of the 2D slice to the standard IdrisiIO handler
            IdrisiIO.save(frame_path, frame_name, frame_data, spatial_context)

            filenames.append(frame_name)
        
        # 3. Export global temporal attributes to JSON
        attrs = {
            'steps': num_steps,
            'downgrade_factor': downgrade_factor,
            'timestamps': [ts.isoformat() for ts in timestamps],
            'filenames': filenames
        }

        attrs_path = folder_path / "attrs.json"
        with open(attrs_path, 'w', encoding='utf-8') as f:
            json.dump(attrs, f, indent=4)

        logger.debug(f"Successfully saved {num_steps} frames and metadata.")