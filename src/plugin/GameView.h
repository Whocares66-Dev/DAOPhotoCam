// The photo camera's side of the game: the game's camera read, the free view and its field of
// view drawn, the player's keys and mouse read and the game's own reaction to them withheld.
// Everything the plugin does to the game goes through here; src/core/ decides what.
//
// The free view, the field of view and the input are DAOLib's to give (DAOLib #125, #126, #127,
// asked 2026-10-15). The free view is the game's own fly camera mode, entered and placed by
// dao::camera::SetFreeView each frame.
#pragma once

#include "core/FlyCamera.h"
#include "core/Session.h"

#include <cstdint>
#include <optional>

namespace photocam::game
{

// Whether this DAOLib can draw a free view and read and withhold the input it is flown by.
bool Available();

// The game's camera now: whether there is one to take over, whether it is the tactical view,
// where it is and where it looks, and its field of view. Call on the game thread.
GameCamera Camera();

// Draws the scene from a pose with a field of view, until Release. Call every frame on the
// game thread while photo cam is on; the first call enters the free view.
// @return false, with nothing changed, if the game would not enter it (not exploring or fighting),
//  or has left it on its own (a menu, a conversation, a script): then it has ended
bool Draw(const Pose &pose, float fovDegrees);

// Where a creature or object is: its position, at its feet, as the scripts' GetPosition has it.
// @return nullopt if the id names nothing in the game
std::optional<Vec3> PositionOf(std::uint32_t id);

// Gives the game back its own camera and field of view, as they were before Draw.
void Release();

// Pauses the world, so nothing moves in the shot, and remembers whether the game was paused
// already. Call before the first Draw, while the game is still exploring or fighting.
void Freeze();

// Gives the game back the pause Freeze found. Call after Release.
void Thaw();

// Registers the flight's keys as actions the player rebinds under "Photo Camera": forward, back,
// left and right, W, S, A and D by default, in photo cam's own key map, so they share their keys
// with the game's walking. Call from DAOLib_Load.
// @return false if DAOLib refused one
bool RegisterMoves();

// The player's input since the last call, for the flight. Call once a frame on the game thread.
FlyInput ReadInput(float seconds);

// Withholds the game's own reaction to the keys and mouse photo cam flies by (moving the
// party, turning, panning and zooming its camera) and turns photo cam's key map on, or gives the
// game them back.
void Withhold(bool withheld);

} // namespace photocam::game
