"""
Manager for coordinating multiple meteorological data fetchers.

This module provides the `RainDataManager` facade, which acts as a single
entry point to extract rainfall data from various external sources simultaneously.
"""

from datetime import datetime
from typing import List

from loguru import logger
from pyproj import CRS

from misc.types import Bounds
from generators.layers.rainfall.extraction.base import BaseFetcher
from generators.layers.rainfall.extraction.sources.saih_jucar.saih_jucar import SAIHJucarFetcher
from generators.layers.rainfall.types import StationRainData


class RainDataManager:
    """
    Facade for managing and executing multiple rainfall data sources.

    Dynamically or statically initializes the required fetchers and aggregates
    the data retrieved from all active sources into a single unified list.

    Attributes:
        fetchers (List[BaseFetcher]): A list of active data fetcher instances.
    """

    def __init__(self) -> None:
        """
        Initializes the RainDataManager with the configured data fetchers.
        """
        self.fetchers: List[BaseFetcher] = [
            SAIHJucarFetcher()
        ]

    def fetch_all(
        self, 
        bounds: Bounds, 
        start_date: datetime, 
        end_date: datetime, 
        target_crs: CRS
    ) -> List[StationRainData]:
        """
        Executes all registered fetchers to retrieve meteorological data.

        Iterates through each active fetcher, triggers its extraction pipeline,
        and aggregates the results into a single collection.

        Args:
            bounds (Bounds): The spatial boundaries of the study area.
            start_date (datetime): The start of the extraction window.
            end_date (datetime): The end of the extraction window.
            target_crs (CRS): The destination coordinate reference system.

        Returns:
            List[StationRainData]: A unified list of all rainfall data retrieved 
                from the active sources.
        """
        unified_data: List[StationRainData] = []

        logger.info(f"Starting Data Extraction across {len(self.fetchers)} active sources...")

        for fetcher in self.fetchers:
            try:
                data = fetcher.run(bounds, start_date, end_date, target_crs)
                if data:
                    unified_data.extend(data)
            except Exception as e:
                # Catching exceptions at this level prevents one faulty source
                # from crashing the entire multi-source extraction process.
                logger.error(f"Extraction failed for source '{fetcher.__class__.__name__}': {e}")
        
        logger.success(f"Global extraction completed. Retrieved {len(unified_data)} data points in total.")
        return unified_data