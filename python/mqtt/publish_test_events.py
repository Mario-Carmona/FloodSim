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

        cells = []
        for i in range(args.cells):
            cells.append(
                {
                    "x": i,
                    "y": i,
                    "state": "LOW_DEPTH" if i % 2 == 0 else "HIGH_DEPTH",
                    "value": 2 if i % 2 == 0 else 4,
                }
            )

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
