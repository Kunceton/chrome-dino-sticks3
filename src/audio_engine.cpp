// 音频引擎实现（API 依据 docs/DESIGN.md §2.1，M5Unified Speaker_Class.hpp 已核实）
// 多声部：旋律 ch0 + 贝斯 ch2（三角波音色）+ 标题和声 ch3，SFX ch1 抢占
#include <M5Unified.h>
#include "audio_engine.h"
#include "bgm_data.h"

static constexpr int kChBgm = 0;
static constexpr int kChSfx = 1;
static constexpr int kChBass = 2;
static constexpr int kChHarmony = 3;

// 方波单周期（16 采样满幅）——8bit 旋律/和声音色
//（M5Unified 内置 tone 波形是正弦波，音量小且不是 8bit 味，见 Speaker_Class.inl _default_tone_wav）
static const uint8_t kSquareWav[16] = {
  255,255,255,255,255,255,255,255, 1,1,1,1,1,1,1,1,
};

// 三角波单周期（128 采样，8bit 无符号）——贝斯声部音色（tone 自定义波形，Speaker_Class.hpp 已核实）
static const uint8_t kTriWav[128] = {
  128,134,140,146,152,158,164,170,176,182,188,194,200,206,212,218,
  224,230,236,242,248,254,255,254,248,242,236,230,224,218,212,
  206,200,194,188,182,176,170,164,158,152,146,140,134,128,122,116,
  110,104, 98, 92, 86, 80, 74, 68, 62, 56, 50, 44, 38, 32, 26,
   20, 14,  8,  2,  1,  2,  8, 14, 20, 26, 32, 38, 44, 50, 56,
   62, 68, 74, 80, 86, 92, 98,104,110,116,122,128,122,116,110,104,
   98, 92, 86, 80, 74, 68, 62, 56, 50, 44, 38, 32, 26, 20, 14,  8,
};

struct Lane {
  const Note* seq;
  int idx;
  uint32_t next_at;      // 下一音符的绝对时刻（网格锁定，防多声部漂移）
  uint8_t channel;
  bool triangle;
  bool loop;             // false = 播完一遍即停（标题音乐）
};

static Lane s_lanes[2] = {
  {nullptr, 0, 0, (uint8_t)kChBgm, false},   // 旋律
  {nullptr, 0, 0, (uint8_t)kChBass, true},   // 低音/和声
};

void audio_init() {
  M5.Speaker.setVolume(150);                    // 主音量下调（试玩反馈）
  M5.Speaker.setChannelVolume(kChSfx, 220);     // 音效
}

void audio_play_bgm(BgmTrack t) {
  M5.Speaker.stop(kChBgm);
  M5.Speaker.stop(kChBass);
  M5.Speaker.stop(kChHarmony);
  uint32_t t0 = millis();
  s_lanes[0] = {nullptr, 0, t0, (uint8_t)kChBgm, false, true};
  s_lanes[1] = {nullptr, 0, t0, (uint8_t)kChBass, true, true};
  switch (t) {
    case BgmTrack::Title:
      s_lanes[0].seq = kBgmTitle;
      s_lanes[1].seq = kBgmTitleHarmony;
      s_lanes[1].channel = kChHarmony;
      s_lanes[1].triangle = false;
      s_lanes[0].loop = false;                        // 标题只播一遍
      s_lanes[1].loop = false;
      M5.Speaker.setChannelVolume(kChBgm, 150);      // 标题旋律
      M5.Speaker.setChannelVolume(kChHarmony, 130);  // 标题和声
      break;
    case BgmTrack::Game:
      s_lanes[0].seq = kBgmGame;
      s_lanes[1].seq = kBgmGameBass;
      s_lanes[1].channel = kChBass;
      s_lanes[1].triangle = true;
      M5.Speaker.setChannelVolume(kChBgm, 150);      // 游戏旋律
      M5.Speaker.setChannelVolume(kChBass, 130);     // 贝斯「蹦擦擦」要听得见
      break;
    default: break;
  }
}

void audio_update() {
  uint32_t now = millis();
  for (int li = 0; li < 2; li++) {
    Lane& l = s_lanes[li];
    if (!l.seq) continue;
    if ((int32_t)(now - l.next_at) < -20) continue;   // 距下一音符还有 >20ms
    if (l.seq[l.idx].dur_ms == 0 && l.seq[l.idx].freq == 0) {
      if (l.loop) { l.idx = 0; }          // 循环
      else { l.seq = nullptr; continue; } // 播完即停
    }
    const Note& cur = l.seq[l.idx];
    if (cur.freq > 0) {
      if (l.triangle) {
        M5.Speaker.tone((float)cur.freq, (uint32_t)cur.dur_ms, l.channel, true,
                        kTriWav, sizeof(kTriWav), false);
      } else {
        M5.Speaker.tone((float)cur.freq, (uint32_t)cur.dur_ms, l.channel, true,
                        kSquareWav, sizeof(kSquareWav), false);
      }
    }
    l.next_at += cur.dur_ms;   // 绝对时间网格推进，不漂移
    l.idx++;
  }
}

void audio_sfx_jump()  { M5.Speaker.tone(660.0f,  60, kChSfx, true, kSquareWav, sizeof(kSquareWav)); }
void audio_sfx_point() { M5.Speaker.tone(1320.0f, 80, kChSfx, true, kSquareWav, sizeof(kSquareWav)); }
void audio_sfx_crash() { M5.Speaker.tone(150.0f, 300, kChSfx, true, kSquareWav, sizeof(kSquareWav)); }
void audio_sfx_menu()  { M5.Speaker.tone(880.0f,  40, kChSfx, true, kSquareWav, sizeof(kSquareWav)); }
