#!/usr/bin/env python3
"""assets/sprites/*.png → src/sprites.h（1bpp 位图 C 数组，行主序高位在前）"""
from PIL import Image
import os

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "..", "assets", "sprites")
OUT = os.path.join(HERE, "..", "src", "sprites.h")

names = sorted(f[:-4] for f in os.listdir(SRC) if f.endswith(".png"))

lines = [
    "// 生成物，勿手改：python3 tools/sprites_to_c.py",
    "// 1bpp 行主序高位在前；像素=1 表示实体像素（渲染时按昼夜配色着色）",
    "#pragma once",
    "#include <cstdint>",
    "",
    "struct Sprite { const uint8_t* data; uint8_t w; uint8_t h; };",
    "",
]
for name in names:
    im = Image.open(os.path.join(SRC, name + ".png")).convert("RGBA")
    w, h = im.size
    px = im.load()
    row_bytes = (w + 7) // 8
    data = []
    for y in range(h):
        for bx in range(row_bytes):
            byte = 0
            for bit in range(8):
                x = bx * 8 + bit
                if x < w and px[x, y][3] > 128:
                    byte |= 0x80 >> bit
            data.append(byte)
    arr = ", ".join(f"0x{b:02X}" for b in data)
    lines.append(f"static const uint8_t spr_{name}_data[] = {{{arr}}};")
    lines.append(f"static const Sprite spr_{name} = {{spr_{name}_data, {w}, {h}}};")
    lines.append("")

with open(OUT, "w") as f:
    f.write("\n".join(lines))
print(f"{len(names)} sprites -> {OUT}")
