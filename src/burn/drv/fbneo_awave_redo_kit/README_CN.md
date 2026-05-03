# FBNeo Atomiswave/Flycast 重做包

这个包不是编译完成的替换版 atomiswave 目录，而是重做用的设计包/骨架包。

我检查了你上传的 `atomiswave.zip`：它现在主要问题不是“多搬了 standalone Flycast shell”，因为包里主要已经是 `flycast/core`。真正问题是 bridge 太重，`awave_core.cpp` 同时承担 libretro host、GL context、视频 readback、音频 FIFO、输入翻译、ROM/content 构造、环境变量 option，导致它很难达到原 Flycast 的流畅度。

建议你按 `docs/01_redo_direction_cn.md` 的阶段重做：

1. 先拆 metadata：`awave_driverdb.h/cpp`
2. 再拆 bridge：host/video/audio/input/content/state
3. 视频改 direct texture first，不再默认 readback
4. 音频改固定目标 latency + bounded FIFO
5. 输入按 profile 拆，不再全局 map
6. 最后用 manifest 重新生成 wrapper 文件，不手工维护 178 个 `awave_wrap_*`

`src_scaffold/` 里是建议的新文件骨架。

`tools/` 里是辅助审计脚本。
