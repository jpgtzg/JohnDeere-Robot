# JohnDeere Robot — Telemetry System

Reads engine/transmission simulation data from an STM32F103RB, forwards it via UART to an ESP32, which publishes it over MQTT to a Raspberry Pi for storage in InfluxDB.

---

## Wiring — STM32 ↔ ESP32

| STM32 (Nucleo-F103RB) | Signal | ESP32 |
|---|---|---|
| PA9 (USART1 TX) — CN10 pin 1 | → | GPIO16 (Serial2 RX) |
| PA10 (USART1 RX) — CN10 pin 33 | ← | GPIO17 (Serial2 TX) |
| GND | — | GND |

- Both devices run at **3.3V logic** — no level shifter required.
- Baud rate: **115200**.
- The STM32 USB cable (ST-Link, USART2) is independent and used only for debug output and flashing.

---

## Data Flow

```
[ADC potentiometer / brake button]
            |
            v
    TIM3 IRQ — every 40 ms
    (PSC=63, ARR=39999 @ 64 MHz)
            |
            v
    EngTrModel_step()
    Simulink-generated engine + transmission model
    Inputs:  Throttle (ADC %), BrakeTorque
    Outputs: EngineSpeed (RPM), VehicleSpeed (km/h), Gear
            |
            v
    transmit_ready flag set
            |
            v
    Main loop — Transmit_Data()
    Sends 3 ASCII lines over USART1 (PA9):
        VS:<value>\r\n   — vehicle speed
        ES:<value>\r\n   — engine speed
        GR:<value>\r\n   — gear
            |
     [physical wire]
            |
            v
    ESP32 Serial2 (GPIO16)
    Reads lines with readStringUntil('\n')
    Parses VS: / ES: / GR: prefix
    Publishes float value to MQTT topic
            |
        [WiFi / LAN]
            |
            v
    Raspberry Pi — mosquitto broker (port 1883)
    paho-mqtt subscriber (main.py)
    Parses float payload per topic:
        robot/vehicle_speed
        robot/engine_speed
        robot/gear
            |
            v
    InfluxDB
    Bucket: telemetria_tractor
    Measurement: tractor_metrics
    Tag: client=Raspberry_Pi
```

---

## MQTT Topics

| Topic | Value | Source field |
|---|---|---|
| `robot/vehicle_speed` | km/h | `EngTrModel_Y.VehicleSpeed` |
| `robot/engine_speed` | RPM | `EngTrModel_Y.EngineSpeed` |
| `robot/gear` | gear number | `EngTrModel_Y.Gear` |

---

## Setup

### Raspberry Pi
```bash
cd rasp-code
./start.sh
```
Installs and starts `mosquitto` if not present, then runs the Python subscriber.

### ESP32
Set the broker address in `esp32-code/esp32-code.ino` to the Raspberry Pi's local IP:
```cpp
const char broker[] = "192.168.X.X";
```
WiFi credentials go here:
```cpp
#define SECRET_SSID "your_ssid"
#define SECRET_PASS "your_password"
```

### STM32
Build and flash with CMake + OpenOCD via the VS Code extension or:
```bash
cd stm32-code
cmake --preset debug
cmake --build build
```
