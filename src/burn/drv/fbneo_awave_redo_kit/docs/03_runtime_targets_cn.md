# 运行效果目标

## 视频目标

必须追求：

```text
Flycast render -> GL texture/FBO -> FBNeo frontend present
```

避免：

```text
Flycast render -> glReadPixels/PBO -> CPU buffer -> FBNeo blit
```

后者即使能跑，也很难接近原 Flycast。

## 音频目标

默认策略：

```text
source_rate = Flycast av_info sample_rate, usually 44100
sink_rate   = nBurnSoundRate
latency     = 3 frames default
queue       = bounded FIFO
resample    = linear first, later can replace
```

不要让音频队列无限增长。超过目标直接裁掉旧样本，宁可轻微跳音也不要越跑越迟。

## 输入目标

每个游戏绑定 input profile，不要全局映射。

```text
AW_DIGITAL_5BTN
AW_LIGHTGUN
AW_RACING
NAOMI_1BTN/2BTN/3BTN/4BTN/6BTN/8BTN
NAOMI_LIGHTGUN
NAOMI_RACING
DREAMCAST_PAD
```

## 平台目标

```text
AWAVE_PLATFORM_ATOMISWAVE
AWAVE_PLATFORM_NAOMI
AWAVE_PLATFORM_NAOMI2
AWAVE_PLATFORM_DREAMCAST
```

NAOMI2 不要继续混在 NAOMI。
