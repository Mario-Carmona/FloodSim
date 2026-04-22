import os
import sys
import threading
import time
from unittest.mock import MagicMock, patch

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "../../..")))

from python.mqtt.main import main
from python.mqtt.network import MQTTMonitorClient
from python.mqtt.visualizer import GridVisualizer


def _run_main_with_messages(messages: list, chunks_per_batch: int) -> int:
    """
    Runs main() in a daemon thread with a mocked MQTT client.
    Feeds `messages` into the queue after startup and returns the number
    of ChunkAck calls recorded.
    """
    captured = {}
    ack_calls = []

    def fake_init(self, q):
        self.queue = q
        captured["q"] = q
        self._logger = MagicMock()
        self.client = MagicMock()

    with (
        patch.object(MQTTMonitorClient, "__init__", fake_init),
        patch.object(MQTTMonitorClient, "connect", lambda self: None),
        patch.object(MQTTMonitorClient, "disconnect", lambda self: None),
        patch.object(MQTTMonitorClient, "publish_chunk_ack", lambda self: ack_calls.append(1)),
        patch.object(GridVisualizer, "save_snapshot", lambda self, grid: None),
        patch("sys.exit"),
    ):
        t = threading.Thread(target=main, daemon=True)
        t.start()

        deadline = time.time() + 2.0
        while "q" not in captured and time.time() < deadline:
            time.sleep(0.01)

        assert "q" in captured, "main() did not expose the queue in time"

        q = captured["q"]
        for msg in messages:
            q.put({"topic": "FloodSim/test/events", "payload": msg})

        t.join(timeout=5.0)
        assert not t.is_alive(), "main() thread did not finish before timeout"

    return len(ack_calls)


def _base_init_messages():
    return [
        {
            "process": "InitMap_Config",
            "map": {"size_x": 10, "size_y": 10, "chunk_size": 2, "cell_resolution_m": 1.0},
        },
        {"process": "Init_EOF", "total_chunks_sent": 0},
    ]


def _chunk_message():
    return {"process": "EYE_SetState_Layer", "changes": {"cells": []}}


def test_chunk_ack_count():
    """6 chunks with batch_size=2 must produce exactly 3 ACKs."""
    messages = _base_init_messages() + [
        {"process": "FrameStart", "total_chunks": 6, "chunks_per_batch": 2},
        _chunk_message(),
        _chunk_message(),
        _chunk_message(),
        _chunk_message(),
        _chunk_message(),
        _chunk_message(),
        {"process": "FrameEnd"},
        {"process": "Sim_End", "sim_time_total": 1.0},
    ]
    acks = _run_main_with_messages(messages, chunks_per_batch=2)
    assert acks == 3, f"expected 3 ChunkAck, got {acks}"
    print("PASS test_chunk_ack_count: 3 ChunkAck received correctly")


def test_no_ack_without_frame_start():
    """Without FrameStart, chunks_per_batch stays 0 and no ACK is sent."""
    messages = _base_init_messages() + [
        _chunk_message(),
        _chunk_message(),
        _chunk_message(),
        {"process": "Sim_End", "sim_time_total": 1.0},
    ]
    acks = _run_main_with_messages(messages, chunks_per_batch=0)
    assert acks == 0, f"expected 0 ChunkAck, got {acks}"
    print("PASS test_no_ack_without_frame_start: no ACK sent (backward compat)")


def test_partial_batch_no_ack():
    """5 chunks with batch_size=3 → only 1 ACK (after chunk 3); chunks 4-5 don't complete the next batch."""
    messages = _base_init_messages() + [
        {"process": "FrameStart", "total_chunks": 5, "chunks_per_batch": 3},
        _chunk_message(),
        _chunk_message(),
        _chunk_message(),  # ACK here
        _chunk_message(),
        _chunk_message(),  # frame ends with 2 pending, no ACK
        {"process": "FrameEnd"},
        {"process": "Sim_End", "sim_time_total": 1.0},
    ]
    acks = _run_main_with_messages(messages, chunks_per_batch=3)
    assert acks == 1, f"expected 1 ChunkAck, got {acks}"
    print("PASS test_partial_batch_no_ack: 1 ACK for full batch, partial remainder silent")


def run() -> int:
    tests = [test_chunk_ack_count, test_no_ack_without_frame_start, test_partial_batch_no_ack]
    failed = 0
    for test in tests:
        try:
            test()
        except Exception as exc:
            print(f"FAIL {test.__name__}: {exc}")
            failed += 1
    if failed == 0:
        print(f"\nAll {len(tests)} tests passed.")
    else:
        print(f"\n{failed}/{len(tests)} tests FAILED.")
    return failed


if __name__ == "__main__":
    raise SystemExit(run())
