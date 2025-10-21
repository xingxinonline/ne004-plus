#!/usr/bin/env python3
import argparse
from pathlib import Path
from PIL import Image

TEMPLATE = """
#include "lvgl.h"

static const uint16_t {sym}_pixels[{w} * {h}] = {{
{data}
}};

const lv_image_dsc_t {sym} = {{
    .header = {{
        .magic = LV_IMAGE_HEADER_MAGIC,
        .cf = LV_COLOR_FORMAT_RGB565,
        .w = {w},
        .h = {h},
        .stride = {w} * 2,
    }},
    .data_size = sizeof({sym}_pixels),
    .data = (const uint8_t*){sym}_pixels,
}};
"""


def to_rgb565(pixel):
    r, g, b, a = pixel if len(pixel) == 4 else (*pixel, 255)
    # premultiply or ignore alpha: here we just ignore alpha (treat as opaque)
    r5 = (r >> 3) & 0x1F
    g6 = (g >> 2) & 0x3F
    b5 = (b >> 3) & 0x1F
    return (r5 << 11) | (g6 << 5) | b5


def convert_png_to_c(png_path: Path, out_dir: Path, symbol: str):
    im = Image.open(png_path).convert("RGBA")
    w, h = im.size
    pixels = list(im.getdata())
    rgb565 = [to_rgb565(p) for p in pixels]
    # Format as 16 per line
    lines = []
    for i in range(0, len(rgb565), 16):
        chunk = rgb565[i:i+16]
        lines.append(", ".join(f"0x{v:04x}" for v in chunk))
    data = ",\n".join(lines)

    c_code = TEMPLATE.format(sym=symbol, w=w, h=h, data=data)
    out_dir.mkdir(parents=True, exist_ok=True)
    out_path = out_dir / f"{symbol}.c"
    out_path.write_text(c_code)
    print(f"Generated {out_path}")


def gen_symbol(name: str) -> str:
    # make a safe C symbol
    s = name.lower()
    s = ''.join(ch if ch.isalnum() else '_' for ch in s)
    if s[0].isdigit():
        s = '_' + s
    return s


def main():
    ap = argparse.ArgumentParser(
        description="Convert PNG to LVGL RGB565 C asset")
    ap.add_argument("inputs", nargs="+", help="PNG files to convert")
    ap.add_argument("--out", default="../Assets/generated",
                    help="Output directory")
    ap.add_argument("--prefix", default="img_", help="Symbol name prefix")
    args = ap.parse_args()

    out_dir = Path(args.out).resolve()
    for p in args.inputs:
        png_path = Path(p)
        sym = gen_symbol(args.prefix + png_path.stem)
        convert_png_to_c(png_path, out_dir, sym)


if __name__ == "__main__":
    main()
