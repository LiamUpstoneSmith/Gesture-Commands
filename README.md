# Gesture-Commands

Install libzmq (the core C library) and its dev headers:

bash

sudo dnf install zeromq zeromq-devel

Install cppzmq (the header-only C++ binding):

bash

sudo dnf install cppzmq-devel



Verify the install:

bash

pkg-config --modversion libzmq
ls /usr/include/zmq.hpp

You should also make sure gcc-c++ (or g++) is installed for the Makefile to work:

bash

sudo dnf install gcc-c++