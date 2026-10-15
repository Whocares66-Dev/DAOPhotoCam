#include "plugin/GameView.h"

#include "dao/Camera.h"
#include "dao/CameraMoves.h"

#include <numbers>

namespace photocam::game
{

namespace
{

constexpr float kPi = std::numbers::pi_v<float>;

// The field of view assumed until DAOLib reads the game's (DAOLib #126).
constexpr float kAssumedFov = 60.0f;

} // namespace

bool Available()
{
    // DAOLib #125 (the free view), #126 (the field of view) and #127 (input) are not in the
    // submodule yet.
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
    game.fovDegrees = kAssumedFov;
    return game;
}

void Draw(const Pose & /*pose*/, float /*fovDegrees*/)
{
    // DAOLib #125, #126.
}

void Release()
{
    // DAOLib #125, #126.
}

FlyInput ReadInput(float seconds)
{
    // DAOLib #127.
    FlyInput input;
    input.seconds = seconds;
    return input;
}

void Withhold(bool /*withheld*/)
{
    // DAOLib #127.
}

} // namespace photocam::game
