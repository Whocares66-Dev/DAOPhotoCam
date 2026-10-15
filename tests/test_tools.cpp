#include "core/Tools.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <nlohmann/json.hpp>

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
    CHECK(state["headingDegrees"].get<double>() == Approx(90.0));
    CHECK(state["fovDegrees"].get<double>() == Approx(50.0));
    CHECK(state.contains("speed"));
}

TEST_CASE("A switch flips with no arguments and is set by its one argument", "[tools]")
{
    using Switch = std::optional<bool>;
    CHECK(std::get<Switch>(tools::ParseSwitch("", "on")) == std::nullopt);
    CHECK(std::get<Switch>(tools::ParseSwitch("{}", "on")) == std::nullopt);
    CHECK(std::get<Switch>(tools::ParseSwitch(R"({"on": true})", "on")) == Switch(true));
    CHECK(std::get<Switch>(tools::ParseSwitch(R"({"locked": false})", "locked")) == Switch(false));

    CHECK(std::holds_alternative<std::string>(tools::ParseSwitch(R"({"on": 1})", "on")));
    CHECK(std::holds_alternative<std::string>(tools::ParseSwitch(R"({"on": true, "off": true})", "on")));
    CHECK(std::holds_alternative<std::string>(tools::ParseSwitch(R"({"off": true, "on": true})", "on")));
    CHECK(std::holds_alternative<std::string>(tools::ParseSwitch("[true]", "on")));
    CHECK(std::holds_alternative<std::string>(tools::ParseSwitch("{", "on")));
}

TEST_CASE("A pose takes a point either way, and only what is given", "[tools][pose]")
{
    const auto asObject = PoseOf(R"({"eye": {"x": 1, "y": 2, "z": 3}})");
    CHECK(asObject.eye == photocam::Vec3{1.0f, 2.0f, 3.0f});
    CHECK_FALSE(asObject.headingDegrees);
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
    CHECK_THAT(ErrorOf(tools::ParsePose(R"({"headingDegrees": "north"})")), ContainsSubstring("`headingDegrees`"));
    CHECK_THAT(ErrorOf(tools::ParsePose("nonsense")), ContainsSubstring("JSON object"));
}

TEST_CASE("Applying a pose places the view within the limits and keeps the rest", "[tools][pose]")
{
    FlyCamera fly(Pose{{1.0f, 2.0f, 3.0f}, 0.5f, 0.1f}, 60.0f, 4.0f);
    tools::Apply(fly, PoseOf(R"({"headingDegrees": 180, "pitchDegrees": -120, "fovDegrees": 500})"));
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

TEST_CASE("Each tool's descriptor is JSON Eluvian takes", "[tools]")
{
    for (const std::string &descriptor :
         {tools::StateDescriptor(), tools::ToggleDescriptor(), tools::LockDescriptor(), tools::PoseDescriptor()})
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
