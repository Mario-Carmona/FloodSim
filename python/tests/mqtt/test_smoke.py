import os
import shutil
import sys
import tempfile
from pathlib import Path

import numpy as np

# AÃ±adir el root del proyecto al sys.path para poder importar python.mqtt.*
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '../../..')))

from python.mqtt.data_model import SimulationGrid
from python.mqtt.visualizer import GridVisualizer


def _build_init_layer(tmp_root: Path) -> Path:
    layer_dir = tmp_root / "water_depth"
    layer_dir.mkdir(parents=True, exist_ok=True)

    # A tiny deterministic layer for fast smoke tests.
    arr = np.array(
        [
            [0, 1, 2, 3],
            [1, 2, 3, 4],
            [2, 3, 4, 5],
            [0, 0, 1, 1],
        ],
        dtype=np.uint8,
    )
    np.savetxt(layer_dir / "water_depth.csv", arr, delimiter=",", fmt="%d")
    return layer_dir


def run() -> int:
    tmp_root = Path(tempfile.mkdtemp(prefix="danasim_smoke_"))
    try:
        init_layer_dir = _build_init_layer(tmp_root)
        output_dir = tmp_root / "out"

        simulation = SimulationGrid()
        viewer = GridVisualizer(output_folder=str(output_dir))

        # Phase 3: initialization sequence
        ok = simulation.apply_init_map_config(
            {
                "process": "InitMap_Config",
                "map": {
                    "size_x": 4,
                    "size_y": 4,
                    "chunk_size": 2,
                    "cell_resolution_m": 5.0,
                },
            }
        )
        if not ok:
            print("SMOKE FAIL: InitMap_Config not applied")
            return 1

        ok = simulation.apply_init_agent_layer(
            {
                "process": "InitAgent_Layer",
                "data_path": str(init_layer_dir),
                "data_filename": "water_depth",
                "id": "water_depth",
            }
        )
        if not ok:
            print("SMOKE FAIL: InitAgent_Layer not applied")
            return 1

        simulation.mark_init_agent_eof({"process": "InitAgent_EOF"})

        simulation.mark_init_eof({"process": "Init_EOF", "total_chunks_sent": 1})
        viewer.save_snapshot(simulation.grid)
        simulation.consume_data()


        changed = simulation.apply_event(
            {
                "process": "EYE_SetState_Layer",
                "changes": {
                    "cells": [
                        {"x": 0, "y": 0, "value": 5},
                        {"x": 1, "y": 1, "state": "LOW_DEPTH"},
                    ]
                },
            }
        )
        if not changed:
            print("SMOKE FAIL: EYE_SetState_Layer did not modify state")
            return 1

        changed = simulation.apply_event(
            {
                "process": "EYE_SetState",
                "changes": {
                    "coord": {"x": 2, "y": 2},
                    "state": "HIGH_DEPTH",
                },
            }
        )
        if not changed:
            print("SMOKE FAIL: EYE_SetState did not modify state")
            return 1

        viewer.save_snapshot(simulation.grid)
        simulation.consume_data()

        viewer.save_snapshot(simulation.grid)

        files = sorted(output_dir.glob("sim_*.png"))
        if len(files) < 2:
            print("SMOKE FAIL: expected at least 2 snapshots, got", len(files))
            return 1

        print("SMOKE PASS: visualizer pipeline validated")
        print("Snapshots:")
        for path in files:
            print(" -", path)
        return 0
    finally:
        shutil.rmtree(tmp_root, ignore_errors=True)


if __name__ == "__main__":
    raise SystemExit(run())
