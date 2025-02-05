#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include "homepage.h"

// Wi-Fi credentials
const char* ssid = "CathalsIphone";
const char* password = "12345678";

// Sensor and actuator pins
#define DHTPIN 14           // DHT sensor pin
#define SOIL_MOISTURE_PIN 26 // Soil moisture sensor pin (analog)
#define WATER_LEVEL_PIN 33   // Water level sensor pin (analog)
#define PUMP_PIN 32         // Pump control pin
#define LED_PIN 2            // LED for system status indication
#define FAN_PIN 25

// Initialize DHT sensor 
#define DHTTYPE DHT11        
DHT dht(DHTPIN, DHTTYPE);

// Create the web server instance on port 80
WebServer server(80);

// Function to read temperature
float Get_Temp() {
  float temp = dht.readTemperature();
  if (isnan(temp)) {
      Serial.println("Failed to read temperature from DHT sensor!");
      return 0.0;
  }
  return temp;
}

// Function to read humidity
float Get_Humid() {
  float humid = dht.readHumidity();
  if (isnan(humid)) {
      Serial.println("Failed to read humidity from DHT sensor!");
      return 0.0;
  }
  return humid;
}

// Function to read soil moisture level
float Get_Moisture() {
    float moistureValue = analogRead(SOIL_MOISTURE_PIN);
    Serial.print("Soil Moisture Raw Value: ");
    Serial.println(moistureValue);
    return moistureValue;
}


// Function to read water level
float Get_Water_level() {
  int waterLevelValue = analogRead(WATER_LEVEL_PIN);
  
  return waterLevelValue;
}

// Function to start the pump
void startPump() {
  digitalWrite(PUMP_PIN, HIGH); // Activate pump
  Serial.println("Pump started");
  server.send(200, "text/plain", "Pump ON");
}

// Function to stop the pump
void stopPump() {
  digitalWrite(PUMP_PIN, LOW); // Deactivate pump
  Serial.println("Pump stopped");
  server.send(200, "text/plain", "Pump OFF");
}
void startFan() {
  digitalWrite(FAN_PIN, HIGH);
  Serial.println("Fan started");
  server.send(200, "text/plain", "Fan ON");
}

void stopFan() {
  digitalWrite(FAN_PIN, LOW);
  Serial.println("Fan stopped");
  server.send(200, "text/plain", "Fan OFF");
}

// Function to send sensor data as JSON
void handleData() {
  StaticJsonDocument<256> doc;
  doc["temperature"] = Get_Temp();
  doc["humidity"] = Get_Humid();
  doc["soil_moisture"] = Get_Moisture();
  doc["water_level"] = Get_Water_level();

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

// Function to handle root endpoint
void handleRoot() {
  String message = homePagePart1; // Assumes homePagePart1 contains the HTML content
  server.send(200, "text/html", message);
}

// Function to handle unknown endpoints
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: " + String(server.method() == HTTP_GET ? "GET" : "POST");  // Fix here
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup(void) {
  Serial.begin(115200);
  
  // Initialize Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("Connecting to Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  dht.begin();
  float moistureValue=0;
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW); // Initialize pump as OFF
  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN, LOW); // Ensure the fan is off initially
  //pinMode(LED_PIN, OUTPUT);
  //digitalWrite(LED_PIN, HIGH); // Assume system is online by default
  

  // Initialize MDNS
  if (MDNS.begin("esp32")) {
      Serial.println("MDNS responder started");
  }

  server.on("/data", handleData);
  server.on("/start-pump", startPump);
  server.on("/stop-pump", stopPump);
  server.on("/start-fan", startFan);
  server.on("/stop-fan", stopFan);
  server.on("/led-status", HTTP_GET, handleLedStatus);
  server.onNotFound(handleNotFound);

  // Start server
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  server.handleClient();
  delay(2); // Allow CPU to switch to other tasks
}



