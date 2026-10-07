import time
import cv2
import json
import mediapipe as mp
import os
import zmq
import threading
from subprocess import Popen
from mediapipe.tasks import python
from mediapipe.tasks.python import vision
from mediapipe.tasks.python.vision.drawing_utils import draw_landmarks

BaseOptions = mp.tasks.BaseOptions
GestureRecognizer = mp.tasks.vision.GestureRecognizer
GestureRecognizerOption = mp.tasks.vision.GestureRecognizerOptions
GestureRecognizerResult = mp.tasks.vision.GestureRecognizerResult
VisionRunningMode = mp.tasks.vision.RunningMode

shared_state = None
shared_state_timestamp = None

COOLDOWN_SECONDS = 3.0

last_execution_time = 0.0   # monotonic time of the last command that ran
last_category = "None"      # gesture seen on the previous callback
command_lock = threading.Lock()

def read_gesture_db():
    '''
    Read JSON file and save as global variable
    '''
    global data
    with open("src/model/commands.json", "r") as json_file: 
        data = json.load(json_file)
    
read_gesture_db()

def edit_db(gesture_to_change: str, new_command: str):
    for gesture in data['preset-gestures']:
        if gesture['category-name'] == gesture_to_change:
            gesture['command-to-execute'] = new_command
            break
    else:
        # Loop finished without a break: no gesture matched
        raise ValueError(f"Unknown gesture: {gesture_to_change}")

    # Save new command in json file (long-term storage)
    with open("src/model/commands.json", "w") as json_file:
        json.dump(data, json_file, indent=3)



def callback(result, output_image: mp.Image, timestamp_ms: int):
    global shared_state, shared_state_timestamp, last_category

    shared_state = result
    shared_state_timestamp = timestamp_ms

    # Pick the first hand that has a real gesture
    category_detected = "None"
    for hand in result.gestures:
        if hand[0].category_name != "None":
            category_detected = hand[0].category_name
            break

    # Edge trigger: only act when the gesture changes
    if category_detected != "None" and category_detected != last_category:
        execute_command(category_detected)

    last_category = category_detected

def execute_command(category_detected):
    global last_execution_time

    with command_lock:
        now = time.monotonic()
        if now - last_execution_time < COOLDOWN_SECONDS:
            return  # still cooling down

        for gesture in data['preset-gestures']:
            if (gesture['category-name'] == category_detected
                    and gesture['command-to-execute'] != "None"):
                last_execution_time = now
                # Popen does not block, so the callback thread stays free
                Popen(gesture['command-to-execute'], shell=True,
                      start_new_session=True)
                break

def main():
    options = GestureRecognizerOption(
        base_options=BaseOptions(model_asset_path='src/model/gesture_recognizer.task'),
        running_mode= VisionRunningMode.LIVE_STREAM,
        num_hands=2,
        min_hand_detection_confidence=0.5,
        min_hand_presence_confidence=0.5,
        min_tracking_confidence=0.5,
        result_callback=callback
    )

    # ZeroMQ PUBLISH socket
    zmq_context = zmq.Context()
    video_socket = zmq_context.socket(zmq.PUB)
    video_socket.setsockopt(zmq.SNDHWM, 1) # never queue more than 1 frame
    video_socket.bind("tcp://localhost:5556")

    # ZeroMQ REP socket
    gesture_socket = zmq_context.socket(zmq.REP)
    gesture_socket.connect("tcp://localhost:5555")

    with GestureRecognizer.create_from_options(options) as recognizer:

        print("Landmarker ready")

        cam = cv2.VideoCapture(0)

        while True:
            check, frame = cam.read()

            if check:  # Check the camera was read properly
                image_rgb = cv2.cvtColor(
                    frame, cv2.COLOR_BGR2RGB
                )  # Convert OpenCVs' BGR to RGB
                mp_image = mp.Image(
                    image_format=mp.ImageFormat.SRGB, data=image_rgb
                )  # Wrap image in mp.Image

                frame_timestamp_ms = int(time.time() * 1000)  # Get Timestamp of frame

                recognizer.recognize_async(mp_image, frame_timestamp_ms)

                if shared_state is not None and len(shared_state.hand_landmarks) > 0:

                    h, w, _ = frame.shape
                    hand_connections = vision.HandLandmarksConnections.HAND_CONNECTIONS


                    # Draw connection lines
                    draw_landmarks(
                        image=frame,
                        landmark_list=shared_state.hand_landmarks[0],
                        connections=hand_connections,
                        is_drawing_landmarks=False,
                    )

                    # Draw landmark dots
                    for landmark in shared_state.hand_landmarks[0]:
                        cx = int(landmark.x * w)
                        cy = int(landmark.y * h)
                        cv2.circle(frame, (cx, cy), 5, (0, 255, 0), -1)

                    # If 2 hands are present
                    if len(shared_state.hand_landmarks) > 1:
                        draw_landmarks(
                            image=frame,
                            landmark_list=shared_state.hand_landmarks[1],
                            connections=hand_connections,
                            is_drawing_landmarks=False,
                        )

                        # Draw landmark dots
                        for landmark in shared_state.hand_landmarks[1]:
                            cx = int(landmark.x * w)
                            cy = int(landmark.y * h)
                            cv2.circle(frame, (cx, cy), 5, (0, 255, 0), -1)

                success, jpeg_buffer = cv2.imencode(".jpg", frame, [cv2.IMWRITE_JPEG_QUALITY, 80])
                if success:
                    video_socket.send(jpeg_buffer.tobytes())

                if gesture_socket.poll(0):
                    try:
                        message = gesture_socket.recv_string()
                        gesture, command = message.split("$$", 1)
                        edit_db(gesture, command)
                        reply = "UPDATED"
                    except Exception as e:
                        reply = f"ERR:{e}"
                    gesture_socket.send_string(reply)

            if cv2.waitKey(1) == ord("q"):
                break

    gesture_socket.close()
    video_socket.close()
    zmq_context.term()
    cam.release()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()
