CXX      ?= g++
PLUGIN   := hypr-mouse-gestures.so
SRC      := src/main.cpp src/gestures/SwipeGesture.cpp

PKG_CFLAGS := $(shell pkg-config --cflags hyprland)
CXXFLAGS   := -std=c++26 -fPIC -shared -O2 -Wall -Wextra -Wno-unused-parameter
CPPFLAGS   := $(PKG_CFLAGS)

.PHONY: all clean

all: $(PLUGIN)

$(PLUGIN): $(SRC) src/Gesture.hpp src/gestures/SwipeGesture.hpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ $(SRC)

clean:
	rm -f $(PLUGIN)
