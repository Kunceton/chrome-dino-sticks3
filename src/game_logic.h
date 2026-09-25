// Chrome Dino 纯游戏逻辑 —— 零硬件依赖（CONSTRAINTS Floor：禁止 include M5/Arduino 头）
// 参数标定见 docs/SPEC.md §3.0（Chromium offline.js ×0.9）
#pragma once
#include <cstdint>

struct Rect { float x, y, w, h; };

constexpr float kGravity = 0.34f;      // 滞空 T≈42 帧（窄屏配平，ADR-001 v3），顶点 ~74px
constexpr float kJumpV0 = -7.1f;
constexpr float kDropVelocity = -3.3f;
constexpr float kSpeedDropCoeff = 3.0f;
constexpr int   kGroundLineY = 126;
constexpr int   kDinoW = 40, kDinoH = 42;
constexpr int   kDinoDuckW = 53, kDinoDuckH = 22;
constexpr float kDinoStandTopY = kGroundLineY - kDinoH;   // 84
constexpr float kMaxJumpTopY = 27.0f;   // 原版 MAX_JUMP_HEIGHT 30 × 0.9
constexpr float kMinJumpRise = 27.0f;   // 原版 MIN_JUMP_HEIGHT 30 × 0.9

struct DinoState {
  float y;            // 头顶 y（站立 84）
  float vy;           // px/帧，向上为负
  bool jumping;
  bool ducking;
  bool reached_min_height;
  bool speed_drop;
};

DinoState dino_create();
void dino_update(DinoState& d, bool jump_pressed, bool duck_held, float speed, float frames);
void dino_cut_jump(DinoState& d);  // 松开跳跃键提前截断（可变跳高，原版 endJump 行为）

// ---- 碰撞（官方碰撞盒 ×0.9，来源 assets/reference/sprite-defs.js） ----
enum class ObstacleType : uint8_t { CactusSmall, CactusLarge, Ptero };

constexpr float kDinoX = 24.0f;

// 恐龙碰撞盒组（跑 6 盒 / 蹲 1 盒，蹲时锚定脚底），返回盒数
int dino_collision_boxes(const DinoState& d, float dino_x, Rect* out, int max_out);
// 障碍物碰撞盒组；total_w 为簇总宽（多株时中盒拉伸、右盒贴右缘，与原版一致）
int obstacle_collision_boxes(ObstacleType t, float ox, float oy, float total_w, Rect* out, int max_out);
bool boxes_collide(const Rect* a, int na, const Rect* b, int nb);
