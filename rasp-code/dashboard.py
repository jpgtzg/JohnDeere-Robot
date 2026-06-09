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
CAMERA_HEIGHT = 340

# John Deere brand palette
JD_GREEN = "#367C2B"
JD_GREEN_DARK = "#2A5E22"
JD_YELLOW = "#FFDE00"
JD_RED = "#C0392B"
JD_GREY = "#6B6B6B"

st.set_page_config(page_title="John Deere — Tractor Telemetry", layout="wide")

st.markdown(
    """
    <style>
      #MainMenu, footer {visibility: hidden;}
      [data-testid="stHeader"] {display: none;}
      .block-container {padding-top: 2.2rem; padding-bottom: 2rem; max-width: 1500px;}
      div[data-testid="stMetric"] {
        background: #FFFFFF; border: 1px solid #E3E8E3; border-radius: 12px;
        padding: 12px 16px;
      }
      div[data-testid="stVerticalBlockBorderWrapper"] {
        box-shadow: 0 1px 4px rgba(0,0,0,0.05);
      }
    </style>
    """,
    unsafe_allow_html=True,
)


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
    out = df.rename(columns={"_time": "time", "_value": field})[["time", field]].copy()
    out["time"] = pd.to_datetime(out["time"])  # real datetime axis, not categorical
    return out.sort_values("time")


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


def send_remote(action: str, value: float):
    """POST a remote command (action = 'acceleration' or 'brake')."""
    try:
        resp = requests.post(
            f"{API_URL}/remote/{action}", json={"value": value}, timeout=2
        )
        return resp.ok, resp.json()
    except requests.RequestException as exc:
        return False, {"error": str(exc)}


# ── ui helpers ────────────────────────────────────────────────────────────────
def card_title(text: str):
    st.markdown(
        f"<div style='font-weight:700;font-size:15px;color:{JD_GREEN_DARK};"
        f"margin-bottom:6px'>{text}</div>",
        unsafe_allow_html=True,
    )


def kpi_card(label: str, value: str, accent: str = JD_GREEN):
    st.markdown(
        f"""
        <div style="background:#fff;border:1px solid #E3E8E3;border-left:6px solid {accent};
                    border-radius:12px;padding:14px 18px;box-shadow:0 1px 3px rgba(0,0,0,.05)">
          <div style="font-size:12px;color:{JD_GREY};font-weight:700;
                      text-transform:uppercase;letter-spacing:.6px">{label}</div>
          <div style="font-size:30px;color:#1A1A1A;font-weight:800;line-height:1.2;
                      margin-top:4px">{value}</div>
        </div>
        """,
        unsafe_allow_html=True,
    )


def gauge(value, max_value, suffix, bar_color=JD_GREEN):
    val = value if value is not None else 0
    return go.Figure(
        go.Indicator(
            mode="gauge+number",
            value=val,
            number={"suffix": f" {suffix}", "font": {"size": 26}},
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
    ).update_layout(
        height=220,
        margin=dict(l=20, r=20, t=10, b=10),
        paper_bgcolor="rgba(0,0,0,0)",
    )


def line_fig(df, field, color, area=False, y_title=None, markers=False):
    fig = go.Figure(
        go.Scatter(
            x=df["time"],
            y=df[field],
            mode="lines+markers" if markers else "lines",
            line=dict(color=color, width=2),
            marker=dict(color=color, size=6) if markers else None,
            fill="tozeroy" if area else None,
            fillcolor="rgba(54,124,43,0.13)" if area else None,
        )
    )
    fig.update_layout(
        height=240,
        margin=dict(l=8, r=8, t=8, b=8),
        xaxis_title="Tiempo",
        yaxis_title=y_title,
        plot_bgcolor="#FFFFFF",
        paper_bgcolor="rgba(0,0,0,0)",
        xaxis=dict(type="date", showgrid=False, tickformat="%H:%M:%S"),
        yaxis=dict(gridcolor="#EEF2EE"),
    )
    return fig


# ── driver camera (persistent MJPEG embed) ────────────────────────────────────
def render_camera():
    """Embed the MJPEG stream from face.py.

    The img src is built from the parent frame's hostname (components.html runs
    in an srcdoc iframe whose own hostname is empty) so it works for any host.
    Rendered once, outside the auto-refreshing fragments, so the video
    connection is never torn down.
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
def kpi_strip():
    rpm = latest_value("engine_speed")
    spd = latest_value("vehicle_speed")
    gear = latest_value("gear")
    mode = get_control_mode()
    looking = latest_value("looking", DRIVER_MEASUREMENT)

    cols = st.columns(5)
    with cols[0]:
        kpi_card("Engine RPM", f"{int(rpm)}" if rpm is not None else "—")
    with cols[1]:
        kpi_card("Velocidad", f"{spd:.0f} km/h" if spd is not None else "—")
    with cols[2]:
        kpi_card("Gear", f"{int(gear)}" if gear is not None else "—")
    with cols[3]:
        kpi_card("Modo", (mode or "—").upper(), JD_GREEN_DARK)
    with cols[4]:
        if looking is None:
            kpi_card("Conductor", "SIN DATOS", JD_GREY)
        elif int(looking) == 1:
            kpi_card("Conductor", "ATENTO", JD_GREEN)
        else:
            kpi_card("Conductor", "DISTRAÍDO", JD_RED)


@st.fragment(run_every=REFRESH_SECONDS)
def driver_status_panel():
    looking = latest_value("looking", DRIVER_MEASUREMENT)
    if looking is None:
        color, text = JD_GREY, "SIN DATOS"
    elif int(looking) == 1:
        color, text = JD_GREEN, "ATENTO"
    else:
        color, text = JD_RED, "DISTRAÍDO"
    st.markdown(
        f"<div style='background:{color};color:#fff;padding:18px;border-radius:10px;"
        f"text-align:center;font-size:22px;font-weight:800;letter-spacing:1px'>{text}</div>",
        unsafe_allow_html=True,
    )
    st.caption(f"Actualizado: {datetime.now(timezone.utc).astimezone():%H:%M:%S}")


@st.fragment(run_every=REFRESH_SECONDS)
def detail_panels():
    rpm = latest_value("engine_speed")
    spd = latest_value("vehicle_speed")
    left, right = st.columns(2)

    with left:
        with st.container(border=True):
            card_title("Motor — RPM")
            st.plotly_chart(gauge(rpm, ENGINE_RPM_MAX, "rpm"), width="stretch")
            st.caption("RPM vs Tiempo (últimos 10 min)")
            rpm_df = time_series("engine_speed")
            if rpm_df.empty:
                st.info("Sin datos de RPM en la ventana.")
            else:
                st.plotly_chart(
                    line_fig(rpm_df, "engine_speed", JD_GREEN, y_title="rpm"),
                    width="stretch",
                )

    with right:
        with st.container(border=True):
            card_title("Velocidad del vehículo")
            st.plotly_chart(
                gauge(spd, VEHICLE_SPEED_MAX, "km/h", JD_GREEN_DARK), width="stretch"
            )
            st.caption("Velocidad vs Tiempo (últimos 10 min)")
            spd_df = time_series("vehicle_speed")
            if spd_df.empty:
                st.info("Sin datos de velocidad en la ventana.")
            else:
                st.plotly_chart(
                    line_fig(spd_df, "vehicle_speed", JD_GREEN, y_title="km/h", markers=True),
                    width="stretch",
                )

    if err := st.session_state.get("influx_error"):
        st.caption(f"InfluxDB: {err}")


# ── control panel (mode + remote driving) ────────────────────────────────────
def _apply_mode():
    ok, payload = set_control_mode(st.session_state.mode_select)
    st.session_state["mode_result"] = (ok, payload)


def _send_accel():
    st.session_state["accel_result"] = send_remote("acceleration", st.session_state.accel_val)


def _send_brake():
    st.session_state["brake_result"] = send_remote("brake", st.session_state.brake_val)


def _show_remote_result(key: str):
    res = st.session_state.get(key)
    if res is None:
        return
    ok, payload = res
    msg = payload.get("command", payload.get("error", "")) if isinstance(payload, dict) else ""
    (st.success if ok else st.error)(msg)


def control_panel():
    with st.container(border=True):
        card_title("Panel de control")
        mode_col, remote_col = st.columns([1, 2])

        # — control mode —
        with mode_col:
            live_mode = get_control_mode()
            if live_mode is None:
                st.error("API no disponible (puerto 5000)")
            else:
                st.markdown(
                    f"<div style='background:{JD_GREEN};color:#fff;padding:10px 14px;"
                    f"border-radius:8px;font-weight:700;text-align:center'>"
                    f"MODO ACTUAL: {live_mode.upper()}</div>",
                    unsafe_allow_html=True,
                )
            if "mode_select" not in st.session_state:
                st.session_state.mode_select = live_mode or "local"
            st.radio(
                "Cambiar modo de control",
                options=["local", "remote"],
                key="mode_select",
                on_change=_apply_mode,
                horizontal=True,
            )
            res = st.session_state.get("mode_result")
            if res is not None:
                ok, payload = res
                if ok:
                    st.success(f"Modo aplicado: {payload.get('mode', '').upper()}")
                else:
                    st.error(payload.get("error", "No se pudo cambiar el modo"))

        # — remote driving (only active in REMOTE mode) —
        with remote_col:
            remote_on = live_mode == "remote"
            st.markdown("**Control remoto**")
            if not remote_on:
                st.info("Disponible solo en modo REMOTE")

            a_col, b_col = st.columns(2)
            with a_col:
                st.slider(
                    "Aceleración (%)", 0, 100, 0, disabled=not remote_on,
                    key="accel_val", on_change=_send_accel,
                )
                _show_remote_result("accel_result")
            with b_col:
                st.slider(
                    "Freno (%)", 0, 100, 0, disabled=not remote_on,
                    key="brake_val", on_change=_send_brake,
                )
                _show_remote_result("brake_result")

            if st.button(
                "PARO — freno máx.",
                type="primary",
                disabled=not remote_on,
                width="stretch",
            ):
                send_remote("acceleration", 0)
                ok, _ = send_remote("brake", 100)
                (st.success if ok else st.error)(
                    "Freno de emergencia enviado" if ok else "Error"
                )


# ── branded header ────────────────────────────────────────────────────────────
st.markdown(
    f"""
    <div style="background:{JD_GREEN};padding:16px 26px;border-radius:12px;
                display:flex;align-items:center;gap:18px;margin-bottom:16px;
                box-shadow:0 2px 6px rgba(0,0,0,.12)">
      <span style="font-family:'Arial Black',Arial,sans-serif;font-weight:900;
                   font-size:34px;color:{JD_YELLOW};letter-spacing:1px">JOHN DEERE</span>
      <span style="color:#ffffff;font-size:20px;font-weight:600">
        Tractor Telemetry Dashboard</span>
    </div>
    """,
    unsafe_allow_html=True,
)

# KPI strip
kpi_strip()
st.write("")

# Control panel (mode + remote driving) — before the camera
control_panel()
st.write("")

# Driver monitoring: live camera (persistent) + live attention status
with st.container(border=True):
    card_title("Monitoreo del conductor")
    cam_col, status_col = st.columns([3, 1])
    with cam_col:
        render_camera()  # rendered once — keeps the MJPEG connection alive
    with status_col:
        st.markdown("**Estado**")
        driver_status_panel()

st.write("")

# Detail gauges + time series
detail_panels()
