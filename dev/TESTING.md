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
