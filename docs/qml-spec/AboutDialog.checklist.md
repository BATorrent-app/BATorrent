# AboutDialog.checklist.md
// Source: BATorrent About.html + bat-dialog.css + <style> inline. BatDialog chrome. .dlg 460×540.
// Footer hint "© 2026 · open-source" + btn.ghost (Discord) + OK.primary "Doar". ttl "Sobre o BATorrent".

## Body (centralizado)
1. `.ahero`: logo.svg 58px (claros: invert para escuro) + wordmark Row: "BAT"(Theme.accent) + "orrent"(Theme.t1) 22/800/letter 1 ; `.vrow` chip "v2.6.1" + "build estável" 10.5 mono Theme.t4 ; `.desc` 11.5 Theme.t2 "Um cliente BitTorrent leve e de código aberto."
2. **.privacy** card (Theme.panel, border Theme.hair, radius 12, padding `13`, margin `22 0`): ícone escudo 30×30 + p 10.5 Theme.t3 "Sem telemetria, sem analytics, sem chamadas pra casa. A única requisição de saída… checagem de release no GitHub — desativável."
3. **.glabel** "BIBLIOTECAS" + lista: Qt 6.10.2 / libtorrent-rasterbar 2.0.11 / OpenSSL 3.6.1 / Boost 1.86 (cada linha: nome 12 Theme.t1 + versão 11 mono Theme.t4, border-bottom hairSoft)
4. **.license** Row "Licença" Theme.t3 + "MIT" mono Theme.t2

## Estados: btn.ghost hover · btn.primary hover
