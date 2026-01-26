from dataclasses import dataclass, field
import numpy as np

@dataclass
class BoundingBox:
    """Defines a 2D spatial rectangle."""
    min_x: float
    max_x: float
    min_y: float
    max_y: float

@dataclass
class RasterGrid:
    """
    Represents a geospatial raster grid containing elevation data.
    
    Attributes:
        grid: Original full-resolution elevation matrix.
        bounds: Spatial boundaries of the grid.
        downgrade_factor: Factor to reduce grid resolution for faster processing.
    """
    grid: np.ndarray
    cell_size: float
    bounds: BoundingBox
    downgrade_factor: int
    
    # Derived attributes
    rows: int = field(init=False)
    cols: int = field(init=False)
    x_coords: np.ndarray = field(init=False)
    y_coords: np.ndarray = field(init=False)
    
    # Reduced attributes for processing
    grid_reduced: np.ndarray = field(init=False)
    rows_reduced: int = field(init=False)
    cols_reduced: int = field(init=False)
    x_coords_reduced: np.ndarray = field(init=False)
    y_coords_reduced: np.ndarray = field(init=False)

    def __post_init__(self):
        self.rows, self.cols = self.grid.shape

        # Generate coordinate vectors (centered on pixels)
        self.x_coords = np.linspace(
            self.bounds.min_x + self.cell_size / 2, 
            self.bounds.max_x - self.cell_size / 2, 
            self.cols
        )
        self.y_coords = np.linspace(
            self.bounds.max_y - self.cell_size / 2,
            self.bounds.min_y + self.cell_size / 2, 
            self.rows
        )

        # Create reduced grid for performance optimization
        self.grid_reduced = self.grid[::self.downgrade_factor, ::self.downgrade_factor]
        self.rows_reduced, self.cols_reduced = self.grid_reduced.shape
        self.x_coords_reduced = self.x_coords[::self.downgrade_factor]
        self.y_coords_reduced = self.y_coords[::self.downgrade_factor]

    def get_elevation_at_point(self, x: float, y: float) -> float:
        """
        Retrieves the Z-value (elevation) for a specific (x, y) coordinate.
        
        Args:
            x: X coordinate (e.g., UTM Easting).
            y: Y coordinate (e.g., UTM Northing).
            
        Returns:
            Float representing elevation.
            
        Raises:
            ValueError: If coordinates are outside bounds.
        """
        if not (self.bounds.min_x <= x <= self.bounds.max_x) or \
           not (self.bounds.min_y <= y <= self.bounds.max_y):
            raise ValueError(f"Coordinate ({x}, {y}) out of bounds.")

        # Rasters are indexed row 0 = North (Max Y)
        row = int((self.bounds.max_y - y) / self.cell_size)
        col = int((x - self.bounds.min_x) / self.cell_size)

        # Clamping to prevent index out of bounds on exact edges
        col = min(col, self.cols - 1)
        row = min(row, self.rows - 1)

        return float(self.grid[row, col])
