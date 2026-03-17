# Resultados simulados para documentación

## Consola serial del ESP32

```text
[BOOT] Inicializando sensor IDS WiFi...
[OLED] SH1106 listo
[WiFi] Conectando a LAB-ESP32-SEGURIDAD
[WiFi] Conectado - IP: 192.168.1.101
[MQTT] Broker: 192.168.1.60:1883
[MQTT] Conectado
[INFO] Monitoreando canal 6
[INFO] Deauths en ventana: 4
[INFO] Deauths en ventana: 7
[ALERTA] Umbral excedido
[ALERTA] Evento publicado en wifi/deauth/esp32-deauth-01
```

## Evento JSON esperado

```json
{
  "event_type": "wifi_deauth",
  "sensor_id": "esp32-deauth-01",
  "count": 27,
  "severity": "critical",
  "window_s": 10,
  "threshold": 20,
  "wifi_rssi": -48,
  "ip": "192.168.1.101",
  "channel": 6
}
```
