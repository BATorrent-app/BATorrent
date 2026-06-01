# BATorrent — Starter Kit p/ porte QML (1:1)

Pacote de **especificação**, não de implementação. Quem escreve os `.qml` é o Claude Code
(que tem runtime `qml` + grep p/ os self-checks). Aqui está a ground-truth mecânica.

## O que tem
- **`Theme.qml`** ✅ pronto — singleton com TODOS os tokens (cores, sp1–sp6, fontSans/fontMono) e as **5 paletas** (dark/light/midnight/sakura/darkstar), derivadas literalmente dos `batorrent-home*.css`. Mais `animeSource`/`animeBottom`/`hasAnime`. Sem hex hardcoded. `qmldir` registra o singleton.
- **`MAPPING.md`** — qual `.qml` ← qual HTML base + variantes + CSS + assets. ⚠️ corrige o briefing: Welcome/Notes/About usam `bat-dialog.css` + `<style>` inline (NÃO `app-screens.css`).
- **`ASSETS.md`** — 13 ícones reais (`icons/*.svg`, de `src/icons`) + 8 imagens (`images/*`), com uso.
- **`MISSING_ASSETS.md`** — glifos que eram SVG inline e NÃO existem em `src/icons` (chevron, coração/Doar, ×/close). Criar antes de portar — sem placeholder.
- **`*.checklist.md`** (15) — um por tela + `BatDialog.checklist.md` (chrome compartilhado). Cada um enumera, na ordem do HTML, todos os elementos visíveis, dimensões/cores literais e os estados (hover/on/sel/disabled/focus). É a "fonte mecânica" do porte.

## Ordem sugerida de implementação
1. `Theme.qml` (pronto) → confira `Theme.name` chaveando as 5 paletas.
2. `BatDialog.qml` (chrome) — base de 8 diálogos.
3. `Main.qml` — a tela mais densa; valida o padrão (ícones SVG tingidos, grade/lista, gráfico, detalhe, anime).
4. Demais telas, cada uma com seu `.checklist.md`.

## Regras de ouro (do briefing)
- Tradução **literal** CSS→QML. Nada de arredondar/“melhorar”. Criatividade = bug.
- Fontes decimais (12.5/11.5/10.5/9.5) → **font.pointSize** (nunca pixelSize decimal).
- Sempre **Theme.name** (nunca `name === ...` solto). Sem demo controls de tema na subbar (mora na Settings).
- Ícones = `icons/*.svg` tingidos (MultiEffect colorization). **Nunca emoji.**
- Cantos arredondados em Image → MultiEffect maskSource (clip não recorta canto).
- Listas completas (6 pills, 9 seções de settings, 5 abas de detalhe, etc.). Sem reduzir.
- Cada `.qml` começa com `// Source: <arquivo>.html + <arquivo>.css` e vem com `<arquivo>.audit.md`.

## Fonte da verdade visual
Os HTML do projeto (pasta raiz, fora deste kit) renderizam 1:1 o alvo. Abra lado a lado ao portar.
