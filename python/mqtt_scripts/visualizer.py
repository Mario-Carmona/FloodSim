
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
import os
import mqtt_scripts.config

class GridVisualizer:
    """
    Guarda el estado del grid como imágenes PNG de alta resolución.
    CORREGIDO: Usa vmin/vmax en lugar de norm para imsave.
    """

    def __init__(self, output_folder="output_frames"):
        self.output_folder = output_folder
        self.frame_count = 0
        
        if not os.path.exists(self.output_folder):
            os.makedirs(self.output_folder)
            print(f"Carpeta creada: {self.output_folder}")

        # Definir Colormap: 
        # 0 = Negro (Fondo), 1 = Azul Cian Brillante (Agua)
        self.cmap = mcolors.ListedColormap(['#000000', '#00FFFF'])

    def save_snapshot(self, grid_data, step_index=None):
        """
        Guarda la matriz actual como una imagen PNG.
        """
        if step_index is None:
            step_index = self.frame_count
        
        filename = os.path.join(self.output_folder, f"sim_{step_index:05d}.png")
        
        # CORRECCIÓN:
        # Eliminamos 'norm=self.norm' y usamos 'vmin=0, vmax=1'.
        # Esto fuerza a que el 0 sea el primer color (Negro) y el 1 el segundo (Cian).
        plt.imsave(
            filename, 
            grid_data, 
            cmap=self.cmap, 
            vmin=0, 
            vmax=1,
            origin='upper'
        )
        
        self.frame_count += 1
        # Opcional: imprimir solo cada X frames para no ensuciar la consola
        if config.DEBUG_MODE:
             print(f"Imagen guardada: {filename}")

    def close(self):
        pass
