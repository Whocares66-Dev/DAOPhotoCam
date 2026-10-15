// Photo cam on Eluvian, the development bench's MCP server, when it is installed: its tools
// (photocam_state, photocam_toggle, photocam_lock, photocam_pose) and its events (photocam.*).
// Players will not have Eluvian, and photo cam carries on without it. The JSON is core/Tools'.
#pragma once

#include <string>

namespace photocam::eluvian
{

// Looks for Eluvian and registers the tools and events with it, once. Call every frame: the
// loader may load eluvian.dll after this plugin, so it is looked for on the first frame
// (EluvianAPI.h), and each later call returns at once.
void Find();

// Publishes an event, if Eluvian is there.
void Emit(const char *topic, const std::string &payloadJson);

} // namespace photocam::eluvian
