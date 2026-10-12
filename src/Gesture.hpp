#pragma once

#include <hyprland/src/devices/IPointer.hpp>
#include <hyprland/src/event/EventBus.hpp>

// One mouse gesture. Button and motion events are offered to every registered
// gesture; a gesture claims a button by handling it.
class IMouseGesture {
  public:
    virtual ~IMouseGesture() = default;

    // Return true when this gesture owns the button event.
    virtual bool onButton(const IPointer::SButtonEvent& event, Event::SCallbackInfo& info) = 0;

    virtual void onMotion(Event::SCallbackInfo& info) = 0;

    // Drop any in-progress gesture. Used when the plugin unloads.
    virtual void reset() = 0;
};
