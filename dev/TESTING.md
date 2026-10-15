# Testing in the game

What is seen in the game, newest last. A test deploys nothing: it runs the named instance `photocam` (Eluvian's `game_instance`), whose profile is `%LOCALAPPDATA%\DAOLib\instances\photocam\BioWare\Dragon Age`, on a staged plugins folder, `build\stage\plugins`, holding this build's `photocam\` and a copy of the installed `eluvian\`:

```
python C:\project\DAOEluvianMCP\tools\eluvian_bridge.py --instance photocam --call game_launch "{\"owner\": \"photocam-agent\", \"seed\": \"save\", \"load\": {\"character\": \"Lyna\", \"kind\": \"manual\", \"number\": 4}, \"plugins\": \"C:/project/DAOPhotoCam/build/stage/plugins\"}"
```

Take pictures with `screenshot {"source": "game"}`: `window` came out black with the display asleep.

## 2026-10-15: the keys and the tools, before the free view

`d262438`, debug, DAOLib master `28824f2`, GOG's game, Lyna's manual 4 in the Forest Clearing.

- `photocam.log`: loaded, then "tools and events registered with Eluvian" on the first frame.
- `health`'s `plugins`: `photocam_state`, `photocam_toggle`, `photocam_lock`, `photocam_pose`. `events {"describe": true}`: the three `photocam.*` topics with their fields.
- `photocam_state`: `{"available": false, "locked": false, "on": false}`.
- `photocam_toggle`: refused, waiting on DAOLib #125 to #127, and the log's warning said so. `{"on": 1}`: refused, `on` is true or false. `photocam_lock` and `photocam_pose` with photo cam off: refused, turn it on first. `photocam_pose {"fov": 90}`: refused, naming the arguments.
- `KeyBindings.ini`: `PhotoCamToggle_0=Keyboard::Button_F8`, `PhotoCamLock_0=Keyboard::Button_F7`.
- The options screen's Controls tab (`images/controls-photo-camera-2026-10-15.jpg`): a "Photo Camera" heading at the end of the list, "Photo camera on/off" F8 and "Lock photo camera" F7.

Not yet seen: the keys pressed (the same code as the tools), and everything the free view does.

## 2026-10-15: the free view, DAOLib #125's branch

DAOLib `gh-125-free-view` `9e3b599` on master `06660d3`, debug, the same save. Driven by the tools, and F8, W and D sent as real keys to the game's window in front.

- F8 starts photo cam: the game's mode becomes `flycam`, the HUD goes, and the picture does not move: the start view is the close camera's, eye (52.30, 144.35, -12.36), looking along +x 5 degrees down (the camera tool's yaw 180, pitch 85; the fly camera's yaw -90, pitch -5).
- The fly camera's yaw is a quarter turn behind the heading: yaw 0 drew a view along +y, -45 one between +x and +y. Its pitch is up positive: 30 drew the sky. `GameView` converts.
- At first F8 started and stopped photo cam in one frame: the key's callback runs before the frame's, and the game changes its mode a frame later. The free view now has ten frames to become the mode.
- W held a second at 4 m/s: the eye moved 4.0 m along the view, down with its pitch. D held half a second: 2.07 m toward -y, the view's right. Poses held still across screenshots: the game does not move the camera itself.
- Off (the tool, the key): explore mode again a frame later, the camera exactly where it was, the HUD back.
- From combat (a wolf spawned hostile): photo cam starts, the fight goes on, off returns to combat. During the fight a script of the game's changed the mode (`gamemode_change`), flycam to explore to combat, and photo cam stopped by itself ("by mode"), the camera given back.
- While on, Eluvian's `screen` and the pick follow the drawn view: Lyna's feet at (797, 545), where she stands in the picture. Nothing near the eye hides or fades; at 0.15 m from Lyna's head nothing is clipped.
- `dao::camera::Axes()` is wrong on this branch (its forward always has x 0); told lib-agent. Photo cam does not use it; `LogAxes` logs it beside the pose at a start and a lock.

Not yet seen: the mouse look and the wheel (the right button sent by `SendInput` went to the window under the real cursor, not the game's), Shift with the wheel, F7, the auto-stop on a menu or a conversation.
