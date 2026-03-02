"""
Geographic type definitions for the flood simulator pipeline.

This module provides standard data structures to represent spatial
coordinates and bounding boxes consistently across the pipeline.
"""

from dataclasses import dataclass

@dataclass
class GeoPoint:
    """
    Represents a geographic point in decimal degrees (WGS84 / EPSG:4326).

    Attributes:
        lat (float): Latitude coordinate (Y-axis).
        lon (float): Longitude coordinate (X-axis).
    """
    lat: float
    lon: float

@dataclass
class GeoBoundingBox:
    """
    Defines a rectangular bounding box using its South-West and North-East corners.

    Attributes:
        south_west (GeoPoint): The bottom-left corner (minimum longitude and latitude).
        north_east (GeoPoint): The top-right corner (maximum longitude and latitude).
    """
    south_west: GeoPoint
    north_east: GeoPoint