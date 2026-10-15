"""Build photo cam's panel movie, photocam.swf, from its ActionScript 2 source.

The game's interface is Scaleform GFx 3, which loads a plain SWF 8 like its
own movies (DAOLib's dao/Gui.h: a plugin's screen finds <movie>.swf beside its
dll). So the movie is built from text with no Flash authoring, as
DAOTurnBased's tools/movie.py builds its turn bar (read 2026-10-15):

  1. this script writes a shell: the stage; the game's two fonts imported by
     name from its font library, as its own movies import them; and the
     options screen's checkbox, slider and button imported by name from
     optionsmenu.swf, which exports them (read with FFDec 2026-10-15), so the
     panel is drawn with the game's own controls; and an empty frame script;
  2. JPEXS FFDec compiles ui/panel/frame_1/DoAction.as into that script;
  3. FFDec decompiles the result, and the build fails unless every function
     and every PhotoCam_ call of the source came back.

    python tools\\movie.py                       into build\\ui\\photocam.swf
    python tools\\movie.py --ffdec C:\\...\\ffdec  FFDec elsewhere than C:\\Modding

FFDec is https://github.com/jindrapetrik/jpexs-decompiler, run on Java,
installed in C:\\Modding for DAOTurnBased (2026-09-30).
"""

from __future__ import annotations

import argparse
import re
import shutil
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
SOURCE = REPO / "ui" / "panel"
SCRIPT = SOURCE / "frame_1" / "DoAction.as"
OUT = REPO / "build" / "ui" / "photocam.swf"
FFDEC = Path(r"C:\Modding\ffdec_26.3.0")

SWF_VERSION = 8  # the game's movies are SWF 8
FPS = 30
STAGE = (1280, 720)  # the panel stands by the view's corner, measured as it runs
# What is imported, by library: the game's own movies import the fonts so from
# gfxfontlib.swf, and the options screen exports its controls by these names.
IMPORTS = {
    "gfxfontlib.swf": ["$TitleFont", "$BodyFont"],
    "optionsmenu.swf": ["checkbox_mc", "slider_m", "ArrowButton"],
}


class Bits:
    """Big-endian bit packing, as a SWF RECT wants it."""

    def __init__(self) -> None:
        self.bits: list[int] = []

    def put(self, value: int, count: int) -> None:
        for i in range(count - 1, -1, -1):
            self.bits.append((value >> i) & 1)

    def bytes(self) -> bytes:
        padded = self.bits + [0] * (-len(self.bits) % 8)
        return bytes(
            int("".join(map(str, padded[i : i + 8])), 2)
            for i in range(0, len(padded), 8)
        )


def rect(width_px: int, height_px: int) -> bytes:
    xmax, ymax = width_px * 20, height_px * 20  # twips
    nbits = max(xmax.bit_length(), ymax.bit_length()) + 1  # signed
    b = Bits()
    b.put(nbits, 5)
    for v in (0, xmax, 0, ymax):
        b.put(v, nbits)
    return b.bytes()


def tag(code: int, body: bytes) -> bytes:
    if len(body) < 0x3F:
        return struct.pack("<H", (code << 6) | len(body)) + body
    return struct.pack("<HI", (code << 6) | 0x3F, len(body)) + body


def shell() -> bytes:
    """A one-frame SWF with the imports and an empty frame script."""
    tags = bytearray()
    tags += tag(69, struct.pack("<I", 0))  # FileAttributes: AS1/2, no network
    character = 1
    for library, names in IMPORTS.items():
        # ImportAssets2, as portraits.gfx imports $BodyFont: the library's url,
        # two reserved bytes (1, 0), the count, then each asset's id and name.
        body = library.encode() + b"\0" + bytes([1, 0]) + struct.pack("<H", len(names))
        for name in names:
            body += struct.pack("<H", character) + name.encode() + b"\0"
            character += 1
        tags += tag(71, body)
    tags += tag(12, b"\0")  # DoAction: just ActionEnd, for FFDec to fill
    tags += tag(1, b"")  # ShowFrame
    tags += tag(0, b"")  # End
    rest = rect(*STAGE) + struct.pack("<HH", FPS << 8, 1) + bytes(tags)
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


def build(root: Path) -> int:
    if not SCRIPT.exists():
        print(f"no {SCRIPT}")
        return 1
    OUT.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory() as work:
        work_dir = Path(work)
        base = work_dir / "shell.swf"
        base.write_bytes(shell())
        scripts = work_dir / "scripts"
        shutil.copytree(SOURCE, scripts)
        # Into the work folder first, so a failed compile leaves the last movie as it was.
        made = work_dir / OUT.name
        result = ffdec(root, "-importScript", str(base), str(made), str(scripts))
        if result.returncode != 0 or not made.exists():
            print(result.stdout, result.stderr)
            print("FFDec could not compile the panel")
            return 1
        back = work_dir / "back"
        ffdec(root, "-export", "script", str(back), str(made))
        exported = list(back.rglob("DoAction.as"))
        text = (
            exported[0].read_text(encoding="utf-8", errors="replace")
            if exported
            else ""
        )
        lost = missing(SCRIPT.read_text(encoding="utf-8"), text)
        if not text or lost:
            print(result.stdout, result.stderr)
            print(
                "the compiled script is not the source's: "
                + (", ".join(lost) if text else "it has none")
            )
            return 1
        shutil.copyfile(made, OUT)
    print(f"built {OUT} ({OUT.stat().st_size} bytes)")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--ffdec", type=Path, default=FFDEC, help="FFDec's folder")
    args = parser.parse_args()
    if not (args.ffdec / "ffdec-cli.jar").exists():
        print(f"no FFDec at {args.ffdec}; see this script's head")
        return 1
    return build(args.ffdec)


if __name__ == "__main__":
    sys.exit(main())
