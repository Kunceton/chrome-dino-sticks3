#!/usr/bin/env python3
"""生成 Chrome Dino @ StickS3 的 240x135 UI 设计稿（输出放大4倍便于查看）
排版原则：所有居中/靠右均用字体真实宽度计算；屏幕边距统一 8px。"""
from PIL import Image, ImageDraw, ImageFont
import os
import random

W, H, SCALE = 240, 135, 4
MARGIN = 8
OUT = os.path.join(os.path.dirname(__file__), "..", "docs", "ui")
os.makedirs(OUT, exist_ok=True)

# ---------- 调色板（FC 风格） ----------
INK      = (83, 83, 83)     # 原版深灰
WHITE    = (247, 247, 247)  # 原版白昼底
BLACK    = (10, 10, 30)     # FC 夜空深蓝黑
RED      = (228, 59, 68)    # FC 红
BLUE     = (64, 116, 228)   # FC 蓝
YELLOW   = (248, 216, 76)
GRAY     = (150, 150, 160)
DIM      = (90, 90, 100)

FONT = ImageFont.load_default()
HERE = os.path.dirname(os.path.abspath(__file__))
SPRITES = os.path.join(HERE, "..", "assets", "sprites")

def load_sprite(name):
    return Image.open(os.path.join(SPRITES, name + ".png"))

def tinted(im, color):
    out = im.copy()
    px = out.load()
    for j in range(out.height):
        for i in range(out.width):
            if px[i, j][3] > 0:
                px[i, j] = color + (255,)
    return out

def new_img(bg):
    return Image.new("RGB", (W, H), bg)

def text(img, x, y, s, color, scale=1, center=None, right=None):
    """按真实字形宽度绘制；center=水平中心x，right=右边缘x（与 x 互斥）"""
    bbox = FONT.getbbox(s)
    w, h = bbox[2] - bbox[0], bbox[3] - bbox[1]
    tmp = Image.new("RGBA", (max(w, 1), max(h, 1)), (0, 0, 0, 0))
    d = ImageDraw.Draw(tmp)
    d.text((-bbox[0], -bbox[1]), s, font=FONT, fill=color + (255,))
    if scale > 1:
        tmp = tmp.resize((tmp.width * scale, tmp.height * scale), Image.NEAREST)
    if center is not None:
        x = center - tmp.width // 2
    elif right is not None:
        x = right - tmp.width
    img.paste(tmp, (x, y), tmp)
    return tmp.width, tmp.height

def draw_pattern(img, ox, oy, rows, color, px=1):
    d = ImageDraw.Draw(img)
    for r, row in enumerate(rows):
        for c, ch in enumerate(row):
            if ch == '#':
                d.rectangle([ox + c*px, oy + r*px, ox + (c+1)*px - 1, oy + (r+1)*px - 1], fill=color)

def draw_sprite_scaled(img, ox, oy, rows, color, tw, th):
    """按目标像素尺寸缩放绘制（模拟实机从 Chromium sprite 等比缩放的效果）"""
    w = max(len(r) for r in rows)
    h = len(rows)
    tmp = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    draw_pattern(tmp, 0, 0, rows, color)
    tmp = tmp.resize((tw, th), Image.NEAREST)
    img.paste(tmp, (ox, oy), tmp)

# ---------- 像素素材 ----------
DINO = [
"        #######",
"       #########",
"       ## ######",
"       #########",
"       ####",
"       ########",
"#     #####",
"#    ######",
"#   ##########",
"#  ######### ##",
"# #####  #####",
"########   ###",
" ########",
"  #######",
"   ######",
"   ##  ##",
"   #    #",
"   ##   ##",
]

CACTUS_S = [
"  ##",
"  ##",
"  ##",
"## ##",
"## ## ##",
"## ## ##",
"## ### ##",
"  ####",
"  ##",
"  ##",
"  ##",
]

BIRD = [
"     ##",
"     ###",
"##   ####",
"##########  ##",
"  ###########",
"     #####",
]

CLOUD = [
"   ####",
" ########",
"##########",
]

def ground(img, y, color):
    d = ImageDraw.Draw(img)
    d.line([0, y, W, y], fill=color)
    random.seed(7)
    for _ in range(12):  # 地面小石子纹理
        x = random.randint(0, W - 4)
        d.point([(x, y + 4), (x + 2, y + 7)], fill=color)

def starfield(img):
    random.seed(42)
    d = ImageDraw.Draw(img)
    for _ in range(28):
        x, y = random.randint(0, W - 1), random.randint(0, H - 1)
        d.point((x, y), fill=(random.randint(80, 200),) * 3)

def measure(s, scale=1):
    bbox = FONT.getbbox(s)
    return (bbox[2] - bbox[0]) * scale, (bbox[3] - bbox[1]) * scale

# ================= 1. 标题画面 =================
DARK_BLUE = (28, 48, 140)
img = new_img(BLACK)
starfield(img)
# 第一行：CHROME（蓝色 + 深蓝描边），真实宽度居中
text(img, 0, 12, "CHROME", DARK_BLUE, scale=3, center=W//2 + 2)
text(img, 0, 10, "CHROME", BLUE, scale=3, center=W//2)
# 第二行：小恐龙 + DINO 同一行，组合整体居中（DINO 白字红影）
dino_tw, dino_th = 22, 27
dino_txt_w, _ = measure("DINO", scale=3)
gap = 8
group_w = dino_tw + gap + dino_txt_w
gx = (W - group_w) // 2
mini = tinted(load_sprite("trex_run1"), YELLOW).resize((dino_tw, dino_th), Image.NEAREST)
img.paste(mini, (gx, 38), mini)
text(img, gx + dino_tw + gap + 2, 38, "DINO", RED, scale=3)
text(img, gx + dino_tw + gap, 36, "DINO", WHITE, scale=3)
# 菜单：两项各自水平居中，光标跟随选中项
m1w, _ = measure("GAME START")
text(img, 0, 92, "GAME START", WHITE, center=W//2)
text(img, 0, 104, "SCORE RANK", GRAY, center=W//2)
text(img, W//2 - m1w//2 - 14, 92, ">", RED)
text(img, 0, 122, "PRESS BTN-A", GRAY, center=W//2)
img.resize((W*SCALE, H*SCALE), Image.NEAREST).save(os.path.join(OUT, "1_title.png"))

# ================= 2. 选名画面（重设计） =================
def name_pick(name, fname):
    img = new_img(BLACK)
    starfield(img)
    text(img, 0, 24, "YOUR NAME IS", WHITE, scale=2, center=W//2)
    # 名字显示区：居中大名 + 下方横线 + 呼吸闪烁输入块
    line_y = 76
    line_x0, line_x1 = W//2 - 55, W//2 + 55
    d = ImageDraw.Draw(img)
    d.line([line_x0, line_y, line_x1, line_y], fill=GRAY)
    if name:
        nw, nh = text(img, 0, line_y - 22, name, YELLOW, scale=2, center=W//2)
        cursor_x = W//2 + nw // 2 + 4
        cursor_y = line_y - 22
    else:
        # 首次游玩：空，光标在横线最左，高度仍与名字字号一致
        bb = FONT.getbbox("Ag")
        nh = (bb[3] - bb[1]) * 2
        cursor_x = line_x0 + 4
        cursor_y = line_y - 22
    # 输入块：高度与名字字形一致（静态稿画常亮态，实机为呼吸闪烁）
    d.rectangle([cursor_x, cursor_y, cursor_x + 9, cursor_y + nh], fill=YELLOW)
    # 底部左右分栏
    text(img, MARGIN, 118, "BtnB: NEW PLAYER", GRAY)
    text(img, 0, 118, "BtnA: START!", WHITE, right=W - MARGIN)
    img.resize((W*SCALE, H*SCALE), Image.NEAREST).save(os.path.join(OUT, fname))

name_pick("SwiftFox", "2_name_pick.png")       # 有上轮玩家名
name_pick("", "2b_name_pick_first.png")        # 首次游玩为空

# ================= 3. 游戏中（白昼，真实官方素材 ×0.9） =================
img = new_img(WHITE)
GY = 126   # 地面线 y（原版底留10px × 0.9）
ground(img, GY, INK)
img.paste(load_sprite("ground_slice"), (0, GY + 2), load_sprite("ground_slice"))
img.paste(load_sprite("trex_run1"), (24, GY - 42), load_sprite("trex_run1"))
img.paste(load_sprite("cactus_s3"), (140, GY - 32), load_sprite("cactus_s3"))
img.paste(load_sprite("cactus_l1"), (105, GY - 45), load_sprite("cactus_l1"))
img.paste(load_sprite("ptero1"), (195, 68), load_sprite("ptero1"))   # 中空翼龙（原版 yPos75 × 0.9）
img.paste(load_sprite("cloud"), (60, 24), load_sprite("cloud"))
img.paste(load_sprite("cloud"), (170, 42), load_sprite("cloud"))
# 分数靠右对齐：当前分贴右缘，HI 在其左
sw, _ = text(img, 0, 8, "00340", INK, right=W - MARGIN)
text(img, 0, 8, "HI 01250", GRAY, right=W - MARGIN - sw - 8)
img.resize((W*SCALE, H*SCALE), Image.NEAREST).save(os.path.join(OUT, "3_gameplay.png"))

# ================= 4. 结算画面 =================
img = new_img(WHITE)
ground(img, GY, INK)
img.paste(load_sprite("trex_crash"), (24, GY - 42), load_sprite("trex_crash"))
img.paste(load_sprite("cactus_s1"), (140, GY - 32), load_sprite("cactus_s1"))
d = ImageDraw.Draw(img)
d.rectangle([35, 18, 205, 84], fill=WHITE, outline=INK, width=2)
text(img, 0, 24, "GAME OVER", INK, scale=2, center=W//2)
text(img, 0, 48, "SCORE 00340  NEW RECORD!", RED, center=W//2)
text(img, 0, 66, "BTN-A: RETRY   BTN-B: TITLE", GRAY, center=W//2)
img.resize((W*SCALE, H*SCALE), Image.NEAREST).save(os.path.join(OUT, "4_game_over.png"))

# ================= 5. 积分榜 =================
img = new_img(BLACK)
starfield(img)
text(img, 0, 10, "SCORE RANK", YELLOW, scale=2, center=W//2)
rows = [("1.", "BravePanda", "09870"), ("2.", "SwiftFox", "00340"),
        ("3.", "MightyOwl", "00125"), ("4.", "-", "00000"), ("5.", "-", "00000")]
y = 40
for rank, name, score in rows:
    hl = (rank == "2.")
    if hl:
        d = ImageDraw.Draw(img)
        d.rectangle([20, y - 2, W - 20, y + 10], outline=RED)
    c = RED if hl else WHITE
    text(img, 30, y, rank, c)
    text(img, 55, y, name, c)
    text(img, 0, y, score, c, right=W - 30)
    y += 14
text(img, 0, 120, "BTN-A: BACK", GRAY, center=W//2)
img.resize((W*SCALE, H*SCALE), Image.NEAREST).save(os.path.join(OUT, "5_leaderboard.png"))

print("done ->", os.path.abspath(OUT))
