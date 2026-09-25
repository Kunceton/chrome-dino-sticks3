// 渲染器实现 —— 布局依据 docs/ui/3_gameplay.png；素材为 1bpp 位图按昼夜配色着色
#include <M5Unified.h>
#include <M5GFX.h>
#include "renderer.h"
#include "sprites.h"

// 昼夜配色（FR-G8）
struct Palette { uint16_t bg, fg, dim, ground; };
static const Palette kDay   = {0xF7BE, 0x528A, 0x9CD3, 0x528A};   // 白底深灰
static const Palette kNight = {0x0841, 0xE71C, 0x4208, 0xE71C};   // 深蓝黑底浅灰

static LGFX_Sprite* s_spr = nullptr;
static bool s_direct = false;   // sprite 申请失败时降级直绘

static lgfx::LGFX_Device& dst() {
  return *static_cast<lgfx::LGFX_Device*>(s_spr ? (lgfx::LGFX_Device*)s_spr : (lgfx::LGFX_Device*)&M5.Display);
}

bool renderer_init() {
  s_spr = new LGFX_Sprite(&M5.Display);
  s_spr->setColorDepth(16);
  if (s_spr->createSprite(240, 135) == nullptr) {
    delete s_spr;
    s_spr = nullptr;
    s_direct = true;   // 降级：直接绘制（可能轻微闪烁），串口告警
    Serial.println("WARN: sprite alloc failed, direct draw");
    return false;
  }
  return true;
}

// 1bpp 位图绘制（行主序高位在前，与 tools/sprites_to_c.py 一致）
static void draw_sprite(int x, int y, const Sprite& s, uint16_t color) {
  auto& d = dst();
  int row_bytes = (s.w + 7) / 8;
  for (int j = 0; j < s.h; j++) {
    for (int i = 0; i < s.w; i++) {
      uint8_t byte = s.data[j * row_bytes + i / 8];
      if (byte & (0x80 >> (i % 8))) d.drawPixel(x + i, y + j, color);
    }
  }
}

// 地面：线 + 滚动的小石头凸起 + 沙土点（程序化生成，对齐原版 horizon 细节）
static void draw_ground(const Palette& pal, float offset) {
  auto& d = dst();
  d.drawFastHLine(0, kGroundLineY, 240, pal.ground);
  // 预生成 480px 虚拟地面带（确定性 LCG，和原版一样的稀疏凸起/沙点）
  static bool init = false;
  static int bump_pos[24];       // 凸起中心位置
  static int n_bumps = 0;
  static uint8_t dot_y[480];     // 0=无沙点，否则线下偏移
  if (!init) {
    uint32_t st = 7;
    int last_bump = -30;
    for (int i = 0; i < 480 && n_bumps < 24; i++) {
      st = st * 1664525u + 1013904223u;
      uint32_t r = st >> 16;
      // 凸起：间距 ≥18px，约每 40px 一个
      if ((r % 100) < 3 && i - last_bump >= 18) {
        bump_pos[n_bumps++] = i;
        last_bump = i;
      }
      dot_y[i] = ((r >> 8) % 100 < 12) ? (uint8_t)(3 + ((r >> 14) % 4)) : 0;
    }
    init = true;
  }
  int shift = (int)offset % 480;
  if (shift < 0) shift += 480;
  for (int b = 0; b < n_bumps; b++) {
    // 凸起：5px 宽小丘（剖面 1-2-2-2-1，中间一行最高）
    int sx = (bump_pos[b] - shift + 480) % 480 - 2;
    if (sx >= -4 && sx < 240) {
      for (int i = 0; i < 5; i++) {
        int h = (i == 0 || i == 4) ? 1 : 2;
        int x = sx + i;
        if (x >= 0 && x < 240) {
          for (int j = 1; j <= h; j++) d.drawPixel(x, kGroundLineY - j, pal.ground);
        }
      }
    }
  }
  for (int x = 0; x < 240; x++) {
    int si = (x + shift) % 480;
    if (dot_y[si]) d.drawPixel(x, kGroundLineY + dot_y[si], pal.dim);
  }
}

static void draw_score(const Palette& pal, uint32_t score, uint32_t hi, bool blink) {
  auto& d = dst();
  char buf[16];
  d.setTextSize(1);
  d.setTextDatum(top_right);
  snprintf(buf, sizeof(buf), "%05lu", (unsigned long)score);
  int score_w = d.textWidth(buf);
  d.setTextColor(blink ? pal.bg : pal.fg, pal.bg);   // 里程碑闪烁：分数隐身一瞬
  d.drawString(buf, 232, 8);
  if (hi > 0) {
    snprintf(buf, sizeof(buf), "HI %05lu", (unsigned long)hi);
    d.setTextColor(pal.dim, pal.bg);
    d.drawString(buf, 232 - score_w - 10, 8);   // HI 在实时分左侧，避免重叠
  }
}

void renderer_game(const DinoState& dino, const ObstaclePool& pool, const GameSession& s,
                   const Cloud* clouds, int n_clouds,
                   uint32_t hi, int run_frame, int ptero_frame, float ground_offset,
                   bool crashed, bool score_blink, bool waiting) {
  const Palette& pal = s.night ? kNight : kDay;
  auto& d = dst();
  d.fillScreen(pal.bg);

  draw_ground(pal, ground_offset);

  // 云（FR-G9，浅灰淡化处理）
  for (int i = 0; i < n_clouds; i++) {
    if (clouds[i].active) draw_sprite((int)clouds[i].x, clouds[i].y, spr_cloud, pal.dim);
  }

  // 障碍
  for (int i = 0; i < kMaxObstacles; i++) {
    const Obstacle& o = pool.items[i];
    if (!o.active) continue;
    const Sprite* sp = &spr_cactus_s1;
    if (o.type == ObstacleType::CactusSmall) {
      sp = o.size >= 3 ? &spr_cactus_s3 : (o.size == 2 ? &spr_cactus_s2 : &spr_cactus_s1);
    } else if (o.type == ObstacleType::CactusLarge) {
      sp = o.size >= 3 ? &spr_cactus_l3 : (o.size == 2 ? &spr_cactus_l2 : &spr_cactus_l1);
    } else {
      sp = ptero_frame ? &spr_ptero2 : &spr_ptero1;
    }
    draw_sprite((int)o.x, (int)o.y, *sp, pal.fg);
  }

  // 恐龙
  const Sprite* ds;
  int dy = (int)dino.y;
  if (crashed) {
    ds = &spr_trex_crash;
  } else if (waiting && !dino.jumping) {
    ds = ptero_frame ? &spr_trex_wait1 : &spr_trex_wait2;   // 待机：默认睁眼，眨眼相位移位时闭眼
  } else if (dino.ducking && !dino.jumping) {
    ds = run_frame ? &spr_trex_duck2 : &spr_trex_duck1;
    dy = kGroundLineY - kDinoDuckH;
  } else if (dino.jumping) {
    ds = &spr_trex_jump;
  } else {
    ds = run_frame ? &spr_trex_run2 : &spr_trex_run1;
  }
  draw_sprite((int)kDinoX, dy, *ds, pal.fg);

  draw_score(pal, s.score, hi, score_blink);
}

void renderer_present() {
  if (s_spr) s_spr->pushSprite(&M5.Display, 0, 0);
}

LGFX_Sprite* renderer_canvas() { return s_spr; }

void renderer_draw_sprite_screen(int x, int y, const Sprite& s, uint16_t color) {
  int row_bytes = (s.w + 7) / 8;
  for (int j = 0; j < s.h; j++) {
    for (int i = 0; i < s.w; i++) {
      uint8_t byte = s.data[j * row_bytes + i / 8];
      if (byte & (0x80 >> (i % 8))) M5.Display.drawPixel(x + i, y + j, color);
    }
  }
}
