#include "core/FlyCamera.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace photocam
{

namespace
{

// The heading kept within one turn either side of zero, so a long session's turning never
// costs it precision.
float Wrapped(float radians)
{
    constexpr float kTurn = 2.0f * std::numbers::pi_v<float>;
    return std::remainder(radians, kTurn);
}

} // namespace

FlyCamera::FlyCamera(Pose start, float fovDegrees, float speed, FlyLimits limits)
    : m_fov(std::clamp(fovDegrees, limits.minFov, limits.maxFov)),
      m_speed(std::clamp(speed, limits.minSpeed, limits.maxSpeed)), m_limits(limits)
{
    Place(start);
}

void FlyCamera::Place(Pose pose)
{
    m_pose = pose;
    m_pose.heading = Wrapped(m_pose.heading);
    m_pose.pitch = std::clamp(m_pose.pitch, -m_limits.maxPitch, m_limits.maxPitch);
}

void FlyCamera::SetFov(float degrees)
{
    m_fov = std::clamp(degrees, m_limits.minFov, m_limits.maxFov);
}

void FlyCamera::SetSpeed(float metresPerSecond)
{
    m_speed = std::clamp(metresPerSecond, m_limits.minSpeed, m_limits.maxSpeed);
}

Vec3 FlyCamera::Forward(const Pose &pose)
{
    const float level = std::cos(pose.pitch);
    return {std::cos(pose.heading) * level, std::sin(pose.heading) * level, std::sin(pose.pitch)};
}

Vec3 FlyCamera::Right(const Pose &pose)
{
    // Forward x up, z up: looking along +x, the right is -y.
    return {std::sin(pose.heading), -std::cos(pose.heading), 0.0f};
}

void FlyCamera::Step(const FlyInput &input)
{
    Wheel(input.wheel, input.shift);
    if (input.look)
    {
        Turn(input.mouseX, input.mouseY);
    }
    Move(input);
}

void FlyCamera::Wheel(int notches, bool shift)
{
    if (notches == 0)
    {
        return;
    }
    if (shift)
    {
        // Away from the player narrows the view, as a lens zooms in.
        m_fov = std::clamp(m_fov - (static_cast<float>(notches) * m_limits.fovStep), m_limits.minFov, m_limits.maxFov);
        return;
    }
    m_speed = std::clamp(m_speed * std::pow(m_limits.speedStep, static_cast<float>(notches)), m_limits.minSpeed,
                         m_limits.maxSpeed);
}

void FlyCamera::Turn(float mouseX, float mouseY)
{
    // The mouse to the right turns the view clockwise seen from above, toward -y from +x; the
    // mouse toward the player (down the screen) looks down.
    m_pose.heading = Wrapped(m_pose.heading - (mouseX * m_limits.radiansPerCount));
    m_pose.pitch =
        std::clamp(m_pose.pitch - (mouseY * m_limits.radiansPerCount), -m_limits.maxPitch, m_limits.maxPitch);
}

void FlyCamera::Move(const FlyInput &input)
{
    const float along = static_cast<float>(input.forward) - static_cast<float>(input.back);
    const float side = static_cast<float>(input.right) - static_cast<float>(input.left);
    if ((along == 0.0f && side == 0.0f) || input.seconds <= 0.0f)
    {
        return;
    }
    const Vec3 forward = Forward(m_pose);
    const Vec3 right = Right(m_pose);
    const Vec3 way{(forward.x * along) + (right.x * side), (forward.y * along) + (right.y * side), forward.z * along};
    // Diagonally no faster than straight on.
    const float length = std::sqrt((way.x * way.x) + (way.y * way.y) + (way.z * way.z));
    const float step = m_speed * std::min(input.seconds, m_limits.maxSeconds) / length;
    m_pose.eye.x += way.x * step;
    m_pose.eye.y += way.y * step;
    m_pose.eye.z += way.z * step;
}

} // namespace photocam
