import cv2
import mediapipe as mp
from mediapipe.tasks.python import vision, BaseOptions
from mediapipe.tasks.python.vision import FaceLandmarker, FaceLandmarkerOptions, RunningMode

MODEL_PATH = "face_landmarker.task"

# Left/right eye contour indices (MediaPipe 478-landmark mesh)
LEFT_EYE  = [33, 160, 158, 133, 153, 144]
RIGHT_EYE = [362, 385, 387, 263, 373, 380]

def draw_eye(frame, landmarks, indices, color):
    h, w = frame.shape[:2]
    pts = [(int(landmarks[i].x * w), int(landmarks[i].y * h)) for i in indices]
    for pt in pts:
        cv2.circle(frame, pt, 2, color, -1)
    for j in range(len(pts)):
        cv2.line(frame, pts[j], pts[(j + 1) % len(pts)], color, 1)

def main():
    options = FaceLandmarkerOptions(
        base_options=BaseOptions(model_asset_path=MODEL_PATH),
        running_mode=RunningMode.VIDEO,
        num_faces=2,
        min_face_detection_confidence=0.5,
        min_face_presence_confidence=0.5,
        min_tracking_confidence=0.5,
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

            for face in (result.face_landmarks or []):
                draw_eye(frame, face, LEFT_EYE,  (0, 255, 0))
                draw_eye(frame, face, RIGHT_EYE, (0, 255, 255))

            cv2.imshow("Eye Detection", frame)
            if cv2.waitKey(1) & 0xFF == ord("q"):
                break

    cap.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
