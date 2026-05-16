from influxdb_client.client.influxdb_client import InfluxDBClient
from influxdb_client.client.write.point import Point
from influxdb_client.client.write_api import SYNCHRONOUS
from paho.mqtt import client as mqtt_client

INFLUX_URL = "http://localhost:8086"
INFLUX_TOKEN = "QJtlgUrDhhwIG7xRC9fTbv-jkxClc-FybK8LxYmNyZB9mWQiOHerUtz4WNwCAXs3gPCT476lEqFVOl3kSAV7jQ=="
INFLUX_ORG = "FacultadIng"
INFLUX_BUCKET = "telemetria_tractor"

CLIENT_NAME = "Raspberry_Pi"
BROKER = "localhost"
PORT = 1883
TOPICS = ["robot/engine_speed", "robot/vehicle_speed", "robot/gear"]

DEBUG = True

client = InfluxDBClient(url=INFLUX_URL, token=INFLUX_TOKEN, org=INFLUX_ORG)
write_api = client.write_api(write_options=SYNCHRONOUS)


def write_to_influxdb(measurement: str, field: str, value: float):
    point = Point(measurement).tag("client", CLIENT_NAME).field(field, value)
    if not DEBUG:
        write_api.write(bucket=INFLUX_BUCKET, org=INFLUX_ORG, record=point)
        print(f"Written to influx db: {measurement} - {field}: {value} at {point.time}")
    else:
        print(f"DEBUG: {measurement} - {field}: {value} at {point.time}")


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
    print(f"Received message: {payload} on topic: {topic}")

    try:
        value = float(payload)
        topic_map = {
            "robot/engine_speed": ("tractor_metrics", "engine_speed"),
            "robot/vehicle_speed": ("tractor_metrics", "vehicle_speed"),
            "robot/gear": ("tractor_metrics", "gear"),
        }

        if topic in topic_map:
            measurement, field = topic_map[topic]
            write_to_influxdb(measurement, field, value)
        else:
            print(f"Unknown topic: {topic}")
    except ValueError:
        print(f"Invalid payload for topic {topic}: {payload}")


def main():
    client = mqtt_client.Client(client_id=CLIENT_NAME)
    client.on_connect = on_connect
    client.on_message = on_message

    print("Connecting to MQTT broker...")
    client.connect(BROKER, PORT)
    print("Conected \nStarting MQTT loop...")
    client.loop_forever()


if __name__ == "__main__":
    main()
