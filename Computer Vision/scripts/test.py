import cv2
import time
from ultralytics import YOLO

model = YOLO("Computer Vision/weights/best.pt")
print(f"[INFO] Loaded classes: {model.names}")

cap = cv2.VideoCapture(0)

cv2.namedWindow("TACTICAL SAR HUD", cv2.WND_PROP_FULLSCREEN)
cv2.setWindowProperty("TACTICAL SAR HUD", cv2.WND_PROP_FULLSCREEN, cv2.WINDOW_FULLSCREEN)

prev_time = 0

while True:
    ret, frame = cap.read()
    if not ret:
        continue

    # 1. Lower threshold to catch out-of-domain angles
    results = model(frame, conf=0.15)
    annotated = results[0].plot()
    h, w, _ = annotated.shape

    # 2. Print raw detections to the terminal
    boxes = results[0].boxes
    if len(boxes) > 0:
        for b in boxes:
            score = float(b.conf[0])
            print(f"[DETECTED] survivor | Confidence: {score*100:.1f}%")

    # FPS computation
    curr_time = time.time()
    fps = 1.0 / (curr_time - prev_time) if (curr_time - prev_time) > 0 else 0
    prev_time = curr_time

    # Top Status Bar
    cv2.rectangle(annotated, (0, 0), (w, 40), (20, 20, 20), -1)
    cv2.putText(annotated, "MISSION: SAR-SEARCH-LOCATE", (15, 26), 
                cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 2)
    cv2.putText(annotated, f"FPS: {fps:.1f} | LINK: 100% | BUS: 11.6V", (w - 340, 26), 
                cv2.FONT_HERSHEY_SIMPLEX, 0.55, (0, 255, 0), 2)

    # Crosshair
    cv2.drawMarker(annotated, (w // 2, h // 2), (0, 255, 255), cv2.MARKER_CROSS, 20, 1)

    # Bottom Triage Banner
    if len(boxes) > 0:
        top_conf = float(boxes[0].conf[0])
        cv2.rectangle(annotated, (0, h - 35), (w, h), (0, 0, 180), -1)
        cv2.putText(annotated, f"! CRITICAL ALERT: {len(boxes)} SURVIVOR(S) IDENTIFIED ({top_conf*100:.0f}%) ! COORDS LOGGED", 
                    (20, h - 12), cv2.FONT_HERSHEY_SIMPLEX, 0.55, (255, 255, 255), 2)

    cv2.imshow("TACTICAL SAR HUD", annotated)
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()