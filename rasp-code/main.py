from paho.mqtt import client as mqtt_client

from influx import write_to_influxdb

CLIENT_NAME = "Raspberry_Pi"
BROKER = "10.24.227.53"
PORT = 1883
TOPICS = [
    "robot/engine_speed",
    "robot/vehicle_speed",
    "robot/gear",
    "esp32/control_mode",
    "esp32/commands",
]

TOPIC_MAP = {
    "robot/engine_speed": ("tractor_metrics", "engine_speed"),
    "robot/vehicle_speed": ("tractor_metrics", "vehicle_speed"),
    "robot/gear": ("tractor_metrics", "gear"),
}

CMD_FIELD_MAP = {"AC": "acceleration_cmd", "BR": "brake_cmd"}


def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT broker")
        for topic in TOPICS:
            client.subscribe(topic)
            print(f"Subscribed to {topic}")
    else:
        print(f"Connection failed, code: {rc}")


def on_message(client, userdata, msg):
    topic = msg.topic
    payload = msg.payload.decode("utf-8").strip()
    print(f"Received: [{topic}] {payload}")

    if topic == "esp32/control_mode":
        value = 1.0 if payload.upper() == "REMOTE" else 0.0
        write_to_influxdb("tractor_metrics", "control_mode", value, CLIENT_NAME)
        return

    if topic == "esp32/commands":
        if ":" in payload:
            prefix, _, raw = payload.partition(":")
            field = CMD_FIELD_MAP.get(prefix.upper())
            if field:
                try:
                    write_to_influxdb("tractor_metrics", field, float(raw), CLIENT_NAME)
                except ValueError:
                    print(f"Bad command value: {payload}")
        return

    try:
        value = float(payload)
        if topic in TOPIC_MAP:
            measurement, field = TOPIC_MAP[topic]
            write_to_influxdb(measurement, field, value, CLIENT_NAME)
        else:
            print(f"Unknown topic: {topic}")
    except ValueError:
        print(f"Invalid payload for topic {topic}: {payload}")


def main():
    mqtt = mqtt_client.Client(client_id=CLIENT_NAME)
    mqtt.on_connect = on_connect
    mqtt.on_message = on_message

    print("Connecting to MQTT broker...")
    mqtt.connect(BROKER, PORT)
    print("Connected. Starting MQTT loop...")
    mqtt.loop_forever()


if __name__ == "__main__":
    main()
