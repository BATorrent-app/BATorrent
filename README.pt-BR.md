<p align="center">
  <a href="README.md">English</a> | <b>Português</b> | <a href="README.zh-CN.md">中文</a> | <a href="README.ja.md">日本語</a> | <a href="README.ru.md">Русский</a> | <a href="README.es.md">Español</a> | <a href="README.de.md">Deutsch</a> | <a href="README.ua.md">Українська</a>
</p>

<p align="center">
  <img src="src/images/logo.svg" alt="BATorrent" width="120">
</p>

<h1 align="center">BATorrent</h1>

<p align="center">
  Um cliente de torrent que mostra seus downloads como capas, toca eles enquanto baixam<br>
  e junta seus filmes, séries e jogos numa biblioteca só.
</p>

<p align="center">
  <a href="https://github.com/BATorrent-app/BATorrent/releases/latest"><img alt="Versão" src="https://img.shields.io/github/v/release/BATorrent-app/BATorrent?style=flat-square&color=dc2626"></a>
  <a href="https://github.com/BATorrent-app/BATorrent/releases"><img alt="Downloads" src="https://img.shields.io/github/downloads/BATorrent-app/BATorrent/total?style=flat-square&color=dc2626"></a>
  <a href="LICENSE"><img alt="Licença" src="https://img.shields.io/github/license/BATorrent-app/BATorrent?style=flat-square&color=dc2626"></a>
  <img alt="Plataformas" src="https://img.shields.io/badge/Windows%20·%20macOS%20·%20Linux-dc2626?style=flat-square">
  <a href="https://apps.microsoft.com/detail/9n4l3tq24rc6"><img alt="Microsoft Store" src="https://img.shields.io/badge/Microsoft%20Store-get-dc2626?style=flat-square&logo=microsoft"></a>
</p>

<p align="center">
  <a href="#instalar"><b>Baixar</b></a> &nbsp;·&nbsp;
  <a href="https://batorrent.com/assets/trailer.mp4"><b>Assistir ao trailer</b></a> &nbsp;·&nbsp;
  <a href="https://batorrent.com"><b>Site</b></a>
</p>

<p align="center">
  <img src="docs/images/grid.jpg" alt="A aba Downloads no modo grade, com uma série selecionada e os episódios dela à direita" width="900">
</p>

O BATorrent é um cliente de torrent para desktop feito sobre o [libtorrent](https://www.libtorrent.org/), o motor que o qBittorrent e o Deluge usam. Ele lê o nome de cada torrent, acha o pôster correspondente (TMDB para filmes e séries, IGDB para jogos) e mostra seus downloads como capas em vez de nomes de arquivo. Episódios da mesma série ficam agrupados, o vídeo toca no app antes de o download terminar, e os jogos podem ser instalados e abertos pela mesma janela.

É gratuito e de código aberto. Sem anúncios, sem conta, sem versão "Pro", sem analytics. Além dos próprios torrents, ele acessa a internet para três coisas: capas e detalhes do TMDB, IGDB e Steam para o que está na sua biblioteca ou o que você abre no Encontrar; a verificação de atualização no GitHub, que dá para desligar ([`updater.cpp`](src/services/integrations/updater.cpp)); e, nos builds oficiais, um relatório de crash se o app travar (que também dá para desligar).

## Três abas

O app tem três abas no topo: **Downloads**, **Encontrar** e **HUB**.

### Downloads

Seus torrents numa grade de capas ou numa lista densa, com os detalhes do que você selecionar ao lado. As séries viram uma linha só com os episódios dentro, então doze episódios de uma série ocupam uma linha em vez de doze.

<p align="center">
  <img src="docs/images/list.jpg" alt="A aba Downloads no modo lista, com uma série aberta mostrando os episódios" width="900">
</p>

### Encontrar e HUB

O **Encontrar** é onde você procura algo novo: uma página inicial com filmes, séries e jogos em alta, e uma busca que passa pelas suas fontes e coloca o melhor resultado primeiro. Cada título tem a própria página. A de um filme lista os releases, ordenados por quem está compartilhando, com uma escolha recomendada e o resto da coleção embaixo. A de uma série lista os episódios por temporada, com o que você tem e o que já assistiu.

<table>
  <tr>
    <td width="50%"><img src="docs/images/find.jpg" alt="Encontrar: a página de um filme com os releases e o resto da coleção"></td>
    <td width="50%"><img src="docs/images/series.jpg" alt="Encontrar: a página de uma série com os episódios, os assistidos e os que estão na sua biblioteca"></td>
  </tr>
  <tr>
    <td align="center"><sub>Um filme no Encontrar</sub></td>
    <td align="center"><sub>Uma série no Encontrar</sub></td>
  </tr>
</table>

O **HUB** é a biblioteca do que você já tem: continuar assistindo, continuar jogando, e tudo agrupado em jogos, séries e filmes.

<p align="center">
  <img src="docs/images/hub.jpg" alt="HUB: continuar assistindo, continuar jogando e a biblioteca" width="900">
</p>

## O player

O player embutido usa FFmpeg, então MKV, AVI e WebM tocam direto. Ele baixa o começo do arquivo primeiro, então você pode assistir enquanto o resto chega. Ele lembra onde você parou, pula aberturas e, no fim, oferece o próximo episódio, ou o próximo filme de uma coleção. Ele acha legendas para você (pelo SubDL) ou carrega o `.srt` que está ao lado do arquivo. A decodificação roda num processo separado e isolado (sandbox), então um arquivo malformado não derruba o app junto.

<p align="center">
  <img src="docs/images/player.jpg" alt="O player embutido, com o próximo episódio na fila" width="900">
</p>

Se você tem uma conta no [Real-Debrid](https://real-debrid.com) ou no [TorBox](https://torbox.app) e o torrent já está em cache lá, ele faz o streaming por eles, e nada é baixado nem semeado na sua máquina.

## O que mais ele faz

- **Jogos.** Capas do IGDB, instalar e abrir pelo HUB, tempo de jogo e conquistas. Seus downloads de jogos acabam parecendo um launcher em vez de uma pasta de instaladores.
- **VPN.** Importe um `.conf` do WireGuard (Proton, Mullvad, IVPN...) e conecte. Só o BATorrent passa por ela; o resto do computador continua com a conexão normal, e nada é instalado no sistema. Ela reconecta sozinha se cair, o kill switch para os torrents até ela voltar e, nos servidores que oferecem isso, ela abre uma porta para os peers conseguirem chegar até você.
- **Privacidade.** Um teste de vazamento que mostra o IP que seus peers veem, vínculo a uma interface de rede, um modo para trackers privados, um preset para Tor e bloqueio de clientes leechers conhecidos.
- **Adicionar.** Cole um magnet, um link ou um info-hash em qualquer lugar com Ctrl+V. RSS com filtros regex, uma pasta monitorada, arrastar e soltar, e downloads HTTP/HTTPS comuns.
- **Controle remoto.** Uma WebUI que você abre no celular escaneando um QR code. O código é gerado na sua máquina.
- **Organização.** Categorias e tags, extração automática de arquivos compactados, limites de proporção e de tempo, um agendamento de banda por hora e dia, e uma velocidade alternativa (a tartaruga) a um clique.
- **Visual.** Sete temas, cada um com a própria fonte e arte, ou um feito por você; um fundo limpo, a arte do tema ou uma imagem sua. Uma paleta de comandos no Ctrl/⌘+K. Nove idiomas, e uma configuração rápida na primeira vez que abre.

<p align="center">
  <img src="docs/images/themes.jpg" alt="A aba Downloads nos sete temas: Default, Cinema, Light, Midnight, Sakura, Dark Star e Matrix" width="900">
</p>

<details>
<summary><b>A lista completa</b></summary>

Prioridade por arquivo, download sequencial, injeção de trackers, controle do layout de conteúdo, regex para excluir arquivos, uma pasta temporária separada, janelas de seed, pausa automática quando os arquivos somem, atualização da biblioteca do Plex/Jellyfin/Emby, notificações no Telegram e no Discord, importação do qBittorrent, criação de `.torrent`, listas de bloqueio de IP, criptografia de protocolo, um espelho de atualização no Gitee, canais de atualização estável e beta, desligar o computador quando os downloads terminam, um ajudante de exclusão no Windows Defender, backup e restauração, histórico de itens removidos recentemente, início forçado, um visualizador de log com diagnósticos, uma barra de menu no Alt, filmes da mesma coleção agrupados na biblioteca, e atalhos de teclado para quase tudo.

</details>

## O motor

O BATorrent traz um pequeno fork do libtorrent com patches, aplicados como patches versionados em [`third_party/patches/`](third_party/patches):

- **Arrancada mais rápida.** Num link rápido com latência alta, o libtorrent padrão aumenta o pipeline de requisições um passo de cada vez. O fork aumenta de forma geométrica, então enche o link em bem menos idas e voltas. Nos testes A/B do projeto, isso deu cerca de 27% a mais de velocidade numa conexão rápida, e nunca ficou mais lento que o padrão.
- **Peers do mesmo país.** Um banco GeoIP offline (db-ip Lite) marca cada peer pelo país e, quando dá para escolher, o fork prefere peers do seu próprio país, o que costuma significar latência menor.

Os dois são opções de compilação e ficam desligados num build padrão.

## Instalar

| Plataforma | Download | Requisitos |
|---|---|---|
| **Windows** | [Microsoft Store](https://apps.microsoft.com/detail/9n4l3tq24rc6) · [Instalador](https://batorrent.com/win) · [Portátil](https://batorrent.com/portable) | Windows 10 ou mais recente |
| **macOS** | `brew install --cask Mateuscruz19/batorrent/batorrent` ou o [`.dmg`](https://batorrent.com/mac) | macOS 12+, Apple Silicon |
| **Linux** | [AppImage](https://batorrent.com/linux) | glibc 2.35+ |

Com o app aberto, solte um `.torrent` ou um link magnet na janela.

<sub><b>macOS:</b> o app ainda não é notarizado (o programa de desenvolvedor da Apple é uma assinatura paga). O Homebrew é o caminho mais fácil porque remove a flag de quarentena. Com o <code>.dmg</code>, clique com o botão direito no app e escolha <b>Abrir</b> na primeira vez.</sub>

<details>
<summary><b>Compilar a partir do código-fonte</b></summary>

**Requisitos:** C++17, CMake 3.16+, Qt 6 (`Widgets`, `Network`, `Svg`, `Multimedia`, `Quick`, `QuickWidgets`, `QuickControls2`), libtorrent-rasterbar 2.0+, Boost, zlib e, opcionalmente, Qt6Keychain.

```bash
# Debian / Ubuntu
sudo apt install build-essential cmake qt6-base-dev qt6-svg-dev qt6-multimedia-dev \
    libtorrent-rasterbar-dev libboost-dev libssl-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j && ./build/BATorrent
```

No macOS: `brew install qt libtorrent-rasterbar boost openssl`.
No Windows: o instalador do Qt mais `vcpkg install libtorrent:x64-windows`.

</details>

<details>
<summary><b>Qualidade e segurança</b></summary>

<p>
  <a href="https://github.com/BATorrent-app/BATorrent/actions/workflows/codeql.yml"><img alt="CodeQL" src="https://github.com/BATorrent-app/BATorrent/actions/workflows/codeql.yml/badge.svg"></a>
  <a href="https://github.com/BATorrent-app/BATorrent/actions/workflows/sanitizers.yml"><img alt="Sanitizers" src="https://github.com/BATorrent-app/BATorrent/actions/workflows/sanitizers.yml/badge.svg"></a>
  <a href="https://sonarcloud.io/summary/new_code?id=Mateuscruz19_BAT-Torrent"><img alt="Quality Gate" src="https://sonarcloud.io/api/project_badges/measure?project=Mateuscruz19_BAT-Torrent&metric=alert_status"></a>
  <a href="https://www.codefactor.io/repository/github/mateuscruz19/batorrent"><img alt="CodeFactor" src="https://www.codefactor.io/repository/github/mateuscruz19/batorrent/badge"></a>
  <a href="https://www.bestpractices.dev/projects/13073"><img alt="OpenSSF Best Practices" src="https://www.bestpractices.dev/projects/13073/badge"></a>
</p>

- Uma suíte Catch2 (unidade, segurança, memória) e os testes de QML rodam em todo build de CI.
- O build passa no AddressSanitizer e no UndefinedBehaviorSanitizer.
- Os segredos ficam no chaveiro do sistema, e a WebUI só se abre para a rede depois que você define uma senha.

</details>

## Por que eu fiz

Sou um desenvolvedor sozinho, no Brasil. Eu queria um cliente de torrent que levasse privacidade a sério, rodasse de forma nativa no Windows, no macOS e no Linux, e não parecesse ter sido feito em 2009. Não achei nenhum que me agradasse, então escrevi o meu. Ele é licenciado sob a MIT, então se um dia ele passasse a ter anúncios ou rastreamento, qualquer pessoa poderia fazer um fork e distribuir sem isso.

## Contribuindo

Issues e pull requests são bem-vindos. Para qualquer coisa maior que uma correção pequena, abra uma issue primeiro para combinarmos a abordagem. Relatos de bug ajudam mais com a sua plataforma, a versão (em `Ajuda → Sobre`) e os passos para reproduzir. Traduções são especialmente bem-vindas. Se você usa ferramentas de IA, leia antes [como eu uso IA](USING_AI.md).

## Licença e marca

O **código** é [MIT](LICENSE), © 2024-2026 Mateus Cruz. Você pode fazer fork e distribuir o seu próprio build.

O **nome "BATorrent" e o logo** não fazem parte da licença do código. Se você distribuir um fork, por favor dê a ele um nome próprio, para as pessoas saberem qual é o build oficial. Os detalhes estão em [TRADEMARK.md](TRADEMARK.md).

<p align="center"><sub>Feito no Brasil.</sub></p>
