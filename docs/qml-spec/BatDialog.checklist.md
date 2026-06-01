# BatDialog.checklist.md — chrome compartilhado dos diálogos
// Source: bat-dialog.css  (tokens já no Theme.qml)

Janela modal: backdrop escuro + card central. Reusado por Magnet, Add Torrent, Create,
Remove, Add Addon, Welcome, Release Notes, About (e o "stage" de Search/RSS é variação).

## Estrutura (ordem)
1. **backdrop** — `position:fixed; inset:0; background:rgba(0,0,0,0.5); backdrop-filter:blur(3px)`
   → QML: Rectangle full-parent, color `Qt.rgba(0,0,0,0.5)` (light: ver telas claras usam rgba(20,20,28,0.32)).
2. **.dlg** (card) — `background:Theme.bg; border:1px solid rgba(255,255,255,0.09); radius:13;
   box-shadow:0 50px 130px -30px rgba(0,0,0,0.85), 0 12px 36px -12px rgba(0,0,0,0.7)`. Largura/altura por tela.
3. **.tb** (titlebar) — `height:36; padding:0 16; background:Theme.elev; border-bottom:1px Theme.hairSoft; position:relative`
   - **.tl**: Row spacing 8 → 3 círculos 12×12 radius 50% (#ff5f57, #febc2e, #28c840)
   - **.ttl**: centralizado absoluto, `font 12.5px / weight 600 / Theme.t2`, uppercase=não
   - **.x-close**: `margin-left:auto; 22×22; radius:6; color Theme.t4`; **:hover** bg Theme.hover + color Theme.t1; ícone × 13px (ver MISSING_ASSETS)
4. **.body** — `flex:1; overflow-y:auto; padding:24` (Theme.sp5)
5. **.foot** — `height:56; background:Theme.elev; border-top:1px Theme.hair; padding:0 24; space-between`
   - **.hint**: `font 10.5px / Theme.t4`
   - **.acts**: Row spacing 8 (Theme.sp2)

## Tipo
- **.eyebrow** — `9px / weight 700 / letter-spacing 1.8 / Theme.t4 / UPPERCASE`; `.eyebrow.red` → Theme.accent
- **h1.title** — `19px / weight 650 / letter-spacing -0.3 / Theme.t1`
- **.sub** — `12px / Theme.t3 / line-height 1.5`
- **.glabel** — `10px / weight 700 / letter-spacing 0.8 / Theme.t4 / UPPERCASE`
- **.lbl** — `11px / weight 600 / Theme.t3 / margin-bottom 7`

## Botões (.btn)
- base — `height:32; padding:0 14; radius:7; font 12px/weight 500; bg Theme.field; border 1px Theme.hair; color Theme.t2`
  - **:hover** → border `rgba(255,255,255,0.2)` (use Theme: ver token; light usa rgba(0,0,0,0.22)) + color Theme.t1
- **.btn.primary** — bg Theme.accent; border transparent; color #fff; weight 600; **:hover** bg Theme.accentDark
- **.btn.ghost** — bg transparent
- **.btn.sm** — height 28; padding 0 11; font 11.5
- ícone interno `.btn svg` 14px

## Campos
- **.field** — `height:34; padding:0 11; gap:8; bg Theme.field; border 1px Theme.hair; radius:8`; **:focus-within** border Theme.accent; ícone 14px Theme.t4; input `12.5px Theme.t1`, placeholder Theme.t4; `.mono` → fontMono 11.5
- **textarea.ta** — `min-height:88; padding:11; bg Theme.field; border 1px Theme.hair; radius:8; fontMono 11.5; line-height 1.5`; **:focus** border Theme.accent
- **.path** — Row gap 8; `.field` flex:1 + botão "Procurar…"

## Toggle (.toggle)
- `38×21; radius:999; bg Theme.field; border 1px Theme.hair`; knob `15×15; radius:50%; top 2; left 2; bg #8c8884`
- **.on** → bg Theme.accent; border transparent; knob `left:19; bg #fff`
- (claros usam track #d4d6dc + knob #fff sempre — ver telas Light/Sakura)

## Checkbox (.chk)
- `17×17; radius:5; border 1px Theme.hair; bg Theme.field`; check svg 11px #fff opacity 0
- **.on** → bg Theme.accent; border transparent; check opacity 1
- **.partial** → bg Theme.field; border Theme.accent; traço central 8×2 Theme.accent

## Card / row / chip
- **.card** — bg Theme.panel; border 1px Theme.hair; radius:11
- **.row** — `padding:11 14; gap:12; border-bottom 1px Theme.hairSoft`; último sem borda
- **.chip** — `font 10 fontMono / Theme.t3; bg Theme.field; border 1px Theme.hair; padding 2 8; radius 999`; `.chip.red` → color Theme.accentText; bg Theme.sel; border `rgba(229,51,43,0.3)`

## Estados cobertos
- .x-close: default ✓ / hover ✓
- .btn: default ✓ / hover ✓ ; .primary default ✓ / hover ✓
- .field/textarea: default ✓ / focus ✓
- .toggle: off ✓ / on ✓ ; .chk: off ✓ / on ✓ / partial ✓

## Anti-bug
- `clip:true` no card NÃO recorta filhos nos cantos arredondados → use MultiEffect maskSource se um Image precisar respeitar o radius do card.
- fontes 12.5/11.5/10.5 → `font.pointSize` (nunca pixelSize decimal).
