#!/bin/bash

set -e

usage() {
    echo "Usage: $0 [--face] [--main] [--both]"
    echo "  --face   Run face.py (gaze detection)"
    echo "  --main   Run main.py (MQTT/InfluxDB bridge)"
    echo "  --both   Run both concurrently"
    exit 1
}

RUN_FACE=false
RUN_MAIN=false

[[ $# -eq 0 ]] && usage

while [[ $# -gt 0 ]]; do
    case $1 in
        --face) RUN_FACE=true ;;
        --main) RUN_MAIN=true ;;
        --both) RUN_FACE=true; RUN_MAIN=true ;;
        *) echo "Unknown flag: $1"; usage ;;
    esac
    shift
done

# Mosquitto is only needed when running main.py
start_mosquitto() {
    if ! command -v mosquitto &>/dev/null; then
        sudo apt install mosquitto mosquitto-clients -y
    fi
    sudo systemctl enable mosquitto
    sudo systemctl start mosquitto
}

stop_mosquitto() {
    sudo systemctl stop mosquitto
}

# Graceful shutdown: kill background jobs and stop mosquitto if we started it
PIDS=()
cleanup() {
    echo ""
    echo "Stopping..."
    for pid in "${PIDS[@]}"; do
        kill "$pid" 2>/dev/null
    done
    wait 2>/dev/null
    "$RUN_MAIN" && stop_mosquitto
    exit 0
}
trap cleanup SIGINT SIGTERM

# ── launch ────────────────────────────────────────────────────────────────────
if "$RUN_MAIN"; then
    start_mosquitto
fi

if "$RUN_FACE" && "$RUN_MAIN"; then
    uv run face.py &
    PIDS+=($!)
    uv run main.py &
    PIDS+=($!)
    wait
elif "$RUN_FACE"; then
    uv run face.py
elif "$RUN_MAIN"; then
    uv run main.py
    stop_mosquitto
fi
