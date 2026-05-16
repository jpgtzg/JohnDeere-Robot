#include "WiFi.h"
#include <ArduinoMqttClient.h>

// ── UART (STM32) ───────────────────────────────────────────
#define STM32_SERIAL Serial2
#define STM32_BAUD   115200

// ── WiFi ───────────────────────────────────────────────────
const char ssid[] = "";
const char pass[] = "";

// ── MQTT ───────────────────────────────────────────────────
WiFiClient  wifiClient;
MqttClient  mqttClient(wifiClient);

const char broker[]        = "10.66.126.53";
const int  port            = 1883;
const char engine_topic[]  = "robot/engine_speed";
const char vehicle_topic[] = "robot/vehicle_speed";
const char gear_topic[]    = "robot/gear";

// ──────────────────────────────────────────────────────────

void connectWiFi() {
  WiFi.disconnect(true);   // clear previous state
  WiFi.mode(WIFI_STA);
  delay(100);

  Serial.printf("Connecting to %s\n", ssid);
  WiFi.begin(ssid, pass);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (++attempts >= 40) {           // 20s timeout → retry
      Serial.println("\nRetrying...");
      WiFi.disconnect();
      delay(1000);
      WiFi.begin(ssid, pass);
      attempts = 0;
    }
  }

  Serial.printf("\nConnected! IP: %s  RSSI: %d dBm\n",
                WiFi.localIP().toString().c_str(), WiFi.RSSI());
}

void publishValue(const char* topic, const char* value) {
  mqttClient.beginMessage(topic);
  mqttClient.print(value);
  mqttClient.endMessage();
  Serial.printf("Published [%s]: %s\n", topic, value);
}

void parseLine(const String& line) {
  if      (line.startsWith("VS:")) publishValue(vehicle_topic, line.substring(3).c_str());
  else if (line.startsWith("ES:")) publishValue(engine_topic,  line.substring(3).c_str());
  else if (line.startsWith("GR:")) publishValue(gear_topic,    line.substring(3).c_str());
}

void setup() {
  Serial.begin(9600);
  STM32_SERIAL.begin(STM32_BAUD, SERIAL_8N1, 16, 17);

  connectWiFi();

  Serial.printf("Connecting to MQTT broker %s...\n", broker);
  if (!mqttClient.connect(broker, port)) {
    Serial.printf("MQTT failed! Error: %d\n", mqttClient.connectError());
    while (1);
  }
  Serial.println("MQTT connected!");
}

void loop() {
  // Auto-reconnect if dropped
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi lost — reconnecting...");
    connectWiFi();
  }

  mqttClient.poll();

  while (STM32_SERIAL.available()) {
    String line = STM32_SERIAL.readStringUntil('\n');
    line.trim();
    if (line.length() > 0) {
      Serial.println("STM32 → " + line);
      parseLine(line);
    }
  }
}
