#include "plugin/Panel.h"

#include "core/FlyCamera.h"
#include "plugin/PhotoCam.h"

#include "dao/ClientExoApp.h"
#include "dao/Gui.h"
#include "dao/Messages.h"
#include "dao/Screenshot.h"

#include <spdlog/spdlog.h>

#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace photocam::panel
{

namespace
{

constexpr std::wstring_view kScreen = L"PhotoCamPanel";
constexpr std::wstring_view kMovie = L"photocam";
// The camera's button, loaded into the top bar's own movie (ui/navbar).
constexpr std::wstring_view kBarMovie = L"NavBar"; // the record's name, as dao::gui::NameOf gives it
constexpr std::wstring_view kBarButton = L"photocam_navbar";
constexpr const char *kBarClip = "PhotoCamNav_mc";

bool g_added = false;
bool g_shown = false;
// The state's text, kept alive for the movie to copy (dao::gui::Value::String keeps no copy).
std::string g_state;
// The state last pushed into the movie, so it is pushed on a change only.
std::string g_pushed;
// A screenshot asked for by the panel's Capture, taken on the next frame: the click arrives inside
// the movie's own call, and the screenshot hides the movie.
bool g_capture = false;

std::optional<double> NumberOf(const dao::gui::Call &call)
{
    return call.args.empty() ? std::nullopt : call.args[0].AsNumber();
}

// "locked|fov|fovMin|fovMax|speed|speedMin|speedMax", or "off".
std::string StateText()
{
    const FlyCamera *fly = plugin::Flight();
    if (fly == nullptr)
    {
        return "off";
    }
    const FlyLimits &limits = fly->Limits();
    return std::format("{}|{}|{}|{}|{}|{}|{}", plugin::State().IsLocked() ? 1 : 0, fly->FovDegrees(), limits.minFov,
                       limits.maxFov, fly->Speed(), limits.minSpeed, limits.maxSpeed);
}

void GetState(const dao::gui::Call &call)
{
    g_state = StateText();
    call.Return(dao::gui::Value::String(g_state.c_str()));
}

// The game is paused while photo cam is on, and a paused game advances none of its movies (seen
// 2026-10-15), so the movie cannot ask each frame: the plugin calls it as the state changes.
void Push()
{
    std::string text = StateText();
    if (text == g_pushed)
    {
        return;
    }
    dao::gui::Movie movie{dao::gui::MovieOf(dao::gui::FindOwner(kMovie))};
    const dao::gui::Value args[] = {dao::gui::Value::String(text.c_str())};
    if (movie.Invoke("_root.refresh", args))
    {
        g_pushed = std::move(text);
    }
}

void SetLocked(const dao::gui::Call &call)
{
    const auto wanted = call.args.empty() ? std::nullopt : call.args[0].AsBool();
    if (wanted && plugin::State().Active() && *wanted != plugin::State().IsLocked())
    {
        plugin::Lock("panel");
    }
}

void SetFov(const dao::gui::Call &call)
{
    FlyCamera *fly = plugin::Flight();
    if (const auto degrees = NumberOf(call); degrees && fly != nullptr)
    {
        fly->SetFov(static_cast<float>(*degrees));
    }
}

void SetSpeed(const dao::gui::Call &call)
{
    FlyCamera *fly = plugin::Flight();
    if (const auto speed = NumberOf(call); speed && fly != nullptr)
    {
        fly->SetSpeed(static_cast<float>(*speed));
    }
}

// The camera's button in the top bar: photo cam on, or off, as its key.
void BarToggle(const dao::gui::Call & /*call*/)
{
    plugin::Toggle("bar");
}

// The top bar's movie the camera's button was last loaded into.
void *g_barView = nullptr;

// Loads the camera's button into the top bar's movie, once for each movie the game makes: it makes
// the bar's movie again as the HUD comes back (a load, the fly camera's end), the button's clip
// gone with the old one. A frame only compares the movie with the last; LoadInto runs on a change,
// and DAOLib keeps the button movie's bytes. (Looked at every 30 frames, the camera came back a
// moment after the bar: the user, 2026-10-15; "do we need to do this work every frame?")
void Bar()
{
    void *owner = dao::gui::FindOwner(kBarMovie);
    void *view = owner != nullptr ? dao::gui::MovieOf(owner) : nullptr;
    if (view == g_barView)
    {
        return;
    }
    g_barView = view;
    if (view == nullptr)
    {
        return;
    }
    if (dao::gui::LoadInto(dao::gui::Movie{view}, kBarClip, kBarButton))
    {
        spdlog::info("photo cam: the camera's button is loaded into the top bar's movie");
    }
    else
    {
        spdlog::warn("photo cam: the top bar's camera button could not be loaded");
        g_barView = nullptr; // tried again next frame
    }
}

void Capture(const dao::gui::Call & /*call*/)
{
    g_capture = true;
}

// The game's own screenshot, as its key takes it (dao/Messages.h); DAOLib's hook (OnScreenshot,
// Register) keeps the panel out of it.
void TakeScreenshot()
{
    dao::CClientExoApp *app = dao::CClientExoApp::Get();
    if (app == nullptr || app->GameModes() == nullptr)
    {
        return;
    }
    auto shot = dao::messages::Make<dao::messages::TakeScreenshotMessage>();
    dao::messages::Send(app->GameModes(), shot);
    spdlog::info("photo cam: a screenshot taken from the panel");
}

void Exit(const dao::gui::Call & /*call*/)
{
    if (plugin::State().Active())
    {
        plugin::Toggle("panel");
    }
}

void Log(const dao::gui::Call &call)
{
    const auto text = call.args.empty() ? std::nullopt : call.args[0].AsString();
    spdlog::info("photo cam: the panel says: {}", text.value_or(""));
}

} // namespace

void Register()
{
    dao::gui::OnExternalInterface("PhotoCam_GetState", &GetState);
    dao::gui::OnExternalInterface("PhotoCam_SetLocked", &SetLocked);
    dao::gui::OnExternalInterface("PhotoCam_SetFov", &SetFov);
    dao::gui::OnExternalInterface("PhotoCam_SetSpeed", &SetSpeed);
    dao::gui::OnExternalInterface("PhotoCam_Exit", &Exit);
    dao::gui::OnExternalInterface("PhotoCam_Capture", &Capture);
    dao::gui::OnExternalInterface("PhotoCam_Toggle", &BarToggle);
    dao::gui::OnExternalInterface("PhotoCam_Log", &Log);
    // Out of the game's screenshots (the user, 2026-10-15: "this ui shouldn't show up in
    // screenshots"): DAOLib #133 holds the capture two frames after `before`, which hides it.
    dao::screenshot::OnScreenshot(
        [] {
            if (g_shown)
            {
                dao::gui::HideScreen(kScreen);
            }
        },
        [] {
            if (g_shown)
            {
                dao::gui::ShowScreen(kScreen);
            }
        });
}

void Frame()
{
    Bar();
    if (std::exchange(g_capture, false))
    {
        TakeScreenshot();
    }
    if (g_added)
    {
        if (g_shown)
        {
            Push();
        }
        return;
    }
    // The interface is up from the main menu's first frame; until then AddScreen says no.
    g_added = dao::gui::AddScreen(kScreen, kMovie, dao::gui::kLayerHud);
    if (g_added)
    {
        spdlog::info("photo cam: the panel's screen is added; its movie {}",
                     dao::gui::HasMovie(L"photocam.swf") ? "found" : "NOT found beside the plugin");
    }
}

void Show(bool shown)
{
    if (!g_added || shown == g_shown)
    {
        return;
    }
    g_shown = shown;
    g_pushed.clear();
    if (shown)
    {
        dao::gui::ShowScreen(kScreen);
    }
    else
    {
        dao::gui::HideScreen(kScreen);
    }
    // And the movie itself, at once: the game shows its HUD's screens again as it leaves the fly
    // camera, the panel's among them (seen 2026-10-15: it stayed after photo cam was off).
    dao::gui::Movie movie{dao::gui::MovieOf(dao::gui::FindOwner(kMovie))};
    const dao::gui::Value args[] = {dao::gui::Value::Bool(shown)};
    movie.Invoke("_root.setShown", args);
}

} // namespace photocam::panel
