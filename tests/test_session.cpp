#include "core/Session.h"

#include <catch2/catch_test_macros.hpp>

using photocam::GameCamera;
using photocam::Session;
using photocam::Toggled;

namespace
{

GameCamera Close()
{
    GameCamera game;
    game.present = true;
    game.view.eye = {10.0f, 20.0f, 5.0f};
    game.view.heading = 1.0f;
    game.view.pitch = -0.3f;
    game.fovDegrees = 50.0f;
    return game;
}

} // namespace

TEST_CASE("The toggle starts from the game's view in the close view, and stops", "[session]")
{
    Session session;
    REQUIRE(session.Toggle(Close()) == Toggled::Started);
    REQUIRE(session.Active());
    CHECK(session.Fly()->Current().eye == Close().view.eye);
    CHECK(session.Fly()->Current().heading == 1.0f);
    CHECK(session.Fly()->FovDegrees() == 50.0f);
    REQUIRE(session.TakenOver().has_value());
    CHECK(session.TakenOver().value_or(GameCamera{}).view.eye == Close().view.eye);

    // A second press stops, whatever the game's camera is by then.
    CHECK(session.Toggle(GameCamera{}) == Toggled::Stopped);
    CHECK_FALSE(session.Active());
    CHECK(session.Fly() == nullptr);
    CHECK_FALSE(session.TakenOver());
}

TEST_CASE("No start in the tactical view or with no camera", "[session]")
{
    Session session;
    CHECK(session.Toggle(GameCamera{}) == Toggled::NoCamera);
    auto tactical = Close();
    tactical.tactical = true;
    CHECK(session.Toggle(tactical) == Toggled::Tactical);
    CHECK_FALSE(session.Active());
}

TEST_CASE("End gives back the game's camera as taken over, once", "[session]")
{
    Session session;
    CHECK_FALSE(session.End());
    session.Toggle(Close());
    auto input = photocam::FlyInput{};
    input.seconds = 0.05f;
    input.forward = true;
    session.Fly()->Step(input);
    const auto ended = session.End();
    REQUIRE(ended.has_value());
    const GameCamera given = ended.value_or(GameCamera{});
    CHECK(given.view.eye == Close().view.eye);
    CHECK(given.fovDegrees == 50.0f);
    CHECK_FALSE(session.End());
}

TEST_CASE("The speed the player chose is kept for the next start; the view is not", "[session]")
{
    Session session;
    session.Toggle(Close());
    auto input = photocam::FlyInput{};
    input.wheel = 2;
    input.shift = false;
    session.Fly()->Step(input);
    const float chosen = session.Fly()->Speed();
    input.shift = true;
    session.Fly()->Step(input);
    session.Toggle(Close());

    session.Toggle(Close());
    CHECK(session.Fly()->Speed() == chosen);
    CHECK(session.Fly()->FovDegrees() == 50.0f);
}

TEST_CASE("The lock holds the view while on, and each start is unlocked", "[session][lock]")
{
    Session session;
    CHECK(session.Lock() == photocam::Locked::Off);
    CHECK_FALSE(session.IsLocked());

    session.Toggle(Close());
    CHECK(session.Lock() == photocam::Locked::Locked);
    CHECK(session.IsLocked());
    CHECK(session.Lock() == photocam::Locked::Unlocked);
    CHECK_FALSE(session.IsLocked());

    session.Lock();
    CHECK(session.Toggle(Close()) == Toggled::Stopped);
    CHECK_FALSE(session.IsLocked());
    session.Toggle(Close());
    CHECK_FALSE(session.IsLocked());
}
