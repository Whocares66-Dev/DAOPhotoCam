#include "core/Tools.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <charconv>
#include <cmath>
#include <format>
#include <numbers>

namespace photocam::tools
{

namespace
{

using Json = nlohmann::json;

constexpr float kDegrees = 180.0f / std::numbers::pi_v<float>;

// The game's facing (the scripts' GetFacing and SetFacing, Eluvian's `facingDegrees`) is 0 along -y
// and grows clockwise seen from above: the core's heading, from +x toward +y, is -90 less it. Seen
// in the game by eluvian-agent 2026-10-15 (Eluvian #143): facing 0 looked along -y, -51.4 along
// (0.78, -0.62).
float FacingOf(float heading)
{
    return std::remainder(-90.0f - (heading * kDegrees), 360.0f);
}

float HeadingOf(float facingDegrees)
{
    return (-90.0f - facingDegrees) / kDegrees;
}

Json VecJson(const Vec3 &v)
{
    return Json{{"x", v.x}, {"y", v.y}, {"z", v.z}};
}

Json PoseJson(const FlyCamera &fly)
{
    const Pose &pose = fly.Current();
    return Json{{"eye", VecJson(pose.eye)},
                {"facingDegrees", FacingOf(pose.heading)},
                {"pitchDegrees", pose.pitch * kDegrees},
                {"fovDegrees", fly.FovDegrees()},
                {"speed", fly.Speed()}};
}

// The agent's arguments as an object; an empty text is no arguments.
std::variant<Json, std::string> ArgsOf(std::string_view argsJson, std::string_view tool)
{
    if (argsJson.find_first_not_of(" \t\r\n") == std::string_view::npos)
    {
        return Json::object();
    }
    Json args = Json::parse(argsJson, nullptr, false);
    if (args.is_discarded() || !args.is_object())
    {
        return std::format("{}: the arguments are not a JSON object.", tool);
    }
    return args;
}

std::optional<float> FiniteOf(const Json &value)
{
    if (!value.is_number())
    {
        return std::nullopt;
    }
    const auto number = value.get<double>();
    return std::isfinite(number) ? std::optional(static_cast<float>(number)) : std::nullopt;
}

std::optional<Vec3> PointOf(const Json &value)
{
    if (value.is_array() && value.size() == 3)
    {
        const auto x = FiniteOf(value[0]);
        const auto y = FiniteOf(value[1]);
        const auto z = FiniteOf(value[2]);
        if (x && y && z)
        {
            return Vec3{*x, *y, *z};
        }
        return std::nullopt;
    }
    if (value.is_object() && value.size() == 3 && value.contains("x") && value.contains("y") && value.contains("z"))
    {
        return PointOf(Json::array({value["x"], value["y"], value["z"]}));
    }
    return std::nullopt;
}

// An id as Eluvian takes it: "0x7d9", "2009" or the number 2009.
std::optional<ObjectId> IdOf(const Json &value)
{
    if (value.is_number_unsigned())
    {
        const auto id = value.get<std::uint64_t>();
        return id <= UINT32_MAX ? std::optional(static_cast<ObjectId>(id)) : std::nullopt;
    }
    if (!value.is_string())
    {
        return std::nullopt;
    }
    const auto &text = value.get_ref<const std::string &>();
    const bool hex = text.starts_with("0x") || text.starts_with("0X");
    const char *first = text.c_str() + (hex ? 2 : 0);
    const char *last = text.c_str() + text.size();
    ObjectId id = 0;
    const auto [end, error] = std::from_chars(first, last, id, hex ? 16 : 10);
    if (first == last || error != std::errc{} || end != last)
    {
        return std::nullopt;
    }
    return id;
}

std::optional<Target> TargetOf(const Json &value)
{
    if (const auto point = PointOf(value))
    {
        return Target{*point};
    }
    if (const auto id = IdOf(value))
    {
        return Target{*id};
    }
    return std::nullopt;
}

Vec3 operator+(const Vec3 &a, const Vec3 &b)
{
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

Vec3 operator*(const Vec3 &v, float k)
{
    return {v.x * k, v.y * k, v.z * k};
}

// The request's number an argument names, null for a name that is none of them.
std::optional<float> *NumberOf(PoseRequest &request, std::string_view name)
{
    if (name == "facingDegrees")
    {
        return &request.facingDegrees;
    }
    if (name == "pitchDegrees")
    {
        return &request.pitchDegrees;
    }
    if (name == "fovDegrees")
    {
        return &request.fovDegrees;
    }
    if (name == "speed")
    {
        return &request.speed;
    }
    if (name == "distance")
    {
        return &request.distance;
    }
    if (name == "height")
    {
        return &request.height;
    }
    return nullptr;
}

// Reads one of photocam_pose's arguments into the request.
// @return empty, or a sentence for the agent
std::string ReadPoseArgument(PoseRequest &request, const std::string &key, const Json &value)
{
    if (key == "eye")
    {
        request.eye = PointOf(value);
        return request.eye ? ""
                           : "photocam_pose: `eye` is a world point of three finite numbers: {x, y, z} or [x, y, z].";
    }
    if (key == "lookAt")
    {
        request.lookAt = TargetOf(value);
        return request.lookAt ? ""
                              : R"(photocam_pose: `lookAt` is a world point, {x, y, z} or [x, y, z], or a creature's )"
                                R"(or object's id, "0x7d9", "2009" or 2009.)";
    }
    std::optional<float> *number = NumberOf(request, key);
    if (number == nullptr)
    {
        return std::format("photocam_pose: unknown argument `{}`: the arguments are eye, facingDegrees, pitchDegrees, "
                           "lookAt, distance, height, fovDegrees and speed.",
                           key);
    }
    *number = FiniteOf(value);
    return *number ? "" : std::format("photocam_pose: `{}` is a finite number.", key);
}

// Whether the request's arguments go together.
// @return empty, or a sentence for the agent
std::string CheckPose(const PoseRequest &request)
{
    if (!request.lookAt && (request.distance || request.height))
    {
        return "photocam_pose: `distance` and `height` go with `lookAt`.";
    }
    if (request.distance && *request.distance < 0.1f)
    {
        return "photocam_pose: `distance` is 0.1 metres or more.";
    }
    if (request.lookAt && request.distance && request.eye)
    {
        return "photocam_pose: with `lookAt` and `distance` the eye is placed for you: leave out `eye`, or leave out "
               "`distance` to turn the view from the eye toward `lookAt`.";
    }
    if (request.lookAt && !request.distance && (request.facingDegrees || request.pitchDegrees))
    {
        return "photocam_pose: `lookAt` without `distance` sets the facing and pitch itself: leave them out, or "
               "give `distance` to place the eye that far from the aim along them.";
    }
    return "";
}

// Said in each tool's description, so an agent reading any one of them cold knows.
constexpr const char *kWhileOn =
    " While photo cam is on the game is in its fly camera mode (`flycam`), the interface is hidden, Eluvian's "
    "`camera` tool does not work and the photocam_ tools have the view; `screen` and the pick follow it, and a "
    "`screenshot` with source game comes out without the interface. `facingDegrees` is the way the view looks, "
    "measured as the game's facing, so a creature's `facingDegrees` copied into photocam_pose looks the way "
    "it does; `pitchDegrees` is 0 level, up positive.";

Json NumberProperty(const char *description)
{
    return Json{{"type", "number"}, {"description", description}};
}

} // namespace

std::string StateDescriptor()
{
    return Json{{"description",
                 std::string("Photo cam's state: whether this build can fly it (`available`), whether it is on and "
                             "locked, and while on, where the view is (`eye`), the way it looks (`facingDegrees`, "
                             "`pitchDegrees`), its field of view and its speed "
                             "in metres a second.") +
                     kWhileOn},
                {"inputSchema", {{"type", "object"}, {"properties", Json::object()}}}}
        .dump();
}

std::string ToggleDescriptor()
{
    return Json{
        {"description",
         std::string(
             "Turns photo cam, the free camera, on or off, as its key does (F8 by default). It starts "
             "from the game's camera in the close view, in explore or combat mode, and not in the "
             "tactical view. The world is paused while it is on, so nothing moves in the shot and no fight "
             "starts; off gives the game back its camera where it was and its pause as it was. It ends by itself when "
             "the game leaves the fly camera mode (a menu, a conversation, a load, or a script of the "
             "game's in a fight). Answers photo cam's state. photocam.started and photocam.stopped "
             "follow; place the view with photocam_pose or photocam_move.") +
             kWhileOn},
        {"inputSchema",
         {{"type", "object"},
          {"properties",
           {{"on",
             {{"type", "boolean"},
              {"description", "true to turn it on, false off; left out, it flips. Already so is no "
                              "error."}}}}}}}}
        .dump();
}

std::string LockDescriptor()
{
    return Json{{"description",
                 "Locks photo cam's view where it is, as its key does (F7 by default): the view, speed and field of "
                 "view hold against the keys and mouse. photocam_pose and photocam_move still place a locked "
                 "view. Only while photo cam is on. Answers photo cam's state; "
                 "photocam.locked follows."},
                {"inputSchema",
                 {{"type", "object"},
                  {"properties",
                   {{"locked",
                     {{"type", "boolean"},
                      {"description", "true to lock, false to unlock; left out, it flips. Already so is no "
                                      "error."}}}}}}}}
        .dump();
}

std::string PoseDescriptor()
{
    return Json{
        {"description",
         "Puts photo cam's view exactly: where it is, the way it looks, its field of view and its speed. What is "
         "left out stays as it is; each value is kept within photo cam's limits, and the answer, photo cam's state, "
         "says what was taken. Only while photo cam is on (photocam_toggle); a locked view is placed too.\n\n"
         "To look at something, give `lookAt`, a point or a creature's or object's id:\n"
         "- alone, or with `eye`: the view turns from the eye to face it.\n"
         "- with `distance`: the eye is put that far from it, back along `facingDegrees` and `pitchDegrees` (given, "
         "or as they are), so it is in the middle of the picture. Shots all round it are calls with facings 0, 45, "
         "90 and so on.\n\n"
         "The view is placed in the game before this answers: a `screenshot` taken after it shows the new view."},
        {"inputSchema",
         {{"type", "object"},
          {"properties",
           {{"eye", {{"description", "Where the view is, a world point: {x, y, z} or [x, y, z], metres, z up."}}},
            {"facingDegrees",
             NumberProperty("The way the view looks, in degrees, as the game measures a creature's facing.")},
            {"pitchDegrees",
             NumberProperty("How far above the horizon the view looks, -89 to 89; negative looks down.")},
            {"lookAt",
             {{"description", R"(What to look at: a world point, {x, y, z} or [x, y, z], or a creature's or )"
                              R"(object's id as `creatures`, `placeables` and `surroundings` give it ("0x7d9", )"
                              R"("2009" or 2009).)"}}},
            {"distance", NumberProperty("With `lookAt`: how far the eye is put from it, in metres, 0.1 or more. "
                                        "Default: the eye stays and the view turns to face it.")},
            {"height", NumberProperty("With `lookAt`: how far above it to aim, in metres. Default: 1.5 for a "
                                      "creature or object, whose position is at its feet; 0 for a point.")},
            {"fovDegrees", NumberProperty("The field of view, top to bottom of the picture, 15 to 120 degrees.")},
            {"speed", NumberProperty("The flying speed, 0.5 to 40 metres a second.")}}}}}}
        .dump();
}

std::string MoveDescriptor()
{
    return Json{{"description",
                 "Moves photo cam's view by a step, as the keys fly it but exactly: `forward` along the way it looks "
                 "(up or down as it looks), `right` level to its side, `up` straight up; negative goes the other "
                 "way. It does not turn. Only while photo cam is on (photocam_toggle); a locked view moves too. "
                 "Answers photo cam's state. The view is placed in the game before this answers: a `screenshot` "
                 "taken after it shows the new view."},
                {"inputSchema",
                 {{"type", "object"},
                  {"properties",
                   {{"forward", NumberProperty("Metres along the way the view looks. Default: 0.")},
                    {"right", NumberProperty("Metres to the view's right, level. Default: 0.")},
                    {"up", NumberProperty("Metres straight up. Default: 0.")}}}}}}
        .dump();
}

std::vector<EventDescription> Events()
{
    return {
        {kStarted, Json{{"description", "Photo cam turned on, from the game's camera."},
                        {"fields",
                         {{"by", "key or tool"},
                          {"eye", "where the view is"},
                          {"facingDegrees", "the way it looks, as a creature's facing"},
                          {"pitchDegrees", "above the horizon"},
                          {"fovDegrees", "the field of view"},
                          {"speed", "metres a second"}}}}
                       .dump()},
        {kStopped, Json{{"description", "Photo cam turned off; the game has its camera back where it was."},
                        {"fields",
                         {{"by", "key, tool, or mode: the game left the modes with a camera (a menu, a "
                                 "conversation, a load)"}}}}
                       .dump()},
        {kLocked, Json{{"description", "Photo cam's view locked or unlocked."},
                       {"fields", {{"locked", "true when locked"}, {"by", "key or tool"}}}}
                      .dump()},
    };
}

std::string StateJson(const Session &session, bool available)
{
    Json state{{"available", available}, {"on", session.Active()}, {"locked", session.IsLocked()}};
    if (const FlyCamera *fly = session.Fly())
    {
        state.update(PoseJson(*fly));
    }
    return state.dump();
}

std::variant<std::optional<bool>, std::string> ParseSwitch(std::string_view argsJson, std::string_view tool,
                                                           std::string_view field)
{
    auto parsed = ArgsOf(argsJson, tool);
    if (auto *error = std::get_if<std::string>(&parsed))
    {
        return std::move(*error);
    }
    const Json &args = std::get<Json>(parsed);
    for (const auto &[key, value] : args.items())
    {
        if (key != field)
        {
            return std::format("{}: unknown argument `{}`: the one argument is `{}`, true or false.", tool, key, field);
        }
    }
    if (args.empty())
    {
        return std::optional<bool>{};
    }
    const Json &value = args[std::string(field)];
    if (!value.is_boolean())
    {
        return std::format("{}: `{}` is true or false, or left out to flip.", tool, field);
    }
    return std::optional(value.get<bool>());
}

std::variant<PoseRequest, std::string> ParsePose(std::string_view argsJson)
{
    auto parsed = ArgsOf(argsJson, kPose);
    if (auto *error = std::get_if<std::string>(&parsed))
    {
        return std::move(*error);
    }
    const Json &args = std::get<Json>(parsed);
    if (args.empty())
    {
        return std::string("photocam_pose: give at least one of eye, facingDegrees, pitchDegrees, lookAt, "
                           "fovDegrees and speed.");
    }
    PoseRequest request;
    for (const auto &[key, value] : args.items())
    {
        if (std::string error = ReadPoseArgument(request, key, value); !error.empty())
        {
            return error;
        }
    }
    if (std::string error = CheckPose(request); !error.empty())
    {
        return error;
    }
    return request;
}

Vec3 AimOf(const PoseRequest &request, Vec3 found)
{
    const bool thing = request.lookAt && std::holds_alternative<ObjectId>(*request.lookAt);
    found.z += request.height.value_or(thing ? kThingHeight : 0.0f);
    return found;
}

std::string Apply(FlyCamera &fly, const PoseRequest &request, std::optional<Vec3> aim)
{
    Pose pose = fly.Current();
    pose.eye = request.eye.value_or(pose.eye);
    pose.heading = request.facingDegrees ? HeadingOf(*request.facingDegrees) : pose.heading;
    pose.pitch = request.pitchDegrees ? *request.pitchDegrees / kDegrees : pose.pitch;
    if (aim && request.distance)
    {
        // Within the limits first, so that the placed view looks at the aim.
        pose.pitch = std::clamp(pose.pitch, -fly.Limits().maxPitch, fly.Limits().maxPitch);
        pose.eye = *aim + (FlyCamera::Forward(pose) * -*request.distance);
    }
    else if (aim)
    {
        const Vec3 to{aim->x - pose.eye.x, aim->y - pose.eye.y, aim->z - pose.eye.z};
        const float level = std::hypot(to.x, to.y);
        if (std::hypot(level, to.z) < 0.01f)
        {
            return "photocam_pose: the eye is at `lookAt`, so there is no way to face it: give `distance`, or move "
                   "the eye.";
        }
        pose.heading = std::atan2(to.y, to.x);
        pose.pitch = std::atan2(to.z, level);
    }
    fly.Place(pose);
    if (request.fovDegrees)
    {
        fly.SetFov(*request.fovDegrees);
    }
    if (request.speed)
    {
        fly.SetSpeed(*request.speed);
    }
    return "";
}

std::variant<MoveRequest, std::string> ParseMove(std::string_view argsJson)
{
    auto parsed = ArgsOf(argsJson, kMove);
    if (auto *error = std::get_if<std::string>(&parsed))
    {
        return std::move(*error);
    }
    const Json &args = std::get<Json>(parsed);
    if (args.empty())
    {
        return std::string("photocam_move: give at least one of forward, right and up, in metres.");
    }
    MoveRequest request;
    for (const auto &[key, value] : args.items())
    {
        float *field = nullptr;
        if (key == "forward")
        {
            field = &request.forward;
        }
        else if (key == "right")
        {
            field = &request.right;
        }
        else if (key == "up")
        {
            field = &request.up;
        }
        if (field == nullptr)
        {
            return std::format("photocam_move: unknown argument `{}`: the arguments are forward, right and up, in "
                               "metres.",
                               key);
        }
        const auto number = FiniteOf(value);
        if (!number)
        {
            return std::format("photocam_move: `{}` is a finite number of metres.", key);
        }
        *field = *number;
    }
    return request;
}

void Apply(FlyCamera &fly, const MoveRequest &request)
{
    Pose pose = fly.Current();
    pose.eye = pose.eye + (FlyCamera::Forward(pose) * request.forward) + (FlyCamera::Right(pose) * request.right) +
               Vec3{0.0f, 0.0f, request.up};
    fly.Place(pose);
}

std::string Refusal(Toggled toggled)
{
    switch (toggled)
    {
    case Toggled::NoCamera:
        return "photocam_toggle: there is no camera to take over: photo cam starts in explore or combat mode. Close "
               "the menu, or "
               "wait for the conversation, cutscene or load to end, and call again.";
    case Toggled::Tactical:
        return "photocam_toggle: the camera is in the tactical view, which photo cam leaves alone. Zoom into the close "
               "view first "
               "(Eluvian's `camera` tool, action zoom, 0.5 to 1), then call again.";
    case Toggled::Started:
    case Toggled::Stopped:
        break;
    }
    return "";
}

std::string StartedJson(const FlyCamera &fly, std::string_view by)
{
    Json payload = PoseJson(fly);
    payload["by"] = by;
    return payload.dump();
}

std::string StoppedJson(std::string_view by)
{
    return Json{{"by", by}}.dump();
}

std::string LockedJson(bool locked, std::string_view by)
{
    return Json{{"locked", locked}, {"by", by}}.dump();
}

} // namespace photocam::tools
