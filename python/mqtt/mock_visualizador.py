
import paho.mqtt.client as mqtt
import json
import time

# --- Configuración ---
BROKER = "100.106.184.68"
PORT = 1883
scenarioName = "scenario_29_10_2024"

# Topics
TOPIC_PING = f"FloodSim/{scenarioName}/system/handshake/ping"
TOPIC_PONG = f"FloodSim/{scenarioName}/system/handshake/pong"
TOPIC_EVENTS = f"FloodSim/{scenarioName}/events" # Nuevo topic para InitConfig

# Variables de estado
chunks_recibidos = 0
celdas_totales_recibidas = 0

# --- Callbacks de MQTT ---
def on_connect(client, userdata, flags, rc, properties=None):
    if rc == 0:
        print(f"[✅] Conectado exitosamente al broker MQTT en {BROKER}:{PORT}")
        
        # Suscripciones
        client.subscribe(TOPIC_PING, qos=1)
        client.subscribe(TOPIC_EVENTS, qos=1)
        
        print(f"[*] Suscrito a '{TOPIC_PING}'")
        print(f"[*] Suscrito a '{TOPIC_EVENTS}'")
        print("[*] Esperando mensajes del Core C++...\n")
    else:
        print(f"[❌] Error de conexión. Código de retorno: {rc}")

def on_message(client, userdata, msg):
    global chunks_recibidos, celdas_totales_recibidas
    
    try:
        payload_str = msg.payload.decode('utf-8')
        data = json.loads(payload_str)
        process = data.get("process")
        
        # --- Lógica de Handshake ---
        if process == "System_Ping":
            print(f"\n[📥 PING] Recibido en '{msg.topic}'")
            time.sleep(0.2) 
            pong_payload = {
                "process": "System_Pong",
                "source": "Mock_Test_Python"
            }
            client.publish(TOPIC_PONG, json.dumps(pong_payload), qos=1)
            print(f"[📤 PONG] Enviado a '{TOPIC_PONG}'")
            
        # --- Lógica de Inicialización (setInitConfig) ---
        elif process == "InitMap_Config":
            print(f"\n[🗺️ INIT] InitMap_Config recibido:\n{json.dumps(data, indent=2)}")
            
        elif process == "InitAgent_Layer":
            print(f"\n[🗂️ INIT] InitAgent_Layer recibido:\n{json.dumps(data, indent=2)}")
            
        elif process == "InitAgent_EOF":
            print(f"\n[🏁 INIT] InitAgent_EOF recibido.")
            
        elif process == "EYE_SetState_Layer":
            # Para evitar saturar la consola, solo contamos las celdas de los chunks
            celdas_en_chunk = len(data.get("changes", {}).get("cells", {}))
            chunks_recibidos += 1
            celdas_totales_recibidas += celdas_en_chunk
            print(f"[🧩 CHUNK] Chunk {chunks_recibidos} recibido con {celdas_en_chunk} celdas. (Total: {celdas_totales_recibidas})")
            
        elif process == "Init_EOF":
            print(f"\n[✅ INIT] Init_EOF recibido:\n{json.dumps(data, indent=2)}")
            print("\n🎉 ¡Secuencia de inicialización completada con éxito!")
            
        else:
            print(f"\n[❓ DESCONOCIDO] Mensaje no reconocido ({process}):\n    {payload_str[:200]}...")

    except json.JSONDecodeError:
        print(f"[❌] Error: El payload en {msg.topic} no es un JSON válido.")
    except Exception as e:
        print(f"[❌] Error inesperado al procesar el mensaje: {e}")

# --- Configuración e Inicialización del Cliente ---
print("Arrancando el Mock del Visualizador (Modo InitConfig Test)...")

try:
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="Python_Mock")
except AttributeError:
    client = mqtt.Client(client_id="Python_Mock")

client.on_connect = on_connect
client.on_message = on_message

try:
    client.connect(BROKER, PORT, 60)
    client.loop_forever()
except KeyboardInterrupt:
    print("\n[🛑] Deteniendo el script de prueba (Ctrl+C).")
    client.disconnect()
except Exception as e:
    print(f"\n[❌] No se pudo conectar al broker Mosquitto: {e}")
