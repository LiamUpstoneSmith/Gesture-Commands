CXX      := g++
CXXFLAGS := -std=c++17 -Wall -O2
LDLIBS   := -lzmq

TARGETS  := server client

all: $(TARGETS)

server: server.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDLIBS)

client: client.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDLIBS)

clean:
	rm -f $(TARGETS)

.PHONY: all clean