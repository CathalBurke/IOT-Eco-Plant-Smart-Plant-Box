#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include "homepage.h"

// Wi-Fi credentials
const char* ssid = "****";
const char* password = "*****";

// Sensor and actuator pins
#define DHTPIN 14            // DHT11 data pin
#define SOIL_MOISTURE_PIN 34 // Soil moisture sensor pin (ADC1)
#define PUMP_PIN 32          // Pump control pin (active-HIGH)
#define FAN_PIN 25           // Fan control pin (active-LOW)
#define LED_PIN 2            // On-board LED (active-LOW)

// Grove water-level strip (I²C)
#define WATER_LEVEL_ADDR_L 0x77
#define WATER_LEVEL_ADDR_H 0x78

// DHT sensor
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Create the web server instance on port 80
WebServer server(80);

// Function to read temperature
float Get_Temp() {
  float t = dht.readTemperature();
  if (isnan(t)) {
    Serial.println("Failed to read temperature from DHT sensor!");
    return 0.0;
  }
  return t;
}

// Function to read humidity
float Get_Humid() {
  float h = dht.readHumidity();
  if (isnan(h)) {
    Serial.println("Failed to read humidity from DHT sensor!");
    return 0.0;
  }
  return h;
}

// Function to read soil moisture (%)
int Get_Moisture() {
  const int dryRAW = 950;  // adjust after calibration
  const int wetRAW =   0;
  int raw = analogRead(SOIL_MOISTURE_PIN);
  raw = constrain(raw, wetRAW, dryRAW);
  return map(raw, dryRAW, wetRAW, 0, 100);
}

// Function to read water level strip (%)
int Get_Water_level() {
  const uint8_t THRESHOLD = 100;
  uint8_t low[8], high[12];
  uint32_t mask = 0;
  uint8_t sections = 0;

  Wire.requestFrom(WATER_LEVEL_ADDR_L, 8);
  for (int i = 0; i < 8; i++) low[i] = Wire.read();

  Wire.requestFrom(WATER_LEVEL_ADDR_H, 12);
  for (int i = 0; i < 12; i++) high[i] = Wire.read();

  for (int i = 0; i < 8; i++)  if (low[i]  > THRESHOLD) mask |= 1UL << i;
  for (int i = 0; i < 12; i++) if (high[i] > THRESHOLD) mask |= 1UL << (8 + i);

  while (mask & 1) { sections++; mask >>= 1; }
  return sections * 5; // 0,5,10 ... 100
}

// Function to start the pump
void startPump() {
  digitalWrite(PUMP_PIN, HIGH);
  Serial.println("Pump started");
  server.send(200, "text/plain", "Pump ON");
}

// Function to stop the pump
void stopPump() {
  digitalWrite(PUMP_PIN, LOW);
  Serial.println("Pump stopped");
  server.send(200, "text/plain", "Pump OFF");
}

// Function to start the fan
void startFan() {
  digitalWrite(FAN_PIN, LOW);
  Serial.println("Fan started");
  server.send(200, "text/plain", "Fan ON");
}

// Function to stop the fan
void stopFan() {
  digitalWrite(FAN_PIN, HIGH);
  Serial.println("Fan stopped");
  server.send(200, "text/plain", "Fan OFF");
}

// Endpoint to send sensor data as JSON
void handleData() {
  StaticJsonDocument<256> doc;
  doc["temperature"] = Get_Temp();
  doc["humidity"] = Get_Humid();
  doc["soil_moisture"] = Get_Moisture();
  doc["water_level"] = Get_Water_level();

  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

// Root endpoint
void handleRoot() {
  server.send(200, "text/html", homePagePart1);
}

// Handle unknown endpoints
void handleNotFound() {
  String msg = "File Not Found\n\nURI: " + server.uri();
  msg += "\nMethod: " + String(server.method() == HTTP_GET ? "GET" : "POST");
  msg += "\nArguments: " + String(server.args()) + "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    msg += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", msg);
}

void setup() {
  Serial.begin(115200);

  // ADC for soil moisture probe
  analogReadResolution(12);
  analogSetPinAttenuation(SOIL_MOISTURE_PIN, ADC_11db);

  pinMode(PUMP_PIN, OUTPUT); digitalWrite(PUMP_PIN, LOW);
  pinMode(FAN_PIN, OUTPUT);  digitalWrite(FAN_PIN, HIGH);
  pinMode(LED_PIN, OUTPUT);  digitalWrite(LED_PIN, HIGH);

  dht.begin();
  Wire.begin(); // SDA 21, SCL 22

  // Connect to Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // mDNS
  MDNS.begin("esp32");

  // Setup server routes
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/start-pump", startPump);
  server.on("/stop-pump", stopPump);
  server.on("/start-fan", startFan);
  server.on("/stop-fan", stopFan);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  delay(2); // Allow CPU to handle Wi-Fi stack
}
