// 恐龙跳跃物理 —— 行为对齐 assets/reference/offline.js 的 startJump/endJump/updateJump
#include "game_logic.h"

DinoState dino_create() {
  return {kDinoStandTopY, 0.0f, false, false, false, false};
}

void dino_update(DinoState& d, bool jump_pressed, bool duck_held, float speed, float frames) {
  if (jump_pressed && !d.jumping) {              // startJump
    d.jumping = true;
    d.vy = kJumpV0 - speed / 10.0f;
    d.reached_min_height = false;
    d.speed_drop = false;
  }
  d.ducking = duck_held && !d.jumping;

  if (d.jumping) {
    if (duck_held) d.speed_drop = true;          // 空中按下蹲 → 速降
    d.y += d.vy * (d.speed_drop ? kSpeedDropCoeff : 1.0f) * frames;
    d.vy += kGravity * frames;
    if (d.y < kDinoStandTopY - kMinJumpRise) d.reached_min_height = true;
    // 顶点钳制：过了最小高度且仍在快速上升且头顶越过上界 → 截断（原版 MAX_JUMP_HEIGHT）
    if (d.reached_min_height && d.vy < kDropVelocity && d.y < kMaxJumpTopY) {
      d.vy = kDropVelocity;
    }
    if (d.y >= kDinoStandTopY) {                 // 落地
      d.y = kDinoStandTopY;
      d.vy = 0.0f;
      d.jumping = false;
      d.speed_drop = false;
    }
    if (d.y < 0.0f) d.y = 0.0f;                  // 防御：离散积分的极端过冲不出屏
  }
}

void dino_cut_jump(DinoState& d) {
  // 原版 endJump：过了最小升高后松键才截断；未过最小升高不截（保证轻点也有 27px）
  if (d.jumping && d.reached_min_height && d.vy < kDropVelocity) {
    d.vy = kDropVelocity;
  }
}

// ---- 碰撞盒（LDPI 官方值 ×0.9 取整） ----
static const float kTrexRunBoxes[][4] = {
  {20, 0, 15, 14}, {1, 16, 27, 8}, {9, 32, 13, 7},
  {1, 22, 26, 5}, {5, 27, 19, 4}, {8, 31, 14, 4},
};
static const float kTrexDuckBoxes[][4] = {{1, 16, 50, 23}};
static const float kCactusSBoxes[][4] = {{0, 6, 5, 24}, {4, 0, 5, 31}, {9, 4, 6, 13}};
static const float kCactusLBoxes[][4] = {{0, 11, 6, 34}, {7, 0, 6, 44}, {12, 9, 9, 34}};
static const float kPteroBoxes[][4] = {
  {14, 14, 14, 5}, {16, 19, 22, 5}, {2, 13, 4, 3}, {5, 9, 4, 6}, {9, 7, 5, 8},
};

static int fill_boxes(const float (*src)[4], int n, float ox, float oy, Rect* out, int max_out) {
  if (n > max_out) n = max_out;
  for (int i = 0; i < n; i++) {
    out[i] = {ox + src[i][0], oy + src[i][1], src[i][2], src[i][3]};
  }
  return n;
}

int dino_collision_boxes(const DinoState& d, float dino_x, Rect* out, int max_out) {
  if (d.ducking && !d.jumping) {
    // 下蹲素材锚定脚底：头顶 = 地面 - 蹲高
    return fill_boxes(kTrexDuckBoxes, 1, dino_x, kGroundLineY - kDinoDuckH, out, max_out);
  }
  return fill_boxes(kTrexRunBoxes, 6, dino_x, d.y, out, max_out);
}

int obstacle_collision_boxes(ObstacleType t, float ox, float oy, float total_w, Rect* out, int max_out) {
  const float (*src)[4];
  int n;
  switch (t) {
    case ObstacleType::CactusSmall: src = kCactusSBoxes; n = 3; break;
    case ObstacleType::CactusLarge: src = kCactusLBoxes; n = 3; break;
    case ObstacleType::Ptero:       src = kPteroBoxes;   n = 5; break;
    default: return 0;
  }
  float unit_w = (t == ObstacleType::CactusSmall) ? 15.0f : (t == ObstacleType::CactusLarge) ? 22.0f : 41.0f;
  if (n > max_out) n = max_out;
  for (int i = 0; i < n; i++) {
    float bx = src[i][0], bw = src[i][2];
    if (total_w > unit_w + 1.0f && n == 3) {
      // 原版簇拉伸：中盒拉宽到中间，右盒移到右缘
      if (i == 1) bw = total_w - src[0][2] - src[2][2];
      if (i == 2) bx = total_w - src[2][2];
    }
    out[i] = {ox + bx, oy + src[i][1], bw, src[i][3]};
  }
  return n;
}

static bool rects_overlap(const Rect& a, const Rect& b) {
  return a.x < b.x + b.w && a.x + a.w > b.x &&
         a.y < b.y + b.h && a.y + a.h > b.y;
}

bool boxes_collide(const Rect* a, int na, const Rect* b, int nb) {
  for (int i = 0; i < na; i++)
    for (int j = 0; j < nb; j++)
      if (rects_overlap(a[i], b[j])) return true;
  return false;
}
