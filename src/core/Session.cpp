#include "core/Session.h"

#include <utility>

namespace photocam
{

std::string_view Name(Toggled toggled)
{
    switch (toggled)
    {
    case Toggled::Started:
        return "started";
    case Toggled::Stopped:
        return "stopped";
    case Toggled::NoCamera:
        return "not started, no camera";
    case Toggled::Tactical:
        return "not started, the tactical view";
    }
    return "?";
}

std::string_view Name(Locked locked)
{
    switch (locked)
    {
    case Locked::Locked:
        return "locked";
    case Locked::Unlocked:
        return "unlocked";
    case Locked::Off:
        return "not locked, photo cam is off";
    }
    return "?";
}

Locked Session::Lock()
{
    if (!Active())
    {
        return Locked::Off;
    }
    m_locked = !m_locked;
    return m_locked ? Locked::Locked : Locked::Unlocked;
}

Toggled Session::Toggle(const GameCamera &game)
{
    if (Active())
    {
        End();
        return Toggled::Stopped;
    }
    if (!game.present)
    {
        return Toggled::NoCamera;
    }
    if (game.tactical)
    {
        return Toggled::Tactical;
    }
    m_taken = game;
    m_fly.emplace(game.view, game.fovDegrees);
    return Toggled::Started;
}

std::optional<GameCamera> Session::End()
{
    if (!m_fly)
    {
        return std::nullopt;
    }
    m_fly.reset();
    m_locked = false;
    return std::exchange(m_taken, std::nullopt);
}

} // namespace photocam
