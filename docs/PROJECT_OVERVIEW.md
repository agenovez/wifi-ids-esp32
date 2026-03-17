# Descripción general del proyecto

Este proyecto implementa un prototipo de IDS WiFi basado en ESP32 para detectar ráfagas de tramas de desautenticación (IEEE 802.11). El sensor opera en modo promiscuo, aplica una ventana temporal con umbral y envía eventos al plano de monitoreo usando MQTT.

## Flujo técnico

1. El ESP32 monitorea el canal WiFi configurado.
2. Si detecta múltiples tramas deauth dentro de una ventana de 10 s, genera una alerta.
3. La alerta se publica como JSON en Mosquitto.
4. Graylog procesa el evento para visualización y análisis.

## Alcance

- Detección defensiva de deauthentication flooding.
- Integración con MQTT y Graylog.
- Entorno de laboratorio y uso académico.
