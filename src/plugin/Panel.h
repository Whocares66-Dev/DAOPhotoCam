// Photo cam's panel: a box in the view's bottom right corner while photo cam is on, with its lock,
// its field of view and its speed, and a button to leave (ui/panel, built by tools/movie.py). The
// movie asks and tells the plugin through ExternalInterface; this answers it.
#pragma once

namespace photocam::panel
{

// Registers the movie's calls. Call from DAOLib_Load.
void Register();

// Adds the panel's screen once the game's interface is up. Call every frame on the game thread.
void Frame();

// Shows the panel, or hides it. Call on the game thread as photo cam starts and stops.
void Show(bool shown);

} // namespace photocam::panel
