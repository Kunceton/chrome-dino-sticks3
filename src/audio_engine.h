// 音频引擎 —— M5Unified Speaker 虚拟通道（依据 docs/adr/ADR-002）
// ch0 = BGM 旋律，ch1 = SFX（抢占式），ch2 = BGM 贝斯（三角波形），ch3 = 标题和声
#pragma once
#include <cstdint>

struct Note { uint16_t freq; uint16_t dur_ms; };  // freq=0 休止符
enum class BgmTrack : uint8_t { None, Title, Game };

void audio_init();
void audio_play_bgm(BgmTrack t);
void audio_update();          // 每帧调用：各声部独立调度（提前 20ms 排下一音避免断音）
void audio_sfx_jump();
void audio_sfx_point();
void audio_sfx_crash();
void audio_sfx_menu();
