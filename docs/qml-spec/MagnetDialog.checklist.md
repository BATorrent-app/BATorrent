# MagnetDialog.checklist.md
// Source: BATorrent Magnet.html + bat-dialog.css. BatDialog chrome. .dlg width 480, ~470h.
// Footer hint "Aceita vários links, um por linha". OK="Adicionar".

## Body (gap 16)
1. col: `.eyebrow.red` "ADICIONAR" + `h1.title` "Link magnet"
2. "Cole o link magnet" (.lbl) + **textarea.ta** (88h, mono 11.5) com magnet de exemplo "magnet:?xt=urn:btih:657c6a58e3b1f24c9a7d42e7&dn=Forza.Horizon.6-CODEX&tr=udp://tracker.opentrackr.org:1337"
3. **.meta-note** card (Theme.panel, border Theme.hair, radius 9, padding `10 12`): ícone info 15 Theme.t3 + p 11 Theme.t3 "Os metadados (nome e lista de arquivos) serão baixados da rede após adicionar."
4. "Salvar em" + `.path` (field mono "/Users/voce/Downloads" + "Procurar…")
5. **.togrow** "Iniciar imediatamente" + toggle .on

## Estados: textarea focus · toggle on/off
