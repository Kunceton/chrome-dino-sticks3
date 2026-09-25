# PRD: Chrome Dino 完美移植 — M5Stack Stick S3

> 版本：v1.0 | 日期：2026-09-23 | 状态：待用户审批
> 平台：M5Stack Stick S3 (ESP32-S3) | 开发：PlatformIO + Arduino 框架

---

## 1. Objective（目标）

将 Chrome 浏览器离线小游戏 Chrome Dino（T-Rex Runner）**完美移植**到 M5Stack Stick S3，并在原版基础上增加 FC 红白机风格的标题界面、玩家名字系统和本地积分榜，配合双 BGM 与原版音效，形成一台「掌上调教完整」的复古小游戏设备。

**用户**：单人开发者本人（设备自用，可给朋友演示）。

**成功画面**：开机 → FC 风格标题画面（空灵 8bit BGM）→ 确认开始 → 随机生成/回选玩家名 → 进入游戏（南极大冒险 8bit BGM）→ BtnA 跳跃、BtnB 下蹲躲避障碍 → 撞毁结算 → 分数进 Top 5 榜单（NVS 断电保留）。

---

## 2. 已确认的需求决策（来自 brainstorming）

| # | 决策点 | 结论 |
|---|---|---|
| 1 | 屏幕 | 240×135 横屏（ST7789P3，M5GFX 驱动） |
| 2 | 按键 | 正面蓝钮 KEY1/BtnA (GPIO11) = 跳跃；侧钮 KEY2/BtnB (GPIO12) = 下蹲/菜单光标；电源键不占用 |
| 3 | 渲染方案 | 方案 A：M5GFX 全屏 sprite 双缓冲，无闪烁，目标 ≥30 fps |
| 4 | 积分榜 | 本地 NVS Flash 存储 Top 5（名字+分数），断电保留；不做联网 |
| 5 | 玩家名 | 英文「形容词+动物/名词」随机组合（如 SwiftFox），可换名、可回选历史名 |
| 6 | 标题 BGM | ~~原创空灵曲~~ → **FC 原版标题音乐**（NSF track 0 分声道提取，G 大调 6 秒 + 和声） |
| 7 | 游戏 BGM | FC《南极冒险》溜冰圆舞曲主旋律，手动转录为 8bit 方波音符数组，循环播放 |
| 8 | 音效 | 原版三音效：跳跃「哔」、每 100 分「叮」、撞击「砰」，独立通道不打断 BGM |
| 9 | 主界面风格 | FC 红白机风格：像素大标题 Logo、▶ 箭头光标菜单、闪烁提示、点阵字体、标题画面起即有 BGM |

---

## 3. 功能需求

### 3.0 原版参数与缩放标定（来源：Chromium 源码 tag 120.0.6099.62 `offline.js`）

原版画布 600×150。移植统一缩放比 **s = 135/150 = 0.9**（按画布高度等比）。原版跳跃顶点头顶距上边缘 20%(y=30/150)，等比缩放后 y=27，**天然不出界**，无需额外压缩。

| 项目 | 原版 | ×0.9 |
|---|---|---|
| 恐龙站立 | 44×47 | 40×42 |
| 恐龙下蹲 | 59×25 | 53×22 |
| 地面线 y（恐龙脚底） | 140（BOTTOM_PAD=10） | 126（底留 9） |
| 最大跳跃升高 | 钳制触发于头顶 y<30，DROP_VELOCITY 续升后实际顶点 y≈9（实测离散轨迹升高 ~84px） | 钳制触发头顶 y<27，实际顶点 y≈2~8（升高 76~82px），恒在屏内 |
| 最小跳跃升高 | 30px | 27px |
| 跳跃初速 / 重力 / 下落截断 | -10 / 0.6 / -5（每帧） | -9 / 0.54 / -4.5（帧时长不变，手感一致） |
| 初速随速度修正 | -speed/10 | 同公式 |
| 速度曲线 | 6 → 13 px/f，加速度 0.001/f | **3.3 → 7.2 px/f，加速度 0.00055/f**（按屏宽折中比 0.55 缩放；ADR-001 v3 二次试玩修订） |
| 计分系数 | 距离 ×0.025 | 距离 ×0.0455（÷0.55 补偿） |
| 翼龙出现阈值 | 速度 ≥8.5 | 速度 ≥4.7 |
| 跳跃重力/初速 | 0.6 / -10 | **0.34 / -7.1**（滞空 42 帧配平窄屏越障余量，顶点 ~74px） |
| 昼夜切换间隔 | 700 分 | 700 分 |

> 横向尺度说明：屏幕宽 240 仅为原版 600 的 40%，横向速度同样 ×0.9 会缩短反应时间；障碍生成间距按新屏宽标定（保证每个障碍可跳过），作为难度补偿手段，不降横向速度。此决策记入 ADR-001。

### 3.1 原版移植清单（FR-GAME）

| 编号 | 需求 | 验收标准 |
|---|---|---|
| FR-G1 | 恐龙跑动动画 | 待机站立；开跑后 2 帧腿部交替动画，帧率随速度加快 |
| FR-G2 | 跳跃物理 | BtnA 触发重力抛物线跳跃；按住跳得更高、轻点跳得低（可变跳高，松键按原版 endJump 截断，须先过最小升高）；空中不可二段跳；参数按 §3.0：升高约 76~82px、最小 27px，**顶点 y≥0 恒在屏内**（native 单测覆盖） |
| FR-G3 | 下蹲 | 按住 BtnB 恐龙变矮（姿态切换）；空中按住 BtnB 快速下落（原版行为） |
| FR-G4 | 仙人掌障碍 | 小/大仙人掌，单株/双株/三株簇，随机生成，间距随速度保证可跳过 |
| FR-G5 | 翼龙障碍 | 达到门槛分数后出现；3 种飞行高度（贴地须跳/中空可跑过/低空须蹲）；2 帧振翅动画 |
| FR-G6 | 难度曲线 | 速度随分数递增，复刻原版加速曲线（起始约 6 px/f，上限约 13 px/f 等比缩放） |
| FR-G7 | 计分 | 按行进距离计分；每 100 分分数闪烁 + 「叮」音效 |
| FR-G8 | 昼夜切换 | 700 分起周期性反转为黑夜配色，再切回（彩色屏实现为白昼/黑夜两套配色） |
| FR-G9 | 场景元素 | 云朵随机漂浮、地面滚动纹理（小石子凸起）、地平线 |
| FR-G10 | 开场待启动 | 恐龙站立在地，显示提示，BtnA 开始 |
| FR-G11 | 撞击结算 | 碰撞后恐龙死亡定格 → 撞击音效 → BGM 停止 → 结算画面 |
| FR-G12 | HI 最高分 | 游戏内右上角显示 HI 分与当前分（原版样式）；HI 分持久化 |
| FR-G13 | 碰撞判定 | AABB 矩形碰撞，判定盒比视觉略小（对齐原版手感） |

### 3.2 FC 风格界面流（FR-UI）

状态机：`BOOT → TITLE → NAME_PICK → GAME_READY → PLAYING → GAME_OVER → (LEADERBOARD) → TITLE`

| 编号 | 需求 | 验收标准 |
|---|---|---|
| FR-U1 | 标题画面 | 「CHROME DINO」像素大 Logo 居中 + 恐龙剪影；▶ 光标菜单（GAME START / SCORE RANK）；底部闪烁提示；空灵 BGM 播放 |
| FR-U2 | 菜单交互 | BtnB 移动 ▶ 光标，BtnA 确认；光标移动有 8bit 选项音效 |
| FR-U3 | 积分榜画面 | 从标题菜单进入；显示 Top 5（名次/名字/分数）；BtnB 返回标题 |
| FR-U4 | 选名画面 | 中央显示「YOUR NAME IS」换行+名字；名字下方一条横线，线上有一个呼吸闪烁的输入块（光标在名字末尾，无名字时在线最左）。**默认显示上一轮玩家的名字；首次游玩为空**。每按一次 BtnB 重新随机生成一个名字。底部左右分栏：左「BtnB: NEW PLAYER」，右「BtnA: START!」。按 BtnA 时若名字为空则先生成一个再进入 |
| FR-U5 | 结算画面 | 撞毁后显示 GAME OVER、本局分数、是否新纪录/上榜；BtnA 重开（同名），BtnB 返回标题 |
| FR-U6 | 上榜高亮 | 若本局进入 Top 5，积分榜中该行闪烁/反色高亮 |
| FR-U7 | 全套像素字体 | 所有 UI 文本使用 8bit 点阵风格字体 |
| FR-U8 | BGM 切换 | TITLE → 标题曲（进入时播一遍）；NAME_PICK/LEADERBOARD → 无 BGM；PLAYING → 南极大冒险循环；GAME_OVER 停 BGM |

### 3.3 音频（FR-AUD）

| 编号 | 需求 | 验收标准 |
|---|---|---|
| FR-A1 | 双 BGM 轨 | 统一音符序列格式（音高+时值数组），切曲仅切数据指针；循环播放 |
| FR-A2 | 音效通道 | 跳/叮/撞/菜单移动四音效独立虚拟通道，与 BGM 混音不互断 |
| FR-A3 | 音量安全 | 默认音量 ≤70%（官方建议电池供电下 <75% 防重启） |
| FR-A4 | 标题 BGM | FC 原版标题音乐（NSF 提取），旋律+和声双声部，循环前留白 2 秒 |
| FR-A5 | 游戏 BGM | FC 原版溜冰圆舞曲（NSF 分声道提取），旋律+三角波贝斯双声部 |

### 3.4 持久化（FR-STO）

| 编号 | 需求 | 验收标准 |
|---|---|---|
| FR-S1 | Top 5 榜单 | NVS 存储 5 条（名字+分数），按分降序；新分数达标即插入；断电重启后仍在 |
| FR-S2 | HI 分 | NVS 存储全局最高分 |
| FR-S3 | 玩家名 | NVS 存储最近使用的名字（选名画面默认显示）；无历史列表需求 |
| FR-S4 | 容错 | NVS 读取失败/数据损坏时回退默认值（空榜单、重新生成名字），不崩溃 |

---

## 4. Tech Stack

- **平台**: PlatformIO (espressif32@6.12.0)，board 按官方 StickS3 配置（`esp32-s3-devkitc-1` + 官方 build_flags，参考 [StickS3 官方文档](https://docs.m5stack.com/en/core/Sticks3)）
- **框架**: Arduino (ESP32-S3)
- **库**: M5Unified（显示/按键/扬声器/电源）、M5GFX（图形渲染）
- **存储**: ESP32 Preferences (NVS)
- **测试**: PlatformIO `native` 环境 + 桌面单测（纯逻辑模块）
- **语言**: C++17

## 5. Commands

```bash
pio run                          # 编译（m5stack-sticks3 环境）
pio run -t upload                # 烧录到 StickS3
pio device monitor               # 串口监视 115200
pio test -e native               # Mac 上跑纯逻辑单元测试
```

## 6. Project Structure

```
platformio.ini        → m5stack-sticks3 环境 + native 测试环境
src/
  main.cpp            → setup/loop，状态机调度
  app_states.{h,cpp}  → 界面状态机 TITLE/NAME_PICK/PLAYING/...
  game_logic.{h,cpp}  → 恐龙物理/障碍生成/碰撞/计分/加速（零硬件依赖，可单测）
  renderer.{h,cpp}    → M5GFX sprite 双缓冲渲染
  sprites.h           → 位图素材数组（从 Chromium 开源 sprite 提取缩放）
  audio_engine.{h,cpp}→ 双 BGM 轨 + SFX 混音
  bgm_data.h          → 音符序列数据（空灵曲/溜冰圆舞曲）
  storage.{h,cpp}     → NVS 榜单/HI分/名字读写（逻辑可单测）
  name_gen.{h,cpp}    → 随机名字生成（可单测）
test/                 → native 单元测试
docs/
  SPEC.md             → 本文件
  CONSTRAINTS.md      → 硬约束
  adr/                → 架构决策记录
tasks/                → 实施计划与任务拆解
```

## 7. Code Style

- 模块化 .h/.cpp 对；`game_logic`、`name_gen`、`storage` 核心逻辑**禁止 include 任何 M5/Arduino 头文件**（保证 native 可测），通过注入接口（如随机数、存储回调）与硬件解耦
- 命名：函数/变量 `snake_case`，类型 `PascalCase`，常量 `kCamelCase` 或 `UPPER_SNAKE`（与兄弟项目 Digital-Stylophone 一致）
- 无动态内存分配（游戏运行期），障碍池用固定数组

```cpp
// game_logic.h —— 纯逻辑示例
struct DinoState {
  float y;          // 离地高度
  float vy;         // 垂直速度
  bool ducking;
  bool dead;
};

void dino_update(DinoState& d, bool jump_pressed, bool duck_held, float dt);
bool check_collision(const Rect& a, const Rect& b);
```

## 8. Testing Strategy

- **native 单测**（`pio test -e native`，Mac 上运行）：
  - 碰撞判定（边界相交/相离用例）
  - 跳跃物理（可变跳高、落地、空中禁二段跳）
  - 加速曲线与计分（里程碑触发）
  - 障碍生成（最小间距可跳过性）
  - 榜单排序/插入/Top5 截断、名字生成唯一性与格式
- **烧录实测**（人工 checklist）：
  - 渲染无闪烁、帧率稳定（目测+帧计数日志）
  - 按键手感（跳/蹲响应 <50ms）
  - 双 BGM 切换、音效不打断 BGM、音量无破音/重启
  - NVS 断电保留验证
- 覆盖率目标：game_logic/name_gen/storage 核心分支 ≥80%

## 9. Boundaries

- **Always**: 提交前跑 `pio run` + `pio test -e native`；硬件相关 API 用法以官方文档为准（M5Unified/M5GFX/StickS3 PinMap）
- **Ask first**: 新增库依赖；修改分区表；引入联网功能；音量默认值超 70%
- **Never**: 在 game_logic 等纯逻辑模块中 include 硬件头文件；运行期动态分配内存；提交任何凭记忆未核实的硬件引脚定义

## 10. Success Criteria

1. `pio run` 编译零错误零警告，`pio test -e native` 全绿
2. 烧录后完整走通：标题（空灵 BGM）→ 选名 → 游戏（南极 BGM）→ 结算 → 榜单，无死机重启
3. 第 3 节全部功能需求验收标准逐条通过
4. 连续游玩 10 分钟无内存泄漏迹象（heap 监控日志稳定）
5. 断电重启后榜单/HI 分/名字完好

## 11. Open Questions

- ~~BGM 数据来源~~（已决：NSF 分声道精确提取，旋律/贝斯/和声均为原版数据）
- 翼龙出现门槛分数：初定 450 分（对齐原版），实测试玩后可调
