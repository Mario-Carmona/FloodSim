# -*- coding: utf-8 -*-

import os
import tensorflow as tf
import tf2onnx
from tf2onnx import utils
from tensorflow_scripts.simulator_module import SimulatorModule

# Helper para imprimir tipos de datos ONNX de forma legible
def get_onnx_type_name(type_id):
    # Mapeo básico de tipos protobuf de ONNX
    types = {
        1: "FLOAT", 2: "UINT8", 3: "INT8", 4: "UINT16", 5: "INT16",
        6: "INT32", 7: "INT64", 8: "STRING", 9: "BOOL",
    }
    return types.get(type_id, f"UNKNOWN({type_id})")

def inspect_onnx_model(model_proto):
    """
    Imprime los metadatos del modelo ONNX generado para su uso en C++.
    """
    print("\n" + "="*60)
    print("   ONNX MODEL INTERFACE (C++ REFERENCE)")
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

def export_to_onnx():
    output_path = "models/simulation_model_cpu.onnx"

    # Crear directorio si no existe
    os.makedirs(os.path.dirname(output_path), exist_ok=True)

    module = SimulatorModule()

    spec = (
        tf.TensorSpec((None, None, None, 1), tf.float32, name="water_depth"),
        tf.TensorSpec((None, None, None, 1), tf.float32, name="elevation"),
        tf.TensorSpec((None, None, None, 1), tf.float32, name="roughness"),
        tf.TensorSpec((None, None, None, 1), tf.float32, name="rainfall"),
        tf.TensorSpec((), tf.float32, name="dt"),
        tf.TensorSpec((), tf.float32, name="dx")
    )

    print(f"Generando grafo ONNX en: {output_path} ...")
    
    # Convertimos la función concreta directamente a ONNX
    model_proto, _ = tf2onnx.convert.from_function(
        function=module.run_simulation_batch,
        input_signature=spec,
        opset=15,
        output_path=output_path
    )
    
    print("Exportacion completada")

    # --- INSPECCIÓN PARA C++ ---
    inspect_onnx_model(model_proto)

if __name__ == "__main__":
    export_to_onnx()
