🌐 [English](README.md) | [Português](README.pt-BR.md) | **中文** | [日本語](README.ja.md) | [Русский](README.ru.md) | [Español](README.es.md) | [Deutsch](README.de.md)

<p align="center">
  <img src="src/images/logo.svg" alt="BATorrent" width="160">
</p>

<h1 align="center">BATorrent</h1>

</p>

<p align="center">
  <a href="https://github.com/Mateuscruz19/BAT-Torrent/releases/latest"><img alt="Release" src="https://img.shields.io/github/v/release/Mateuscruz19/BAT-Torrent?style=flat-square&color=dc2626"></a>
  <a href="https://github.com/Mateuscruz19/BAT-Torrent/releases"><img alt="Downloads" src="https://img.shields.io/github/downloads/Mateuscruz19/BAT-Torrent/total?style=flat-square&color=dc2626"></a>
  <a href="LICENSE"><img alt="License" src="https://img.shields.io/github/license/Mateuscruz19/BAT-Torrent?style=flat-square&color=dc2626"></a>
  <img alt="Platforms" src="https://img.shields.io/badge/platform-Windows%20%7C%20macOS%20%7C%20Linux-dc2626?style=flat-square">
  <img alt="C++" src="https://img.shields.io/badge/C%2B%2B-17-dc2626?style=flat-square&logo=c%2B%2B">
  <img alt="Qt" src="https://img.shields.io/badge/Qt-6-dc2626?style=flat-square&logo=qt">
</p>

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## 概述

BATorrent 是一款注重隐私、性能和清晰度的现代跨平台 BitTorrent 客户端。它将成熟的 libtorrent-rasterbar 引擎与精心调校的 Qt 6 界面相结合，提供远程控制 WebUI、RSS 自动下载、兼容 Stremio 的搜索、网络代理流量隔离以及内置媒体服务器集成。

> **无遥测、无分析、无后台通信。** 应用唯一在未经用户操作的情况下发出的网络请求是 GitHub 版本检查，且可在设置中关闭。你可以在 [`src/app/updater.cpp`](src/app/updater.cpp) 中自行审计。

![主窗口 — 深色主题](src/images/image1.png)

![主窗口 — 浅色主题](src/images/image2.png)

![详情面板](src/images/image3.png)

![设置对话框](src/images/image4.png)

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## 下载

各平台最新版本的预构建二进制文件：

| 平台 | 格式 | 备注 |
|---|---|---|
| Windows | [安装程序 (`.exe`)](https://github.com/Mateuscruz19/BAT-Torrent/releases/latest) · [便携版 (`.zip`)](https://github.com/Mateuscruz19/BAT-Torrent/releases/latest) | Windows 10+（x86_64） |
| macOS | [磁盘映像 (`.dmg`)](https://github.com/Mateuscruz19/BAT-Torrent/releases/latest) | macOS 12+（Apple Silicon） |
| Linux | [AppImage](https://github.com/Mateuscruz19/BAT-Torrent/releases/latest) | Glibc 2.35+（x86_64） |

所有构建产物均由 [Build & Release](.github/workflows/build.yml) 工作流在每次打标签发布时自动生成。

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## 功能特性

### 种子管理
- `.torrent` 文件和磁力链接，支持持久化恢复数据
- **Thunder:// 链接解码** — 中文论坛常用迅雷的 `thunder://` 格式分享链接；BATorrent 通过智能粘贴（Ctrl+V）自动解码
- **智能粘贴** — 当剪贴板中有磁力链接、info hash 或 thunder:// 链接时，按 Ctrl+V 立即弹出添加提示
- **种子检查器** — 在开始下载前预览 `.torrent` 文件信息（名称、大小、文件列表、Tracker、哈希值、创建者、私有标志）
- 按文件设置优先级、顺序下载、手动校验和重新宣告
- 自动注入 Tracker，来源为 [ngosang/trackerslist](https://github.com/ngosang/trackerslist)
- 多标签系统（自由标签，每个种子可同时拥有多个标签和单个分类）
- **内容布局** — 原始、创建子文件夹或不创建子文件夹，控制多文件种子的磁盘布局
- **排除文件模式** — 正则规则自动跳过文件（如 `.nfo`、`.txt`、`sample`）
- **临时下载路径** — 先下载到暂存文件夹，完成后自动移至保存路径（防止媒体服务器扫描不完整文件）
- 分类管理、拖放排序和右键菜单操作
- 从 qBittorrent 导入现有状态
- 从任意文件或文件夹创建新的 `.torrent` 文件

### 状态管理
- **已完成** 状态 — 可手动标记，也可在设定的做种时间（1、3、7、14 或 30 天）后自动提升。与做种中/已完成状态不同，跨重启持久化保存。
- **停止按钮** 可冻结已完成的种子而不移除；**恢复** 按钮取消标记并重新加入做种池。
- 停止做种规则：全局分享率限制和最大做种时间，支持按种子单独覆盖。
- **文件错误时自动暂停** — 如果 libtorrent 无法读取已完成种子的文件（例如外置硬盘被拔出），将暂停而非静默重新下载。

### 内容发现
- **RSS 自动下载**，支持正则过滤、按订阅源设置保存路径、间隔调度和重复检测。可处理磁力链接、`.torrent` URL 和 `<enclosure>` 标签。
- **兼容 Stremio 的搜索**，通过内置的 Cinemeta 和 Torrentio 插件搜索电影和剧集。

### 流媒体播放
- 边下边播 — 支持 `.mp4`、`.mkv`、`.avi`、`.mov`、`.wmv`、`.flv`、`.webm`、`.m4v`、`.ts` 格式。
- 自动检测 VLC 和 IINA，未找到时回退到系统默认播放器。

### 网络代理与流量保护
- **接口绑定** 将所有种子流量锁定到指定的网络接口（如 `tun0`、`utun4`）。
- **断网保护（Kill Switch）** 当绑定的接口断开时立即暂停所有活动种子，可选在接口恢复时自动继续。
- **PT 模式** — 一键私有 Tracker 合规：禁用 DHT/PEX/LSD，强制匿名握手，向所有层级宣告。M-Team、TorrentLeech 等比率追踪社区必备。
- **反吸血屏蔽** — 自动检测并封禁迅雷（Thunder）、QQ旋风、百度网盘 P2P 等只下载不做种的客户端。通过 BitTorrent 握手中的 peer_id 前缀进行检测。**这是 BATorrent 的核心卖点之一——保护你的上传带宽不被吸血客户端窃取。**
- **匿名模式** — 在握手中隐藏客户端名称/版本，禁用 UPnP/NAT-PMP 广告。
- **Tor 代理预设** — 一键填充 SOCKS5 127.0.0.1:9050。
- **强制 IPv4** — 拒绝 IPv6 对等方，防止隧道未覆盖 IPv6 时的地址泄露。
- 支持检测 WireGuard、Mullvad、NordLynx、ProtonVPN 及通用 tun/tap 接口。
- SOCKS5 和 HTTP 代理，支持身份认证。
- IP 屏蔽列表支持（P2P 格式）。
- 协议加密：启用 / 强制 / 禁用。

### WebUI
- 基于浏览器的控制面板，地址为 `http://localhost:8080`（端口和远程访问可配置）。
- **二维码配对** — 用手机扫描二维码即可打开 WebUI，无需输入 IP 地址。二维码由纯 C++ 在本地生成（你的局域网地址绝不会离开本机）。
- REST API，JSON 格式响应；支持磁力链接或 `.torrent` 文件上传添加；暂停 / 恢复 / 删除；按种子查看文件和对等方。
- SHA-256 哈希 Basic Auth，主题匹配的深色 UI，完全响应式移动端布局。

### 带宽与调度
- 独立的下载和上传速度限制。
- 备用速度配置，支持按小时和星期几设定计划（支持跨夜时间段）。
- 按种子和全局的分享率及做种时间限制。

### 媒体服务器集成
- 下载完成时通知 **Plex**、**Jellyfin** 或 **Emby** 扫描媒体库。
- 每个服务器可单独配置 URL 和令牌 / API 密钥。

### 通知与集成
- **Telegram Webhook** — 下载完成、断网保护触发、RSS 自动下载和种子错误等事件推送到任意 Telegram 聊天，通过 Bot Token 实现。支持按事件勾选 + 测试按钮。
- **Discord Rich Presence** — 在 Discord 个人资料上显示"正在下载 X · 67%"，附带"下载 BATorrent"和"在 GitHub 查看"按钮。开箱即用。

### 界面
- 三套主题 — 深色、浅色（暖色奶油"舒适"配色）和午夜 — 通过全局 QPalette 覆盖，使普通控件也跟随当前主题。
- 实时速度图表、详情面板（概要 · 对等方 · 文件 · Tracker · 分块）、按状态着色的进度条、点击聚焦的托盘通知。
- 自定义托盘弹窗（跨平台），显示实时速度、活动种子预览及预计剩余时间、网络代理状态和退出操作。
- 过滤标签及实时计数（全部 / 活动 / 下载中 / 做种中 / 已完成 / 已暂停 / 已结束 / 排队中），搜索栏和分类筛选。
- 支持拖放 `.torrent` 文件和磁力链接。
- **七种界面语言**，支持自动检测：English、Português (BR)、Español、Deutsch、Русский、日本語、中文 — 630+ 条翻译字符串，缺失键自动回退英文。
- 速度显示可选字节（KB/s、MB/s）或比特（Kbps、Mbps）— 在设置中切换。
- 区域感知的数字格式化（例如俄语区域显示 `1 234,5`）。

### 系统
- 自动更新，可配置来源：**GitHub**（默认）或 **Gitee**（中国镜像）或禁用。
- 所有下载完成后自动关机（60 秒可取消倒计时）。
- **完整备份/恢复** 所有设置 + 恢复数据，打包为单个归档文件，方便跨机器迁移。
- **最近删除** 历史记录（最近 50 个种子，一键恢复）。
- **强制启动** — 为单个种子绕过活动下载队列上限。
- 内置**日志查看器**，支持文件轮转（5 MB/文件，保留 3 个）、级别筛选、导出用于提交 Bug 报告，以及 `--debug` 命令行参数。
- **诊断对话框** — 健康检查（保存路径、端口、DHT、加密、网络代理、吸血客户端屏蔽）+ 出站 IP 泄露测试（通过 api.ipify.org）。
- 命令行参数：启动时可传入任意数量的 `.torrent` 路径或 `magnet:` URI；后续启动会转发到已运行的实例。
- 版本更新后首次启动时自动弹出更新日志。
- 快捷键及 `?` 快速参考对话框。

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## 快速开始

1. 从[发布页面](https://github.com/Mateuscruz19/BAT-Torrent/releases/latest)下载适合你平台的版本。
2. 首次启动时，欢迎对话框将引导你设置默认保存路径、主题和语言。
3. 将 `.torrent` 文件或磁力链接拖放到窗口中 — 或使用 **文件 → 打开种子** / **文件 → 添加磁力链接**。
4. 可选：在 **设置 → 网络代理** 中绑定出站接口，并在添加敏感种子前启用断网保护。

> **代理提示：** 当设置了**接口绑定**后，所有宣告和对等方连接都只通过该接口发出。如果接口断开，断网保护将暂停一切直到恢复。

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## 从源码构建

### 依赖要求
- C++17 工具链（GCC 11+、Clang 14+ 或 MSVC 19.30+）
- CMake 3.16+
- Qt 6（`Widgets`、`Network`、`Svg`、`Multimedia`）
- libtorrent-rasterbar 2.0+
- Boost（libtorrent 的传递依赖）
- 可选：Qt6Keychain（将凭据存储在操作系统密钥环中，而非明文 QSettings）

### Linux

```bash
# Debian / Ubuntu
sudo apt install build-essential cmake \
    qt6-base-dev qt6-svg-dev qt6-multimedia-dev \
    libtorrent-rasterbar-dev libboost-dev libssl-dev

# Arch
sudo pacman -S cmake qt6-base qt6-svg qt6-multimedia \
    libtorrent-rasterbar boost openssl

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/BATorrent
```

### macOS

```bash
brew install qt libtorrent-rasterbar boost openssl
cmake -B build -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$(brew --prefix qt);$(brew --prefix libtorrent-rasterbar);$(brew --prefix openssl)"
cmake --build build -j
open build/BATorrent.app
```

### Windows

通过官方安装程序安装 Qt 6，通过 vcpkg 安装 libtorrent：

```powershell
vcpkg install libtorrent:x64-windows
cmake -B build -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake"
cmake --build build --config Release
```

### 测试

测试套件为可选构建：

```bash
cmake -B build -DBAT_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## 项目结构

```
src/
├── torrent/      libtorrent 封装、恢复数据、队列、做种规则
├── app/          翻译器、更新器、RSS、插件、密钥存储、GeoIP
├── gui/          Qt Widgets UI（主窗口、对话框、详情面板、托盘弹窗）
├── webui/        内嵌 HTTP 服务器 + 浏览器 UI
└── main.cpp      单实例启动、命令行解析、主题设置
.github/
└── workflows/    Linux AppImage、macOS DMG、Windows 安装程序 + zip
installer/        用于 Windows 安装程序的 Inno Setup 脚本
dist/             Linux 打包用的 desktop 文件和资源
```

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## 参与贡献

欢迎提交 Issue 和 Pull Request。对于较大的改动，请先开一个 Issue 讨论方案。可以通过 **Build & Release** 工作流（`workflow_dispatch`）为任意分支生成预构建产物。

报告 Bug 时，请附上：
- 平台和版本（`帮助 → 关于`）
- 复现步骤
- `~/.local/share/BATorrent/`（Linux）、`~/Library/Application Support/BATorrent/`（macOS）或 `%APPDATA%\BATorrent\`（Windows）中涉及恢复数据或设置的相关部分。

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## 许可证

[MIT](LICENSE) © 2024–2026 Mateus Cruz
