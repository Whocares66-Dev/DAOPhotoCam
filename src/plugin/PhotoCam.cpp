// photocam.dll: a free camera for screenshots. A key of the player's choosing, listed under its
// own heading in the controls screen, takes the camera off the party in the close view; the
// right mouse button held turns it, W A S D fly it, the wheel sets the speed and Shift with the
// wheel the field of view. The world is paused while it is on, so nothing changes in the shot. The
// key again gives the game its camera back where it was, and its pause as it was. A second key
// locks the view where it is; pressed again, the view flies again.
//
// What is decided (the flight, when it may start) is src/core/'s and tested; what is done to the
// game is plugin/GameView's; Eluvian's tools are plugin/Eluvian's. This file joins them: the keys,
// the frame, and the log.
#include "plugin/PhotoCam.h"

#include "core/Tools.h"
#include "plugin/Eluvian.h"
#include "plugin/GameView.h"
#include "plugin/Panel.h"

#include "dao/Frame.h"
#include "dao/GameModes.h"
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
// Esc ends photo cam too, for a player who forgets its key (the user, 2026-10-15): in photo cam's
// key map, so while it is on Esc does not open the game's menu, and otherwise does.
constexpr std::wstring_view kExitAction = L"PhotoCamExit";

Session g_session;
std::chrono::steady_clock::time_point g_lastFrame;

// Gives the game back its camera, its input and its pause, after the session has ended.
void GiveBack(std::string_view by)
{
    game::Withhold(false);
    game::Release();
    game::Thaw();
    panel::Show(false);
    eluvian::Emit(tools::kStopped, tools::StoppedJson(by));
}

void Frame()
{
    eluvian::Find();
    panel::Frame();
    FlyCamera *fly = g_session.Fly();
    if (fly == nullptr)
    {
        return;
    }
    const auto now = std::chrono::steady_clock::now();
    const float seconds = std::chrono::duration<float>(now - g_lastFrame).count();
    g_lastFrame = now;
    if (g_session.IsLocked())
    {
        game::HoldCursor(false);
    }
    else
    {
        const FlyInput input = game::ReadInput(seconds);
        game::HoldCursor(input.look);
        fly->Step(input);
    }
    DrawNow();
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

void DrawNow()
{
    const FlyCamera *fly = g_session.Fly();
    if (fly == nullptr)
    {
        return;
    }
    // A menu, a conversation, a cutscene, a load or a script of the game's leaves the free view's
    // mode: DAOLib ends the free view, the game keeps the mode it chose, and photo cam ends.
    if (!game::Draw(fly->Current(), fly->FovDegrees()))
    {
        spdlog::info("photo cam: stopped, the game has left the free view");
        g_session.End();
        GiveBack("mode");
    }
}

std::optional<Toggled> Toggle(std::string_view by)
{
    if (!g_session.Active() && !game::Available())
    {
        spdlog::warn("photo cam: not started by the {}, this DAOLib has no free view yet (DAOLib #125)", by);
        return std::nullopt;
    }
    const bool wasActive = g_session.Active();
    auto toggled = g_session.Toggle(wasActive ? GameCamera{} : game::Camera());
    // The world holds still while photo cam is on, so nothing changes in the shot being looked
    // for (the user, 2026-10-15): paused before the fly camera's mode, as the engine's pause is
    // made for the modes before it. The game may still refuse that mode; then photo cam has not
    // started.
    if (toggled == Toggled::Started)
    {
        game::Freeze();
        if (!game::Draw(g_session.Fly()->Current(), g_session.Fly()->FovDegrees()))
        {
            game::Thaw();
            g_session.End();
            toggled = Toggled::NoCamera;
        }
    }
    spdlog::info("photo cam: {} by the {}", Name(toggled), by);
    if (toggled == Toggled::Started)
    {
        g_lastFrame = std::chrono::steady_clock::now();
        game::Withhold(true);
        panel::Show(true);
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
    // Locked, the view only holds still: the world is paused, so the game's keys and mouse would
    // move nothing, and they stay withheld.
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
    // The keys under a heading of their own in the controls screen, where the player rebinds
    // them; F7 and F8 are bound to nothing of the game's, and the flight's W, A, S and D are in
    // photo cam's key map (game::RegisterMoves). While photo cam is on the game is in its
    // fly camera mode, where an action's key works only if the action names the mode (dao/Input.h;
    // seen 2026-10-15: F7 did nothing there): the toggle works in it and where photo cam starts,
    // the lock only in it.
    const bool registered = dao::input::RegisterAction(
                                {
                                    .name = std::wstring(kToggleAction),
                                    .label = L"On / off",
                                    .key = dao::input::Key::F8,
                                    .modes = {dao::GameMode::Explore, dao::GameMode::Combat, dao::GameMode::FlyCam},
                                    .section = L"Photo Camera",
                                },
                                [] { Toggle("key"); }) &&
                            dao::input::RegisterAction(
                                {
                                    .name = std::wstring(kLockAction),
                                    .label = L"Lock camera",
                                    .key = dao::input::Key::F7,
                                    .modes = {dao::GameMode::FlyCam},
                                    .section = L"Photo Camera",
                                },
                                [] { Lock("key"); }) &&
                            dao::input::RegisterAction(
                                {
                                    .name = std::wstring(kExitAction),
                                    .label = L"Exit",
                                    .key = dao::input::Key::Escape,
                                    .modes = {dao::GameMode::FlyCam},
                                    .section = L"Photo Camera",
                                    .keymap = std::wstring(photocam::game::kKeymap),
                                },
                                [] {
                                    if (State().Active())
                                    {
                                        Toggle("key");
                                    }
                                }) &&
                            photocam::game::RegisterMoves();
    if (!registered)
    {
        spdlog::error("photo cam: the keys were not registered");
        return false;
    }
    photocam::panel::Register();
    dao::OnFrame(Frame);
    spdlog::info("photo cam: loaded, on and off by F8, locked by F7, flown by W, A, S and D, unless rebound in the "
                 "controls screen");
    return true;
}
