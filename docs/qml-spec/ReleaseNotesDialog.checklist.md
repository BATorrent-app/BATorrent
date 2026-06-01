# ReleaseNotesDialog.checklist.md
// Source: BATorrent Release Notes.html + bat-dialog.css + <style> inline. BatDialog chrome. .dlg 540×560.
// Footer hint "CHANGELOG.md · fonte de verdade" + btn.ghost (GitHub) + OK.primary "Fechar".

## Body
1. **.nhead** Row gap 14, border-bottom hairSoft: `.badge` 44×44 radius 10 (Theme.panel, ★ 22 Theme.accentText) + coluna [ Row(`.eyebrow.red` "NOVIDADES" + chip "v2.6.1") / `h1.title` "Confira o que mudou" / `.date` 10.5 mono Theme.t4 "28 mai 2026 · estável" ]
2. **.clg** grupo "CORREÇÃO CRÍTICA" (`.cltag.crit` dot Theme.accent + label 9/700/1.4 Theme.accentText) → itens `.cli` (marcador 13 + p 11.5 Theme.t2 line 1.45): auto-updater quebrado desde v2.5.0 (nome em b Theme.t1); timeout 15s; atualização manual da 2.5→2.6.
3. **.cldiv** divisor "DA v2.6.0"
4. **.clg** "Plugins de busca" (`.cltag.feat` dot Theme.t4): TPB+Nyaa embutidos.
5. **.clg** "Tradução & Categorias": 683+ chaves × 7 idiomas → JSON (`<code>` translator.cpp); caminhos temporários por categoria.

## Estados: btn.ghost hover
