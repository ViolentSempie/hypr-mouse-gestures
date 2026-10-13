# hypr-mouse-gestures

This plugin adds mouse gestures to Hyprland. The first gesture is a middle-button drag that switches workspaces.

## Requirements

- Hyprland 0.56.2
- A mouse

The plugin loads only into a Hyprland build from the same commit it was compiled against.

## Install

```sh
hyprpm add https://github.com/ViolentSempie/hypr-mouse-gestures
hyprpm enable hypr-mouse-gestures
hyprpm reload
```

After a Hyprland update, `hyprpm update` rebuilds the plugin when that release has a pin in `hyprpm.toml`.

## Configure

Holding the middle button and dragging horizontally follows the cursor: one monitor width is one workspace. Releasing after a quarter of the next workspace is visible switches to it. Reversing past the start reveals the workspace on the other side without letting go. A short press that stays inside the drag threshold is a normal middle click.

```lua
hl.config({
    plugin = {
        hypr_mouse_gestures = {
            drag_threshold = 10, -- pixels before a drag becomes a swipe (0–400)
            swipe_button = 274,  -- middle button
        },
    },
})
```

In hyprlang the same options are `plugin:hypr-mouse-gestures:drag_threshold` and `plugin:hypr-mouse-gestures:swipe_button`.

The drag also follows `gestures:workspace_swipe_invert`, `gestures:workspace_swipe_distance`, `general:gaps_workspaces`, and `gestures:workspace_swipe_min_speed_to_force`.

## Develop

Building needs:

- Hyprland headers that match the compositor the plugin will load into (`hyprland-devel` on Fedora). The running compositor and the headers must report the same API hash.
- `pkg-config` with a `hyprland` module.
- `g++` with C++26, the same major compiler Hyprland itself was built with.
- Linux input headers (`linux/input-event-codes.h`).

```sh
make
make clean
```

`make` writes `hypr-mouse-gestures.so` in the repository root. The `.so` is not linked against the Hyprland libraries. Those symbols come from the compositor at load time.

`sandbox/hyprland.lua` is a nested Hyprland config for trying a build. It loads `~/.config/hypr/plugins/hypr-mouse-gestures/hypr-mouse-gestures.so`. Build first, then start it without letting it take the active seat:

```sh
LIBSEAT_BACKEND=seatd HYPRLAND_NO_SD_VARS=1 HYPRLAND_NO_SD_NOTIFY=1 \
    Hyprland -c sandbox/hyprland.lua
```

`LIBSEAT_BACKEND=seatd` fails immediately when seatd is not running, which keeps the DRM backend from opening the real seat. `HYPRLAND_NO_SD_VARS` and `HYPRLAND_NO_SD_NOTIFY` keep the nested instance from rewriting the user systemd environment.

Inside the sandbox, Super+Return opens kitty and Super+M exits. Talk to that instance with `hyprctl -i SIGNATURE`.

## AI Notice

This repository was written with AI assistance and reviewed by me.
