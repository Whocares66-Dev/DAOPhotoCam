"""Draw photo cam's gold camera for the game's top bar, in its icons' manner, as three PNGs.

The top bar's icons (navbar.gfx's NavBar_I* images in the interface atlases, cut out and read
2026-10-15) are 50 x 50, embossed gold lit from the top left with a dark shadow to the bottom right,
in three states: as they stand, under the pointer (a pale glow about them), and pressed. This draws
a camera the same way rather than painting one:

  1. its silhouette, a body, the viewfinder's hump, a button and the lens, at four times the size;
  2. a height map from the silhouette blurred, lit from the top left as an emboss;
  3. the light mapped onto the icons' own gold (their pixels sampled dark to light) with a little
     grain for wear, and the lens's glass dark with a catch of light;
  4. the icons' drop shadow, and for the other two states their glow.

    python tools\\camera_icon.py              into build\\ui: camera.png, camera_over.png, camera_down.png
    python tools\\camera_icon.py --sheet      and a sheet of it beside the game's own icons, to compare
"""

from __future__ import annotations

import argparse
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw, ImageFilter

REPO = Path(__file__).resolve().parent.parent
OUT = REPO / "build" / "ui"
SIZE = 50  # the game's icons'
SCALE = 4  # drawn this much larger, then reduced, for smooth edges
FILL = 0.72  # of the icon its art fills, as the bar's own leave room for their glow
TILT = -10  # degrees, as the bar's objects stand a little turned in their light
# The icons' gold from dark to light (NavBar_I69's, I3C's, I29's and I7's pixels, sorted by light).
GOLD = [
    (0.00, (34, 22, 10)),
    (0.25, (82, 54, 26)),
    (0.50, (150, 108, 55)),
    (0.72, (205, 168, 106)),
    (0.88, (240, 218, 168)),
    (1.00, (255, 242, 210)),
]
GLOW = (247, 243, 181)  # the pale glow about an icon under the pointer


def gold(value: float) -> tuple[int, int, int]:
    for (a, ca), (b, cb) in zip(GOLD, GOLD[1:], strict=False):
        if value <= b:
            t = (value - a) / (b - a)
            return tuple(round(x + (y - x) * t) for x, y in zip(ca, cb, strict=True))
    return GOLD[-1][1]


def silhouette(size: int) -> tuple[Image.Image, Image.Image]:
    """The camera's metal and its lens's glass, as masks at `size`."""
    s = size / 200 * FILL
    pad = size * (1 - FILL) / 2
    metal = Image.new("L", (size, size), 0)
    d = ImageDraw.Draw(metal)

    def box(x0, y0, x1, y1, r=0):
        d.rounded_rectangle([x0 * s, y0 * s, x1 * s, y1 * s], radius=r * s, fill=255)

    box(28, 72, 172, 160, 20)  # the body
    d.polygon(
        [(70 * s, 74 * s), (82 * s, 50 * s), (122 * s, 50 * s), (134 * s, 74 * s)],
        fill=255,
    )  # the hump
    box(140, 60, 162, 74, 4)  # the shutter button
    # The lens's mount stands out of the body as a ring.
    centre, ring, glass = (100 * s, 116 * s), 38 * s, 26 * s
    d.ellipse(
        [centre[0] - ring, centre[1] - ring, centre[0] + ring, centre[1] + ring],
        fill=255,
    )
    lens = Image.new("L", (size, size), 0)
    ImageDraw.Draw(lens).ellipse(
        [centre[0] - glass, centre[1] - glass, centre[0] + glass, centre[1] + glass],
        fill=255,
    )
    # A groove about the ring and the body's band, cut in so the emboss draws them.
    cut = Image.new("L", (size, size), 0)
    c = ImageDraw.Draw(cut)
    c.ellipse(
        [centre[0] - ring, centre[1] - ring, centre[0] + ring, centre[1] + ring],
        outline=255,
        width=round(5 * s),
    )
    c.rectangle([30 * s, 86 * s, 62 * s, 90 * s], fill=255)
    c.rectangle([138 * s, 86 * s, 170 * s, 90 * s], fill=255)
    metal = ImageChops.subtract(metal, cut)
    # Into the middle of the icon, with room about it, and turned a little.
    metal = ImageChops.offset(metal, round(pad), round(pad)).rotate(
        TILT, resample=Image.BICUBIC
    )
    lens = ImageChops.offset(lens, round(pad), round(pad)).rotate(
        TILT, resample=Image.BICUBIC
    )
    return metal, lens


def emboss(mask: Image.Image, blur: float) -> Image.Image:
    """Light on a height map, lit from the top left: 128 flat, brighter facing the light."""
    height = mask.filter(ImageFilter.GaussianBlur(blur))
    shift = max(1, round(blur / 2))
    lit = ImageChops.offset(height, shift, shift)
    dark = ImageChops.offset(height, -shift, -shift)
    return ImageChops.add(
        ImageChops.subtract(lit, dark, 1, 128), Image.new("L", mask.size, 0)
    )


def draw(state: str) -> Image.Image:
    big = SIZE * SCALE
    metal, lens = silhouette(big)
    light = emboss(metal, 2 * SCALE)
    grain = Image.effect_noise((big, big), 18).filter(ImageFilter.GaussianBlur(1))
    # The metal: its light on the gold, a broad fall from top left to bottom right under it, as the
    # icons are lit, and grain for wear.
    out = Image.new("RGBA", (big, big), (0, 0, 0, 0))
    pixels = out.load()
    lp, mp, gp, lens_p = light.load(), metal.load(), grain.load(), lens.load()
    for y in range(big):
        for x in range(big):
            if mp[x, y] == 0 and lens_p[x, y] == 0:
                continue
            fall = 0.66 - 0.42 * ((x + y) / (2 * big))
            value = fall + (lp[x, y] - 128) / 128 * 0.9 + (gp[x, y] - 128) / 128 * 0.06
            if lens_p[x, y]:
                # The glass: dark, with a catch of light to its top left.
                dx, dy = x / big - 0.45, y / big - 0.53
                catch = max(0.0, 1 - (dx * dx + dy * dy) / 0.0012)
                value = 0.08 + 0.85 * catch
            r, g, b = gold(min(max(value, 0.0), 1.0))
            pixels[x, y] = (r, g, b, max(mp[x, y], lens_p[x, y]))
    icon = out.resize((SIZE, SIZE), Image.LANCZOS)
    # The icons' shadow, dark and soft to the bottom right.
    alpha = icon.getchannel("A")
    shadow = Image.new("RGBA", icon.size, (0, 0, 0, 0))
    shadow.putalpha(
        ImageChops.offset(alpha, 1, 2)
        .filter(ImageFilter.GaussianBlur(1.5))
        .point(lambda a: a * 0.8)
    )
    framed = Image.new("RGBA", icon.size, (0, 0, 0, 0))
    if state != "normal":
        # The bar's icons under the pointer and pressed: a wide pale glow about the object, near
        # white close to it, and the object itself lit brighter (NavBar_I38 and I3E beside I3C).
        strength = 1.0 if state == "over" else 0.75
        glow = Image.new("RGBA", icon.size, GLOW + (0,))
        wide = alpha.point(lambda a: 255 if a > 40 else 0).filter(
            ImageFilter.GaussianBlur(7)
        )
        glow.putalpha(wide.point(lambda a: min(255, int(a * 2.6 * strength))))
        framed.alpha_composite(glow)
        brighter = 1.35 if state == "over" else 1.2
        r, g, b, a = icon.split()
        icon = Image.merge(
            "RGBA",
            [c.point(lambda v: min(255, int(v * brighter))) for c in (r, g, b)] + [a],
        )
    framed.alpha_composite(shadow)
    framed.alpha_composite(icon)
    return framed


def sheet(icons: dict[str, Image.Image]) -> Path:
    """The camera beside the game's own icons on the bar's art, at twice the size, to compare."""
    research = REPO / "build" / "research" / "navbar"
    bar = Image.open(research / "NavBar_I71.png").convert("RGBA")
    row = Image.new("RGBA", (bar.width + 120, 60 * 3 + 10), (24, 20, 16, 255))
    for i, state in enumerate(("normal", "over", "down")):
        y = 5 + i * 60
        row.alpha_composite(bar, (0, y))
        neighbours = {
            "normal": ("I3C", "I69"),
            "over": ("I38", "I65"),
            "down": ("I3E", "I6B"),
        }[state]
        row.alpha_composite(
            Image.open(research / f"NavBar_{neighbours[0]}.png").convert("RGBA"),
            (300, y),
        )
        row.alpha_composite(icons[state], (352, y))
        row.alpha_composite(
            Image.open(research / f"NavBar_{neighbours[1]}.png").convert("RGBA"),
            (404, y),
        )
    row = row.resize((row.width * 2, row.height * 2), Image.LANCZOS)
    path = OUT / "camera_sheet.png"
    row.save(path)
    return path


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument(
        "--sheet", action="store_true", help="also a sheet beside the game's icons"
    )
    args = parser.parse_args()
    OUT.mkdir(parents=True, exist_ok=True)
    icons = {state: draw(state) for state in ("normal", "over", "down")}
    names = {
        "normal": "camera.png",
        "over": "camera_over.png",
        "down": "camera_down.png",
    }
    for state, icon in icons.items():
        icon.save(OUT / names[state])
    print("drew", ", ".join(names.values()))
    if args.sheet:
        print("sheet", sheet(icons))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
