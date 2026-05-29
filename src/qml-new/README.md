# BATorrent — QML

Recriação fiel do BATorrent em QML, com os **5 temas** num único `Theme.qml`
(Dark, Light, Midnight, Sakura, Dark Star) e as **artes anime** por tema.
Trocar de tema = mudar `Theme.name` — todas as telas reagem.

## Telas (cada uma roda com `qml <Arquivo>.qml`)
- `Main.qml`               — tela principal (toolbar, grade/lista, gráfico, detalhe, status, anime)
- `SettingsWindow.qml`     — Preferências (sidebar + conteúdo)
- `AddTorrentDialog.qml`   — adicionar torrent (seleção de arquivos + caminho)
- `MagnetDialog.qml`       — colar magnet
- `CreateTorrentDialog.qml`— criar .torrent
- `RemoveDialog.qml`       — confirmar remoção
- `SearchWindow.qml`       — buscar em provedores
- `RssWindow.qml`          — feeds + regras
- `AddAddonDialog.qml`     — instalar addon
- `DetailWindow.qml`       — abas Peers/Arquivos/Trackers/Pedaços
- `EmptyWindow.qml`        — estado vazio
- `WelcomeDialog.qml`      — boas-vindas
- `ReleaseNotesDialog.qml` — notas de versão
- `AboutDialog.qml`        — sobre

## Infra
- `Theme.qml`   — singleton com os 5 temas (cores, acentos, arte anime). `Theme.name` / `Theme.anime`.
- `BatDialog.qml`— moldura reutilizável dos diálogos (titlebar + corpo + rodapé).
- `qmldir`      — registra o singleton `Theme`.
- `images/`     — logo, pôsteres e artes anime (olhos por tema + aranha do Dark Star).

## Rodar
    qml Main.qml            # precisa do qmldir na mesma pasta (singleton Theme)

Para alternar tema/anime no protótipo: botões "Tema" e "Anime" na subbar do `Main.qml`.
No app real, ligue `Theme.name` ao seu ThemeManager.

## Notas
- A arte anime usa fundo preto que funde nos temas escuros; no Sakura (claro) já vem recortada.
  Para o fade suave das bordas (como no HTML), aplique um `MultiEffect`/máscara de gradiente.
- Ícones da toolbar/diálogos estão como glifos/emoji placeholder — troque pelos SVGs de `src/icons` do app.
- Os HTML originais (pasta raiz) servem de referência visual 1:1 para ajustes finos.
