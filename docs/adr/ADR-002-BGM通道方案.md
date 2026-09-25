# ADR-002：BGM 用 Speaker.tone 虚拟通道而非采样混音

- 状态：已决定（2026-09-24）
- 背景：8bit BGM 需要循环旋律 + 不中断的音效。可选：①Speaker.tone 多虚拟通道；②playRaw 推送软件合成的 PCM 流；③PWM/LEDC 自写驱动。

## 决策

用 `M5.Speaker.tone(freq, duration, channel)`：ch0 跑 BGM 音符序列（`isPlaying(ch0)==0` 前排入下一音符），ch1 跑音效（`stop_current_sound=true` 抢占）。

## 理由

- tone() 内置方波就是 8bit 音色，零合成代码
- 硬件混音（ES8311 路径由 M5Unified 管理），BGM 与音效天然不互断
- playRaw 软件混音需要在主循环或高优先级任务里持续喂 PCM 缓冲，增加帧率抖动风险和复杂度，收益为零

## 已核实依据

`Speaker_Class.hpp`（M5Unified master）：8 虚拟通道、tone 非阻塞到时自停、setChannelVolume 分通道音量。

## 后果

- 音符间存在微小时序依赖主循环轮询（每帧检查一次足够，16.6ms 粒度对 8bit 音乐可接受）
- 双声部（旋律+低音伴奏）如将来需要，ch2 再加一条 tone 序列即可
