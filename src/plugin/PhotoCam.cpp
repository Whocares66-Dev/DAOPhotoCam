// photocam.dll: a free camera for screenshots. A key of the player's choosing, listed under its
// own heading in the controls screen, takes the camera off the party in the close view; the
// right mouse button held turns it, W A S D fly it, the wheel sets the speed and Shift with the
// wheel the field of view. The key again gives the game its camera back where it was. A second
// key locks the view where it is and gives the game its keys and mouse back, so the party can be
// walked into the shot; pressed again, the view flies again.
//
// What is decided (the flight, when it may start) is src/core/'s and tested; what is done to the
// game is plugin/GameView's; Eluvian's tools are plugin/Eluvian's. This file joins them: the keys,
// the frame, and the log.
#include "plugin/PhotoCam.h"

#include "core/Tools.h"
#include "plugin/Eluvian.h"
#include "plugin/GameView.h"

#include "dao/Frame.h"
#include "dao/Input.h"
#include "dao/Log.h"
#include "dao/Plugin.h"

#include <spdlog/spdlog.h>

#include <chrono>
#include <string>

namespace photocam::plugin
{

namespace
{

constexpr std::wstring_view kToggleAction = L"PhotoCamToggle";
constexpr std::wstring_view kLockAction = L"PhotoCamLock";

Session g_session;
std::chrono::steady_clock::time_point g_lastFrame;

// Gives the game back its camera and input, after the session has ended.
void GiveBack(std::string_view by)
{
    game::Withhold(false);
    game::Release();
    eluvian::Emit(tools::kStopped, tools::StoppedJson(by));
}

void Frame()
{
    eluvian::Find();
    FlyCamera *fly = g_session.Fly();
    if (fly == nullptr)
    {
        return;
    }
    // A menu, a conversation, a cutscene or a load takes the camera: photo cam ends, and the
    // game has its own camera back when it returns.
    if (!game::Camera().present)
    {
        spdlog::info("photo cam: stopped, the game has left the close view's modes");
        g_session.End();
        GiveBack("mode");
        return;
    }
    const auto now = std::chrono::steady_clock::now();
    const float seconds = std::chrono::duration<float>(now - g_lastFrame).count();
    g_lastFrame = now;
    if (!g_session.IsLocked())
    {
        fly->Step(game::ReadInput(seconds));
    }
    game::Draw(fly->Current(), fly->FovDegrees());
}

} // namespace

const Session &State()
{
    return g_session;
}

FlyCamera *Flight()
{
    return g_session.Fly();
}

std::optional<Toggled> Toggle(std::string_view by)
{
    if (!g_session.Active() && !game::Available())
    {
        spdlog::warn("photo cam: not started by the {}, this DAOLib has no free view yet (DAOLib #125, #126, #127)",
                     by);
        return std::nullopt;
    }
    const bool wasActive = g_session.Active();
    const auto toggled = g_session.Toggle(wasActive ? GameCamera{} : game::Camera());
    spdlog::info("photo cam: {} by the {}", Name(toggled), by);
    if (toggled == Toggled::Started)
    {
        g_lastFrame = std::chrono::steady_clock::now();
        game::ReadInput(0.0f); // what moved before the start is not the flight's
        game::Withhold(true);
        eluvian::Emit(tools::kStarted, tools::StartedJson(*g_session.Fly(), by));
    }
    else if (toggled == Toggled::Stopped)
    {
        GiveBack(by);
    }
    return toggled;
}

Locked Lock(std::string_view by)
{
    const auto locked = g_session.Lock();
    spdlog::info("photo cam: {} by the {}", Name(locked), by);
    if (locked == Locked::Off)
    {
        return locked;
    }
    // Locked, the view takes no input, so the game has its own back.
    game::ReadInput(0.0f); // what moved while locked is not the flight's
    game::Withhold(locked == Locked::Unlocked);
    eluvian::Emit(tools::kLocked, tools::LockedJson(locked == Locked::Locked, by));
    return locked;
}

} // namespace photocam::plugin

extern "C" __declspec(dllexport) bool __cdecl DAOLib_Load(const dao::LoaderInfo *info)
{
    using namespace photocam::plugin;
    dao::OpenLog("photocam");
    if (info == nullptr || info->apiVersion != dao::kPluginApiVersion || !info->knownExecutable)
    {
        spdlog::error("photo cam: not loaded, not the loader or the game this was built for");
        return false;
    }
    // Both keys under a heading of their own in the controls screen, where the player rebinds
    // them; F7 and F8 are bound to nothing of the game's.
    const bool registered = dao::input::RegisterAction(
                                {
                                    .name = std::wstring(kToggleAction),
                                    .label = L"Photo camera on/off",
                                    .key = dao::input::Key::F8,
                                    .section = L"Photo Camera",
                                },
                                [] { Toggle("key"); }) &&
                            dao::input::RegisterAction(
                                {
                                    .name = std::wstring(kLockAction),
                                    .label = L"Lock photo camera",
                                    .key = dao::input::Key::F7,
                                    .section = L"Photo Camera",
                                },
                                [] { Lock("key"); });
    if (!registered)
    {
        spdlog::error("photo cam: the keys were not registered");
        return false;
    }
    dao::OnFrame(Frame);
    spdlog::info("photo cam: loaded, on and off by F8, locked by F7, unless rebound in the controls screen");
    return true;
}
