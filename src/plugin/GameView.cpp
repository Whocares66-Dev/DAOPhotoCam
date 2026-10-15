#include "plugin/GameView.h"

#include "dao/Camera.h"
#include "dao/ClientExoApp.h"
#include "dao/Gui.h"
#include "dao/Input.h"
#include "dao/generated/ScriptApi.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <optional>
#include <string>
#include <string_view>

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

// Photo cam's key map: the flight's actions answer their keys while it is on, and the game's
// actions on the same keys do not (dao/Input.h, DAOLib #130). The user, 2026-10-15: the keys
// read from the device until DAOLib let them share the game's, then rebindable actions.
constexpr std::wstring_view kForward = L"PhotoCamForward";
constexpr std::wstring_view kBack = L"PhotoCamBack";
constexpr std::wstring_view kLeft = L"PhotoCamLeft";
constexpr std::wstring_view kRight = L"PhotoCamRight";
constexpr std::wstring_view kUp = L"PhotoCamUp";
constexpr std::wstring_view kDown = L"PhotoCamDown";

// The field of view last held, so it is set on a change and not every frame.
std::optional<float> g_heldFov;

// Whether photo cam holds the cursor (HoldCursor), so DAOLib is called on a change only.
bool g_cursorHeld = false;

// Whether the game was paused when photo cam paused it, while photo cam is on.
std::optional<bool> g_pausedBefore;

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

// Whether a loading screen is still up, its fade among them (flag 0x10, dao/Gui.h's Screen). Started
// as a load's fade was ending, photo cam's pause held the fade over the view until it was off (seen
// 2026-10-15), so it does not start then.
static bool Loading()
{
    constexpr std::uint32_t kLoadingScreen = 0x10;
    const auto shown = dao::gui::ShownScreens();
    return std::ranges::any_of(shown,
                               [](const dao::gui::Screen &screen) { return (screen.flags & kLoadingScreen) != 0; });
}

GameCamera Camera()
{
    GameCamera game;
    dao::CameraController *controller = dao::camera::Current();
    const auto view = dao::camera::CurrentView();
    if (controller == nullptr || !view || Loading())
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

// Before the free view, as the engine's SetPaused updates the interface for modes 0 to 5 only
// (dao/Addresses.h) and the fly camera is mode 6.
void Freeze()
{
    dao::CClientExoApp *app = dao::CClientExoApp::Get();
    if (app == nullptr)
    {
        return;
    }
    g_pausedBefore = app->IsPaused();
    if (!*g_pausedBefore)
    {
        app->SetPaused(true);
    }
}

void Thaw()
{
    dao::CClientExoApp *app = dao::CClientExoApp::Get();
    if (app != nullptr && g_pausedBefore && !*g_pausedBefore)
    {
        app->SetPaused(false);
    }
    g_pausedBefore.reset();
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

bool RegisterMoves()
{
    struct Move
    {
        std::wstring_view name;
        const wchar_t *label;
        dao::input::Key key;
    };
    using dao::input::Key;
    for (const Move &move : {Move{kForward, L"Camera forward", Key::W}, Move{kBack, L"Camera back", Key::S},
                             Move{kLeft, L"Camera left", Key::A}, Move{kRight, L"Camera right", Key::D},
                             Move{kUp, L"Camera up", Key::Q}, Move{kDown, L"Camera down", Key::E}})
    {
        // Held, not pressed: the callback has nothing to do, and DAOLib asks for one.
        if (!dao::input::RegisterAction(
                {
                    .name = std::wstring(move.name),
                    .label = move.label,
                    .key = move.key,
                    .modes = {dao::GameMode::FlyCam},
                    .section = L"Photo Cam",
                    .keymap = std::wstring(kKeymap),
                },
                [] {}))
        {
            return false;
        }
    }
    return true;
}

// The mouse and the wheel stay read from the device: an action's binding to a mouse button
// does not count for Held, and the wheel is no key (dao/Input.h).
FlyInput ReadInput(float seconds)
{
    using dao::input::Down;
    using dao::input::Held;
    using dao::input::Key;
    const auto mouse = dao::input::MouseDelta();
    FlyInput input;
    input.seconds = seconds;
    input.look = Down(dao::input::MouseButton::Right);
    input.mouseX = static_cast<float>(mouse.x);
    input.mouseY = static_cast<float>(mouse.y);
    input.forward = Held(kForward);
    input.back = Held(kBack);
    input.left = Held(kLeft);
    input.right = Held(kRight);
    input.up = Held(kUp);
    input.down = Held(kDown);
    input.wheel = dao::input::WheelNotches();
    input.shift = Down(Key::LeftShift) || Down(Key::RightShift);
    return input;
}

// The user, 2026-10-15: "the mouse pointer should disappear when right clicking like in normal
// game". DAOLib #132 holds it through the game's own path, the one its orbit uses.
void HoldCursor(bool held)
{
    if (held != g_cursorHeld)
    {
        dao::input::HoldCursor(held);
        g_cursorHeld = held;
    }
}

void Withhold(bool withheld)
{
    dao::input::Withhold(withheld ? kFlying : 0);
    dao::input::SetKeymap(kKeymap, withheld);
    if (!withheld)
    {
        HoldCursor(false);
    }
}

} // namespace photocam::game
