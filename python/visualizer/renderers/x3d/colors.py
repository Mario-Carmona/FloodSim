from __future__ import annotations

import json
import logging
from dataclasses import dataclass, field
from pathlib import Path

RGB = tuple[float, float, float]


@dataclass
class X3DColorScheme:
    """Scene colors for the X3D renderer. Loaded from color_palette.json["x3d"]."""
    sky: RGB = (0.53, 0.81, 0.98)
    water_rgb: RGB = (0.10, 0.55, 0.95)
    water_shininess: float = 0.5
    water_transparency: float = 0.15
    # Each band: (max_height_range, rgb). Ascending order; last entry is the catch-all.
    terrain_bands: list[tuple[float, RGB]] = field(default_factory=lambda: [
        (0.5, (0.62, 0.50, 0.25)),
        (2.0, (0.34, 0.52, 0.22)),
        (float("inf"), (0.60, 0.60, 0.60)),
    ])

    def terrain_rgb(self, height_range: float) -> RGB:
        for max_r, rgb in self.terrain_bands:
            if height_range < max_r:
                return rgb
        return self.terrain_bands[-1][1]


def load_colors(palette_path: Path) -> X3DColorScheme:
    """Load X3D colors from the x3d section of color_palette.json. Falls back to defaults."""
    try:
        data = json.loads(palette_path.read_text(encoding="utf-8"))
        x3d = data.get("x3d", {})
        if not x3d:
            return X3DColorScheme()

        sky = tuple(x3d.get("sky_rgb", (0.53, 0.81, 0.98)))
        w = x3d.get("water", {})
        water_rgb = tuple(w.get("rgb", (0.10, 0.55, 0.95)))
        shininess = float(w.get("shininess", 0.5))
        transparency = float(w.get("transparency", 0.15))

        raw_bands = x3d.get("terrain", [])
        bands = []
        for b in raw_bands:
            rgb = tuple(b["rgb"])
            max_r = float(b.get("max_height_range", float("inf")))
            bands.append((max_r, rgb))
        if not bands:
            bands = X3DColorScheme().terrain_bands

        return X3DColorScheme(
            sky=sky,
            water_rgb=water_rgb,
            water_shininess=shininess,
            water_transparency=transparency,
            terrain_bands=bands,
        )
    except Exception as exc:
        logging.getLogger(__name__).warning(
            "Could not load X3D colors from %s: %s. Using defaults.", palette_path, exc
        )
        return X3DColorScheme()
