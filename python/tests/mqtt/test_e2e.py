import argparse
import os
import subprocess
import sys
import tempfile
import time
import shutil
from pathlib import Path

# Add project root to sys.path
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '../../..')))

def run_e2e():
    """
    Spins up the subscriber (visualizer) and publisher.
    Requires Mosquitto to be running on localhost:1883.
    """
    tmp_root = Path(tempfile.mkdtemp(prefix="danasim_e2e_"))
    scenario = "e2e_test_scenario"
    out_dir = tmp_root / ("scenario_x3d_phase1_" + scenario)
    
    # We will pass an environment variable to override output dir if possible,
    # or just let it generate output locally and we check the results.
    # Currently the visualizer writes to ./outputs/scenario_x3d_phase1_<TIMESTAMP>
    # We will just look at the ./outputs directory after running.
    
    python_exec = sys.executable
    
    print("=== Starting E2E Test ===")
    print(f"Creating a small dummy CSV at {tmp_root}/water_depth.csv")
    layer_dir = tmp_root / "water_depth"
    layer_dir.mkdir(parents=True, exist_ok=True)
    with open(layer_dir / "water_depth.csv", "w") as f:
        f.write("0,1,2\n1,2,3\n2,3,4\n")

    subscriber_env = os.environ.copy()
    subscriber_env["DANASIM_SCENARIO"] = scenario
    subscriber_env["DANASIM_OUTPUT_FOLDER"] = str(out_dir)
    
    print("Starting Subscriber (main.py)...")
    subscriber_proc = subprocess.Popen(
        [python_exec, "-m", "python.mqtt.main"],
        env=subscriber_env,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True
    )
    
    # Wait for subscriber to connect
    time.sleep(2)
    
    print("Starting Publisher...")
    publisher_proc = subprocess.run(
        [
            python_exec, "-m", "python.tests.mqtt.publish_test_events",
            "--scenario", scenario,
            "--host", "localhost",
            "--port", "1883",
            "--init-layer-path", str(layer_dir),
            "--init-layer-filename", "water_depth",
            "--init-layer-id", "water_depth",
            "--cells", "30"
        ],
        capture_output=True,
        text=True
    )
    
    # Wait for subscriber to finish processing Sim_End and write files
    try:
        sub_out, _ = subscriber_proc.communicate(timeout=30)
    except subprocess.TimeoutExpired:
        print("Timeout waiting for subscriber to exit upon Sim_End.")
        subscriber_proc.kill()
        sub_out, _ = subscriber_proc.communicate()
        print("Subscriber Output:\n", sub_out)
        return 1

    print("Publisher Output:\n", publisher_proc.stdout)
    print("Subscriber Output:\n", sub_out)

    if "Clean exit after Sim_End" not in sub_out:
        print("E2E FAIL: Subscriber did not cleanly exit upon Sim_End.")
        return 1
        
    print("E2E PASS: Both publisher and subscriber completed successfully.")
    return 0

if __name__ == "__main__":
    sys.exit(run_e2e())
