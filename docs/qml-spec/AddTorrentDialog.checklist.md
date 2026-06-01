# AddTorrentDialog.checklist.md
// Source: BATorrent Add Torrent.html + bat-dialog.css + <style> inline
// BatDialog chrome. .dlg width 580, height 632. Footer hint "5 de 7 arquivos · 92.1 GB". OK="Adicionar".

## Body (padding 24, gap 16)
1. **.hdr** Row gap 16: `.ic` 44×44 radius 10 (Image forza cover, border 1px Theme.hair) + coluna [`.eyebrow.red` "CONFIRMAR DOWNLOAD" / `h1.title` "Forza Horizon 6" / `.m` 11.5 mono Theme.t3 "92.4 GB · 6 itens"]
2. **.fhead** space-between: `.sel-all` (chk + "Selecionar arquivos" 12 Theme.t2) + `.cnt` 11 mono Theme.t4 (dinâmico "5 de 7 · 92.1 GB")
3. **.card .ftree** max-height 224 overflow auto → linhas **.fr** height (8 0)→~40, gap 10, border-bottom 1px Theme.hairSoft, padding-left `14 + depth*18`:
   - `.chk` (ver BatDialog) on/off · ícone arquivo/pasta 15px (pasta = Theme.amber) · `.nm` 12 Theme.t1 (dir = weight 600) · `.sz` 11 mono Theme.t4
   - Itens: 📁Forza Horizon 6 (dir) / Forza.Horizon.6-CODEX.bin 78.0 GB ✓ / data1.pak 8.2 ✓ / data2.pak 5.9 ✓ / setup.exe 18 MB ✓ / 📁bonus / soundtrack.flac 492 MB ✗ / wallpapers.zip 120 MB ✗ / readme.txt 4 KB ✓
   - total dinâmico atualiza `.cnt` e footer ao marcar/desmarcar
4. **"Salvar em"** (.lbl) + `.path` (field mono "/Users/voce/Downloads" + btn "Procurar…")
5. **.togrow** "Iniciar imediatamente" + toggle **.on**

## Estados: chk on/off (clique alterna + recalcula total) · toggle on/off
