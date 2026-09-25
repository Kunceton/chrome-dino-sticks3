# 设计文档 — Chrome Dino @ M5Stack Stick S3

> 日期：2026-09-24 | 需求依据：[SPEC.md](SPEC.md) v1.x | 约束依据：[CONSTRAINTS.md](CONSTRAINTS.md)

## 1. 架构总览

```
main.cpp
  └─ AppStates（状态机：TITLE/NAME_PICK/GAME_READY/PLAYING/GAME_OVER/LEADERBOARD）
       ├─ GameLogic   纯逻辑：物理/障碍/碰撞/计分/昼夜（零硬件依赖，native 单测）
       ├─ Renderer    M5GFX LGFX_Sprite 全屏双缓冲（240×135 RGB565，~63KB）
       ├─ AudioEngine BGM 双轨 + SFX 通道（M5Unified Speaker 虚拟通道混音）
       ├─ Storage     NVS 读写（Top5/HI 分/最近玩家名）——逻辑层注入回调解耦
       └─ NameGen     形容词×动物随机名（esp_random 注入，native 可测）
```

依赖方向单向：AppStates → 各模块；GameLogic/NameGen/Storage 逻辑层不依赖 M5/Arduino 头文件（Floor 约束）。

## 2. 已核实的 API 用法（source-driven）

### 2.1 音频：M5Unified Speaker（ES8311 codec）
来源：`m5stack/M5Unified` `src/utility/Speaker_Class.hpp`（master，2026-09 拉取存档）

- `M5.Speaker.tone(freq, duration_ms, channel, stop_current_sound)`：内置方波，非阻塞，到时自动停止——**8bit 方波 BGM 的原生能力**
- 8 个虚拟通道（0~7）硬件混音：**ch0 = BGM，ch1 = SFX**，互不打断
- `M5.Speaker.setChannelVolume(ch, 0~255)`：分通道音量；BGM 通道压低，SFX 通道正常
- `M5.Speaker.isPlaying(ch)`：BGM 音符调度依据——当前音符播完前约 10ms 排入下一音符（提前排队避免断音）

BGM 数据格式：`{uint16_t freq_hz, uint16_t duration_ms}` 数组，0 = 休止符。切曲 = 切换数组指针+索引归零。

### 2.2 渲染：M5GFX LGFX_Sprite
来源：`m5stack/M5GFX` `src/lgfx/v1/LGFX_Sprite.hpp`

- `sprite.createSprite(240, 135)` 申请帧缓冲；`sprite.setPsram(true)` 可放 PSRAM（内部 RAM 够用则不放，更快）
- 每帧：sprite 上全量绘制（清屏→地面→云→障碍→恐龙→HUD）→ `sprite.pushSprite(&M5.Display, 0, 0)` 一次推屏，无撕裂
- 帧率控制：固定 60fps 目标帧时长 16.6ms，逻辑按 `dt` 推进（与原版 `deltaTime/msPerFrame` 一致）

### 2.3 按键：M5Unified
`M5.update()` 每帧轮询；`M5.BtnA.wasPressed()/isPressed()`、`M5.BtnB` 同理（官方 Button 示例模式）。跳跃用 `wasPressed` 触发 + `isPressed` 持续判定可变跳高；下蹲用 `isPressed` 持续状态。

### 2.4 存储：ESP32 Preferences (NVS)
namespace `dino`：`hi`(u32)、`board`(5×{name[16],score u32} blob)、`last_name`(string)。只在结算/确认时写入（CONSTRAINTS：游戏循环内零写入）。

## 3. 游戏逻辑标定（依据 Chromium 源码，见 SPEC §3.0）

- 统一缩放 s=0.9；物理参数 ×0.9：v0=-9、g=0.54/帧、下落截断 -4.5、最大升高 57px、最小 27px
- 速度 5.4→11.7 px/f，加速度 0.0009/f；计分 = 距离×系数
- 碰撞盒：采用官方 6/3/5 矩形组（存档 `assets/reference/sprite-defs.js`），坐标 ×0.9
- 翼龙：速度 ≥8.5×0.9≈7.7 后出现；3 高度 y = 45/68/90（原版 50/75/100 ×0.9）；振翅 2 帧 @6fps
- 障碍池：固定数组 6 个，复用槽位；间距 minGap×0.9 起步并随速度放大（GAP_COEFFICIENT 0.6），保证可跳过
- 昼夜：每 700 分触发，黑夜=反色配色表（双配色表切换，不重算素材）

## 4. 状态机与界面

```
BOOT → TITLE（空灵BGM，▶菜单：GAME START/SCORE RANK）
TITLE → NAME_PICK（BtnA on GAME START）
TITLE → LEADERBOARD（BtnA on SCORE RANK）→ TITLE（BtnA）
NAME_PICK：显示 last_name（首次为空）+呼吸输入块；BtnB 换随机名；BtnA 确认
  → GAME_READY（恐龙站立，BtnA 起跳开跑）→ PLAYING（南极BGM）
PLAYING → GAME_OVER（碰撞；停BGM，撞音效；BtnA 重开=GAME_READY 同名，BtnB=TITLE）
GAME_OVER → LEADERBOARD 自动展示（若上榜，高亮行）→ 按键返回
```

## 5. 错误处理

| 场景 | 处理 |
|---|---|
| NVS 读取失败/CRC 错 | 回退空榜单+空名字，继续运行 |
| sprite 创建失败 | 降级为直接绘制（无缓冲），串口告警 |
| 名字生成耗尽组合 | 允许重复，不做唯一性强制 |
| 音频初始化失败 | 静默降级为无声模式，游戏正常跑 |

## 6. 测试策略（对照 SPEC §8 / CONSTRAINTS）

- native：`game_logic`（跳跃轨迹/碰撞/加速/计分/障碍间距）、`name_gen`、`storage` 排序插入逻辑
- 烧录 checklist：帧率 ≥30、音量 ≤70%、按键 <50ms、断电数据保留、10 分钟烤机 heap 稳定
