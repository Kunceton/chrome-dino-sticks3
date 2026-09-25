# Chrome Dino @ M5Stack StickS3

**[English](README.md) | [中文](README.zh-CN.md)**

<p>
  <img src="docs/ui/1_title.png" width="49%" alt="标题画面">
  <img src="docs/ui/3_gameplay.png" width="49%" alt="游戏画面">
</p>

Chrome 离线小恐龙（T-Rex Runner）在 M5Stack StickS3 (ESP32-S3) 上的完美移植，外加 FC 红白机风格的标题界面、玩家名字、本地积分榜和多声道 8bit 音乐。

## 特性

- **完美移植**：跑动/跳跃/下蹲/速降物理、大小仙人掌簇、三高度翼龙、加速曲线、昼夜切换、里程碑音效——物理参数与碰撞盒取自 Chromium 官方源码 `offline.js`，等比缩放到 240×135 屏幕
- **FC 风格界面**：标题入场降落动效、左右菜单、眨眼待机恐龙、呼吸光标选名
- **玩家系统**：随机英文名（形容词+动物）、本地 Top 5 积分榜（NVS 断电保留）
- **音乐**：FC《南极大冒险》原版 BGM——标题曲 + 溜冰圆舞曲主旋律，旋律/贝斯/和声多声道（从官方 NSF 分声道精确提取，方波+三角波音色）
- **功耗管理**：闲置 30s 降背光 / 5min 浅睡秒醒 / 30min 自动关机；游戏内左上角电量显示
- **隐藏操作**：榜单页 1 秒内连按 5 次 BtnA 清空全部记录

## 操作

| 按键 | 功能 |
|---|---|
| BtnA（正面蓝钮） | 跳跃 / 确认 |
| BtnB（侧钮） | 下蹲 / 菜单切换 / 返回 |

## 构建

需要 [PlatformIO](https://platformio.org/):

```bash
pio run                    # 编译
pio run -t upload          # 烧录（如失败：长按侧边复位键约 2 秒进下载模式）
pio test -e native         # Mac/PC 上跑 28 项纯逻辑单元测试
```

## 文档

- [docs/SPEC.md](docs/SPEC.md) — 需求与验收标准（含原版参数缩放标定表）
- [docs/DESIGN.md](docs/DESIGN.md) — 架构设计
- [docs/adr/](docs/adr/) — 关键决策记录
- [docs/CONSTRAINTS.md](docs/CONSTRAINTS.md) — 质量与硬件约束
- [docs/ui/](docs/ui/) — UI 设计稿（240×135 等比）

## 许可

- 本项目代码：MIT（见 [LICENSE](LICENSE)）
- 游戏素材（`assets/`）：提取自 Chromium 项目，BSD-3-Clause © The Chromium Authors
- BGM 音符数据：旋律为《溜冰圆舞曲》（Émile Waldteufel, 1882，公有领域）；音符与时值通过对 FC 版录音的信号分析提取
- 仅供学习交流，非商业用途
