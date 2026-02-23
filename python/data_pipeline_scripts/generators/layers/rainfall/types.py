"""
Specific data types for the rainfall generation layer.

This module defines the data structures used to handle meteorological stations 
and their associated precipitation measurements over time.
"""

from dataclasses import dataclass
from datetime import datetime

from misc.types import Position


@dataclass
class StationMetadata:
    """
    Metadata for a meteorological weather station.

    Attributes:
        station_id (str): Unique identifier for the station.
        position (Position): Geographical coordinates of the station.
        location (str): Name of the town or specific location.
        province (str): Province or administrative region.
    """
    station_id: str
    position: Position
    location: str
    province: str


@dataclass
class StationRainData:
    """
    Represents a continuous precipitation measurement over a specific time window.
    
    Attributes:
        station (StationMetadata): The station that recorded the data.
        mm_per_hour (float): Calculated rainfall intensity in millimeters per hour.
        start_timestamp (datetime): Beginning of the measurement period.
        end_timestamp (datetime): End of the measurement period.
    """
    station: StationMetadata
    mm_per_hour: float
    start_timestamp: datetime
    end_timestamp: datetime


@dataclass
class RainDataPoint:
    """
    A simplified representation of rain data for spatial interpolation at a specific moment.

    Attributes:
        position (Position): Spatial coordinate of the measurement.
        value (float): The recorded rainfall intensity (e.g., mm/h).
    """
    position: Position
    value: float