# WelcomeDialog.checklist.md
// Source: BATorrent Welcome.html + bat-dialog.css + <style> inline. BatDialog chrome. .dlg ~560×470.
// Footer: checkbox "Não mostrar novamente" (esquerda) + OK.primary "Começar". ttl "Bem-vindo ao BATorrent".

## Body (centralizado)
1. `.hero`: logo.svg 52px + `.eyebrow.red` "BEM-VINDO" + `h1` "Pronto pra compartilhar." (25/800/sans) + `p` 12.5 Theme.t2 max 400 center "BATorrent é um cliente BitTorrent leve e open-source. Funcional, escuro, focado."
2. `.cards` grid 2×2 (gap 10, margin-top 22): 4 `.acard` (height ~64, radius 11, bg Theme.panel, border 1px Theme.hair; **:hover** border Theme.accent): ícone 38×38 radius 9 (Theme.field/Theme.accentText) + [`.t` 12.5/600 + `.d` 10.5 Theme.t4]
   - "Abrir .torrent" / "Adicionar arquivo do disco"
   - "Colar magnet" / "Da área de transferência"
   - "Buscar" / "Stremio · Torrentio"
   - "Inscrever RSS" / "Auto-baixar novos"
3. footer checkbox `.chk.on` "Não mostrar novamente"

## Estados: acard hover · chk on/off
