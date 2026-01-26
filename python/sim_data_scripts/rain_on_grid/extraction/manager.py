from typing import List, Dict, Type
from datetime import datetime

from common_scripts.misc.types import BoundingBox

from sim_data_scripts.rain_on_grid.misc.types import StationRainData
from sim_data_scripts.rain_on_grid.extraction.base import BaseFetcher
from sim_data_scripts.rain_on_grid.extraction.sources.saih_jucar.saih_jucar import SAIHJucarFetcher

class RainDataManager:
    """
    Facade for managing multiple data sources.
    Dynamically initializes fetchers based on the configuration list.
    """

    # Mapping string names from YAML to Python Classes
    SOURCE_MAP: Dict[str, Type[BaseFetcher]] = {
        "saih_jucar": SAIHJucarFetcher
    }

    def __init__(self, active_sources: List[str]):
        """
        Args:
            active_sources: List of strings (e.g., ["saih_jucar", "saih_segura"])
        """
        self.fetchers: List[BaseFetcher] = []
        
        print(f"--- Initializing Data Manager ---")
        
        for source_name in active_sources:
            source_key = source_name.lower()
            
            if source_key in self.SOURCE_MAP:
                fetcher_class = self.SOURCE_MAP[source_key]
                
                try:
                    # Instantiate and add to list
                    instance = fetcher_class()
                    self.fetchers.append(instance)
                    print(f"   [OK] Loaded source: {source_name.upper()}")
                except Exception as e:
                    print(f"   [ERROR] Failed to load {source_name}: {e}")
            else:
                print(f"   [WARN] Unknown source in config: '{source_name}' (Skipping)")

    def fetch_all(self, bounds: BoundingBox, start: datetime, end: datetime) -> List[StationRainData]:
        unified_data = []
        
        if not self.fetchers:
            print("[RainDataManager] Warning: No sources are active.")
            return []

        print(f"--- Starting Data Extraction ({len(self.fetchers)} sources) ---")

        for fetcher in self.fetchers:
            data = fetcher.run(bounds, start, end)
            if data:
                unified_data.extend(data)
        
        return unified_data