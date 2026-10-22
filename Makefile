CXX      ?= g++
PLUGIN   := hypr-mouse-gestures.so
SRC      := src/main.cpp src/gestures/SwipeGesture.cpp

PKG_CFLAGS := $(shell pkg-config --cflags hyprland)
CXXFLAGS   := -std=c++26 -fPIC -shared -O2 -Wall -Wextra -Wno-unused-parameter
# Hyprland's headers use partial designated initializers. Marking those
# include paths as system headers keeps that warning out of this build.
CPPFLAGS   := $(patsubst -I%,-isystem %,$(PKG_CFLAGS))

.PHONY: all clean

all: $(PLUGIN)

$(PLUGIN): $(SRC) src/Gesture.hpp src/gestures/SwipeGesture.hpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ $(SRC)

clean:
	rm -f $(PLUGIN)
