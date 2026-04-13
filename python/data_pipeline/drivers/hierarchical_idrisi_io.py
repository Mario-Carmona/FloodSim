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
from pyproj import CRS
from rasterio.transform import from_bounds

import numpy as np
from loguru import logger

from drivers.idrisi_io import IdrisiIO
from utils.types import SpatialContext, DynamicRaster


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
        if data.dtype == np.int8:
            data_type = 'byte'
        elif np.issubdtype(data.dtype, np.integer):
            data_type = 'integer'
        else:
            data_type = 'real'

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
        """
        # Ensure the root output directory exists before writing any files
        folder_path.mkdir(parents=True, exist_ok=True)

        logger.info(f"Saving Hierarchical IDRISI dataset to: {folder_path}")

        # 1. Write the master .hif metadata file
        HierarchicalIdrisiIO._write_hif(folder_path / f"{filename}.hif", data, spatial_context)

        num_steps = data.shape[0]
        filenames = []

        # 2. Save each temporal frame as an individual 2D IDRISI raster
        for t in range(num_steps):
            frame_name = f"{t:06d}"
            frame_data = data[t]

            # Delegate the saving of the 2D slice to the standard IdrisiIO handler
            IdrisiIO.save(folder_path, frame_name, frame_data, spatial_context, save_metadata = False)

            filenames.append(frame_name)
        
        # 3. Export global temporal attributes to JSON
        metadata = {
            'steps': num_steps,
            'downgrade_factor': downgrade_factor,
            'timestamps': [ts.isoformat() for ts in timestamps],
            'filenames': filenames
        }

        json_path = folder_path / f"{filename}_metadata.json"
        with open(json_path, 'w', encoding='utf-8') as f:
            json.dump(metadata, f, indent=4)

        logger.debug(f"Successfully saved {num_steps} frames and metadata.")

    @staticmethod
    def read(folder_path: Path, filename: str) -> DynamicRaster:
        """Reads a Hierarchical IDRISI Format (HIF) dataset from disk.

        This method reconstructs a 3D spatiotemporal data cube (DynamicRaster) 
        by parsing the global `.hif` metadata file, the `.json` attributes file, 
        and iteratively loading the individual 2D IDRISI raster frames using 
        the `IdrisiIO` driver.

        Args:
            folder_path (Path): The directory path containing the HIF dataset.
            filename (str): The base name of the dataset (without extensions).

        Returns:
            DynamicRaster: An object containing the 3D data cube, timestamps, 
                downgrade factor, and the global spatial context.

        Raises:
            FileNotFoundError: If the main `.hif` or `_metadata.json` files 
                are not found in the specified directory.
        """
        hif_path = folder_path / f"{filename}.hif"
        json_path = folder_path / f"{filename}_metadata.json"

        if not hif_path.exists():
            raise FileNotFoundError(f"Global HIF metadata not found: {hif_path}")
        if not json_path.exists():
            raise FileNotFoundError(f"JSON metadata not found: {json_path}")

        # 1. Load temporal and file metadata from JSON
        with open(json_path, 'r', encoding='utf-8') as f:
            json_meta = json.load(f)
        
        timestamps = [datetime.fromisoformat(ts) for ts in json_meta.get('timestamps', [])]
        filenames = json_meta.get('filenames', [])
        downgrade_factor = json_meta.get('downgrade_factor', 1)
        num_steps = len(filenames)

        # 2. Extract spatial metadata from the .hif file
        metadata = {}
        with open(hif_path, 'r', encoding='utf-8') as f:
            for line in f:
                if ':' in line:
                    key, val = line.split(':', 1)
                    metadata[key.strip()] = val.strip()

        cols = int(metadata.get('columns', 0))
        rows = int(metadata.get('rows', 0))
        min_x = float(metadata.get('min. X', 0.0))
        max_x = float(metadata.get('max. X', 0.0))
        min_y = float(metadata.get('min. Y', 0.0))
        max_y = float(metadata.get('max. Y', 0.0))
        data_type = metadata.get('data type', 'real')
        crs_string = metadata.get('ref. system', '')
        
        try:
            crs = CRS.from_string(crs_string)
        except Exception:
            crs = None

        ctx = SpatialContext(
            crs=crs,
            transform=from_bounds(min_x, min_y, max_x, max_y, cols, rows),
            width=cols,
            height=rows
        )

        # 3. Initialize 3D matrix and DELEGATE the frame reading to IdrisiIO
        if data_type == 'real':
            dtype = np.float32
        elif data_type == 'byte':
            dtype = np.int8
        else:
            dtype = np.int32

        data_3d = np.zeros((num_steps, rows, cols), dtype=dtype)

        logger.debug(f"Delegating the loading of {num_steps} frames to IdrisiIO...")
        
        for i, fname in enumerate(filenames):
            # Exact delegation: injecting the global context to avoid redundant parsing
            static_raster = IdrisiIO.read(
                folder_path=folder_path,
                filename=fname,
                read_metadata=False,  # Prevent searching for individual .doc files
                spatial_context=ctx,  # Inject the parsed global context
                data_type=data_type
            )
            # Extract the 2D data matrix and store it in our 3D cube
            data_3d[i] = static_raster.data

        logger.success("Successfully loaded HIF dataset.")
        
        return DynamicRaster(
            data=data_3d, 
            timestamps=timestamps, 
            downgrade_factor=downgrade_factor, 
            spatial_context=ctx
        )