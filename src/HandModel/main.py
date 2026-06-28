import cv2
import mediapipe as mp
from mediapipe.tasks import python, BaseOptions
from mediapipe.tasks.python import vision
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


options = HandLandmarkerOptions(
    base_options = BaseOptions,
    RunningMode =  'LIVE_STREAM',
    num_hands = 1,
    min_hand_detection_confidence = 0.5,
    min_hand_presence_confidence = 0.5,
    min_tracking_confidence = 0.5,
    result_callback = callback()
) 

