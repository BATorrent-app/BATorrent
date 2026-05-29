# MISSING_ASSETS.md

Ícones referenciados no HTML que **não existem** em `src/icons` (eram SVG inline no HTML).
Exportar do app ou recriar antes de portar — NÃO usar placeholder/emoji.

| onde            | glifo necessário          | HTML (inline path)                                  |
|-----------------|---------------------------|-----------------------------------------------------|
| subbar `.cat`   | chevron ⌄ (seta baixo)    | `<path d="M6 9l6 6 6-6"/>` 13px, stroke t-4         |
| subbar `.donate`| coração ♥ preenchido      | `<path d="M12 21s-7.5-4.6-10-9.3..."/>` 14px, fill accent |
| `.x-close` (diálogos) | × fechar            | `<path d="M5 5l14 14M19 5L5 19"/>` 13px, stroke t-4 |
| detalhe/RSS/etc | setas, estrela, etc.      | ver SVG inline de cada HTML correspondente          |

Sugestão: criar `icons/chevron.svg`, `icons/heart.svg`, `icons/close.svg` com os paths exatos acima.
