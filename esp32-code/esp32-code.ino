#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// ── UART (STM32) ───────────────────────────────────────────
#define STM32_SERIAL Serial2
#define STM32_BAUD   115200

// ── BLE (Nordic UART Service — used by most generic "BLE UART"
//    controller apps, incl. BLE Controller - Arduino ESP32) ──
#define BLE_DEVICE_NAME "ESP32-Robot"
#define NUS_SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define NUS_RX_UUID      "6E400002-B5A3-F393-E0A9-E50E24DCCA9E" // app writes here
#define NUS_TX_UUID      "6E400003-B5A3-F393-E0A9-E50E24DCCA9E" // ESP32 notifies here

static BLECharacteristic *pTxCharacteristic;
static bool bleClientConnected = false;

// Non-blocking STM32 serial line buffer
static String serialBuf;

// ── D-pad control state ──────────────────────────────────────
// The app sends discrete "UP"/"DOWN"/"LEFT"/"RIGHT" tokens repeatedly
// while a direction is held, but never sends a release event. A
// direction is treated as active only while it keeps arriving within
// DIRECTION_TIMEOUT_MS of the last one — this both synthesizes the
// missing release and acts as a connection-loss watchdog.
#define DIRECTION_TIMEOUT_MS 400
#define DRIVE_SPEED           70.0f
#define TURN_SPEED            70.0f
#define BRAKE_KEY             "B"

static unsigned long upSeenAt = 0, downSeenAt = 0, leftSeenAt = 0, rightSeenAt = 0;
static bool braking = false;

// ──────────────────────────────────────────────────────────

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *server) override {
    bleClientConnected = true;
    Serial.println("BLE client connected");
  }
  void onDisconnect(BLEServer *server) override {
    bleClientConnected = false;
    Serial.println("BLE client disconnected, restarting advertising");
    /* stop driving immediately rather than waiting for the timeout */
    upSeenAt = downSeenAt = leftSeenAt = rightSeenAt = 0;
    braking  = true;
    server->getAdvertising()->start();
  }
};

class RxCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *characteristic) override {
    String value = characteristic->getValue().c_str();
    if (value.length() == 0) return;

    Serial.printf("BLE RX (%u bytes): \"%s\"\n", value.length(), value.c_str());

    if (value == BRAKE_KEY) {
      braking = true;
      return;
    }

    unsigned long now = millis();
    if      (value == "UP")    { upSeenAt    = now; braking = false; }
    else if (value == "DOWN")  { downSeenAt  = now; braking = false; }
    else if (value == "LEFT")  { leftSeenAt  = now; braking = false; }
    else if (value == "RIGHT") { rightSeenAt = now; braking = false; }
    /* HORN / LIGHT / A / C / D / Speed_xxx: not wired up yet, ignored */
  }
};

/* Runs on a fixed tick regardless of BLE traffic so a held direction that
 * stops arriving (release, or dropped connection) is zeroed out promptly. */
void driveControlTick() {
  unsigned long now = millis();
  float xspeed = 0.0f, yspeed = 0.0f;

  if (!braking) {
    bool up    = upSeenAt    != 0 && (now - upSeenAt)    < DIRECTION_TIMEOUT_MS;
    bool down  = downSeenAt  != 0 && (now - downSeenAt)  < DIRECTION_TIMEOUT_MS;
    bool left  = leftSeenAt  != 0 && (now - leftSeenAt)  < DIRECTION_TIMEOUT_MS;
    bool right = rightSeenAt != 0 && (now - rightSeenAt) < DIRECTION_TIMEOUT_MS;

    if (up)    xspeed += DRIVE_SPEED;
    if (down)  xspeed -= DRIVE_SPEED;
    if (right) yspeed += TURN_SPEED;
    if (left)  yspeed -= TURN_SPEED;
  }

  STM32_SERIAL.printf("XY:%.1f,%.1f\r\n", xspeed, yspeed);
}

void parseLine(const String& line) {
  if (bleClientConnected && (line.startsWith("VS:") || line.startsWith("ES:") || line.startsWith("GR:"))) {
    String notification = line + "\n";
    pTxCharacteristic->setValue((uint8_t *)notification.c_str(), notification.length());
    pTxCharacteristic->notify();
  }
}

// ──────────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);
  STM32_SERIAL.begin(STM32_BAUD, SERIAL_8N1, 16, 17);

  BLEDevice::init(BLE_DEVICE_NAME);
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  BLEService *pService = pServer->createService(NUS_SERVICE_UUID);

  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
      NUS_RX_UUID, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  pRxCharacteristic->setCallbacks(new RxCallbacks());

  pTxCharacteristic = pService->createCharacteristic(NUS_TX_UUID, BLECharacteristic::PROPERTY_NOTIFY);
  pTxCharacteristic->addDescriptor(new BLE2902());

  pService->start();
  pServer->getAdvertising()->addServiceUUID(NUS_SERVICE_UUID);
  pServer->getAdvertising()->start();

  Serial.println("BLE advertising as \"" BLE_DEVICE_NAME "\"");
}

void loop() {
  static unsigned long lastTickAt = 0;
  unsigned long now = millis();
  if (now - lastTickAt >= 50) {
    lastTickAt = now;
    driveControlTick();
  }

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
