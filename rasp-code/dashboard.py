"""Streamlit telemetry dashboard for the tractor robot (John Deere themed).

Reads live metrics from InfluxDB (written by main.py / face.py), embeds the
driver camera MJPEG stream (served by face.py), and lets the operator
view/change the control mode through the Flask API (api.py).

Run on the Raspberry Pi with:
    uv run streamlit run dashboard.py --server.address 0.0.0.0
"""

import warnings
from datetime import datetime, timezone

import pandas as pd
import plotly.graph_objects as go
import requests
import streamlit as st
import streamlit.components.v1 as components
from influxdb_client.client.influxdb_client import InfluxDBClient
from influxdb_client.client.warnings import MissingPivotFunction

from influx import INFLUX_BUCKET, INFLUX_ORG, INFLUX_TOKEN, INFLUX_URL

warnings.simplefilter("ignore", MissingPivotFunction)

# ── config ────────────────────────────────────────────────────────────────────
API_URL = "http://localhost:5000"
MEASUREMENT = "tractor_metrics"
DRIVER_MEASUREMENT = "driver_metrics"

ENGINE_RPM_MAX = 4000  # gauge full-scale
VEHICLE_SPEED_MAX = 40  # km/h, gauge full-scale
REFRESH_SECONDS = 2
TIMESERIES_WINDOW = "-10m"
LATEST_WINDOW = "-5m"

CAMERA_STREAM_PORT = 8080  # must match STREAM_PORT used by face.py
CAMERA_HEIGHT = 360

# John Deere brand palette
JD_GREEN = "#367C2B"
JD_GREEN_DARK = "#2A5E22"
JD_YELLOW = "#FFDE00"
JD_RED = "#C0392B"
JD_GREY = "#6B6B6B"

st.set_page_config(page_title="John Deere — Tractor Telemetry", layout="wide")


# ── influx access ─────────────────────────────────────────────────────────────
@st.cache_resource
def get_query_api():
    client = InfluxDBClient(url=INFLUX_URL, token=INFLUX_TOKEN, org=INFLUX_ORG)
    return client.query_api()


def latest_value(field: str, measurement: str = MEASUREMENT):
    """Return the most recent value of a field, or None if no data."""
    flux = f"""
        from(bucket: "{INFLUX_BUCKET}")
          |> range(start: {LATEST_WINDOW})
          |> filter(fn: (r) => r._measurement == "{measurement}" and r._field == "{field}")
          |> last()
    """
    try:
        tables = get_query_api().query(flux, org=INFLUX_ORG)
    except Exception as exc:  # noqa: BLE001
        st.session_state["influx_error"] = str(exc)
        return None
    for table in tables:
        for record in table.records:
            return record.get_value()
    return None


def time_series(field: str, measurement: str = MEASUREMENT) -> pd.DataFrame:
    """Return a DataFrame with time + value columns for a field."""
    flux = f"""
        from(bucket: "{INFLUX_BUCKET}")
          |> range(start: {TIMESERIES_WINDOW})
          |> filter(fn: (r) => r._measurement == "{measurement}" and r._field == "{field}")
          |> keep(columns: ["_time", "_value"])
          |> sort(columns: ["_time"])
    """
    try:
        df = get_query_api().query_data_frame(flux, org=INFLUX_ORG)
    except Exception as exc:  # noqa: BLE001
        st.session_state["influx_error"] = str(exc)
        return pd.DataFrame(columns=["time", field])

    if isinstance(df, list):
        df = pd.concat(df) if df else pd.DataFrame()
    if df.empty or "_value" not in df:
        return pd.DataFrame(columns=["time", field])
    return df.rename(columns={"_time": "time", "_value": field})[["time", field]]


# ── control mode API ──────────────────────────────────────────────────────────
def get_control_mode():
    try:
        return requests.get(f"{API_URL}/control/mode", timeout=2).json().get("mode")
    except requests.RequestException:
        return None


def set_control_mode(mode: str):
    try:
        resp = requests.post(f"{API_URL}/control/mode", json={"mode": mode}, timeout=2)
        return resp.ok, resp.json()
    except requests.RequestException as exc:
        return False, {"error": str(exc)}


# ── ui helpers ────────────────────────────────────────────────────────────────
def badge(text: str, color: str):
    st.markdown(
        f"<div style='background:{color};color:#fff;padding:16px;border-radius:8px;"
        f"text-align:center;font-size:22px;font-weight:700;letter-spacing:1px'>"
        f"{text}</div>",
        unsafe_allow_html=True,
    )


def gauge(value, title, max_value, suffix, bar_color=JD_GREEN):
    val = value if value is not None else 0
    return go.Figure(
        go.Indicator(
            mode="gauge+number",
            value=val,
            number={"suffix": f" {suffix}"},
            title={"text": title},
            gauge={
                "axis": {"range": [0, max_value]},
                "bar": {"color": bar_color},
                "steps": [
                    {"range": [0, max_value * 0.6], "color": "#E8F0E5"},
                    {"range": [max_value * 0.6, max_value * 0.85], "color": "#FBF6C8"},
                    {"range": [max_value * 0.85, max_value], "color": "#F3D2CC"},
                ],
            },
        )
    ).update_layout(height=280, margin=dict(l=20, r=20, t=50, b=10))


# ── driver camera (persistent MJPEG embed) ────────────────────────────────────
def render_camera():
    """Embed the MJPEG stream from face.py.

    The img src is built from the browser's own hostname so it works no matter
    which IP/host is used to reach the dashboard. Rendered once (outside the
    auto-refreshing fragments) so the video connection is never torn down.
    """
    components.html(
        f"""
        <div style="width:100%;text-align:center;background:#111;border-radius:8px;padding:4px">
          <img id="drivercam"
               style="width:100%;max-height:{CAMERA_HEIGHT}px;object-fit:contain;border-radius:6px"
               onerror="this.style.display='none';document.getElementById('camoff').style.display='block'"/>
          <div id="camoff" style="display:none;color:#aaa;padding:60px 10px">
            Camara sin senal &mdash; corre <code>face.py</code> (stream en el puerto {CAMERA_STREAM_PORT})
          </div>
        </div>
        <script>
          (function() {{
            // Inside components.html the iframe is srcdoc, so its own
            // location.hostname is empty — read the real host from the parent.
            let host = '';
            try {{ host = window.parent.location.hostname; }} catch (e) {{}}
            if (!host) host = window.location.hostname;
            const img = document.getElementById('drivercam');
            img.src = 'http://' + host + ':{CAMERA_STREAM_PORT}/stream?t=' + Date.now();
          }})();
        </script>
        """,
        height=CAMERA_HEIGHT + 30,
    )


# ── live fragments (re-poll every REFRESH_SECONDS without a full rerun) ────────
@st.fragment(run_every=REFRESH_SECONDS)
def driver_status_panel():
    looking = latest_value("looking", DRIVER_MEASUREMENT)
    st.markdown("**Estado del conductor**")
    if looking is None:
        badge("SIN DATOS", JD_GREY)
    elif int(looking) == 1:
        badge("ATENTO", JD_GREEN)
    else:
        badge("DISTRAIDO", JD_RED)
    st.caption(f"Actualizado: {datetime.now(timezone.utc).astimezone():%H:%M:%S}")


@st.fragment(run_every=REFRESH_SECONDS)
def live_metrics():
    engine_rpm = latest_value("engine_speed")
    vehicle_speed = latest_value("vehicle_speed")
    gear = latest_value("gear")
    mode = get_control_mode()

    # top row: engine gauge | gear | control mode
    c1, c2, c3 = st.columns([2, 1, 1])
    with c1:
        st.plotly_chart(
            gauge(engine_rpm, "Engine RPM", ENGINE_RPM_MAX, "rpm"),
            width="stretch",
        )
    with c2:
        st.metric("Gear", f"{int(gear)}" if gear is not None else "-")
    with c3:
        st.metric("Control Mode", (mode or "-").upper())

    # vehicle speed gauge + time series
    g1, g2 = st.columns([1, 3])
    with g1:
        st.plotly_chart(
            gauge(vehicle_speed, "Vehicle Speed", VEHICLE_SPEED_MAX, "km/h", JD_GREEN_DARK),
            width="stretch",
        )
    with g2:
        st.subheader("Vehicle Speed - last 10 min")
        speed_df = time_series("vehicle_speed")
        if speed_df.empty:
            st.info("No vehicle speed data in the selected window.")
        else:
            fig = go.Figure(
                go.Scatter(
                    x=speed_df["time"],
                    y=speed_df["vehicle_speed"],
                    mode="lines",
                    line=dict(color=JD_GREEN, width=2),
                    fill="tozeroy",
                    fillcolor="rgba(54,124,43,0.15)",
                )
            )
            fig.update_layout(
                height=300,
                margin=dict(l=10, r=10, t=10, b=10),
                xaxis_title="Time",
                yaxis_title="km/h",
            )
            st.plotly_chart(fig, width="stretch")

    # engine rpm trend
    st.subheader("Engine RPM - last 10 min")
    rpm_df = time_series("engine_speed")
    if rpm_df.empty:
        st.info("No engine RPM data in the selected window.")
    else:
        fig = go.Figure(
            go.Scatter(
                x=rpm_df["time"],
                y=rpm_df["engine_speed"],
                mode="lines",
                line=dict(color=JD_GREEN, width=2),
            )
        )
        fig.update_layout(
            height=260,
            margin=dict(l=10, r=10, t=10, b=10),
            xaxis_title="Time",
            yaxis_title="rpm",
        )
        st.plotly_chart(fig, width="stretch")

    if err := st.session_state.get("influx_error"):
        st.caption(f"InfluxDB: {err}")


# ── sidebar: control mode (full rerun on interaction) ─────────────────────────
with st.sidebar:
    st.header("Control")
    current_mode = get_control_mode()
    if current_mode is None:
        st.error("API not reachable (port 5000)")
    else:
        st.success(f"Mode: **{current_mode.upper()}**")

    new_mode = st.radio(
        "Set control mode",
        options=["local", "remote"],
        index=0 if (current_mode or "local") == "local" else 1,
    )
    if st.button("Change mode", width="stretch"):
        ok, payload = set_control_mode(new_mode)
        if ok:
            st.success(f"Mode set to {payload.get('mode', new_mode).upper()}")
        else:
            st.error(payload.get("error", "Failed to set mode"))

    st.divider()
    st.caption(f"Auto-refresh cada {REFRESH_SECONDS}s")


# ── branded header ────────────────────────────────────────────────────────────
st.markdown(
    f"""
    <div style="background:{JD_GREEN};padding:16px 24px;border-radius:10px;
                display:flex;align-items:center;gap:18px;margin-bottom:14px">
      <span style="font-family:'Arial Black',Arial,sans-serif;font-weight:900;
                   font-size:32px;color:{JD_YELLOW};letter-spacing:1px">JOHN DEERE</span>
      <span style="color:#ffffff;font-size:20px;font-weight:600">
        Tractor Telemetry Dashboard</span>
    </div>
    """,
    unsafe_allow_html=True,
)

# Driver monitoring: live camera (persistent) + live attention status
st.subheader("Monitoreo del conductor")
cam_col, status_col = st.columns([3, 1])
with cam_col:
    render_camera()  # rendered once — keeps the MJPEG connection alive
with status_col:
    driver_status_panel()

st.divider()
live_metrics()
