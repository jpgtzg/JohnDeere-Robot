# Gaze Detection Algorithm — Explanation

## Overview

The algorithm answers one question every frame: **is the person looking at the camera?**

It does this by combining two independent signals:
1. **Iris position** — where inside the eye socket is the iris sitting?
2. **Head rotation** — is the face itself pointing at the camera?

Both must pass their respective thresholds simultaneously for the result to be `True`.

---

## Imports and dependencies

```python
import math
import cv2
import mediapipe as mp
import numpy as np
```

- `cv2` (OpenCV) handles webcam capture and drawing on frames.
- `mediapipe` runs the neural network that locates 478 facial landmarks per frame.
- `numpy` is used to reshape the 4×4 transformation matrix returned by MediaPipe.
- `math` provides `asin` and `atan2` for converting the rotation matrix to angles.

---

## Landmark index constants

```python
LEFT_EYE  = [33, 160, 158, 133, 153, 144]
RIGHT_EYE = [362, 385, 387, 263, 373, 380]
```

MediaPipe's Face Mesh model assigns a fixed integer index to each of the 478 points on the face. These two lists are the indices of the six points that form the eyelid contour for each eye. They are used purely for drawing the outline.

```python
LEFT_IRIS  = [468, 469, 470, 471, 472]
RIGHT_IRIS = [473, 474, 475, 476, 477]
```

Indices 468–477 are the **iris landmarks**, only present in the 478-point model (enabled by default in the `face_landmarker.task` file). Five points trace the edge of each iris. Their average position gives the iris center.

```python
LEFT_INNER,  LEFT_OUTER  = 133, 33
LEFT_TOP,    LEFT_BOTTOM = 159, 145
RIGHT_INNER, RIGHT_OUTER = 362, 263
RIGHT_TOP,   RIGHT_BOTTOM = 386, 374
```

These four boundary points per eye define a bounding box around each eye socket — inner corner, outer corner, top eyelid, bottom eyelid. They are used to measure how far the iris has deviated from the center of the socket.

---

## Thresholds

```python
GAZE_H_THRESHOLD = 0.12
GAZE_V_THRESHOLD = 0.12
YAW_THRESHOLD    = 20.0
PITCH_THRESHOLD  = 20.0
```

- `GAZE_H_THRESHOLD` / `GAZE_V_THRESHOLD`: the iris ratio must be within this distance of `0.5` (dead center) in both axes. `0.12` means the iris can be up to 12% of the eye width/height away from center before it counts as "not looking". Lower = stricter.
- `YAW_THRESHOLD`: maximum allowed left/right head rotation in degrees.
- `PITCH_THRESHOLD`: maximum allowed up/down head rotation in degrees.

---

## `iris_center(landmarks, indices)`

```python
def iris_center(landmarks, indices):
    xs = [landmarks[i].x for i in indices]
    ys = [landmarks[i].y for i in indices]
    return sum(xs) / len(xs), sum(ys) / len(ys)
```

Takes the five iris landmark points and computes their **centroid** — a simple average of the x and y coordinates. All landmark coordinates from MediaPipe are normalized to the range `[0.0, 1.0]` relative to the frame dimensions (0 = left/top edge, 1 = right/bottom edge).

---

## `gaze_ratio(cx, cy, inner, outer, top, bottom, lm)`

```python
def gaze_ratio(cx, cy, inner, outer, top, bottom, lm):
    eye_w = abs(lm[outer].x - lm[inner].x)
    eye_h = abs(lm[bottom].y - lm[top].y)
    if eye_w < 1e-6 or eye_h < 1e-6:
        return 0.5, 0.5
    h_ratio = (cx - min(lm[inner].x, lm[outer].x)) / eye_w
    v_ratio = (cy - lm[top].y) / eye_h
    return h_ratio, v_ratio
```

This is the core of the iris gaze estimation. Given the iris center `(cx, cy)` and the four boundary landmarks of the eye socket, it computes two ratios:

**Horizontal ratio:**
```
h_ratio = (iris_center_x - left_edge_x) / eye_width
```
- `0.0` means the iris is at the inner (nasal) edge.
- `1.0` means the iris is at the outer edge.
- `0.5` means perfectly centered — i.e., looking straight ahead.

**Vertical ratio:**
```
v_ratio = (iris_center_y - top_eyelid_y) / eye_height
```
- `0.0` means the iris is at the top of the eye.
- `1.0` means the iris is at the bottom.
- `0.5` means centered vertically.

The guard `if eye_w < 1e-6` handles the degenerate case where the eye is nearly closed or occluded, returning `0.5` (neutral) to avoid division by zero.

---

## `head_angles(matrix)`

```python
def head_angles(matrix):
    m = np.array(matrix.data).reshape(4, 4)
    r = m[:3, :3]
    pitch = math.degrees(math.asin(-r[2, 0]))
    yaw   = math.degrees(math.atan2(r[1, 0], r[0, 0]))
    return yaw, pitch
```

MediaPipe provides a **4×4 facial transformation matrix** per detected face. This matrix encodes the full 3D pose (position + rotation) of the face in camera space. It is stored as a flat list of 16 values in row-major order.

`m[:3, :3]` extracts the upper-left 3×3 **rotation submatrix** `R`, ignoring the translation column.

The rotation matrix encodes orientation using the ZYX Euler convention. The angles are recovered as:

- **Pitch** (nodding up/down): `-arcsin(R[2,0])`. The negative sign accounts for the axis orientation MediaPipe uses — positive pitch means the face is tilted downward.
- **Yaw** (turning left/right): `atan2(R[1,0], R[0,0])`. `atan2` is used instead of plain `atan` because it correctly handles all four quadrants and returns values in `[-180°, +180°]`.

Both are converted from radians to degrees for human-readable thresholding.

---

## `looking_at_camera(lm, transform_matrix)`

```python
def looking_at_camera(lm, transform_matrix):
    if len(lm) < 478:
        return None, (0.5, 0.5), (0.5, 0.5), (0.0, 0.0)
```

If the landmark list has fewer than 478 points the iris landmarks are missing (older model variant), so the function returns early with neutral values and `None` for the looking flag.

```python
    lx, ly = iris_center(lm, LEFT_IRIS)
    rx, ry = iris_center(lm, RIGHT_IRIS)

    lh, lv = gaze_ratio(lx, ly, LEFT_INNER, LEFT_OUTER, LEFT_TOP, LEFT_BOTTOM, lm)
    rh, rv = gaze_ratio(rx, ry, RIGHT_INNER, RIGHT_OUTER, RIGHT_TOP, RIGHT_BOTTOM, lm)

    avg_h = (lh + rh) / 2
    avg_v = (lv + rv) / 2
```

The iris center and gaze ratio are computed independently for each eye, then averaged. Averaging both eyes makes the estimate more robust — if one eye is partially obscured by a squint or shadow, the other eye still contributes.

```python
    yaw, pitch = head_angles(transform_matrix) if transform_matrix is not None else (0.0, 0.0)

    looking = (
        abs(avg_h - 0.5) < GAZE_H_THRESHOLD
        and abs(avg_v - 0.5) < GAZE_V_THRESHOLD
        and abs(yaw)   < YAW_THRESHOLD
        and abs(pitch) < PITCH_THRESHOLD
    )
```

All four conditions must be true at the same time:

| Condition | What it checks |
|---|---|
| `abs(avg_h - 0.5) < GAZE_H_THRESHOLD` | Iris is horizontally centered in the eye socket |
| `abs(avg_v - 0.5) < GAZE_V_THRESHOLD` | Iris is vertically centered in the eye socket |
| `abs(yaw) < YAW_THRESHOLD` | Head is not turned left or right by more than 20° |
| `abs(pitch) < PITCH_THRESHOLD` | Head is not tilted up or down by more than 20° |

The iris check alone is insufficient because a person can look sideways with their eyes while keeping their head straight — the iris would be off-center. Conversely, the head pose check alone is insufficient because someone can turn to face the camera but roll their eyes away. Only together do they reliably capture "actually paying attention to the camera".

---

## Main loop

```python
options = FaceLandmarkerOptions(
    ...
    output_facial_transformation_matrixes=True,
)
```

`output_facial_transformation_matrixes=True` tells MediaPipe to also compute and return the 4×4 pose matrix alongside the landmarks. Without this flag, `result.facial_transformation_matrixes` would be empty.

```python
cap = cv2.VideoCapture(0)
```

Opens the default webcam (index `0`). On systems with multiple cameras, `1`, `2`, etc. address the others.

```python
timestamp_ms = int(cap.get(cv2.CAP_PROP_POS_MSEC))
```

The `VIDEO` running mode requires a monotonically increasing timestamp with each frame so that the internal Kalman-filter tracker knows how much time has elapsed between frames. `CAP_PROP_POS_MSEC` reads it directly from OpenCV's capture clock.

```python
rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=rgb)
result = detector.detect_for_video(mp_image, timestamp_ms)
```

OpenCV stores frames as BGR by default; MediaPipe expects RGB. The frame is converted, wrapped in a `mp.Image` container, and fed to the detector. The result contains `face_landmarks` (list of 478 points per face) and `facial_transformation_matrixes` (one 4×4 matrix per face).

```python
matrices = result.facial_transformation_matrixes or []

for i, face in enumerate(result.face_landmarks or []):
    matrix = matrices[i] if i < len(matrices) else None
```

Iterates over detected faces (up to `num_faces=1` here). The matrix list is indexed in the same order as the landmarks list, so `matrices[i]` corresponds to `face_landmarks[i]`.

```python
eye_color = (0, 255, 0) if looking else (0, 0, 255)
```

Green (BGR `0,255,0`) when looking, red (BGR `0,0,255`) when not. This color is applied to both the eye outlines and the status label so the feedback is immediately visible at a glance.

```python
if cv2.waitKey(1) & 0xFF == ord("q"):
    break
```

`waitKey(1)` gives OpenCV 1 millisecond to process GUI events each frame. Without it the window would freeze. Pressing `q` sets the low byte to `113` (`ord('q')`), which breaks the loop and triggers cleanup.

---

## Why both signals are necessary

The iris ratio tells you **where the eyes are pointing within the face**. The head rotation tells you **where the face is pointing in the world**. A person looking at the camera satisfies both:

- Their face is oriented toward the camera (low yaw and pitch).
- Their irises are centered in their eye sockets (gaze ratios near 0.5).

Dropping either check leads to false positives: iris-only misses head turns; head-pose-only misses deliberate eye aversion while facing forward.
