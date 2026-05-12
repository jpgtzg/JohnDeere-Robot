# Raspberry Pi Code

MQTT subscriber that listens for tractor telemetry data and writes it to InfluxDB.

## What it does

Connects to a local Mosquitto MQTT broker, subscribes to three topics, and stores incoming values in InfluxDB:

| Topic | InfluxDB field |
|---|---|
| `robot/engine_speed` | `engine_speed` |
| `robot/vehicle_speed` | `vehicle_speed` |
| `robot/gear` | `gear` |

All fields are written to the `tractor_metrics` measurement in the `telemetria_tractor` bucket.

## Requirements

- [uv](https://github.com/astral-sh/uv)
- Mosquitto (installed automatically by `start.sh`)
- A running InfluxDB instance at `http://localhost:8086`

## Running

```bash
./start.sh
```

This will install and start Mosquitto, run `main.py`, then stop Mosquitto when the script exits.

## Configuration

Edit the constants at the top of `main.py` to change the InfluxDB URL, token, org, or bucket, or to add/remove MQTT topics.

