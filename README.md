# DAOPhotoCam

A free camera for screenshots in Dragon Age: Origins, a [DAOLib](https://github.com/Whocares66-Dev/DAOLib) plugin.

**Not yet released.** It needs DAOLib at its master or later (the free view, the cursor held, the screenshot hook).

While the photo camera is on the world is paused, in a fight too, so nothing changes in the shot, and the game's interface is hidden. Off, the game carries on as it was.

A gold camera in the game's top bar, before Options, turns it on. In the screen's bottom right a panel, made of the game's own options controls, has the lock, a switch for the game's interface, the field of view and the speed, and Exit; its square at the top right makes it small. The panel is never in a screenshot. Each start begins at 4 m/s and the game's field of view.

## Controls

| Key | Does |
|---|---|
| F8 | Photo camera on and off. Starts from the close (over-the-shoulder) view, not the tactical view; off puts the camera back where it was |
| Esc | Photo camera off, as F8 |
| F7 | Lock the photo camera where it is: the view holds against the keys and mouse. Again to unlock |
| H | Show the game's interface over the view, or hide it again |
| Right mouse button held, mouse | Look around (the pointer hidden) |
| W / S | Fly forward / back, the way the view looks |
| A / D | Fly left / right |
| Q / E | Fly straight up / down |
| Wheel up / down | Faster / slower (0.5 to 40 m/s) |
| Shift + wheel up / down | Narrower / wider field of view (15 to 120 degrees, top to bottom) |

Every key but the mouse's and the wheel is rebound in the options screen's Controls tab, under "Photo Camera". Esc, H and W, A, S, D, Q, E are photo cam's own while it is on: the game keeps its own use of those keys otherwise.

## For agents

With [Eluvian](https://github.com/Whocares66-Dev/DAOEluvianMCP) installed, photo cam adds the tools `photocam_state`, `photocam_toggle`, `photocam_lock`, `photocam_pose` (the view placed exactly: `eye`, `facingDegrees` as the game measures a creature's facing, `pitchDegrees`, `fovDegrees`, `speed`, or aimed at a point or creature with `lookAt` and `distance`) and `photocam_move` (a step along the view), and the events `photocam.started`, `photocam.stopped` and `photocam.locked`. Without Eluvian it works the same, with no tools.

## Installing

Install DAOLib first. Then extract the plugin's zip into the game's folder, so that `bin_ship\daolib\plugins\photocam\photocam.dll` is there.

## Building

Visual Studio 2022 with the C++ workload, vcpkg (`VCPKG_ROOT`), and LLVM for the linters. The panel's and the top bar's movies need Python with Pillow, Java and [JPEXS FFDec](https://github.com/jindrapetrik/jpexs-decompiler) (`C:\Modding\ffdec_26.3.0`, or `--ffdec`). After cloning:

```powershell
git submodule update --init --recursive
python tools\camera_icon.py
python tools\movie.py
.\tools\build.ps1 -Preset release -Test
.\tools\deploy.ps1 -Preset release
```

## Licence

GPLv3, as DAOLib's (`LICENSE`).
