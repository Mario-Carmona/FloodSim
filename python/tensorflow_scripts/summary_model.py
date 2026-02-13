import onnx
from collections import Counter

# Carga tu modelo
model_path = "models/simulation_model_cpu.onnx"
model = onnx.load(model_path)

# Contador de tipos de nodos
op_counts = Counter()
for node in model.graph.node:
    op_counts[node.op_type] += 1

print("--- Resumen del Grafo Principal ---")
print(op_counts)

# Si hay bucles (Loop), la lógica pesada está DENTRO del nodo Loop.
# Vamos a intentar mirar dentro de los subgrafos (el cuerpo del bucle).
total_subgraph_ops = Counter()
for node in model.graph.node:
    if node.op_type == "Loop":
        # El cuerpo del bucle suele estar en el atributo 'body'
        for attr in node.attribute:
            if attr.name == "body":
                for sub_node in attr.g.node:
                    total_subgraph_ops[sub_node.op_type] += 1

print("\n--- Resumen DENTRO del Bucle (La física real) ---")
print(total_subgraph_ops)