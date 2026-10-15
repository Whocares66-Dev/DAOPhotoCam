#include "plugin/GameView.h"

#include "dao/Camera.h"
#include "dao/ClientExoApp.h"
#include "dao/GameModes.h"
#include "dao/Input.h"

#include <spdlog/spdlog.h>

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

// The game changes its mode on a frame after it is asked (seen 2026-10-15: F8's callback ran
// before the frame's, which still found explore mode), so the free view is given a few frames to
// become the game's mode before Free() says it has been left.
constexpr int kFramesToEnter = 10;
int g_entering = 0;

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
    const bool entering = !g_heldFov;
    if (!dao::camera::SetFreeView(ToView(pose)))
    {
        return false;
    }
    if (entering)
    {
        g_entering = kFramesToEnter;
    }
    if (!g_heldFov || std::abs(*g_heldFov - fovDegrees) > 0.001f)
    {
        dao::camera::SetFieldOfView(fovDegrees);
        g_heldFov = fovDegrees;
    }
    return true;
}

bool Free()
{
    if (dao::CClientExoApp::CurrentMode() == dao::GameMode::FlyCam)
    {
        g_entering = 0;
        return true;
    }
    return g_entering-- > 0;
}

void LogAxes(const Pose &pose)
{
    const auto axes = dao::camera::Axes();
    if (!axes)
    {
        return;
    }
    const float level = std::cos(pose.pitch);
    spdlog::info("photo cam: drawn forward ({:.3f}, {:.3f}, {:.3f}), the pose's ({:.3f}, {:.3f}, {:.3f}), eye ({:.2f}, "
                 "{:.2f}, {:.2f}), heading {:.1f}, pitch {:.1f}",
                 axes->forward.x, axes->forward.y, axes->forward.z, level * std::cos(pose.heading),
                 level * std::sin(pose.heading), std::sin(pose.pitch), pose.eye.x, pose.eye.y, pose.eye.z,
                 pose.heading * (180.0f / kPi), pose.pitch * (180.0f / kPi));
}

void Release()
{
    dao::camera::SetFreeView(std::nullopt);
    dao::camera::SetFieldOfView(std::nullopt);
    g_heldFov.reset();
    g_entering = 0;
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
