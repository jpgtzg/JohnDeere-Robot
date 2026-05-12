#! /bin/bash

set -e

if ! command -v mosquitto &> /dev/null; then
    sudo apt install mosquitto mosquitto-clients -y
fi
sudo systemctl enable mosquitto
sudo systemctl start mosquitto

uv run main.py

sudo systemctl stop mosquitto
