// Photo cam as the rest of the plugin asks it, by its keys or by Eluvian's tools alike: each
// change is logged and its event published, whoever made it. On the game thread.
#pragma once

#include "core/Session.h"

#include <optional>
#include <string_view>

namespace photocam::plugin
{

// Photo cam's session: on or off, locked, and its flight.
const Session &State();
// The flight while photo cam is on, for a tool to place; null while off.
FlyCamera *Flight();

// Draws the flight's view now, as a tool places it, so a screenshot after the tool's answer shows
// it. If the game has left the free view, photo cam ends, as on a frame.
void DrawNow();

// Turns photo cam on or off.
// @param by who asked, for the log and the events: "key" or "tool"
// @return what the toggle did, or nullopt if this build cannot draw a free view
std::optional<Toggled> Toggle(std::string_view by);

// Locks photo cam's view or lets it fly again.
// @param by who asked: "key" or "tool"
Locked Lock(std::string_view by);

} // namespace photocam::plugin
