<p align="center">
  <a href="README.md">English</a> | <a href="README.pt-BR.md">Português</a> | <a href="README.zh-CN.md">中文</a> | <b>日本語</b> | <a href="README.ru.md">Русский</a> | <a href="README.es.md">Español</a> | <a href="README.de.md">Deutsch</a> | <a href="README.ua.md">Українська</a>
</p>

<p align="center">
  <img src="src/images/logo.svg" alt="BATorrent" width="140">
</p>

<h1 align="center">BATorrent</h1>

<p align="center">
  <i>ダウンロードを表計算の行ではなく、カバーアートで表示する BitTorrent クライアント。</i>
</p>

<p align="center">
  <a href="https://github.com/BATorrent-app/BATorrent/releases/latest"><img alt="Release" src="https://img.shields.io/github/v/release/BATorrent-app/BATorrent?style=flat-square&color=dc2626"></a>
  <a href="https://github.com/BATorrent-app/BATorrent/releases"><img alt="Downloads" src="https://img.shields.io/github/downloads/BATorrent-app/BATorrent/total?style=flat-square&color=dc2626"></a>
  <a href="LICENSE"><img alt="License" src="https://img.shields.io/github/license/BATorrent-app/BATorrent?style=flat-square&color=dc2626"></a>
  <img alt="Platforms" src="https://img.shields.io/badge/Windows%20·%20macOS%20·%20Linux-dc2626?style=flat-square">
  <a href="https://apps.microsoft.com/detail/9n4l3tq24rc6"><img alt="Microsoft Store" src="https://img.shields.io/badge/Microsoft%20Store-get-dc2626?style=flat-square&logo=microsoft"></a>
</p>

<p align="center">
  <a href="https://batorrent.com/assets/trailer.mp4"><img src="src/images/trailer-poster.jpg" alt="BATorrent のトレーラーを見る (1:30)" width="860"></a>
</p>

BATorrent は [libtorrent](https://www.libtorrent.org/) エンジンで動くデスクトップ向けトレントクライアントです。qBittorrent や Deluge と同じエンジンを使っています。フロントエンドは各トレントの名前を読み取って対応するポスターを探し (映画やドラマは TMDB、ゲームは IGDB から取得)、ダウンロードをファイル名の一覧ではなくカバーのグリッドとして並べます。その下では、[パッチを当てたエンジン](#エンジン) の上でフル機能のクライアントが動いています。

無料のオープンソースで、広告、テレメトリ、「Pro」プラン、アカウントはありません。アプリが自分から行う通信は GitHub への更新確認だけで、これもスイッチでオフにできます。確かめたい場合は [`updater.cpp`](src/services/integrations/updater.cpp) のコードを見てください。

## なぜ作ったのか

私はブラジルに住む個人開発者です。プライバシーをきちんと扱い、Windows、macOS、Linux でネイティブに動き、2009年に作られたような見た目ではないトレントクライアントが欲しかったのですが、気に入るものが見つからなかったので自分で書きました。MIT ライセンスなので、もしこのプロジェクトがテレメトリや広告を追加したとしても、誰でもコードをフォークしてそれらを外したものを配布できます。インターフェイスは9言語に翻訳されています。

## インターフェイス

<p align="center">
  <img src="src/images/shot-list.jpg" alt="見た目より情報量を優先したいときのコンパクトなリスト表示" width="860">
</p>

<p align="center">
  <img src="src/images/shot-palette-v43.jpg" alt="コマンドパレット (Ctrl/⌘+K): どのトレントや操作もあいまい検索で呼び出せる" width="860">
</p>

<p align="center">
  <img src="src/images/shot-theme.jpg" alt="内蔵テーマのひとつ、Sakura" width="860">
</p>

- **カバーアート。** トレント名からポスターを取得してグリッドに表示します。見た目より情報量が欲しいときは、1クリックでコンパクトなリスト表示に切り替えられます。
- **6つのテーマ。** Dark、Light、Midnight、Sakura、Dark Star、それに背景とアクセントカラーを自分で選べる Custom テーマがあります。どのテーマにもアニメ風のアクセントアートを付けられます (任意)。
- **コマンドパレット。** Ctrl/⌘+K であいまい検索が開き、トレントや操作をすぐ呼び出せます。すべて一時停止、代替速度の切り替え、任意のページへの移動などが、マウスなしでできます。
- **リアルタイムの状態表示。** リアルタイムの速度グラフ、状態ごとに色分けされた進捗バー、現在の速度と残り時間を表示するトレイポップアップがあります。

## できること

**アプリ内で視聴。** 動画プレーヤーを内蔵しています (FFmpeg ベースなので MKV、AVI、WebM をそのまま再生できます)。ファイルの先頭から先に取得するので、ダウンロード中でも再生を始められます。字幕の検索とダウンロード (SubDL 経由) も行い、同じフォルダの `.srt`/`.vtt` ファイルは自動で読み込み、再生しながらタイミングを微調整できます。ダウンロード完了時に Plex、Jellyfin、Emby のライブラリを更新することもできます。

**debrid で即時再生。** [Real-Debrid](https://real-debrid.com) か [TorBox](https://torbox.app) のアカウントを接続すると、magnet がすでに向こう側にキャッシュされている場合、BATorrent はリンクの制限を解除して内蔵プレーヤーに直接ストリーミングします。手元のマシンでダウンロードやシードは発生しません。

**ゲームにも対応。** ゲームのトレントにもカバーアートが付きます (IGDB 経由)。ゲームのカタログを検索してダウンロードし、そのままアプリ内でインストールと起動ができるので、海賊版ライブラリがセットアップファイルの詰まったフォルダではなく、Steam のリストに少し近い使い心地になります。

**Discover。** Netflix 風に眺められるトップページ (トレンドのポスター、切り替わるヒーロー表示) があり、アプリを離れずに次にダウンロードするものを探せます。

<p align="center">
  <img src="src/images/shot-discover.jpg" alt="Discover: トレンドのポスターを眺められるトップページ" width="860">
</p>

**プライバシー。** 特定の VPN インターフェイスにバインドでき、トンネルが切れたら全通信を遮断するキルスイッチも備えています。ほかにプライベートトラッカー用モード、Tor プリセット、匿名ハンドシェイク、リーチャーのクライアントのブロックがあります。正しく機能しているかは内蔵の IP リークテストで確認できます。

**探して追加。** 内蔵検索 (ログイン不要の CIS/RuTor のオープンなソースも含む)、Ctrl+V で magnet、`.torrent`、`thunder://` リンク、info hash を認識する Smart Paste、正規表現フィルタ付きの RSS 自動ダウンロード、監視フォルダ、ドラッグ＆ドロップに対応しています。

<p align="center">
  <img src="src/images/shot-search.jpg" alt="内蔵検索: カバーアート、評価、いちばん合う結果をすぐ表示" width="860">
</p>

**リモート操作。** ブラウザから使える WebUI があり、QR コードでペアリングできます。IP アドレスを打ち込む代わりに、スマホでコードを読み取るだけです。QR はあなたのマシン上で生成され、アドレスが外に出ることはありません。

**整理。** 完了時のアーカイブ自動展開、カテゴリとタグによる分類、トレント別と全体のレシオ制限と時間制限、時間帯と曜日ごとの帯域スケジュールに対応しています。

**通知。** ネイティブのデスクトップ通知、Telegram へのメッセージ、Discord Rich Presence に対応しています。

<details>
<summary><b>全機能一覧</b></summary>

ファイルごとの優先度、順次ダウンロード、トラッカーの自動追加、コンテンツ配置の制御、除外ファイルの正規表現、一時ダウンロード先の個別指定、シード期間付きの完了状態、ファイルエラー時の自動一時停止、全体とトレント別のレシオ制限と時間制限、時間帯と曜日による帯域スケジューラ、qBittorrent からのインポート、`.torrent` の作成、トレントインスペクタ、IP ブロックリスト、プロトコル暗号化、Gitee の更新ミラー、ダウンロード完了時の自動シャットダウン、Windows Defender の除外設定ヘルパー、完全なバックアップと復元、最近削除した項目の履歴、強制開始、診断と IP リークテスト付きの内蔵ログビューア、ロケールに合わせた表示形式、キーボードショートカット。

</details>

## エンジン

多くのトレントアプリは標準の libtorrent をそのままリンクしています。BATorrent はこれに小さなパッチを当てたフォークを同梱しているので、公開 API からは変えられないエンジンの挙動にも手を入れられます。

- **パイプラインの立ち上がりが速い。** 帯域が広く遅延の大きい回線では、標準のリクエストパイプラインは1段ずつしか伸びません。フォークでは等比級数的に伸ばすので、太い回線をずっと少ない往復回数で埋められます。プロジェクト独自の A/B ベンチマークでは、高速回線で約 +27% を計測しました。標準版で起きる実行ごとの停滞もなく、性能が落ちるケースもありません。
- **同じ国のピアを優先。** オフラインの GeoIP データベース (db-ip Lite) で各ピアに国を割り当て、フォークのピアランキングは選べる場合に自分と同じ国のピアを優先します。その結果、遅延が小さくなり、帯域制限のかかった国際経路を通ることが減る傾向があります。

どちらもフォークのコンパイル時機能で、標準ビルドでは無効です。コードを丸ごと取り込むのではなく、[`third_party/patches/`](third_party/patches) 以下のバージョン管理されたパッチとして適用しています。

## インストール

| プラットフォーム | ダウンロード | 動作環境 |
|---|---|---|
| **Windows** | [Microsoft Store](https://apps.microsoft.com/detail/9n4l3tq24rc6)、[インストーラー](https://batorrent.com/win)、[ポータブル版](https://batorrent.com/portable) | Windows 10 以降 |
| **macOS** | `brew install --cask Mateuscruz19/batorrent/batorrent` または [`.dmg`](https://batorrent.com/mac) | macOS 12+、Apple Silicon |
| **Linux** | [AppImage](https://batorrent.com/linux) | glibc 2.35+ |

起動したら、`.torrent` ファイルか magnet リンクをウィンドウにドロップしてください。

<sub><b>macOS についての注意:</b> アプリはまだ公証 (notarization) を受けていません (Apple の開発者プログラムは有料のサブスクリプションのため)。<code>brew</code> が隔離フラグを外してくれるので、Homebrew を使うのがいちばん簡単で、Gatekeeper のダイアログも出ません。<code>.dmg</code> を使う場合は、初回だけアプリを右クリックして<b>開く</b>を選んでください。</sub>

<details>
<summary><b>ソースからビルド</b></summary>

**必要なもの:** C++17、CMake 3.16+、Qt 6 (`Widgets`, `Network`, `Svg`, `Multimedia`, `Quick`, `QuickWidgets`, `QuickControls2`)、libtorrent-rasterbar 2.0+、Boost、必要に応じて Qt6Keychain。

```bash
# Debian / Ubuntu
sudo apt install build-essential cmake qt6-base-dev qt6-svg-dev qt6-multimedia-dev \
    libtorrent-rasterbar-dev libboost-dev libssl-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j && ./build/BATorrent
```

macOS の場合: `brew install qt libtorrent-rasterbar boost openssl`。
Windows の場合: Qt インストーラーと `vcpkg install libtorrent:x64-windows`。

</details>

<details>
<summary><b>品質とセキュリティ</b></summary>

<p>
  <a href="https://github.com/BATorrent-app/BATorrent/actions/workflows/codeql.yml"><img alt="CodeQL" src="https://github.com/BATorrent-app/BATorrent/actions/workflows/codeql.yml/badge.svg"></a>
  <a href="https://github.com/BATorrent-app/BATorrent/actions/workflows/sanitizers.yml"><img alt="Sanitizers" src="https://github.com/BATorrent-app/BATorrent/actions/workflows/sanitizers.yml/badge.svg"></a>
  <a href="https://sonarcloud.io/summary/new_code?id=Mateuscruz19_BAT-Torrent"><img alt="Quality Gate" src="https://sonarcloud.io/api/project_badges/measure?project=Mateuscruz19_BAT-Torrent&metric=alert_status"></a>
  <a href="https://www.codefactor.io/repository/github/mateuscruz19/batorrent"><img alt="CodeFactor" src="https://www.codefactor.io/repository/github/mateuscruz19/batorrent/badge"></a>
  <a href="https://www.bestpractices.dev/projects/13073"><img alt="OpenSSF Best Practices" src="https://www.bestpractices.dev/projects/13073/badge"></a>
</p>

- CI のビルドごとに Catch2 のテストスイート (ユニット、セキュリティ、メモリ) を実行しています。バックエンドに新しい挙動を加えるときは、テストも一緒に追加します。
- ビルドは AddressSanitizer と UndefinedBehaviorSanitizer の下で問題なく通ります。
- リリースの前には毎回、メモリとスレッドの安全性、WebUI の認証、インジェクション、パストラバーサル、入力検証、シークレットの扱いについてコードをレビューしています。シークレットは平文ではなく OS のキーチェーンに保存し、WebUI はパスワードを設定するまでネットワークに公開されません。

</details>

## コントリビュート

Issue とプルリクエストを歓迎します。小さな修正でない場合は、進め方をすり合わせるために先に Issue を立ててください。バグ報告には、プラットフォームとバージョン (`ヘルプ → バージョン情報` で確認できます)、再現手順を書いてもらえると助かります。翻訳は特に歓迎しています。

## ライセンスと商標

**コード** のライセンスは [MIT](LICENSE) です。© 2024-2026 Mateus Cruz。自由にフォークして、独自のビルドを配布できます。

**「BATorrent」という名前とロゴ** はプロジェクトに帰属し、コードのライセンスの対象外です。フォークを再配布する場合は、どれが公式ビルドかユーザーが見分けられるよう、別の名前を付けてください。詳しくは [TRADEMARK.md](TRADEMARK.md) にあります。善意のフォークやコントリビュートは歓迎します。

Made in Brazil.
