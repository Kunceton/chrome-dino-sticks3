// NVS 持久化（硬件层，Preferences）—— 只在结算/确认时调用（CONSTRAINTS：游戏循环内零写入）
#pragma once
#include "leaderboard.h"

bool storage_load_board(Leaderboard& b);            // 失败/损坏回退空榜
bool storage_save_board(const Leaderboard& b);
uint32_t storage_load_hi();
void storage_save_hi(uint32_t score);
bool storage_load_last_name(char* buf, int size);   // 无记录返回 false
void storage_save_last_name(const char* name);
void storage_clear_all();   // 清空全部记录（榜单/HI/名字），隐藏操作用
