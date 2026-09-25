// 障碍生成器实现 —— 公式逐行对应原版 getGap / Obstacle.init / duplicateObstacleCheck
#include "obstacle.h"

// 类型参数表（原版值 → 缩放后）：minGap 120×0.9=108，翼龙 150×0.9=135
// multipleSpeed（簇解禁速度）：小 4×0.4=1.6，大 7×0.4=2.8，翼龙 999（恒单只）
static const float kMinGap[] = {108.0f, 108.0f, 135.0f};
static const float kMultipleSpeed[] = {2.2f, 3.85f, 999.0f};  // 原版 4/7 × 0.55
static const float kUnitW[] = {15.0f, 22.0f, 41.0f};   // 单株/单只宽
static const float kUnitH[] = {32.0f, 45.0f, 36.0f};
static const float kGapCoeff = 0.6f;        // 原版 GAP_COEFFICIENT
static const float kMaxGapCoeff = 1.5f;     // 原版 MAX_GAP_COEFFICIENT

float jump_air_distance(float speed) {
  float v0 = -(kJumpV0 - speed / 10.0f);
  float air_frames = 2.0f * v0 / kGravity;
  return speed * air_frames;
}

void pool_init(ObstaclePool& p) {
  for (int i = 0; i < kMaxObstacles; i++) p.items[i].active = false;
  p.history_len = 0;
  p.gap_countdown = 0;
  p.started = false;
}

// 原版 getGap：minGap = round(w*speed + minGapType*gapCoeff)，再乘随机 1.0~1.5
static float calc_gap(float w, float speed, ObstacleType t, RngFn rng, void* ctx) {
  float min_gap = w * speed + kMinGap[(int)t] * kGapCoeff;
  // 可跳过性下限（本机窄屏保险，见 ADR-001 v2）：不小于滞空位移
  float air = jump_air_distance(speed);
  if (air > min_gap) min_gap = air;
  float coef = 1.0f + (float)(rng(ctx) % 51) / 100.0f;   // 1.00~1.50 ≈ 原版 random(minGap, maxGap)
  return min_gap * coef;
}

static void spawn(ObstaclePool& p, float speed, RngFn rng, void* ctx) {
  int slot = -1;
  for (int i = 0; i < kMaxObstacles; i++) if (!p.items[i].active) { slot = i; break; }
  if (slot < 0) return;

  // 类型随机（原版：3 类均匀随机，minSpeed 门控 + 去重检查不满足则重掷）
  ObstacleType t = ObstacleType::CactusSmall;
  for (int tries = 0; tries < 8; tries++) {
    t = (ObstacleType)(rng(ctx) % 3);
    if (t == ObstacleType::Ptero && speed < kPteroMinSpeed) continue;   // minSpeed 门控
    // duplicateObstacleCheck：前 2 个同类则重掷
    if (p.history_len >= 2 && p.history[0] == t && p.history[1] == t) continue;
    break;
  }

  // 簇大小：原版随机 1~3，multipleSpeed 未达标强制为 1
  uint8_t size = 1 + (uint8_t)(rng(ctx) % 3);
  if (size > 1 && kMultipleSpeed[(int)t] > speed) size = 1;
  if (t == ObstacleType::Ptero) size = 1;

  Obstacle& o = p.items[slot];
  o.type = t;
  o.size = size;
  o.active = true;
  o.x = (float)kScreenW;
  o.w = kUnitW[(int)t] * size;
  o.h = kUnitH[(int)t];
  o.y = (t == ObstacleType::Ptero) ? kPteroHeights[rng(ctx) % 3]
                                   : (float)kGroundLineY - o.h;

  p.gap_countdown = calc_gap(o.w, speed, t, rng, ctx);
  p.history[1] = p.history[0];
  p.history[0] = t;
  if (p.history_len < 2) p.history_len++;
}

void pool_update(ObstaclePool& p, float advance, float speed, RngFn rng, void* ctx) {
  for (int i = 0; i < kMaxObstacles; i++) {
    if (!p.items[i].active) continue;
    p.items[i].x -= advance;
    if (p.items[i].x + p.items[i].w < 0) p.items[i].active = false;
  }

  if (!p.started) {
    // 原版 CLEAR_TIME=3000ms：开场 3 秒无障碍（折算为距离 px = 速度 × 180 帧）
    p.gap_countdown = speed * 180.0f;
    p.started = true;
    return;
  }

  p.gap_countdown -= advance;
  if (p.gap_countdown <= 0) spawn(p, speed, rng, ctx);
}
