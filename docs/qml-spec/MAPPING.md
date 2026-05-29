# MAPPING.md — BATorrent QML port manifest

Cada `.qml` consolida a tela **base** (sem sufixo) + suas variantes de tema
num único arquivo, chaveado por `Theme.name` / `Theme.anime`. As variantes
`*Light/Midnight/Sakura/Darkstar.html` existem só para conferir o chaveamento —
NÃO geram .qml separados.

| .qml a gerar          | HTML base                     | HTML variantes (verificação)                                   | CSS (fonte da verdade)                  | Assets consumidos |
|-----------------------|-------------------------------|----------------------------------------------------------------|-----------------------------------------|-------------------|
| `Theme.qml` ✅ pronto | (síntese)                     | —                                                              | `batorrent-home*.css`                   | — |
| `Main.qml`            | `BATorrent Home.html`         | `Home Light/Midnight/Sakura/Darkstar(.Anime).html`, `Home Eyes.html` | `batorrent-home.css` + `batorrent-home.js` (model) | icons/*, images/forza.png,007.jpg,hollow.webp,logo.svg, eyes-*.png, spider.jpg |
| `SettingsWindow.qml`  | `BATorrent Settings.html`     | `Settings {Light,Midnight,Sakura,Darkstar}.html`              | `batorrent-settings.css` + `settings-data.js` (seções) | icons/settings.svg, logo.svg |
| `AddTorrentDialog.qml`| `BATorrent Add Torrent.html`  | `Add Torrent {Light,…}.html`                                   | `bat-dialog.css` (+ <style> inline no HTML) | images/forza.png |
| `MagnetDialog.qml`    | `BATorrent Magnet.html`       | `Magnet {Light,…}.html`                                        | `bat-dialog.css` (+ inline)             | — |
| `CreateTorrentDialog.qml` | `BATorrent Create Torrent.html` | `Create Torrent {Light,…}.html`                            | `bat-dialog.css` (+ inline)             | — |
| `RemoveDialog.qml`    | `BATorrent Remove Confirm.html` | `Remove Confirm {Light,…}.html`                              | `bat-dialog.css` (+ inline)             | — |
| `SearchWindow.qml`    | `BATorrent Search.html`       | `Search {Light,…}.html`                                        | `bat-dialog.css` + `<style>` inline NO HTML | — |
| `RssWindow.qml`       | `BATorrent RSS.html`          | `RSS {Light,…}.html`                                           | `bat-dialog.css` + `<style>` inline NO HTML | — |
| `AddAddonDialog.qml`  | `BATorrent Add Addon.html`    | `Add Addon {Light,…}.html`                                     | `bat-dialog.css` (+ inline)             | — |
| `DetailWindow.qml`    | `BATorrent Detail Tabs.html`  | `Detail Tabs {Light,…}.html`                                   | `bat-detail.css` + `<script>` inline (peers/files/trackers/pieces) | — |
| `EmptyWindow.qml`     | `BATorrent Empty.html`        | `Empty {Light,…}.html`                                         | `batorrent-home.css` + `<style>` inline | logo.svg |
| `WelcomeDialog.qml`   | `BATorrent Welcome.html`      | `Welcome {Light,…}.html`                                       | `bat-dialog.css` + `<style>` inline     | logo.svg |
| `ReleaseNotesDialog.qml` | `BATorrent Release Notes.html` | `Release Notes {Light,…}.html`                              | `bat-dialog.css` + `<style>` inline     | — |
| `AboutDialog.qml`     | `BATorrent About.html`        | `About {Light,…}.html`                                         | `bat-dialog.css` + `<style>` inline     | logo.svg |
| `BatDialog.qml` (chrome) | (compartilhado)            | —                                                              | `bat-dialog.css` (.dlg/.tb/.body/.foot/.btn/.field/.toggle/.chk/.card/.row/.chip) | icons/* |

## ⚠️ Correção importante vs. briefing
As telas **Welcome / Release Notes / About** no pacote **NÃO** usam `app-screens.css` —
foram reescritas usando `bat-dialog.css` + um `<style>` inline em cada HTML.
Use o `<style>` inline do próprio arquivo como fonte (o `app-screens.css` é de uma versão antiga).

## Modelos de dados (do JS, para reproduzir os mesmos itens)
- **Torrents** (`batorrent-home.js`): Hollow Knight (dl 42%), 007 First Light (pa 12%), Forza Horizon 6 (dl 67%, selecionado). Campos: file, title, cat, poster, state(dl|pa|se|cp), label, size, pct, down, up, peers.
- **Settings** (`settings-data.js`): 9 seções — Geral, Velocidade, Rede, VPN/Kill Switch, Proxy & Filtro de IP, WebUI & Pareamento, Notificações, Addons & Mídia, Avançado.

## Chrome compartilhado
Gere `BatDialog.qml` primeiro (titlebar 36 + body padding 24 + footer 56 + botões/campos/toggle/checkbox).
Todos os diálogos reusam. Veja `BatDialog.checklist.md`.
