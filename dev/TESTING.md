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

## 2026-10-15: #125 at db4588f, the keys and mouse, the new tools, combat

DAOLib `gh-125-free-view` `db4588f`, debug, the same save. While the user was away, the game's window was put in front and the cursor over it, and keys and the mouse were sent as real input.

- F8 starts and (from the fly camera mode, once the toggle's action named that mode) stops photo cam. F7 locks there. Before the actions named the mode, F7 did nothing while photo cam was on: an action's key works only in the modes it names.
- The right button held with the mouse 400 counts right turned the view 54 degrees clockwise (facing -90 to -35.6). Two wheel notches up: speed 4 to 6.25. Shift with two notches up: field of view 60 to 50. The game's own zoom and orbit did not move.
- Locked, the view held while W was down, but W did not walk Lyna and a left click on the ground did not move her: in the fly camera mode the party takes no orders. Asked of lib-agent.
- The facing is the game's: the start view along +x read facing -90, Lyna's own facing, as she faces +x.
- `photocam_pose {lookAt: "0x10d", distance: 3, facingDegrees: 90, pitchDegrees: -10}` put the eye 3 m on Tamlen's +x side; `screen` put him at x 800 of 1600, his chest about the middle. `photocam_move {forward: 1, right: 1, up: 0.5}` moved the eye exactly so (along -x 10 degrees down, toward +y, up).
- Off a frame after on (two tool calls): explore again the frame after. `SetGameMode(GM_EXPLORE)` by Eluvian's script tool while on: photo cam stopped by "mode", the game stayed in explore, the camera given back.
- Combat: started during a fight with three wolves, photo cam held to the fight's end, the party fighting on; off went fly camera, combat, explore. But a wolf spawned hostile while photo cam was on stood idle for 15 s and the fight began only when photo cam was off: a fight does not start in the fly camera mode. Asked of lib-agent.

## 2026-10-15: the world paused while photo cam is on

The same branch and save. The user: "the whole world should probably pause so things don't change what the user is looking for the best screenshot".

- Two wolves spawned hostile, the fight begun; photo cam on: the game paused in the fly camera mode, a wolf held mid-leap at Lyna. Flown round it with `photocam_pose {lookAt: "0x82", distance: 3.5, facingDegrees: 0}` and real S held (2.07 m back while paused): the wolves kept their poses and every creature its health.
- Off: the game went back to combat unpaused, and the fight went on (Tamlen killed a wolf).
- Paused first by Eluvian's `pause`, then photo cam on and off: back in combat and still paused.
- Space, the game's pause key, does nothing while photo cam is on (the fly camera mode), so the world stays still.

## 2026-10-15: W, A, S and D as photo cam's actions (DAOLib #130)

DAOLib master `4b4ff22`, debug, the same save, real keys.

- `KeyBindings.ini` has `PhotoCamForward_0=Keyboard::Button_W` (and Back S, Left A, Right D) beside the game's `RunCharacterForward_0=Keyboard::Button_W`: both keep W.
- F8, then W held a second: the view flew 4.0 m along the view, and Lyna did not move.
- F8 off, then W held a second: Lyna walked (0.9 m from Tamlen before, 2.0 m after).

## 2026-10-15: the panel, the top bar's camera, Esc, Q and E, the cursor

Branch `photocam-panel`, on a local merge of DAOLib's branches for #132 (the cursor held), #133 (screenshots) and #134 (a plugin's screen played while paused) over master `7d21272`. The user played; I drove the tools and read the logs.

- The panel in the bottom right: the options screen's checkbox, sliders and Cancel button imported from `optionsmenu.swf`, in the tooltip's box. It follows the tools' changes (field of view 90, speed 12, the lock), and the user's clicks work while paused: "unlocked by the panel", the sliders, Exit, minimize and maximize.
- Paused, the game advanced the panel's movie with no time: GFx's own pause, which the game sets on every new movie (lib-agent's probe, `probe.log`). With `kScreenAdvancesWhilePaused`, `photocam.log` says "photocam's movie was paused by the game; it plays on", the tick shows at once, and the glows play (the user).
- Off, the panel goes with photo cam; the game shows its HUD's screens again as it leaves the fly camera, so the movie itself is hidden too.
- Screenshots: Eluvian's source "game" and the user's Print Screen (`Screenshot20261015114054715.jpg`) have no panel, and it comes back after.
- The top bar's camera: before Options, the bar's art one place wider; loaded into the bar's movie (record `NavBar`) once each movie; its tooltip "Photo Cam (F8)" (first "Photo Camera"); a click toggles photo cam; back at once with the bar as photo cam ends (the user: "they all work").
- Esc ends photo cam (it takes the game's ToggleMainMenu while photo cam's key map is on); Q and E fly up and down; the pointer is hidden while the right button looks (the user).
- Opening and closing play the inventory's sounds.
