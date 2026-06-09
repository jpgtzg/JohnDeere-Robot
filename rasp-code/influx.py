from influxdb_client.client.influxdb_client import InfluxDBClient
from influxdb_client.client.write.point import Point
from influxdb_client.client.write_api import SYNCHRONOUS

INFLUX_URL = "http://localhost:8086"
INFLUX_TOKEN = "QJtlgUrDhhwIG7xRC9fTbv-jkxClc-FybK8LxYmNyZB9mWQiOHerUtz4WNwCAXs3gPCT476lEqFVOl3kSAV7jQ=="
INFLUX_ORG = "FacultadIng"
INFLUX_BUCKET = "telemetria_tractor"

influx_client = InfluxDBClient(url=INFLUX_URL, token=INFLUX_TOKEN, org=INFLUX_ORG)
write_api = influx_client.write_api(write_options=SYNCHRONOUS)


def write_to_influxdb(measurement: str, field: str, value: float, client: str):
    point = Point(measurement).tag("client", client).field(field, value)
    write_api.write(bucket=INFLUX_BUCKET, org=INFLUX_ORG, record=point)
    print(f"Written to influx db: {measurement} - {field}: {value} at {point.time}")
