// Photo cam on and off: when the toggle key may start it, what it starts from, and what is given
// back when it ends. Like FlyCamera, it sees nothing of the game: the plugin tells it what the
// game's camera is, and does what it answers.
#pragma once

#include "core/FlyCamera.h"

#include <optional>
#include <string_view>

namespace photocam
{

// The game's camera as the plugin read it when the toggle key was pressed.
struct GameCamera
{
    bool present = false;  // explore or combat mode, with a camera to take over
    bool tactical = false; // the tactical view, high above
    Pose view;             // where the game's camera is and where it looks
    float fovDegrees = 0.0f;
};

// What a press of the toggle key did.
enum class Toggled
{
    Started,  // photo cam is on, from the game's view
    Stopped,  // photo cam is off; give the game back its camera
    NoCamera, // not started: no camera to take over (a menu, a conversation, loading)
    Tactical, // not started: the tactical view, which photo cam leaves alone
};

// Returns a short lower-case description of a toggle's outcome, for the log.
std::string_view Name(Toggled toggled);

// What a press of the lock key did.
enum class Locked
{
    Locked,   // the view holds still against the keys and mouse
    Unlocked, // the view flies again
    Off,      // nothing: photo cam is off
};

// Returns a short lower-case description of a lock's outcome, for the log.
std::string_view Name(Locked locked);

// Whether photo cam is on, its flight while it is, and the game's camera it took over.
class Session
{
  public:
    // A press of the toggle key: starts from the game's camera, or stops.
    Toggled Toggle(const GameCamera &game);

    // A press of the lock key: while photo cam is on, holds the view still or lets it fly
    // again. Each start is unlocked.
    Locked Lock();
    // Whether the view holds still: photo cam on and locked.
    bool IsLocked() const
    {
        return m_locked;
    }

    // Ends photo cam without the key, as when the game leaves the mode it has a camera in.
    // @return the game's camera as it was taken over, or nullopt if photo cam was off
    std::optional<GameCamera> End();

    bool Active() const
    {
        return m_fly.has_value();
    }
    // The flight, while photo cam is on; null while it is off.
    FlyCamera *Fly()
    {
        return m_fly ? &*m_fly : nullptr;
    }
    const FlyCamera *Fly() const
    {
        return m_fly ? &*m_fly : nullptr;
    }
    // The game's camera as photo cam took it over; nullopt while photo cam is off.
    const std::optional<GameCamera> &TakenOver() const
    {
        return m_taken;
    }

  private:
    std::optional<FlyCamera> m_fly;
    std::optional<GameCamera> m_taken;
    bool m_locked = false;
    float m_speed = 4.0f; // the speed the player chose last, kept for the next start
};

} // namespace photocam
