#!/usr/bin/env python3
"""Bridge opcional MQTT -> GELF UDP para Graylog.
Usar solo si prefiere GELF en lugar de un input MQTT directo o un pipeline externo.
"""
import json, socket
from paho.mqtt import client as mqtt

MQTT_HOST = "127.0.0.1"
MQTT_PORT = 1883
MQTT_TOPIC = "wifi/deauth/#"
GELF_HOST = "127.0.0.1"
GELF_PORT = 12201

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

def on_message(client, userdata, msg):
    try:
        data = json.loads(msg.payload.decode())
    except Exception as exc:
        print(f"Payload inválido: {exc}")
        return

    gelf = {
        "version": "1.1",
        "host": data.get("sensor_id", "esp32"),
        "short_message": "WiFi deauthentication event detected",
        "level": 4,
        **data,
    }
    sock.sendto(json.dumps(gelf).encode(), (GELF_HOST, GELF_PORT))
    print("Reenviado a Graylog:", gelf)

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.on_message = on_message
client.connect(MQTT_HOST, MQTT_PORT)
client.subscribe(MQTT_TOPIC)
print(f"Escuchando {MQTT_TOPIC} y reenviando a GELF {GELF_HOST}:{GELF_PORT}")
client.loop_forever()
