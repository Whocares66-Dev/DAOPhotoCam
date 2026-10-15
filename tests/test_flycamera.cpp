#include "core/FlyCamera.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <numbers>

using Catch::Approx;
using photocam::FlyCamera;
using photocam::FlyInput;
using photocam::Pose;

namespace
{

constexpr float kPi = std::numbers::pi_v<float>;

// A view at the origin looking along +x, level.
FlyCamera Level(float speed = 4.0f, float fov = 60.0f)
{
    return FlyCamera(Pose{}, fov, speed);
}

FlyInput Second()
{
    FlyInput input;
    input.seconds = 0.05f;
    return input;
}

} // namespace

TEST_CASE("W flies along the view at the speed, S back", "[fly]")
{
    auto fly = Level(4.0f);
    auto input = Second();
    input.forward = true;
    fly.Step(input);
    CHECK(fly.Current().eye.x == Approx(0.2f));
    CHECK(fly.Current().eye.y == Approx(0.0f).margin(1e-6));

    input.forward = false;
    input.back = true;
    fly.Step(input);
    fly.Step(input);
    CHECK(fly.Current().eye.x == Approx(-0.2f));
}

TEST_CASE("W follows the pitch: looking up flies up", "[fly]")
{
    FlyCamera fly(Pose{{}, 0.0f, kPi / 4.0f}, 60.0f, 4.0f);
    auto input = Second();
    input.forward = true;
    fly.Step(input);
    CHECK(fly.Current().eye.x == Approx(0.2f * std::numbers::sqrt2_v<float> / 2.0f));
    CHECK(fly.Current().eye.z == Approx(0.2f * std::numbers::sqrt2_v<float> / 2.0f));
}

TEST_CASE("A and D move level to the side: looking along +x, right is -y", "[fly]")
{
    FlyCamera fly(Pose{{}, 0.0f, -kPi / 3.0f}, 60.0f, 4.0f);
    auto input = Second();
    input.right = true;
    fly.Step(input);
    CHECK(fly.Current().eye.y == Approx(-0.2f));
    CHECK(fly.Current().eye.z == Approx(0.0f).margin(1e-6));

    input.right = false;
    input.left = true;
    fly.Step(input);
    CHECK(fly.Current().eye.y == Approx(0.0f).margin(1e-6));
}

TEST_CASE("Diagonals are no faster than straight on; opposite keys cancel", "[fly]")
{
    auto fly = Level(4.0f);
    auto input = Second();
    input.forward = true;
    input.right = true;
    fly.Step(input);
    const auto &eye = fly.Current().eye;
    CHECK((eye.x * eye.x) + (eye.y * eye.y) == Approx(0.04f));

    auto still = Level();
    auto both = Second();
    both.forward = both.back = both.left = both.right = true;
    still.Step(both);
    CHECK(still.Current().eye == photocam::Vec3{});
}

TEST_CASE("A long frame moves no farther than the cap", "[fly]")
{
    auto fly = Level(10.0f);
    FlyInput input;
    input.seconds = 3.0f;
    input.forward = true;
    fly.Step(input);
    CHECK(fly.Current().eye.x == Approx(10.0f * fly.Limits().maxSeconds));
}

TEST_CASE("The mouse turns the view only with the right button held", "[fly][look]")
{
    auto fly = Level();
    auto input = Second();
    input.mouseX = 100.0f;
    input.mouseY = 100.0f;
    fly.Step(input);
    CHECK(fly.Current().heading == 0.0f);
    CHECK(fly.Current().pitch == 0.0f);

    input.look = true;
    fly.Step(input);
    // Rightward turns clockwise seen from above; toward the player looks down.
    CHECK(fly.Current().heading == Approx(-0.25f));
    CHECK(fly.Current().pitch == Approx(-0.25f));
}

TEST_CASE("The pitch stops short of straight up and down; the heading wraps", "[fly][look]")
{
    auto fly = Level();
    auto input = Second();
    input.look = true;
    input.mouseY = -100000.0f;
    fly.Step(input);
    CHECK(fly.Current().pitch == Approx(fly.Limits().maxPitch));
    input.mouseY = 100000.0f;
    fly.Step(input);
    CHECK(fly.Current().pitch == Approx(-fly.Limits().maxPitch));

    input.mouseY = 0.0f;
    input.mouseX = 3.0f * kPi / fly.Limits().radiansPerCount; // one and a half turns
    fly.Step(input);
    CHECK(fly.Current().heading >= -kPi);
    CHECK(fly.Current().heading <= kPi);
    CHECK(std::abs(fly.Current().heading) == Approx(kPi));
}

TEST_CASE("The wheel sets the speed within its limits", "[fly][wheel]")
{
    auto fly = Level(4.0f);
    auto input = Second();
    input.wheel = 1;
    fly.Step(input);
    CHECK(fly.Speed() == Approx(5.0f));
    input.wheel = -2;
    fly.Step(input);
    CHECK(fly.Speed() == Approx(3.2f));
    input.wheel = 100;
    fly.Step(input);
    CHECK(fly.Speed() == fly.Limits().maxSpeed);
    input.wheel = -100;
    fly.Step(input);
    CHECK(fly.Speed() == fly.Limits().minSpeed);
    CHECK(fly.FovDegrees() == 60.0f);
}

TEST_CASE("Shift and the wheel set the field of view, 30 to 150, up narrows it", "[fly][wheel]")
{
    auto fly = Level(4.0f, 60.0f);
    auto input = Second();
    input.shift = true;
    input.wheel = 1;
    fly.Step(input);
    CHECK(fly.FovDegrees() == Approx(55.0f));
    input.wheel = -3;
    fly.Step(input);
    CHECK(fly.FovDegrees() == Approx(70.0f));
    input.wheel = 100;
    fly.Step(input);
    CHECK(fly.FovDegrees() == 30.0f);
    input.wheel = -100;
    fly.Step(input);
    CHECK(fly.FovDegrees() == 150.0f);
    CHECK(fly.Speed() == 4.0f);
}

TEST_CASE("A start outside the limits is brought within them", "[fly]")
{
    FlyCamera fly(Pose{{1.0f, 2.0f, 3.0f}, 7.0f, 2.0f}, 10.0f, 1000.0f);
    CHECK(fly.FovDegrees() == 30.0f);
    CHECK(fly.Speed() == fly.Limits().maxSpeed);
    CHECK(fly.Current().pitch == Approx(fly.Limits().maxPitch));
    CHECK(fly.Current().heading == Approx(7.0f - (2.0f * kPi)));
    CHECK(fly.Current().eye == photocam::Vec3{1.0f, 2.0f, 3.0f});
}
