import os
import sys
from datetime import datetime, timedelta
import numpy as np
import argparse

from common_scripts.data_io.idrisi import IdrisiReader

from sim_data_scripts.rain_on_grid.misc.config import Config
from sim_data_scripts.rain_on_grid.data_io.hdf5 import HDF5Writer
from sim_data_scripts.rain_on_grid.extraction.manager import RainDataManager
from sim_data_scripts.rain_on_grid.interpolation.kriging import KrigingInterpolator
from sim_data_scripts.rain_on_grid.misc.types import RainDataPoint, StationRainData
from sim_data_scripts.rain_on_grid.formatting.visualizer import Visualizer

def main():
    parser = argparse.ArgumentParser(description="Tool to extract precipitation data for a specific geographical area.")

    BASE_DIR = os.path.dirname(os.path.abspath(__file__))
    default_config_path = os.path.join(BASE_DIR, "config.yaml")

    parser.add_argument(
        "config_path", 
        nargs="?", 
        default=default_config_path, 
        help="Path to the .yaml configuration file"
    )

    args = parser.parse_args()

    # 1. Load Configuration
    try:
        print(f"Loading configuration from: {args.config_path}")
        cfg = Config.load(args.config_path)
    except Exception as e:
        print(f"Error loading config file: {e}")
        sys.exit(1)

    # Parse dates
    start_date = datetime.strptime(cfg.simulation['start_date'], "%Y-%m-%d %H:%M:%S")
    end_date = datetime.strptime(cfg.simulation['end_date'], "%Y-%m-%d %H:%M:%S")

    t_temp = datetime.strptime(cfg.simulation['time_step'], "%H:%M:%S")
    time_step = timedelta(hours=t_temp.hour, minutes=t_temp.minute, seconds=t_temp.second)

    # Initialize visualizer
    viz = Visualizer(cfg.visualization)

    # 2. Load Elevation Map (DEM)
    try:
        dem_reader = IdrisiReader(
            cfg.geography['dem_folder'], 
            cfg.geography['dem_filename'],
            downgrade_factor=cfg.geography['downgrade_factor']
        )
        dem = dem_reader.read()

        # Display Orography once at startup
        viz.plot_dem(dem)
    except Exception as e:
        print(f"Simulation aborted. Map error: {e}")
        return

    # 3. Fetch Rain Data
    manager = RainDataManager(cfg.active_sources)
    rain_data = manager.fetch_all(dem.bounds, start_date, end_date)

    if not rain_data:
        print("Simulation aborted: No rain data available.")
        return

    # 4. Temporal Simulation Setup
    total_seconds = (end_date - start_date).total_seconds()
    step_seconds = time_step.total_seconds()
    num_steps = int(total_seconds // step_seconds) + (1 if total_seconds % step_seconds != 0 else 0)

    # Initialize 3D stack (Time, Y, X)
    stack_3d = np.zeros(
        (num_steps, dem.rows_reduced, dem.cols_reduced), 
        dtype='float32'
    )
    timestamps = []

    interpolator = KrigingInterpolator(cfg.interpolation['variogram_model'])
    current_time = start_date

    print(f"--- Starting Simulation Loop ({num_steps} steps) ---")

    for i in range(num_steps):
        print(f"Processing: {current_time}")
        
        # Filter points active in this time window
        active_points = []
        next_time = current_time + time_step
        
        for p in rain_data:
            if p.start_timestamp <= current_time < p.end_timestamp:
                active_points.append(RainDataPoint(
                    x=p.station.x,
                    y=p.station.y,
                    value=p.mm_per_hour
                ))

        # Interpolate if enough data exists
        if len(active_points) >= cfg.interpolation['min_points_required']:
            stack_3d[i, :, :], _ = interpolator.process(dem, active_points)

            # --- NUEVO: CALCULAR Y MOSTRAR ESTADÍSTICAS ---
            step_min = np.min(stack_3d[i, :, :])
            step_max = np.max(stack_3d[i, :, :])
            
            # Imprimimos con 4 decimales para precisión
            print(f"   -> Stats Step {i}: Max = {step_max:.4f} mm/h")
            # ----------------------------------------------

            # Generate frame
            viz.plot_rain_step(
                rain_grid=stack_3d[i, :, :], 
                dem=dem,
                timestamp=current_time,
                step_index=i
            )
        
        timestamps.append(current_time)
        current_time = next_time

    # 5. Save Output
    meta = {
        "downgrade_factor": dem.downgrade_factor
    }

    writer = HDF5Writer(cfg.simulation['output_file'])
    writer.save(stack_3d, timestamps, meta)

if __name__ == "__main__":
    main()