from abc import ABC, abstractmethod
from typing import List
from datetime import datetime

from common_scripts.misc.types import BoundingBox

from sim_data_scripts.rain_on_grid.misc.types import StationMetadata, StationRainData

class BaseFetcher(ABC):
    """
    Abstract Base Class for data fetchers.
    Enforces the implementation of station retrieval and bulk data downloading.
    """

    def is_station_relevant(self, station: StationMetadata, bounds: BoundingBox) -> bool:
        """Checks if a station lies within the bounding box."""
        return (bounds.min_x <= station.x <= bounds.max_x) and \
               (bounds.min_y <= station.y <= bounds.max_y)

    @abstractmethod
    def fetch_stations(self) -> List[StationMetadata]:
        """Retrieves available stations from the source."""
        pass

    @abstractmethod
    def fetch_data(self, stations: List[StationMetadata], start: datetime, end: datetime) -> List[StationRainData]:
        """Retrieves rainfall data for the specified stations and time range."""
        pass

    def run(self, bounds: BoundingBox, start: datetime, end: datetime) -> List[StationRainData]:
        """Executes the standard extraction pipeline."""
        print(f"[{self.__class__.__name__}] Starting extraction...")

        all_stations = self.fetch_stations()
        relevant_stations = [s for s in all_stations if self.is_station_relevant(s, bounds)]
        
        print(f"[{self.__class__.__name__}] Total stations: {len(all_stations)}. In bounds: {len(relevant_stations)}.")

        if not relevant_stations:
            return []

        try:
            return self.fetch_data(relevant_stations, start, end)
        except Exception as e:
            print(f"[{self.__class__.__name__}] Critical error during fetch: {e}")
            return []