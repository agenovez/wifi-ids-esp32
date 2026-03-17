#!/usr/bin/env python3
import json
import time
import random
import paho.mqtt.publish as publish

payload = {
    "event_type": "wifi_deauth",
    "sensor_id": "esp32-deauth-01",
    "count": random.randint(21, 60),
    "severity": "critical",
    "window_s": 10,
    "threshold": 20,
    "wifi_rssi": -55,
    "ip": "192.168.1.101",
    "ts": int(time.time())
}

publish.single(
    topic="wifi/deauth/esp32-deauth-01",
    payload=json.dumps(payload),
    hostname="127.0.0.1",
    port=1883,
)
print("Evento MQTT publicado:")
print(json.dumps(payload, indent=2))
