
import tensorflow as tf
from tensorflow import keras
from tensorflow.keras.layers import Input, Lambda, Concatenate, Add, Multiply, Subtract, ReLU
import tf2onnx
import numpy as np
import sys

# Función de Operación de Desplazamiento (Roll)
def roll_layer(x, shift_y, shift_x):
    # La operación Roll en TF es tf.roll
    return Lambda(lambda t: tf.roll(t, shift=(shift_y, shift_x), axis=(1, 2)), 
                  name=f"roll_{shift_y}_{shift_x}")(x)

# 2. Replicación del Grafo buildGraph() adaptada
def create_ca_model(rows, cols, time_step):
    # // Input: [1, H, W, 2] -> Canal 0: Elev, Canal 1: Agua
    # inputStatePH = Placeholder(*rootScope, DT_FLOAT, Placeholder::Attrs().Shape({1, rows, cols, 2}));
        
    # auto elev = Slice(*rootScope, inputStatePH, {0,0,0,0}, {1, rows, cols, 1});
    # auto water = Slice(*rootScope, inputStatePH, {0,0,0,1}, {1, rows, cols, 1});
    # auto head = Add(*rootScope, elev, water);

    # std::vector<Output> flows_out;
    # int shifts_y[] = {-1, -1, -1,  0, 0,  1, 1, 1};
    # int shifts_x[] = {-1,  0,  1, -1, 1, -1, 0, 1};
    # float dists[]  = {1.414, 1.0, 1.414, 1.0, 1.0, 1.414, 1.0, 1.414}; 

    # auto zero = Const(*rootScope, 0.0f);
    # auto dt = Const(*rootScope, (float)config.timeStep);
    # auto two_g = Const(*rootScope, 2.0f * GRAVITY);

    # for(int k=0; k<8; ++k) {
    #     // 1. Shift para alinear vecino
    #     auto neighbor_head = Roll(*rootScope, head, {shifts_y[k], shifts_x[k]}, {1, 2});
            
    #     // 2. Diff y Gradiente
    #     auto diff = Sub(*rootScope, head, neighbor_head);
    #     auto gradient = Maximum(*rootScope, diff, zero); // ReLU
            
    #     // 3. Torricelli: v = sqrt(2gh)
    #     auto vel = Sqrt(*rootScope, Multiply(*rootScope, two_g, gradient));
            
    #     // 4. Flujo Potencial
    #     auto flow = Multiply(*rootScope, vel, water);
    #     flow = Multiply(*rootScope, flow, Const(*rootScope, (float)config.timeStep / dists[k]));
    #     flow = Multiply(*rootScope, flow, Const(*rootScope, 0.1f)); // Fricción / Estabilidad
            
    #     flows_out.push_back(flow);
    # }

    # // 5. Balance de Masas (Limiter)
    # auto sum_out = AddN(*rootScope, flows_out);
    # auto safe_water = Maximum(*rootScope, water, Const(*rootScope, 1e-5f));
    # auto fraction = Div(*rootScope, safe_water, Add(*rootScope, sum_out, safe_water));
    # auto limiter = Minimum(*rootScope, fraction, Const(*rootScope, 1.0f));

    # // 6. Calcular Flujos Netos
    # std::vector<Output> net_fluxes;
    # for(int k=0; k<8; ++k) {
    #     auto out_f = Multiply(*rootScope, flows_out[k], limiter);
    #     // Inflow es el Outflow del vecino en dirección opuesta
    #     auto in_f = Roll(*rootScope, out_f, {-shifts_y[k], -shifts_x[k]}, {1, 2});
    #     net_fluxes.push_back(Sub(*rootScope, in_f, out_f));
    # }

    # auto net_change = AddN(*rootScope, net_fluxes);
    # auto new_water = Add(*rootScope, water, net_change);
    # new_water = Maximum(*rootScope, new_water, zero); // Clamp 0

    # evolvedStateOp = Concat(*rootScope, 3, {elev, new_water});
    
    # El cambio clave: El shape de la entrada ahora usa las variables `rows` y `cols`
    input_state = Input(shape=(rows, cols, 2), dtype=tf.float32, name="input_state")
    
    # ... (Resto de la lógica del modelo, incluyendo Slice, Roll, Add, etc.) ...
    
    # ... (Se omite el cuerpo para brevedad, usa la lógica del ejemplo anterior) ...

    # Ejemplo de la salida final
    evolved_state = Concatenate(axis=-1, name="evolved_state")([elev, new_water])
    model = keras.Model(inputs=input_state, outputs=evolved_state)
    return model

# --- 3. EXPORTACIÓN DEL MODELO ---
def export_model(model, output_filename):
    print(f"Iniciando exportación a ONNX como {output_filename}...")
    
    # Convertir a ONNX y guardarlo directamente
    model_proto, _ = tf2onnx.convert.from_keras(model, 
                                                opset=13, # Especificar un opset estable
                                                output_path=output_filename)
    
    print(f"✅ Modelo exportado exitosamente a {output_filename}")


if __name__ == '__main__':
    # Verificar argumentos
    if len(sys.argv) < 4:
        print("Uso: python generate_onnx.py <rows> <cols> <time_step>")
        sys.exit(1)

    # Leer argumentos desde la línea de comandos
    try:
        ROWS = int(sys.argv[1])
        COLS = int(sys.argv[2])
        TIME_STEP = float(sys.argv[3])
    except ValueError:
        print("Error: Los argumentos deben ser números enteros o flotantes.")
        sys.exit(1)

    OUTPUT_FILE = "ca_model_dynamic.onnx"

    # 1. Crear el modelo con las dimensiones dinámicas
    ca_model = create_ca_model(rows=ROWS, cols=COLS, time_step=TIME_STEP)

    # 2. Exportar
    export_model(ca_model, OUTPUT_FILE)
