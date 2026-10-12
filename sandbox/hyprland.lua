-- Nested Hyprland used to exercise hypr-mouse-gestures.
-- This file is not the live session config.

do
    local plugin = os.getenv("HOME") .. "/.config/hypr/plugins/hypr-mouse-gestures/hypr-mouse-gestures.so"
    local file = io.open(plugin, "r")
    if file then
        file:close()
        hl.plugin.load(plugin)
    end
end

hl.monitor({
    output   = "",
    mode     = "preferred",
    position = "auto",
    scale    = "1",
})

hl.config({
    general = {
        gaps_in     = 5,
        gaps_out    = 10,
        border_size = 2,
    },
    input = {
        follow_mouse = 1,
    },
    gestures = {
        workspace_swipe_distance = 300,
    },
})

hl.bind("SUPER + M", hl.dsp.exec_cmd("hyprctl dispatch 'hl.dsp.exit()'"))
hl.bind("SUPER + Return", hl.dsp.exec_cmd("kitty"))
