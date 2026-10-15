// Photo cam's tools and events on Eluvian, the development bench's MCP server (C:\project\DAOEluvianMCP),
// as JSON: what each tool says it is, what it answers, the pose an agent asks for, and the events'
// payloads. Here so it is tested; src/plugin/Eluvian.cpp registers the tools and calls these.
//
// Names are prefixed with the plugin's, as Eluvian asks of a plugin's own (letters, digits and '_'
// in a tool's name, which reaches a model; dots in an event's topic). Angles are in degrees, and
// points are {x, y, z} or [x, y, z], as Eluvian's own tools have them.
#pragma once

#include "core/FlyCamera.h"
#include "core/Session.h"

#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace photocam::tools
{

inline constexpr const char *kState = "photocam_state";
inline constexpr const char *kToggle = "photocam_toggle";
inline constexpr const char *kLock = "photocam_lock";
inline constexpr const char *kPose = "photocam_pose";

inline constexpr const char *kStarted = "photocam.started";
inline constexpr const char *kStopped = "photocam.stopped";
inline constexpr const char *kLocked = "photocam.locked";

// What a tool is, for RegisterTool: {"description": ..., "inputSchema": ...}.
std::string StateDescriptor();
std::string ToggleDescriptor();
std::string LockDescriptor();
std::string PoseDescriptor();

// An event's topic and what it means, for DescribeEvent.
struct EventDescription
{
    const char *topic;
    std::string descriptor;
};
std::vector<EventDescription> Events();

// Photo cam's state as photocam_state answers it, and as the other tools answer after acting.
// @param available whether this build can draw a free view at all
std::string StateJson(const Session &session, bool available);

// What a photocam_toggle or photocam_lock call asks: to flip, or to be on (or locked) or off.
// @return the wanted value, nullopt to flip; or a sentence for the agent
std::variant<std::optional<bool>, std::string> ParseSwitch(std::string_view argsJson, std::string_view field);

// The pose an agent asks for with photocam_pose; what it leaves out stays as it is.
struct PoseRequest
{
    std::optional<Vec3> eye;
    std::optional<float> headingDegrees;
    std::optional<float> pitchDegrees;
    std::optional<float> fovDegrees;
    std::optional<float> speed;
};

// Reads photocam_pose's arguments.
// @return the request, or a sentence for the agent saying what is wrong with them
std::variant<PoseRequest, std::string> ParsePose(std::string_view argsJson);

// Puts the flight where the request says, each value within the flight's limits.
void Apply(FlyCamera &fly, const PoseRequest &request);

// The sentence a tool answers when the toggle did not start photo cam.
std::string Refusal(Toggled toggled);

// Who or what turned photo cam on or off, or locked it: "key", "tool", or for a stop, "mode"
// when the game left the modes that have a camera.
std::string StartedJson(const FlyCamera &fly, std::string_view by);
std::string StoppedJson(std::string_view by);
std::string LockedJson(bool locked, std::string_view by);

} // namespace photocam::tools
