// 应用状态机实现
#include <M5Unified.h>
#include <esp_random.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include "app_states.h"
#include "audio_engine.h"
#include "game_logic.h"
#include "game_session.h"
#include "leaderboard.h"
#include "name_gen.h"
#include "obstacle.h"
#include "renderer.h"
#include "sprites.h"
#include "storage.h"

// FC 配色（RGB565）
static const uint16_t C_BG     = 0x0843;   // 夜空深蓝黑
static const uint16_t C_BLUE   = 0x439C;
static const uint16_t C_DARKBL = 0x1188;   // 深蓝描边
static const uint16_t C_RED    = 0xE1C8;
static const uint16_t C_YELLOW = 0xFEC9;
static const uint16_t C_WHITE  = 0xFFFF;
static const uint16_t C_GRAY   = 0x94B4;
static const uint16_t C_DIM    = 0x52AA;

static AppState s_state;
static bool s_need_redraw;

// 游戏态
static DinoState s_dino;
static ObstaclePool s_pool;
static GameSession s_session;
static Leaderboard s_board;
static uint32_t s_hi;
static char s_name[kNameMax];
static int s_last_rank;          // 本局上榜名次，-1 未上榜
static float s_ground_off;
static int s_menu_idx;           // 标题菜单光标
static uint32_t s_frame_ms;      // 上一帧时刻
static Cloud s_clouds[kMaxClouds];
static uint32_t s_blink_until;   // 分数里程碑闪烁截止时刻
static uint32_t s_last_render;   // 60fps 限帧用
static uint32_t s_last_activity; // 最近按键操作时刻（闲置分级计时）
static bool s_dimmed;            // 背光已降
static bool s_slept;             // 浅睡中标志
static int s_dino_gx;            // 标题恐龙 x（动画重绘用）
static uint32_t s_title_enter_ms; // 标题入场动效起点
static bool s_title_anim_done;    // 入场动效完成标志
static uint32_t s_lb_enter_ms;    // 榜单入场动效起点
static bool s_lb_anim_done;       // 榜单入场动效完成标志

static uint32_t rng_wrap(void*) { return esp_random(); }

// ---------- 界面绘制 ----------

// 1bpp 素材绘制到指定目标（sprite 画布或屏幕）
static void draw_sprite_to(lgfx::LGFX_Device& d, int x, int y, const Sprite& spr, uint16_t color) {
  int row_bytes = (spr.w + 7) / 8;
  for (int j = 0; j < spr.h; j++)
    for (int i = 0; i < spr.w; i++) {
      uint8_t byte = spr.data[j * row_bytes + i / 8];
      if (byte & (0x80 >> (i % 8))) d.drawPixel(x + i, y + j, color);
    }
}

static void draw_title_hint(lgfx::LGFX_Device& d, bool on);
static void draw_name_breath(float brightness);

static void draw_stars(lgfx::LGFX_Device& d) {
  uint32_t st = 42;
  for (int i = 0; i < 28; i++) {
    st = st * 1664525u + 1013904223u;
    int x = (st >> 8) % 240;
    st = st * 1664525u + 1013904223u;
    int y = (st >> 8) % 135;
    uint8_t v = 80 + (st >> 16) % 120;
    d.drawPixel(x, y, d.color565(v, v, v));
  }
}

// y_off: 标题组纵向偏移（入场降落动效）；menu_stage: 0 隐藏 / 1 渐显中 / 2 完全显示
static void draw_title(lgfx::LGFX_Device& d, int y_off, int menu_stage, bool show_hint, int dino_frame = 0) {
  d.fillScreen(C_BG);
  draw_stars(d);
  d.setTextDatum(textdatum_t::top_center);
  // CHROME 蓝色 + 深蓝描边（size3 ≈ 高 24px）
  d.setTextSize(3);
  d.setTextColor(C_DARKBL, C_BG);
  d.drawString("CHROME", 122, 12 + y_off);
  d.setTextColor(C_BLUE, C_BG);
  d.drawString("CHROME", 120, 10 + y_off);
  // DINO + 小恐龙同行（素材实际 40px 宽）：恐龙居左，DINO 在右
  d.setTextDatum(textdatum_t::top_left);
  d.setTextSize(3);
  int dino_txt_w = d.textWidth("DINO");
  int group_w = 40 + 10 + dino_txt_w;
  int gx = (240 - group_w) / 2;
  s_dino_gx = gx;
  draw_sprite_to(d, gx, 36 + y_off, dino_frame ? spr_trex_run2 : spr_trex_run1, C_YELLOW);
  d.setTextColor(C_RED, C_BG);
  d.drawString("DINO", gx + 52, 38 + y_off);
  d.setTextColor(C_WHITE, C_BG);
  d.drawString("DINO", gx + 50, 36 + y_off);
  // 菜单左右分列（BtnB 切换，选中项红框），入场动效后渐显
  if (menu_stage > 0) {
    d.setTextSize(1.5f);
    const char* items[2] = {"GAME START", "SCORE RANK"};
    const int cx[2] = {58, 178};
    for (int i = 0; i < 2; i++) {
      d.setTextDatum(textdatum_t::top_center);
      uint16_t on_c = menu_stage == 2 ? C_WHITE : C_GRAY;
      d.setTextColor(s_menu_idx == i ? on_c : C_GRAY, C_BG);
      d.drawString(items[i], cx[i], 102);
      if (menu_stage == 2 && s_menu_idx == i) {
        int w = d.textWidth(items[i]);
        d.drawRect(cx[i] - w / 2 - 5, 98, w + 10, 18, C_RED);
      }
    }
  }
  if (menu_stage == 2) draw_title_hint(d, show_hint);
}

// 仅重绘提示文字区（闪烁不波及全屏）
static void draw_title_hint(lgfx::LGFX_Device& d, bool on) {
  d.fillRect(70, 116, 100, 15, C_BG);
  if (on) {
    d.setTextSize(1);
    d.setTextDatum(textdatum_t::top_center);
    d.setTextColor(C_GRAY, C_BG);
    d.drawString("PRESS BtnA", 120, 122);
  }
}

// 选名界面：brightness 为呼吸亮度 0.0~1.0
static void draw_name_pick(float brightness) {
  auto& d = M5.Display;
  d.fillScreen(C_BG);
  draw_stars(d);
  d.setTextDatum(textdatum_t::top_center);
  d.setTextSize(2);
  d.setTextColor(C_WHITE, C_BG);
  d.drawString("YOUR NAME IS", 120, 18);
  // 名字区：横线加长（45~195），光标与名字等高对齐（名字 y=54 高 16）
  int line_y = 76, line_x0 = 45, line_x1 = 195;
  d.drawFastHLine(line_x0, line_y, line_x1 - line_x0, C_GRAY);
  int cursor_x = line_x0 + 4;
  if (s_name[0]) {
    d.setTextColor(C_YELLOW, C_BG);
    d.drawString(s_name, 120, 54);
    cursor_x = 120 + d.textWidth(s_name) / 2 + 4;
  }
  // START 静态底（呼吸由 draw_name_breath 局部更新）
  d.setTextSize(1.5f);
  d.setTextColor(C_WHITE, C_BG);
  d.drawString("BtnA: START!", 120, 92);
  draw_name_breath(brightness);
  // 底部：BtnB 小字沉底
  d.setTextSize(1);
  d.setTextDatum(textdatum_t::top_center);
  d.setTextColor(C_GRAY, C_BG);
  d.drawString("BtnB x1: NEW NAME  x2: BACK", 120, 122);
}

// 呼吸局部重绘：光标（与名字等高，1.2s 周期）+ START（稍大字号，600ms 快呼吸，下限不闪没）
static void draw_name_breath(float brightness) {
  auto& d = M5.Display;
  // 光标：顶对齐名字（y=54），高度=字形高 16px
  d.setTextSize(2);
  int cursor_x = s_name[0] ? (120 + d.textWidth(s_name) / 2 + 4) : 49;
  uint8_t r = 10 + (uint8_t)((248 - 10) * brightness);
  uint8_t g = 10 + (uint8_t)((216 - 10) * brightness);
  uint8_t b = 30 + (uint8_t)((76 - 30) * brightness);
  d.fillRect(cursor_x, 54, 9, 16, d.color565(r, g, b));
  // START：1.5 号字，独立 600ms 快周期，亮度下限 0.4
  float t2 = (float)(millis() % 600) / 600.0f;
  float tri = t2 < 0.5f ? t2 * 2.0f : 2.0f - t2 * 2.0f;
  float sb_brightness = 0.4f + 0.6f * tri;
  uint8_t sr_ = 10 + (uint8_t)((255 - 10) * sb_brightness);
  uint8_t sg = 10 + (uint8_t)((255 - 10) * sb_brightness);
  uint8_t sb = 30 + (uint8_t)((255 - 30) * sb_brightness);
  d.setTextSize(1.5f);
  d.setTextDatum(textdatum_t::top_center);
  d.setTextColor(d.color565(sr_, sg, sb), C_BG);
  d.drawString("BtnA: START!", 120, 92);
}

static void draw_game_over() {
  // 场景定格（恐龙用撞毁帧）+ 面板
  renderer_game(s_dino, s_pool, s_session, s_clouds, kMaxClouds, s_hi, 0, 0, s_ground_off, true);
  renderer_present();
  auto& d = M5.Display;
  d.fillRect(35, 18, 171, 67, s_session.night ? 0x0841 : 0xF7BE);
  d.drawRect(35, 18, 171, 67, s_session.night ? 0xE71C : 0x528A);
  d.drawRect(36, 19, 169, 65, s_session.night ? 0xE71C : 0x528A);
  d.setTextDatum(textdatum_t::top_center);
  d.setTextSize(2);
  d.setTextColor(s_session.night ? 0xE71C : 0x528A);
  d.drawString("GAME OVER", 120, 24);
  d.setTextSize(1);
  char buf[40];
  if (s_last_rank >= 0) {
    snprintf(buf, sizeof(buf), "SCORE %05lu  RANK #%d!", (unsigned long)s_session.score, s_last_rank + 1);
    d.setTextColor(C_RED);
  } else {
    snprintf(buf, sizeof(buf), "SCORE %05lu", (unsigned long)s_session.score);
    d.setTextColor(s_session.night ? 0xE71C : 0x528A);
  }
  d.drawString(buf, 120, 48);
  d.setTextColor(C_GRAY);
  d.drawString("BtnA: RETRY   BtnB: TITLE", 120, 66);
}

// y_off: 标题纵向偏移（入场降落）；stage: 0 只有标题 / 1 表单渐显 / 2 全部（含返回提示）
static void draw_leaderboard(lgfx::LGFX_Device& d, int y_off, int stage) {
  d.fillScreen(C_BG);
  draw_stars(d);
  d.setTextDatum(textdatum_t::top_center);
  d.setTextSize(2);
  d.setTextColor(C_YELLOW, C_BG);
  d.drawString("SCORE RANK", 120, 10 + y_off);
  if (stage > 0) {
    d.setTextSize(1);
    int y = 40;
    for (int i = 0; i < kBoardSize; i++) {
      bool hl = (stage == 2 && i == s_last_rank);
      if (hl) d.drawRect(20, y - 2, 200, 12, C_RED);
      uint16_t c = hl ? C_RED : (stage == 2 ? C_WHITE : C_GRAY);
      d.setTextDatum(textdatum_t::top_left);
      d.setTextColor(c, C_BG);
      char buf[8];
      snprintf(buf, sizeof(buf), "%d.", i + 1);
      d.drawString(buf, 30, y);
      const char* nm = (i < s_board.count) ? s_board.e[i].name : "-";
      d.drawString(nm, 55, y);
      char sc[8];
      snprintf(sc, sizeof(sc), "%05lu", (i < s_board.count) ? (unsigned long)s_board.e[i].score : 0ul);
      d.setTextDatum(textdatum_t::top_right);
      d.drawString(sc, 210, y);
      y += 14;
    }
  }
  if (stage == 2) {
    d.setTextDatum(textdatum_t::top_center);
    d.setTextColor(C_GRAY, C_BG);
    d.drawString("BtnB: BACK", 120, 120);
  }
}

// 电池电量：图标 + 百分比（与右上角得分同字号）；<20% 红色警示
static void draw_battery(lgfx::LGFX_Device& d, int level, bool night) {
  uint16_t fg = night ? 0xE71C : 0x528A;
  uint16_t c = (level >= 0 && level < 20) ? C_RED : fg;
  d.drawRect(8, 8, 13, 7, c);                    // 电池框
  d.fillRect(21, 10, 2, 3, c);                   // 电极头
  if (level > 0) {
    int w = level * 11 / 100;
    if (w > 0) d.fillRect(9, 9, w, 5, c);        // 电量填充
  }
  char buf[8];
  snprintf(buf, sizeof(buf), "%d%%", level);
  d.setTextSize(1);
  d.setTextDatum(textdatum_t::top_left);
  d.setTextColor(c);
  d.drawString(buf, 25, 8);
}

static void draw_ready(bool blink) {
  // 待机恐龙：原地不动 + 周期性眨眼（原版 WAITING 行为）
  renderer_game(s_dino, s_pool, s_session, s_clouds, kMaxClouds, s_hi,
                blink ? 1 : 0, blink ? 1 : 0, 0, false, false, true);
  // 提示画进 sprite 再推屏（防闪烁）；无 sprite 时降级直绘
  LGFX_Sprite* cv = renderer_canvas();
  lgfx::LGFX_Device& d = cv ? *(lgfx::LGFX_Device*)cv : (lgfx::LGFX_Device&)M5.Display;
  d.setTextDatum(textdatum_t::top_center);
  d.setTextSize(1);
  d.setTextColor(C_GRAY);
  d.drawString("PRESS BtnA TO RUN", 120, 30);
  renderer_present();
}

// ---------- 状态切换 ----------

static void enter(AppState s) {
  s_state = s;
  s_need_redraw = true;
  switch (s) {
    case AppState::Title:
      audio_play_bgm(BgmTrack::Title);   // 标题 BGM 只在标题界面播
      s_title_enter_ms = millis();
      s_title_anim_done = false;
      break;
    case AppState::NamePick:
      audio_play_bgm(BgmTrack::None);    // 选名无 BGM
      break;
    case AppState::Leaderboard:
      audio_play_bgm(BgmTrack::None);    // 榜单无 BGM
      s_lb_enter_ms = millis();
      s_lb_anim_done = false;
      break;
    case AppState::Playing:
      audio_play_bgm(BgmTrack::Game);
      break;
    case AppState::GameOver:
      audio_play_bgm(BgmTrack::None);
      break;
    case AppState::GameReady: s_dino = dino_create(); break;
  }
}

static void start_run() {
  s_dino = dino_create();
  pool_init(s_pool);
  session_init(s_session);
  s_ground_off = 0;
  s_blink_until = 0;
  for (int i = 0; i < kMaxClouds; i++) {
    s_clouds[i].active = true;
    s_clouds[i].x = 60.0f + i * 110.0f;
    s_clouds[i].y = 18 + (int)(rng_wrap(nullptr) % 40);
  }
  enter(AppState::Playing);
}

static void finish_game() {
  audio_sfx_crash();
  if (s_session.score > s_hi) { s_hi = s_session.score; storage_save_hi(s_hi); }
  s_last_rank = board_insert(s_board, s_name, s_session.score);
  if (s_last_rank >= 0) storage_save_board(s_board);
  enter(AppState::GameOver);
}

// ---------- 主更新 ----------

void app_init() {
  renderer_init();
  audio_init();
  storage_load_board(s_board);
  s_hi = storage_load_hi();
  if (!storage_load_last_name(s_name, sizeof(s_name))) s_name[0] = 0;
  s_menu_idx = 0;
  s_dino = dino_create();
  s_blink_until = 0;
  s_last_render = 0;
  for (int i = 0; i < kMaxClouds; i++) s_clouds[i].active = false;
  s_frame_ms = millis();
  s_last_activity = millis();
  s_dimmed = false;
  s_slept = false;
  enter(AppState::Title);
}

void app_update() {
  uint32_t now = millis();
  float frames = (float)(now - s_frame_ms) * 0.06f;   // 60fps 帧数
  if (frames > 3.0f) frames = 3.0f;                    // 防大步长穿墙
  s_frame_ms = now;

  // ---- 闲置功耗分级（游戏中不触发） ----
  bool active = M5.BtnA.wasPressed() || M5.BtnB.wasPressed() ||
                M5.BtnA.isPressed() || M5.BtnB.isPressed();
  if (active) {
    s_last_activity = now;
    if (s_dimmed) { M5.Display.setBrightness(128); s_dimmed = false; }
  }
  if (s_state != AppState::Playing) {
    uint32_t idle = now - s_last_activity;
    if (idle > 1800000UL) {                    // 30min → 直接关机（14µA）
      M5.Speaker.end();                        // 睡前关功放
      M5.Power.powerOff();                     // 不返回
    } else if (idle > 300000UL && !s_slept) {  // 5min → 浅睡
      s_slept = true;
      M5.Speaker.end();
      M5.Display.setBrightness(0);
      M5.Power.lightSleep();                   // 阻塞至唤醒
      // ---- 唤醒点 ----
      M5.Display.setBrightness(128);
      M5.Speaker.begin();
      audio_init();                            // 恢复音量配置
      s_last_activity = millis();
      s_slept = false;
      enter(s_state);                          // 重进当前界面（BGM/画面恢复）
      return;
    } else if (idle > 30000UL && !s_dimmed) {  // 30s → 背光 30%
      M5.Display.setBrightness(38);
      s_dimmed = true;
    }
  }

  audio_update();

  switch (s_state) {
    case AppState::Title: {
      LGFX_Sprite* cv = renderer_canvas();
      lgfx::LGFX_Device& td = cv ? *(lgfx::LGFX_Device*)cv : (lgfx::LGFX_Device&)M5.Display;
      if (!s_title_anim_done) {
        uint32_t el = now - s_title_enter_ms;
        if (el < 2000) {
          float t = el / 2000.0f;
          float e = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);   // ease-out cubic
          draw_title(td, (int)(-90.0f * (1.0f - e)), 0, false);
        } else {
          draw_title(td, 0, 2, true);   // 降完立即全部显示（含红框）
          s_title_anim_done = true;
        }
        if (cv) renderer_present();
        break;   // 动效期间不吃按键，防止残影
      }
      // 稳态：每 50ms 双缓冲整帧重绘（恐龙跑动与提示闪烁都在帧内，不直绘屏幕）
      static uint32_t last_frame = 0;
      if (s_need_redraw || now - last_frame >= 50) {
        last_frame = now;
        s_need_redraw = false;
        draw_title(td, 0, 2, (now / 500) % 2 == 0, (now / 250) % 2);
        if (cv) renderer_present();
      }
      if (M5.BtnB.wasPressed()) { s_menu_idx ^= 1; audio_sfx_menu(); s_need_redraw = true; }
      if (M5.BtnA.wasPressed()) {
        audio_sfx_menu();
        if (s_menu_idx == 0) enter(AppState::NamePick);   // GAME START → 选名
        else { s_last_rank = -1; enter(AppState::Leaderboard); }  // SCORE RANK → 直接榜单
      }
      break;
    }

    case AppState::NamePick: {
      // 呼吸光标：1.2s 周期三角波，只局部重绘光标（整屏不闪）
      float t = (float)(now % 1200) / 1200.0f;
      float brightness = t < 0.5f ? t * 2.0f : 2.0f - t * 2.0f;
      static uint32_t last_breath_draw = 0;
      if (s_need_redraw) {
        draw_name_pick(1.0f);
        s_need_redraw = false;
        last_breath_draw = now;
      } else if (now - last_breath_draw >= 50) {
        draw_name_breath(brightness);
        last_breath_draw = now;
      }
      if (M5.BtnB.wasDoubleClicked()) {          // 双击返回标题
        audio_sfx_menu();
        enter(AppState::Title);
      } else if (M5.BtnB.wasSingleClicked()) {   // 单击换新名（等双击窗口结束才判定）
        name_generate(s_name, sizeof(s_name), rng_wrap, nullptr);
        audio_sfx_menu();
        s_need_redraw = true;
      }
      if (M5.BtnA.wasPressed()) {
        if (!s_name[0]) {
          // 首次游玩为空：先生成并展示，再按一次才开始
          name_generate(s_name, sizeof(s_name), rng_wrap, nullptr);
          audio_sfx_menu();
          s_need_redraw = true;
        } else {
          storage_save_last_name(s_name);
          audio_sfx_jump();
          enter(AppState::GameReady);
        }
      }
      break;
    }

    case AppState::GameReady: {
      // 眨眼：随机间隔 3~7s（原版 blinkDelay = random × 7000ms），闭眼 300ms
      static uint32_t last_ready_draw = 0;
      static uint32_t next_blink = 0;
      static uint32_t blink_until = 0;
      if (next_blink == 0) next_blink = now + 2000 + rng_wrap(nullptr) % 4000;
      if (now >= next_blink) {
        blink_until = now + 300;
        next_blink = now + 3000 + rng_wrap(nullptr) % 4000;
      }
      if (s_need_redraw || now - last_ready_draw >= 100) {
        draw_ready(now < blink_until);
        s_need_redraw = false;
        last_ready_draw = now;
      }
      if (M5.BtnA.wasPressed()) { start_run(); }
      if (M5.BtnB.wasPressed()) { enter(AppState::Title); }
      break;
    }

    case AppState::Playing: {
      bool jump = M5.BtnA.wasPressed();
      bool duck = M5.BtnB.isPressed();
      if (jump) audio_sfx_jump();
      if (M5.BtnA.wasReleased()) dino_cut_jump(s_dino);
      dino_update(s_dino, jump, duck, s_session.speed, frames);
      float advance = s_session.speed * frames;
      pool_update(s_pool, advance, s_session.speed, rng_wrap, nullptr);
      s_ground_off += advance;
      // 云：低速漂移（原版 BG_CLOUD_SPEED 0.2），出屏右侧回绕
      for (int i = 0; i < kMaxClouds; i++) {
        if (!s_clouds[i].active) continue;
        s_clouds[i].x -= advance * 0.2f;
        if (s_clouds[i].x < -41.0f) {
          s_clouds[i].x = 240.0f;
          s_clouds[i].y = 18 + (int)(rng_wrap(nullptr) % 40);
        }
      }
      uint32_t evt = session_update(s_session, frames);
      if (evt & kEvtMilestone) {
        audio_sfx_point();
        s_blink_until = now + 300;   // 分数闪烁 300ms（FR-G7）
      }

      // 碰撞
      Rect db[6], ob[6];
      int nd = dino_collision_boxes(s_dino, kDinoX, db, 6);
      for (int i = 0; i < kMaxObstacles; i++) {
        if (!s_pool.items[i].active) continue;
        int no = obstacle_collision_boxes(s_pool.items[i].type, s_pool.items[i].x,
                                          s_pool.items[i].y, s_pool.items[i].w, ob, 6);
        if (boxes_collide(db, nd, ob, no)) { finish_game(); break; }
      }
      if (s_state != AppState::Playing) break;

      // 60fps 限帧渲染（动画帧率按真实时间计算，与渲染解耦）
      if (now - s_last_render < 16) break;
      s_last_render = now;
      int run_frame = (int)(now / 83) % 2;    // 原版跑动 12fps
      int ptero_frame = (int)(now / 166) % 2; // 原版翼龙 6fps
      renderer_game(s_dino, s_pool, s_session, s_clouds, kMaxClouds, s_hi,
                    run_frame, ptero_frame, s_ground_off, false, now < s_blink_until);
      // 电池电量（PMIC 读数 5s 缓存）
      static uint32_t last_batt_read = 0;
      static int batt_level = -1;
      if (batt_level < 0 || now - last_batt_read > 5000) {
        batt_level = M5.Power.getBatteryLevel();
        last_batt_read = now;
      }
      {
        LGFX_Sprite* cv = renderer_canvas();
        if (cv) draw_battery(*(lgfx::LGFX_Device*)cv, batt_level, s_session.night);
      }
      renderer_present();
      break;
    }

    case AppState::GameOver:
      if (s_need_redraw) { draw_game_over(); s_need_redraw = false; }
      if (M5.BtnA.wasPressed()) enter(AppState::GameReady);
      if (M5.BtnB.wasPressed()) {
        if (s_last_rank >= 0) enter(AppState::Leaderboard);
        else enter(AppState::Title);
      }
      break;

    case AppState::Leaderboard: {
      LGFX_Sprite* cv = renderer_canvas();
      lgfx::LGFX_Device& td = cv ? *(lgfx::LGFX_Device*)cv : (lgfx::LGFX_Device&)M5.Display;
      if (!s_lb_anim_done) {
        uint32_t el = now - s_lb_enter_ms;
        if (el < 2000) {
          float t = el / 2000.0f;
          float e = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
          draw_leaderboard(td, (int)(-60.0f * (1.0f - e)), 0);
        } else if (el < 2400) {
          draw_leaderboard(td, 0, 1);   // 降完表单渐显
        } else {
          draw_leaderboard(td, 0, 2);
          s_lb_anim_done = true;
        }
        if (cv) renderer_present();
        break;
      }
      if (s_need_redraw) { draw_leaderboard(M5.Display, 0, 2); s_need_redraw = false; }
      // 隐藏操作：连续按 5 次 BtnA（间隔 <1s）清空全部记录
      static int clear_count = 0;
      static uint32_t last_clear_press = 0;
      if (M5.BtnA.wasPressed()) {
        if (now - last_clear_press < 1000) clear_count++; else clear_count = 1;
        last_clear_press = now;
        if (clear_count >= 5) {
          clear_count = 0;
          storage_clear_all();
          board_init(s_board);
          s_hi = 0;
          s_name[0] = 0;
          s_last_rank = -1;
          audio_sfx_crash();   // 低沉一声作为确认反馈（UI 无提示）
          draw_leaderboard(M5.Display, 0, 2);
        }
      }
      if (M5.BtnB.wasPressed()) { s_last_rank = -1; enter(AppState::Title); }  // BtnB 返回
      break;
    }
  }
}
