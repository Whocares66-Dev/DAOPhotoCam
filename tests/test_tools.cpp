#include "core/Tools.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <nlohmann/json.hpp>

#include <format>
#include <numbers>
#include <string>

using Catch::Approx;
using nlohmann::json;
using photocam::FlyCamera;
using photocam::GameCamera;
using photocam::Pose;
using photocam::Session;
namespace tools = photocam::tools;

namespace
{

GameCamera Close()
{
    GameCamera game;
    game.present = true;
    game.view.eye = {1.0f, 2.0f, 3.0f};
    game.view.heading = std::numbers::pi_v<float> / 2.0f;
    game.fovDegrees = 50.0f;
    return game;
}

tools::PoseRequest PoseOf(std::string_view args)
{
    const auto parsed = tools::ParsePose(args);
    REQUIRE(std::holds_alternative<tools::PoseRequest>(parsed));
    return std::get<tools::PoseRequest>(parsed);
}

std::string ErrorOf(const std::variant<tools::PoseRequest, std::string> &parsed)
{
    REQUIRE(std::holds_alternative<std::string>(parsed));
    return std::get<std::string>(parsed);
}

} // namespace

TEST_CASE("The state says off, then on with the pose in degrees", "[tools]")
{
    Session session;
    auto state = json::parse(tools::StateJson(session, false));
    CHECK(state == json{{"available", false}, {"on", false}, {"locked", false}});

    session.Toggle(Close());
    session.Lock();
    state = json::parse(tools::StateJson(session, true));
    CHECK(state["on"] == true);
    CHECK(state["locked"] == true);
    CHECK(state["eye"] == json{{"x", 1.0}, {"y", 2.0}, {"z", 3.0}});
    // Looking along +y is the game's facing 180 (or -180).
    CHECK(std::abs(state["facingDegrees"].get<double>()) == Approx(180.0));
    CHECK(state["fovDegrees"].get<double>() == Approx(50.0));
    CHECK(state.contains("speed"));
}

TEST_CASE("A switch flips with no arguments and is set by its one argument", "[tools]")
{
    using Switch = std::optional<bool>;
    CHECK(std::get<Switch>(tools::ParseSwitch("", tools::kToggle, "on")) == std::nullopt);
    CHECK(std::get<Switch>(tools::ParseSwitch("{}", tools::kToggle, "on")) == std::nullopt);
    CHECK(std::get<Switch>(tools::ParseSwitch(R"({"on": true})", tools::kToggle, "on")) == Switch(true));
    CHECK(std::get<Switch>(tools::ParseSwitch(R"({"locked": false})", tools::kLock, "locked")) == Switch(false));

    CHECK(std::holds_alternative<std::string>(tools::ParseSwitch(R"({"on": 1})", tools::kToggle, "on")));
    CHECK(
        std::holds_alternative<std::string>(tools::ParseSwitch(R"({"on": true, "off": true})", tools::kToggle, "on")));
    CHECK(
        std::holds_alternative<std::string>(tools::ParseSwitch(R"({"off": true, "on": true})", tools::kToggle, "on")));
    CHECK(std::holds_alternative<std::string>(tools::ParseSwitch("[true]", tools::kToggle, "on")));
    CHECK(std::holds_alternative<std::string>(tools::ParseSwitch("{", tools::kToggle, "on")));
}

TEST_CASE("A pose takes a point either way, and only what is given", "[tools][pose]")
{
    const auto asObject = PoseOf(R"({"eye": {"x": 1, "y": 2, "z": 3}})");
    CHECK(asObject.eye == photocam::Vec3{1.0f, 2.0f, 3.0f});
    CHECK_FALSE(asObject.facingDegrees);
    CHECK_FALSE(asObject.fovDegrees);

    const auto asArray = PoseOf(R"({"eye": [4, 5, 6], "fovDegrees": 90, "speed": 2.5})");
    CHECK(asArray.eye == photocam::Vec3{4.0f, 5.0f, 6.0f});
    CHECK(asArray.fovDegrees == 90.0f);
    CHECK(asArray.speed == 2.5f);
}

TEST_CASE("A pose's mistakes are named for the agent", "[tools][pose]")
{
    using Catch::Matchers::ContainsSubstring;
    CHECK_THAT(ErrorOf(tools::ParsePose("{}")), ContainsSubstring("at least one"));
    CHECK_THAT(ErrorOf(tools::ParsePose(R"({"fov": 90})")), ContainsSubstring("`fov`"));
    CHECK_THAT(ErrorOf(tools::ParsePose(R"({"eye": [1, 2]})")), ContainsSubstring("`eye`"));
    CHECK_THAT(ErrorOf(tools::ParsePose(R"({"eye": {"x": 1, "y": 2}})")), ContainsSubstring("`eye`"));
    CHECK_THAT(ErrorOf(tools::ParsePose(R"({"eye": [1, "2", 3]})")), ContainsSubstring("`eye`"));
    CHECK_THAT(ErrorOf(tools::ParsePose(R"({"facingDegrees": "north"})")), ContainsSubstring("`facingDegrees`"));
    CHECK_THAT(ErrorOf(tools::ParsePose("nonsense")), ContainsSubstring("JSON object"));
}

TEST_CASE("Applying a pose places the view within the limits and keeps the rest", "[tools][pose]")
{
    FlyCamera fly(Pose{{1.0f, 2.0f, 3.0f}, 0.5f, 0.1f}, 60.0f, 4.0f);
    tools::Apply(fly, PoseOf(R"({"facingDegrees": 90, "pitchDegrees": -120, "fovDegrees": 500})"));
    CHECK(fly.Current().eye == photocam::Vec3{1.0f, 2.0f, 3.0f});
    CHECK(std::abs(fly.Current().heading) == Approx(std::numbers::pi_v<float>));
    CHECK(fly.Current().pitch == Approx(-fly.Limits().maxPitch));
    CHECK(fly.FovDegrees() == fly.Limits().maxFov);
    CHECK(fly.Speed() == 4.0f);

    tools::Apply(fly, PoseOf(R"({"eye": [7, 8, 9], "speed": 0.01})"));
    CHECK(fly.Current().eye == photocam::Vec3{7.0f, 8.0f, 9.0f});
    CHECK(fly.Speed() == fly.Limits().minSpeed);
    CHECK(fly.FovDegrees() == fly.Limits().maxFov);
}

TEST_CASE("lookAt takes a point, or an id in each of Eluvian's three ways", "[tools][pose]")
{
    const auto targetOf = [](std::string_view args) {
        const auto request = PoseOf(args);
        REQUIRE(request.lookAt.has_value());
        return request.lookAt.value_or(tools::Target{});
    };
    CHECK(std::get<photocam::Vec3>(targetOf(R"({"lookAt": [1, 2, 3]})")) == photocam::Vec3{1.0f, 2.0f, 3.0f});
    CHECK(std::get<tools::ObjectId>(targetOf(R"({"lookAt": "0x7d9"})")) == 2009u);
    CHECK(std::get<tools::ObjectId>(targetOf(R"({"lookAt": "2009"})")) == 2009u);
    CHECK(std::get<tools::ObjectId>(targetOf(R"({"lookAt": 2009})")) == 2009u);
}

TEST_CASE("lookAt names what it takes when given something else", "[tools][pose]")
{
    using Catch::Matchers::ContainsSubstring;
    for (const char *args : {R"({"lookAt": "0x"})", R"({"lookAt": "Lyna"})", R"({"lookAt": -1})",
                             R"({"lookAt": "0x7d9z"})", R"({"lookAt": [1, 2]})"})
    {
        CHECK_THAT(ErrorOf(tools::ParsePose(args)), ContainsSubstring("`lookAt`"));
    }
}

TEST_CASE("lookAt's arguments that do not go together are named", "[tools][pose]")
{
    using Catch::Matchers::ContainsSubstring;
    CHECK_THAT(ErrorOf(tools::ParsePose(R"({"distance": 3})")), ContainsSubstring("go with `lookAt`"));
    CHECK_THAT(ErrorOf(tools::ParsePose(R"({"height": 1})")), ContainsSubstring("go with `lookAt`"));
    CHECK_THAT(ErrorOf(tools::ParsePose(R"({"lookAt": 5, "distance": 0})")), ContainsSubstring("0.1"));
    CHECK_THAT(ErrorOf(tools::ParsePose(R"({"lookAt": 5, "distance": 3, "eye": [0, 0, 0]})")),
               ContainsSubstring("leave out `eye`"));
    CHECK_THAT(ErrorOf(tools::ParsePose(R"({"lookAt": 5, "facingDegrees": 90})")), ContainsSubstring("leave them out"));
    CHECK(ErrorOf(tools::ParsePose(R"({"fov": 90})")).starts_with("photocam_pose: "));
}

TEST_CASE("A thing is aimed at above its feet, a point at itself, unless a height is given", "[tools][pose]")
{
    const photocam::Vec3 feet{1.0f, 2.0f, 3.0f};
    CHECK(tools::AimOf(PoseOf(R"({"lookAt": 7})"), feet).z == Approx(3.0f + tools::kThingHeight));
    CHECK(tools::AimOf(PoseOf(R"({"lookAt": [1, 2, 3]})"), feet).z == Approx(3.0f));
    CHECK(tools::AimOf(PoseOf(R"({"lookAt": 7, "height": 0.5})"), feet).z == Approx(3.5f));
}

TEST_CASE("lookAt alone turns the view from the eye to face the aim", "[tools][pose]")
{
    FlyCamera fly(Pose{{0.0f, 0.0f, 0.0f}, 0.0f, 0.0f}, 60.0f);
    const auto request = PoseOf(R"({"lookAt": [0, 10, 10]})");
    CHECK(tools::Apply(fly, request, tools::AimOf(request, {0.0f, 10.0f, 10.0f})).empty());
    CHECK(fly.Current().eye == photocam::Vec3{});
    CHECK(fly.Current().heading == Approx(std::numbers::pi_v<float> / 2.0f));
    CHECK(fly.Current().pitch == Approx(std::numbers::pi_v<float> / 4.0f));

    const auto onIt = PoseOf(R"({"lookAt": [0, 0, 0]})");
    CHECK_FALSE(tools::Apply(fly, onIt, photocam::Vec3{}).empty());
}

TEST_CASE("lookAt with a distance puts the eye that far back along the facing", "[tools][pose]")
{
    FlyCamera fly(Pose{{50.0f, 50.0f, 50.0f}, 1.0f, 0.3f}, 60.0f);
    const auto request = PoseOf(R"({"lookAt": [10, 20, 0], "distance": 4, "facingDegrees": -90, "pitchDegrees": 0})");
    CHECK(tools::Apply(fly, request, tools::AimOf(request, {10.0f, 20.0f, 0.0f})).empty());
    CHECK(fly.Current().eye.x == Approx(6.0f));
    CHECK(fly.Current().eye.y == Approx(20.0f));
    CHECK(fly.Current().eye.z == Approx(0.0f).margin(1e-5));

    // Kept facing and pitch: the aim is ahead of the placed eye, at the distance.
    const auto kept = PoseOf(R"({"lookAt": [0, 0, 0], "distance": 5, "pitchDegrees": -30})");
    CHECK(tools::Apply(fly, kept, photocam::Vec3{}).empty());
    const photocam::Vec3 ahead = FlyCamera::Forward(fly.Current());
    CHECK(fly.Current().eye.x + (ahead.x * 5.0f) == Approx(0.0f).margin(1e-4));
    CHECK(fly.Current().eye.z + (ahead.z * 5.0f) == Approx(0.0f).margin(1e-4));
    CHECK(fly.Current().eye.z == Approx(2.5f));
}

TEST_CASE("A move steps along the view, to its right and up, and does not turn", "[tools][move]")
{
    FlyCamera fly(Pose{{0.0f, 0.0f, 0.0f}, 0.0f, 0.0f}, 60.0f);
    const auto parsed = tools::ParseMove(R"({"forward": 2, "right": 1, "up": 3})");
    REQUIRE(std::holds_alternative<tools::MoveRequest>(parsed));
    tools::Apply(fly, std::get<tools::MoveRequest>(parsed));
    CHECK(fly.Current().eye.x == Approx(2.0f));
    CHECK(fly.Current().eye.y == Approx(-1.0f));
    CHECK(fly.Current().eye.z == Approx(3.0f));
    CHECK(fly.Current().heading == 0.0f);
}

TEST_CASE("A move's mistakes are named for the agent", "[tools][move]")
{
    using Catch::Matchers::ContainsSubstring;
    const auto error = [](std::string_view args) {
        const auto result = tools::ParseMove(args);
        REQUIRE(std::holds_alternative<std::string>(result));
        return std::get<std::string>(result);
    };
    CHECK_THAT(error("{}"), ContainsSubstring("at least one"));
    CHECK_THAT(error(R"({"back": 1})"), ContainsSubstring("`back`"));
    CHECK_THAT(error(R"({"up": "high"})"), ContainsSubstring("`up`"));
    CHECK(error("[]").starts_with("photocam_move: "));
}

TEST_CASE("Facing is the game's: 0 along -y, -90 along +x, clockwise from above", "[tools][pose]")
{
    FlyCamera fly(Pose{}, 60.0f);
    const auto along = [&](float facing) {
        tools::Apply(fly, PoseOf(std::format(R"({{"facingDegrees": {}, "pitchDegrees": 0}})", facing)));
        return FlyCamera::Forward(fly.Current());
    };
    CHECK(along(0.0f).y == Approx(-1.0f));
    CHECK(along(-90.0f).x == Approx(1.0f));
    CHECK(along(90.0f).x == Approx(-1.0f));
    CHECK(along(180.0f).y == Approx(1.0f));
    // eluvian-agent's reading: facing -51.4 looks along (0.78, -0.62).
    const photocam::Vec3 seen = along(-51.4f);
    CHECK(seen.x == Approx(0.78f).margin(0.01));
    CHECK(seen.y == Approx(-0.62f).margin(0.01));
}

TEST_CASE("The state answers the facing it was given", "[tools][pose]")
{
    Session session;
    session.Toggle(Close());
    REQUIRE(session.Fly() != nullptr);
    tools::Apply(*session.Fly(), PoseOf(R"({"facingDegrees": -51.4})"));
    CHECK(json::parse(tools::StateJson(session, true))["facingDegrees"].get<double>() == Approx(-51.4));
}

TEST_CASE("Each tool's descriptor is JSON Eluvian takes", "[tools]")
{
    for (const std::string &descriptor : {tools::StateDescriptor(), tools::ToggleDescriptor(), tools::LockDescriptor(),
                                          tools::PoseDescriptor(), tools::MoveDescriptor()})
    {
        const auto parsed = json::parse(descriptor);
        CHECK(parsed["description"].is_string());
        CHECK(parsed["inputSchema"]["type"] == "object");
    }
}

TEST_CASE("Each event is described under the plugin's prefix", "[tools]")
{
    for (const auto &event : tools::Events())
    {
        CHECK(std::string(event.topic).starts_with("photocam."));
        const auto parsed = json::parse(event.descriptor);
        CHECK(parsed["description"].is_string());
        CHECK(parsed["fields"].is_object());
    }
}

TEST_CASE("The events' payloads say who did it", "[tools]")
{
    CHECK(json::parse(tools::StoppedJson("mode")) == json{{"by", "mode"}});
    CHECK(json::parse(tools::LockedJson(true, "tool")) == json{{"locked", true}, {"by", "tool"}});
    const FlyCamera fly(Pose{}, 60.0f);
    CHECK(json::parse(tools::StartedJson(fly, "key"))["by"] == "key");
}

TEST_CASE("Only a refused toggle has a sentence", "[tools]")
{
    CHECK_FALSE(tools::Refusal(photocam::Toggled::NoCamera).empty());
    CHECK_FALSE(tools::Refusal(photocam::Toggled::Tactical).empty());
    CHECK(tools::Refusal(photocam::Toggled::Started).empty());
    CHECK(tools::Refusal(photocam::Toggled::Stopped).empty());
}
