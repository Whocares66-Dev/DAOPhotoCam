// Photo cam's tools and events on Eluvian, the development bench's MCP server (C:\project\DAOEluvianMCP),
// as JSON: what each tool says it is, what it answers, the pose an agent asks for, and the events'
// payloads. Here so it is tested; src/plugin/Eluvian.cpp registers the tools and calls these.
//
// Names are prefixed with the plugin's, as Eluvian asks of a plugin's own (letters, digits and '_'
// in a tool's name, which reaches a model; dots in an event's topic). Angles are in degrees, the way
// a view looks as the game measures a creature's facing and its pitch 0 level, up positive, and
// points are {x, y, z} or [x, y, z], as Eluvian's own tools have them (Eluvian #143).
#pragma once

#include "core/FlyCamera.h"
#include "core/Session.h"

#include <cstdint>
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
inline constexpr const char *kMove = "photocam_move";

inline constexpr const char *kStarted = "photocam.started";
inline constexpr const char *kStopped = "photocam.stopped";
inline constexpr const char *kLocked = "photocam.locked";

// What a tool is, for RegisterTool: {"description": ..., "inputSchema": ...}.
std::string StateDescriptor();
std::string ToggleDescriptor();
std::string LockDescriptor();
std::string PoseDescriptor();
std::string MoveDescriptor();

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
// @param tool the tool's name, which starts a sentence for the agent
// @return the wanted value, nullopt to flip; or a sentence for the agent
std::variant<std::optional<bool>, std::string> ParseSwitch(std::string_view argsJson, std::string_view tool,
                                                           std::string_view field);

// A creature's or object's id, as Eluvian writes it ("0x7d9") and takes it ("0x7d9", "2009", 2009).
using ObjectId = std::uint32_t;

// What photocam_pose's lookAt names: a world point, or a thing in the world the plugin finds.
using Target = std::variant<Vec3, ObjectId>;

// How far above a creature's or object's position, which is at its feet, lookAt aims by default:
// about a person's chest, so a view aimed at one standing near does not look down on it (Eluvian's
// camera face, seen 2026-10).
inline constexpr float kThingHeight = 1.5f;

// The pose an agent asks for with photocam_pose; what it leaves out stays as it is.
struct PoseRequest
{
    std::optional<Vec3> eye;
    std::optional<float> facingDegrees; // the game's facing measure
    std::optional<float> pitchDegrees;
    std::optional<float> fovDegrees;
    std::optional<float> speed;
    std::optional<Target> lookAt;
    std::optional<float> distance; // metres from the aim, with lookAt
    std::optional<float> height;   // metres above lookAt's point or thing
};

// Reads photocam_pose's arguments.
// @return the request, or a sentence for the agent saying what is wrong with them
std::variant<PoseRequest, std::string> ParsePose(std::string_view argsJson);

// The point lookAt aims at: where it is, raised by the request's height, by default kThingHeight
// for a thing and none for a point.
// @param found the point, or the thing's position
Vec3 AimOf(const PoseRequest &request, Vec3 found);

// Puts the flight where the request says, each value within the flight's limits. With lookAt and
// a distance, the eye is that far from the aim, back along the facing and pitch (asked, or as they
// were); with lookAt alone, the view turns from the eye to face the aim.
// @param aim AimOf, for a request with lookAt
// @return empty, or a sentence for the agent when the view cannot face the aim
std::string Apply(FlyCamera &fly, const PoseRequest &request, std::optional<Vec3> aim = std::nullopt);

// A step photocam_move asks for, in metres: along the way the view looks, to its right (level),
// and up.
struct MoveRequest
{
    float forward = 0.0f;
    float right = 0.0f;
    float up = 0.0f;
};

// Reads photocam_move's arguments.
// @return the request, or a sentence for the agent saying what is wrong with them
std::variant<MoveRequest, std::string> ParseMove(std::string_view argsJson);

// Moves the view by the step; it turns not at all.
void Apply(FlyCamera &fly, const MoveRequest &request);

// The sentence a tool answers when the toggle did not start photo cam.
std::string Refusal(Toggled toggled);

// Who or what turned photo cam on or off, or locked it: "key", "tool", or for a stop, "mode"
// when the game left the modes that have a camera.
std::string StartedJson(const FlyCamera &fly, std::string_view by);
std::string StoppedJson(std::string_view by);
std::string LockedJson(bool locked, std::string_view by);

} // namespace photocam::tools
