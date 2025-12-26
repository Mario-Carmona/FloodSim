
import time
import logging
import matplotlib.pyplot as plt
import config

# Import our new modules
from data_model import SimulationGrid
from network import MQTTMonitorClient
from visualizer import GridVisualizer

# Configure logging format
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - [%(levelname)s] - %(message)s'
)

def main():
    # 1. Initialize the Data Model
    simulation = SimulationGrid()

    # 2. Initialize and start Network Client
    mqtt_client = MQTTMonitorClient(simulation)
    mqtt_client.connect()

    # 3. Initialize Visualization
    # We pass the initial grid state to setup the window
    viewer = GridVisualizer(simulation.grid)
    
    print("Visual Client Started. Press Ctrl+C to exit.")

    try:
        # 4. Main GUI Loop
        while True:
            # Check if the model has new data to render
            if simulation.has_new_data:
                # Retrieve updated grid and reset flag
                current_grid = simulation.consume_data()
                viewer.refresh(current_grid)
            else:
                # Sleep briefly to save CPU when idle
                plt.pause(config.IDLE_SLEEP_SECONDS)

    except KeyboardInterrupt:
        print("\nUser interrupted. Shutting down...")
    finally:
        mqtt_client.disconnect()
        viewer.close()

if __name__ == "__main__":
    main()
