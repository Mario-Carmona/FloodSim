#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Automated ONNX Exporter for Hydrodynamic Cellular Automata Models.

This script scans a source directory for TensorFlow models defined as tf.Module,
exports their 'preprocess' and 'step' methods to ONNX format, and generates
a comprehensive JSON metadata file for seamless C++ ONNX Runtime integration.
"""

import os
import glob
import json
import importlib.util
import tensorflow as tf
import tf2onnx

def get_onnx_type_name(type_id: int) -> str:
    """Maps ONNX protobuf type integer to a readable string."""
    onnx_types = {
        1: "FLOAT", 2: "UINT8", 3: "INT8", 4: "UINT16", 5: "INT16",
        6: "INT32", 7: "INT64", 8: "STRING", 9: "BOOL",
    }
    return onnx_types.get(type_id, f"UNKNOWN({type_id})")

def extract_node_metadata(nodes) -> list:
    """Extracts name, type, and shape information from ONNX graph nodes."""
    metadata = []
    for node in nodes:
        tensor_type = node.type.tensor_type
        elem_type = get_onnx_type_name(tensor_type.elem_type)
        
        dims = []
        for dim in tensor_type.shape.dim:
            if dim.dim_value > 0:
                dims.append(dim.dim_value)
            elif dim.dim_param:
                dims.append(dim.dim_param)  # Dynamic dimension (e.g., "unk__1")
            else:
                dims.append("?")
                
        metadata.append({
            "name": node.name,
            "type": elem_type,
            "shape": dims
        })
    return metadata

def generate_graph_metadata(model_proto) -> dict:
    """Extracts full input/output metadata from an ONNX ModelProto."""
    graph = model_proto.graph
    return {
        "inputs": extract_node_metadata(graph.input),
        "outputs": extract_node_metadata(graph.output)
    }

def print_summary(metadata: dict):
    """Prints a clean, formatted summary of the exported model to the console."""
    print(f"\n{'-'*50}")
    print(f" MODEL EXPORTED: {metadata['model_name']}")
    print(f" FLUID LAYER: {metadata['fluid_layer']}")
    print(f" FLUID MOVEMENT STATE LAYER: {metadata['fluid_movement_state_layer']}")
    print(f"{'-'*50}")
    
    for method in ["preprocess", "step"]:
        if method in metadata:
            print(f" [{method.upper()} GRAPH]")
            print("  Inputs:")
            for inp in metadata[method]["inputs"]:
                print(f"   - {inp['name']} ({inp['type']}) Shape: {inp['shape']}")
            print("  Outputs:")
            for out in metadata[method]["outputs"]:
                print(f"   - {out['name']} ({out['type']}) Shape: {out['shape']}")
            print()

def export_all_models(source_dir: str, output_dir: str, target_class: str = "FloodModelCA"):
    """
    Finds model definitions in the source directory, exports them to ONNX, 
    and generates a JSON metadata file for C++ integration.
    """
    os.makedirs(output_dir, exist_ok=True)
    search_pattern = os.path.join(source_dir, "*.py")
    py_files = [f for f in glob.glob(search_pattern) if not f.endswith("__init__.py")]

    if not py_files:
        print(f"No Python models found in {source_dir}")
        return

    for filepath in py_files:
        filename = os.path.basename(filepath)
        model_name = os.path.splitext(filename)[0]
        model_export_path = os.path.join(output_dir, model_name)
        
        print(f"\nProcessing '{model_name}'...")

        # 1. Dynamically load the module
        spec_import = importlib.util.spec_from_file_location(model_name, filepath)
        module = importlib.util.module_from_spec(spec_import)
        spec_import.loader.exec_module(module)

        # 2. Retrieve the model class
        try:
            model_class = getattr(module, target_class)
        except AttributeError:
            print(f"  [SKIPPED] Class '{target_class}' not found in {filename}.")
            continue

        # 3. Instantiate the model
        model_instance = model_class()

        # Validate that required methods and signatures exist
        if not hasattr(model_instance, "preprocess") or not hasattr(model_instance, "step"):
            print(f"  [ERROR] Model '{model_name}' is missing 'preprocess' or 'step' methods.")
            continue

        # Extraer el nombre de la capa de fluido dinámicamente
        fluid_layer_name = "unknown"
        if hasattr(model_instance, "get_fluid_layer_name"):
            fluid_layer_name = model_instance.get_fluid_layer_name()
        else:
            print(f"  [ERROR] Model '{model_name}' is missing 'get_fluid_layer_name'. Defaulting to 'unknown'.")

        fluid_movement_state_layer_name = "unknown"
        if hasattr(model_instance, "get_fluid_movement_state_layer_name"):
            fluid_movement_state_layer_name = model_instance.get_fluid_movement_state_layer_name()
        else:
            print(f"  [ERROR] Model '{model_name}' is missing 'get_fluid_movement_state_layer_name'. Defaulting to 'unknown'.")

        os.makedirs(model_export_path, exist_ok=True)

        model_metadata = {
            "model_name": model_name,
            "fluid_layer": fluid_layer_name,
            "fluid_movement_state_layer": fluid_movement_state_layer_name
        }

        try:
            # ==========================================
            # Export Preprocess Graph
            # ==========================================
            preprocess_onnx_path = os.path.join(model_export_path, "preprocess_model.onnx")
            model_proto_preprocess, _ = tf2onnx.convert.from_function(
                function=model_instance.preprocess,
                input_signature=model_instance.preprocess.input_signature,
                opset=15,
                output_path=preprocess_onnx_path
            )
            model_metadata["preprocess"] = generate_graph_metadata(model_proto_preprocess)

            # ==========================================
            # Export Step Graph
            # ==========================================
            step_onnx_path = os.path.join(model_export_path, "step_model.onnx")
            model_proto_step, _ = tf2onnx.convert.from_function(
                function=model_instance.step,
                input_signature=model_instance.step.input_signature,
                opset=15,
                output_path=step_onnx_path
            )
            model_metadata["step"] = generate_graph_metadata(model_proto_step)

            # ==========================================
            # Generate JSON Metadata
            # ==========================================
            json_metadata_path = os.path.join(model_export_path, "metadata.json")
            with open(json_metadata_path, "w", encoding="utf-8") as json_file:
                json.dump(model_metadata, json_file, indent=4)

            # Output success and summary
            print_summary(model_metadata)

        except Exception as e:
            print(f"  [ERROR] Failed to export {model_name}: {e}")


if __name__ == "__main__":
    # Resolve absolute paths
    script_dir = os.path.dirname(os.path.abspath(__file__))
    source_directory = os.path.abspath(os.path.join(script_dir, "versions"))
    output_directory = os.path.abspath(os.path.join(script_dir, "../../exported_models"))

    print("Starting automated ONNX export pipeline...")
    export_all_models(source_dir=source_directory, output_dir=output_directory)

    # CREATE SYNCHRONIZATION TIMESTAMP FOR CMAKE
    timestamp_path = os.path.join(output_directory, ".export_timestamp")
    with open(timestamp_path, "w") as f:
        f.write("Synchronized")

    print(f"\nPipeline finished. All models exported to: {output_directory}")
