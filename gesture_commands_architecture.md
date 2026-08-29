# Gesture-Commands: Architecture Plan

## Goal

Desktop application for hands-free computer interaction via real-time webcam hand gesture
recognition. Detected gestures map to system commands, hotkeys, and workspace switching.

C++ GUI (Dear ImGui) for C++ experience. Python backend reusing existing MediaPipe Tasks work.
Primary development on Linux (Fedora).

---

## Responsibility split

**Python owns:** capture, MediaPipe landmark detection, drawing the landmark overlay with
OpenCV, JPEG encoding, gesture matching, command execution.

**C++ owns:** displaying the video frame, gesture toggles, the Edit Gestures scene.

This is non-traditional (the "slower" language does the processing) and correct here. The heavy
compute is neural network inference, already compiled C++ inside MediaPipe; Python only
orchestrates. And the purpose of the C++ layer is GUI learning, so optimising it away would
defeat the point.

---

## Key decision: Python draws the overlay

Python draws landmarks onto the frame and sends a finished picture. C++ never sees landmark
coordinates.

**Why:** it eliminates a synchronisation class of bug. If C++ received frames and coordinates
separately, it would have to guarantee landmark set N is drawn on frame N; any drift makes the
dots visibly lag the hand. Bundling them makes the problem impossible rather than merely
unlikely.

(Note: the reason is *synchronisation*, not data corruption. Transports here are reliable and
ordered; bytes do not rot in transit.)

**Cost accepted:** per-frame JPEG encode in Python, decode plus GPU texture upload in C++.
Measure the encode once running. First tuning knobs are JPEG quality and resolution.

---

## Sizing

- Raw 640x480x3 at 30fps: ~27.6 MB/s. Too much.
- JPEG compressed: roughly 3 to 9 MB/s. Workable.

---

## Decoupling

The two sides run at whatever rate each achieves. Neither blocks the other.

Frames are **latest-only**. For a live feed, showing the newest frame is the correct behaviour;
a backlog of stale frames has no value.

Do not rate-match the two sides. Their speeds depend on independent physical systems (camera
driver, inference time, encode vs GPU, compositor, monitor refresh). Coupling them means a slow
GUI silently degrades gesture recognition, and a stalled camera freezes the UI.

---

## Atomicity

A reader must never see a half-written frame.

- Raw shared memory would require **double buffering**: write to the back slot, read the front,
  and only the flip needs to be atomic.
- ZeroMQ preserves message boundaries, so a whole frame arrives or nothing does. Atomicity comes
  free. `CONFLATE` supplies the latest-only behaviour.

Do not hand-build double buffering. Recognising that the library already solved it is the point.

---

## Transport: ZeroMQ, two channels

### Channel A: frames

- Pattern: PUB/SUB
- Python publishes, C++ subscribes
- `CONFLATE` set: keep only the newest message
- Slow-joiner message loss is harmless here

### Channel B: control

- Pattern: REQ/REP
- C++ requests (toggle, edit, add, delete), Python replies
- Lockstep is a feature: guaranteed delivery and acknowledgement for messages that must not be
  dropped
- C++ initiates all config changes, so the requester-speaks-first constraint fits naturally

### Bind and connect

**Python binds both sockets. C++ connects both.**

Rule: the stable, long-lived, server-like side binds; the transient side connects.

Consequences: the GUI can be restarted freely without disturbing the backend, and ZeroMQ
reconnects automatically. A connecting socket waits patiently, so start order does not matter.

---

## Frame payload

- JPEG bytes only. Dimensions are already in the JPEG header and the decoder reports them, so
  do not send a second copy that can disagree with the data.
- No timestamp needed: the GUI displays whatever arrives, and skipped frames are correct
  behaviour.
- A frame number is optional, purely for diagnostics ("am I dropping frames, and how many?").

## Gesture status

Rides with the frame as **current state**, not as a one-off event.

Consequence of `CONFLATE`: if a gesture fires on a frame that gets discarded, that message is
gone. Acceptable for a live "currently active" indicator, because the next frame carries the
state again. It would be wrong for counting firings or keeping a history. If reliable events
are ever needed, that is when a third channel earns its place.

---

## Storage

A single `gestures.json` holding a list of gesture objects: name, command, landmark template.

- Read once at startup into memory
- Rewritten only when the set changes
- Disk is never in the live loop, so read speed is irrelevant

One structured file beats one text file per gesture: no hand-written parser to maintain, no
directory scanning, no consistency problem across N files.

---

## Cross-platform notes

ZeroMQ, MediaPipe, and OpenCV are all cross-platform, so the architecture survives a port.

The real portability cost is in **command execution**: sending hotkeys and switching workspaces
shares nothing between Linux and Windows. C++ build tooling also differs sharply.

Develop on Linux. Ubuntu and POP!_OS are both Debian-based with effectively identical tooling,
so moving between them is nearly free.

Isolate command execution behind a single small Python module exposing something like
`execute_command(name)`, so a future port touches one file.

---

## Open items

1. **Landmark normalisation.** Raw coordinates depend on where the hand sits in frame and how
   far from the camera. A gesture must be recognisable regardless of position, scale, and
   rotation. This is the real ML content of the project. Parked, but coming.
2. **ZeroMQ's C++ dependency.** Linking an external library from C++ for the first time is not
   a trivial afternoon. Budget for it.
3. **Dear ImGui.** Immediate mode: redraws every frame in a loop rather than reacting to
   events, which suits "check for a new message once per frame". Next topic: turning JPEG bytes
   into an OpenGL texture for `ImGui::Image`.
