#pragma once

#include "../Gesture.hpp"

// Middle-button drag drives the same workspace swipe as a horizontal
// three-finger trackpad gesture. A press that never passes the drag
// threshold is forwarded as a normal middle click.
class CSwipeGesture : public IMouseGesture {
  public:
    // Registers the gesture's config values. Safe to use the gesture either way:
    // a failed registration keeps the built-in defaults.
    static bool registerConfig(void* handle);

    bool onButton(const IPointer::SButtonEvent& event, Event::SCallbackInfo& info) override;
    void onMotion(Event::SCallbackInfo& info) override;
    void reset() override;

  private:
    enum class Phase {
        Idle,
        Armed,
        Active,
        // The press was taken, but the swipe ended on its own. Swallow the
        // release so it is not delivered as a click.
        Consumed,
    };

    void beginSwipe();
    void replayClick(const IPointer::SButtonEvent& release);

    Phase                  m_phase     = Phase::Idle;
    Vector2D               m_origin    = {};
    Vector2D               m_last      = {};
    uint32_t               m_button    = 0;
    uint32_t               m_pressTime = 0;
};
