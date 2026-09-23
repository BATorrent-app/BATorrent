<p align="center">
  <a href="README.md">English</a> | <a href="README.pt-BR.md">Português</a> | <b>中文</b> | <a href="README.ja.md">日本語</a> | <a href="README.ru.md">Русский</a> | <a href="README.es.md">Español</a> | <a href="README.de.md">Deutsch</a> | <a href="README.ua.md">Українська</a>
</p>

<p align="center">
  <img src="src/images/logo.svg" alt="BATorrent" width="140">
</p>

<h1 align="center">BATorrent</h1>

<p align="center">
  <i>一个用封面图而不是表格行来展示下载任务的 BitTorrent 客户端。</i>
</p>

<p align="center">
  <a href="https://github.com/BATorrent-app/BATorrent/releases/latest"><img alt="Release" src="https://img.shields.io/github/v/release/BATorrent-app/BATorrent?style=flat-square&color=dc2626"></a>
  <a href="https://github.com/BATorrent-app/BATorrent/releases"><img alt="Downloads" src="https://img.shields.io/github/downloads/BATorrent-app/BATorrent/total?style=flat-square&color=dc2626"></a>
  <a href="LICENSE"><img alt="License" src="https://img.shields.io/github/license/BATorrent-app/BATorrent?style=flat-square&color=dc2626"></a>
  <img alt="Platforms" src="https://img.shields.io/badge/Windows%20·%20macOS%20·%20Linux-dc2626?style=flat-square">
  <a href="https://apps.microsoft.com/detail/9n4l3tq24rc6"><img alt="Microsoft Store" src="https://img.shields.io/badge/Microsoft%20Store-get-dc2626?style=flat-square&logo=microsoft"></a>
</p>

<p align="center">
  <a href="https://batorrent.com/assets/trailer.mp4"><img src="src/images/trailer-poster.jpg" alt="观看 BATorrent 预告片（1:30）" width="860"></a>
</p>

BATorrent 是一款桌面 BT 客户端，基于 [libtorrent](https://www.libtorrent.org/) 引擎，qBittorrent 和 Deluge 用的也是这个引擎。前端会读取每个种子的名称，找到对应的海报（电影和剧集来自 TMDB，游戏来自 IGDB），再把你的下载排成一面封面网格，而不是一串文件名。底层是一个功能完整的客户端，运行在[打过补丁的引擎版本](#引擎)上。

它免费开源，没有广告、遥测、"Pro" 版，也不需要账号。它唯一会主动发起的网络请求是向 GitHub 检查更新，而且有开关可以关掉。想自己确认的话，代码在 [`updater.cpp`](src/services/integrations/updater.cpp)。

## 为什么做这个项目

我是巴西的一名独立开发者。我想要一个认真对待隐私、在 Windows、macOS 和 Linux 上原生运行、看起来也不像 2009 年设计的 BT 客户端。找不到满意的，就自己写了一个。项目采用 MIT 许可，所以哪天它要是加了遥测或广告，任何人都可以 fork 代码，发布一个去掉这些东西的版本。界面已翻译成九种语言。

## 界面

<p align="center">
  <img src="src/images/shot-list.jpg" alt="紧凑列表视图，适合更看重信息而非装饰的时候" width="860">
</p>

<p align="center">
  <img src="src/images/shot-palette-v43.jpg" alt="命令面板（Ctrl/⌘+K）：模糊查找任意种子或操作" width="860">
</p>

<p align="center">
  <img src="src/images/shot-theme.jpg" alt="Sakura，内置主题之一" width="860">
</p>

- **封面图。** 根据种子名称匹配海报，并以网格展示。想看详细信息时，点一下就能切换到紧凑列表。
- **六款主题。** Dark、Light、Midnight、Sakura、Dark Star，以及一个可以自选背景和强调色的 Custom 主题。每款主题都可以选配动漫风格的点缀插画。
- **命令面板。** Ctrl/⌘+K 打开模糊查找，可以找任意种子或操作：全部暂停、切换备用限速、跳转到任意页面。全程不用鼠标。
- **实时状态。** 实时速度曲线、按状态着色的进度条，以及显示当前速度和剩余时间的托盘弹窗。

## 功能

**在应用里直接看。** 内置视频播放器（基于 FFmpeg，可以直接播放 MKV、AVI 和 WebM）。因为会优先下载文件开头部分，所以文件还在下载时就能开始看。它会帮你搜索并下载字幕（通过 SubDL），自动加载同目录的 `.srt`/`.vtt` 文件，还能在播放时实时微调字幕同步。下载完成后可以刷新 Plex、Jellyfin 或 Emby 媒体库。

**借助 debrid 即时播放。** 连接 [Real-Debrid](https://real-debrid.com) 或 [TorBox](https://torbox.app) 账号后，如果某个磁力链接在他们那边已有缓存，BATorrent 会解析出直链，直接推流到内置播放器，你的电脑上不会下载也不会做种。

**也支持游戏。** 游戏种子同样有封面（通过 IGDB）。你可以搜索游戏目录、下载，然后在应用内安装和启动，这样你的盗版游戏库用起来有点像 Steam 列表，而不是一个塞满安装程序的文件夹。

**发现。** 一个可浏览的 Netflix 风格首页（热门海报、轮播大图），不用离开应用就能找到想下载的内容。

<p align="center">
  <img src="src/images/shot-discover.jpg" alt="发现页：可浏览的热门海报首页" width="860">
</p>

**隐私。** 可以绑定到指定的 VPN 网络接口，并配有断网保护（kill switch），隧道一断就切断所有流量。另外还有私有站（PT）模式、Tor 预设、匿名握手，以及屏蔽吸血客户端的功能。内置 IP 泄漏测试，可以确认这些设置确实生效。

**查找与添加。** 内置搜索（包括无需登录的开放 CIS/RuTor 源）；Smart Paste 在按下 Ctrl+V 时能识别磁力链接、`.torrent`、`thunder://` 链接或 info hash；支持带正则过滤的 RSS 自动下载、监视文件夹和拖放。

<p align="center">
  <img src="src/images/shot-search.jpg" alt="内置搜索：封面图、评分和即时最佳匹配" width="860">
</p>

**远程控制。** 浏览器 WebUI 支持二维码配对：用手机扫码即可，不用手动输入 IP 地址。二维码在你的电脑上生成，地址不会离开本机。

**整理。** 下载完成后自动解压压缩包，用分类和标签整理，为单个种子或全局设置分享率和时间限制，还能按小时和星期安排带宽。

**通知。** 原生桌面通知、Telegram 消息和 Discord Rich Presence。

<details>
<summary><b>完整功能列表</b></summary>

单文件优先级、顺序下载、自动注入 Tracker、内容布局控制、排除文件正则、独立的临时下载目录、带做种时间窗口的"已完成"状态、文件出错时自动暂停、全局和单种子的分享率与时间限制、按小时和星期的带宽计划、从 qBittorrent 导入、创建 `.torrent` 文件、种子检查器、IP 屏蔽列表、协议加密、Gitee 更新镜像、下载完成后自动关机、Windows Defender 排除辅助、完整备份与恢复、最近删除记录、强制开始、带诊断和 IP 泄漏测试的内置日志查看器、按区域设置格式化，以及键盘快捷键。

</details>

## 引擎

大多数 BT 应用链接的是原版 libtorrent。BATorrent 附带一个小幅打过补丁的分支，因此能改动公共 API 触及不到的引擎行为：

- **更快的管线爬升。** 在高带宽、高延迟的线路上，原版的请求管线每次只增长一步；这个分支让它按几何级数增长，只需少量往返就能把宽带跑满。在项目自己的 A/B 基准测试中，快速线路上实测提升约 +27%，没有原版那种每次运行间的卡顿，也从未出现性能倒退。
- **同国 peer 优先。** 离线 GeoIP 数据库（db-ip Lite）会按国家标记每个 peer。在有选择的情况下，分支的 peer 排序会优先选择和你同一国家的 peer，这通常意味着更低的延迟，也更少遇到被限速的跨境线路。

这两项都是分支的编译期特性，在原版构建中是关闭的。它们以带版本号的补丁形式放在 [`third_party/patches/`](third_party/patches) 下，而不是直接内嵌一份源码副本。

## 安装

| 平台 | 下载 | 系统要求 |
|---|---|---|
| **Windows** | [Microsoft Store](https://apps.microsoft.com/detail/9n4l3tq24rc6)、[安装版](https://batorrent.com/win)或[便携版](https://batorrent.com/portable) | Windows 10 或更高版本 |
| **macOS** | `brew install --cask Mateuscruz19/batorrent/batorrent` 或 [`.dmg`](https://batorrent.com/mac) | macOS 12+，Apple Silicon |
| **Linux** | [AppImage](https://batorrent.com/linux) | glibc 2.35+ |

运行后，把 `.torrent` 文件或磁力链接拖到窗口里即可。

<sub><b>macOS 说明：</b>应用还没有经过公证（Apple 开发者计划需要付费订阅）。用 Homebrew 安装最省事，因为 <code>brew</code> 会移除隔离标记，打开时不会弹出 Gatekeeper 提示。如果用 <code>.dmg</code> 安装，第一次请右键点击应用，选择<b>打开</b>。</sub>

<details>
<summary><b>从源码构建</b></summary>

**依赖：** C++17、CMake 3.16+、Qt 6（`Widgets`、`Network`、`Svg`、`Multimedia`、`Quick`、`QuickWidgets`、`QuickControls2`）、libtorrent-rasterbar 2.0+、Boost，以及可选的 Qt6Keychain。

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

- 每次 CI 构建都会运行 Catch2 测试套件（单元、安全、内存）；新的后端行为都会附带测试。
- 构建在 AddressSanitizer 和 UndefinedBehaviorSanitizer 下干净通过。
- 每次发布前都会审查代码的内存与线程安全、WebUI 鉴权、注入、路径穿越、输入校验和密钥处理。密钥存放在系统钥匙串中，不以明文保存；只有在你设置密码之后，WebUI 才会对网络开放。

</details>

## 参与贡献

欢迎提交 Issue 和 Pull Request。较大的改动请先开一个 Issue，方便我们先就做法达成一致。提交 Bug 报告时，最好附上你的平台和版本号（见 `帮助 → 关于`）以及复现步骤。尤其欢迎帮忙翻译。

## 许可证与商标

**代码**采用 [MIT](LICENSE) 许可，© 2024-2026 Mateus Cruz。你可以自由 fork 并发布自己的构建版本。

**"BATorrent" 这个名称和 logo** 归项目所有，不在代码许可范围内。如果你要再分发某个 fork，请给它起一个自己的名字，方便用户分辨哪个才是官方版本。详情见 [TRADEMARK.md](TRADEMARK.md)。欢迎善意的 fork 和贡献。

于巴西制作。
