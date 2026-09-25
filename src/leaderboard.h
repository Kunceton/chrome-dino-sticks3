// 本地 Top5 积分榜（纯逻辑；NVS 读写见 storage.cpp 硬件层）
#pragma once
#include <cstdint>

constexpr int kBoardSize = 5;
constexpr int kNameMax = 16;

struct BoardEntry { char name[kNameMax]; uint32_t score; };
struct Leaderboard { BoardEntry e[kBoardSize]; int count; };

void board_init(Leaderboard& b);
// 插入成绩返回名次 0~4；未上榜返回 -1
int board_insert(Leaderboard& b, const char* name, uint32_t score);
