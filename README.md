# hypr-mouse-gestures

Mouse gestures for Hyprland. Holding the middle button and dragging horizontally swipes workspaces the same way a three-finger trackpad swipe does. The workspace follows the cursor: one monitor width is one workspace. Releasing after a quarter of the next workspace is visible switches to it. A short press that never crosses the drag threshold is still a normal middle click.

## Requirements

### Build

- Hyprland headers that match the compositor you will load the plugin into. The running compositor and the headers must report the same API hash, or the plugin refuses to load.
- `pkg-config` with a `hyprland` module (`hyprland-devel` on Fedora).
- `g++` with C++26, the same major compiler Hyprland itself was built with.
- Linux input headers (`linux/input-event-codes.h`).

The `.so` is not linked against the Hyprland libraries. Those symbols come from the compositor at load time.

This tree is pinned to Hyprland 0.56.2 (`efb50993780079460b0cbed1363e2166a2de1d9f`). Other releases need their own pin in `hyprpm.toml`.

### Runtime

- A Hyprland session built from the same commit the plugin was compiled against.
- A mouse. The default gesture button is the middle button, code `274` (`BTN_MIDDLE`).

`hyprpm` is only needed for the install path below. Loading the `.so` yourself does not need it.

### Repository

`hyprpm.toml` is the install manifest: repository name, authors, build command (`make all`), output file (`hypr-mouse-gestures.so`), and one commit pin per supported Hyprland release. A pin is `[hyprland commit, plugin commit]`. If the running Hyprland commit matches, hyprpm checks out that plugin commit before building. If nothing matches, it builds the latest commit.

`.clang-format` is Hyprland’s formatting config for the pinned release. New C++ should follow it.

Built binaries, object files, logs, and editor swap files stay out of git. See `.gitignore`.

Input is handled with event listeners. Do not add function hooks for something an event already covers, and do not start threads that touch compositor state. Use the plugin API where it exists. The workspace swipe itself has no public entry point, so that gesture calls Hyprland’s internal swipe.

## Install

### hyprpm

```sh
hyprpm add https://github.com/ViolentSempie/hypr-mouse-gestures
hyprpm enable hypr-mouse-gestures
hyprpm reload
```

`hyprpm update` rebuilds installed plugins after a Hyprland update. The plugin still has to be pinned for that Hyprland commit, and the headers used at build time have to match the running compositor.

### Manual

```sh
make
hyprctl plugin load "$PWD/hypr-mouse-gestures.so"
```

To load it with the session, put the load at the top of the Lua config, before `hl.config` and any plugin dispatchers. A failed load can abort the rest of the config, so skip a missing file:

```lua
do
    local plugin = os.getenv("HOME") .. "/path/to/hypr-mouse-gestures.so"
    local file = io.open(plugin, "r")
    if file then
        file:close()
        hl.plugin.load(plugin)
    end
end
```

Unload with `hyprctl plugin unload` and the same path. After replacing the `.so` on disk, unload and load it again. Hyprland keeps the previous file mapped until that load uses a new path.

## Configure

Both options are registered when the plugin loads. Lua names use dots and underscores:

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

The same options in hyprlang are `plugin:hypr-mouse-gestures:drag_threshold` and `plugin:hypr-mouse-gestures:swipe_button`.

The drag also follows these existing Hyprland options:

- `gestures:workspace_swipe_invert` reverses the drag direction.
- `gestures:workspace_swipe_distance` and `general:gaps_workspaces` scale cursor movement into the swipe.
- `gestures:workspace_swipe_min_speed_to_force` can still commit a fast flick.

The mouse gesture commits at a quarter of a workspace. The trackpad gesture keeps `gestures:workspace_swipe_cancel_ratio`.

A drag that is mostly vertical is consumed and is not turned into a click. The gesture does not start while the session is locked or while another workspace swipe is already running.

## Develop

```sh
make
make clean
```

`make` writes `hypr-mouse-gestures.so` in the repository root.

A new gesture implements `IMouseGesture` (`src/Gesture.hpp`) and is added to `g_gestures` in `src/main.cpp`. Button events go to the first gesture that claims them. Motion events go to every gesture. `PLUGIN_EXIT` must drop listeners and end an in-progress gesture.

Config values are created in `PLUGIN_INIT` with `HyprlandAPI::addConfigValueV2`. Names start with `plugin:`. On a Lua config the legacy `addConfigValue` does not register anything.

Format C++ with the repository `.clang-format` before committing:

```sh
clang-format -i src/*.cpp src/*.hpp src/gestures/*.cpp src/gestures/*.hpp
```

When a change supports a Hyprland release, append a pin to `commit_pins` in `hyprpm.toml`:

```toml
["hyprland commit", "plugin commit that builds against it"], # x.y.z
```

The plugin commit has to be the commit hyprpm should check out, so update the pin after that commit exists.

### Nested session

`sandbox/hyprland.lua` is a small config for a nested Hyprland. It is not a live session config. It loads the `.so` from `~/.config/hypr/plugins/hypr-mouse-gestures/hypr-mouse-gestures.so`, so a clone in another directory needs that path changed.

Build first, then start the nested compositor without letting it take the active seat:

```sh
LIBSEAT_BACKEND=seatd HYPRLAND_NO_SD_VARS=1 HYPRLAND_NO_SD_NOTIFY=1 \
    Hyprland -c sandbox/hyprland.lua
```

`LIBSEAT_BACKEND=seatd` fails immediately when seatd is not running, which keeps the DRM backend from opening the real seat. Do not set `LIBSEAT_BACKEND=noop`. `HYPRLAND_NO_SD_VARS` and `HYPRLAND_NO_SD_NOTIFY` keep the nested instance from rewriting the user systemd environment.

Inside the sandbox, Super+Return opens kitty and Super+M exits. Talk to that instance with `hyprctl -i SIGNATURE`, not the default `hyprctl`, so commands do not hit the live session.
