import time
import cv2
import mediapipe as mp
from mediapipe.tasks import python
from mediapipe.tasks.python import vision, BaseOptions
from mediapipe.tasks.python.vision import HandLandmarker, HandLandmarkerOptions, RunningMode
from mediapipe.tasks.python.components.containers.landmark import NormalizedLandmark


shared_state = None
shared_state_timestamp = None

def callback(result, output_image, timestamp_ms):
    '''
    Update global vairables if hand is present
    '''

    global shared_state
    global shared_state_timestamp

    shared_state = result
    shared_state_timestamp = timestamp_ms

    print(f"Num Hands Detected: {len(result.hand_landmarks)}")


base_options = BaseOptions(model_asset_path='hand_landmarker.task')

options = HandLandmarkerOptions(
    base_options = base_options,
    running_mode =  RunningMode.LIVE_STREAM,
    num_hands = 1,
    min_hand_detection_confidence = 0.5,
    min_hand_presence_confidence = 0.5,
    min_tracking_confidence = 0.5,
    result_callback = callback
) 

with HandLandmarker.create_from_options(options) as landmarker:

    print("Landmarker ready")

    cam = cv2.VideoCapture(0)

    while True:
        check, frame = cam.read() # Read data from live stream source (webcam)

        if check: # Check the camera was read properly
            image_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB) # Convert OpenCVs' BGR to RGB
            mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=image_rgb) # Wrap image in mp.Image

            frame_timestamp_ms = int(time.time() * 1000) # Get Timestamp of frame

            landmarker.detect_async(mp_image, frame_timestamp_ms)

            if shared_state is not None and len(shared_state.hand_landmarks) > 0:

                h, w, _ = frame.shape

                for landmark in shared_state.hand_landmarks[0]:
                    
                    cx = int(landmark.x * w)
                    cy = int(landmark.y * h)
                    cv2.circle(frame, (cx, cy), 5, (0, 255, 0), -1)

            cv2.imshow('Video', frame) # Show frame

        if cv2.waitKey(1) == ord('q'):
            break

cam.release()
cv2.destroyAllWindows()