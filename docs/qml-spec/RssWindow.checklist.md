# RssWindow.checklist.md
// Source: BATorrent RSS.html (bat-dialog.css + <style> inline — usar inline como fonte exata).
// Janela .dlg 760×560. Body padding 0 (layout próprio).

## Estrutura
1. **.tb** "RSS"
2. **.rhead** padding `16 24`, border-bottom hairSoft: `h1` "Feeds RSS" 16/650 + chip "3 feeds" + spacer + btn.sm "Editar regras" + btn.primary.sm "＋ Adicionar feed"
3. **.rcols** Row fill:
   - **.feeds** width 232, border-right 1px Theme.hair, padding `12 8` → `.feed` height 38, radius 7 (dot saúde verde/cinza + nome + count mono): "Nyaa · Anime 1080p"(12, on=Theme.sel), "EZTV · Séries"(31), "Linux ISOs"(4, dot Theme.t4)
   - **.items** fill: 
     - **.rule** banner (bg Theme.panel, border-bottom hairSoft, padding `10 24`): ícone 14 Theme.accentText + "Auto-baixar itens que contenham **1080p**" + spacer + toggle **.on**
     - **.ilist** → `.item` padding `11 24`, border-bottom hairSoft: `.nm` fill 12.5 + `.badge-auto` (verde, "Auto", quando aplicável) + `.meta` mono Theme.t4 + `.dl` 28×28 download (hover border accent)
     - 5 itens (Frieren 28/27/27-720p/26/25, badge Auto exceto o 720p)

## Estados: feed on/hover · toggle on · .dl hover
## Dims exatas: ler <style> inline de BATorrent RSS.html.
