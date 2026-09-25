// 渲染器 —— M5GFX LGFX_Sprite 全屏双缓冲（API 依据 docs/DESIGN.md §2.2 已核实）
#pragma once
#include <cstdint>
#include "game_logic.h"
#include "game_session.h"
#include "obstacle.h"

// 云朵（FR-G9：低速漂移，循环回绕）
struct Cloud { float x; int y; bool active; };
constexpr int kMaxClouds = 3;

bool renderer_init();        // 创建 240×135 RGB565 sprite；失败返回 false（调用方降级直绘）
void renderer_game(const DinoState& d, const ObstaclePool& p, const GameSession& s,
                   const Cloud* clouds, int n_clouds,
                   uint32_t hi, int run_frame, int ptero_frame, float ground_offset,
                   bool crashed = false, bool score_blink = false, bool waiting = false);
void renderer_present();     // pushSprite 推屏
#include <M5GFX.h>
LGFX_Sprite* renderer_canvas();   // 双缓冲画布（菜单动效用；nullptr = 降级直绘）

// 直绘 1bpp 素材到屏幕（菜单界面用，不经 sprite）
struct Sprite;
void renderer_draw_sprite_screen(int x, int y, const Sprite& s, uint16_t color);
