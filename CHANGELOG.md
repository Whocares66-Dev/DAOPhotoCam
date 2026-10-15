# Changelog

## Unreleased

- The photo camera's keys, on and off (F8), lock (F7) and flying (W, A, S, D, and Q and E up and down), under "Photo Cam" in the controls screen; flying shares its keys with the game's walking (DAOLib #130).
- Its flight: look with the right mouse button, W A S D, the wheel for speed and Shift with the wheel for the field of view (15 to 120 degrees, top to bottom), each within its limits; tested without the game.
- Eluvian's tools when it is installed: `photocam_state`, `photocam_toggle`, `photocam_lock`, `photocam_pose` (with `lookAt`, `distance` and `height`), `photocam_move`, and the events `photocam.started`, `photocam.stopped`, `photocam.locked`. Angles are the game's facing and a pitch level at 0.
- The free view, the field of view and the keys and mouse through DAOLib #125, #126 and #127.
- The world paused while photo cam is on, and the game's interface hidden.
- A panel in the bottom right of the game's own options controls: lock, field of view, speed, Capture (the game's screenshot), Exit, and a square to make it small. Kept out of screenshots (DAOLib #133).
- A gold camera in the top bar, before Options, with its tooltip, to turn photo cam on.
- Esc ends photo cam; the pointer is hidden while the right button looks (DAOLib #132); the inventory's sounds as it opens and closes.
- Each start at 4 m/s and the game's field of view; the field of view 15 to 120 degrees.
- Not started while a loading screen is up.
