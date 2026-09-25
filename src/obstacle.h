// 障碍生成器 —— 忠实移植原版逻辑（assets/reference/offline.js Obstacle/Horizon）
// 尺寸缩放 ×0.9，速度缩放 ×0.4（ADR-001 v2）
#pragma once
#include "game_logic.h"

constexpr int kMaxObstacles = 6;
constexpr int kScreenW = 240;
constexpr float kPteroMinSpeed = 4.7f;   // 原版 minSpeed 8.5 × 0.55
constexpr float kPteroHeights[3] = {45.0f, 68.0f, 90.0f};  // 原版 yPos 50/75/100 ×0.9

struct Obstacle {
  ObstacleType type;
  float x, y;      // 素材左上坐标
  float w, h;      // 总尺寸（含簇宽）
  uint8_t size;    // 簇内株数 1~3（翼龙恒 1）
  bool active;
};

struct ObstaclePool {
  Obstacle items[kMaxObstacles];
  ObstacleType history[2];   // 去重窗口（原版 MAX_OBSTACLE_DUPLICATION=2）
  int history_len;
  float gap_countdown;
  bool started;
};

using RngFn = uint32_t (*)(void*);   // 注入随机源（固件 esp_random，测试用定值）

void pool_init(ObstaclePool& p);
void pool_update(ObstaclePool& p, float advance, float speed, RngFn rng, void* ctx);

// 当前速度下满跳滞空的水平位移（px）
float jump_air_distance(float speed);
