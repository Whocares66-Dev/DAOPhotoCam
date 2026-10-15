#include "plugin/Eluvian.h"

#include "core/Tools.h"
#include "plugin/EluvianAPI.h"
#include "plugin/GameView.h"
#include "plugin/PhotoCam.h"

#include <spdlog/spdlog.h>

#include <cstdint>
#include <optional>
#include <string>
#include <variant>

namespace photocam::eluvian
{

namespace
{

const EluvianInterface *g_eluvian = nullptr;
bool g_looked = false;

void ReplyState(EluvianReply *reply)
{
    const std::string state = tools::StateJson(plugin::State(), game::Available());
    reply->ok(reply, state.c_str());
}

void StateTool(void * /*context*/, const char * /*args*/, EluvianReply *reply)
{
    ReplyState(reply);
}

void ToggleTool(void * /*context*/, const char *args, EluvianReply *reply)
{
    const auto parsed = tools::ParseSwitch(args != nullptr ? args : "", tools::kToggle, "on");
    if (const auto *error = std::get_if<std::string>(&parsed))
    {
        reply->error(reply, error->c_str());
        return;
    }
    const auto wanted = std::get<std::optional<bool>>(parsed);
    if (wanted && *wanted == plugin::State().Active())
    {
        ReplyState(reply);
        return;
    }
    const auto toggled = plugin::Toggle("tool");
    if (!toggled)
    {
        reply->error(reply, "photocam_toggle: this build of photo cam cannot draw a free view: it needs DAOLib's "
                            "free view (DAOLib #125).");
        return;
    }
    if (const std::string refusal = tools::Refusal(*toggled); !refusal.empty())
    {
        reply->error(reply, refusal.c_str());
        return;
    }
    ReplyState(reply);
}

void LockTool(void * /*context*/, const char *args, EluvianReply *reply)
{
    const auto parsed = tools::ParseSwitch(args != nullptr ? args : "", tools::kLock, "locked");
    if (const auto *error = std::get_if<std::string>(&parsed))
    {
        reply->error(reply, error->c_str());
        return;
    }
    if (!plugin::State().Active())
    {
        reply->error(reply, "photocam_lock: photo cam is off, so there is no view to lock: turn it on with "
                            "photocam_toggle first.");
        return;
    }
    const auto wanted = std::get<std::optional<bool>>(parsed);
    if (!wanted || *wanted != plugin::State().IsLocked())
    {
        plugin::Lock("tool");
    }
    ReplyState(reply);
}

void PoseTool(void * /*context*/, const char *args, EluvianReply *reply)
{
    const auto parsed = tools::ParsePose(args != nullptr ? args : "");
    if (const auto *error = std::get_if<std::string>(&parsed))
    {
        reply->error(reply, error->c_str());
        return;
    }
    FlyCamera *fly = plugin::Flight();
    if (fly == nullptr)
    {
        reply->error(reply, "photocam_pose: photo cam is off, so there is no view to place: turn it on with "
                            "photocam_toggle first.");
        return;
    }
    const auto &request = std::get<tools::PoseRequest>(parsed);
    std::optional<Vec3> aim;
    if (request.lookAt)
    {
        const auto *point = std::get_if<Vec3>(&*request.lookAt);
        const auto found =
            point != nullptr ? std::optional(*point) : game::PositionOf(std::get<tools::ObjectId>(*request.lookAt));
        if (!found)
        {
            reply->error(reply, "photocam_pose: `lookAt` names no creature or object in the game: take an id from "
                                "`creatures`, `placeables` or `surroundings`.");
            return;
        }
        aim = tools::AimOf(request, *found);
    }
    if (const std::string error = tools::Apply(*fly, request, aim); !error.empty())
    {
        reply->error(reply, error.c_str());
        return;
    }
    // Drawn now: Eluvian takes a plugin's answer before its handler returns (its HostApi.cpp, read
    // 2026-10-15), so the answer cannot wait for a frame, and a screenshot after it shows the view.
    plugin::DrawNow();
    ReplyState(reply);
}

void MoveTool(void * /*context*/, const char *args, EluvianReply *reply)
{
    const auto parsed = tools::ParseMove(args != nullptr ? args : "");
    if (const auto *error = std::get_if<std::string>(&parsed))
    {
        reply->error(reply, error->c_str());
        return;
    }
    FlyCamera *fly = plugin::Flight();
    if (fly == nullptr)
    {
        reply->error(reply, "photocam_move: photo cam is off, so there is no view to move: turn it on with "
                            "photocam_toggle first.");
        return;
    }
    tools::Apply(*fly, std::get<tools::MoveRequest>(parsed));
    plugin::DrawNow();
    ReplyState(reply);
}

void Register(const char *name, const std::string &descriptor, EluvianToolFn handler, std::uint32_t flags)
{
    if (g_eluvian->RegisterTool(name, descriptor.c_str(), handler, nullptr, flags) < 0)
    {
        spdlog::warn("photo cam: Eluvian refused the tool {} (eluvian.log says why)", name);
    }
}

} // namespace

void Find()
{
    if (g_looked)
    {
        return;
    }
    g_looked = true;
    g_eluvian = EluvianFind();
    if (g_eluvian == nullptr)
    {
        return;
    }
    Register(tools::kState, tools::StateDescriptor(), &StateTool, ELUVIAN_READ_ONLY);
    Register(tools::kToggle, tools::ToggleDescriptor(), &ToggleTool, 0);
    Register(tools::kLock, tools::LockDescriptor(), &LockTool, 0);
    Register(tools::kPose, tools::PoseDescriptor(), &PoseTool, 0);
    Register(tools::kMove, tools::MoveDescriptor(), &MoveTool, 0);
    for (const auto &event : tools::Events())
    {
        if (g_eluvian->DescribeEvent(event.topic, event.descriptor.c_str()) < 0)
        {
            spdlog::warn("photo cam: Eluvian refused the event {} (eluvian.log says why)", event.topic);
        }
    }
    spdlog::info("photo cam: tools and events registered with Eluvian");
}

void Emit(const char *topic, const std::string &payloadJson)
{
    if (g_eluvian != nullptr)
    {
        g_eluvian->EmitEvent(topic, payloadJson.c_str());
    }
}

} // namespace photocam::eluvian
