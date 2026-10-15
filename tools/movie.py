"""Build photo cam's movies from their ActionScript 2 sources.

Two of them: the panel, photocam.swf (ui/panel), a screen of photo cam's own; and the camera
button in the game's top bar, photocam_navbar.swf (ui/navbar), loaded into the bar's own movie.

The game's interface is Scaleform GFx 3, which loads a plain SWF 8 like its own movies (DAOLib's
dao/Gui.h: a plugin's screen finds <movie>.swf beside its dll, and LoadInto loads one into a game
movie). So a movie is built from text with no Flash authoring, as DAOTurnBased's tools/movie.py
builds its turn bar (read 2026-10-15):

  1. this script writes a shell: the stage; what the movie imports by name, as the game's own
     movies import (the fonts from gfxfontlib.swf, the panel's controls from optionsmenu.swf,
     which exports them: read with FFDec 2026-10-15); pictures of its own, each a bitmap drawn
     in a sprite exported by name for attachMovie (the camera, tools/camera_icon.py); and an empty
     frame script;
  2. JPEXS FFDec compiles the movie's frame_1/DoAction.as into that script;
  3. FFDec decompiles the result, and the build fails unless every function and every PhotoCam_
     call of the source came back.

    python tools\\movie.py                       both, into build\\ui
    python tools\\movie.py photocam_navbar       that one alone
    python tools\\movie.py --ffdec C:\\...\\ffdec  FFDec elsewhere than C:\\Modding

FFDec is https://github.com/jindrapetrik/jpexs-decompiler, run on Java, installed in C:\\Modding
for DAOTurnBased (2026-09-30).
"""

from __future__ import annotations

import argparse
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import zlib
from dataclasses import dataclass, field
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
OUT_DIR = REPO / "build" / "ui"
FFDEC = Path(r"C:\Modding\ffdec_26.3.0")

SWF_VERSION = 8  # the game's movies are SWF 8
FPS = 30
FONTS = {"gfxfontlib.swf": ["$TitleFont", "$BodyFont"]}


@dataclass(frozen=True)
class Movie:
    """One of photo cam's movies: its name, its script's folder, what it imports and draws."""

    name: str  # built as build/ui/<name>.swf and served from beside the dll
    source: Path  # holds frame_1/DoAction.as
    stage: tuple[int, int]
    imports: dict[str, list[str]] = field(
        default_factory=dict
    )  # library: exported names
    pictures: dict[str, Path] = field(default_factory=dict)  # exported name: PNG

    @property
    def out(self) -> Path:
        return OUT_DIR / f"{self.name}.swf"

    @property
    def script(self) -> Path:
        return self.source / "frame_1" / "DoAction.as"


PANEL = Movie(
    "photocam",
    REPO / "ui" / "panel",
    (1280, 720),  # the panel stands by the view's corner, measured as it runs
    imports=FONTS | {"optionsmenu.swf": ["checkbox_mc", "slider_m", "ArrowButton"]},
)
# Loaded into the top bar's movie, on its stage; its pictures are the camera's three looks.
NAVBAR = Movie(
    "photocam_navbar",
    REPO / "ui" / "navbar",
    (1280, 720),
    pictures={
        "PhotoCam_Camera": OUT_DIR / "camera.png",
        "PhotoCam_CameraOver": OUT_DIR / "camera_over.png",
        "PhotoCam_CameraDown": OUT_DIR / "camera_down.png",
    },
)
MOVIES = (PANEL, NAVBAR)


class Bits:
    """Big-endian bit packing, as a SWF RECT, MATRIX and shape records want it."""

    def __init__(self) -> None:
        self.bits: list[int] = []

    def put(self, value: int, count: int) -> None:
        value &= (1 << count) - 1  # two's complement for a signed field
        for i in range(count - 1, -1, -1):
            self.bits.append((value >> i) & 1)

    def bytes(self) -> bytes:
        padded = self.bits + [0] * (-len(self.bits) % 8)
        return bytes(
            int("".join(map(str, padded[i : i + 8])), 2)
            for i in range(0, len(padded), 8)
        )


def signed_bits(*values: int) -> int:
    return max(max(abs(v).bit_length() for v in values) + 1, 2)


def rect(width_px: int, height_px: int) -> bytes:
    xmax, ymax = width_px * 20, height_px * 20  # twips
    nbits = signed_bits(xmax, ymax)
    b = Bits()
    b.put(nbits, 5)
    for v in (0, xmax, 0, ymax):
        b.put(v, nbits)
    return b.bytes()


def tag(code: int, body: bytes) -> bytes:
    if len(body) < 0x3F:
        return struct.pack("<H", (code << 6) | len(body)) + body
    return struct.pack("<HI", (code << 6) | 0x3F, len(body)) + body


def picture_tags(png: Path, bitmap: int, shape: int, sprite: int, name: str) -> bytes:
    """A PNG as a bitmap, a shape filled with it at a pixel a pixel, and a sprite holding the
    shape, exported by name: DefineBitsLossless2, DefineShape, DefineSprite and ExportAssets."""
    from PIL import Image  # only where a movie has pictures

    image = Image.open(png).convert("RGBA")
    w, h = image.size
    # Format 5: ARGB, a pixel's colour premultiplied by its alpha, compressed with zlib.
    argb = bytearray()
    for r, g, b, a in image.getdata():
        argb += bytes([a, r * a // 255, g * a // 255, b * a // 255])
    out = tag(36, struct.pack("<HBHH", bitmap, 5, w, h) + zlib.compress(bytes(argb)))

    bits = Bits()
    # FILLSTYLEARRAY: one fill, a clipped bitmap (0x41), its id, and a matrix scaling the
    # bitmap's pixels to twips: 20 as 16.16 fixed point.
    fill = bytes([1, 0x41]) + struct.pack("<H", bitmap)
    m = Bits()
    m.put(1, 1)  # HasScale
    scale = 20 << 16
    nbits = signed_bits(scale)
    m.put(nbits, 5)
    m.put(scale, nbits)
    m.put(scale, nbits)
    m.put(0, 1)  # no rotate
    m.put(0, 5)  # translate bits: none
    fill += m.bytes()
    lines = bytes([0])  # LINESTYLEARRAY: none
    W, H = w * 20, h * 20
    bits.put(1, 4)  # NumFillBits
    bits.put(0, 4)  # NumLineBits
    # StyleChangeRecord: a move to 0,0 and fill style 0 is 1.
    bits.put(0, 1)  # not an edge
    bits.put(0b00011, 5)  # flags: StateFillStyle0 and StateMoveTo
    mbits = 2
    bits.put(mbits, 5)
    bits.put(0, mbits)
    bits.put(0, mbits)
    bits.put(1, 1)  # FillStyle0
    for dx, dy in ((W, 0), (0, H), (-W, 0), (0, -H)):
        # StraightEdgeRecord, a horizontal or vertical line.
        n = signed_bits(dx, dy)
        bits.put(1, 1)  # edge
        bits.put(1, 1)  # straight
        bits.put(n - 2, 4)
        bits.put(0, 1)  # not a general line
        vertical = dx == 0
        bits.put(1 if vertical else 0, 1)
        bits.put(dy if vertical else dx, n)
    bits.put(0, 6)  # EndShapeRecord
    body = struct.pack("<H", shape) + rect(w, h) + fill + lines + bits.bytes()
    out += tag(2, body)
    # The sprite: the shape placed at depth 1, one frame.
    place = struct.pack("<BHH", 0x02, 1, shape)  # PlaceObject2: has character
    sprite_tags = tag(26, place) + tag(1, b"") + tag(0, b"")
    out += tag(39, struct.pack("<HH", sprite, 1) + sprite_tags)
    out += tag(56, struct.pack("<HH", 1, sprite) + name.encode() + b"\0")
    return out


def shell(movie: Movie) -> bytes:
    """A one-frame SWF with the movie's imports and pictures and an empty frame script."""
    tags = bytearray()
    tags += tag(69, struct.pack("<I", 0))  # FileAttributes: AS1/2, no network
    character = 1
    for library, names in movie.imports.items():
        # ImportAssets2, as portraits.gfx imports $BodyFont: the library's url, two reserved
        # bytes (1, 0), the count, then each asset's id and name.
        body = library.encode() + b"\0" + bytes([1, 0]) + struct.pack("<H", len(names))
        for name in names:
            body += struct.pack("<H", character) + name.encode() + b"\0"
            character += 1
        tags += tag(71, body)
    for name, png in movie.pictures.items():
        tags += picture_tags(png, character, character + 1, character + 2, name)
        character += 3
    tags += tag(12, b"\0")  # DoAction: just ActionEnd, for FFDec to fill
    tags += tag(1, b"")  # ShowFrame
    tags += tag(0, b"")  # End
    rest = rect(*movie.stage) + struct.pack("<HH", FPS << 8, 1) + bytes(tags)
    return b"FWS" + bytes([SWF_VERSION]) + struct.pack("<I", 8 + len(rest)) + rest


def missing(source: str, compiled: str) -> list[str]:
    """What of the source's functions and calls to the plugin a decompiled script lacks."""
    wanted = re.findall(r"^function (\w+)", source, re.MULTILINE)
    wanted += re.findall(r'"(PhotoCam_\w+)"', source)
    return sorted({name for name in wanted if name not in compiled})


def ffdec(root: Path, *args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        ["java", "-jar", str(root / "ffdec-cli.jar"), *args],
        capture_output=True,
        text=True,
        check=False,
    )


def build(root: Path, movie: Movie) -> int:
    if not movie.script.exists():
        print(f"no {movie.script}")
        return 1
    absent = [str(png) for png in movie.pictures.values() if not png.exists()]
    if absent:
        print(
            f"{movie.name}: no {', '.join(absent)}; draw them first (python tools\\camera_icon.py)"
        )
        return 1
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory() as work:
        work_dir = Path(work)
        base = work_dir / "shell.swf"
        base.write_bytes(shell(movie))
        scripts = work_dir / "scripts"
        shutil.copytree(movie.source, scripts)
        # Into the work folder first, so a failed compile leaves the last movie as it was.
        made = work_dir / movie.out.name
        result = ffdec(root, "-importScript", str(base), str(made), str(scripts))
        if result.returncode != 0 or not made.exists():
            print(result.stdout, result.stderr)
            print(f"FFDec could not compile {movie.name}")
            return 1
        back = work_dir / "back"
        ffdec(root, "-export", "script", str(back), str(made))
        exported = list(back.rglob("DoAction.as"))
        text = (
            exported[0].read_text(encoding="utf-8", errors="replace")
            if exported
            else ""
        )
        lost = missing(movie.script.read_text(encoding="utf-8"), text)
        if not text or lost:
            print(result.stdout, result.stderr)
            print(
                f"the compiled {movie.name}'s script is not the source's: "
                + (", ".join(lost) if text else "it has none")
            )
            return 1
        shutil.copyfile(made, movie.out)
    print(f"built {movie.out} ({movie.out.stat().st_size} bytes)")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--ffdec", type=Path, default=FFDEC, help="FFDec's folder")
    parser.add_argument(
        "movies",
        nargs="*",
        metavar="movie",
        help="which: " + ", ".join(m.name for m in MOVIES),
    )
    args = parser.parse_args()
    known = {movie.name: movie for movie in MOVIES}
    unknown = [name for name in args.movies if name not in known]
    if unknown:
        print(f"no movie named {', '.join(unknown)}: there are {', '.join(known)}")
        return 1
    if not (args.ffdec / "ffdec-cli.jar").exists():
        print(f"no FFDec at {args.ffdec}; see this script's head")
        return 1
    failed = 0
    for movie in MOVIES:
        if not args.movies or movie.name in args.movies:
            failed |= build(args.ffdec, movie)
    return failed


if __name__ == "__main__":
    sys.exit(main())
