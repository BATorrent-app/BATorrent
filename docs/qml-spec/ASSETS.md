# ASSETS.md — cobertura de assets

Todos os caminhos abaixo existem no pacote (pasta `qml-export/`). Use `qrc:/` conforme seu .qrc.

## Ícones (de `src/icons` do app — line icons stroke, viewBox 0 0 24 24 salvo nota)
| arquivo                | uso no QML                          |
|------------------------|-------------------------------------|
| `icons/open.svg`       | toolbar "Abrir"                     |
| `icons/magnet.svg`     | toolbar "Magnet"                    |
| `icons/pause.svg`      | toolbar "Pausar"                    |
| `icons/play.svg`       | toolbar "Retomar"                   |
| `icons/stop.svg`       | toolbar "Parar"                     |
| `icons/trash.svg`      | toolbar "Remover"                   |
| `icons/search.svg`     | toolbar "Buscar" + campo de busca (subbar)   |
| `icons/rss.svg`        | toolbar "RSS"                       |
| `icons/settings.svg`   | toolbar "Config."                   |
| `icons/grid.svg`       | toggle "Grade"                      |
| `icons/list.svg`       | toggle "Lista"                      |
| `icons/download.svg`   | spd-mod seta download + .v-dn       |
| `icons/upload.svg`     | spd-mod seta upload + .v-up         |

> Tingir os SVGs com a cor do tema via `MultiEffect { colorization: 1.0; colorizationColor: Theme.t3 }`
> (ou a cor do estado). NÃO usar emoji. Chevron de "Categorias" e estrela/coração de "Doar"
> não existem em `src/icons` — ver `MISSING_ASSETS.md`.

## Imagens
| arquivo                  | uso                                  |
|--------------------------|--------------------------------------|
| `images/logo.svg`        | brand (toolbar 30px), about, welcome, empty |
| `images/forza.png`       | pôster Forza Horizon 6 + capa detalhe |
| `images/007.jpg`         | pôster 007 First Light               |
| `images/hollow.webp`     | pôster Hollow Knight                 |
| `images/eyes-dark.png`   | anime · tema Dark (canto sup-dir)    |
| `images/eyes-midnight.png` | anime · tema Midnight              |
| `images/eyes-sakura.png` | anime · tema Sakura (fundo já recortado/transparente) |
| `images/spider.jpg`      | anime · tema Dark Star (canto inf-dir, ~560px) |

Tema **Light** não tem arte anime (`Theme.animeSource === ""`).
