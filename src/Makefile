#
# Cross Platform Makefile
# Compatible with MSYS2/MINGW and Ubuntu 14.04.1
#
# You will need GLFW (http://www.glfw.org):
# Linux:
#   apt-get install libglfw-dev
# MSYS2:
#   pacman -S --noconfirm --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-glfw
#
# You will also need ZeroMQ + its C++ header-only bindings (cppzmq):
# Fedora:
#   sudo dnf install zeromq-devel cppzmq-devel
# Ubuntu/Pop!_OS:
#   sudo apt install libzmq3-dev
#   (cppzmq's zmq.hpp is header-only; if your distro has no cppzmq package,
#    drop zmq.hpp from https://github.com/zeromq/cppzmq into a location on
#    your include path, e.g. gui/)
#

#CXX = g++
#CXX = clang++

EXE = demo_app
IMGUI_DIR = gui
SOURCES = $(IMGUI_DIR)/main.cpp
SOURCES += $(IMGUI_DIR)/imgui/imgui.cpp $(IMGUI_DIR)/imgui/imgui_demo.cpp $(IMGUI_DIR)/imgui/imgui_draw.cpp $(IMGUI_DIR)/imgui/imgui_tables.cpp $(IMGUI_DIR)/imgui/imgui_widgets.cpp
SOURCES += $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp

# Build directory - keeps .o files out of the source tree
OBJ_DIR = build
OBJS = $(addprefix $(OBJ_DIR)/, $(addsuffix .o, $(basename $(notdir $(SOURCES)))))

UNAME_S := $(shell uname -s)
LINUX_GL_LIBS = -lGL

CXXFLAGS = -std=c++17 -I$(IMGUI_DIR)/imgui -I$(IMGUI_DIR)/backends
CXXFLAGS += -g -Wall -Wformat
LIBS =

PROJECT_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
CXXFLAGS += -DPROJECT_ROOT=\"$(PROJECT_ROOT)\"

##---------------------------------------------------------------------
## OPENGL ES
##---------------------------------------------------------------------

## This assumes a GL ES library available in the system, e.g. libGLESv2.so
# CXXFLAGS += -DIMGUI_IMPL_OPENGL_ES2
# LINUX_GL_LIBS = -lGLESv2

##---------------------------------------------------------------------
## BUILD FLAGS PER PLATFORM
##---------------------------------------------------------------------

ifeq ($(UNAME_S), Linux)
	ECHO_MESSAGE = "Linux"
	LIBS += $(LINUX_GL_LIBS) `pkg-config --static --libs glfw3` `pkg-config --libs libzmq` -pthread
	CXXFLAGS += `pkg-config --cflags glfw3` `pkg-config --cflags libzmq` -pthread
	CFLAGS = $(CXXFLAGS)
endif

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------

# Make sure "make" with no arguments builds the app, not just the build dir.
.DEFAULT_GOAL := all

# Tell make where to actually find the .cpp files listed in SOURCES,
# since they live in subdirectories rather than next to the Makefile.
vpath %.cpp $(sort $(dir $(SOURCES)))

all: $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: %.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -rf $(OBJ_DIR) $(EXE)

.PHONY: all clean
