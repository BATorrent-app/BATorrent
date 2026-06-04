🌐 [English](README.md) | [Português](README.pt-BR.md) | [中文](README.zh-CN.md) | **[日本語](README.ja.md)** | [Русский](README.ru.md) | [Español](README.es.md) | [Deutsch](README.de.md)

<p align="center">
  <img src="src/images/logo.svg" alt="BATorrent" width="160">
</p>

<h1 align="center">BATorrent</h1>

</p>

<p align="center">
  <a href="https://github.com/Mateuscruz19/BATorrent/releases/latest"><img alt="Release" src="https://img.shields.io/github/v/release/Mateuscruz19/BATorrent?style=flat-square&color=dc2626"></a>
  <a href="https://github.com/Mateuscruz19/BATorrent/releases"><img alt="Downloads" src="https://img.shields.io/github/downloads/Mateuscruz19/BATorrent/total?style=flat-square&color=dc2626"></a>
  <a href="LICENSE"><img alt="License" src="https://img.shields.io/github/license/Mateuscruz19/BATorrent?style=flat-square&color=dc2626"></a>
  <img alt="Platforms" src="https://img.shields.io/badge/platform-Windows%20%7C%20macOS%20%7C%20Linux-dc2626?style=flat-square">
  <a href="https://apps.microsoft.com/detail/9n4l3tq24rc6"><img alt="Microsoft Store" src="https://img.shields.io/badge/Microsoft%20Store-available-dc2626?style=flat-square&logo=microsoft"></a>
</p>

<p align="center">
  <a href="https://github.com/Mateuscruz19/BATorrent/actions/workflows/codeql.yml"><img alt="CodeQL" src="https://github.com/Mateuscruz19/BATorrent/actions/workflows/codeql.yml/badge.svg"></a>
  <a href="https://github.com/Mateuscruz19/BATorrent/actions/workflows/sanitizers.yml"><img alt="Sanitizers" src="https://github.com/Mateuscruz19/BATorrent/actions/workflows/sanitizers.yml/badge.svg"></a>
  <a href="https://sonarcloud.io/summary/new_code?id=Mateuscruz19_BAT-Torrent"><img alt="Quality Gate Status" src="https://sonarcloud.io/api/project_badges/measure?project=Mateuscruz19_BAT-Torrent&metric=alert_status"></a>
  <a href="https://www.codefactor.io/repository/github/mateuscruz19/batorrent"><img alt="CodeFactor" src="https://www.codefactor.io/repository/github/mateuscruz19/batorrent/badge"></a>
  <a href="https://www.bestpractices.dev/projects/13073"><img alt="OpenSSF Best Practices" src="https://www.bestpractices.dev/projects/13073/badge"></a>
</p>

<p align="center"><em>プライバシー、パフォーマンス、使いやすさを重視したモダンなクロスプラットフォーム BitTorrent クライアント</em></p>

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## 概要

BATorrent は、明快さ・パフォーマンス・プライバシーを最優先に設計されたデスクトップ BitTorrent クライアントです。成熟した libtorrent-rasterbar エンジンと丁寧にチューニングされた Qt 6 インターフェースを組み合わせ、リモート操作可能な WebUI、RSS 自動ダウンロード、Stremio 互換検索、VPN 対応のトラフィック分離、メディアサーバー連携を内蔵しています。

> **テレメトリなし、アナリティクスなし、外部通信なし。** アプリがユーザーの操作なしに発信する唯一のリクエストは GitHub リリースチェックのみで、設定から無効化できます。[`src/app/updater.cpp`](src/app/updater.cpp) でご自身でご確認ください。

![メインウィンドウ — ダークテーマ](src/images/image1.png)

![メインウィンドウ — ライトテーマ](src/images/image2.png)

![詳細パネル](src/images/image3.png)

![設定ダイアログ](src/images/image4.png)

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## ダウンロード

最新リリースのビルド済みバイナリ：

| プラットフォーム | 形式 | 備考 |
|---|---|---|
| Windows | [インストーラー (`.exe`)](https://github.com/Mateuscruz19/BATorrent/releases/latest) · [ポータブル版 (`.zip`)](https://github.com/Mateuscruz19/BATorrent/releases/latest) | Windows 10+ (x86_64) |
| macOS | **`brew install --cask Mateuscruz19/batorrent/batorrent`**（推奨）· [ディスクイメージ (`.dmg`)](https://github.com/Mateuscruz19/BATorrent/releases/latest) | macOS 12+ (Apple Silicon) |
| Linux | [AppImage](https://github.com/Mateuscruz19/BATorrent/releases/latest) | Glibc 2.35+ (x86_64) |

> **macOS — セキュリティ警告について：** このビルドはまだ公証（notarization）されていません（Apple の開発者プログラムは有料で、個人プロジェクトにはハードルです）。**Homebrew が最もスムーズ**です —— `brew` がインストール時に検疫属性を外すため、Gatekeeper の警告なしにそのまま起動します。`.dmg` を使う場合は、初回のみアプリを右クリックして**開く**を選び、「開発元が未確認」の警告を回避してください。

すべてのアーティファクトは、タグ付きリリースごとに [Build & Release](.github/workflows/build.yml) ワークフローで生成されます。

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## 機能

### トレント
- `.torrent` ファイルとマグネットリンク（永続的なレジュームデータ対応）
- **Thunder:// リンクのデコード** — 中国のフォーラムでは Xunlei（迅雷）の `thunder://` 形式でリンクが共有されます。BATorrent はスマートペースト（Ctrl+V）で自動的にデコードします
- **スマートペースト** — クリップボードにマグネットリンク、info hash、または thunder:// リンクがある状態で Ctrl+V を押すと、即座に追加ダイアログが表示されます
- **トレントインスペクター** — `.torrent` ファイルのプレビュー（名前、サイズ、ファイル構成、トラッカー、ハッシュ、作成者、プライベートフラグ）をダウンロード前に確認可能
- ファイルごとの優先度設定、シーケンシャルダウンロード、手動リチェック・リアナウンス
- [ngosang/trackerslist](https://github.com/ngosang/trackerslist) からのトラッカー自動注入
- マルチタグシステム（自由入力、単一カテゴリと併用可能な複数タグ）
- **コンテンツレイアウト** — オリジナル、サブフォルダ作成、サブフォルダなしでマルチファイルトレントのディスク配置を制御
- **除外ファイルパターン** — トレント追加時にファイルを自動スキップする正規表現ルール（例：`.nfo`、`.txt`、`sample`）
- **一時ダウンロードパス** — まずステージングフォルダにダウンロードし、完了後に保存先へ自動移動（メディアサーバーの部分スキャン防止）
- **アーカイブの自動展開** — 完了時に `.rar`/`.zip`/`.7z` を自動的に展開。パスワード保護されたアーカイブ用のパスワードリスト付き（Windows では 7-Zip または WinRAR、macOS/Linux では `unrar`/`unzip` を使用）
- カテゴリ、ドラッグ＆ドロップ並べ替え、右クリックコンテキスト操作
- qBittorrent からの既存状態のインポート
- 任意のファイルまたはフォルダから新しい `.torrent` ファイルを作成

### 状態管理
- **完了（Completed）状態** — 手動マーク、または設定可能なシード期間（1, 3, 7, 14, 30 日）後に自動昇格。Seeding/Finished とは区別され、再起動後も保持されます。
- **停止ボタン** — 完了したトレントを削除せずに凍結。**再開**で停止を解除し、シードプールに復帰します。
- シード停止ルール：グローバル比率制限と最大シード時間（トレントごとのオーバーライド可能）。
- **ファイルエラー時の自動一時停止** — libtorrent が完了トレントのファイルを読み取れない場合（例：外付けドライブの取り外し）、サイレントに再ダウンロードする代わりに一時停止します。

### ディスカバリー
- **RSS 自動ダウンロード** — 正規表現フィルター、フィードごとの保存パス、インターバルスケジューリング、重複検出対応。マグネットリンク、`.torrent` URL、`<enclosure>` タグに対応。**Nyaa.si などのアニメトラッカーに最適なプリセット**を備えており、新着エピソードの自動取得が簡単に設定できます。
- **Stremio 互換検索** — バンドルされた Cinemeta および Torrentio アドオンによる映画・シリーズの検索。

### ストリーミング
- ダウンロード中に再生可能 — `.mp4`, `.mkv`, `.avi`, `.mov`, `.wmv`, `.flv`, `.webm`, `.m4v`, `.ts`。
- VLC と IINA を自動検出し、見つからない場合はシステムデフォルトプレーヤーにフォールバック。

### VPN とプライバシー
- **インターフェースバインディング** — すべてのトレント通信を指定したネットワークインターフェース（例：`tun0`, `utun4`）に限定します。
- **キルスイッチ** — バインドされたインターフェースがダウンした瞬間、すべてのアクティブなトレントを一時停止。インターフェース復帰時の自動再開オプション付き。
- **PT モード** — プライベートトラッカー準拠をワンタッチで設定：DHT/PEX/LSD を無効化、匿名ハンドシェイクを強制、すべての Tier にアナウンス。M-Team、TorrentLeech などのレシオ管理コミュニティで必須。
- **アンチリーチャーブロック** — Xunlei（迅雷）、QQDownload、Baidu Netdisk P2P など、シードせずにダウンロードだけ行うクライアントを自動検出しブロック。BitTorrent ハンドシェイクの peer_id プレフィックスで検出。
- **匿名モード** — ハンドシェイクでクライアント名/バージョンを隠し、UPnP/NAT-PMP アドバタイズを無効化。**IP アドレスの漏洩リスクを最小限に抑え**、プライバシーを重視するユーザーに最適です。
- **Tor プロキシプリセット** — ワンクリックで SOCKS5 127.0.0.1:9050 を設定。
- **IPv4 強制** — IPv6 ピアを拒否し、トンネルが IPv6 をカバーしていない場合の v6 リークを防止。
- WireGuard、Mullvad、NordLynx、ProtonVPN、汎用 tun/tap の VPN 検出。
- 認証付き SOCKS5 および HTTP プロキシ。
- IP ブロックリスト対応（P2P 形式）。
- プロトコル暗号化：有効 / 強制 / 無効。

### WebUI
- ブラウザベースのコントロールパネル（`http://localhost:8080`、ポートとリモートアクセスは設定可能）。
- **QR コードペアリング** — スマートフォンから QR コードをスキャンするだけで WebUI に即座にアクセス。IP アドレスの入力不要。QR は純粋な C++ でローカル生成されます（LAN アドレスは外部に送信されません）。
- JSON レスポンスの REST API：マグネットまたは `.torrent` アップロードで追加、一時停止/再開/削除、トレントごとのファイル・ピア表示。
- SHA-256 ハッシュ化 Basic Auth、テーマ連動ダーク UI、完全レスポンシブなモバイルレイアウト。

### 帯域幅とスケジューリング
- ダウンロードとアップロードの独立した制限。
- 時間帯＋曜日スケジュール付きの代替速度プロファイル（深夜帯の範囲指定対応）。
- トレントごとおよびグローバルのシード比率・シード時間制限。

### メディアサーバー連携
- ダウンロード完了時に **Plex**、**Jellyfin**、または **Emby** にライブラリスキャンを通知。
- サーバーごとに URL とトークン / API キーを設定可能。

### 通知と連携
- **Telegram Webhook** — ダウンロード完了、キルスイッチ発動、RSS 自動ダウンロード、トレントエラーを bot トークン経由で任意の Telegram チャットにプッシュ。イベントごとのチェックボックスとテストボタン付き。
- **Discord Rich Presence** — Discord プロフィールに「Downloading X · 67%」と表示。「Download BATorrent」「View on GitHub」ボタン付き。設定不要で動作します。

### インターフェース
- **6 つのテーマ** — Dark、Light（暖かいクリーム色の「Comfortable」パレット）、Midnight、Sakura、Dark Star、そして完全な**カスタム**テーマ（独自の背景画像 + アクセントカラー）。それぞれにオプションで**アニメアクセントアート**を設定可能。
- **自動カバーアート** — トレント名から映画/シリーズのポスター（TMDB）やゲームアート（IGDB）を取得し、ポスターの**グリッド表示**に。コンパクトなリスト表示に切り替えも可能。
- リアルタイム速度グラフ、詳細パネル（全般・ピア・ファイル・トラッカー・ピース）、状態色分けプログレスバー、クリックでフォーカスするトレイ通知。
- カスタムトレイポップアップ（クロスプラットフォーム）— リアルタイム速度、アクティブトレントのプレビューと ETA、VPN ステータス、終了操作。
- ライブカウント付きフィルターピル（全て / アクティブ / ダウンロード中 / シード中 / 完了 / 一時停止 / Finished / キュー中）、検索バー、カテゴリフィルター。
- `.torrent` ファイルとマグネットリンクのドラッグ＆ドロップ。
- **7 つの UI 言語**（自動検出対応）：English、Português (BR)、Español、Deutsch、Русский、日本語、中文 — 1,000+ の翻訳文字列（不足キーは英語にフォールバック）。
- 速度表示：バイト単位（KB/s, MB/s）またはビット単位（Kbps, Mbps）— 設定で切り替え可能。
- ロケール対応の数値フォーマット（例：ロシア語ロケールでは `1 234,5`）。

### システム
- 自動アップデート（ソース設定可能）：**GitHub**（デフォルト）、**Gitee**（中国ミラー）、または無効。
- すべてのダウンロード完了時に自動シャットダウン（60 秒のキャンセル可能カウントダウン）。
- **Windows Defender 除外** — ワンクリックでダウンロードフォルダを Defender の除外リストに追加し、ダウンロードしたファイルのフラグ付け/スキャンを停止（UAC 昇格）。
- **フルバックアップ/リストア** — すべての設定とレジュームデータを単一アーカイブにまとめ、マシン間の移行が可能。
- **最近削除した履歴**（直近 50 トレント、ワンクリックリストア）。
- **強制スタート** — 単一トレントのアクティブダウンロードキュー上限をバイパス。
- 内蔵**ログビューワー**（ファイルローテーション：5 MB/ファイル、3 世代保持）、レベルフィルター、バグレポート用エクスポート、`--debug` CLI フラグ。
- **診断ダイアログ** — ヘルスチェック（保存パス、ポート、DHT、暗号化、VPN、リーチャーブロック）+ 送信 IP リークテスト（api.ipify.org 経由）。
- CLI 引数：起動時に任意の数の `.torrent` パスまたは `magnet:` URI を渡せます。後続の起動は実行中のインスタンスに転送されます。
- バージョンアップ後の初回起動時にチェンジログを自動表示。
- キーボードショートカットと `?` クイックリファレンスダイアログ。

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## はじめに

1. お使いのプラットフォーム用のビルドを[リリースページ](https://github.com/Mateuscruz19/BATorrent/releases/latest)からダウンロードしてください。
2. 初回起動時のウェルカムダイアログで、デフォルトの保存パス、テーマ、言語を設定します。
3. `.torrent` ファイルまたはマグネットリンクをウィンドウにドラッグするか、**ファイル → トレントを開く** / **ファイル → マグネットを追加** を使用します。
4. オプション：機密性の高いトレントを追加する前に、**設定 → VPN** で送信インターフェースをバインドし、キルスイッチを有効にしてください。

> **VPN のヒント：** **インターフェースバインディング**が設定されている場合、すべてのアナウンスとピア接続はそのインターフェースのみを経由します。インターフェースがダウンした場合、キルスイッチが復帰するまですべてを一時停止します。

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## ソースからビルド

### 要件
- C++17 ツールチェーン（GCC 11+、Clang 14+、または MSVC 19.30+）
- CMake 3.16+
- Qt 6（`Widgets`, `Network`, `Svg`, `Multimedia`）
- libtorrent-rasterbar 2.0+
- Boost（libtorrent の推移的依存関係）
- オプション：Qt6Keychain（資格情報をプレーンテキストの QSettings ではなく OS キーリングに保存）

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

Qt 6 を公式インストーラーでインストールし、libtorrent を vcpkg でインストールしてください：

```powershell
vcpkg install libtorrent:x64-windows
cmake -B build -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake"
cmake --build build --config Release
```

### テスト

テストスイートはオプトイン方式です：

```bash
cmake -B build -DBAT_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## プロジェクト構成

```
src/
├── torrent/      libtorrent ラッパー、レジュームデータ、キュー、シードルール
├── app/          トランスレーター、アップデーター、RSS、アドオン、シークレットストア、GeoIP
├── gui/          Qt Widgets UI（メインウィンドウ、ダイアログ、詳細、トレイポップアップ）
├── webui/        組み込み HTTP サーバー + ブラウザ UI
└── main.cpp      シングルインスタンス起動、CLI パース、テーマ設定
.github/
└── workflows/    Linux AppImage、macOS DMG、Windows インストーラー + zip
installer/        Windows インストーラー用 Inno Setup スクリプト
dist/             Linux パッケージング用デスクトップファイル + アセット
```

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## コントリビューション

Issue やプルリクエストを歓迎します。大きな変更の場合は、まず Issue を開いてアプローチを議論してください。ビルド済みアーティファクトは **Build & Release** ワークフロー（`workflow_dispatch`）を通じて任意のブランチで生成できます。

バグ報告時には以下を添付してください：
- プラットフォーム + バージョン（`ヘルプ → バージョン情報`）
- 再現手順
- レジュームデータや設定に関連する場合は、`~/.local/share/BATorrent/`（Linux）、`~/Library/Application Support/BATorrent/`（macOS）、`%APPDATA%\BATorrent\`（Windows）の該当部分

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## ライセンス

[MIT](LICENSE) © 2024–2026 Mateus Cruz
