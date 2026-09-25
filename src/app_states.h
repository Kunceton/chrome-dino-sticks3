// 应用状态机 —— 界面流见 docs/DESIGN.md §4，布局依据 docs/ui/ 设计稿
#pragma once

enum class AppState : uint8_t { Title, NamePick, GameReady, Playing, GameOver, Leaderboard };

void app_init();
void app_update();   // 每帧调用
