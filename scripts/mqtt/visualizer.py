
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
import numpy as np
import config

class GridVisualizer:
    """
    Manages the Real-time Matplotlib visualization.
    """

    def __init__(self, initial_grid):
        self.fig = None
        self.im = None
        self.setup_plot(initial_grid)

    def setup_plot(self, grid_data):
        """
        Initializes the figure, colormap, and grid lines.
        """
        plt.ion()  # Interactive mode ON
        self.fig, ax = plt.subplots(figsize=(5, 5))

        # Custom Colormap:
        # 0 = Light Grey (Dry Land), 1 = Blue (Water)
        cmap = mcolors.ListedColormap(['#f0f0f0', '#0077be'])
        bounds = [-0.5, 0.5, 1.5]
        norm = mcolors.BoundaryNorm(bounds, cmap.N)

        # Draw initial image
        self.im = ax.imshow(
            grid_data, 
            cmap=cmap, 
            norm=norm, 
            origin='upper', 
            interpolation='nearest'
        )

        # Cosmetic grid setup
        ax.set_xticks(np.arange(-.5, config.MAP_SIZE, 1), minor=True)
        ax.set_yticks(np.arange(-.5, config.MAP_SIZE, 1), minor=True)
        ax.grid(which='minor', color='gray', linestyle='-', linewidth=0.5)
        ax.tick_params(which='minor', size=0)
        ax.set_title(f"Hydrological Simulation {config.MAP_SIZE}x{config.MAP_SIZE}")

    def refresh(self, grid_data):
        """
        Updates the image data efficiently (Blitting-like behavior).
        """
        self.im.set_data(grid_data)
        # Short pause to allow the GUI backend to process the draw event
        plt.pause(config.REFRESH_RATE_SECONDS)

    def close(self):
        plt.close(self.fig)
