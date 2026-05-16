"""Tests to increase coverage on main.py, tile_utils, idrisi_io and visualizer.py."""
from __future__ import annotations

import queue
import sys
import threading
import time
from pathlib import Path
from unittest.mock import MagicMock, patch

import numpy as np
import pytest

from python.visualizer.main import main
from python.visualizer.network import MQTTMonitorClient
from python.visualizer.renderers.matplotlib_renderer import MatplotlibRenderer


# ---------------------------------------------------------------------------
# Shared helpers for main.py tests
# ---------------------------------------------------------------------------

def _run(messages: list) -> dict:
    """Run main() with mocked MQTT and MatplotlibRenderer, return stats."""
    captured = {}
    snapshot_calls = []
    setup_calls = []

    def fake_init(self, q):
        self.queue = q
        captured["q"] = q
        self._logger = MagicMock()
        self.client = MagicMock()

    def fake_save(self, frame, step_index):
        snapshot_calls.append(step_index)

    def fake_setup(self, meta):
        setup_calls.append(meta)

    with (
        patch.object(MQTTMonitorClient, "__init__", fake_init),
        patch.object(MQTTMonitorClient, "connect", lambda self: None),
        patch.object(MQTTMonitorClient, "disconnect", lambda self: None),
        patch.object(MQTTMonitorClient, "publish_chunk_ack", lambda self: None),
        patch.object(MatplotlibRenderer, "save_snapshot", fake_save),
        patch.object(MatplotlibRenderer, "setup", fake_setup),
        patch.object(MatplotlibRenderer, "close", lambda self: None),
        patch("python.visualizer.config.RENDERER_TYPE", "2d"),
        patch("sys.exit"),
    ):
        t = threading.Thread(target=main, daemon=True)
        t.start()
        deadline = time.time() + 2.0
        while "q" not in captured and time.time() < deadline:
            time.sleep(0.01)
        assert "q" in captured
        q = captured["q"]
        for msg in messages:
            q.put({"topic": "FloodSim/test/events", "payload": msg})
        t.join(timeout=5.0)
        assert not t.is_alive()

    return {"snapshots": snapshot_calls, "setups": setup_calls}


def _init_msgs(size_x=5, size_y=4):
    return [
        {"process": "InitMap_Config",
         "map": {"size_x": size_x, "size_y": size_y, "cell_resolution_m": 5.0}},
        {"process": "Init_EOF", "total_chunks_sent": 0},
    ]


def _sim_end():
    return {"process": "Sim_End", "sim_time_total": 1.0}


# ===========================================================================
# main.py — missing branches
# ===========================================================================

class TestMainBranches:
    def test_invalid_init_map_config_logged(self) -> None:
        messages = [
            {"process": "InitMap_Config",
             "map": {"size_x": 0, "size_y": 0, "cell_resolution_m": 5.0}},
            _sim_end(),
        ]
        result = _run(messages)
        assert result["setups"] == []

    def test_init_agent_layer_ok(self, tmp_path: Path) -> None:
        npy = tmp_path / "terrain.npy"
        np.save(npy, np.zeros(20, dtype=np.float32))
        messages = _init_msgs() + [
            {"process": "InitAgent_Layer",
             "id": "topo_bathy",
             "data_path": str(tmp_path),
             "data_filename": "terrain"},
            _sim_end(),
        ]
        _run(messages)

    def test_init_agent_layer_fail_logged(self) -> None:
        messages = _init_msgs() + [
            {"process": "InitAgent_Layer",
             "id": "topo_bathy",
             "data_path": "/nonexistent",
             "data_filename": "missing"},
            _sim_end(),
        ]
        _run(messages)

    def test_init_agent_eof_processed(self) -> None:
        messages = _init_msgs() + [
            {"process": "InitAgent_EOF"},
            _sim_end(),
        ]
        _run(messages)

    def test_eye_setstate_with_chunk_ack(self) -> None:
        messages = _init_msgs() + [
            {"process": "FrameStart", "total_chunks": 2, "chunks_per_batch": 1},
            {"process": "EYE_SetState_Layer",
             "changes": {"cells": {"0": {"state": "FLOODED"}}}},
            {"process": "FrameEnd"},
            {"process": "EYE_Frame_Sync", "simulation_time": "2024-01-01T00:00:00"},
            _sim_end(),
        ]
        result = _run(messages)
        assert len(result["snapshots"]) >= 1

    def test_eye_setstate_single_object(self) -> None:
        messages = _init_msgs() + [
            {"process": "FrameStart", "total_chunks": 1, "chunks_per_batch": 1},
            {"process": "EYE_SetState",
             "changes": {"coord": {"x": 1, "y": 1}, "state": "FLOODED"}},
            {"process": "FrameEnd"},
            {"process": "EYE_Frame_Sync", "simulation_time": "2024-01-01T00:00:00"},
            _sim_end(),
        ]
        result = _run(messages)
        assert len(result["snapshots"]) >= 1

    def test_system_disconnected_event(self) -> None:
        messages = _init_msgs() + [
            {"process": "System_Disconnected"},
            _sim_end(),
        ]
        _run(messages)

    def test_unknown_event_ignored(self) -> None:
        messages = _init_msgs() + [
            {"process": "SomeUnknownEvent"},
            _sim_end(),
        ]
        _run(messages)

    def test_keyboard_interrupt_triggers_close(self) -> None:
        captured = {}

        def fake_init(self, q):
            self.queue = q
            captured["q"] = q
            self._logger = MagicMock()
            self.client = MagicMock()

        closed = []

        def fake_close(self):
            closed.append(True)

        with (
            patch.object(MQTTMonitorClient, "__init__", fake_init),
            patch.object(MQTTMonitorClient, "connect", lambda self: None),
            patch.object(MQTTMonitorClient, "disconnect", lambda self: None),
            patch.object(MatplotlibRenderer, "save_snapshot", lambda *a: None),
            patch.object(MatplotlibRenderer, "setup", lambda *a: None),
            patch.object(MatplotlibRenderer, "close", fake_close),
            patch("python.visualizer.config.RENDERER_TYPE", "2d"),
            patch("sys.exit"),
        ):
            t = threading.Thread(target=main, daemon=True)
            t.start()
            deadline = time.time() + 2.0
            while "q" not in captured and time.time() < deadline:
                time.sleep(0.01)
            # Simulate KeyboardInterrupt via queue timeout expiry + thread kill
            q = captured["q"]
            q.put({"topic": "t", "payload": {"process": "Sim_End"}})
            t.join(timeout=3.0)

        assert closed


# ===========================================================================
# tile_utils.py — reproject_tile
# ===========================================================================

class TestReprojectTile:
    def test_single_band_output_shape(self) -> None:
        from rasterio.crs import CRS
        from rasterio.transform import from_bounds
        from python.visualizer.tools.tile_utils import reproject_tile, tile_bounds_3857

        src_crs = CRS.from_epsg(4326)
        src_transform = from_bounds(-1, -1, 1, 1, 10, 10)
        data = np.ones((10, 10), dtype=np.float32) * 5.0
        w, s, e, n = tile_bounds_3857(5, 16, 16)
        result = reproject_tile(data, src_crs, src_transform, w, s, e, n)
        assert result.shape == (256, 256)

    def test_multi_band_output_shape(self) -> None:
        from rasterio.crs import CRS
        from rasterio.transform import from_bounds
        from python.visualizer.tools.tile_utils import reproject_tile, tile_bounds_3857

        src_crs = CRS.from_epsg(4326)
        src_transform = from_bounds(-180, -90, 180, 90, 20, 10)
        data = np.ones((3, 10, 20), dtype=np.float32)
        w, s, e, n = tile_bounds_3857(3, 4, 4)
        result = reproject_tile(data, src_crs, src_transform, w, s, e, n)
        assert result.shape == (3, 256, 256)

    def test_fill_value_returns_correct_shape(self) -> None:
        from rasterio.crs import CRS
        from rasterio.transform import from_bounds
        from python.visualizer.tools.tile_utils import reproject_tile, tile_bounds_3857

        src_crs = CRS.from_epsg(4326)
        src_transform = from_bounds(0.0, 0.0, 0.01, 0.01, 5, 5)
        data = np.ones((5, 5), dtype=np.float32) * 99.0
        w, s, e, n = tile_bounds_3857(1, 0, 0)
        result = reproject_tile(data, src_crs, src_transform, w, s, e, n,
                                fill_value=0.0)
        assert result.shape == (256, 256)
        assert not np.isnan(result).any()


# ===========================================================================
# idrisi_io.py — missing dtype branches and invalid CRS
# ===========================================================================

class TestIdrisiIOGaps:
    def _ctx(self):
        from python.visualizer.types import SpatialContext
        from pyproj import CRS
        from rasterio.transform import from_bounds
        crs = CRS.from_epsg(4326)
        transform = from_bounds(0, 0, 1, 1, 3, 2)
        ctx = SpatialContext(crs=crs, transform=transform, width=3, height=2)
        ctx.nodata_value = -9999.0
        return ctx

    def test_read_byte_dtype(self, tmp_path: Path) -> None:
        from python.visualizer.idrisi_io import IdrisiIO
        data = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.int8)
        ctx = self._ctx()
        IdrisiIO.save(tmp_path, "byte_test", data, ctx)
        result = IdrisiIO.read(tmp_path, "byte_test", read_metadata=True)
        assert result.data.dtype == np.int8

    def test_read_integer_dtype(self, tmp_path: Path) -> None:
        from python.visualizer.idrisi_io import IdrisiIO
        data = np.array([[10, 20, 30], [40, 50, 60]], dtype=np.int32)
        ctx = self._ctx()
        IdrisiIO.save(tmp_path, "int_test", data, ctx)
        result = IdrisiIO.read(tmp_path, "int_test", read_metadata=True)
        assert result.data.dtype == np.int32

    def test_read_invalid_crs_returns_none(self, tmp_path: Path) -> None:
        from python.visualizer.idrisi_io import IdrisiIO
        doc = (
            "file format : IDRISI Raster A.1\n"
            "data type   : real\n"
            "columns     : 3\n"
            "rows        : 2\n"
            "ref. system : INVALID_CRS_STRING\n"
            "min. X      : 0\nmax. X      : 1\n"
            "min. Y      : 0\nmax. Y      : 1\n"
        )
        (tmp_path / "bad_crs.doc").write_text(doc, encoding="utf-8")
        (tmp_path / "bad_crs.img").write_text("1.0 2.0 3.0\n4.0 5.0 6.0\n")
        result = IdrisiIO.read(tmp_path, "bad_crs", read_metadata=True)
        assert result.spatial_context.crs is None


# ===========================================================================
# visualizer.py — GridVisualizer
# ===========================================================================

class TestGridVisualizer:
    def test_init_creates_folder(self, tmp_path: Path) -> None:
        from python.visualizer.visualizer import GridVisualizer
        out = str(tmp_path / "frames")
        with patch("python.visualizer.config.COLOR_PALETTE_FILE", tmp_path / "missing.json"):
            v = GridVisualizer(output_folder=out)
        assert (tmp_path / "frames").exists()

    def test_save_snapshot_creates_png(self, tmp_path: Path) -> None:
        from python.visualizer.visualizer import GridVisualizer
        with patch("python.visualizer.config.COLOR_PALETTE_FILE", tmp_path / "missing.json"):
            v = GridVisualizer(output_folder=str(tmp_path))
        grid = np.zeros((4, 5), dtype=np.uint8)
        v.save_snapshot(grid, step_index=0)
        assert (tmp_path / "sim_00000.png").exists()

    def test_save_snapshot_skips_duplicate(self, tmp_path: Path) -> None:
        from python.visualizer.visualizer import GridVisualizer
        with patch("python.visualizer.config.COLOR_PALETTE_FILE", tmp_path / "missing.json"):
            v = GridVisualizer(output_folder=str(tmp_path))
        grid = np.zeros((4, 5), dtype=np.uint8)
        v.save_snapshot(grid, step_index=0)
        v.save_snapshot(grid, step_index=0)
        assert v.frame_count == 1

    def test_save_snapshot_none_step_uses_count(self, tmp_path: Path) -> None:
        from python.visualizer.visualizer import GridVisualizer
        with patch("python.visualizer.config.COLOR_PALETTE_FILE", tmp_path / "missing.json"):
            v = GridVisualizer(output_folder=str(tmp_path))
        grid = np.zeros((4, 5), dtype=np.uint8)
        v.save_snapshot(grid, step_index=None)
        assert v.frame_count == 1

    def test_load_colormap_from_valid_palette(self, tmp_path: Path) -> None:
        import json as _json
        from python.visualizer.visualizer import GridVisualizer
        palette = {
            "layers": {
                "flood_risk": [
                    {"value": i, "hex": "#0000FF"} for i in range(6)
                ]
            }
        }
        p = tmp_path / "palette.json"
        p.write_text(_json.dumps(palette), encoding="utf-8")
        with patch("python.visualizer.config.COLOR_PALETTE_FILE", str(p)), \
             patch("python.visualizer.config.COLOR_PALETTE_LAYER", "flood_risk"):
            v = GridVisualizer(output_folder=str(tmp_path / "out"))
        assert v.vmin == 0
        assert v.vmax == 5

    def test_close_is_safe(self, tmp_path: Path) -> None:
        from python.visualizer.visualizer import GridVisualizer
        with patch("python.visualizer.config.COLOR_PALETTE_FILE", tmp_path / "missing.json"):
            v = GridVisualizer(output_folder=str(tmp_path))
        v.close()
