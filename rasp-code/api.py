import paho.mqtt.client as mqtt_client
import paho.mqtt.publish as publish
from flask import Flask, jsonify, request
from flask_cors import CORS

from main import BROKER

app = Flask(__name__)
CORS(app)

COMMAND_TOPIC = "esp32/commands"
MODE_TOPIC = "esp32/control_mode"

VALID_MODES = {"remote", "local"}
_control_mode = "local"


def _sync_mode_from_broker():
    """Read the retained control_mode from the broker so api state survives restarts."""
    global _control_mode
    received = []

    def _on_message(client, userdata, msg):
        received.append(msg.payload.decode("utf-8").strip().lower())

    client = mqtt_client.Client(client_id="api-sync")
    client.on_message = _on_message
    try:
        client.connect(BROKER, 1883, keepalive=5)
        client.subscribe(MODE_TOPIC, qos=1)
        client.loop_start()
        import time
        time.sleep(0.5)
        client.loop_stop()
        client.disconnect()
    except Exception:
        return
    if received and received[0] in VALID_MODES:
        _control_mode = received[0]


_sync_mode_from_broker()


def _publish(topic: str, payload: str, retain: bool = False):
    publish.single(topic, payload=payload, hostname=BROKER, qos=1, retain=retain)
    print(f"Published [{topic}]: {payload}")


# ── control mode ──────────────────────────────────────────────────────────────


@app.route("/control/mode", methods=["GET"])
def get_control_mode():
    return jsonify({"mode": _control_mode})


@app.route("/control/mode", methods=["POST"])
def set_control_mode():
    global _control_mode
    data = request.json
    mode = data.get("mode", "").lower()
    if mode not in VALID_MODES:
        return jsonify(
            {"error": f"Invalid mode. Must be one of: {sorted(VALID_MODES)}"}
        ), 400
    _control_mode = mode
    _publish(MODE_TOPIC, mode.upper(), retain=True)
    return jsonify({"mode": _control_mode})


# ── remote commands (only active in remote mode) ──────────────────────────────
@app.route("/remote/acceleration", methods=["POST"])
def remote_acceleration():
    if _control_mode != "remote":
        return jsonify({"error": "Not in remote mode"}), 403
    value = float(request.json["value"])
    payload = f"AC:{value}"
    _publish(COMMAND_TOPIC, payload)
    return jsonify({"status": "sent", "command": payload})


@app.route("/remote/brake", methods=["POST"])
def remote_brake():
    if _control_mode != "remote":
        return jsonify({"error": "Not in remote mode"}), 403
    value = float(request.json["value"])
    payload = f"BR:{value}"
    _publish(COMMAND_TOPIC, payload)
    return jsonify({"status": "sent", "command": payload})


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
