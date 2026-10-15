// Photo cam's panel: a box in the view's bottom right corner, where the game shows a party
// member's few lines as the pointer is on its portrait, with the camera's lock, its field of
// view and its speed, and a button to leave. The user, 2026-10-15: "a box (same as char info on
// portrait hover) w/ checkbox for Lock camera, sliders for Field of view & Move speed that
// reflects current values. reuse ui elements from options", and "exit button in the bottom
// right of ui helper. the helper itself should be bottom right of screen".
//
// The box is the game's tooltip box drawn to its measures, as DAOTurnBased draws its sheet
// (its dev/UI.md, "A creature's sheet"). The checkbox, the sliders and the button are the
// options screen's own, imported by name from optionsmenu.swf (tools/movie.py). Their classes
// are the options screen's and not loaded here, so this script drives them as those classes do
// (optionsmenu.gfx's SharedLibrary.Slider and Behaviour.ToggleButton, read with FFDec
// 2026-10-15): the tick is the checkbox's Down_mc on its second frame, a pressed state the
// same, and the thumb stands along the track by the thumb's hit area.
//
// The plugin calls refresh with photo cam's state as it changes, and the movie tells it through
// ExternalInterface, PhotoCam_SetLocked, PhotoCam_SetFov, PhotoCam_SetSpeed and PhotoCam_Exit, as
// the player acts. The game is paused while photo cam is on, and a paused game advances none of
// its movies (seen 2026-10-15, on the HUD's layer and the menus'): so nothing here waits on a
// frame, the plugin pushes, and a drag follows the mouse's own events.

Stage.scaleMode = "noScale";
Stage.align = "TL";

var FIELD = "|";
var TITLE = 0xCBB789; // the place name's under the minimap
var BOX_GLOW = 0xF7F3B5;
var BOX_GLOW_ALPHAS = [22, 19, 16, 13, 10, 8, 5, 3, 1];
var BOX_INSET = 9;
var BOX_RADIUS = 13;
var CORNER = 5; // pixels in from the view's bottom right, as the game's fixed tooltip stands
var PAD = 24;
var ARROW = 33; // how far the slider's left arrow stands out of it
var SLIDER = 350; // the slider's width, its arrows in
var WIDTH = PAD + ARROW + SLIDER - ARROW + PAD + 12;
var FOV_STEP = 5; // as a notch of the wheel moves them (src/core/FlyCamera.h)
var SPEED_STEP = 1.25;
var SMALL_WIDTH = 210; // the panel made small: its title and the button that brings it back
var SMALL_HEIGHT = 64;
var SQUARE = 22;
var TITLE_MIDDLE = 13; // the title row's middle, below its top
var EXIT_SCALE = 70;
var OPEN_BOOK = 2; // SoundEvents.ST_OPEN_BOOK
var CLOSE_BOOK = 4; // SoundEvents.ST_CLOSE_BOOK

var state = undefined; // the plugin's last answer
var tick = 0;

function roundedBox(clip, x, y, w, h, r, colour, alpha)
{
    clip.beginFill(colour, alpha);
    clip.moveTo(x + r, y);
    clip.lineTo(x + w - r, y);
    clip.curveTo(x + w, y, x + w, y + r);
    clip.lineTo(x + w, y + h - r);
    clip.curveTo(x + w, y + h, x + w - r, y + h);
    clip.lineTo(x + r, y + h);
    clip.curveTo(x, y + h, x, y + h - r);
    clip.lineTo(x, y + r);
    clip.curveTo(x, y, x + r, y);
    clip.endFill();
}

// The tooltips' TooltipBackground (tooltips.gfx, Tooltips_I26) as DAOTurnBased measured it:
// a rounded black at 80%, 9 pixels in with corners of 13, and a pale glow fading outside it.
function tooltipBox(clip, w, h)
{
    for (var k = 0; k < BOX_GLOW_ALPHAS.length; k++)
    {
        var out = k + 0.5;
        clip.lineStyle(1, BOX_GLOW, BOX_GLOW_ALPHAS[k]);
        roundedBox(clip, BOX_INSET - out, BOX_INSET - out, w - 2 * (BOX_INSET - out), h - 2 * (BOX_INSET - out),
                   BOX_RADIUS + out, 0x000000, 0);
    }
    clip.lineStyle();
    roundedBox(clip, BOX_INSET, BOX_INSET, w - 2 * BOX_INSET, h - 2 * BOX_INSET, BOX_RADIUS, 0x000000, 80);
}

// The title, its letters' middle at `middle`: a text field has 2 pixels of gutter above its line.
function title(clip, x, middle, w, text)
{
    clip.createTextField("title", 1, x, 0, w, 26);
    var field = clip.title;
    var format = new TextFormat();
    format.font = "$TitleFont";
    format.size = 18;
    format.color = TITLE;
    field.embedFonts = true;
    field.selectable = false;
    field.setNewTextFormat(format);
    field.hitTestDisable = true;
    field.text = text;
    field._y = middle - field.textHeight / 2 - 2;
}

function ask(name, value)
{
    return flash.external.ExternalInterface.call(name, value);
}

function click()
{
    fscommand("PlayGUISound", "1 Click"); // SoundEvents.ST_BUTTON, ET_CLICK, as the game's buttons
}

function stopAll(clip)
{
    clip.stop();
    for (var child in clip)
    {
        if (typeof(clip[child]) == "movieclip" && clip[child]._parent == clip)
        {
            stopAll(clip[child]);
        }
    }
}

// A control's look under the pointer and pressed, as the game's behaviours show them: its
// RollOver_mc played forward to its last frame and held there while the pointer is on it, and
// played back to its first as the pointer leaves (Behaviour.RollOver's PlayForward and
// PlayBackward). Played to the end and let loop, the glow came and went (the user, 2026-10-15:
// "exit button is highlighted for a moment on hover, then not. shouldn't it stay highlighted?").
function playTo(clip, last)
{
    if (clip == undefined)
    {
        return;
    }
    clip.target = last ? clip._totalframes : 1;
    clip.onEnterFrame = function()
    {
        if (this._currentframe == this.target)
        {
            this.stop();
            delete this.onEnterFrame;
        }
        else
        {
            this.gotoAndStop(this._currentframe + (this.target > this._currentframe ? 1 : -1));
        }
    };
}

function hover(clip)
{
    clip.onRollOver = clip.onDragOver = function()
    {
        playTo(this.RollOver_mc, true);
    };
    clip.onRollOut = clip.onDragOut = function()
    {
        playTo(this.RollOver_mc, false);
        this.Down_mc.gotoAndStop(1);
    };
}

// ---------------------------------------------------------------------------------------------
// The checkbox

// One of the options screen's checkboxes, its label `text`; `onClick` asks for the change.
function checkBox(panel, name, depth, y, text, onClick)
{
    var box = panel.attachMovie("checkbox_mc", name, depth);
    stopAll(box);
    box._x = PAD;
    box._y = y;
    box.Label_mc.TextField.text = text;
    // The tick is Down_mc's second frame: it stands there and is shown or hidden, as it was while a
    // paused movie did not go to a frame (seen 2026-10-15, before DAOLib #134).
    box.Down_mc.gotoAndStop(2);
    box.Down_mc._visible = false;
    box.onRelease = function()
    {
        click();
        onClick();
        refresh();
    };
    return box;
}

// ---------------------------------------------------------------------------------------------
// A slider: its value as a fraction along the track, to and from the setting

function slider(panel, name, depth, y, text, toFraction, fromFraction, step, shown, setter)
{
    var row = panel.attachMovie("slider_m", name, depth);
    stopAll(row);
    row._x = PAD + ARROW;
    row._y = y;
    row.Label_mc.TextField.text = text;
    row.toFraction = toFraction;
    row.fromFraction = fromFraction;
    row.shown = shown;
    row.setter = setter;
    row.dragging = false;
    var bar = row.Slider_mc;
    var thumb = bar.Thumb_mc;
    hover(thumb);
    thumb.onPress = bar.Track_mc.onPress = function()
    {
        click();
        row.dragging = true;
        thumb.Down_mc.gotoAndStop(2);
        drag(row);
    };
    thumb.onRelease = thumb.onReleaseOutside = bar.Track_mc.onRelease = bar.Track_mc.onReleaseOutside = function()
    {
        row.dragging = false;
        thumb.Down_mc.gotoAndStop(1);
    };
    arrow(row, row.DownArrow_mc, -1, step);
    arrow(row, row.UpArrow_mc, 1, step);
    return row;
}

function arrow(row, button, way, step)
{
    hover(button);
    button.onPress = function()
    {
        this.Down_mc.gotoAndStop(2);
    };
    button.onRelease = function()
    {
        this.Down_mc.gotoAndStop(1);
        click();
        var value = step(row.value, way);
        ask(row.setter, value);
        refresh();
    };
}

// Where the thumb stands for a fraction, as SharedLibrary.Slider places it: the track's left end
// plus the fraction of the track less the thumb's hit area, less where that area starts.
function thumbAt(row, fraction)
{
    var bar = row.Slider_mc;
    var hit = bar.Thumb_mc.HitArea_mc;
    var range = bar.Track_mc._width - hit._width;
    bar.Thumb_mc._x = bar.Track_mc._x + range * Math.min(Math.max(fraction, 0), 1) - hit._x;
}

function drag(row)
{
    var bar = row.Slider_mc;
    var hit = bar.Thumb_mc.HitArea_mc;
    var range = bar.Track_mc._width - hit._width;
    var fraction = (bar._xmouse - bar.Track_mc._x - hit._width / 2) / range;
    fraction = Math.min(Math.max(fraction, 0), 1);
    thumbAt(row, fraction);
    var value = row.fromFraction(fraction);
    row.Value_mc.TextField.text = row.shown(value);
    ask(row.setter, value);
}

function showSlider(row, value)
{
    row.value = value;
    if (!row.dragging)
    {
        thumbAt(row, row.toFraction(value));
        row.Value_mc.TextField.text = row.shown(value);
    }
}

// The field of view along the track evenly; the speed by its ratio, as the wheel steps it, so
// that a slow walk and a fast flight both have room (0.5 to 40 is eighty times).
function fovFraction(v)
{
    return (v - state.fovMin) / (state.fovMax - state.fovMin);
}

function fovOf(f)
{
    return Math.round(state.fovMin + f * (state.fovMax - state.fovMin));
}

function fovStep(v, way)
{
    return Math.min(Math.max(v + way * FOV_STEP, state.fovMin), state.fovMax);
}

function speedFraction(v)
{
    return Math.log(v / state.speedMin) / Math.log(state.speedMax / state.speedMin);
}

function speedOf(f)
{
    return state.speedMin * Math.pow(state.speedMax / state.speedMin, f);
}

function speedStep(v, way)
{
    return Math.min(Math.max(way > 0 ? v * SPEED_STEP : v / SPEED_STEP, state.speedMin), state.speedMax);
}

function degrees(v)
{
    return Math.round(v) + "°";
}

function metres(v)
{
    return (Math.round(v * 10) / 10) + " m/s";
}

// ---------------------------------------------------------------------------------------------
// The button

// The options screen's Cancel button (its export ArrowButton), the curved one at its bottom right,
// which leaves it as Exit leaves photo cam: the user, 2026-10-15, of the plain push button first
// used, "can we not reuse an existing options menu button instead of creating one that doesn't fit
// the theme?"
function exitButton(panel, y)
{
    var button = panel.attachMovie("ArrowButton", "exit", 50);
    stopAll(button);
    setButtonText(button, "Exit");
    // Smaller than the options screen has it (the user: "exit button should be smaller").
    button._xscale = button._yscale = EXIT_SCALE;
    button._x = WIDTH - PAD + 8 - button._width;
    button._y = y;
    hover(button);
    button.onPress = function()
    {
        this.Down_mc.gotoAndStop(2);
    };
    button.onRelease = function()
    {
        this.Down_mc.gotoAndStop(1);
        click();
        ask("PhotoCam_Exit");
    };
    return button;
}

// The first text field anywhere in a button, its label.
function setButtonText(clip, text)
{
    for (var child in clip)
    {
        if (clip[child] instanceof TextField)
        {
            clip[child].text = text;
            return true;
        }
        if (typeof(clip[child]) == "movieclip" && clip[child]._parent == clip && setButtonText(clip[child], text))
        {
            return true;
        }
    }
    return false;
}

// What a clip holds, for the log while an imported control is learned.
function describe(clip, name, depth)
{
    var text = name + " " + Math.round(clip._x) + "," + Math.round(clip._y) + " " + Math.round(clip._width) + "x"
               + Math.round(clip._height) + " frames " + clip._totalframes;
    if (depth > 2)
    {
        return text;
    }
    for (var child in clip)
    {
        if (typeof(clip[child]) == "movieclip" && clip[child]._parent == clip)
        {
            text += "\n" + describe(clip[child], name + "." + child, depth + 1);
        }
        else if (clip[child] instanceof TextField)
        {
            text += "\n" + name + "." + child + " text '" + clip[child].text + "'";
        }
    }
    return text;
}

// ---------------------------------------------------------------------------------------------

// A small square of the checkbox's black and the box's gold, with a gold minus or plus in it: the
// button that makes the panel small, or brings it back, like a window's. `x` is its right edge and
// `middle` the height of its middle. The minus and plus are drawn, so that they stand in the middle
// (the title font's plus stood off it: the user, 2026-10-15, "plus symbol isn't centered").
// (The options checkbox's own square, masked out of it, came out clipped: seen 2026-10-15.)
function squareButton(clip, name, depth, plus, x, middle, onClick)
{
    var holder = clip.createEmptyMovieClip(name, depth);
    holder.lineStyle(1, TITLE, 70);
    roundedBox(holder, 0.5, 0.5, SQUARE - 1, SQUARE - 1, 3, 0x000000, 90);
    var c = SQUARE / 2;
    var arm = 5;
    holder.lineStyle(2, TITLE, 100);
    holder.moveTo(c - arm, c);
    holder.lineTo(c + arm, c);
    if (plus)
    {
        holder.moveTo(c, c - arm);
        holder.lineTo(c, c + arm);
    }
    holder._x = x - SQUARE;
    holder._y = middle - c;
    holder.onRelease = function()
    {
        click();
        onClick();
    };
    return holder;
}

// Shown or hidden by the plugin, at once: the game shows its HUD's screens again as it leaves the
// fly camera, this one among them (seen 2026-10-15: the panel stayed after photo cam was off).
function setShown(shown)
{
    _root._visible = shown;
    // The sounds the inventory makes as it opens and closes (inventory.gfx's Inventory, read
    // 2026-10-15: SoundEvents.ST_OPEN_BOOK and ST_CLOSE_BOOK): the user, "entering / exiting photo
    // camera should play same sound as opening / closing inventory, journal, etc."
    fscommand("PlayGUISound", (shown ? OPEN_BOOK : CLOSE_BOOK) + " Click");
}

function build()
{
    var panel = _root.createEmptyMovieClip("panel", 10);
    var y = PAD - 2;
    title(panel, PAD, y + TITLE_MIDDLE, WIDTH - 2 * PAD, "Photo Camera");
    // The user, 2026-10-15: "a top right button to minimize the ui, retaining only a small box w/ a
    // maximize button".
    squareButton(panel, "minimize", 60, false, WIDTH - PAD + 4, y + TITLE_MIDDLE, function()
    {
        showSmall(true);
    });
    y += 34;
    panel.lock = checkBox(panel, "lock", 20, y, "Lock camera", function()
    {
        ask("PhotoCam_SetLocked", !state.locked);
    });
    y += 34;
    // The user, 2026-10-15: "a check under lock camera to toggle game ui, default off".
    panel.ui = checkBox(panel, "ui", 21, y, "Show interface", function()
    {
        ask("PhotoCam_SetInterface", !state.shown);
    });
    y += 40;
    panel.fov = slider(panel, "fov", 30, y, "Field of view", fovFraction, fovOf, fovStep, degrees, "PhotoCam_SetFov");
    y += 54;
    panel.speed = slider(panel, "speed", 40, y, "Move speed", speedFraction, speedOf, speedStep, metres,
                         "PhotoCam_SetSpeed");
    y += 58;
    var exit = exitButton(panel, y);
    y += exit._height;
    var back = panel.createEmptyMovieClip("back", 0);
    tooltipBox(back, WIDTH, y + PAD - 4);
    panel.boxHeight = y + PAD - 4;
    panel.boxWidth = WIDTH;

    var small = _root.createEmptyMovieClip("small", 11);
    title(small, PAD, SMALL_HEIGHT / 2, SMALL_WIDTH - 2 * PAD, "Photo Camera");
    squareButton(small, "maximize", 60, true, SMALL_WIDTH - PAD + 4, SMALL_HEIGHT / 2, function()
    {
        showSmall(false);
    });
    var smallBack = small.createEmptyMovieClip("back", 0);
    tooltipBox(smallBack, SMALL_WIDTH, SMALL_HEIGHT);
    small.boxHeight = SMALL_HEIGHT;
    small.boxWidth = SMALL_WIDTH;
    showSmall(false);
}

// Shown or hidden at once, as a paused movie does with _visible and not with a frame.
function showSmall(small)
{
    _root.panel._visible = !small;
    _root.small._visible = small;
}

function placeBox(clip)
{
    clip._x = Stage.width - clip.boxWidth - CORNER;
    clip._y = Stage.height - clip.boxHeight - CORNER;
}

function place()
{
    placeBox(_root.panel);
    placeBox(_root.small);
}

function refresh(answer)
{
    if (answer == undefined)
    {
        answer = ask("PhotoCam_GetState");
    }
    if (answer == undefined || answer == null || String(answer) == "off")
    {
        return;
    }
    var f = String(answer).split(FIELD);
    state = {locked: f[0] == "1", fov: Number(f[1]), fovMin: Number(f[2]), fovMax: Number(f[3]),
             speed: Number(f[4]), speedMin: Number(f[5]), speedMax: Number(f[6]), shown: f[7] == "1"};
    var panel = _root.panel;
    panel.lock.Down_mc._visible = state.locked;
    panel.ui.Down_mc._visible = state.shown;
    showSlider(panel.fov, state.fov);
    showSlider(panel.speed, state.speed);
}

build();
refresh();
place();
_root.onMouseMove = function()
{
    var panel = _root.panel;
    if (panel.fov.dragging)
    {
        drag(panel.fov);
    }
    else if (panel.speed.dragging)
    {
        drag(panel.speed);
    }
};
// A view resized while the panel is up: placed again as it is next shown.
_root.onEnterFrame = function()
{
    if (++tick % 30 == 0)
    {
        place();
    }
};
