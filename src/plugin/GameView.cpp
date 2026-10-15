#include "plugin/GameView.h"

#include "dao/Camera.h"
#include "dao/Input.h"

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

} // namespace

bool Available()
{
    // DAOLib #125, the free view the scene is drawn from, is not in the submodule yet; the
    // field of view (#126) and the input (#127) are.
    return false;
}

// The game's scene camera keeps its yaw as the direction from the focus to the camera, and
// its pitch from straight above the pivot (dao/Camera.h). The camera looks back along the
// first, so the view's heading is the yaw turned half round; and a camera at the pitch p
// from above looks down by p less a quarter turn: 90 degrees, level, looks along the horizon.
// To be checked in play against the camera's own view when the free view lands.
GameCamera Camera()
{
    GameCamera game;
    dao::CameraController *controller = dao::camera::Current();
    if (controller == nullptr || controller->Scene() == nullptr)
    {
        return game;
    }
    const dao::SceneCamera *scene = controller->Scene();
    const dao::Vector eye = scene->Position();
    game.present = true;
    game.tactical = controller->Tactical();
    game.view.eye = {eye.x, eye.y, eye.z};
    game.view.heading = scene->Yaw() + kPi;
    game.view.pitch = scene->Pitch() - (kPi / 2.0f);
    game.fovDegrees = dao::camera::FieldOfView().value_or(kFallbackFov);
    return game;
}

void Draw(const Pose & /*pose*/, float fovDegrees)
{
    // The pose waits on DAOLib #125.
    if (!g_heldFov || std::abs(*g_heldFov - fovDegrees) > 0.001f)
    {
        dao::camera::SetFieldOfView(fovDegrees);
        g_heldFov = fovDegrees;
    }
}

void Release()
{
    // The pose waits on DAOLib #125.
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
