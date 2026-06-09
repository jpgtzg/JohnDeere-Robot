import math
import os
import zipfile

import cv2
import numpy as np
from ai_edge_litert.interpreter import Interpreter

from influx import write_to_influxdb

# ── model setup ───────────────────────────────────────────────────────────────
TASK_PATH = "face_landmarker.task"
_MODEL_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), ".models")
DETECTOR_PATH = os.path.join(_MODEL_DIR, "face_detector.tflite")
LANDMARKS_PATH = os.path.join(_MODEL_DIR, "face_landmarks_detector.tflite")


def _ensure_models():
    """Extract TFLite models from the .task bundle on first run."""
    if os.path.exists(DETECTOR_PATH) and os.path.exists(LANDMARKS_PATH):
        return
    os.makedirs(_MODEL_DIR, exist_ok=True)
    with zipfile.ZipFile(TASK_PATH) as z:
        with open(DETECTOR_PATH, "wb") as f:
            f.write(z.read("face_detector.tflite"))
        with open(LANDMARKS_PATH, "wb") as f:
            f.write(z.read("face_landmarks_detector.tflite"))


# ── landmark indices (same 478-point topology as MediaPipe) ───────────────────
LEFT_EYE = [33, 160, 158, 133, 153, 144]
RIGHT_EYE = [362, 385, 387, 263, 373, 380]
LEFT_IRIS = [468, 469, 470, 471, 472]
RIGHT_IRIS = [473, 474, 475, 476, 477]
LEFT_INNER, LEFT_OUTER = 133, 33
LEFT_TOP, LEFT_BOTTOM = 159, 145
RIGHT_INNER, RIGHT_OUTER = 362, 263
RIGHT_TOP, RIGHT_BOTTOM = 386, 374

GAZE_H_THRESHOLD = 0.12
GAZE_V_THRESHOLD = 0.12
YAW_THRESHOLD = 20.0
PITCH_THRESHOLD = 20.0

# Canonical 3D face model (mm) used by solvePnP for head pose
_FACE_3D = np.array(
    [
        [0.0, 0.0, 0.0],  # nose tip        lm 1
        [0.0, -330.0, -65.0],  # chin            lm 152
        [-225.0, 170.0, -135.0],  # left eye left   lm 33
        [225.0, 170.0, -135.0],  # right eye right lm 263
        [-150.0, -150.0, -125.0],  # left mouth      lm 61
        [150.0, -150.0, -125.0],  # right mouth     lm 291
    ],
    dtype=np.float64,
)
_POSE_IDX = [1, 152, 33, 263, 61, 291]


# ── BlazeFace anchors: 16×16 grid ×2 + 8×8 grid ×6 = 896 total ───────────────
def _make_anchors():
    a = []
    for y in range(16):
        for x in range(16):
            cx, cy = (x + 0.5) / 16.0, (y + 0.5) / 16.0
            a.append([cx, cy])
            a.append([cx, cy])
    for y in range(8):
        for x in range(8):
            cx, cy = (x + 0.5) / 8.0, (y + 0.5) / 8.0
            a += [[cx, cy]] * 6
    return np.array(a, dtype=np.float32)  # [896, 2]


_ANCHORS = _make_anchors()


class _LM:
    """Minimal landmark point with .x / .y / .z attributes."""

    __slots__ = ("x", "y", "z")

    def __init__(self, x, y, z):
        self.x = x
        self.y = y
        self.z = z


# ── face detection (BlazeFace short-range) ────────────────────────────────────
class _FaceDetector:
    _SCORE = 0.5
    _IOU = 0.3

    def __init__(self):
        self._interp = Interpreter(model_path=DETECTOR_PATH, num_threads=4)
        self._interp.allocate_tensors()
        ins = self._interp.get_input_details()
        outs = self._interp.get_output_details()
        self._in = ins[0]["index"]
        self._reg = outs[0]["index"]  # regressors   [1, 896, 16]
        self._cls = outs[1]["index"]  # classificators [1, 896, 1]

    def detect(self, rgb):
        """Return list of (x1,y1,x2,y2) pixel boxes."""
        h, w = rgb.shape[:2]
        inp = cv2.resize(rgb, (128, 128))
        inp = (inp.astype(np.float32) / 127.5 - 1.0)[np.newaxis]
        self._interp.set_tensor(self._in, inp)
        self._interp.invoke()

        regs = self._interp.get_tensor(self._reg)[0]  # [896, 16]
        logits = self._interp.get_tensor(self._cls)[0, :, 0]  # [896]
        scores = 1.0 / (1.0 + np.exp(-np.clip(logits, -88.0, 88.0)))

        # SSD decode: offset/128 + anchor_center
        cx = regs[:, 0] / 128.0 + _ANCHORS[:, 0]
        cy = regs[:, 1] / 128.0 + _ANCHORS[:, 1]
        bw = regs[:, 2] / 128.0
        bh = regs[:, 3] / 128.0

        keep = scores > self._SCORE
        if not keep.any():
            return []
        cx, cy, bw, bh, sc = cx[keep], cy[keep], bw[keep], bh[keep], scores[keep]

        x1 = np.clip((cx - bw / 2) * w, 0, w - 1).astype(int)
        y1 = np.clip((cy - bh / 2) * h, 0, h - 1).astype(int)
        x2 = np.clip((cx + bw / 2) * w, 0, w - 1).astype(int)
        y2 = np.clip((cy + bh / 2) * h, 0, h - 1).astype(int)

        boxes_xywh = [
            (int(x1[i]), int(y1[i]), int(x2[i] - x1[i]), int(y2[i] - y1[i]))
            for i in range(len(x1))
        ]
        idxs = cv2.dnn.NMSBoxes(boxes_xywh, sc.tolist(), self._SCORE, self._IOU)
        if len(idxs) == 0:
            return []
        idxs = np.array(idxs).flatten()
        return [(x1[i], y1[i], x2[i], y2[i]) for i in idxs]


# ── 478-point face landmark detector ─────────────────────────────────────────
class _FaceLandmarker:
    _SIZE = 256
    _PAD = 0.25  # extra padding around detected face box

    def __init__(self):
        self._interp = Interpreter(model_path=LANDMARKS_PATH, num_threads=4)
        self._interp.allocate_tensors()
        ins = self._interp.get_input_details()
        outs = self._interp.get_output_details()
        self._in = ins[0]["index"]
        self._lm = outs[0]["index"]  # Identity [1,1,1,1434] = 478×3

    def detect(self, rgb, face_box):
        """Return list of 478 _LM in normalised [0,1] frame coords, or None."""
        h, w = rgb.shape[:2]
        x1, y1, x2, y2 = face_box
        bw, bh = x2 - x1, y2 - y1
        px, py = int(bw * self._PAD), int(bh * self._PAD)
        cx1, cy1 = max(0, x1 - px), max(0, y1 - py)
        cx2, cy2 = min(w, x2 + px), min(h, y2 + py)

        crop = rgb[cy1:cy2, cx1:cx2]
        if crop.size == 0:
            return None
        ch, cw = crop.shape[:2]

        inp = cv2.resize(crop, (self._SIZE, self._SIZE))
        inp = (inp.astype(np.float32) / 255.0)[np.newaxis]
        self._interp.set_tensor(self._in, inp)
        self._interp.invoke()

        raw = self._interp.get_tensor(self._lm).reshape(478, 3)
        # Map crop space [0,256] → normalised frame [0,1]
        return [
            _LM(
                (raw[i, 0] / self._SIZE) * cw / w + cx1 / w,
                (raw[i, 1] / self._SIZE) * ch / h + cy1 / h,
                raw[i, 2] / self._SIZE,
            )
            for i in range(478)
        ]


# ── gaze / head-pose helpers (unchanged logic from original) ──────────────────
def iris_center(lm, indices):
    xs = [lm[i].x for i in indices]
    ys = [lm[i].y for i in indices]
    return sum(xs) / len(xs), sum(ys) / len(ys)


def gaze_ratio(cx, cy, inner, outer, top, bottom, lm):
    eye_w = abs(lm[outer].x - lm[inner].x)
    eye_h = abs(lm[bottom].y - lm[top].y)
    if eye_w < 1e-6 or eye_h < 1e-6:
        return 0.5, 0.5
    h_ratio = (cx - min(lm[inner].x, lm[outer].x)) / eye_w
    v_ratio = (cy - lm[top].y) / eye_h
    return h_ratio, v_ratio


def head_angles(lm, frame_w, frame_h):
    """Yaw and pitch in degrees via solvePnP (replaces mediapipe transform matrix)."""
    pts2d = np.array(
        [[lm[i].x * frame_w, lm[i].y * frame_h] for i in _POSE_IDX],
        dtype=np.float64,
    )
    focal = float(frame_w)
    cam = np.array(
        [[focal, 0, frame_w / 2.0], [0, focal, frame_h / 2.0], [0, 0, 1.0]],
        dtype=np.float64,
    )
    ok, rvec, _ = cv2.solvePnP(_FACE_3D, pts2d, cam, None, flags=cv2.SOLVEPNP_ITERATIVE)
    if not ok:
        return 0.0, 0.0
    r, _ = cv2.Rodrigues(rvec)
    pitch = math.degrees(math.asin(max(-1.0, min(1.0, -r[2, 0]))))
    yaw = math.degrees(math.atan2(r[1, 0], r[0, 0]))
    return yaw, pitch


def looking_at_camera(lm, frame_w, frame_h):
    if len(lm) < 478:
        return None, (0.5, 0.5), (0.5, 0.5), (0.0, 0.0)

    lx, ly = iris_center(lm, LEFT_IRIS)
    rx, ry = iris_center(lm, RIGHT_IRIS)

    lh, lv = gaze_ratio(lx, ly, LEFT_INNER, LEFT_OUTER, LEFT_TOP, LEFT_BOTTOM, lm)
    rh, rv = gaze_ratio(rx, ry, RIGHT_INNER, RIGHT_OUTER, RIGHT_TOP, RIGHT_BOTTOM, lm)

    avg_h = (lh + rh) / 2
    avg_v = (lv + rv) / 2
    yaw, pitch = head_angles(lm, frame_w, frame_h)

    looking = (
        abs(avg_h - 0.5) < GAZE_H_THRESHOLD
        and abs(avg_v - 0.5) < GAZE_V_THRESHOLD
        and abs(yaw) < YAW_THRESHOLD
        and abs(pitch) < PITCH_THRESHOLD
    )
    return looking, (lh, lv), (rh, rv), (yaw, pitch)


def draw_eye(frame, lm, indices, color):
    h, w = frame.shape[:2]
    pts = [(int(lm[i].x * w), int(lm[i].y * h)) for i in indices]
    for pt in pts:
        cv2.circle(frame, pt, 2, color, -1)
    for j in range(len(pts)):
        cv2.line(frame, pts[j], pts[(j + 1) % len(pts)], color, 1)


def draw_iris(frame, lm, indices, color):
    h, w = frame.shape[:2]
    cx, cy = iris_center(lm, indices)
    cv2.circle(frame, (int(cx * w), int(cy * h)), 3, color, -1)


# ── main ──────────────────────────────────────────────────────────────────────
def main():
    _ensure_models()
    detector = _FaceDetector()
    landmarker = _FaceLandmarker()

    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        print("Cannot open webcam")
        return

    while True:
        ret, frame = cap.read()
        if not ret:
            break

        fh, fw = frame.shape[:2]
        rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        faces = detector.detect(rgb)

        for face_box in faces:
            lm = landmarker.detect(rgb, face_box)
            if lm is None:
                continue

            looking, (lh, lv), (rh, rv), (yaw, pitch) = looking_at_camera(lm, fw, fh)

            if looking is not None:
                write_to_influxdb(
                    "driver_metrics", "looking", int(looking), "Raspberry_Pi"
                )

            eye_color = (0, 255, 0) if looking else (0, 0, 255)
            draw_eye(frame, lm, LEFT_EYE, eye_color)
            draw_eye(frame, lm, RIGHT_EYE, eye_color)
            draw_iris(frame, lm, LEFT_IRIS, (255, 255, 0))
            draw_iris(frame, lm, RIGHT_IRIS, (255, 255, 0))

            label = "Looking at camera" if looking else "Not looking"
            gaze_dbg = f"iris  L h:{lh:.2f} v:{lv:.2f}  R h:{rh:.2f} v:{rv:.2f}"
            pose_dbg = f"head  yaw:{yaw:+.1f}  pitch:{pitch:+.1f}"
            cv2.putText(
                frame, label, (20, 40), cv2.FONT_HERSHEY_SIMPLEX, 1.0, eye_color, 2
            )
            cv2.putText(
                frame,
                gaze_dbg,
                (20, 75),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.5,
                (200, 200, 200),
                1,
            )
            cv2.putText(
                frame,
                pose_dbg,
                (20, 95),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.5,
                (200, 200, 200),
                1,
            )

        cv2.imshow("Gaze Detection", frame)
        if cv2.waitKey(1) & 0xFF == ord("q"):
            break

    cap.release()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()
