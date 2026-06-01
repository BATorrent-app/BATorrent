# DetailWindow.checklist.md
// Source: BATorrent Detail Tabs.html + bat-detail.css (+ <script> inline gera peers/files/trackers/pieces).
// Janela 940×420 (painel de detalhe destacado). 5 abas com troca funcional.

## Estrutura
1. **.dtabs** height 44, padding `0 24`, gap 24, border-bottom 1px Theme.hair → tabs 12.5: **Geral**, **Peers**(38), **Arquivos**(6), **Trackers**(4), **Pedaços** (count `.ct` 11 mono Theme.t4 ao lado). **.on** Theme.t1 + underline 2px Theme.accent. (Peers = on por padrão)
2. **.dview** (fill) — pane ativo:
   - **Geral**: `.gbody` cover 96×134 (gradiente) + `.gmain` (h3 "Forza Horizon 6" 16/650, sub 11.5 Theme.t3, desc 12 Theme.t2) + `.gcols` 2 colunas KV (Transferência: Baixado/Download(v-dn)/Upload(v-up)/ETA; Peers: Seeds/Peers/Ratio)
   - **Peers**: `.thd` (ENDEREÇO IP fill / CLIENTE 150 / FLAGS 90 / PROGRESSO 64 dir / DOWN 96 dir / UP 96 dir, 10/700 Theme.t4) + linhas `.tr` height 38: ip mono · cliente Theme.t2 · flags mono Theme.t4 (🇧🇷 D U etc) · dot saúde + % mono · down (Theme.accentText/"—") · up (Theme.up/"—"). 7 peers.
   - **Arquivos**: `.fr2` height 40: chk + ícone (pasta amber) + nome + barra fina (track + fill Theme.accent width pct) + % mono + tamanho + `.pri` select (Alta=accentText/Normal/Baixa/Pular=Theme.t4). 6 arquivos.
   - **Trackers**: `.thd` (URL fill mono / STATUS 120 / SEEDS / PEERS / LEECH / PRÓX.) + linhas: status `.st-ok`(verde dot+"Funcionando") / `.st-work`(cinza "Atualizando…") / `.st-err`(vermelho "Tempo esgotado"); inclui ** [DHT] ** e ** [PEX] **. 6 linhas.
   - **Pedaços**: `.pmap` grid 40 colunas de células `.pc` (aspect 1, radius 2): **.done** Theme.accent / **.dl** Theme.amber / **.part** rgba(accent,0.4) / pendente Theme.field. ~62% done. + `.plegend` (4 itens) + `.pinfo` (Pedaços 14.760 / Tamanho 8 MiB / Concluídos 9.889 (67%) / Disponibilidade 2.41)

## Estados: tab default/on (clique troca pane) · chk on/off · select hover
## Dims exatas: bat-detail.css.
