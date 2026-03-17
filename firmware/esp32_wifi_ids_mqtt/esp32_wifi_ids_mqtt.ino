#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <esp_wifi.h>

// =============================
// Pantalla OLED SH1106
// =============================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET   -1
#define OLED_ADDR    0x3C
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// =============================
// Configuración red
// =============================
const char* WIFI_SSID   = "LAB-ESP32-SEGURIDAD";
const char* WIFI_PASS   = "password_lab";
const char* MQTT_BROKER = "192.168.1.60";
const uint16_t MQTT_PORT = 1883;
const char* SENSOR_ID = "esp32-deauth-01";
const char* MQTT_TOPIC = "wifi/deauth/esp32-deauth-01";

// Canal a monitorear (ajustar según AP)
const uint8_t WIFI_CHANNEL = 6;

// =============================
// Detección
// =============================
constexpr uint32_t THRESHOLD = 20;
constexpr uint32_t WINDOW_SECONDS = 10;
constexpr uint32_t WINDOW_MS = WINDOW_SECONDS * 1000UL;
volatile uint32_t deauthCount = 0;
uint32_t windowStart = 0;
uint32_t lastDisplayUpdate = 0;
uint32_t lastWifiRetry = 0;
uint32_t lastMqttRetry = 0;
constexpr uint32_t DISPLAY_REFRESH_MS = 1000;
constexpr uint32_t WIFI_RETRY_INTERVAL_MS = 5000;
constexpr uint32_t MQTT_RETRY_INTERVAL_MS = 5000;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// =============================
// Estructuras 802.11 mínimas
// =============================
typedef struct {
  uint16_t frame_ctrl;
  uint16_t duration_id;
  uint8_t addr1[6];
  uint8_t addr2[6];
  uint8_t addr3[6];
  uint16_t sequence_ctrl;
  uint8_t payload[0];
} wifi_ieee80211_mac_hdr_t;

typedef struct {
  wifi_ieee80211_mac_hdr_t hdr;
  uint8_t payload[0];
} wifi_ieee80211_packet_t;

String macToString(const uint8_t* mac) {
  char buf[18];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}

void clearAndHeader(const char* title, bool isAlert = false) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextColor(SH110X_WHITE);
  if (isAlert) {
    display.setTextSize(2);
    display.println("ALERTA");
    display.setTextSize(1);
  } else {
    display.setTextSize(1);
    display.println(title);
  }
  display.println("----------------");
}

void updateDisplayStatus(const char* line1, const char* line2 = "", bool isAlert = false) {
  clearAndHeader(isAlert ? "ALERTA" : "Estado del sistema", isAlert);
  display.println(line1);
  if (strlen(line2) > 0) display.println(line2);
  display.display();
}

void updateDisplayMonitoring(uint32_t count) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.println("Monitoreando WiFi");
  display.println("----------------");
  display.print("Sensor: "); display.println(SENSOR_ID);
  display.print("Canal: "); display.println(WIFI_CHANNEL);
  display.print("WiFi: "); display.println(WiFi.status() == WL_CONNECTED ? "OK" : "DOWN");
  display.print("MQTT: "); display.println(mqttClient.connected() ? "OK" : "DOWN");
  display.print("Deauths: "); display.println(count);
  uint32_t elapsed = (millis() - windowStart) / 1000UL;
  display.print("Ventana: "); display.print(elapsed); display.print("/"); display.print(WINDOW_SECONDS); display.println(" s");
  display.display();
}

void beginWiFiConnection() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  updateDisplayStatus("Conectando WiFi...", WIFI_SSID);
}

void ensureWiFiConnected() {
  if (WiFi.status() == WL_CONNECTED) return;
  uint32_t now = millis();
  if (now - lastWifiRetry >= WIFI_RETRY_INTERVAL_MS) {
    lastWifiRetry = now;
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    updateDisplayStatus("Reintentando WiFi...", WIFI_SSID);
  }
}

void ensureMQTTConnected() {
  if (WiFi.status() != WL_CONNECTED || mqttClient.connected()) return;
  uint32_t now = millis();
  if (now - lastMqttRetry < MQTT_RETRY_INTERVAL_MS) return;
  lastMqttRetry = now;
  updateDisplayStatus("Conectando MQTT...", MQTT_BROKER);
  if (mqttClient.connect(SENSOR_ID)) {
    updateDisplayStatus("MQTT conectado", SENSOR_ID);
  } else {
    updateDisplayStatus("MQTT fallo", "Reintentando...");
  }
}

void publishDeauthEvent(uint32_t count, const String& src = "", const String& dst = "", const String& bssid = "") {
  StaticJsonDocument<384> doc;
  doc["event_type"] = "wifi_deauth";
  doc["sensor_id"] = SENSOR_ID;
  doc["count"] = count;
  doc["severity"] = "critical";
  doc["window_s"] = WINDOW_SECONDS;
  doc["threshold"] = THRESHOLD;
  doc["wifi_rssi"] = WiFi.RSSI();
  doc["ip"] = WiFi.localIP().toString();
  doc["channel"] = WIFI_CHANNEL;
  if (src.length()) doc["src"] = src;
  if (dst.length()) doc["dst"] = dst;
  if (bssid.length()) doc["bssid"] = bssid;

  char payload[384];
  size_t len = serializeJson(doc, payload);
  bool ok = mqttClient.publish(MQTT_TOPIC, payload, len);
  Serial.println(payload);
  updateDisplayStatus("Ataque detectado", ok ? "Evento enviado" : "Fallo MQTT", true);
}

void wifiSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT) return;

  const wifi_promiscuous_pkt_t* ppkt = (wifi_promiscuous_pkt_t*)buf;
  const wifi_ieee80211_packet_t* ipkt = (wifi_ieee80211_packet_t*)ppkt->payload;
  const wifi_ieee80211_mac_hdr_t* hdr = &ipkt->hdr;

  uint16_t fc = hdr->frame_ctrl;
  uint8_t frameType = (fc & 0x0c) >> 2;
  uint8_t subType   = (fc & 0xf0) >> 4;

  // Gestión + Deauthentication (subtype 0x0C)
  if (frameType == 0 && subType == 12) {
    deauthCount++;
  }
}

void setupPromiscuousMode() {
  esp_wifi_set_promiscuous(false);
  esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
  wifi_promiscuous_filter_t filt;
  filt.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT;
  esp_wifi_set_promiscuous_filter(&filt);
  esp_wifi_set_promiscuous_rx_cb(&wifiSnifferCallback);
  esp_wifi_set_promiscuous(true);
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Wire.begin();

  if (!display.begin(OLED_ADDR, true)) {
    while (true) delay(1000);
  }
  display.setRotation(0);
  display.setTextWrap(true);
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);

  updateDisplayStatus("Inicializando...", SENSOR_ID);
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setBufferSize(512);

  beginWiFiConnection();
  setupPromiscuousMode();
  windowStart = millis();
}

void loop() {
  ensureWiFiConnected();
  if (WiFi.status() == WL_CONNECTED) ensureMQTTConnected();
  if (mqttClient.connected()) mqttClient.loop();

  uint32_t now = millis();
  if (now - windowStart >= WINDOW_MS) {
    uint32_t count = deauthCount;
    deauthCount = 0;
    windowStart = now;
    if (count >= THRESHOLD) {
      publishDeauthEvent(count);
    }
  }

  if (now - lastDisplayUpdate >= DISPLAY_REFRESH_MS) {
    lastDisplayUpdate = now;
    updateDisplayMonitoring(deauthCount);
  }
  delay(50);
}
