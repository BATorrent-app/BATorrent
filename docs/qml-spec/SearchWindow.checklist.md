# SearchWindow.checklist.md
// Source: BATorrent Search.html (bat-dialog.css + <style> inline NO HTML — usar o inline como fonte exata).
// Janela .dlg 780×560. Sem footer primary (OK="Fechar"). Body padding 0 (layout próprio).

## Estrutura (ordem)
1. **.tb** titlebar (chrome) "Buscar torrents"
2. **.sbar** padding `16 24`(?), gap 12: `.field` fill (search.svg 14 + input value "forza horizon 6") + `.prov` (border Theme.hair, radius 8, "Todos os provedores" + chevron)
3. **.rhd** header height 34, border-bottom 1px Theme.hair, 10/700 Theme.t4 UPPER: NOME(fill) · PROVEDOR(120) · TAMANHO(78,dir) · SEEDS(56,dir) · LEECH(56,dir) · IDADE(60,dir) · (ação 36)
4. **.results** scroll → linhas **.res** padding `11 24`, gap 16, border-bottom hairSoft:
   - `.name` fill 12.5 Theme.t1 elide · `.prv` chip 120 · `.sz` mono Theme.t2 · `.se` mono **Theme.up**(verde) · `.le` mono Theme.t3 · `.ag` mono Theme.t4 · `.add` 28×28 botão + (border Theme.hair; **:hover** border Theme.accent + Theme.accentText)
   - 6 resultados (Forza.Horizon.6-CODEX TPB 92.4GB 412/88 3d; FitGirl Repack 58.1GB 389/120; MULTi15-ElAmigos 1337x 71.9GB 154/63; Deluxe RuTracker 96GB 77/41; Update v1.2 TPB 2.1GB 203/12; OST FLAC 1337x 480MB 34/5)
5. **.foot** hint "42 resultados · 3 provedores"

## Estados: .add hover · .res hover (se houver)
## Valores exatos de dims: ler o <style> inline de BATorrent Search.html.
