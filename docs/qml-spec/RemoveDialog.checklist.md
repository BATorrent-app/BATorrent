# RemoveDialog.checklist.md
// Source: BATorrent Remove Confirm.html + bat-dialog.css. BatDialog chrome. .dlg 440×360.
// OK="Remover" (primary, bg Theme.accent). Footer hint vazio.

## Body (padding-top 24, gap 16)
1. **.top** Row gap 12: `.warn-ic` 40×40 radius 10 bg rgba(229,51,43,0.12) border rgba(229,51,43,0.32) → ícone ⚠ 20px Theme.accentText ; coluna [ `h1` "Remover este torrent?" 16/650 Theme.t1 / `p` 12 Theme.t3 max 320 "Você está prestes a remover **Forza.Horizon.6-CODEX** da lista. Esta ação não pode ser desfeita." (nome em Theme.t2/600) ]
2. **.card** 1 linha **.opt** (clicável, padding `12 14`, gap 12): `.chk` **.on** (✓) + coluna [ "Também excluir os arquivos do disco" 12.5 Theme.t1 / "Apaga 62.1 GB já baixados permanentemente" 10.5 Theme.t4 ]

## Estados: chk on/off (clique alterna)
