#include <ArduinoMqttClient.h>
#include <WiFiNINA.h>
#include "arduino_secrets.h"

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

const char broker[] = "test.mosquitto.org"; // RASP IP ADDRESS
int        port     = 1883;

const char engine_topic[]  = "robot/engine_speed";
const char vehicle_topic[] = "robot/vehicle_speed";
const char gear_topic[]    = "robot/gear";

// STM32 UART: RX=GPIO16, TX=GPIO17 at 115200 baud
#define STM32_SERIAL Serial2
#define STM32_BAUD   115200

void publishValue(const char* topic, const char* value) {
  mqttClient.beginMessage(topic);
  mqttClient.print(value);
  mqttClient.endMessage();
  Serial.print("Published to ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(value);
}

void parseLine(const String& line) {
  if (line.startsWith("VS:")) {
    publishValue(vehicle_topic, line.substring(3).c_str());
  } else if (line.startsWith("ES:")) {
    publishValue(engine_topic, line.substring(3).c_str());
  } else if (line.startsWith("GR:")) {
    publishValue(gear_topic, line.substring(3).c_str());
  }
}

void setup() {
  Serial.begin(9600);
  STM32_SERIAL.begin(STM32_BAUD);

  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.println(ssid);
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    Serial.print(".");
    delay(5000);
  }
  Serial.println("Connected to WiFi Network");

  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(broker);
  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
    while (1);
  }
  Serial.println("You're connected to the MQTT broker!");
}

void loop() {
  mqttClient.poll();

  while (STM32_SERIAL.available()) {
    String line = STM32_SERIAL.readStringUntil('\n');
    line.trim();
    if (line.length() > 0) {
      parseLine(line);
    }
  }
}
