# AddAddonDialog.checklist.md
// Source: BATorrent Add Addon.html + bat-dialog.css. BatDialog chrome. .dlg 560×540.
// Footer hint "Instale apenas addons em que você confia". OK="Fechar" (ghost; sem primary).

## Body (gap 16)
1. col: `.eyebrow.red` "ADDONS" + `h1.title` "Adicionar addon" + `.sub` 12 Theme.t3 "Estenda o BATorrent com extensões da comunidade para catálogos, streams e busca." (max 460)
2. "URL do manifesto" + `.path`: field mono placeholder "https://addon.exemplo.com/manifest.json" (ícone link 14) + **btn.primary** "Instalar"
3. **.glabel** "DA COMUNIDADE" + **.card** com 4 `.addrow` (padding `12 14`, gap 12, border-bottom hairSoft):
   - `.ic` 36×36 radius 9 (Theme.field, border Theme.hair, ícone 18 Theme.accentText) + `.info` [`.nm` 12.5/600 + `.d` 10.5 Theme.t4] + ação à direita
   - **Torrentio** "Streams de torrent para filmes e séries" → `.installed` (✓ 14 Theme.up + "Instalado" 11 Theme.up)
   - **Stremio Catalogs** "Catálogos populares e listas em alta" → btn.sm "Instalar"
   - **The Pirate Bay** "Provedor de busca (apibay)" → btn.sm "Instalar"
   - **Nyaa.si** "Provedor de busca para anime" → btn.sm "Instalar"

## Estados: btn hover · btn.primary hover
