<p align="center">
  <a href="README.md">English</a> | <a href="README.pt-BR.md">Português</a> | <a href="README.zh-CN.md">中文</a> | <b>日本語</b> | <a href="README.ru.md">Русский</a> | <a href="README.es.md">Español</a> | <a href="README.de.md">Deutsch</a> | <a href="README.ua.md">Українська</a>
</p>

<p align="center">
  <img src="src/images/logo.svg" alt="BATorrent" width="120">
</p>

<h1 align="center">BATorrent</h1>

<p align="center">
  ダウンロードをカバーアートで表示し、ダウンロード中でも再生でき、<br>
  映画、ドラマ、ゲームをひとつのライブラリにまとめるトレントクライアント。
</p>

<p align="center">
  <a href="https://github.com/BATorrent-app/BATorrent/releases/latest"><img alt="Release" src="https://img.shields.io/github/v/release/BATorrent-app/BATorrent?style=flat-square&color=dc2626"></a>
  <a href="https://github.com/BATorrent-app/BATorrent/releases"><img alt="Downloads" src="https://img.shields.io/github/downloads/BATorrent-app/BATorrent/total?style=flat-square&color=dc2626"></a>
  <a href="LICENSE"><img alt="License" src="https://img.shields.io/github/license/BATorrent-app/BATorrent?style=flat-square&color=dc2626"></a>
  <img alt="Platforms" src="https://img.shields.io/badge/Windows%20·%20macOS%20·%20Linux-dc2626?style=flat-square">
  <a href="https://apps.microsoft.com/detail/9n4l3tq24rc6"><img alt="Microsoft Store" src="https://img.shields.io/badge/Microsoft%20Store-get-dc2626?style=flat-square&logo=microsoft"></a>
</p>

<p align="center">
  <a href="#インストール"><b>ダウンロード</b></a> &nbsp;·&nbsp;
  <a href="https://batorrent.com/assets/trailer.mp4"><b>トレーラーを見る</b></a> &nbsp;·&nbsp;
  <a href="https://batorrent.com"><b>Web サイト</b></a>
</p>

<p align="center">
  <img src="docs/images/grid.jpg" alt="グリッド表示の「ダウンロード」タブ。ドラマを選択すると右側にエピソードが並ぶ" width="900">
</p>

BATorrent は [libtorrent](https://www.libtorrent.org/) をベースにしたデスクトップ向けトレントクライアントです。qBittorrent や Deluge と同じエンジンを使っています。各トレントの名前を読み取って対応するポスターを探し (映画やドラマは TMDB、ゲームは IGDB)、ダウンロードをファイル名ではなくカバーとして並べます。同じドラマのエピソードはひとつにまとめられ、動画はダウンロードが終わる前からアプリ内で再生でき、ゲームは同じウィンドウからインストールして起動できます。

無料のオープンソースで、広告、アカウント、「Pro」プラン、アクセス解析はありません。トレント自体の通信以外でアプリがネットに接続するのは3つだけです。ライブラリにあるものや「見つける」で開いたものについて TMDB、IGDB、Steam からカバーと詳細を取得すること、GitHub への更新確認 (オフにできます、[`updater.cpp`](src/services/integrations/updater.cpp))、そして公式ビルドでは、アプリがクラッシュしたときのクラッシュレポートです（これもオフにできます）。

## 3つのタブ

画面上部に **ダウンロード**、**見つける**、**HUB** の3つのタブがあります。

### ダウンロード

トレントをカバーのグリッドか詰めたリストで表示し、選択したものの詳細を横に出します。ドラマはエピソードを中に収めた1行にまとまるので、12話あるドラマも12行ではなく1行で済みます。

<p align="center">
  <img src="docs/images/list.jpg" alt="リスト表示の「ダウンロード」タブ。ドラマを展開してエピソードを表示している" width="900">
</p>

### 見つける と HUB

**見つける** は新しいものを探す場所です。トレンドの映画、ドラマ、ゲームが並ぶトップページと、複数のソースをまとめて検索していちばん合う結果を先頭に出す検索があります。タイトルごとに専用のページがあります。映画のページには、共有している人の数で並べたリリースの一覧、おすすめの1件、その下に同じシリーズの他の作品が表示されます。ドラマのページには、シーズンごとのエピソードと、手元にあるもの、視聴済みのものが表示されます。

<table>
  <tr>
    <td width="50%"><img src="docs/images/find.jpg" alt="見つける: 映画のページ。リリースの一覧と同じシリーズの他の作品"></td>
    <td width="50%"><img src="docs/images/series.jpg" alt="見つける: ドラマのページ。エピソード、視聴済みのものとライブラリにあるもの"></td>
  </tr>
  <tr>
    <td align="center"><sub>「見つける」で開いた映画</sub></td>
    <td align="center"><sub>「見つける」で開いたドラマ</sub></td>
  </tr>
</table>

**HUB** はすでに持っているもののライブラリです。「視聴を続ける」「プレイを続ける」があり、すべてがゲーム、ドラマ、映画に分けて並びます。

<p align="center">
  <img src="docs/images/hub.jpg" alt="HUB: 視聴を続ける、プレイを続ける、ライブラリ" width="900">
</p>

## プレーヤー

内蔵プレーヤーは FFmpeg を使っているので、MKV、AVI、WebM をそのまま再生できます。ファイルの先頭から先にダウンロードするので、残りを受信しながら視聴できます。止めた位置を覚えていて、オープニングをスキップでき、最後には次のエピソードや、シリーズの次の映画を提案します。字幕は自動で探すか (SubDL 経由)、ファイルの隣にある `.srt` を読み込みます。デコードはサンドボックス化された別プロセスで行うので、壊れたファイルを開いてもアプリごと落ちることはありません。

<p align="center">
  <img src="docs/images/player.jpg" alt="内蔵プレーヤー。次のエピソードが待機している" width="900">
</p>

[Real-Debrid](https://real-debrid.com) か [TorBox](https://torbox.app) のアカウントがあり、トレントがすでに向こう側にキャッシュされている場合は、そちらからストリーミングします。この場合、手元のマシンでダウンロードやシードは発生しません。

## そのほかの機能

- **ゲーム。** IGDB のカバー、HUB からのインストールと起動、プレイ時間と実績。ゲームのダウンロードが、セットアップファイルの詰まったフォルダではなくランチャーのような見た目になります。
- **VPN。** WireGuard の `.conf` (Proton、Mullvad、IVPN など) を読み込めば接続できます。経由するのは BATorrent だけで、パソコンのほかの通信は普段どおりの接続のままです。システムには何もインストールしません。切れたら自動で再接続し、戻るまではキルスイッチがトレントを止めます。対応しているサーバーでは、ピアから接続できるようにポートを開きます。
- **プライバシー。** ピアから見える IP を表示するリークテスト、ネットワークインターフェイスへのバインド、プライベートトラッカー用モード、Tor プリセット、既知のリーチャーのクライアントのブロック。
- **追加の方法。** magnet、リンク、info hash はどこでも Ctrl+V で貼り付けられます。正規表現フィルタ付きの RSS、監視フォルダ、ドラッグ＆ドロップ、通常の HTTP/HTTPS ダウンロードにも対応しています。
- **リモート操作。** スマホで QR コードを読み取って開く WebUI。コードはあなたのマシン上で生成されます。
- **整理。** カテゴリとタグ、アーカイブの自動展開、レシオ制限と時間制限、時間帯と曜日ごとの帯域スケジュール、1クリックで切り替えられる代替速度 (カメのアイコン)。
- **見た目。** 7つのテーマがあり、それぞれ専用のフォントとアートが付いています。自作のテーマも使えます。背景は無地、テーマのアート、自分の画像から選べます。Ctrl/⌘+K でコマンドパレット。9言語に対応し、初回起動時には短いセットアップがあります。

<p align="center">
  <img src="docs/images/themes.jpg" alt="7つのテーマで表示した「ダウンロード」タブ: Default、Cinema、Light、Midnight、Sakura、Dark Star、Matrix" width="900">
</p>

<details>
<summary><b>全機能一覧</b></summary>

ファイルごとの優先度、順次ダウンロード、トラッカーの追加、コンテンツ配置の制御、除外ファイルの正規表現、一時フォルダの個別指定、シード期間、ファイルが見つからないときの自動一時停止、Plex/Jellyfin/Emby のライブラリ更新、Telegram と Discord への通知、qBittorrent からのインポート、`.torrent` の作成、IP ブロックリスト、プロトコル暗号化、Gitee の更新ミラー、安定版とベータ版の更新チャンネル、ダウンロード完了時のシャットダウン、Windows Defender の除外設定ヘルパー、バックアップと復元、最近削除した項目の履歴、強制開始、診断付きのログビューア、Alt で出るメニューバー、ライブラリでの同じシリーズの映画のグループ化、ほとんどの操作に使えるキーボードショートカット。

</details>

## エンジン

BATorrent には小さなパッチを当てた libtorrent のフォークが同梱されています。パッチは [`third_party/patches/`](third_party/patches) 以下にバージョン管理されたパッチとして置いてあります。

- **立ち上がりが速い。** 帯域が広く遅延の大きい回線では、標準の libtorrent はリクエストパイプラインを1段ずつしか伸ばしません。フォークでは等比級数的に伸ばすので、ずっと少ない往復回数で回線を埋められます。プロジェクトの A/B 計測では高速回線で約 27% 速く、標準版より遅くなることはありませんでした。
- **同じ国のピア。** オフラインの GeoIP データベース (db-ip Lite) で各ピアに国を割り当て、選べる場合は自分と同じ国のピアを優先します。たいていはそのほうが遅延が小さくなります。

どちらもコンパイル時のスイッチで、標準ビルドでは無効です。

## インストール

| プラットフォーム | ダウンロード | 動作環境 |
|---|---|---|
| **Windows** | [Microsoft Store](https://apps.microsoft.com/detail/9n4l3tq24rc6) · [インストーラー](https://batorrent.com/win) · [ポータブル版](https://batorrent.com/portable) | Windows 10 以降 |
| **macOS** | `brew install --cask Mateuscruz19/batorrent/batorrent` または [`.dmg`](https://batorrent.com/mac) | macOS 12+、Apple Silicon |
| **Linux** | [AppImage](https://batorrent.com/linux) | glibc 2.35+ |

起動したら、`.torrent` か magnet リンクをウィンドウにドロップしてください。

<sub><b>macOS:</b> アプリはまだ公証を受けていません (Apple の開発者プログラムは有料のサブスクリプションのため)。Homebrew なら隔離フラグを外してくれるので、いちばん簡単です。<code>.dmg</code> を使う場合は、初回だけアプリを右クリックして<b>開く</b>を選んでください。</sub>

<details>
<summary><b>ソースからビルド</b></summary>

**必要なもの:** C++17、CMake 3.16+、Qt 6 (`Widgets`, `Network`, `Svg`, `Multimedia`, `Quick`, `QuickWidgets`, `QuickControls2`)、libtorrent-rasterbar 2.0+、Boost、zlib、必要に応じて Qt6Keychain。

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

- CI のビルドごとに Catch2 のテストスイート (ユニット、セキュリティ、メモリ) と QML のテストを実行しています。
- ビルドは AddressSanitizer と UndefinedBehaviorSanitizer の下で問題なく通ります。
- シークレットは OS のキーチェーンに保存し、WebUI はパスワードを設定するまでネットワークに公開されません。

</details>

## なぜ作ったのか

私はブラジルに住む個人開発者です。プライバシーをきちんと扱い、Windows、macOS、Linux でネイティブに動き、2009年に作られたような見た目ではないトレントクライアントが欲しかったのですが、気に入るものが見つからなかったので自分で書きました。MIT ライセンスなので、もしこのアプリに広告やトラッキングが入るようなことがあっても、誰でもフォークしてそれらを外したものを配布できます。

## コントリビュート

Issue とプルリクエストを歓迎します。小さな修正でない場合は、進め方をすり合わせるために先に Issue を立ててください。バグ報告には、プラットフォーム、バージョン (`ヘルプ → BATorrent について` で確認できます)、再現手順を書いてもらえるといちばん助かります。翻訳は特に歓迎しています。AI ツールを使う場合は、先に [私の AI の使い方](USING_AI.md) を読んでください。

## ライセンスと商標

**コード** のライセンスは [MIT](LICENSE) です。© 2024-2026 Mateus Cruz。自由にフォークして、独自のビルドを配布できます。

**「BATorrent」という名前とロゴ** はコードのライセンスの対象外です。フォークを配布する場合は、どれが公式ビルドか見分けられるよう、別の名前を付けてください。詳しくは [TRADEMARK.md](TRADEMARK.md) にあります。

<p align="center"><sub>ブラジルで作っています。</sub></p>
