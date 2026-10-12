#include "Gesture.hpp"
#include "gestures/SwipeGesture.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <hyprland/src/plugins/PluginAPI.hpp>

inline HANDLE PHANDLE = nullptr;

static std::vector<std::unique_ptr<IMouseGesture>> g_gestures;
static Hyprutils::Signal::CHyprSignalListener      g_buttonListener;
static Hyprutils::Signal::CHyprSignalListener      g_motionListener;

APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

    const std::string COMPOSITOR_HASH = __hyprland_api_get_hash();
    const std::string CLIENT_HASH     = __hyprland_api_get_client_hash();

    if (COMPOSITOR_HASH != CLIENT_HASH) {
        HyprlandAPI::addNotification(PHANDLE, "[hypr-mouse-gestures] Mismatched headers. Can't proceed.", {1.0F, 0.2F, 0.2F, 1.0F}, 5000);
        throw std::runtime_error("[hypr-mouse-gestures] version mismatch");
    }

    if (!CSwipeGesture::registerConfig(handle))
        HyprlandAPI::addNotification(PHANDLE, "[hypr-mouse-gestures] Could not register config. Using defaults.", {1.0F, 0.6F, 0.2F, 1.0F}, 5000);

    g_gestures.push_back(std::make_unique<CSwipeGesture>());

    g_buttonListener = Event::bus()->m_events.input.mouse.button.listen([](IPointer::SButtonEvent event, Event::SCallbackInfo& info) {
        for (const auto& gesture : g_gestures) {
            if (gesture->onButton(event, info))
                return;
        }
    });

    g_motionListener = Event::bus()->m_events.input.mouse.move.listen([](Vector2D, Event::SCallbackInfo& info) {
        for (const auto& gesture : g_gestures)
            gesture->onMotion(info);
    });

    return {"hypr-mouse-gestures", "Mouse gestures. Middle-button drag swipes workspaces.", "semfa", "0.1.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    for (const auto& gesture : g_gestures)
        gesture->reset();

    g_gestures.clear();
    g_buttonListener.reset();
    g_motionListener.reset();
}
