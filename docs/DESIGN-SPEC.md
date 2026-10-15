# BATorrent: especificação de design

Consulte este arquivo antes de criar ou mudar qualquer tela. Se uma tela nova precisar de algo que não está aqui, adicione aqui primeiro, depois desenhe.
Referências vivas: `Downloads Final.dc.html` (tela principal), `Add Download v2`, `Player`, `VPN`, `Settings v3`, `Welcome Wizard`, `Themes v4`.

---

## 1. Princípios

1. **Um sinal por informação.** Cada estado aparece uma vez por item: um símbolo, um rótulo, uma barra. Nunca selo + texto + ícone + cor dizendo a mesma coisa.
2. **Uma cor, um significado.** Vermelho = baixando agora ou problema. Verde = concluído. Âmbar = só a seta de upload. Laranja = aviso que pede ação. Cinza = parado. Nenhuma cor de estado é usada como decoração.
3. **A arte é a cor.** Pôsteres e capas trazem a cor; a interface é neutra em volta deles.
4. **Mostrar, não explicar.** Nada de caixas de "como funciona". Se algo precisa de explicação, mostre o estado real ("Buscando metadados · 0:12").
5. **Dizer o porquê.** Todo item parado ou com problema diz o motivo numa frase curta ("Sem peers", "Sem espaço · faltam 12 GB", "Ausente · movido").
6. **Aparecer só quando serve.** Controles dependentes ficam ocultos até fazerem sentido (setas de navegação, "Sua imagem", editor de tema).
7. **Mesma linguagem, layouts diferentes.** Grade e lista são duas vistas da mesma coisa: os mesmos estados, símbolos, cores e textos.
8. **Nada duplicado na tela.** Se uma informação já está visível (velocidade global, VPN, espaço livre), outra área não repete.

---

## 2. Cores (tema Default)

Os temas mudam os valores, mas **nunca os papéis**. Todo componente usa o papel, nunca o hex.

### Superfícies
| Papel | Hex | Uso |
|---|---|---|
| `frame` | `#050506` | Barra de título da janela (35px). Mais escura que o app, de propósito: separa a moldura do conteúdo |
| `nav` | `#0c0c0e` | Navegação, status bar |
| `bg` | `#0e0e10` | Área de conteúdo |
| `panel` | `#141416` | Details, inspector, cards de settings |
| `elev` | `#1b1b1e` | Modais, menus, popovers |
| `field` | `#0b0b0c` | Campos, segmentados, trilhos |
| `hover` | `rgba(255,255,255,0.06)` | Hover de linhas e botões fantasma |
| `selected` | `rgba(255,255,255,0.08)` | Linha/chip selecionado |
| `hair` | `rgba(255,255,255,0.08)` | Divisores, bordas de campo |
| `hairSoft` | `rgba(255,255,255,0.05)` | Divisores entre linhas de tabela |

Separação entre a barra de título e a navegação: só a linha `hair` de 1px. Nunca um vão nem um degrau de cor sem linha.

### Texto
| Papel | Hex | Contraste em `bg` | Uso |
|---|---|---|---|
| `t1` | `#f3f3f4` | ~17:1 | Títulos, nomes, valores |
| `t2` | `#d9dade` | ~13:1 | Texto de leitura, valores secundários |
| `t3` | `#999a9f` | ~7:1 | Metadados, rótulos de estado, legendas |
| `t4` | `#7d7e87` | ~4.8:1 | Rótulos de coluna, dicas. Mínimo absoluto |

Nenhum texto abaixo de `t4`. Nada de texto com opacidade reduzida.

### Estados (fixos em todos os temas; só a luminosidade é ajustada)
| Papel | Hex | Onde |
|---|---|---|
| `down` | `#e5332b` | Barra e seta de baixando; botão primário; aba ativa (sublinhado) |
| `downText` | `#ec6a64` | Texto vermelho sobre fundo escuro (links, erro) |
| `done` | `#24692f` | Preenchimento de barra concluída (lista e details) |
| `doneDot` | `#3fb950` | Ponto/check de concluído, "cabe" |
| `up` | `#d99a2b` | **Só** a seta ↑ de upload |
| `warn` | `#e8833a` / texto `#f0a35e` | Aviso que pede ação: sem espaço, buscando há muito tempo, destino não cabe |
| `paused` | `#54555c` | Barra pausada, ponto de pausado/na fila |
| `missing` | listras `#e5332b` / `#ffffff` 45° | Arquivos ausentes (único estado com padrão) |

ΔE mínimo entre `down` e `doneDot` ≥ 20; entre `warn` e `up` ≥ 10. Não crie uma cor nova para um estado novo: encaixe-o num dos papéis acima.

---

## 3. Tipografia

- **Interface:** IBM Plex Sans (400/500/600/700). **Mono:** IBM Plex Mono para hashes, caminhos, magnets e códigos.
- **`fontDisplay`** (fonte do tema) só em: abas da navegação, título do details, títulos de página e de wizard. **Nunca em números.**
- Números sempre com `font-variant-numeric: tabular-nums`.
- Renderização: `NativeTextRendering`, `pixelSize` (nunca `pointSize`). Carregar todos os pesos usados.

| Nível | Tamanho / peso | Uso |
|---|---|---|
| Rótulo caps | 11px · 600 · +0.08em · CAIXA ALTA · `t3`/`t4` | Cabeçalhos de seção ("STORAGE"), rótulos de estado na lista |
| Meta | 12–12.5px · 400 · `t3` | Legenda de card, linha de metadados |
| Tabela | 13px · 400 | Linhas da lista, colunas |
| Corpo | 14–14.5px · 400/500 | Settings, modais, menus |
| Aba / botão | 15px · 500–600 | Navegação, botões de diálogo |
| Título de diálogo | 17px · 600 | Um por modal, sem eyebrow |
| Título de details | 20–22px · 600 · `fontDisplay` | Nome do item selecionado |
| Título de página | 28–52px · `fontDisplay` | Wizard, telas vazias, onboarding |

Mínimo: 11px, e só para rótulos caps. Texto de leitura nunca abaixo de 12.5px.

---

## 4. Espaço, cantos e alturas

- **Grade de 4px:** 4 · 6 · 8 · 10 · 12 · 16 · 20 · 24 · 32 · 48. Grupos de irmãos usam `gap`, nunca margens soltas.
- **Cantos (`radius` do tema; valores do Default):** controles 6 · botões e chips 8 · cards e painéis 10 · modais 12 · pílulas 999.

| Elemento | Altura |
|---|---|
| Barra de título (Windows) | 35px + 1px de linha |
| Navegação | 52px |
| Toolbar | 50px |
| Barra de filtros | 46px |
| Cabeçalho de tabela | 26px |
| Linha da lista | 40px |
| Status bar | 26px |
| Botão de toolbar | 32px |
| Botão de diálogo | 36px (primário e cancelar) |
| Botão grande (wizard, vazio) | 46–48px |
| Chip / segmentado | 28–36px |
| Alvo mínimo clicável | 28×28px |
| Painel de details (lista) | 262px de altura |
| Inspector (grade) | 320–340px de largura |

---

## 5. Moldura do app (igual em todas as telas)

```
┌ Barra de título 35px · frame ─────────────────────────────────────┐
│ [← →] (só quando há histórico)   título cinza 12.5px   – ▢ ✕     │
├ Navegação 52px · nav ─────────────────────────────────────────────┤
│ logo▾  Downloads  Find  HUB  ···(área de arrastar)···  [Continuar] VPN ⚙ │
├ Toolbar 50px ─────────────────────────────────────────────────────┤
│ [+ Add] | contexto da seleção | ações          disco · ↓ · ↑ · tartaruga │
├ Filtros 46px ─────────────────────────────────────────────────────┤
│ busca · Grid/List · chips de estado com contagem   ·   categoria ▾ │
├ Conteúdo ─────────────────────────────────────────┬ Inspector ────┤
│                                                   │ (grade)        │
├ Details 262px (lista) ────────────────────────────┴───────────────┤
└ Status bar 26px · versão · itens · porta ─── totais ──────────────┘
```

- **Barra de título:** só o título (sem logo, que já está na navegação). Setas aparecem só com histórico; voltar ativo e avançar apagado até haver para onde ir. Sem "?" (o Help está no menu da logo). O título diz onde você está: "Downloads · Silent Hill: Townfall".
- **Navegação:** a logo abre o menu (File/Torrent/Settings/Help). A aba ativa tem sublinhado `down` de 2px. A área vazia arrasta a janela; duplo clique maximiza.
- **Status bar:** só o que não está em outro lugar (versão, contagem, porta, totais). Sem velocidades, sem espaço livre, sem frases de enfeite.

---

## 6. Componentes

### Botões
| Tipo | Visual | Uso |
|---|---|---|
| Primário | fundo `down`, texto branco 600 | Uma ação por tela/modal (Add, Continuar) |
| Primário dividido | primário + ▾ separado por 1px escuro | Quando há variantes (Add and start / Add paused) |
| Secundário | borda `hair`, texto `t2` | Cancelar, Browse, Alterar |
| Fantasma | sem fundo, hover `hover` | Ações de toolbar, ícones |
| Destrutivo | borda e texto `downText` | Remover, Excluir tema (só em contexto de edição) |
Texto do botão = verbo + objeto quando há ambiguidade ("Add 3", "Use Games", "Keep only Português (BR)").

### Chips e segmentados
- **Chip de filtro:** texto `t3` + contagem `t4`; ativo = `t1` 600 + fundo `selected`.
- **Chip de escolha** (destino, idioma): borda `hair`; selecionado = anel 1.5px `t1`; problema = anel `warn` + subtexto laranja; cabe = subtexto `doneDot`.
- **Segmentado:** trilho `field` com borda `hair`; opção ativa com fundo `rgba(255,255,255,0.12)`; desabilitada com opacidade 0.4 e tooltip explicando o porquê.

### Progresso
| Onde | Forma | % |
|---|---|---|
| Grade (sobre a arte) | barra 6–9px no rodapé do pôster, com gradiente escuro atrás | à direita da barra, branco 600 |
| Lista | pílula 18px, preenchimento sólido da cor do estado | centralizado dentro |
| Details | barra 6–7px | na linha de texto abaixo ("62% · 4,6 de 7,4 GB") |
- Preenchimento com largura mínima igual à altura (não vira "bolinha" em % baixo).
- Concluído na grade: **sem barra**. Concluído na lista e no details: barra verde `done`.
- Brilho em movimento só quando há transferência real (> 0 B/s).
- Pausado em 0%: sem barra; só a legenda.

### Estados: vocabulário fixo
| Estado | Símbolo | Cor do símbolo | Rótulo (lista) | Legenda (grade) |
|---|---|---|---|---|
| Baixando | ↓ | `down` | DOWNLOADING · 12m | Baixando · 12m |
| Semeando (enviando) | ↑ | `up` | SEEDING | Enviando 2,3 MB/s |
| Semeando (ninguém baixando) | ↑ | `up` | SEEDING | ano · gêneros |
| Concluído (envio parado) | ✓ | `doneDot` | DONE | ano · gêneros |
| Pausado | ‖ | `paused` | PAUSED | Pausado · 7,2 de 39,9 GB |
| Na fila | ··· | `paused` | QUEUED | Na fila · 3º |
| Buscando metadados | ○ | `t3` (`warn` após 2 min) | FETCHING · 2:35 | Buscando · 2:35 |
| Sem peers / sem seeds / recusando | ↓ | `down` | NO PEERS | Sem peers · parado há 4m |
| Sem espaço | ! | `warn` | NO SPACE | Sem espaço · faltam 12 GB |
| Ausente | listras | `missing` | MISSING | Ausente · movido |
| Erro | ! | `down` | ERROR | Erro · motivo curto |
- Rótulo da lista sempre `t3`, 11px 600, caixa alta e nunca truncado; só o símbolo é colorido.
- Na grade, o ponto (6px) tem a cor do estado e o texto fica `t2` quando ativo ou com problema, `t3` quando parado.
- Upload/download por linha: "0 B/s" = ativo sem tráfego; "—" = parado. Setas só no cabeçalho da coluna.

### Card da grade (pôster)
- Proporção 2:3, cantos `radius`, **título fora da arte** (14px 600, uma linha com reticências) + legenda de estado (12px).
- Nenhum selo de estado sobre a arte. Sobre a arte só: barra de progresso e contagem de série (pílula escura no canto).
- Série = pilha de capas (2 lâminas atrás) + contagem; legenda "S01 · 3 episódios" ou "Temporada 1 + 2 episódios".
- Sem arte: fundo `panel`, tipo do arquivo em caps à esquerda em cima, nome do arquivo embaixo. Alinhado à esquerda, sem linha decorativa.
- Seleção: anel 2px `t1` (neutro, nunca vermelho).

### Linha da lista
- 40px; nome 13px (episódio: "S01E01 · nome", com o nome bruto no tooltip); motivo do estado em `t3` ao lado do nome.
- Seleção: fundo `selected` + barra de 2px `t1` à esquerda. Grupo de série expansível com ▾ e contagem.

### Details / inspector
- Identidade fixa à esquerda (lista) ou no topo (inspector): capa, título `fontDisplay`, "ano · gêneros · nota", progresso e velocidades (↓ `down`, ↑ `up`, ETA `t1`).
- Abas: General · Peers · Files · Trackers · Pieces · Graph. O indicador desliza (300ms) e o conteúdo entra com fade + 14px na direção da troca.
- Seções com rótulo caps (STORAGE, TRANSFER, HEALTH); pares rótulo `t4` / valor `t2` alinhados à direita.
- Sem seleção (grade): Visão geral = Precisa de você / Pronto para usar / Quando tudo terminar. Nada que já esteja na tela.
- Valores impossíveis nunca aparecem: disponibilidade ≥ 1.00 em item completo; ratio com 2 casas arredondadas.

### Modais
- Largura 560–600px, `elev`, cantos 12, sombra `0 30px 80px rgba(0,0,0,.6)`, fundo da janela escurecido a 70%.
- **Um título** (17px). Sem eyebrow, sem subtítulo que repita o título.
- Corpo com `gap` 16–18px. Rodapé de 64px: resumo à esquerda (`t3`), Cancelar + primário à direita.
- Esc fecha; Enter aciona o primário; o primeiro campo recebe o foco.
- Avisos dentro do modal ficam **junto do controle que resolvem** (o de espaço vai junto do destino), com as ações de correção como botões.

### Menus e menu de contexto
- `elev`, cantos 9, padding 6, itens de 34px, 14px. Grupos separados por linha `hair`.
- Itens dependem do estado e do tipo: Jogar/Assistir só quando é reproduzível; Pausar envio / Pausar / Retomar; "Copiar ▸" agrupa tudo que copia; Extrair só em compactados.
- Destrutivo por último, em `downText`.

### Campos
- 42–46px, `field`, borda `hair`; foco = anel 1.5px `rgba(255,255,255,0.35)`.
- Placeholder em `t4` diz o que entra ("Magnet, download link or info-hash"), nunca "Digite aqui".
- Caminhos e hashes em mono.

---

## 7. Movimento

- Curva padrão: `cubic-bezier(.2,.8,.2,1)`. Durações: 160ms (hover, cor), 240ms (conteúdo), 300ms (indicadores), 400–520ms (troca de passo, tema).
- Troca de conteúdo: fade + 14px na direção da navegação. Indicadores deslizam, não pulam.
- Nada se move sem causa (sem animação em loop, exceto o brilho de transferência ativa).
- `reduceMotion`: só fade, sem deslocamento.

---

## 8. Ícones e imagens

- Um conjunto só (`src/icons`, traço de 1.5px, 16–18px na interface, 14px em toolbar densa). Cor herdada do texto (`t2`/`t3`); colorido só quando o ícone é o sinal de estado.
- Nenhuma ilustração desenhada à mão na interface. Imagem é sempre pôster/capa real ou arte do tema.
- Arte de tema e de fundo: só domínio público, CC0, licenciada ou feita para o projeto, com crédito no handoff. Imagem do usuário entra pelo modo "Minha imagem".
- Arte de fundo apaga para o lado do texto e nunca passa por baixo de texto de leitura sem painel.

---

## 9. Texto (copy)

- **"Item/itens"**, nunca "torrents" para o que não é torrent (links diretos também são itens).
- Frases curtas, em caixa normal (sentence case). Caixa alta só nos rótulos caps.
- Números: separador do idioma ("4,6 GB" em PT, "4.6 GB" em EN), tamanho como "4,6 de 7,4 GB", tempo como "12m", "2:35", "parado há 4m".
- Motivo sempre junto do estado: "Pausado · 7,2 de 39,9 GB", "Sem espaço · faltam 12 GB".
- Botões com verbo. Avisos dizem o problema e oferecem a correção ("Doesn't fit in Downloads." + "Use Games").
- Sem texto de enfeite ("Torrents share what you download") em superfícies permanentes; diga uma vez, onde a pessoa está lendo (wizard).
- Notas pessoais do desenvolvedor (o que há de novo) não são traduzidas automaticamente.

---

## 10. Temas

- Os temas mudam: superfícies, texto, `accent`, `fontDisplay`, `fontSans` (só Sakura e Matrix), `radius`, arte, textura.
- Os temas **não** mudam: layout, alturas, tamanhos de fonte, papéis de cor de estado, símbolos.
- Contraste mínimo por tema: t1 ≥ 12:1, t2 ≥ 7:1, t3 ≥ 5:1, t4 ≥ 4.5:1 no `panel`.
- Exceções documentadas: Midnight (`down` = coral) e Matrix (`doneDot` = fósforo mais aceso).
- Toda tela nova precisa ser conferida em Default, Light (tema claro) e Matrix (mono, cantos 0) antes de entrar.

---

## 11. Checklist para uma tela nova

- [ ] Usa só papéis de cor da seção 2; nenhum hex novo.
- [ ] Cada estado aparece uma vez, com o vocabulário da seção 6.
- [ ] Vermelho só para baixando ou problema; seleção neutra.
- [ ] Nenhuma informação repete o que já está visível na moldura (velocidade, VPN, disco).
- [ ] Um título por superfície; nenhuma caixa explicando o protocolo.
- [ ] Todo estado parado ou com problema diz o motivo.
- [ ] Controles dependentes aparecem só quando servem.
- [ ] Texto ≥ 12.5px (rótulos caps ≥ 11px), contraste ≥ 4.5:1, números tabulares.
- [ ] Alturas e espaçamentos da seção 4; grupos com `gap`.
- [ ] Movimento com as durações da seção 7 e alternativa para reduceMotion.
- [ ] Conferida em Default, Light e Matrix.
- [ ] Textos com "item", verbo nos botões e motivo junto do estado.
- [ ] Estados vazio, carregando, erro e "não cabe" desenhados, não só o caminho feliz.
