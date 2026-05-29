# Main.checklist.md — tela principal
// Source: BATorrent Home.html + batorrent-home.css (+ batorrent-home.js model)

Janela 1360×884, `.win` bg Theme.bg, radius 12, border 1px rgba(255,255,255,0.07).
Layout vertical (ColumnLayout spacing 0). `.win.v-grid` mostra grade / `.win.v-list` mostra lista.

## 1. TITLEBAR — `.titlebar` (line 41-46)
- height **34**, padding `0 16` (Theme.sp4), bg **Theme.elev**, border-bottom 1px **Theme.hairSoft**
- `.tl`: Row gap 8 (sp2) → 3 círculos 12×12 r50% (#ff5f57 / #febc2e / #28c840)
- `.ttl`: absoluto centralizado, text "BATorrent", `12.5px / weight 600 / Theme.t2`

## 2. TOOLBAR — `.toolbar` (line 49) height **66**, padding `0 16`, gap 8, bg Theme.elev, border-bottom 1px Theme.hair
- `.brand` (padding-left 4): `images/logo.svg` 30×30
- `.brand-div`: 1×26, bg Theme.hair, margin `0 4`
- **grupos `.tgrp`** (Row gap 2). Entre grupos: `.tgrp + .tgrp` → margin-left 8, padding-left 8, **border-left 1px Theme.hair** (divisor vertical)
  - G1: **Abrir** (open.svg), **Magnet** (magnet.svg)
  - G2: **Pausar** (pause), **Retomar** (play), **Parar** (stop)
  - G3: **Remover** (trash)  ← no HTML Home está em grupo próprio
  - G4: **Buscar** (search), **RSS** (rss)
  - G5: **Config.** (settings)
- **`.tbtn`** (line 50-53): width **52**, padding `6 0` (top/bottom 6), Column gap 3, radius 8, color **Theme.t3**, cursor pointer
  - ícone svg **18×18** (tingir Theme.t3 → no hover Theme.t1)
  - label `.l` **10.5px / weight 500**
  - **:hover** → bg Theme.hover + color Theme.t1  (MouseArea.containsMouse)
  - **.disabled** → opacity 0.35, sem hover
- `.tb-spacer`: Item Layout.fillWidth
- **`.spd-mod`** (line 60): Row, border 1px Theme.hair, radius 9, overflow hidden
  - 2× `.c` (padding `8 14`, Column gap 3); divisor `.c + .c` border-left 1px Theme.hair
  - `.k` eyebrow **9px / weight 700 / letter-spacing 1 / Theme.t4** ("DOWNLOAD" / "UPLOAD")
  - `.v` **13px**, Row gap 5: seta svg 12px + valor. Download → `.v-dn` (download.svg + valor) cor **Theme.accentText** + **weight 700**; Upload → `.v-up` cor **Theme.up**
  - valores: "↓ 8.5 MB/s" / "↑ 1.7 MB/s" (mono, ver fonte)

## 3. SUBBAR — `.subbar` (line 73) height **54**, padding `0 16`, gap 12 (sp3), border-bottom 1px Theme.hair
- **`.search`** width **240**, height 34, padding `0 11`, gap 8, bg **Theme.panel**, border 1px Theme.hair, radius 8 → search.svg 14px Theme.t4 + input placeholder "Buscar torrents" (12.5 Theme.t1, placeholder Theme.t4)
- **`.seg`** toggle: padding 2, bg Theme.panel, border 1px Theme.hair, radius 8 → 2 botões `.seg button` (height 28, padding `0 11`, gap 6, radius 6, **11.5px / weight 500 / Theme.t3**, ícone 14px): **Grade** (grid.svg) + **Lista** (list.svg)
  - **.on** → bg **rgba(255,255,255,0.08)** (Theme: hover-ish; use token) + color Theme.t1. Bind a `gridView`.
- **`.pills`** Row gap 4 → **6 pills**: **Todos**(3), **Ativos**(2), **Baixando**(2), **Semeando**(0), **Pausado**(1), **Concluído**(0)
  - `.pill` height 30, padding `0 13`, gap 7, radius 8, **12px / weight 500 / Theme.t3**; `.ct` 11px Theme.t4
  - **:hover** → bg Theme.hover + color Theme.t2
  - **.on** → bg **Theme.accentTint** (rgba accent 0.12) + color **Theme.accentText**; `.ct` Theme.accentText. (Todos = on)
- `.tb-spacer` fillWidth
- **`.cat`** height 34, padding `0 12`, gap 8, border 1px Theme.hair, radius 8, **12px Theme.t2** → "Todas as categorias" + chevron 13px Theme.t4 (chevron.svg — ver MISSING)
- **`.donate`** height 34, padding `0 13`, gap 7, border **1px rgba(229,51,43,0.4)**, radius 8, **12px / weight 600 / Theme.accentText** → coração 14px (heart.svg) + "Doar"; **:hover** bg rgba(accent,0.1)

## 4. CONTENT — `.content` fillHeight, overflow-y auto. Mostra GRID ou LIST (bind gridView).
### GRID `.grid` padding `24 16`, colunas `repeat(auto-fill, minmax(178, 1fr))`, gap `24 16`, align-content start
- `.tile` (cursor pointer) → `.poster` width 178 aspect-ratio **3/4**, radius 10, overflow hidden, bg #161618, border 1px Theme.hair
  - **.tile:hover .poster** → border rgba(255,255,255,0.2); **.tile.sel .poster** → border Theme.accent + inset 0 0 0 1px Theme.accent
  - `.poster.has-img .pimg` (Image cover, fill) ; `.pscrim` gradiente inferior 42% transparent→rgba(0,0,0,0.5)
  - `.pbar` (z2) bottom, height **3**, bg rgba(0,0,0,0.5); fill `i` width = pct% , cor por estado: dl→Theme.accent, se/cp→Theme.amber, pa→Theme.pausedFill
  - (sem pôster: `.wm` inicial gigante rgba(255,255,255,0.05) + `.pcat` + `.ptitle` — fallback)
  - `.tmeta` padding-top 12: `.st` (Row gap 6: dot 6×6 + label 11.5px cor estado) + `.sz` 11.5px Theme.t4
  - estados/cores: st-dl Theme.accentText, st-se/cp Theme.up, st-pa Theme.t3; dot bg-dl accent, bg-se amber, bg-pa t4
  - Itens (model): Hollow Knight (dl 42% 5.20 GB), 007 First Light (pa 12% 7.49 GB), Forza Horizon 6 (dl 67% 92.4 GB, **selecionado**)
### LIST `.lhead` (header) height 36, padding `0 16`, gap 16, **10.5px/weight 600/letter-spacing 0.6/Theme.t4 UPPERCASE**
- colunas: NOME(fill) · TAMANHO(78,dir) · PROGRESSO(130) · DOWN(78,dir) · UP(78,dir) · ESTADO(110) · CATEGORIA(90) · PEERS(56,dir)
- `.row` height **48**, padding `0 16`, gap 16, border-bottom 1px Theme.hairSoft; **:hover** bg Theme.hover; **.sel** bg Theme.sel + inset 2px Theme.accent (barra esquerda)
  - `.name`: dot 7×7 (bg estado) + nome 13px Theme.t1 (elide)
  - `.size` 78 dir 12px Theme.t2
  - **`.prog`** 130 → **`.pbar2`**: height **18**, radius 4, bg **#000**, border 1px Theme.hair, overflow hidden; fill width pct% cor estado; `.pct` centralizado **10.5px mono weight 600 #fff** (texto "67%")
  - `.dn`/`.up` 78 dir 12px (dl→v-dn accentText / up→v-up Theme.up / "—"→Theme.t4)
  - `.state` 110, 12px weight 500, cor por estado
  - `.catg` 90, 12px Theme.t3 ("Jogos")
  - `.pr` 56 dir 12px Theme.t2 ("0"→Theme.t4)

### ARTE ANIME (quando `Theme.hasAnime`)
- Image `Theme.animeSource`, no canto: topo-direito (eyes) ou inferior-direito (`Theme.animeBottom`, spider ~560px).
- Grade: opacity 0.9. Lista: opacity 0.6, ATRÁS das linhas (z 0; linhas z 1); textos ESTADO/CATEGORIA/PEERS ganham peso 600 + sombra p/ legibilidade.
- Fade das bordas (esquerda+baixo p/ eyes; topo+esquerda p/ spider) via MultiEffect maskSource (gradiente). Light: sem arte.

## 5. GRAPH — `.graph` height **64**, border-top 1px Theme.hair, bg Theme.bg
- `.scale` top-left 6, "1 KB/s" 10px Theme.t4
- `.legend` top-right 6, Row gap 16: 2× (dot 6×6 + valor 11px): accent "8.5 MB/s" / amber "1.7 MB/s"
- 2 curvas (Shape/Canvas): download stroke Theme.accent + fill gradiente accent→transparent; upload stroke Theme.amber + fill amber→transparent (baixa opacidade)

## 6. DETAIL — `.detail` height **270**, border-top 1px Theme.hair, bg Theme.panel
- `.dtabs` height 42, padding `0 24`, gap 24 → 5 tabs **12.5px Theme.t3**: Geral/Peers/Arquivos/Trackers/Pedaços; **.on** color Theme.t1 + underline 2px Theme.accent (bottom)
- `.dbody` padding 24, gap 32: 
  - `.dcover` 104×146 radius 8 (Image forza cover) border 1px Theme.hair
  - `.dmain` max-width 460: `h3` 17px/weight 650 ("Forza Horizon 6"); `.sub` 11.5px Theme.t3 ("2026 · Racing, Simulator, Sport · 8.6/10"); `.desc` 12px Theme.t2 line-height 1.55
  - `.dcols` margin-left auto, gap 32: 3 colunas `.dcol` (min 168) com `.ch` (10px/700/0.8/Theme.t4 UPPER) + linhas `.kv` (k 11.5 Theme.t3 / v 12 Theme.t1). Conteúdo: INFO (Nome/Tamanho/Hash/Estado), TRANSFERÊNCIA (Baixado/Download/Upload/ETA), PEERS (Seeds/Peers/Ratio)

## 7. STATUS — `.status` height **30**, padding `0 16`, gap 8, bg Theme.elev, border-top 1px Theme.hair, **11.5px Theme.t4**
- esquerda "3 torrents · 2 ativos"; spacer; dots accent/amber 6×6 + "8.5 MB/s"/"1.7 MB/s" (Theme.t3 mono); "· Total 31,04 GB ↓ · 2,09 GB ↑ · Ratio 0.07"

## Estados cobertos
- tbtn: default/hover/disabled · seg: on/off · pill: default/hover/on · cat/donate: default/hover
- tile: default/hover/sel · row: default/hover/sel · dtab: default/on
- gridView⇄listView toggle · Theme.name (5) · Theme.anime (on/off)

## Anti-bug
- Fontes decimais (12.5/11.5/10.5/9.5) → font.pointSize. Sem demo controls de tema na subbar (mora em Settings).
- Ícones = SVG (icons/*.svg) tingidos por MultiEffect; nunca emoji. Cores só via Theme.
- `.pbar2` track #000 é literal (assinatura) mesmo em temas claros.
