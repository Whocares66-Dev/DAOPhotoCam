# DAOPhotoCam -- project context

A free camera for screenshots in Dragon Age: Origins, a plugin built on DAOLib (the native plugin loader and library, `C:\project\DAOLib`, here the submodule `extern/DAOLib`). This session is photocam-agent.

Markdown here is not hard-wrapped: one line per paragraph or list item.

## What it does -- the user's words, 2026-10-15

"When toggled on in over-the-shoulder mode (not tactical mode), it should allow user to fly around freely without going into tactical mode when zoomed out too far. Right mouse button held and mouse move to look around, W to move forward, S to move back, A to move left, D to move right. Scroll up to increase movement speed, down to reduce movement speed (within limits). Shift-scroll up to reduce FOV (min 30), shift-scroll down to increase FOV (max 150)." A key the player sets in the options screen toggles it; "when off return to previous camera position before photo cam was activated". And "a key to lock camera. When locked, camera cannot be moved."

As built: two actions under the heading "Photo Camera" in the controls screen, `PhotoCamToggle` (F8) and `PhotoCamLock` (F7), rebindable and kept by the game in `KeyBindings.ini`. Photo cam starts only in explore or combat mode with the close view up. Locked, the view, speed and field of view hold, and the game has its keys and mouse back so the party can be walked into the shot. Photo cam ends by itself when the game leaves the modes that have a camera (a menu, a conversation, a load). The speed is kept from one start to the next; the field of view starts at the game's each time.

## On Eluvian

When Eluvian (the bench's MCP server, `C:\project\DAOEluvianMCP`, eluvian-agent's) is installed, photo cam registers its own tools and events through Eluvian's C interface (`src/plugin/EluvianAPI.h`, a copy of Eluvian's `src/api/EluvianAPI.h`, interface version 3; copy it again when Eluvian's changes). Eluvian is general and a mod's tools are the mod's to register and document (Eluvian's `CLAUDE.md`), so they are here, not there. Names carry the plugin's prefix, as Eluvian and the models' APIs allow only letters, digits, `_` and `-` in a tool's name; topics may have dots.

- `photocam_state` (read-only): available, on, locked, and while on the eye, `headingDegrees`, `pitchDegrees`, `fovDegrees`, `speed`.
- `photocam_toggle {on?}` and `photocam_lock {locked?}`: as the keys, or set outright; already so is no error.
- `photocam_pose {eye?, headingDegrees?, pitchDegrees?, fovDegrees?, speed?}`: the view placed exactly, for an agent that cannot hold a mouse button; within the flight's limits.
- `photocam.started`, `photocam.stopped`, `photocam.locked`, each with `by`: key, tool, or (stopped) mode.

The JSON is `src/core/Tools`, tested; `src/plugin/Eluvian.cpp` looks for Eluvian on the first frame and registers. Seen registered and answering in the game 2026-10-15 (`dev/TESTING.md`).

## Status

The flight and the on/off rules are built and tested. Drawing the free view, the field of view and the input wait on DAOLib: #125 (a free view the scene is drawn from), #126 (the field of view), #127 (keys, mouse and wheel read through the game's input, and the game's reaction withheld), asked of lib-agent 2026-10-15. Until they land `game::Available()` is false and the toggle logs that and does nothing. When each lands: move `extern/DAOLib` to its commit, fill in `src/plugin/GameView.cpp`, and see it in the game.

## DAOLib

What is general belongs in DAOLib and is lib-agent's to build (the user, 2026-10-15: "ask lib-agent to do work on daolib as necessary"); nothing in `extern/DAOLib` is changed from here (one agent, one repo). An engine address is named through addr-lib-agent, as DAOLib's `CLAUDE.md` says. `DAOLIB_DIR` (the environment's, or CMake's) builds against another checkout instead of the submodule. DAOLib's manifest dependencies are repeated in `vcpkg.json` (a manifest build reads only the top-level one): `directxtex` is there for DAOLib's core.

## Layout

```
src/core/      the flight (FlyCamera), photo cam on, off and locked (Session), the tools' JSON (Tools): no Windows, no dao::, tested
src/plugin/    photocam.dll: PhotoCam the keys, the frame and the log; GameView the game's side; Eluvian the tools
tests/         Catch2 over src/core
tools/         build.ps1, deploy.ps1, check-powershell.ps1, console.ps1
extern/DAOLib  the library, a submodule
```

The core's world is the game's: metres, z up. Its heading is the direction the view looks, from +x toward +y; the game's `SceneCamera::Yaw` is from the focus to the camera, so `GameView` turns one into the other, and the pitch from the scene camera's from-straight-above. Both conversions are to be checked in play when the free view lands.

## Building and playing

```powershell
.\tools\build.ps1 -Preset debug -Test -Format -Lint   # build, tests, formatters, clang-tidy, ruff, PSScriptAnalyzer
.\tools\build.ps1 -Preset debug-asan -Test            # the tests under AddressSanitizer
.\tools\build.ps1 -Preset debug-analyze               # MSVC's /analyze
.\tools\build.ps1 -Preset release
.\tools\deploy.ps1 -Preset release                    # the game closed
.\tools\deploy.ps1 -Remove
```

Everything is 32-bit, as the game is. Output is `build\<preset>\daolib\plugins\photocam\`; the log is `Documents\BioWare\Dragon Age\daolib\photocam.log` (on this machine under OneDrive). The four runs above are the gate before a merge to master, not before every commit. Last green 2026-10-15.

The game is driven through Eluvian (`.mcp.json`), as the named instance `photocam` on a staged plugins folder, so nothing is deployed (`dev/TESTING.md` has the launch). Other sessions share the machine: check for a running game first, and quit ours after.

## Style

DAOLib's: clang-format's Microsoft style, clang-tidy with `.clang-tidy`'s exclusions, comments that say why and what was read, with dates.
