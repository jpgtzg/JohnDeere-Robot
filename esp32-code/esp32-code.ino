#include <WiFi.h>
#include <PubSubClient.h>

// ── UART (STM32) ───────────────────────────────────────────
#define STM32_SERIAL Serial2
#define STM32_BAUD   115200

// ── WiFi ───────────────────────────────────────────────────
const char* ssid = "jpgtzg-phone";
const char* pass = "jpgtzg24";

// ── MQTT ───────────────────────────────────────────────────
const char* mqttServer = "10.24.227.53";
const int   mqttPort   = 1883;

WiFiClient   wifiClient;
PubSubClient mqttClient(wifiClient);

// Upward topics (STM32 → Raspberry Pi)
const char engine_topic[]  = "robot/engine_speed";
const char vehicle_topic[] = "robot/vehicle_speed";
const char gear_topic[]    = "robot/gear";

// Downward topics (Raspberry Pi → STM32)
const char cmd_topic[]  = "esp32/commands";
const char mode_topic[] = "esp32/control_mode";

// ── State ──────────────────────────────────────────────────
String controlMode = "LOCAL";

// Non-blocking serial line buffer
static String serialBuf;

// ──────────────────────────────────────────────────────────

void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  String topicStr(topic);
  String payloadStr;
  for (unsigned int i = 0; i < length; i++) payloadStr += (char)payload[i];

  Serial.printf("MSG: [%s] %s\n", topic, payloadStr.c_str());

  if (topicStr == mode_topic) {
    controlMode = payloadStr;
    STM32_SERIAL.printf("MD:%s\r\n", payloadStr.c_str());
    Serial.printf("Mode → %s\n", payloadStr.c_str());
  } else if (topicStr == cmd_topic) {
    if (controlMode == "REMOTE") {
      STM32_SERIAL.printf("%s\r\n", payloadStr.c_str());
      Serial.printf("CMD → STM32: %s\n", payloadStr.c_str());
    }
  }
}

void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf(" connected! IP: %s\n", WiFi.localIP().toString().c_str());
}

void connectMQTT() {
  while (!mqttClient.connected()) {
    String clientId = "ESP32-" + String(random(0xFFFF), HEX);
    Serial.printf("Connecting to MQTT as %s...", clientId.c_str());
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println(" connected!");
      mqttClient.subscribe(cmd_topic,  1);
      mqttClient.subscribe(mode_topic, 1);
    } else {
      Serial.printf(" failed (rc=%d). Retrying in 5s...\n", mqttClient.state());
      delay(5000);
    }
  }
}

void publishValue(const char* topic, const char* value) {
  mqttClient.publish(topic, value);
  Serial.printf("Published [%s]: %s\n", topic, value);
}

void parseLine(const String& line) {
  if      (line.startsWith("VS:")) publishValue(vehicle_topic, line.substring(3).c_str());
  else if (line.startsWith("ES:")) publishValue(engine_topic,  line.substring(3).c_str());
  else if (line.startsWith("GR:")) publishValue(gear_topic,    line.substring(3).c_str());
}

// ──────────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);
  STM32_SERIAL.begin(STM32_BAUD, SERIAL_8N1, 16, 17);

  connectWiFi();
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(onMqttMessage);
  connectMQTT();
}

void loop() {
  if (!mqttClient.connected()) connectMQTT();
  mqttClient.loop();

  // Non-blocking: read one character at a time so mqtt.loop() runs every iteration
  while (STM32_SERIAL.available()) {
    char c = STM32_SERIAL.read();
    if (c == '\n') {
      serialBuf.trim();
      if (serialBuf.length() > 0) {
        Serial.println("STM32 → " + serialBuf);
        parseLine(serialBuf);
      }
      serialBuf = "";
    } else if (c != '\r') {
      serialBuf += c;
    }
  }
}
