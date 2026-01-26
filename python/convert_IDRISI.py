
import numpy as np

INPUT_TXT = "MDH_Recortado.txt"
# Definimos el nombre base para los archivos de salida (sin extensión)
OUTPUT_BASE = "MDH_Recortado"

print("Cargando matriz VV...")
A = np.loadtxt(INPUT_TXT, dtype=float)
print("Dimensiones:", A.shape)

# Guardamos las dimensiones para el archivo de cabecera (.rdc)
rows, cols = A.shape

# 1. PROCESAMIENTO DE DATOS
# ------------------------------------------------
# Guardamos dónde está el mar (valores 0 en el original)
mar_mask = (A == 0)

A_db = A.copy()
# Evitamos log(0) o log(negativos) poniendo NaN
A_db[A_db <= 0] = np.nan

VV_dB = 10 * np.log10(A_db)

print("Rango VV en dB:")
print("  Min:", np.nanmin(VV_dB))
print("  Max:", np.nanmax(VV_dB))

UMBRAL_DB = -21.0

agua_mask = (VV_dB < UMBRAL_DB)

# Combinamos máscaras (Agua por umbral O zona de mar)
mask_final = (agua_mask | mar_mask)

# 2. CREACIÓN DE LA MATRIZ DE SALIDA (0.0 y 0.5)
# ------------------------------------------------
# Creamos una matriz de ceros tipo float32 (Real)
# IDRISI trabaja mejor con float32 para archivos 'real'
salida = np.zeros(A.shape, dtype=np.float32)

# Asignamos 0.5 donde la máscara es verdadera (antes eran 1s)
salida[mask_final] = 0.5

# 3. GUARDADO EN FORMATO IDRISI (.rst y .rdc)
# ------------------------------------------------
archivo_rst = OUTPUT_BASE + ".rst"
archivo_rdc = OUTPUT_BASE + ".rdc"

# A) Guardar archivo de datos binario (.rst)
print(f"Generando archivo binario {archivo_rst}...")
salida.tofile(archivo_rst)

# B) Guardar archivo de documentación (.rdc)
print(f"Generando archivo de cabecera {archivo_rdc}...")
with open(archivo_rdc, "w") as f:
    f.write("file format : IDRISI Raster A.1\n")
    f.write(f"file title  : Mascara agua (0.0 y 0.5)\n")
    f.write("data type   : real\n")      # 'real' porque usamos decimales (0.5)
    f.write("file type   : binary\n")
    f.write(f"columns     : {cols}\n")
    f.write(f"rows        : {rows}\n")
    f.write("ref. system : plane\n")     # Sistema genérico plano
    f.write("ref. units  : m\n")
    f.write("unit dist.  : 1.0\n")
    f.write("min. X      : 0\n")
    f.write(f"max. X      : {cols}\n")
    f.write("min. Y      : 0\n")
    f.write(f"max. Y      : {rows}\n")
    f.write("pos'n error : unknown\n")
    f.write("resolution  : 1.0\n")
    f.write("min. value  : 0.0\n")
    f.write("max. value  : 0.5\n")
    f.write("display min : 0.0\n")
    f.write("display max : 0.5\n")
    f.write("value units : classes\n")
    f.write("value error : unknown\n")
    f.write("flag value  : none\n")
    f.write("flag def'n  : none\n")
    f.write("legend cats : 0\n")

print("------------------------------------------------")
print("Archivos GIS creados correctamente")
print(f"Criterio: VV < {UMBRAL_DB} dB + mar")
print(f"Archivos generados: {archivo_rst}, {archivo_rdc}")
print("Valores: 0.0 (Tierra/No agua) y 0.5 (Agua)")
print("------------------------------------------------")
 
