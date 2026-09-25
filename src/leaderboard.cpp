#include "leaderboard.h"
#include <cstring>

void board_init(Leaderboard& b) {
  b.count = 0;
  for (int i = 0; i < kBoardSize; i++) { b.e[i].name[0] = 0; b.e[i].score = 0; }
}

int board_insert(Leaderboard& b, const char* name, uint32_t score) {
  // 找插入位（降序；同分排后）
  int pos = -1;
  for (int i = 0; i < b.count; i++) {
    if (score > b.e[i].score) { pos = i; break; }
  }
  if (pos < 0) {
    if (b.count >= kBoardSize) return -1;   // 榜满且不够分
    pos = b.count;
  }
  int tail = b.count < kBoardSize ? b.count : kBoardSize - 1;
  for (int i = tail; i > pos; i--) b.e[i] = b.e[i - 1];   // 后移
  strncpy(b.e[pos].name, name, kNameMax - 1);
  b.e[pos].name[kNameMax - 1] = 0;
  b.e[pos].score = score;
  if (b.count < kBoardSize) b.count++;
  return pos;
}
