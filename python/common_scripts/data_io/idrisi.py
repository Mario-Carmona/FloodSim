import os
import re
import numpy as np
from typing import Tuple, Dict, Any, List, Optional
from common_scripts.misc.types import RasterGrid, BoundingBox

class IdrisiReader:
    """
    Parses IDRISI32 (.rdc/.rst) GIS files.
    Extracts metadata, geometry, and raw binary raster data.
    """

    def __init__(self, folder_path: str, filename: str, downgrade_factor: int = 1):
        self.rdc_path = self._resolve_path(folder_path, filename, ".rdc")
        self.rst_path = self._resolve_path(folder_path, filename, ".rst")
        self.downgrade_factor = downgrade_factor

    def _resolve_path(self, folder: str, filename: str, ext: str) -> str:
        path = os.path.join(folder, filename + ext)
        if not os.path.exists(path):
            raise FileNotFoundError(f"File not found: {path}")
        return path

    def _parse_metadata(self) -> Tuple[Dict[str, Any], List[str]]:
        metadata = {}
        comments = []

        try:
            with open(self.rdc_path, 'r', encoding='utf-8', errors='ignore') as f:
                for line in f:
                    line = line.strip()
                    if not line: continue
                    
                    if ":" in line:
                        key_part, val_part = line.split(":", 1)
                        key = key_part.strip().lower().replace(".", "")
                        val = val_part.strip()

                        if key == "comment":
                            comments.append(val)
                            continue
                        
                        try:
                            # Try converting to int or float
                            num_val = float(val)
                            metadata[key] = int(num_val) if num_val.is_integer() else num_val
                        except ValueError:
                            metadata[key] = val
                            
        except Exception as e:
            raise IOError(f"Failed to read metadata {self.rdc_path}: {e}")

        required_keys = ["min x", "max x", "min y", "max y", "columns", "rows", "resolution", "data type"]
        if any(k not in metadata for k in required_keys):
            raise ValueError("Invalid IDRISI file: missing required metadata keys.")

        return metadata, comments

    def _get_dtype(self, metadata: Dict[str, Any]) -> np.dtype:
        dtype_str = str(metadata.get("data type", "")).lower()
        if "byte" in dtype_str: return np.uint8
        if "integer" in dtype_str: return np.int16
        if "real" in dtype_str: return np.float32
        raise ValueError(f"Unsupported IDRISI data type: {dtype_str}")

    def read(self) -> RasterGrid:
        """Reads the binary data and returns a RasterGrid object."""
        meta, _ = self._parse_metadata()
        dtype = self._get_dtype(meta)
        
        expected_size = meta["rows"] * meta["columns"]
        
        try:
            # IDRISI uses flat binary files
            data = np.fromfile(self.rst_path, dtype=dtype, count=expected_size)
            grid = data.reshape((meta["rows"], meta["columns"]))

            return RasterGrid(
                grid=grid,
                cell_size=meta["resolution"],
                bounds=BoundingBox(
                    min_x=meta["min x"], max_x=meta["max x"],
                    min_y=meta["min y"], max_y=meta["max y"]
                ),
                downgrade_factor=self.downgrade_factor
            )
        except Exception as e:
            raise IOError(f"Error reading raster binary: {e}")