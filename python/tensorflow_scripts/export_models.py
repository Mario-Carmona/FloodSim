# -*- coding: utf-8 -*-

import os
import glob
import importlib.util
import tensorflow as tf
import tf2onnx
from tf2onnx import utils

# Helper para imprimir tipos de datos ONNX de forma legible
def get_onnx_type_name(type_id):
    # Mapeo básico de tipos protobuf de ONNX
    types = {
        1: "FLOAT", 2: "UINT8", 3: "INT8", 4: "UINT16", 5: "INT16",
        6: "INT32", 7: "INT64", 8: "STRING", 9: "BOOL",
    }
    return types.get(type_id, f"UNKNOWN({type_id})")

def inspect_onnx_model(model_proto, model_name):
    print("\n" + "="*60)
    print(f"   ONNX MODEL INTERFACE: {model_name} (C++ REFERENCE)")
    print("="*60 + "\n")

    graph = model_proto.graph

    def print_node_info(nodes, title):
        print(f"--- {title} ---")
        for node in nodes:
            name = node.name
            
            # Obtener tipo y forma
            tensor_type = node.type.tensor_type
            elem_type = get_onnx_type_name(tensor_type.elem_type)
            
            dims = []
            for dim in tensor_type.shape.dim:
                if dim.dim_value > 0:
                    dims.append(str(dim.dim_value))
                elif dim.dim_param:
                    dims.append(f"? ({dim.dim_param})") # Dimensión dinámica
                else:
                    dims.append("?")
            
            shape_str = f"[{', '.join(dims)}]"
            
            print(f" Name:  '{name}'")
            print(f" Type:  {elem_type}")
            print(f" Shape: {shape_str}")
            print("-" * 30)

    print_node_info(graph.input, "INPUTS (Usar en input_names_)")
    print("\n")
    print_node_info(graph.output, "OUTPUTS (Usar en output_names_)")
    print("="*60 + "\n")

def export_all_models(source_dir, output_dir):
    """
    Busca archivos .py en source_dir, encuentra la clase del modelo,
    y exporta un .onnx en output_dir con el mismo nombre del archivo.
    """
    os.makedirs(output_dir, exist_ok=True)

    # Buscar todos los archivos .py en el directorio fuente
    search_pattern = os.path.join(source_dir, "*.py")

    for filepath in glob.glob(search_pattern):
        filename = os.path.basename(filepath)
        if filename == "__init__.py":
            continue

        model_name = os.path.splitext(filename)[0]
        output_path = os.path.join(output_dir, model_name)

        os.makedirs(output_path, exist_ok=True)
        
        print(f"\nProcesando archivo: {filename}...")

        # 1. Cargar el módulo (archivo .py) dinámicamente
        spec_import = importlib.util.spec_from_file_location(model_name, filepath)
        module = importlib.util.module_from_spec(spec_import)
        spec_import.loader.exec_module(module)

        # 2. Obtener la clase directamente por su nombre
        class_name = "SimulatorModule"

        try:
            model_class = getattr(module, class_name)
        except AttributeError:
            print(f"Omitiendo {filename}: No se encontró la clase '{class_name}'.")
            continue

        # 3. Instanciar el modelo y exportar
        print(f"Encontrada clase '{model_class.__name__}'. Generando ONNX en: {output_path}")
        
        try:
            model_instance = model_class()
            
            model_init, _ = tf2onnx.convert.from_function(
                function=model_instance.precompute,
                input_signature=[
                    tf.TensorSpec((1, None, None, 1), tf.int8,   name="land_cover"),
                    tf.TensorSpec((), tf.float32, name="dt"),
                    tf.TensorSpec((), tf.float32, name="dx")
                ],
                opset=15,
                output_path=os.path.join(output_path, f"init_model.onnx")
            )
            
            model_run, _ = tf2onnx.convert.from_function(
                function=model_instance.run,
                input_signature=[
                    tf.TensorSpec((1, None, None, 1), tf.float32, name="water_depth"),
                    tf.TensorSpec((1, None, None, 1), tf.float32, name="topo_bathy"),
                    tf.TensorSpec((1, None, None, 1), tf.float32, name="k_spatial"),
                    tf.TensorSpec((1, None, None, 1), tf.float32, name="rainfall")
                ],
                opset=15,
                output_path=os.path.join(output_path, f"run_model.onnx")
            )
            
            print(f"Exportación completada para {model_name}")
            inspect_onnx_model(model_init, model_name)
            inspect_onnx_model(model_run, model_name)
            
        except AttributeError:
            print(f"Error: La clase '{model_class.__name__}' no tiene el método 'run'.")
        except Exception as e:
            print(f"Error al exportar {model_name}: {e}")


if __name__ == "__main__":
    # 1. Obtener la ruta absoluta del directorio donde reside este script exacto
    script_dir = os.path.dirname(os.path.abspath(__file__))

    # 2. Construir las rutas uniendo el directorio del script con los nombres de tus carpetas
    source_directory = os.path.abspath(os.path.join(script_dir, "models"))
    output_directory = os.path.abspath(os.path.join(script_dir, "../../exported_models"))

    export_all_models(source_dir=source_directory, output_dir=output_directory)

    # CREAR ARCHIVO DE MARCA (Timestamp)
    # Esto le dice a CMake que la carpeta 'models' ya está sincronizada
    timestamp_path = os.path.join(output_directory, ".export_timestamp")
    with open(timestamp_path, "w") as f:
        f.write("Synchronized")
