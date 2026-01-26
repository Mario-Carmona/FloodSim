import h5py
import numpy as np
from datetime import datetime
from typing import List, Dict, Any

class HDF5Writer:
    """Handles writing 3D simulation data (Time, Y, X) to HDF5 format."""

    def __init__(self, output_file: str):
        self.output_file = output_file

    def save(self, rain_stack: np.ndarray, timestamps: List[datetime], geo_metadata: Dict[str, Any]):
        """
        Saves the simulation stack.
        
        Args:
            rain_stack: 3D Numpy array (Time, Rows, Cols).
            timestamps: List of datetime objects corresponding to the time axis.
            geo_metadata: dictionary containing spatial references.
        """
        print(f"--- Saving HDF5: {self.output_file} ---")
        
        # Calculate elapsed time in seconds (assuming constant rate)
        if len(timestamps) > 1:
            dt_seconds = (timestamps[1] - timestamps[0]).total_seconds()
        else:
            dt_seconds = 3600.0 # Default if there is only 1 frame

        # Convert times to POSIX (float) for fast reading in C++
        time_unix = [t.timestamp() for t in timestamps]

        # Convert datetimes to ISO strings for HDF5 compatibility
        str_times = [t.strftime("%Y-%m-%d %H:%M:%S").encode("utf-8") for t in timestamps]

        with h5py.File(self.output_file, "w") as f:
            group = f.create_group("Rainfall")

            # Store the 3D matrix with GZIP compression
            dset = group.create_dataset(
                "Data", 
                data=rain_stack, 
                dtype='float32', 
                compression="gzip", 
                compression_opts=4
            )
            
            # Physical Metadata (Attributes)
            dset.attrs["Units"] = "mm/h"
            dset.attrs["NoData_Value"] = -9999.0

            # Spatial Metadata
            group.attrs["Downgrade_Factor"] = geo_metadata['downgrade_factor']

            # Temporal Metadata
            group.attrs["Time_Step_Seconds"] = float(dt_seconds)
            group.attrs["Start_Time_Unix"] = time_unix[0]

            # Store timestamps
            group.create_dataset("Time_Unix", data=time_unix, dtype='float64')
            group.create_dataset("Time_String", data=str_times)

        print(f"File saved successfully. Grid Shape: {rain_stack.shape}. Time Step: {dt_seconds}s")