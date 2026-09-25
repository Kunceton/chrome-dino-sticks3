# 素材说明

- `offline-sprite-2x.png`：Chromium 官方离线小游戏 2x 图集
- `reference/offline.js`、`reference/sprite-defs.js`：官方源码与 sprite 坐标定义（存档备查）
- `sprites/`：由 [tools/extract_sprites.py](../tools/extract_sprites.py) 从图集提取并缩放到 StickS3 目标尺寸（LDPI×0.9）的单色素材

来源：Chromium 项目（tag 120.0.6099.62），BSD-3-Clause 许可证，仅用于个人非商业项目。
重新生成：`python3 tools/extract_sprites.py`
