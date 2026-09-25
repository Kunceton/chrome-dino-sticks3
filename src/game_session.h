// 游戏会话：计分 / 加速 / 昼夜切换（纯逻辑）
#pragma once
#include <cstdint>

// 横向速度按屏宽比缩放（240/600=0.4）：穿越屏幕时长与原版一致（ADR-001 v2 试玩后修订）
constexpr float kSpeedStart = 3.3f;    // 原版 6 × 0.55（ADR-001 v3）
constexpr float kSpeedMax = 7.2f;      // 原版 13 × 0.55
constexpr float kAccel = 0.00035f;     // 到满速约 200s（试玩反馈：加速再放缓）
constexpr float kScoreCoef = 0.0455f;  // 原版 0.025 ÷ 0.55
constexpr uint32_t kNightPeriod = 700;   // 昼夜切换分数间隔

constexpr uint32_t kEvtMilestone = 1u << 0;  // 跨过 100 分里程碑
constexpr uint32_t kEvtNightFlip = 1u << 1;  // 昼夜翻转

struct GameSession {
  float speed;
  float distance_px;
  uint32_t score;
  bool night;
  uint32_t last_milestone;   // 已触发音效的最高 100 分档
  uint32_t last_night_mark;  // 已翻转的最高 700 分档
};

void session_init(GameSession& s);
uint32_t session_update(GameSession& s, float frames);
