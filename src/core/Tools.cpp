#include "core/Tools.h"

#include <nlohmann/json.hpp>

#include <cmath>
#include <format>
#include <numbers>

namespace photocam::tools
{

namespace
{

using Json = nlohmann::json;

constexpr float kDegrees = 180.0f / std::numbers::pi_v<float>;

Json VecJson(const Vec3 &v)
{
    return Json{{"x", v.x}, {"y", v.y}, {"z", v.z}};
}

Json PoseJson(const FlyCamera &fly)
{
    const Pose &pose = fly.Current();
    return Json{{"eye", VecJson(pose.eye)},
                {"headingDegrees", pose.heading * kDegrees},
                {"pitchDegrees", pose.pitch * kDegrees},
                {"fovDegrees", fly.FovDegrees()},
                {"speed", fly.Speed()}};
}

// The agent's arguments as an object; an empty text is no arguments.
std::variant<Json, std::string> ArgsOf(std::string_view argsJson)
{
    if (argsJson.find_first_not_of(" \t\r\n") == std::string_view::npos)
    {
        return Json::object();
    }
    Json args = Json::parse(argsJson, nullptr, false);
    if (args.is_discarded() || !args.is_object())
    {
        return std::string("The arguments are not a JSON object.");
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

// The request's number an argument names, null for a name that is none of them.
std::optional<float> *NumberOf(PoseRequest &request, std::string_view name)
{
    if (name == "headingDegrees")
    {
        return &request.headingDegrees;
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
    return nullptr;
}

Json NumberProperty(const char *description)
{
    return Json{{"type", "number"}, {"description", description}};
}

} // namespace

std::string StateDescriptor()
{
    return Json{{"description",
                 "Photo cam's state: whether this build can fly it (`available`), whether it is on and locked, and "
                 "while on, where the view is (`eye`), the way it looks (`headingDegrees` from +x toward +y, "
                 "`pitchDegrees` above the horizon), its field of view and its speed in metres a second."},
                {"inputSchema", {{"type", "object"}, {"properties", Json::object()}}}}
        .dump();
}

std::string ToggleDescriptor()
{
    return Json{{"description",
                 "Turns photo cam, the free camera, on or off, as its key does (F8 by default). It starts from the "
                 "game's camera in the close view, in explore or combat mode, and not in the tactical view; off "
                 "gives the game back its camera where it was. Answers photo cam's state. photocam.started and "
                 "photocam.stopped follow."},
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
                 "view hold, and the game has its keys and mouse back, so the party can be moved into the shot. "
                 "photocam_pose still places a locked view. Only while photo cam is on. Answers photo cam's state; "
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
    return Json{{"description",
                 "Puts photo cam's view exactly: where it is, the way it looks, its field of view and its speed. "
                 "What is left out stays as it is; each value is kept within photo cam's limits, and the answer, "
                 "photo cam's state, says what was taken. Only while photo cam is on (photocam_toggle). The new "
                 "view is drawn from the next frame: take a screenshot after this call answers."},
                {"inputSchema",
                 {{"type", "object"},
                  {"properties",
                   {{"eye",
                     {{"description", "Where the view is, a world point: {x, y, z} or [x, y, z], metres, z "
                                      "up."}}},
                    {"headingDegrees", NumberProperty("The way the view looks, in degrees from +x toward +y.")},
                    {"pitchDegrees",
                     NumberProperty("How far above the horizon the view looks, -89 to 89; negative looks down.")},
                    {"fovDegrees", NumberProperty("The field of view, 30 to 150 degrees.")},
                    {"speed", NumberProperty("The flying speed, 0.5 to 40 metres a second.")}}}}}}
        .dump();
}

std::vector<EventDescription> Events()
{
    return {
        {kStarted, Json{{"description", "Photo cam turned on, from the game's camera."},
                        {"fields",
                         {{"by", "key or tool"},
                          {"eye", "where the view is"},
                          {"headingDegrees", "the way it looks, from +x toward +y"},
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

std::variant<std::optional<bool>, std::string> ParseSwitch(std::string_view argsJson, std::string_view field)
{
    auto parsed = ArgsOf(argsJson);
    if (auto *error = std::get_if<std::string>(&parsed))
    {
        return std::move(*error);
    }
    const Json &args = std::get<Json>(parsed);
    for (const auto &[key, value] : args.items())
    {
        if (key != field)
        {
            return std::format("Unknown argument `{}`: the one argument is `{}`, true or false.", key, field);
        }
    }
    if (args.empty())
    {
        return std::optional<bool>{};
    }
    const Json &value = args[std::string(field)];
    if (!value.is_boolean())
    {
        return std::format("`{}` is true or false, or left out to flip.", field);
    }
    return std::optional(value.get<bool>());
}

std::variant<PoseRequest, std::string> ParsePose(std::string_view argsJson)
{
    auto parsed = ArgsOf(argsJson);
    if (auto *error = std::get_if<std::string>(&parsed))
    {
        return std::move(*error);
    }
    const Json &args = std::get<Json>(parsed);
    if (args.empty())
    {
        return std::string("Give at least one of eye, headingDegrees, pitchDegrees, fovDegrees and speed.");
    }
    PoseRequest request;
    for (const auto &[key, value] : args.items())
    {
        if (key == "eye")
        {
            request.eye = PointOf(value);
            if (!request.eye)
            {
                return std::string("`eye` is a world point of three finite numbers: {x, y, z} or [x, y, z].");
            }
            continue;
        }
        std::optional<float> *number = NumberOf(request, key);
        if (number == nullptr)
        {
            return std::format("Unknown argument `{}`: the arguments are eye, headingDegrees, pitchDegrees, "
                               "fovDegrees and speed.",
                               key);
        }
        *number = FiniteOf(value);
        if (!*number)
        {
            return std::format("`{}` is a finite number.", key);
        }
    }
    return request;
}

void Apply(FlyCamera &fly, const PoseRequest &request)
{
    Pose pose = fly.Current();
    pose.eye = request.eye.value_or(pose.eye);
    pose.heading = request.headingDegrees ? *request.headingDegrees / kDegrees : pose.heading;
    pose.pitch = request.pitchDegrees ? *request.pitchDegrees / kDegrees : pose.pitch;
    fly.Place(pose);
    if (request.fovDegrees)
    {
        fly.SetFov(*request.fovDegrees);
    }
    if (request.speed)
    {
        fly.SetSpeed(*request.speed);
    }
}

std::string Refusal(Toggled toggled)
{
    switch (toggled)
    {
    case Toggled::NoCamera:
        return "There is no camera to take over: photo cam starts in explore or combat mode. Close the menu, or "
               "wait for the conversation, cutscene or load to end, and call again.";
    case Toggled::Tactical:
        return "The camera is in the tactical view, which photo cam leaves alone. Zoom into the close view first "
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
