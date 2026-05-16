"""Tests for network.py — MQTTMonitorClient with mocked paho."""
from __future__ import annotations

import json
import queue
from unittest.mock import MagicMock, patch, call

import pytest

from python.visualizer.network import MQTTMonitorClient


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _make_msg(topic: str, payload: dict) -> MagicMock:
    msg = MagicMock()
    msg.topic = topic
    msg.payload = json.dumps(payload).encode("utf-8")
    return msg


def _make_client() -> tuple[MQTTMonitorClient, MagicMock, queue.Queue]:
    """Return (client, mock_paho_client, queue)."""
    q = queue.Queue()
    mock_paho = MagicMock()
    with patch("paho.mqtt.client.Client", return_value=mock_paho):
        client = MQTTMonitorClient(q)
    return client, mock_paho, q


# ===========================================================================
# __init__
# ===========================================================================

class TestInit:
    def test_queue_stored(self) -> None:
        client, _, q = _make_client()
        assert client.queue is q

    def test_on_connect_registered(self) -> None:
        client, mock_paho, _ = _make_client()
        assert mock_paho.on_connect == client._on_connect

    def test_on_message_registered(self) -> None:
        client, mock_paho, _ = _make_client()
        assert mock_paho.on_message == client._on_message

    def test_lwt_set(self) -> None:
        _, mock_paho, _ = _make_client()
        mock_paho.will_set.assert_called_once()
        args, kwargs = mock_paho.will_set.call_args
        payload = json.loads(kwargs.get("payload") or args[1])
        assert payload["process"] == "System_Disconnected"


# ===========================================================================
# connect / disconnect
# ===========================================================================

class TestConnect:
    def test_connect_calls_broker(self) -> None:
        client, mock_paho, _ = _make_client()
        client.connect()
        mock_paho.connect.assert_called_once()
        mock_paho.loop_start.assert_called_once()

    def test_connect_raises_on_failure(self) -> None:
        client, mock_paho, _ = _make_client()
        mock_paho.connect.side_effect = ConnectionRefusedError("refused")
        with pytest.raises(ConnectionRefusedError):
            client.connect()

    def test_disconnect(self) -> None:
        client, mock_paho, _ = _make_client()
        client.disconnect()
        mock_paho.loop_stop.assert_called_once()
        mock_paho.disconnect.assert_called_once()


# ===========================================================================
# _on_connect
# ===========================================================================

class TestOnConnect:
    def test_subscribes_on_success(self) -> None:
        client, mock_paho, _ = _make_client()
        client._on_connect(mock_paho, None, {}, rc=0)
        assert mock_paho.subscribe.call_count == 2

    def test_no_subscribe_on_failure(self) -> None:
        client, mock_paho, _ = _make_client()
        client._on_connect(mock_paho, None, {}, rc=1)
        mock_paho.subscribe.assert_not_called()


# ===========================================================================
# _on_message
# ===========================================================================

class TestOnMessage:
    def test_regular_event_queued(self) -> None:
        client, mock_paho, q = _make_client()
        from python.visualizer import config
        msg = _make_msg(config.TOPIC_EVENTS, {"process": "FrameStart", "total_chunks": 5})
        client._on_message(mock_paho, None, msg)
        item = q.get_nowait()
        assert item["payload"]["process"] == "FrameStart"
        assert item["topic"] == config.TOPIC_EVENTS

    def test_ping_triggers_pong(self) -> None:
        client, mock_paho, q = _make_client()
        from python.visualizer import config
        msg = _make_msg(config.TOPIC_HANDSHAKE_PING, {"process": "System_Ping"})
        client._on_message(mock_paho, None, msg)
        mock_paho.publish.assert_called_once()
        args, kwargs = mock_paho.publish.call_args
        topic = args[0] if args else kwargs.get("topic")
        assert topic == config.TOPIC_HANDSHAKE_PONG
        assert q.empty()  # ping not forwarded to queue

    def test_invalid_json_not_queued(self) -> None:
        client, mock_paho, q = _make_client()
        msg = MagicMock()
        msg.topic = "some/topic"
        msg.payload = b"not valid json {{{"
        client._on_message(mock_paho, None, msg)
        assert q.empty()

    def test_non_ping_on_handshake_topic_queued(self) -> None:
        client, mock_paho, q = _make_client()
        from python.visualizer import config
        msg = _make_msg(config.TOPIC_HANDSHAKE_PING, {"process": "System_Pong"})
        client._on_message(mock_paho, None, msg)
        assert not q.empty()


# ===========================================================================
# publish_chunk_ack
# ===========================================================================

class TestPublishChunkAck:
    def test_publishes_chunk_ack(self) -> None:
        client, mock_paho, _ = _make_client()
        client.publish_chunk_ack()
        mock_paho.publish.assert_called_once()
        args, kwargs = mock_paho.publish.call_args
        payload = json.loads(args[1] if len(args) > 1 else kwargs["payload"])
        assert payload["process"] == "ChunkAck"


# ===========================================================================
# publish_ping
# ===========================================================================

class TestPublishPing:
    def test_publishes_ping(self) -> None:
        client, mock_paho, _ = _make_client()
        client.publish_ping()
        mock_paho.publish.assert_called_once()
        args, kwargs = mock_paho.publish.call_args
        payload = json.loads(args[1] if len(args) > 1 else kwargs["payload"])
        assert payload["process"] == "System_Ping"
