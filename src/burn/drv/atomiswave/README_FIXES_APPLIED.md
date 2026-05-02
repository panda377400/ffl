# atomiswave 修复版说明

本包基于你上传的 `atomiswave.zip` 直接修改生成。

## 已改动文件

- `awave_core.cpp`
- `awave_core.h`
- 新增 `AWAVE_FIX_REPORT.md`
- 新增 `README_FIXES_APPLIED.md`

## 主要修复

1. 修复 profile 日志里的 `1.#IO` / `-1.#IO` 非法浮点输出。
2. 修复 audio underrun 时直接清空整个 `NaomiAudio` 队列的问题，避免声音断续、细节丢失。
3. 将 HLE BIOS 改成可用环境变量控制：
   - 默认仍是 enabled，保持原有能启动行为。
   - 设置 `FBNEO_AWAVE_HLE_BIOS=0` 或 `disabled` 可强制使用真实 BIOS。
4. 将启动期视频 readback suppression 从 `45/180` 降到 `12/60`，减少启动黑屏/卡顿感。
5. 在 `NaomiGameInputType` 里加入 `NAOMI_GAME_INPUT_DREAMCAST_PAD`，为后续 Dreamcast/Maple 输入拆分留接口。

## 没有删除这些文件的原因

当前 `awave_core.cpp` 仍然依赖 Flycast runtime、临时 zip 构造、PVR/AICA/ARM7/SH4/Maple/Naomi cart wrapper。直接删除 `libzip`、`archive`、`zlib`、`hw_*`、`rend_*` 等文件很可能导致编译失败或无法启动。

日志文件、临时文件、`.bak` 文件可以删；核心 wrapper 暂时不要删。

## BIOS 注意

如果日志仍然出现 `Cannot open epr-21579d.ic27` 或 `Region bios not found`，这是 BIOS 文件/路径问题，不是源码能完全修掉的问题。需要补齐 `naomi.zip` 或对应 BIOS archive。

## 建议测试顺序

1. 先测一个已知能进游戏的 Atomiswave 普通按键游戏。
2. 再测光枪/赛车类游戏。
3. 再测 NAOMI。
4. 最后再扩 Dreamcast；Dreamcast 不能继续走 coin/service/test 的街机输入模型，需要 Maple pad 专门映射。
