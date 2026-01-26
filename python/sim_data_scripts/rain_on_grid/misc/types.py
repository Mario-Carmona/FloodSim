from dataclasses import dataclass, field
from datetime import datetime

@dataclass
class StationMetadata:
    """Metadata for a weather station."""
    station_id: str
    x: float
    y: float
    location: str
    province: str

@dataclass
class StationRainData:
    """
    Represents a specific rainfall measurement.
    
    Attributes:
        mm_per_hour: Calculated intensity (mm/h).
    """
    station: StationMetadata
    mm_per_hour: float
    start_timestamp: datetime
    end_timestamp: datetime

@dataclass
class RainDataPoint:
    x: float
    y: float
    value: float