# WiFi IDS ESP32

Sistema de detección de intrusiones WiFi basado en ESP32, con envío de eventos vía MQTT y centralización en Graylog. El proyecto está orientado a detección defensiva de ataques de desautenticación en entornos de laboratorio controlados.

## Componentes

- **ESP32** como sensor IDS WiFi
- **OLED SH1106** opcional para estado local
- **Mosquitto** como broker MQTT
- **Graylog 7.x** como SIEM y visor de eventos
- **Docker Compose** para levantar la infraestructura de laboratorio

## Estructura

- `firmware/esp32_wifi_ids_mqtt/`: firmware del sensor ESP32
- `docker-compose.yml`: stack de laboratorio
- `mosquitto/config/`: configuración del broker
- `docs/`: arquitectura, metodología y diagramas PlantUML
- `scripts/`: pruebas rápidas y utilidades

## Requisitos

### Firmware
- ESP32 DevKit / ESP32-WROOM-32
- Arduino IDE 2.x o PlatformIO
- Librerías Arduino:
  - Adafruit GFX Library
  - Adafruit SH110X
  - Adafruit BusIO
  - PubSubClient
  - ArduinoJson

### Infraestructura
- Docker Engine
- Docker Compose

## Despliegue rápido

```bash
cp .env.example .env
docker compose up -d
```

### Servicios publicados
- Graylog: `http://localhost:9000`
- Mosquitto MQTT: `tcp://localhost:1883`
- GELF UDP: `12201/udp`

## Configuración de Graylog

1. Ingresar a Graylog.
2. Crear un input **GELF UDP** en el puerto `12201`.
3. Crear un stream opcional con regla `event_type == wifi_deauth`.

## Prueba rápida

```bash
python3 scripts/publish_test_event.py
```

Luego, en Graylog buscar:

```text
event_type:wifi_deauth
```

## Notas de laboratorio

- El firmware está preparado para **detección** de tramas de desautenticación en modo promiscuo.
- Debe usarse **solo** en entornos controlados y autorizados.
- El proyecto no implementa bloqueo activo ni explotación ofensiva.

## Licencia

MIT
