#include "SwipeGesture.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <linux/input-event-codes.h>

#include <hyprland/src/config/ConfigValue.hpp>
#include <hyprland/src/config/values/ConfigValues.hpp>
#include <hyprland/src/managers/SeatManager.hpp>
#include <hyprland/src/output/Monitor.hpp>
#include <hyprland/src/managers/SessionLockManager.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>

// m_delta on the unified swipe, and processMouseDownNormal, are private.
// The plugin needs both so a mouse drag can feed the same gesture the
// trackpad uses, and so a short press can still be delivered as a click.
#define private public
#include <hyprland/src/managers/input/InputManager.hpp>
#include <hyprland/src/managers/input/UnifiedWorkspaceSwipeGesture.hpp>
#undef private

namespace {
    // Held for the life of the plugin. CConfigValue lookups assert when the
    // name is missing, and the legacy addConfigValue API does not register
    // anything under a Lua config, so these are the V2 values themselves.
    SP<Config::Values::CIntValue> g_threshold;
    SP<Config::Values::CIntValue> g_button;

    int64_t dragThreshold() {
        if (!g_threshold)
            return 10;
        return std::max<int64_t>(0, g_threshold->value());
    }

    uint32_t swipeButton() {
        if (!g_button || g_button->value() <= 0)
            return BTN_MIDDLE;
        return static_cast<uint32_t>(g_button->value());
    }

    bool swipeInverted() {
        static auto INVERT = CConfigValue<Config::INTEGER>("gestures:workspace_swipe_invert");
        return INVERT.good() && *INVERT;
    }

    // The swipe gesture treats `workspace_swipe_distance` as a full transition.
    // Scale cursor pixels into that range so the workspace tracks the pointer:
    // dragging one monitor-width slides exactly one workspace.
    double cursorToSwipe(double cursorPixels) {
        static auto DISTANCE = CConfigValue<Config::INTEGER>("gestures:workspace_swipe_distance");
        static auto GAP      = CConfigValue<Config::INTEGER>("general:gaps_workspaces");

        const double distance = DISTANCE.good() ? static_cast<double>(std::max<int64_t>(1, *DISTANCE)) : 1.0;
        const double gap      = GAP.good() ? static_cast<double>(*GAP) : 0.0;

        const auto monitor = g_pUnifiedWorkspaceSwipe->m_monitor.lock();
        const double span  = monitor ? monitor->m_size.x + gap : 0.0;
        if (span <= 1.0)
            return cursorPixels;

        return cursorPixels * (distance / span);
    }

    // Hyprland commits a swipe at workspace_swipe_cancel_ratio (half a workspace).
    // The pointer tracking above stays 1:1. On release, a drag that has already
    // revealed a quarter of the next workspace is treated as committed.
    void finishSwipe() {
        if (!g_pUnifiedWorkspaceSwipe->isGestureInProgress())
            return;

        static auto DISTANCE = CConfigValue<Config::INTEGER>("gestures:workspace_swipe_distance");
        const double distance = DISTANCE.good() ? static_cast<double>(std::max<int64_t>(1, *DISTANCE)) : 1.0;

        constexpr double kCommitFraction = 0.25;
        auto&            delta           = g_pUnifiedWorkspaceSwipe->m_delta;
        if (std::abs(delta) >= distance * kCommitFraction)
            delta = std::copysign(distance, delta);

        g_pUnifiedWorkspaceSwipe->end();
    }

    bool compositorReady() {
        return g_pInputManager && g_pSessionLockManager && g_pUnifiedWorkspaceSwipe && g_pSeatManager;
    }
}

bool CSwipeGesture::registerConfig(void* handle) {
    g_threshold = Config::Values::makeConfigValue<Config::Values::CIntValue>(
        "plugin:hypr-mouse-gestures:drag_threshold", "Pointer travel, in pixels, before a middle-button drag starts a workspace swipe.", static_cast<Config::INTEGER>(10),
        Config::Values::SIntValueOptions{.min = 0, .max = 400});
    g_button = Config::Values::makeConfigValue<Config::Values::CIntValue>(
        "plugin:hypr-mouse-gestures:swipe_button", "Mouse button code that arms the workspace swipe. 274 is the middle button.", static_cast<Config::INTEGER>(BTN_MIDDLE),
        Config::Values::SIntValueOptions{.min = 1, .max = 1000});

    const bool thresholdOk = HyprlandAPI::addConfigValueV2(handle, g_threshold);
    const bool buttonOk    = HyprlandAPI::addConfigValueV2(handle, g_button);
    return thresholdOk && buttonOk;
}

void CSwipeGesture::beginSwipe() {
    if (!compositorReady() || g_pSessionLockManager->isSessionLocked() || g_pUnifiedWorkspaceSwipe->isGestureInProgress()) {
        m_phase = Phase::Consumed;
        return;
    }

    g_pUnifiedWorkspaceSwipe->begin();
    if (!g_pUnifiedWorkspaceSwipe->isGestureInProgress()) {
        m_phase = Phase::Consumed;
        return;
    }

    m_phase = Phase::Active;
    m_last  = m_origin;
}

void CSwipeGesture::replayClick(const IPointer::SButtonEvent& release) {
    IPointer::SButtonEvent press = release;
    press.state                  = WL_POINTER_BUTTON_STATE_PRESSED;
    press.timeMs                 = m_pressTime;
    press.mouse                  = true;

    g_pInputManager->processMouseDownNormal(press, {});
    g_pInputManager->processMouseDownNormal(release, {});
    g_pSeatManager->sendPointerFrame();
}

bool CSwipeGesture::onButton(const IPointer::SButtonEvent& event, Event::SCallbackInfo& info) {
    // Virtual pointers use the same button codes but leave the mouse flag unset.
    if (!compositorReady() || event.button != swipeButton())
        return false;

    if (event.state == WL_POINTER_BUTTON_STATE_PRESSED) {
        if (m_phase != Phase::Idle || g_pUnifiedWorkspaceSwipe->isGestureInProgress() || g_pSessionLockManager->isSessionLocked())
            return false;

        m_origin    = g_pInputManager->getMouseCoordsInternal();
        m_last      = m_origin;
        m_button    = event.button;
        m_pressTime = event.timeMs;
        m_phase     = Phase::Armed;
        // Hold the click until we know whether this is a drag.
        info.cancelled = true;
        return true;
    }

    if (m_phase == Phase::Idle || event.button != m_button)
        return false;

    info.cancelled = true;

    if (m_phase == Phase::Active) {
        finishSwipe();
    } else if (m_phase == Phase::Armed) {
        const auto traveled = g_pInputManager->getMouseCoordsInternal() - m_origin;
        const auto moved    = std::hypot(traveled.x, traveled.y);
        if (moved <= static_cast<double>(dragThreshold()))
            replayClick(event);
    }

    m_phase = Phase::Idle;
    return true;
}

void CSwipeGesture::onMotion(Event::SCallbackInfo& info) {
    if (!compositorReady() || m_phase == Phase::Idle || m_phase == Phase::Consumed)
        return;

    const auto pos = g_pInputManager->getMouseCoordsInternal();

    if (m_phase == Phase::Armed) {
        const auto traveled = pos - m_origin;
        if (std::abs(traveled.x) < static_cast<double>(dragThreshold()))
            return;
        if (std::abs(traveled.x) < std::abs(traveled.y))
            return;

        beginSwipe();
        if (m_phase != Phase::Active)
            return;
    }

    if (!g_pUnifiedWorkspaceSwipe->isGestureInProgress()) {
        m_phase = Phase::Consumed;
        return;
    }

    const double step = pos.x - m_last.x;
    m_last            = pos;
    const double signedStep = swipeInverted() ? -step : step;
    g_pUnifiedWorkspaceSwipe->update(g_pUnifiedWorkspaceSwipe->m_delta + cursorToSwipe(signedStep));

    // Keep follow_mouse from focusing whatever is under the cursor mid-swipe.
    info.cancelled = true;

    if (!g_pUnifiedWorkspaceSwipe->isGestureInProgress())
        m_phase = Phase::Consumed;
}

void CSwipeGesture::reset() {
    if (compositorReady() && m_phase == Phase::Active)
        finishSwipe();
    m_phase = Phase::Idle;
}
