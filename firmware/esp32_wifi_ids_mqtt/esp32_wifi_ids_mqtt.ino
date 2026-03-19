#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDR     0x3C
#define I2C_SDA       21
#define I2C_SCL       22

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

bool initOLED() {
  Wire.end();
  delay(50);
  Wire.begin(I2C_SDA, I2C_SCL);
  delay(300);

  for (int i = 0; i < 5; i++) {
    Serial.printf("Intentando OLED, intento %d...\n", i + 1);

    if (display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 0);
      display.println("GME12864-54 OK");
      display.println("Controlador:");
      display.println("SSD1315/SSD1306");
      display.display();
      return true;
    }

    delay(300);
  }

  return false;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Iniciando prueba OLED...");
  delay(800);  // estabilizacion de alimentacion

  if (!initOLED()) {
    Serial.println("No se pudo iniciar la OLED.");
    Serial.println("Pruebe direccion 0x3D si 0x3C no responde.");
    while (true) delay(1000);
  }

  Serial.println("OLED inicializada correctamente.");
}

void loop() {
}
