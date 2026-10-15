---
title: For agents
nav_order: 4
---

# For agents

With [Eluvian](https://github.com/Whocares66-Dev/DAOEluvianMCP), the test bench's MCP server, installed, Photo Cam adds tools of its own. Without Eluvian it works the same, with no tools.

| Tool | Does |
|---|---|
| `photocam_state` | Whether it is on and locked, and the view: `eye`, `facingDegrees`, `pitchDegrees`, `fovDegrees`, `speed` |
| `photocam_toggle {on?}` | On or off, as <kbd>F8</kbd> |
| `photocam_lock {locked?}` | Lock or unlock, as <kbd>F7</kbd> |
| `photocam_pose {...}` | Place the view exactly |
| `photocam_move {forward?, right?, up?}` | Step along the view, in metres |

## Placing the view

`photocam_pose` takes any of:

- `eye`: a world point, `{x, y, z}` or `[x, y, z]`, in metres, z up.
- `facingDegrees`: the way the view looks, measured as the game measures a creature's facing. A creature's own facing looks its way.
- `pitchDegrees`: 0 level, up positive, -89 to 89.
- `lookAt`: a point, or a creature's or object's id (`"0x7d9"`, `"2009"` or `2009`). Alone, the view turns to face it. With `distance`, the eye goes that far back from it along the facing and pitch.
- `height`: how far above `lookAt` to aim. 1.5 m for a creature, whose position is its feet.
- `fovDegrees`, `speed`.

A view is placed before the tool answers, so a screenshot taken right after shows it. A pose read from `photocam_state` and given back to `photocam_pose` puts the camera in the same place, launch after launch.

```json
{"lookAt": "0x82", "distance": 3, "facingDegrees": 90, "pitchDegrees": -10}
```

## Events

`photocam.started`, `photocam.stopped` and `photocam.locked`, each with `by`: `key`, `tool`, `panel`, `bar`, or for a stop, `mode` (the game left the free view itself).
