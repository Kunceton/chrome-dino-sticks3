#include "name_gen.h"
#include <cstring>

static const char* kAdj[] = {
  "Swift", "Brave", "Mighty", "Nimble", "Rapid",
  "Clever", "Bold", "Lucky", "Sneaky", "Turbo",
  "Cosmic", "Astro", "Pixel", "Jolly", "Fuzzy",
};
static const char* kNoun[] = {
  "Fox", "Panda", "Owl", "Wolf", "Tiger",
  "Bear", "Hawk", "Otter", "Lynx", "Falcon",
  "Badger", "Cobra", "Gecko", "Bison", "Raven",
};

void name_generate(char* buf, int buf_size, RngFn rng, void* ctx) {
  if (buf_size < 2) { if (buf_size == 1) buf[0] = 0; return; }
  uint32_t r = rng(ctx);
  const char* a = kAdj[r % 15];
  const char* n = kNoun[(r >> 8) % 15];
  // 手动拼接，超长截断（防御）
  int i = 0;
  for (const char* p = a; *p && i < buf_size - 1; p++) buf[i++] = *p;
  for (const char* p = n; *p && i < buf_size - 1; p++) buf[i++] = *p;
  buf[i] = 0;
}
