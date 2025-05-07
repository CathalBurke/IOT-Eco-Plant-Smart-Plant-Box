#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include "homepage.h"

// Wi-Fi credentials
const char* ssid = "CathalsIphone";
const char* password = "12345678";

// Sensor and actuator pins
#define DHTPIN 14            // DHT11 data pin
#define SOIL_MOISTURE_PIN 34 // Soil moisture sensor pin (ADC1)
#define PUMP_PIN 32          // Pump control pin (active-HIGH)
#define FAN_PIN 25           // Fan control pin (active-LOW)

// Grove water-level strip (I²C)
#define WATER_LEVEL_ADDR_L 0x77
#define WATER_LEVEL_ADDR_H 0x78

// DHT sensor
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// soil thresholds for pump
const int  SOIL_ON_THRESH     = 5;     // start watering below 35 %
const int  SOIL_OFF_THRESH    = 15;     // stop watering above 60 %
const int  SOIL_SENSOR_MIN    = 5;      // treat <5 % as sensor error
const int  WATER_MIN          = 5;      // don’t water if tank <5 %
const unsigned long PUMP_BURST_MS = 4000;  // run pump 4 s per burst
const unsigned long POLL_MS       = 5000;  // re-check sensors every 5 s

// flags
static bool pumpIsOn        = false;    // true while relay/MOSFET is on
static bool manualOverride  = false;    // set by /start-pump, cleared by /stop-pump
static unsigned long burstStart = 0;    // millis() timestamp
static unsigned long lastPoll   = 0;



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
void startPump() {                        // called by /start-pump
  digitalWrite(PUMP_PIN, HIGH);           // active-HIGH driver
  pumpIsOn       = true;
  manualOverride = true;                  // disables auto logic
  Serial.println("Pump manual ON");
  server.send(200, "text/plain", "Pump ON");
}

// Function to stop the pump
void stopPump() {                         // called by /stop-pump
  digitalWrite(PUMP_PIN, LOW);
  pumpIsOn       = false;
  manualOverride = false;
  Serial.println("Pump manual OFF");
  server.send(200, "text/plain", "Pump OFF");
}

 send sensor data as JSON
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

  pinMode(PUMP_PIN, OUTPUT); 
  digitalWrite(PUMP_PIN, LOW);
  
 

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
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop()
{
  server.handleClient();                  

  if (millis() - lastPoll >= POLL_MS) {

    lastPoll = millis();
    int soil  = Get_Moisture();           // %
    int water = Get_Water_level();        // %

    //automatic watering 
    if (!manualOverride) {

      /* start a new burst */
      if (!pumpIsOn && soil  < SOIL_ON_THRESH && soil  >= SOIL_SENSOR_MIN && water > WATER_MIN) {
        digitalWrite(PUMP_PIN, HIGH);
        pumpIsOn    = true;
        burstStart  = millis();
        Serial.println("Pump auto ON");
      }

      /* end current burst */
      if (pumpIsOn &&
         (millis() - burstStart >= PUMP_BURST_MS ||   // burst finished 
         soil  > SOIL_OFF_THRESH                 ||   // soil wet enough
          water <= WATER_MIN                      ||   // tank too low
          soil  < SOIL_SENSOR_MIN)) {                 // bad sensor
        digitalWrite(PUMP_PIN, LOW);
        pumpIsOn = false;
        Serial.println("Pump auto OFF");
      }
    }
  }
}


