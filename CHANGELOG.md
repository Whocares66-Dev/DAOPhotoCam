# Changelog

## 0.1.0

The first release. Requires [DAOLib](https://www.nexusmods.com/dragonage/mods/7052) 0.3.0.

- A free camera for screenshots: on and off with F8 or the camera button in the top bar, from the close view in exploring or a fight.
- Look with the right mouse button held; fly with W, A, S, D, and Q and E straight up and down. The wheel sets the speed (0.5 to 40 m/s), Shift with the wheel the field of view (15 to 120 degrees). F7 locks the view; Esc ends it.
- The world is paused while it is on, in a fight too, and the game's interface hidden. Off, the camera goes back to the party.
- A panel in the game's own style: Lock camera, Field of view, Move speed, Capture (the game's screenshot) and Exit, and a square to make it small. It is never in a screenshot.
- Every key but the mouse and the wheel rebound in Options, Controls, under "Photo Cam".
- With [Eluvian](https://github.com/Whocares66-Dev/DAOEluvianMCP) installed, the tools `photocam_state`, `photocam_toggle`, `photocam_lock`, `photocam_pose` and `photocam_move`, and the events `photocam.started`, `photocam.stopped` and `photocam.locked`.

Built on DAOLib's free view (#125), field of view (#126), input (#127), key maps (#130), held cursor (#132) and screenshot hook (#133).
