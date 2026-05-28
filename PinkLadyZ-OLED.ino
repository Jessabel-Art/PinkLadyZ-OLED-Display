#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "bitmaps.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "time.h"

// -------------------------
// WIFI SETTINGS
// -------------------------

#include "config.h"

// Eastern Time
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -18000;
const int daylightOffset_sec = 3600;

// -------------------------
// DISPLAY SETTINGS
// -------------------------

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// -------------------------
// BITMAP DEFINES
// -------------------------

#define CAR_SIDE          epd_bitmap_z32_side
#define CAR_ANGLE         epd_bitmap_z32_angle
#define CAR_FRONT         epd_bitmap_z32_front

#define CAR_REAR_OFF      epd_bitmap_z32_rear_lights_off
#define CAR_REAR_ON       epd_bitmap_z32_rear_lights_on

#define GARAGE_OPEN       epd_bitmap_garage_open
#define GARAGE_HALF       epd_bitmap_garage_half
#define GARAGE_CLOSED     epd_bitmap_garage_closed

#define DASH_FRAME        epd_bitmap_dashboard_frame
#define GLITCH_1          epd_bitmap_glitch_scene
#define GLITCH_2          epd_bitmap_glitch_scene_2
#define GLITCH_3          epd_bitmap_glitch_scene_3
#define HELLO_CARD        epd_bitmap_hello_pinkladyz

#define BITMAP_W 128
#define BITMAP_H 64

// -------------------------
// STATE SYSTEM
// -------------------------

enum DisplayState {
  STATE_BOOT,
  STATE_DASHBOARD,
  STATE_CRUISE_IDLE,
  STATE_NIGHT_IDLE,
  STATE_GARAGE_IDLE,
  STATE_SHUTDOWN
};

DisplayState currentState = STATE_BOOT;

unsigned long stateStartTime = 0;
unsigned long lastFrameTime = 0;
unsigned long lastWeatherUpdate = 0;
unsigned long lastTimeUpdate = 0;

const unsigned long weatherInterval = 15UL * 60UL * 1000UL;
const unsigned long timeInterval = 1000UL;

int frame = 0;

String currentTimeText = "--:--";
String weatherText = "SYNC";
String tempText = "--F";

bool isNight = false;
bool isRaining = false;
bool isCold = false;
bool isStorming = false;
bool wifiConnected = false;

// -------------------------
// WIFI + LIVE DATA
// -------------------------

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 30) {
    delay(300);
    tries++;
  }

  wifiConnected = WiFi.status() == WL_CONNECTED;

  if (wifiConnected) {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  }
}

void updateClock() {
  if (!wifiConnected) {
    currentTimeText = "--:--";
    return;
  }

  struct tm timeinfo;

  if (getLocalTime(&timeinfo)) {
    char timeBuffer[10];
    strftime(timeBuffer, sizeof(timeBuffer), "%I:%M%p", &timeinfo);
    currentTimeText = String(timeBuffer);

    int hour = timeinfo.tm_hour;
    isNight = (hour >= 20 || hour < 6);
  }
}

String decodeWeatherCode(int code) {
  if (code == 0) return "CLEAR";
  if (code == 1 || code == 2 || code == 3) return "CLOUDY";
  if (code == 45 || code == 48) return "FOG";
  if (code >= 51 && code <= 67) return "RAIN";
  if (code >= 71 && code <= 77) return "SNOW";
  if (code >= 80 && code <= 82) return "SHOWERS";
  if (code >= 95) return "STORM";

  return "WX?";
}

void fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) {
    wifiConnected = false;
    weatherText = "NO WIFI";
    return;
  }

  wifiConnected = true;

  HTTPClient http;

  String url = "https://api.open-meteo.com/v1/forecast?latitude=";
  url += String(latitude, 4);
  url += "&longitude=";
  url += String(longitude, 4);
  url += "&current=temperature_2m,weather_code,is_day&temperature_unit=fahrenheit";

  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();

    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      float temp = doc["current"]["temperature_2m"];
      int weatherCode = doc["current"]["weather_code"];
      int isDay = doc["current"]["is_day"];

      tempText = String((int)round(temp)) + "F";
      weatherText = decodeWeatherCode(weatherCode);

      isNight = (isDay == 0);
      isRaining = weatherText == "RAIN" || weatherText == "SHOWERS" || weatherText == "STORM";
      isCold = weatherText == "SNOW";
      isStorming = weatherText == "STORM";
    } else {
      weatherText = "JSON ERR";
    }
  } else {
    weatherText = "WX ERR";
  }

  http.end();
}

// -------------------------
// HELPERS
// -------------------------

void changeState(DisplayState newState) {
  currentState = newState;
  stateStartTime = millis();
  frame = 0;
  display.clearDisplay();
}

void centerText(String text, int y, int size = 1) {
  display.setTextSize(size);
  display.setTextColor(SSD1306_WHITE);

  int16_t x1, y1;
  uint16_t w, h;

  display.getTextBounds(text, 0, y, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, y);
  display.print(text);
}

void drawScanlines() {
  for (int y = 0; y < SCREEN_HEIGHT; y += 4) {
    display.drawFastHLine(0, y, SCREEN_WIDTH, SSD1306_WHITE);
  }
}

void drawStars(int offset) {
  for (int i = 0; i < 18; i++) {
    int x = (i * 17 + offset) % SCREEN_WIDTH;
    int y = (i * 9 + 3) % 30;
    display.drawPixel(x, y, SSD1306_WHITE);
  }
}

void drawRain(int offset) {
  for (int i = 0; i < 16; i++) {
    int x = (i * 13 + offset) % SCREEN_WIDTH;
    int y = (i * 9 + offset) % SCREEN_HEIGHT;
    display.drawLine(x, y, x - 2, y + 5, SSD1306_WHITE);
  }
}

void drawSnow(int offset) {
  for (int i = 0; i < 14; i++) {
    int x = (i * 13 + offset) % SCREEN_WIDTH;
    int y = (i * 8 + offset) % SCREEN_HEIGHT;
    display.drawPixel(x, y, SSD1306_WHITE);
    display.drawPixel(x + 1, y, SSD1306_WHITE);
  }
}

void drawMoon() {
  display.fillCircle(108, 12, 8, SSD1306_WHITE);
  display.fillCircle(112, 10, 8, SSD1306_BLACK);
}

void drawLightningFlash() {
  if ((frame % 45) < 4) {
    display.drawLine(76, 0, 62, 18, SSD1306_WHITE);
    display.drawLine(62, 18, 70, 18, SSD1306_WHITE);
    display.drawLine(70, 18, 54, 42, SSD1306_WHITE);
    display.drawLine(62, 18, 52, 28, SSD1306_WHITE);
    display.drawLine(70, 18, 84, 31, SSD1306_WHITE);
  }
}

void drawVaporGrid() {
  display.drawFastHLine(0, 48, 128, SSD1306_WHITE);
  display.drawLine(0, 63, 64, 48, SSD1306_WHITE);
  display.drawLine(128, 63, 64, 48, SSD1306_WHITE);

  for (int x = 16; x < 128; x += 16) {
    display.drawLine(x, 63, 64, 48, SSD1306_WHITE);
  }

  for (int y = 52; y < 64; y += 4) {
    display.drawFastHLine(0, y, 128, SSD1306_WHITE);
  }
}

// -------------------------
// BOOT SEQUENCE
// -------------------------

void runBootSequence() {
  unsigned long elapsed = millis() - stateStartTime;

  display.clearDisplay();

  if (elapsed < 1000) {
    if ((elapsed / 250) % 2 == 0) {
      display.fillRect(62, 30, 5, 8, SSD1306_WHITE);
    }
  }

  else if (elapsed < 2500) {
    centerText("INITIALIZING", 22, 1);
    centerText("SYSTEM...", 36, 1);

    if ((elapsed / 200) % 2 == 0) {
      display.fillRect(101, 36, 5, 7, SSD1306_WHITE);
    }
  }

  else if (elapsed < 3500) {
    drawScanlines();
    centerText("SYSTEM CHECK", 28, 1);
  }

  else if (elapsed < 4500) {
    display.drawBitmap(0, 0, CAR_SIDE, BITMAP_W, BITMAP_H, SSD1306_WHITE);
  }

  else if (elapsed < 5500) {
    display.drawBitmap(0, 0, CAR_ANGLE, BITMAP_W, BITMAP_H, SSD1306_WHITE);
  }

  else if (elapsed < 6500) {
    display.drawBitmap(0, 0, CAR_FRONT, BITMAP_W, BITMAP_H, SSD1306_WHITE);
  }

  else if (elapsed < 7300) {
    display.drawBitmap(0, 0, CAR_FRONT, BITMAP_W, BITMAP_H, SSD1306_WHITE);

    if ((elapsed / 120) % 2 == 0) {
      display.fillCircle(42, 39, 3, SSD1306_WHITE);
      display.fillCircle(86, 39, 3, SSD1306_WHITE);
    }
  }

  else if (elapsed < 8100) {
    display.drawBitmap(0, 0, GLITCH_1, BITMAP_W, BITMAP_H, SSD1306_WHITE);
  }

  else if (elapsed < 8600) {
    display.drawBitmap(0, 0, GLITCH_2, BITMAP_W, BITMAP_H, SSD1306_WHITE);
  }

  else if (elapsed < 9100) {
    display.drawBitmap(0, 0, GLITCH_3, BITMAP_W, BITMAP_H, SSD1306_WHITE);
  }

  else if (elapsed < 10800) {
    display.drawBitmap(0, 0, HELLO_CARD, BITMAP_W, BITMAP_H, SSD1306_WHITE);
  }

  else {
    changeState(STATE_DASHBOARD);
  }

  display.display();
}

// -------------------------
// DASHBOARD
// -------------------------

void runDashboard() {
  unsigned long elapsed = millis() - stateStartTime;

  display.clearDisplay();

  display.drawBitmap(0, 0, DASH_FRAME, BITMAP_W, BITMAP_H, SSD1306_WHITE);

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(6, 6);
  display.print(currentTimeText);

  display.setCursor(92, 6);
  display.print(tempText);

  centerText("PINKLADY-Z", 20, 1);

  display.setCursor(12, 36);
  display.print("WX:");
  display.print(weatherText);

  display.setCursor(12, 48);
  display.print("MODE:");
  display.print(isNight ? "NIGHT" : "DAY");

  display.display();

  if (elapsed > 4500) {
    changeState(isNight ? STATE_NIGHT_IDLE : STATE_CRUISE_IDLE);
  }
}

// -------------------------
// IDLE ANIMATIONS
// -------------------------

void runCruiseIdle() {
  display.clearDisplay();

  int offset = frame * 2;

  drawStars(offset);
  display.drawBitmap(0, 0, CAR_SIDE, BITMAP_W, BITMAP_H, SSD1306_WHITE);

  if ((frame / 20) % 2 == 0) {
    display.fillCircle(104, 42, 2, SSD1306_WHITE);
  }

  display.display();

  frame++;

  if (millis() - stateStartTime > 12000) {
    changeState(STATE_GARAGE_IDLE);
  }
}

void runNightIdle() {
  display.clearDisplay();

  int offset = frame * 2;

  drawMoon();
  drawStars(offset);

  if (isRaining) {
    drawRain(offset);
  }

  if (isCold) {
    drawSnow(offset);
  }

  if (isStorming) {
    drawLightningFlash();
  }

  drawVaporGrid();

  display.drawBitmap(0, 0, CAR_SIDE, BITMAP_W, BITMAP_H, SSD1306_WHITE);

  display.display();

  frame++;

  if (millis() - stateStartTime > 12000) {
    changeState(STATE_GARAGE_IDLE);
  }
}

void runGarageIdle() {
  display.clearDisplay();

  display.drawBitmap(0, 0, GARAGE_OPEN, BITMAP_W, BITMAP_H, SSD1306_WHITE);

  if ((frame / 12) % 2 == 0) {
    display.drawBitmap(0, 0, CAR_REAR_ON, BITMAP_W, BITMAP_H, SSD1306_WHITE);
  } else {
    display.drawBitmap(0, 0, CAR_REAR_OFF, BITMAP_W, BITMAP_H, SSD1306_WHITE);
  }

  display.display();

  frame++;

  if (millis() - stateStartTime > 10000) {
    changeState(isNight ? STATE_NIGHT_IDLE : STATE_CRUISE_IDLE);
  }
}

// -------------------------
// SHUTDOWN / PARKING ANIMATION
// -------------------------

void runShutdownAnimation() {
  unsigned long elapsed = millis() - stateStartTime;

  display.clearDisplay();

  if (elapsed < 1200) {
    centerText("PARKING...", 28, 1);
  }

  else if (elapsed < 3500) {
    display.drawBitmap(0, 0, GARAGE_OPEN, BITMAP_W, BITMAP_H, SSD1306_WHITE);

    if ((elapsed / 250) % 2 == 0) {
      display.drawBitmap(0, 0, CAR_REAR_ON, BITMAP_W, BITMAP_H, SSD1306_WHITE);
    } else {
      display.drawBitmap(0, 0, CAR_REAR_OFF, BITMAP_W, BITMAP_H, SSD1306_WHITE);
    }
  }

  else if (elapsed < 5000) {
    display.drawBitmap(0, 0, GARAGE_HALF, BITMAP_W, BITMAP_H, SSD1306_WHITE);
    display.drawBitmap(0, 0, CAR_REAR_OFF, BITMAP_W, BITMAP_H, SSD1306_WHITE);
  }

  else if (elapsed < 6500) {
    display.drawBitmap(0, 0, GARAGE_CLOSED, BITMAP_W, BITMAP_H, SSD1306_WHITE);
  }

  else if (elapsed < 8500) {
    centerText("GOODNIGHT", 20, 1);
    centerText("PINKLADYZ", 36, 1);
  }

  else {
    display.clearDisplay();
  }

  display.display();
}

// -------------------------
// SETUP + LOOP
// -------------------------

void setup() {
  Serial.begin(115200);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED failed");
    while (true);
  }

  display.clearDisplay();
  display.display();

  randomSeed(analogRead(0));

  connectWiFi();
  updateClock();
  fetchWeather();

  lastWeatherUpdate = millis();
  lastTimeUpdate = millis();

  changeState(STATE_BOOT);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    wifiConnected = false;
  } else {
    wifiConnected = true;
  }

  if (millis() - lastTimeUpdate >= timeInterval) {
    updateClock();
    lastTimeUpdate = millis();
  }

  if (millis() - lastWeatherUpdate >= weatherInterval) {
    fetchWeather();
    lastWeatherUpdate = millis();
  }

  if (Serial.available()) {
    char input = Serial.read();

    if (input == 'p') changeState(STATE_SHUTDOWN);
    if (input == 'b') changeState(STATE_BOOT);
    if (input == 'g') changeState(STATE_GARAGE_IDLE);
    if (input == 'd') changeState(STATE_DASHBOARD);
    if (input == 'n') {
      isNight = true;
      changeState(STATE_NIGHT_IDLE);
    }
    if (input == 'c') {
      isNight = false;
      changeState(STATE_CRUISE_IDLE);
    }
    if (input == 'w') {
      fetchWeather();
      changeState(STATE_DASHBOARD);
    }
  }

  if (millis() - lastFrameTime < 80) {
    return;
  }

  lastFrameTime = millis();

  switch (currentState) {
    case STATE_BOOT:
      runBootSequence();
      break;

    case STATE_DASHBOARD:
      runDashboard();
      break;

    case STATE_CRUISE_IDLE:
      runCruiseIdle();
      break;

    case STATE_NIGHT_IDLE:
      runNightIdle();
      break;

    case STATE_GARAGE_IDLE:
      runGarageIdle();
      break;

    case STATE_SHUTDOWN:
      runShutdownAnimation();
      break;
  }
}