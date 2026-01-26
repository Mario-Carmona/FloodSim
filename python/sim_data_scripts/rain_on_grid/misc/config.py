import yaml
from dataclasses import dataclass
from typing import Any, Dict, List

@dataclass
class Config:
    """Singleton-like access to configuration."""
    _config: Dict[str, Any]

    @classmethod
    def load(cls, path: str = "config.yaml"):
        with open(path, "r") as f:
            return cls(yaml.safe_load(f))

    @property
    def simulation(self): return self._config['simulation']
    
    @property
    def geography(self): return self._config['geography']
    
    @property
    def interpolation(self): return self._config['interpolation']

    @property
    def active_sources(self) -> List[str]:
        """Returns the list of enabled source names."""
        return self._config.get('sources', [])

    @property
    def visualization(self) -> Dict[str, Any]:
        return self._config.get('visualization', {})