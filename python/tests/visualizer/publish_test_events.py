import argparse
import json
import time
from datetime import datetime, timezone

import paho.mqtt.client as mqtt


def utc_now_iso() -> str:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")


def build_topics(scenario: str):
    base = f"FloodSim/{scenario}"
    return {
        "events": f"{base}/events",
        "ping": f"{base}/system/handshake/ping",
        "pong": f"{base}/system/handshake/pong",
    }


def main():
    parser = argparse.ArgumentParser(description="Publish test MQTT events for DanaSim visualizer")
    parser.add_argument("--host", default="localhost")
    parser.add_argument("--port", type=int, default=1883)
    parser.add_argument("--scenario", required=True)
    parser.add_argument("--qos", type=int, default=1)
    parser.add_argument("--wait-pong", type=float, default=5.0)
    parser.add_argument("--cells", type=int, default=30, help="Number of test cells to publish")
    parser.add_argument(
        "--init-layer-path",
        default="data/data_29_10_2024/water_depth",
        help="Path sent in InitAgent_Layer data_path",
    )
    parser.add_argument("--init-layer-filename", default="water_depth")
    parser.add_argument("--init-layer-id", default="water_depth")
    args = parser.parse_args()

    topics = build_topics(args.scenario)
    got_pong = {"value": False}

    client = mqtt.Client(client_id=f"DanaSim_TestPublisher_{args.scenario}", clean_session=True)

    def on_connect(cli, userdata, flags, rc):
        if rc != 0:
            print(f"Connection failed with code {rc}")
            return
        cli.subscribe(topics["pong"], qos=args.qos)
        print(f"Connected. Listening for pong on: {topics['pong']}")

    def on_message(cli, userdata, msg):
        try:
            payload = json.loads(msg.payload.decode("utf-8"))
        except Exception:
            return
        if payload.get("process") == "System_Pong":
            got_pong["value"] = True
            print("Received System_Pong")

    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(args.host, args.port, 60)
    client.loop_start()

    try:
        ping = {
            "process": "System_Ping",
            "source": "Test_Publisher",
            "scenario": args.scenario,
            "timestamp_utc": utc_now_iso(),
        }
        client.publish(topics["ping"], payload=json.dumps(ping), qos=args.qos)
        print(f"Published ping to: {topics['ping']}")

        deadline = time.time() + args.wait_pong
        while time.time() < deadline and not got_pong["value"]:
            time.sleep(0.05)

        if not got_pong["value"]:
            print("No pong received in time. Continuing to publish sample events anyway.")

        init_map = {
            "process": "InitMap_Config",
            "source": "Test_Publisher",
            "scenario": args.scenario,
            "timestamp_utc": utc_now_iso(),
            "map": {
                "size_x": 9403,
                "size_y": 9403,
                "chunk_size": 100,
                "cell_resolution_m": 5.0,
            },
            "metadata": {
                "sim_start_time": utc_now_iso(),
                "time_step_s": 1.0,
            },
        }
        client.publish(topics["events"], payload=json.dumps(init_map), qos=args.qos)
        print(f"Published InitMap_Config to: {topics['events']}")

        init_layer = {
            "process": "InitAgent_Layer",
            "source": "Test_Publisher",
            "scenario": args.scenario,
            "timestamp_utc": utc_now_iso(),
            "data_path": args.init_layer_path,
            "data_filename": args.init_layer_filename,
            "id": args.init_layer_id,
        }
        client.publish(topics["events"], payload=json.dumps(init_layer), qos=args.qos)
        print(f"Published InitAgent_Layer to: {topics['events']}")

        init_agent_eof = {
            "process": "InitAgent_EOF",
            "source": "Test_Publisher",
            "scenario": args.scenario,
            "timestamp_utc": utc_now_iso(),
        }
        client.publish(topics["events"], payload=json.dumps(init_agent_eof), qos=args.qos)
        print(f"Published InitAgent_EOF to: {topics['events']}")

        # Initial color payload sent before Init_EOF, as agreed.
        initial_color = {
            "process": "EYE_SetState_Layer",
            "source": "Test_Publisher",
            "scenario": args.scenario,
            "timestamp_utc": utc_now_iso(),
            "id": "water_depth",
            "changes": {
                "cells": [
                    {"x": 0, "y": 0, "value": 4},
                    {"x": 1, "y": 0, "value": 3},
                    {"x": 2, "y": 0, "value": 2},
                ]
            },
        }
        client.publish(topics["events"], payload=json.dumps(initial_color), qos=args.qos)
        print(f"Published initial color payload to: {topics['events']}")

        init_eof = {
            "process": "Init_EOF",
            "source": "Test_Publisher",
            "scenario": args.scenario,
            "timestamp_utc": utc_now_iso(),
            "total_chunks_sent": 1,
        }
        client.publish(topics["events"], payload=json.dumps(init_eof), qos=args.qos)
        print(f"Published Init_EOF to: {topics['events']}")

        cells = []
        for i in range(args.cells):
            if i % 3 == 0:
                # Test formato string
                cells.append({"x": i, "y": i, "state": "FLOODED"})
            elif i % 3 == 1:
                # Test formato número en clave 'state'
                cells.append({"x": i, "y": i, "state": 4})
            else:
                # Test formato número en clave 'value' (legacy)
                cells.append({"x": i, "y": i, "value": 2})

        update_layer = {
            "process": "EYE_SetState_Layer",
            "source": "Test_Publisher",
            "scenario": args.scenario,
            "timestamp_utc": utc_now_iso(),
            "id": "CellState",
            "changes": {
                "cells": cells,
            },
        }
        client.publish(topics["events"], payload=json.dumps(update_layer), qos=args.qos)
        print(f"Published EYE_SetState_Layer ({len(cells)} cells) to: {topics['events']}")

        frame_sync = {
            "process": "EYE_Frame_Sync",
            "source": "Test_Publisher",
            "scenario": args.scenario,
            "timestamp_utc": utc_now_iso(),
            "total_chunks_sent": len(cells),
            "simulation_time": utc_now_iso(),
        }
        client.publish(topics["events"], payload=json.dumps(frame_sync), qos=args.qos)
        print(f"Published EYE_Frame_Sync to: {topics['events']}")

        sim_end = {
            "process": "Sim_End",
            "source": "Test_Publisher",
            "scenario": args.scenario,
            "timestamp_utc": utc_now_iso(),
            "sim_time_total": 60.0,
        }
        client.publish(topics["events"], payload=json.dumps(sim_end), qos=args.qos)
        print(f"Published Sim_End to: {topics['events']}")
    finally:
        time.sleep(0.2)
        client.loop_stop()
        client.disconnect()


if __name__ == "__main__":
    main()
