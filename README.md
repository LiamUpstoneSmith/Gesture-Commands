# Gesture-Commands

## Install


Install libzmq (the core C library) and its dev headers:


sudo dnf install zeromq zeromq-devel

Install cppzmq (the header-only C++ binding):


sudo dnf install cppzmq-devel



Verify the install:


pkg-config --modversion libzmq
ls /usr/include/zmq.hpp

You should also make sure gcc-c++ (or g++) is installed for the Makefile to work:


sudo dnf install gcc-c++

### Install GLFW
sudo dnf install glfw
sudo dnf install glfw-devel



