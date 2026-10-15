# DAOPhotoCam

A free camera for screenshots in Dragon Age: Origins, a [DAOLib](https://github.com/Whocares66-Dev/DAOLib) plugin.

**Not yet released:** it needs DAOLib's free view (DAOLib #125), not yet on DAOLib's master. In a fight already under way it works; a fight does not start while it is on, and while locked the party does not take orders yet.

## Controls

| Key | Does |
|---|---|
| F8 | Photo camera on and off. Starts from the close (over-the-shoulder) view, not the tactical view; off puts the camera back where it was |
| F7 | Lock the photo camera where it is: the view holds, and the party can be moved into the shot. Again to unlock |
| Right mouse button held, mouse | Look around |
| W / S | Fly forward / back, the way the view looks |
| A / D | Fly left / right |
| Wheel up / down | Faster / slower (0.5 to 40 m/s) |
| Shift + wheel up / down | Narrower / wider field of view (30 to 150 degrees) |

F7 and F8 are rebound in the options screen's Controls tab, under "Photo Camera".

## For agents

With [Eluvian](https://github.com/Whocares66-Dev/DAOEluvianMCP) installed, photo cam adds the tools `photocam_state`, `photocam_toggle`, `photocam_lock`, `photocam_pose` (the view placed exactly: `eye`, `facingDegrees` as the game measures a creature's facing, `pitchDegrees`, `fovDegrees`, `speed`, or aimed at a point or creature with `lookAt` and `distance`) and `photocam_move` (a step along the view), and the events `photocam.started`, `photocam.stopped` and `photocam.locked`. Without Eluvian it works the same, with no tools.

## Installing

Install DAOLib first. Then extract the plugin's zip into the game's folder, so that `bin_ship\daolib\plugins\photocam\photocam.dll` is there.

## Building

Visual Studio 2022 with the C++ workload, vcpkg (`VCPKG_ROOT`), and LLVM for the linters. After cloning:

```powershell
git submodule update --init --recursive
.\tools\build.ps1 -Preset release -Test
.\tools\deploy.ps1 -Preset release
```

## Licence

GPLv3, as DAOLib's (`LICENSE`).
