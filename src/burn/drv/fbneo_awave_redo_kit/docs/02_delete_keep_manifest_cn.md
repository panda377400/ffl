# 删除/保留清单

## 必须保留，直到替代层完成

```text
awave_core.cpp / awave_core.h
awave_embed.cpp
awave_embed_stubs.cpp
awave_imgread_stubs.cpp
awave_softtexcache.cpp
awave_wrap_config.h
d_awave.cpp
d_naomi.cpp
flycast/core/**
awave_wrap_core_*.cpp / *.c / *.S
```

这些文件虽然多，但现在仍参与构建。直接删会导致编译失败。

## 第一批可删除候选

如果你的工作树里还存在完整 Flycast 仓库外壳，以下不要进 FBNeo driver：

```text
flycast/.github/
flycast/docs/
flycast/shell/android/
flycast/shell/apple/
flycast/shell/linux/
flycast/shell/windows/
flycast/shell/qt/
flycast/shell/sdl/
flycast/cmake 平台打包文件
standalone UI 资源
ci/release/packaging 文件
```

你上传的包主要已经只保留 `flycast/core`，问题不是 standalone shell 太多，而是 wrapper 和 bridge 设计太重。

## 第二批可删候选

只有在完成 direct archive/external content 统一后，才考虑删：

```text
awave_wrap_core_deps_libzip_*.c
awave_wrap_core_archive_*.cpp
```

注意：当前 Flycast libretro 可能仍会使用 archive/vfs 路径。没替代前不要删。

## 不建议保留的长期形态

```text
awave_core.cpp 一个文件承担所有逻辑
178 个 awave_wrap_* 手写/静态维护
用环境变量散落控制核心行为
Dreamcast 和 arcade driver 混在一个加载路径
```
