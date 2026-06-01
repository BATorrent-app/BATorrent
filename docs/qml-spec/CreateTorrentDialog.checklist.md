# CreateTorrentDialog.checklist.md
// Source: BATorrent Create Torrent.html + bat-dialog.css. BatDialog chrome. .dlg 560×560.
// Footer hint "Salva um arquivo .torrent". OK="Criar".

## Body (gap 16)
1. col: `.eyebrow.red` "NOVO" + `h1.title` "Criar torrent"
2. "Origem (arquivo ou pasta)" + `.path` (field mono "/Users/voce/Projetos/ubuntu-remix" + "Procurar…")
3. "Trackers (um por linha)" + **textarea.ta** mono com 3 linhas (udp://tracker.opentrackr.org:1337/announce, openbittorrent:6969, https://tracker.example.org/announce)
4. **.grid2** (2 colunas, gap 16): [ "Tamanho de peça" + **.select** (Automático/256KB/512KB/1MB(sel)/2MB/4MB/8MB) ] [ "Comentário (opcional)" + field placeholder "ex: build noturna" ]
5. **.card** 2 linhas `.row` (togrow): "Torrent privado" + sub "Desativa DHT, PEX e LSD — para trackers privados" + toggle **off**; divisor; "Iniciar seeding após criar" + sub "Adiciona o torrent e começa a semear" + toggle **on**

## Estados: select hover/focus · toggle off/on · textarea focus
