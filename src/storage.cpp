// NVS 实现：namespace "dino"；board 用 blob + 魔数/长度校验，损坏回退默认
#include "storage.h"
#include <Preferences.h>
#include <cstring>

static const char* kNs = "dino";
static const uint32_t kMagic = 0x44494E4F;   // "DINO"

struct BoardBlob { uint32_t magic; Leaderboard board; };

bool storage_load_board(Leaderboard& b) {
  Preferences p;
  if (!p.begin(kNs, true)) { board_init(b); return false; }
  BoardBlob blob;
  size_t n = p.getBytes("board", &blob, sizeof(blob));
  p.end();
  if (n != sizeof(blob) || blob.magic != kMagic || blob.board.count < 0 ||
      blob.board.count > kBoardSize) {
    board_init(b);
    return false;
  }
  b = blob.board;
  return true;
}

bool storage_save_board(const Leaderboard& b) {
  Preferences p;
  if (!p.begin(kNs, false)) return false;
  BoardBlob blob{kMagic, b};
  size_t n = p.putBytes("board", &blob, sizeof(blob));
  p.end();
  return n == sizeof(blob);
}

uint32_t storage_load_hi() {
  Preferences p;
  uint32_t v = 0;
  if (p.begin(kNs, true)) { v = p.getUInt("hi", 0); p.end(); }
  return v;
}

void storage_save_hi(uint32_t score) {
  Preferences p;
  if (p.begin(kNs, false)) { p.putUInt("hi", score); p.end(); }
}

bool storage_load_last_name(char* buf, int size) {
  Preferences p;
  bool ok = false;
  if (p.begin(kNs, true)) {
    size_t n = p.getString("name", buf, size);
    ok = n > 0;
    p.end();
  }
  if (!ok && size > 0) buf[0] = 0;
  return ok;
}

void storage_save_last_name(const char* name) {
  Preferences p;
  if (p.begin(kNs, false)) { p.putString("name", name); p.end(); }
}

void storage_clear_all() {
  Preferences p;
  if (p.begin(kNs, false)) { p.clear(); p.end(); }
}
