# -*- coding: utf-8 -*-

import os
import re

def load_simulation_constants():
    """
    Busca el archivo SimulationConstants.hpp y extrae las constantes.
    Devuelve un diccionario con los nombres y valores.
    """
    # 1. Definir la ruta relativa al archivo C++
    # Ajusta esta ruta según donde coloques este script
    base_dir = os.path.dirname(os.path.abspath(__file__))
    # Subimos niveles hasta encontrar src (ajusta '../..' según tu estructura)
    hpp_path = os.path.join(base_dir, "../../include/core/common/SimulationConstants.hpp")
    
    constants = {}

    if not os.path.exists(hpp_path):
        raise FileNotFoundError(f"No se encuentra el archivo de constantes en: {hpp_path}")

    # 2. Regex para capturar: static constexpr TIPO NOMBRE = VALOR;
    # Soporta int8_t y float
    regex_int = re.compile(r'static\s+constexpr\s+int8_t\s+(\w+)\s*=\s*(\d+);')
    regex_float = re.compile(r'static\s+constexpr\s+float\s+(\w+)\s*=\s*([\d\.e\-]+)f?;')

    with open(hpp_path, 'r') as f:
        for line in f:
            line = line.strip()
            
            # Intentar match con ENTEROS (Estados)
            match_int = regex_int.search(line)
            if match_int:
                name, val = match_int.groups()
                constants[name] = int(val)
                continue

            # Intentar match con FLOATS (Epsilon)
            match_float = regex_float.search(line)
            if match_float:
                name, val = match_float.groups()
                constants[name] = float(val)

    return constants
