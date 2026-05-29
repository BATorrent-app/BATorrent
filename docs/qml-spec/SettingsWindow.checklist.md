# SettingsWindow.checklist.md
// Source: BATorrent Settings.html + batorrent-settings.css (+ settings-data.js)
// Janela top-level 884×624 (NÃO usa BatDialog; tem layout próprio sidebar+conteúdo+footer).

## Estrutura (ordem)
1. **.titlebar** height 36, bg Theme.elev, border-bottom 1px Theme.hairSoft → `.tl` 3 traffic + `.ttl` "Preferências" (12.5/600/Theme.t2)
2. **.sbody** (Row, fill): 
   - **.sside** width **226**, bg Theme.panel, border-right 1px Theme.hair, padding `12 8`
     - **.ssearch** height 32, padding `0 10`, bg Theme.field, border 1px Theme.hair, radius 8; **:focus-within** border (light: rgba(0,0,0,0.25)); ícone search 14px Theme.t4 + input "Buscar configuração — DHT, porta, ratio…" 12px
     - **.snav** Column gap 1 → **.nrow** height 34, padding `0 10`, gap 12, radius 7, color Theme.t2, ícone 16px Theme.t3 + label 12.5; **:hover** bg Theme.hover + Theme.t1; **.on** bg Theme.sel + Theme.t1, ícone Theme.accent, barra esquerda 2px Theme.accent (`::before` left -8, top/bottom 7)
     - 9 seções (settings-data.js): **Geral**(gear), **Velocidade**, **Rede**, **VPN / Kill Switch**, **Proxy & Filtro de IP**, **WebUI & Pareamento**, **Notificações**, **Addons & Mídia**, **Avançado**. (Geral = on)
   - **.scontent** fill, overflow-y auto, padding 24
     - **.shead**: `.eyebrow` "PREFERÊNCIAS" (9/800/letter 1.8/Theme.t4 — NB: settings usa weight 700/800; ler css) ; `h1` "Geral" 19/650; `p` subtítulo 12 Theme.t3 (margin-top 8)
     - grupos: **.glabel** (10/700/0.8/Theme.t4 UPPER, margin `24 0 8 2`) seguido de **.card** (bg Theme.panel, border 1px Theme.hair, radius 11, padding `0 16`)
     - **.srow** padding `13 0`, gap 16, border-bottom 1px Theme.hairSoft (último sem); `.smeta` (label `.lbl` 12.5 Theme.t1 + `.note` 10.5 Theme.t4) + `.sctrl`
       - `.srow.stack` (path/iface/warning) → coluna, ctrl width 100%
3. **.sfoot** height 56, bg Theme.elev, border-top 1px Theme.hair, padding `0 24`, space-between → `.hint` "As alterações são aplicadas ao confirmar" 10.5 Theme.t4 ; `.acts`: btn "Cancelar" (ghost) + btn "OK" .primary (bg Theme.accent, #fff, 600), ambos height 33 padding `0 18`

## Controles (valores em batorrent-settings.css)
- **.toggle** 38×21 r999 (off: bg Theme.field+knob #8c8884 / claros: track #d4d6dc+knob #fff; on: bg Theme.accent + knob left19 #fff)
- **.input** height 30, padding `0 10`, bg Theme.field, border 1px Theme.hair, radius 7; **:focus** Theme.accent; `.num` width 92 dir mono; `.w-sm`120 `.w-md`210 `.grow` fill; `.suffix` 10.5 mono Theme.t4
- **.select** height 30, appearance none, padding `0 30 0 10`, bg Theme.field, border 1px Theme.hair, radius 7, caret triângulo Theme.t3 (right 10)
- **.seg** (segmented) inline, padding 2, bg Theme.field, border 1px Theme.hair, radius 8; button height 25, padding `0 11`, 11/500; **.sel** bg Theme.sel + Theme.accentText ; `.dot` 11×11 r50 border (claro rgba(0,0,0,0.18))
- **.btn** height 30, padding `0 13`, bg Theme.field, border 1px Theme.hair, 11.5/500/Theme.t2; **:hover** border + Theme.t1
- **.warn** padding `11 13`, bg rgba(229,51,43,0.08), border 1px rgba(229,51,43,0.3), radius 9; ícone 15 Theme.accentText; p 11 (cor escura-vermelha: dark #e6b3b0 / claro #962018)
- **.timerange** Row gap 8 (inputs width 74 center mono + "até"); **.days** 7× `.day` 27×27 r7 (off border/field; **.on** bg Theme.sel + border accent + Theme.accentText)
- **.badge** 9.5 mono Theme.t3 bg Theme.field border Theme.hair padding `2 7` r999

## Estados: nrow default/hover/on · toggle off/on · input default/focus · seg default/sel · btn default/hover · day off/on
## NOTA: troca de TEMA mora aqui (seção Geral → "Tema" como segmented/opções). É o ÚNICO lugar com Theme switching.
