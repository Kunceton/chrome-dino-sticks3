#!/usr/bin/env python3
"""从 Chromium 官方 2x sprite 图集提取素材并缩放到 StickS3 目标尺寸（LDPI x0.9）。
坐标来源：offline-sprite-definitions.js HDPI 段 + offline.js Trex.animFrames。
缩放方法：LANCZOS 降采样后按 50% 阈值还原为单色像素，保证锐利边缘。
产出：assets/sprites/*.png（RGBA，墨色像素+透明底），供设计稿和固件 C 数组转换共用。"""
from PIL import Image
import os

HERE = os.path.dirname(os.path.abspath(__file__))
ATLAS = os.path.join(HERE, "..", "assets", "offline-sprite-2x.png")
OUT = os.path.join(HERE, "..", "assets", "sprites")
os.makedirs(OUT, exist_ok=True)

S = 0.45  # 目标尺寸系数：LDPI x0.9 = HDPI x0.45
INK = (83, 83, 83, 255)  # 原版深灰

atlas = Image.open(ATLAS).convert("RGBA")

def extract(name, x, y, w, h, lum_max=128, color=INK):
    crop = atlas.crop((x, y, x + w, y + h))
    tw, th = round(w * S), round(h * S)
    la = crop.convert("LA").resize((tw, th), Image.LANCZOS)
    out = Image.new("RGBA", (tw, th), (0, 0, 0, 0))
    src, dst = la.load(), out.load()
    for j in range(th):
        for i in range(tw):
            lum, alpha = src[i, j]
            if alpha > 128 and lum < lum_max:
                dst[i, j] = color
    out.save(os.path.join(OUT, name + ".png"))
    print(f"{name}: {tw}x{th}")

# ---- T-REX（HDPI 原点 1678,2；帧相对坐标 = LDPI x2，尺寸 88x94 / 下蹲 118x50） ----
TX, TY = 1678, 2
extract("trex_wait1",   TX + 88,  TY, 88, 94)
extract("trex_wait2",   TX + 0,   TY, 88, 94)
extract("trex_run1",    TX + 176, TY, 88, 94)
extract("trex_run2",    TX + 264, TY, 88, 94)
extract("trex_jump",    TX + 0,   TY, 88, 94)
extract("trex_crash",   TX + 440, TY, 88, 94)
extract("trex_duck1",   TX + 528, 42, 118, 50)
extract("trex_duck2",   TX + 646, 42, 118, 50)

# ---- 障碍（HDPI） ----
extract("cactus_s1", 446, 2, 34, 70)    # 小仙人掌 单株
extract("cactus_s2", 480, 2, 68, 70)    # 双株
extract("cactus_s3", 548, 2, 102, 70)   # 三株
extract("cactus_l1", 652, 2, 50, 100)   # 大仙人掌 单株
extract("cactus_l2", 702, 2, 100, 100)  # 双株
extract("cactus_l3", 802, 2, 150, 100)  # 三株
extract("ptero1", 260, 2, 92, 80)       # 翼龙 帧1
extract("ptero2", 352, 2, 92, 80)       # 翼龙 帧2

# ---- 场景 ----
# 云在原版图集中是浅灰色，放宽亮度阈值并保留浅灰色调
extract("cloud", 166, 2, 92, 28, lum_max=240, color=(194, 194, 194, 255))
# 地面纹理条：HDPI y=104 高 24，取 533px 宽缩到 240 供整屏平铺参考
extract("ground_slice", 2, 104, 533, 24)

# 后处理：睁眼帧补 2x2 眼睛洞（原版 LDPI 为 2x2 白洞，降采样丢了/只剩 1px，规格化回来）
# 位置按 HDPI 图集实测换算：约 x19-20, y4-5（40x42 目标坐标）
# wait1 是闭眼帧，不补；crash 是 X 眼，不动
for name in ("trex_run1", "trex_run2", "trex_wait2", "trex_jump"):
    fp = os.path.join(OUT, name + ".png")
    im = Image.open(fp)
    px = im.load()
    for ey in (4, 5):
        for ex in (19, 20):
            px[ex, ey] = (0, 0, 0, 0)
    im.save(fp)
    print(name, "eye carved")

print("done ->", os.path.abspath(OUT))
