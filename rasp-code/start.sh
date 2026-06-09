#!/bin/bash

set -e

usage() {
    echo "Usage: $0 [--face] [--main] [--api] [--all]"
    echo "  --face   Run face.py (gaze detection → InfluxDB)"
    echo "  --main   Run main.py (MQTT → InfluxDB bridge)"
    echo "  --api    Run api.py  (REST API for remote control)"
    echo "  --all    Run all three concurrently"
    exit 1
}

RUN_FACE=false
RUN_MAIN=false
RUN_API=false

[[ $# -eq 0 ]] && usage

while [[ $# -gt 0 ]]; do
    case $1 in
        --face) RUN_FACE=true ;;
        --main) RUN_MAIN=true ;;
        --api)  RUN_API=true ;;
        --all)  RUN_FACE=true; RUN_MAIN=true; RUN_API=true ;;
        *) echo "Unknown flag: $1"; usage ;;
    esac
    shift
done

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

PIDS=()
cleanup() {
    echo ""
    echo "Stopping..."
    for pid in "${PIDS[@]}"; do
        kill "$pid" 2>/dev/null
    done
    wait 2>/dev/null
    if "$RUN_MAIN" || "$RUN_API"; then
        stop_mosquitto
    fi
    exit 0
}
trap cleanup SIGINT SIGTERM

# Mosquitto is needed by main.py (MQTT subscriber) and api.py (MQTT publisher)
if "$RUN_MAIN" || "$RUN_API"; then
    start_mosquitto
fi

# Launch selected processes
if "$RUN_FACE"; then
    uv run face.py &
    PIDS+=($!)
fi

if "$RUN_MAIN"; then
    uv run main.py &
    PIDS+=($!)
fi

if "$RUN_API"; then
    uv run api.py &
    PIDS+=($!)
fi

# If only one process, run it in foreground instead
if [[ ${#PIDS[@]} -eq 0 ]]; then
    echo "No processes to run."
    exit 1
fi

wait
