// The photo camera's side of the game: the game's camera read, the free view and its field of
// view drawn, the player's keys and mouse read and the game's own reaction to them withheld.
// Everything the plugin does to the game goes through here; src/core/ decides what.
//
// The free view, the field of view and the input are DAOLib's to give (DAOLib #125, #126, #127,
// asked 2026-10-15). The field of view and the input are on DAOLib's master; until the free view
// is, Available() says no and photo cam does not start.
#pragma once

#include "core/FlyCamera.h"
#include "core/Session.h"

namespace photocam::game
{

// Whether this DAOLib can draw a free view and read and withhold the input it is flown by.
bool Available();

// The game's camera now: whether there is one to take over, whether it is the tactical view,
// where it is and where it looks, and its field of view. Call on the game thread.
GameCamera Camera();

// Draws the scene from a pose with a field of view, until Release. Call every frame on the
// game thread while photo cam is on.
void Draw(const Pose &pose, float fovDegrees);

// Gives the game back its own camera and field of view, as they were before Draw.
void Release();

// The player's input since the last call, for the flight. Call once a frame on the game thread.
FlyInput ReadInput(float seconds);

// Withholds the game's own reaction to the keys and mouse photo cam flies by (moving the
// party, turning, panning and zooming its camera), or gives it back.
void Withhold(bool withheld);

} // namespace photocam::game
