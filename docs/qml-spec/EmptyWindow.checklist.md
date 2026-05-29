# EmptyWindow.checklist.md
// Source: BATorrent Empty.html + batorrent-home.css + <style> inline.
// Janela principal SEM torrents — reusa toolbar/subbar do Main, conteúdo central vazio.

## Estrutura
1. **.titlebar** 34 (igual Main)
2. **.toolbar** 66 (igual Main) — mas Pausar/Retomar/Parar/Remover em **.disabled** (opacity 0.35); spd-mod mostra "0 KB/s" (Theme.t4)
3. **.subbar** 54 — pills com counts 0; seg em "Lista" on (ou Grade); donate presente
4. **.content** fill → **.empty** centralizado (Column):
   - `.mk` 86×86 radius 22, bg Theme.panel, border 1px Theme.hair → logo.svg 44px opacity 0.5
   - `h2` "Nenhum torrent ainda" 19/650 Theme.t1 (margin-top 22)
   - `p` 12.5 Theme.t3 max 360 center "Adicione um .torrent, cole um link magnet ou busque — seus downloads aparecem aqui."
   - `.cta` Row gap 10 (margin-top 24): **.ebtn.primary** "Abrir .torrent" (bg Theme.accent #fff 600, height 36) + **.ebtn** "Colar magnet" (bg Theme.panel border Theme.hair)
   - `.hintdrop` Row (margin-top 26): ícone 14 + "ou arraste arquivos pra esta janela" 11 Theme.t4
5. **.status** 30 → "0 torrents" + speeds 0 KB/s

## Estados: ebtn hover · ebtn.primary hover · tbtn.disabled (sem hover)
