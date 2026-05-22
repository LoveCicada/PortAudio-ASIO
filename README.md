# LTC ASIO 播放器

基于 **PortAudio 19.7.0** + **Steinberg ASIO SDK** + **MFC** 的 Windows 桌面程序，通过 ASIO 驱动将 LTC（或任意 PCM）WAV 文件播放到声卡，优先支持 **Dante Virtual Soundcard**。

## 功能概览

- 枚举并选择 ASIO 输出设备（自动优先选中 Dante Virtual Soundcard）
- 可配置 ASIO 输出通道、缓冲区帧数、循环播放
- 支持单声道 / 立体声 PCM WAV（16 / 24 / 32 位整型、32 位浮点）
- 打开 ASIO 驱动控制面板，便于调整采样率等参数

## 依赖路径（默认）

| 组件 | 路径 |
|------|------|
| PortAudio 19.7.0 | `E:/opensource/portaudio/portaudio-19.7.0` |
| ASIO SDK 2.3.4 | `E:/opensource/portaudio/ASIO-SDK_2.3.4_2025-10-15/ASIOSDK` |

可在 CMake 配置时覆盖：

```powershell
cmake -S . -B build -DPORTAUDIO_ROOT="..." -DASIOSDK_ROOT_DIR="..."
```

## 快速构建

**环境要求**：Windows 10+、Visual Studio（含「使用 C++ 的桌面开发」与 **MFC/ATL 共享 DLL**）、CMake 3.16+。

```powershell
cd E:\opensource\portaudio\PortAudio-ASIO
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"
cmake --build build --config Release
```

生成文件：`build\Release\LtcAsioPlayer.exe`  
若本机为 VS 2022，将生成器改为 `"Visual Studio 17 2022"`。

## 快速使用

1. 启动 `LtcAsioPlayer.exe`
2. 确认 ASIO 设备列表（Dante 存在时会默认选中）
3. 设置输出通道（单声道 LTC 默认通道 0）、缓冲区大小
4. 浏览选择 WAV 文件，可选「循环播放」
5. 点击「播放」；需要时点击「ASIO 控制面板」调整驱动

推荐 LTC 素材：**48000 Hz、单声道** PCM WAV。

## 项目结构

```
PortAudio-ASIO/
  CMakeLists.txt          # 根构建脚本
  README.md               # 本文件（中文概览）
  docs/                   # 详细中文文档
  src/
    app/                  # MFC 程序入口
    ui/                   # 主对话框
    audio/                # WAV 读取、设备枚举、播放引擎
  res/                    # 对话框资源
```

## 中文文档索引

| 文档 | 说明 |
|------|------|
| [docs/文档索引.md](docs/文档索引.md) | 文档目录与阅读顺序 |
| [docs/项目概述.md](docs/项目概述.md) | 目标、技术栈、范围 |
| [docs/构建指南.md](docs/构建指南.md) | 环境、CMake 选项、常见构建问题 |
| [docs/使用说明.md](docs/使用说明.md) | 界面说明、LTC 路由、采样率 |
| [docs/架构与模块.md](docs/架构与模块.md) | 模块划分、数据流、线程模型 |
| [docs/故障排除.md](docs/故障排除.md) | 播放与构建故障排查 |

## 许可证说明

- PortAudio、Steinberg ASIO SDK 分别受其各自许可证约束。
- 本仓库应用源码为 PortAudio + ASIO 集成的参考实现，使用前请自行确认第三方许可要求。
