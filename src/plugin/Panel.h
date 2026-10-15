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

// Shows the game's interface over the free view, or hides it again as the fly camera has it. The
// user, 2026-10-15: "a check under lock camera to toggle game ui, default off", and H for it.
void ShowInterface(bool shown);
// Whether the game's interface is shown over the free view.
bool InterfaceShown();

// Shows the panel, or hides it. Call on the game thread as photo cam starts and stops.
void Show(bool shown);

} // namespace photocam::panel
