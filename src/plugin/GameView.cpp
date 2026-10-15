#include "plugin/GameView.h"

#include "dao/Camera.h"
#include "dao/Input.h"
#include "dao/generated/ScriptApi.h"

#include <cmath>
#include <cstdint>
#include <numbers>
#include <optional>

namespace photocam::game
{

namespace
{

constexpr float kPi = std::numbers::pi_v<float>;

// The field of view taken if the game's cannot be read, the scene not yet drawn.
constexpr float kFallbackFov = 60.0f;

// What photo cam withholds of the game's reaction to the keys and mouse while it flies: the
// party's walking, the camera's turning (the right mouse's orbit among it), its zoom and its pan.
constexpr std::uint32_t kFlying =
    dao::input::kMovement | dao::input::kCameraTurn | dao::input::kCameraZoom | dao::input::kCameraPan;

// The field of view last held, so it is set on a change and not every frame.
std::optional<float> g_heldFov;

// The game's fly camera (dao/Camera.h, DAOLib #125) measures its yaw from +y, a quarter turn
// behind the core's heading from +x: seen in play 2026-10-15, the close view looking along +x
// read as yaw -90, yaw 0 drew a view along +y and yaw -45 one between them. Its pitch is the
// core's, up positive (30 drew the sky).
constexpr float kYawToHeading = kPi / 2.0f;

Pose FromView(const dao::camera::View &view)
{
    return {.eye = {view.eye.x, view.eye.y, view.eye.z}, .heading = view.yaw + kYawToHeading, .pitch = view.pitch};
}

dao::camera::View ToView(const Pose &pose)
{
    return {.eye = {pose.eye.x, pose.eye.y, pose.eye.z}, .yaw = pose.heading - kYawToHeading, .pitch = pose.pitch};
}

} // namespace

bool Available()
{
    return true;
}

GameCamera Camera()
{
    GameCamera game;
    dao::CameraController *controller = dao::camera::Current();
    const auto view = dao::camera::CurrentView();
    if (controller == nullptr || !view)
    {
        return game;
    }
    game.present = true;
    game.tactical = controller->Tactical();
    game.view = FromView(*view);
    game.fovDegrees = dao::camera::FieldOfView().value_or(kFallbackFov);
    return game;
}

bool Draw(const Pose &pose, float fovDegrees)
{
    if (!dao::camera::SetFreeView(ToView(pose)))
    {
        return false;
    }
    if (!g_heldFov || std::abs(*g_heldFov - fovDegrees) > 0.001f)
    {
        dao::camera::SetFieldOfView(fovDegrees);
        g_heldFov = fovDegrees;
    }
    return true;
}

// As Eluvian finds a thing's position (eluvian-agent, 2026-10-15): the scripts' GetPosition.
std::optional<Vec3> PositionOf(std::uint32_t id)
{
    if (dao::script::api::IsObjectValid(id) == 0)
    {
        return std::nullopt;
    }
    const dao::Vector at = dao::script::api::GetPosition(id);
    return Vec3{at.x, at.y, at.z};
}

void Release()
{
    dao::camera::SetFreeView(std::nullopt);
    dao::camera::SetFieldOfView(std::nullopt);
    g_heldFov.reset();
}

FlyInput ReadInput(float seconds)
{
    using dao::input::Down;
    using dao::input::Key;
    const auto mouse = dao::input::MouseDelta();
    FlyInput input;
    input.seconds = seconds;
    input.look = Down(dao::input::MouseButton::Right);
    input.mouseX = static_cast<float>(mouse.x);
    input.mouseY = static_cast<float>(mouse.y);
    input.forward = Down(Key::W);
    input.back = Down(Key::S);
    input.left = Down(Key::A);
    input.right = Down(Key::D);
    input.wheel = dao::input::WheelNotches();
    input.shift = Down(Key::LeftShift) || Down(Key::RightShift);
    return input;
}

void Withhold(bool withheld)
{
    dao::input::Withhold(withheld ? kFlying : 0);
}

} // namespace photocam::game
