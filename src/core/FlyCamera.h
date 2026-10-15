// The photo camera's flight: where the free view is and where it looks, moved by one frame's
// input. It never includes <Windows.h>, links DAOLib or sees a dao:: type, so it is tested
// without the game (tests/), the only kind of test there is; the plugin (src/plugin/) reads the
// game's input and camera, steps this, and hands the pose back to the game.
//
// The world is the game's: metres, z up. The heading is the direction the view looks, in
// radians from +x toward +y, and the pitch is how far above the horizon it looks. (The game's
// SceneCamera measures its yaw the other way about, from what it looks at to the camera; the
// plugin turns one into the other.)
#pragma once

namespace photocam
{

// A point or a direction in the game's world.
struct Vec3
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    friend bool operator==(const Vec3 &, const Vec3 &) = default;
};

// Where the view is and where it looks.
struct Pose
{
    Vec3 eye;
    float heading = 0.0f; // radians from +x toward +y
    float pitch = 0.0f;   // radians above the horizon; negative looks down
};

// One frame's input, as the plugin read it from the game.
struct FlyInput
{
    float seconds = 0.0f; // the frame's time
    bool look = false;    // the right mouse button held: the mouse turns the view
    float mouseX = 0.0f;  // the mouse's movement since the last frame, rightward positive
    float mouseY = 0.0f;  // downward positive, as the screen counts
    bool forward = false; // W
    bool back = false;    // S
    bool left = false;    // A
    bool right = false;   // D
    bool up = false;      // Q
    bool down = false;    // E
    int wheel = 0;        // wheel notches since the last frame, away from the player positive
    bool shift = false;   // Shift held: the wheel changes the field of view, not the speed
};

// The limits and rates of the flight.
struct FlyLimits
{
    float minSpeed = 0.5f;           // metres a second
    float maxSpeed = 40.0f;          // metres a second
    float speedStep = 1.25f;         // the speed is multiplied or divided by this per notch
    float minFov = 15.0f;            // degrees, vertical, as the game measures it
    float maxFov = 120.0f;           // degrees, vertical
    float fovStep = 5.0f;            // degrees per notch
    float radiansPerCount = 0.0025f; // the view's turn per unit of mouse movement
    float maxPitch = 1.5533430f;     // 89 degrees: never straight up or down, where heading is lost
    float maxSeconds = 0.1f;         // a longer frame (a hitch, a load) moves no farther than this
};

// The free view, flown by the player's input a frame at a time.
class FlyCamera
{
  public:
    // Starts at a pose with a speed and a field of view, each clamped to the limits.
    FlyCamera(Pose start, float fovDegrees, float speed = 4.0f, FlyLimits limits = {});

    // Moves and turns the view by one frame's input.
    //
    // The wheel changes the speed, or with Shift the field of view (away from the player
    // narrows it, as a lens zooms in). With the right mouse button held the mouse turns the
    // view. W and S move along the way the view looks, up or down as it looks; A and D move
    // level to the side; Q and E straight up and down.
    void Step(const FlyInput &input);

    // Puts the view at a pose at once, the pitch within its limits.
    void Place(Pose pose);
    // Sets the field of view, within its limits.
    void SetFov(float degrees);
    // Sets the speed, within its limits.
    void SetSpeed(float metresPerSecond);

    const Pose &Current() const
    {
        return m_pose;
    }
    float Speed() const
    {
        return m_speed;
    }
    float FovDegrees() const
    {
        return m_fov;
    }
    const FlyLimits &Limits() const
    {
        return m_limits;
    }

    // The direction the view looks, of length one.
    static Vec3 Forward(const Pose &pose);
    // The direction to the view's right, level, of length one.
    static Vec3 Right(const Pose &pose);

  private:
    void Wheel(int notches, bool shift);
    void Turn(float mouseX, float mouseY);
    void Move(const FlyInput &input);

    Pose m_pose;
    float m_fov;
    float m_speed;
    FlyLimits m_limits;
};

} // namespace photocam
