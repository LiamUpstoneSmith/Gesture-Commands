import time
import zmq

context = zmq.Context()
socket = context.socket(zmq.REP)
socket.bind("tcp://localhost:5555")

try:
    while True:
        message = socket.recv()
        print(f"Received request: {message}")

        time.sleep(1)# This is the time it does "work"

        socket.send(b"Twerk on that thang!")
except KeyboardInterrupt: # Press Ctrl + C to stop
    pass
    