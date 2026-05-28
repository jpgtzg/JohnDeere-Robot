import math

import cv2
import mediapipe as mp
import numpy as np
from mediapipe.tasks.python import BaseOptions
from mediapipe.tasks.python.vision import (
    FaceLandmarker,
    FaceLandmarkerOptions,
    RunningMode,
)

MODEL_PATH = "face_landmarker.task"

LEFT_EYE  = [33, 160, 158, 133, 153, 144]
RIGHT_EYE = [362, 385, 387, 263, 373, 380]

LEFT_IRIS  = [468, 469, 470, 471, 472]
RIGHT_IRIS = [473, 474, 475, 476, 477]

LEFT_INNER,  LEFT_OUTER  = 133, 33
LEFT_TOP,    LEFT_BOTTOM = 159, 145
RIGHT_INNER, RIGHT_OUTER = 362, 263
RIGHT_TOP,   RIGHT_BOTTOM = 386, 374

GAZE_H_THRESHOLD = 0.12
GAZE_V_THRESHOLD = 0.12
YAW_THRESHOLD    = 20.0   # degrees left/right
PITCH_THRESHOLD  = 20.0   # degrees up/down


def iris_center(landmarks, indices):
    xs = [landmarks[i].x for i in indices]
    ys = [landmarks[i].y for i in indices]
    return sum(xs) / len(xs), sum(ys) / len(ys)


def gaze_ratio(cx, cy, inner, outer, top, bottom, lm):
    eye_w = abs(lm[outer].x - lm[inner].x)
    eye_h = abs(lm[bottom].y - lm[top].y)
    if eye_w < 1e-6 or eye_h < 1e-6:
        return 0.5, 0.5
    h_ratio = (cx - min(lm[inner].x, lm[outer].x)) / eye_w
    v_ratio = (cy - lm[top].y) / eye_h
    return h_ratio, v_ratio


def head_angles(matrix):
    """Extract yaw and pitch in degrees from a 4x4 facial transformation matrix."""
    m = np.array(matrix.data).reshape(4, 4)
    # Rotation submatrix
    r = m[:3, :3]
    pitch = math.degrees(math.asin(-r[2, 0]))
    yaw   = math.degrees(math.atan2(r[1, 0], r[0, 0]))
    return yaw, pitch


def looking_at_camera(lm, transform_matrix):
    if len(lm) < 478:
        return None, (0.5, 0.5), (0.5, 0.5), (0.0, 0.0)

    lx, ly = iris_center(lm, LEFT_IRIS)
    rx, ry = iris_center(lm, RIGHT_IRIS)

    lh, lv = gaze_ratio(lx, ly, LEFT_INNER, LEFT_OUTER, LEFT_TOP, LEFT_BOTTOM, lm)
    rh, rv = gaze_ratio(rx, ry, RIGHT_INNER, RIGHT_OUTER, RIGHT_TOP, RIGHT_BOTTOM, lm)

    avg_h = (lh + rh) / 2
    avg_v = (lv + rv) / 2

    yaw, pitch = head_angles(transform_matrix) if transform_matrix is not None else (0.0, 0.0)

    looking = (
        abs(avg_h - 0.5) < GAZE_H_THRESHOLD
        and abs(avg_v - 0.5) < GAZE_V_THRESHOLD
        and abs(yaw)   < YAW_THRESHOLD
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


def main():
    options = FaceLandmarkerOptions(
        base_options=BaseOptions(model_asset_path=MODEL_PATH),
        running_mode=RunningMode.VIDEO,
        num_faces=1,
        min_face_detection_confidence=0.5,
        min_face_presence_confidence=0.5,
        min_tracking_confidence=0.5,
        output_facial_transformation_matrixes=True,
    )

    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        print("Cannot open webcam")
        return

    with FaceLandmarker.create_from_options(options) as detector:
        while True:
            ret, frame = cap.read()
            if not ret:
                break

            timestamp_ms = int(cap.get(cv2.CAP_PROP_POS_MSEC))
            rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=rgb)
            result = detector.detect_for_video(mp_image, timestamp_ms)

            matrices = result.facial_transformation_matrixes or []

            for i, face in enumerate(result.face_landmarks or []):
                matrix = matrices[i] if i < len(matrices) else None
                looking, (lh, lv), (rh, rv), (yaw, pitch) = looking_at_camera(face, matrix)

                eye_color = (0, 255, 0) if looking else (0, 0, 255)
                draw_eye(frame, face, LEFT_EYE,  eye_color)
                draw_eye(frame, face, RIGHT_EYE, eye_color)
                draw_iris(frame, face, LEFT_IRIS,  (255, 255, 0))
                draw_iris(frame, face, RIGHT_IRIS, (255, 255, 0))

                label = "Looking at camera" if looking else "Not looking"
                cv2.putText(frame, label, (20, 40),
                            cv2.FONT_HERSHEY_SIMPLEX, 1.0, eye_color, 2)

                gaze_dbg = f"iris  L h:{lh:.2f} v:{lv:.2f}  R h:{rh:.2f} v:{rv:.2f}"
                pose_dbg = f"head  yaw:{yaw:+.1f}  pitch:{pitch:+.1f}"
                cv2.putText(frame, gaze_dbg, (20, 75),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.5, (200, 200, 200), 1)
                cv2.putText(frame, pose_dbg, (20, 95),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.5, (200, 200, 200), 1)

            cv2.imshow("Gaze Detection", frame)
            if cv2.waitKey(1) & 0xFF == ord("q"):
                break

    cap.release()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()
