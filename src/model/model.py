import time
import cv2
import json
import mediapipe as mp
import os
from subprocess import call
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

def read_gesture_db():
    '''
    Read JSON file and save as global variable
    '''
    global data
    with open("commands.json", "r") as json_file: 
        data = json.load(json_file)
    
read_gesture_db()

def edit_db(gesture_to_change: str, new_command: str):
    # Change command 
    if data['preset-gestures']['category-name'] == gesture_to_change:
        data['preset-gestures']['command-to-execute'] = new_command

    # Save new command in json file. (For long-term storage)
    with open("commands.json", "w") as json_file:
        json.dump(json.dumps(data, indent=3), json_file)

    read_gesture_db() # Re-decalre global db variable

def callback(result: GestureRecognizerResult, output_image: mp.Image, timestamp_ms: int):
    global shared_state
    global shared_state_timestamp

    shared_state = result
    shared_state_timestamp = timestamp_ms


    # Print name of gesture detected
    for hand in result.gestures:
        category_detected = hand[0].category_name
        if category_detected != "None":
            # If debug = true
            # print(f"\nCategory Detected: {category_detected}")
            execture_command(category_detected)

def execture_command(category_detected):
    for gesture in data['preset-gestures']:
        if gesture['category-name'] == category_detected and gesture['command-to-execute'] == "None":

            # Execute command from JSON file to shell
            call(f'{gesture['command-to-execute']}', shell=True)
            

def main(debug=False):
    options = GestureRecognizerOption(
        base_options=BaseOptions(model_asset_path='gesture_recognizer.task'),
        running_mode= VisionRunningMode.LIVE_STREAM,
        num_hands=2,
        min_hand_detection_confidence=0.5,
        min_hand_presence_confidence=0.5,
        min_tracking_confidence=0.5,
        result_callback=callback
    )

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

                if debug:
                    cv2.imshow("Video", frame)  # Show frame

            if cv2.waitKey(1) == ord("q"):
                break

    cam.release()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    main(debug=True)