# -*- coding: utf-8 -*-

import os
import sys
import shutil
import subprocess
import re
import tensorflow as tf

from tensorflow_scripts.simulator_module import SimulatorModule


def parse_tf_cli_output(text):
    """Analiza la salida de saved_model_cli para extraer firmas."""
    signatures = text.split("signature_def['")
    parsed_data = {}

    for sig in signatures[1:]:
        sig_name = sig.split("']")[0]
        parsed_data[sig_name] = {'inputs': [], 'outputs': []}
        
        # Regex para capturar: nombre_python, nombre_nodo_tf, indice
        # Inputs
        input_matches = re.finditer(r"inputs\['(.*?)'\].*?name: (.*?):(\d+)", sig, re.DOTALL)
        for m in input_matches:
            parsed_data[sig_name]['inputs'].append((m.group(1), m.group(2), m.group(3)))

        # Outputs
        output_matches = re.finditer(r"outputs\['(.*?)'\].*?name: (.*?):(\d+)", sig, re.DOTALL)
        for m in output_matches:
            parsed_data[sig_name]['outputs'].append((m.group(1), m.group(2), m.group(3)))
            
    return parsed_data

def print_cpp_guide(data):
    """Print the formatted table for C++."""
    print("\n   INTEGRATION GUIDE TENSORFLOW -> C++")
    print("   " + "-"*74)
    
    for method, content in data.items():
        if method == "__saved_model_init_op": continue
        
        print(f"\n   >> METHOD: '{method}'")
        print("   " + "-" * 74)
        print(f"   {'TYPE':<8} | {'PYTHON KEY':<15} | {'C++ NODE NAME':<35} | {'IDX':<3}")
        print("   " + "-" * 74)
        
        for py, node, idx in content['inputs']:
            print(f"   {'INPUT':<8} | {py:<15} | {node:<35} | {idx:<3}")
            
        for py, node, idx in content['outputs']:
            print(f"   {'OUTPUT':<8} | {py:<15} | {node:<35} | {idx:<3}")
        print("   " + "-" * 74 + "\n")


if __name__ == "__main__":
    # --- EJECUCIÓN ---

    # 2. Imprimir Bloque Encapsulado
    print("\n" + "#"*80)

    output_dir = 'models'
    if os.path.exists(output_dir): shutil.rmtree(output_dir)
    
    module = SimulatorModule()


    # --- 1. DEFINIR FIRMAS (SIGNATURES) ---
    
    # INIT: Acepta configuración + steps_batch
    c_init = module.initialize_state.get_concrete_function()
    
    # RUN_BATCH: Solo acepta el estado actual. El número de pasos ya está guardado dentro.
    c_run = module.run_simulation_batch.get_concrete_function()

    c_render = module.get_view_changes.get_concrete_function()


    # --- 2. GUARDAR MODELO ---
    tf.saved_model.save(module, output_dir, signatures={
        'init': c_init,
        'run_batch': c_run,
        'render': c_render
    })

    print(f"   SavedModel exported to '{output_dir}'")

    print("#"*80)

    try:
        # 1. Ejecutar el comando CLI de TensorFlow
        cmd = [
            sys.executable, "-m", "tensorflow.python.tools.saved_model_cli", 
            "show", "--dir", output_dir, "--all"
        ]

        result = subprocess.run(cmd, capture_output=True, text=True)
    
        if result.returncode == 0:
            data = parse_tf_cli_output(result.stdout)
            print_cpp_guide(data)
        else:
            print("\n   [ERROR] No se pudo inspeccionar el modelo:")
            print(result.stderr)

    except Exception as e:
        print(f"\n   [EXCEPCIÓN] Fallo al ejecutar saved_model_cli: {e}")

    print("#"*80 + "\n")
