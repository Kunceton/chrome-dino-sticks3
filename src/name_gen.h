// 随机玩家名：形容词 + 动物/名词（纯逻辑，随机源注入）
#pragma once
#include <cstdint>

using RngFn = uint32_t (*)(void*);

// buf 至少 17 字节（15 字符 + 终止符）
void name_generate(char* buf, int buf_size, RngFn rng, void* ctx);
