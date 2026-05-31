"""
Abstract base class for meteorological data extraction.

This module defines the `BaseFetcher` interface, which standardizes the 
process of downloading, filtering, and reprojecting rainfall data from 
various external sources.
"""

from abc import ABC, abstractmethod
from datetime import datetime
from typing import List

from loguru import logger
from pyproj import CRS, Transformer

from utils.types import Bounds
from generators.layers.rainfall.types import StationMetadata, StationRainData


class BaseFetcher(ABC):
    """
    Abstract Base Class for meteorological data fetchers.

    Enforces the implementation of station retrieval and bulk data downloading,
    while providing standardized methods for spatial filtering and reprojection.
    """

    def is_station_relevant(self, station: StationMetadata, bounds: Bounds) -> bool:
        """
        Checks if a station lies within the specified spatial bounding box.

        Args:
            station (StationMetadata): The metadata of the station to check.
            bounds (Bounds): The spatial boundaries of the target area.

        Returns:
            bool: True if the station is inside the bounds, False otherwise.
        """
        return (bounds.min_x <= station.position.x <= bounds.max_x) and \
               (bounds.min_y <= station.position.y <= bounds.max_y)

    def reproject_station(self, station: StationMetadata, target_crs: CRS) -> StationMetadata:
        """
        Reprojects the spatial coordinates of a station to a target Coordinate Reference System.

        Args:
            station (StationMetadata): The station to reproject.
            target_crs (CRS): The destination CRS.

        Returns:
            StationMetadata: The station object with updated x and y coordinates.
        """
        # always_xy=True is vital to maintain (Easting, Northing) or (Lon, Lat) order
        # independently of standard EPSG definitions.
        transformer = Transformer.from_crs(station.position.crs, target_crs, always_xy=True)
    
        station.position.x, station.position.y = transformer.transform(
            station.position.x, station.position.y
        )
        return station

    @abstractmethod
    def fetch_stations(self) -> List[StationMetadata]:
        """
        Retrieves available meteorological stations from the external source.

        Returns:
            List[StationMetadata]: A list of metadata for all available stations.
        """
        pass

    @abstractmethod
    def fetch_data(
        self, 
        stations: List[StationMetadata], 
        start_date: datetime, 
        end_date: datetime
    ) -> List[StationRainData]:
        """
        Retrieves rainfall data for the specified stations and time range.

        Args:
            stations (List[StationMetadata]): The list of relevant stations.
            start_date (datetime): The start of the extraction window.
            end_date (datetime): The end of the extraction window.

        Returns:
            List[StationRainData]: A list of rainfall data points.
        """
        pass

    def run(
        self, 
        bounds: Bounds, 
        start_date: datetime, 
        end_date: datetime, 
        target_crs: CRS
    ) -> List[StationRainData]:
        """
        Executes the standard extraction pipeline for a specific data source.

        This orchestrates fetching stations, reprojecting them, filtering out
        irrelevant ones, and finally fetching the rainfall data for the valid ones.

        Args:
            bounds (Bounds): The spatial boundaries to filter stations.
            start_date (datetime): Simulation start date.
            end_date (datetime): Simulation end date.
            target_crs (CRS): The target coordinate reference system.

        Returns:
            List[StationRainData]: The extracted rainfall data for the area.
        """
        logger.info(f"[{self.__class__.__name__}] Starting extraction pipeline...")

        # 1. Fetch and reproject stations
        all_stations = self.fetch_stations()
        all_stations_reprojected = [
            self.reproject_station(s, target_crs) for s in all_stations
        ]

        # 2. Filter stations by bounding box
        relevant_stations = [
            s for s in all_stations_reprojected if self.is_station_relevant(s, bounds)
        ]
        
        logger.debug(
            f"[{self.__class__.__name__}] Total stations: {len(all_stations)}. "
            f"In bounds: {len(relevant_stations)}."
        )

        if not relevant_stations:
            logger.warning(f"[{self.__class__.__name__}] No relevant stations found within bounds.")
            return []

        # 3. Fetch data for relevant stations
        try:
            return self.fetch_data(relevant_stations, start_date, end_date)
        except Exception as e:
            logger.error(f"[{self.__class__.__name__}] Data extraction failed: {e}")
            raise