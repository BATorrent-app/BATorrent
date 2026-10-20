<p align="center">
  <a href="README.md">English</a> | <a href="README.pt-BR.md">Português</a> | <b>中文</b> | <a href="README.ja.md">日本語</a> | <a href="README.ru.md">Русский</a> | <a href="README.es.md">Español</a> | <a href="README.de.md">Deutsch</a> | <a href="README.ua.md">Українська</a>
</p>

<p align="center">
  <img src="src/images/logo.svg" alt="BATorrent" width="120">
</p>

<h1 align="center">BATorrent</h1>

<p align="center">
  一个用封面图展示下载、边下边播，<br>
  并把电影、剧集和游戏放进同一个媒体库的 BT 客户端。
</p>

<p align="center">
  <a href="https://github.com/BATorrent-app/BATorrent/releases/latest"><img alt="Release" src="https://img.shields.io/github/v/release/BATorrent-app/BATorrent?style=flat-square&color=dc2626"></a>
  <a href="https://github.com/BATorrent-app/BATorrent/releases"><img alt="Downloads" src="https://img.shields.io/github/downloads/BATorrent-app/BATorrent/total?style=flat-square&color=dc2626"></a>
  <a href="LICENSE"><img alt="License" src="https://img.shields.io/github/license/BATorrent-app/BATorrent?style=flat-square&color=dc2626"></a>
  <img alt="Platforms" src="https://img.shields.io/badge/Windows%20·%20macOS%20·%20Linux-dc2626?style=flat-square">
  <a href="https://apps.microsoft.com/detail/9n4l3tq24rc6"><img alt="Microsoft Store" src="https://img.shields.io/badge/Microsoft%20Store-get-dc2626?style=flat-square&logo=microsoft"></a>
</p>

<p align="center">
  <a href="#安装"><b>下载</b></a> &nbsp;·&nbsp;
  <a href="https://batorrent.com/assets/trailer.mp4"><b>观看预告片</b></a> &nbsp;·&nbsp;
  <a href="https://batorrent.com"><b>官网</b></a>
</p>

<p align="center">
  <img src="docs/images/grid.jpg" alt="网格视图下的“下载”标签页，选中了一部剧集，右侧是它的各集" width="900">
</p>

BATorrent 是一款基于 [libtorrent](https://www.libtorrent.org/) 的桌面 BT 客户端，qBittorrent 和 Deluge 用的也是这个引擎。它会读取每个种子的名称，找到对应的海报（电影和剧集来自 TMDB，游戏来自 IGDB），把你的下载排成一张张封面，而不是一串文件名。同一部剧的各集会归到一起，视频在下载完成前就能在应用里播放，游戏也能在同一个窗口里安装和启动。

它免费开源，没有广告、账号、"Pro" 版，也没有数据分析。除了种子本身的流量，它只会因为三件事联网：为你媒体库里的内容或你在“查找”中打开的内容，从 TMDB、IGDB 和 Steam 获取封面和详情；向 GitHub 检查更新，可以关掉（[`updater.cpp`](src/services/integrations/updater.cpp)）；以及在官方构建中，应用崩溃时发送崩溃报告。

## 三个标签页

应用顶部有三个标签页：**下载**、**查找** 和 **HUB**。

### 下载

你的种子以封面网格或紧凑列表显示，选中项的详情显示在侧边。剧集会折叠成一行，各集收在里面，所以一部剧的十二集只占一行，而不是十二行。

<p align="center">
  <img src="docs/images/list.jpg" alt="列表视图下的“下载”标签页，一部剧集展开显示各集" width="900">
</p>

### 查找和 HUB

**查找** 是找新内容的地方：首页是热门的电影、剧集和游戏，搜索会同时查你所有的来源，并把最匹配的结果放在最前面。每个作品都有自己的页面。电影页面列出它的各个发布版本，按分享人数排序，给出一个推荐选择，下面是同系列的其他电影。剧集页面按季列出各集，并标出你已经有的和看过的。

<table>
  <tr>
    <td width="50%"><img src="docs/images/find.jpg" alt="查找：一部电影的页面，包括发布版本和同系列的其他电影"></td>
    <td width="50%"><img src="docs/images/series.jpg" alt="查找：一部剧集的页面，显示各集、看过的和已在媒体库中的"></td>
  </tr>
  <tr>
    <td align="center"><sub>在“查找”中打开的电影</sub></td>
    <td align="center"><sub>在“查找”中打开的剧集</sub></td>
  </tr>
</table>

**HUB** 是你已有内容的媒体库：继续观看、继续游戏，所有内容按游戏、剧集和电影分组。

<p align="center">
  <img src="docs/images/hub.jpg" alt="HUB：继续观看、继续游戏和媒体库" width="900">
</p>

## 播放器

内置播放器基于 FFmpeg，可以直接播放 MKV、AVI 和 WebM。它会先下载文件开头部分，所以剩下的还在下载时你就能开始看。它会记住你停在哪里，可以跳过片头，播完后会提供下一集，或者同系列的下一部电影。字幕可以自动帮你找（通过 SubDL），也能加载文件旁边的 `.srt`。解码在一个单独的沙盒进程里进行，所以损坏的文件不会把整个应用一起拖垮。

<p align="center">
  <img src="docs/images/player.jpg" alt="内置播放器，下一集已经准备好" width="900">
</p>

如果你有 [Real-Debrid](https://real-debrid.com) 或 [TorBox](https://torbox.app) 账号，而且这个种子在他们那边已有缓存，就会改为从他们那里串流，你的电脑上不会下载也不会做种。

## 其他功能

- **游戏。** 来自 IGDB 的封面，在 HUB 里安装和启动，游戏时长和成就。你下载的游戏看起来像一个启动器，而不是一个塞满安装程序的文件夹。
- **VPN。** 导入 WireGuard 的 `.conf`（Proton、Mullvad、IVPN 等）就能连接。只有 BATorrent 走这条通道，电脑上的其他程序照常联网，系统里也不会安装任何东西。断线后会自动重连，恢复之前断网保护（kill switch）会暂停种子；在支持的服务器上，它还会开放一个端口，让 peer 能连到你。
- **隐私。** 显示 peer 看到的 IP 的泄漏测试、绑定网络接口、私有站（PT）模式、Tor 预设，以及屏蔽已知的吸血客户端。
- **添加内容。** 在任何地方按 Ctrl+V 粘贴磁力链接、链接或 info hash。支持带正则过滤的 RSS、监视文件夹、拖放，以及普通的 HTTP/HTTPS 下载。
- **远程控制。** 用手机扫二维码就能打开的 WebUI。二维码在你的电脑上生成。
- **整理。** 分类和标签、自动解压压缩包、分享率和时间限制、按小时和星期安排的带宽计划，还有点一下就能切换的备用速度（那只乌龟）。
- **外观。** 七款主题，每款都有自己的字体和插画，也可以用你自己做的主题；背景可以是纯色、主题插画或你自己的图片。Ctrl/⌘+K 打开命令面板。支持九种语言，首次运行时有一个简短的设置向导。

<p align="center">
  <img src="docs/images/themes.jpg" alt="七款主题下的“下载”标签页：Default、Cinema、Light、Midnight、Sakura、Dark Star 和 Matrix" width="900">
</p>

<details>
<summary><b>完整功能列表</b></summary>

单文件优先级、顺序下载、注入 Tracker、内容布局控制、排除文件正则、独立的临时文件夹、做种时间窗口、文件丢失时自动暂停、刷新 Plex/Jellyfin/Emby 媒体库、Telegram 和 Discord 通知、从 qBittorrent 导入、创建 `.torrent`、IP 屏蔽列表、协议加密、Gitee 更新镜像、稳定版和测试版更新通道、下载完成后关机、Windows Defender 排除辅助、备份与恢复、最近删除记录、强制开始、带诊断功能的日志查看器、按 Alt 显示的菜单栏、在媒体库中把同系列的电影归为一组，以及覆盖大多数操作的键盘快捷键。

</details>

## 引擎

BATorrent 附带一个小幅打过补丁的 libtorrent 分支，补丁以带版本号的形式放在 [`third_party/patches/`](third_party/patches) 下：

- **更快的爬升。** 在高速但高延迟的线路上，原版 libtorrent 的请求管线每次只增长一步。这个分支让它按几何级数增长，用少得多的往返次数就能把线路跑满。在项目的 A/B 测试中，快速线路上大约快 27%，而且从没比原版慢过。
- **同国 peer。** 离线 GeoIP 数据库（db-ip Lite）会按国家标记每个 peer，在有选择的情况下，这个分支会优先选择和你同一国家的 peer，这通常意味着更低的延迟。

两者都是编译期开关，在原版构建中是关闭的。

## 安装

| 平台 | 下载 | 系统要求 |
|---|---|---|
| **Windows** | [Microsoft Store](https://apps.microsoft.com/detail/9n4l3tq24rc6) · [安装版](https://batorrent.com/win) · [便携版](https://batorrent.com/portable) | Windows 10 或更高版本 |
| **macOS** | `brew install --cask Mateuscruz19/batorrent/batorrent` 或 [`.dmg`](https://batorrent.com/mac) | macOS 12+，Apple Silicon |
| **Linux** | [AppImage](https://batorrent.com/linux) | glibc 2.35+ |

打开后，把 `.torrent` 或磁力链接拖到窗口里即可。

<sub><b>macOS：</b>应用还没有经过公证（Apple 开发者计划需要付费订阅）。用 Homebrew 安装最省事，因为它会移除隔离标记。如果用 <code>.dmg</code>，第一次请右键点击应用，选择<b>打开</b>。</sub>

<details>
<summary><b>从源码构建</b></summary>

**依赖：** C++17、CMake 3.16+、Qt 6（`Widgets`、`Network`、`Svg`、`Multimedia`、`Quick`、`QuickWidgets`、`QuickControls2`）、libtorrent-rasterbar 2.0+、Boost、zlib，以及可选的 Qt6Keychain。

```bash
# Debian / Ubuntu
sudo apt install build-essential cmake qt6-base-dev qt6-svg-dev qt6-multimedia-dev \
    libtorrent-rasterbar-dev libboost-dev libssl-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j && ./build/BATorrent
```

macOS：`brew install qt libtorrent-rasterbar boost openssl`。
Windows：Qt 安装器，加上 `vcpkg install libtorrent:x64-windows`。

</details>

<details>
<summary><b>质量与安全</b></summary>

<p>
  <a href="https://github.com/BATorrent-app/BATorrent/actions/workflows/codeql.yml"><img alt="CodeQL" src="https://github.com/BATorrent-app/BATorrent/actions/workflows/codeql.yml/badge.svg"></a>
  <a href="https://github.com/BATorrent-app/BATorrent/actions/workflows/sanitizers.yml"><img alt="Sanitizers" src="https://github.com/BATorrent-app/BATorrent/actions/workflows/sanitizers.yml/badge.svg"></a>
  <a href="https://sonarcloud.io/summary/new_code?id=Mateuscruz19_BAT-Torrent"><img alt="Quality Gate" src="https://sonarcloud.io/api/project_badges/measure?project=Mateuscruz19_BAT-Torrent&metric=alert_status"></a>
  <a href="https://www.codefactor.io/repository/github/mateuscruz19/batorrent"><img alt="CodeFactor" src="https://www.codefactor.io/repository/github/mateuscruz19/batorrent/badge"></a>
  <a href="https://www.bestpractices.dev/projects/13073"><img alt="OpenSSF Best Practices" src="https://www.bestpractices.dev/projects/13073/badge"></a>
</p>

- 每次 CI 构建都会运行 Catch2 测试套件（单元、安全、内存）和 QML 测试。
- 构建在 AddressSanitizer 和 UndefinedBehaviorSanitizer 下干净通过。
- 密钥存放在系统钥匙串中；只有在你设置密码之后，WebUI 才会对网络开放。

</details>

## 为什么做这个项目

我是巴西的一名独立开发者。我想要一个认真对待隐私、在 Windows、macOS 和 Linux 上原生运行、看起来也不像 2009 年做的 BT 客户端。找不到满意的，就自己写了一个。项目采用 MIT 许可，所以哪天它要是加了广告或追踪，任何人都可以 fork 一份，发布一个去掉这些东西的版本。

## 参与贡献

欢迎提交 Issue 和 Pull Request。比小修复更大的改动，请先开一个 Issue，方便我们先就做法达成一致。Bug 报告最好附上你的平台、版本号（见 `帮助 → 关于 BATorrent`）以及复现步骤。尤其欢迎帮忙翻译。如果你使用 AI 工具，请先阅读[我如何使用 AI](USING_AI.md)。

## 许可证与商标

**代码**采用 [MIT](LICENSE) 许可，© 2024-2026 Mateus Cruz。你可以自由 fork 并发布自己的构建版本。

**"BATorrent" 这个名称和 logo** 不在代码许可范围内。如果你要发布某个 fork，请给它起一个自己的名字，方便大家分辨哪个才是官方版本。详情见 [TRADEMARK.md](TRADEMARK.md)。

<p align="center"><sub>于巴西制作。</sub></p>
