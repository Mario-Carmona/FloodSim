from __future__ import annotations

import logging
from pathlib import Path

from ... import config
from ..base import BaseRenderer, FrameData, GridMeta
from .colors import load_colors
from .serializer import X3DSerializer


class X3DRenderer(BaseRenderer):
    def __init__(self, output_folder: str) -> None:
        self._output_folder = Path(output_folder) / "x3d_files"
        self._serializer = X3DSerializer()
        self._colors = load_colors(config.COLOR_PALETTE_FILE)
        self._water_frames: list[str] = []
        self._logger = logging.getLogger(__name__)

    def setup(self, meta: GridMeta) -> None:
        self._serializer.configure(meta, self._colors)
        self._output_folder.mkdir(parents=True, exist_ok=True)
        self._logger.info(
            "X3DRenderer ready — output: %s  grid: %sx%s  cell: %.2fm",
            self._output_folder, meta.rows, meta.cols, meta.cell_size_m,
        )

    def save_snapshot(self, frame: FrameData, step_index: int) -> None:
        html, water_str = self._serializer.serialize(frame)
        self._water_frames.append(water_str)
        path = self._output_folder / f"step_{step_index:05d}.html"
        try:
            path.write_text(html, encoding="utf-8")
            self._logger.debug("X3D frame saved: %s", path)
        except OSError as exc:
            self._logger.error("Failed to write X3D frame %s: %s", path, exc)

    def close(self) -> None:
        if not self._water_frames:
            return
        player_path = self._output_folder / "player.html"
        try:
            player_html = self._serializer.generate_player(self._water_frames)
            player_path.write_text(player_html, encoding="utf-8")
            self._logger.info("Animation player: %s", player_path)
        except OSError as exc:
            self._logger.error("Failed to write player.html: %s", exc)
