// Photo cam's button in the game's top bar, before Options: the user, 2026-10-15, "create a gold
// camera icon and put it before options in the top bar". Loaded into the bar's own movie
// (navbar.gfx, its record NavBar) by DAOLib's LoadInto, so it shares that movie's _root: its
// buttons are _root.NavBar_mc.Buttons_mc's children, NavButtons of the game's (navbar.gfx's NavBar
// and NavButton, read with FFDec 2026-10-15), each a toggle that opens its screen.
//
// The bar as it stands (read in the game 2026-10-15): nine buttons 37 to 38 pixels apart in
// Buttons_mc, Options (EscapeButton_mc) last, on Background_mc, the bar's art, 449 wide. The camera
// takes Options' place and Options moves one place on; the bar's art is made one place wider and
// all is moved back half a place, so that the bar stays centred.
//
// The camera is drawn by tools/camera_icon.py in the bar's icons' manner, in three looks embedded
// here: PhotoCam_Camera, PhotoCam_CameraOver under the pointer, PhotoCam_CameraDown pressed. A
// click turns photo cam on through ExternalInterface, PhotoCam_Toggle.

var SPACING = 38; // between two of the bar's buttons
var TOOLTIP_DELAY = 200; // milliseconds, Behaviour.DisplayTooltip's InitialDelay
var SIMPLE_FLOATING = 1; // Behaviour.DisplayTooltip.TOOLTIP_TYPE_SIMPLE_FLOATING, the bar's
var ANCHOR_TOP = 1;
var ANCHOR_BOTTOM = 2;

function ask(name, value)
{
    return flash.external.ExternalInterface.call(name, value);
}

function look(button, which)
{
    button.normal._visible = which == 0;
    button.over._visible = which == 1;
    button.down._visible = which == 2;
}

// The tooltip the bar's buttons show, as NavButton and Behaviour.DisplayTooltip show it (navbar.gfx,
// read 2026-10-15): the label with the key bound to it in bold, under the button after a moment,
// through the game's floating tooltip, or the bar's own tooltip clip where it has one. The user,
// 2026-10-15: "add a tooltip, 'Photo Camera (F8)'", then "Photo Cam". The key is the one the player bound.
function tooltipText()
{
    var key = flash.external.ExternalInterface.call("GetValue",
                                                     "InputMappings.Mappings.PhotoCamToggle.PrimaryMappedKey");
    var valid = key != undefined && key != null && String(key).length > 0
                && String(key).lastIndexOf("?") != String(key).length - 1;
    return "Photo Cam" + (valid ? "  <b>(" + key + ") </b>" : "");
}

function setValue(name, value)
{
    fscommand("SetValue", name + " " + String(value));
}

function setText(name, value)
{
    fscommand("SetValueUnicode", name + " " + String(value));
}

function showTooltip(button)
{
    var text = tooltipText();
    if (_root.Tooltip_mc != undefined)
    {
        _root.Tooltip_mc.DisplayTooltip(button, text);
        return;
    }
    var bounds = button.getBounds(_root);
    setValue("GUITooltips.TooltipType", SIMPLE_FLOATING);
    setValue("GUITooltips.HostAnchor", ANCHOR_BOTTOM);
    setValue("GUITooltips.TooltipAnchor", ANCHOR_TOP);
    setValue("GUITooltips.MinTooltipWidth", 0);
    setValue("GUITooltips.MaxTooltipWidth", 0);
    setValue("GUITooltips.HostX", bounds.xMin);
    setValue("GUITooltips.HostY", bounds.yMin);
    setValue("GUITooltips.HostWidth", bounds.xMax - bounds.xMin);
    setValue("GUITooltips.HostHeight", bounds.yMax - bounds.yMin);
    setText("GUITooltips.TooltipCompare0", "");
    setText("GUITooltips.TooltipCompare1", "");
    setText("GUITooltips.TooltipText", text);
}

function hideTooltip(button)
{
    clearInterval(button.tooltipWait);
    button.tooltipWait = 0;
    if (_root.Tooltip_mc != undefined)
    {
        _root.Tooltip_mc._visible = false;
    }
    else
    {
        setText("GUITooltips.TooltipText", "");
    }
}

// Where the camera stands on the bar, in this clip's own space: the place Options had, in the
// bar's buttons, carried over. The bar is placed by the game's layout as the view changes.
function follow(button)
{
    var buttons = _root.NavBar_mc.Buttons_mc;
    var corner = {x: button.slot, y: 0};
    var across = {x: button.slot + 100, y: 0};
    buttons.localToGlobal(corner);
    buttons.localToGlobal(across);
    home.globalToLocal(corner);
    home.globalToLocal(across);
    button._x = corner.x;
    button._y = corner.y;
    button._xscale = button._yscale = across.x - corner.x;
}

function addButton()
{
    var bar = _root.NavBar_mc;
    var buttons = bar.Buttons_mc;
    var options = buttons.EscapeButton_mc;
    if (options == undefined)
    {
        return false;
    }
    if (home.camera != undefined)
    {
        return true;
    }
    // In this clip, not the bar's: attachMovie takes symbols from the clip's own movie, and the
    // camera's pictures are this movie's (seen 2026-10-15: attached in the bar's clip, none came).
    var button = home.createEmptyMovieClip("camera", 1);
    button.attachMovie("PhotoCam_Camera", "normal", 1);
    button.attachMovie("PhotoCam_CameraOver", "over", 2);
    button.attachMovie("PhotoCam_CameraDown", "down", 3);
    look(button, 0);
    button.slot = options._x;
    options._x += SPACING;
    buttons._x -= SPACING / 2;
    bar.Background_mc._x -= SPACING / 2;
    bar.Background_mc._width += SPACING;
    bar.HitArea_mc._x -= SPACING / 2;
    bar.HitArea_mc._width += SPACING;
    follow(button);

    button.onRollOver = button.onDragOver = function()
    {
        look(this, 1);
        var self = this;
        clearInterval(this.tooltipWait);
        this.tooltipWait = setInterval(function()
        {
            clearInterval(self.tooltipWait);
            showTooltip(self);
        }, TOOLTIP_DELAY);
    };
    button.onRollOut = button.onDragOut = button.onReleaseOutside = function()
    {
        look(this, 0);
        hideTooltip(this);
    };
    button.onPress = function()
    {
        look(this, 2);
    };
    button.onRelease = function()
    {
        look(this, 1);
        hideTooltip(this);
        ask("PhotoCam_Toggle");
    };
    return true;
}

var home = this;
// The bar's buttons are made as its movie starts; until they are, look again each frame. Then
// keep the camera over its place, which the game's layout moves with the view.
this.onEnterFrame = function()
{
    if (addButton())
    {
        follow(home.camera);
    }
};
