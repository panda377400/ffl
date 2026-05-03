# FBNeo Atomiswave/Flycast 重做方向

## 结论

当前移植已经能跑，说明 Flycast libretro wrapper 和 FBNeo 驱动之间的基本调用链是通的。但它很难接近原 Flycast 的流畅度，因为当前设计仍然把 Flycast 当成一个被塞进 FBNeo 的 libretro core，而不是把 Flycast core 当成被 FBNeo 调度的硬件模块。

因此建议重做，不建议继续堆补丁。

## 当前包的现状

上传的 atomiswave.zip 展开后约 771 个文件、约 9.5 MiB 源码：

- `flycast/core/**`: 583 个文件，是主要体积来源。
- `awave_wrap_*`: 178 个包装编译单元，主要用于把 Flycast 源文件塞进 FBNeo 工程。
- `awave_core.cpp/.h`: 当前 bridge。
- `d_awave.cpp`: Atomiswave driver 表和输入表。
- `d_naomi.cpp`: NAOMI driver 表和输入表。

这不是“文件多所以慢”，而是边界错了：视频、音频、输入、平台配置、内容加载都在 bridge 里揉成一团。

## 不建议继续保留的设计

当前方式：

```text
FBNeo driver -> awave_core.cpp -> renamed Flycast libretro.cpp -> Flycast core
```

问题：

1. libretro wrapper 的生命周期、option 查询、hw render、audio callback 原本是给 RetroArch/前端用的，不是给 FBNeo driver 直接内嵌用的。
2. 当前 bridge 里同时处理 ZIP 生成、direct archive、GL fallback context、PBO readback、音频 FIFO、输入翻译、环境变量 option，这导致任何一个环节变动都会影响整体稳定性。
3. 视频走 CPU readback/PBO 时，本质还是会破坏 Flycast 原始渲染节奏。
4. 音频从 Flycast callback 队列再被 FBNeo drain，容易出现 underrun、排队过多、音画错位。
5. Dreamcast、NAOMI、NAOMI2、Atomiswave 的加载方式差异没有在 driver metadata 层彻底固定。

## 新结构建议

```text
src/burn/drv/atomiswave/
  d_awave.cpp                 # 只保留 Atomiswave driver 表、ROM 表、输入定义
  d_naomi.cpp                 # 只保留 NAOMI/NAOMI2 driver 表、ROM 表、输入定义
  d_dreamcast.cpp             # 可选；Dreamcast 走 external content，不要混进街机 zip
  awave_driverdb.h/cpp        # 游戏元数据表：平台、BIOS、输入类型、内容类型
  awave_fbneo_bridge.h/cpp    # FBNeo 生命周期和 Burn 接口
  awave_flycast_host.h/cpp    # Flycast host 接口：环境、路径、option、log
  awave_video.h/cpp           # 视频 present：direct texture 优先，readback fallback
  awave_audio.h/cpp           # 音频 clock/FIFO/resampler
  awave_input.h/cpp           # 输入：digital/analog/lightgun/dc pad
  awave_content.h/cpp         # ROM zip/direct archive/external disc
  awave_state.h/cpp           # scan/save-state
  flycast_embed/              # 只放 Flycast core 或 wrapper 必需文件
```

## 核心目标

### 1. 不再把 libretro.cpp 当成主适配层

短期可以保留 renamed libretro.cpp 作为过渡，但长期应该把 `retro_environment`、`retro_audio_batch`、`retro_video_refresh` 这些处理从 `awave_core.cpp` 拆出，成为清晰的 host 层。

### 2. 视频改成 direct texture first

目标顺序：

```text
shared frontend GL context + direct texture
  -> frontend 可直接取纹理/FBO
  -> 不读回 CPU

fallback GL context + PBO/readback
  -> 只用于不支持 direct texture 的前端
```

原 Flycast 流畅度的关键之一是避免每帧 glReadPixels 式同步。FBNeo 侧如果不能直接显示 Flycast FBO/texture，就很难追平原 Flycast。

### 3. 音频以 Flycast timing 为源时钟

不要把音频队列当无限 FIFO。建议：

- 使用 Flycast `retro_system_av_info` 的 sample_rate 和 fps。
- 每帧预期样本数按 fps 算。
- FBNeo `nBurnSoundLen` 作为输出窗口。
- 队列目标维持 2-3 帧，超过直接 drop 到目标，低于目标做补偿。
- 默认不要 1 帧 latency。

### 4. 平台和内容加载必须显式

不要靠扩展名/文件名猜平台。

每个游戏元数据必须包含：

```cpp
platform: atomiswave / naomi / naomi2 / dreamcast
bios: awbios / naomi / naomi2 / dc_boot
content_type: arcade_zip / direct_archive / external_disc
input_profile: digital / lightgun / racing / analog / dreamcast_pad
```

Dreamcast 只接受 external disc：`.gdi`, `.chd`, `.cue`, `.cdi`, `.m3u`。

### 5. 输入拆成 profile

不要让 Atomiswave、NAOMI、Dreamcast 共用一个映射。

- Atomiswave：JAMMA/街机按钮。
- NAOMI：街机按钮 + 特殊控制器。
- NAOMI2：同 NAOMI，但 platform 必须独立。
- Dreamcast：DC pad、analog stick、analog trigger、VMU。

## 重做阶段

### Phase 0：冻结当前包

先不要继续添加游戏。先把现有 `d_awave.cpp`, `d_naomi.cpp` 和 `awave_core.cpp` 冻结。

### Phase 1：拆 metadata

把 `NaomiGameConfig` 从两个 driver cpp 中移到 `awave_driverdb.h/cpp`。driver cpp 只负责 BurnDriver 注册。

### Phase 2：拆 bridge

把 `awave_core.cpp` 拆成：

- content
- host/options
- video
- audio
- input
- lifecycle

### Phase 3：改视频为 direct texture first

先要求 libretro/OpenGL 前端注册 shared GL context。没有 context 的情况下只提供兼容 fallback。

### Phase 4：重写音频

从“callback 累积 + 每帧 drain”改成“固定目标 latency + 源时钟 resample”。默认 3 帧。

### Phase 5：重新生成 wrapper 编译单元

不要手工维护 178 个 `awave_wrap_*` 文件。用脚本从 manifest 生成。这样以后同步 Flycast 更容易。

