#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET   -1        // No reset pin, share Arduino reset
#define SCREEN_ADDRESS 0x3C    // Change to 0x3D if not working

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(115200);
  delay(200);

  Serial.println("Booting...");

  // Initialize I2C on default ESP32 pins (SDA=21, SCL=22)
  Wire.begin(21, 22);

  // Init OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("SSD1306 allocation failed");
    for (;;);  // Halt
  }

  Serial.println("OLED initialized!");

  // Clear buffer
  display.clearDisplay();

  // Draw something
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Hello!");
  display.setTextSize(1);
  display.setCursor(0, 30);
  display.println("ESP32 + OLED");
  display.println("PlatformIO rocks");

  // Push buffer to screen
  display.display();
}

void loop() {
  // Nothing yet — will animate in next step
}