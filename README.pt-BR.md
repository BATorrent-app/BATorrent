<p align="center">
  <a href="README.md">English</a> | <b>Português</b> | <a href="README.zh-CN.md">中文</a> | <a href="README.ja.md">日本語</a> | <a href="README.ru.md">Русский</a> | <a href="README.es.md">Español</a> | <a href="README.de.md">Deutsch</a> | <a href="README.ua.md">Українська</a>
</p>

<p align="center">
  <img src="src/images/logo.svg" alt="BATorrent" width="140">
</p>

<h1 align="center">BATorrent</h1>

<p align="center">
  <i>Um cliente BitTorrent que mostra seus downloads como capas, não como linhas de planilha.</i>
</p>

<p align="center">
  <a href="https://github.com/BATorrent-app/BATorrent/releases/latest"><img alt="Release" src="https://img.shields.io/github/v/release/BATorrent-app/BATorrent?style=flat-square&color=dc2626"></a>
  <a href="https://github.com/BATorrent-app/BATorrent/releases"><img alt="Downloads" src="https://img.shields.io/github/downloads/BATorrent-app/BATorrent/total?style=flat-square&color=dc2626"></a>
  <a href="LICENSE"><img alt="License" src="https://img.shields.io/github/license/BATorrent-app/BATorrent?style=flat-square&color=dc2626"></a>
  <img alt="Plataformas" src="https://img.shields.io/badge/Windows%20·%20macOS%20·%20Linux-dc2626?style=flat-square">
  <a href="https://apps.microsoft.com/detail/9n4l3tq24rc6"><img alt="Microsoft Store" src="https://img.shields.io/badge/Microsoft%20Store-baixar-dc2626?style=flat-square&logo=microsoft"></a>
</p>

<p align="center">
  <img src="src/images/shot-grid-v43.jpg" alt="BATorrent — seus downloads como uma parede de capas" width="860">
</p>

O BATorrent é um cliente de torrent para desktop feito sobre o motor [libtorrent](https://www.libtorrent.org/), o mesmo que o qBittorrent e o Deluge usam. A interface lê o nome de cada torrent, busca o pôster correspondente (filmes e séries no TMDB, jogos no IGDB) e organiza seus downloads numa grade de capas, em vez de uma lista de nomes de arquivo. As capas ficam em cima de um cliente que aguenta o tranco, e sobre um motor que foi [ajustado, não apenas usado de fábrica](#o-motor).

É gratuito e de código aberto. Sem anúncios, sem telemetria, sem versão "Pro", sem conta. A única requisição que ele faz por conta própria é a verificação de atualização no GitHub, e existe uma opção para desligar isso. Se quiser conferir, o código está em [`updater.cpp`](src/services/integrations/updater.cpp).

## Por que eu fiz isso

Sou um desenvolvedor sozinho, no Brasil. Eu queria um cliente de torrent que levasse privacidade a sério, rodasse de forma nativa no Windows, no macOS e no Linux, e que não parecesse ter sido desenhado em 2009. Como não achei nenhum que me agradasse, escrevi o meu. Ele é licenciado sob a MIT, o que significa que nenhuma telemetria pode ser enfiada mais tarde e ninguém pode comprar o projeto e grudar anúncios nele. A interface vem em nove idiomas, porque "útil" não deveria significar "só em inglês".

## A interface

<p align="center">
  <img src="src/images/shot-list.jpg" alt="Modo lista denso, para quando você quer detalhe em vez de decoração" width="860">
</p>

<p align="center">
  <img src="src/images/shot-palette-v43.jpg" alt="Paleta de comandos (Ctrl/⌘+K): encontre qualquer torrent ou ação" width="860">
</p>

<p align="center">
  <img src="src/images/shot-theme.jpg" alt="Sakura, um dos temas embutidos" width="860">
</p>

- **Capas automáticas.** Ele identifica o pôster a partir do nome do torrent e o exibe numa grade. Um clique alterna para uma lista densa quando você quer detalhe em vez de decoração.
- **Seis temas.** Dark, Light, Midnight, Sakura, Dark Star e um tema Custom em que você escolhe o próprio fundo e a cor de destaque. Cada um aceita arte de destaque de anime, opcional.
- **Paleta de comandos.** O Ctrl/⌘+K abre uma busca rápida por qualquer torrent ou ação: pausar tudo, alternar a velocidade alternativa, ir para qualquer página, sem precisar do mouse.
- **Status ao vivo.** Um gráfico de velocidade em tempo real, barras de progresso coloridas por estado e um popup na bandeja com as velocidades atuais e o tempo restante.

## O que ele faz

**Assista dentro do app.** Tem um player de vídeo embutido (com FFmpeg, então toca MKV, AVI e WebM direto) e você pode começar a assistir enquanto o arquivo ainda baixa, ele só busca o começo primeiro. Ele procura e baixa legendas para você (pelo SubDL), carrega automaticamente arquivos `.srt`/`.vtt` ao lado do vídeo e deixa você ajustar a sincronia ao vivo. Ao concluir, pode atualizar uma biblioteca do Plex, Jellyfin ou Emby.

**Reprodução instantânea com debrid.** Conecte uma conta [Real-Debrid](https://real-debrid.com) ou [TorBox](https://torbox.app) e, quando um magnet já está em cache no lado deles, o BATorrent libera o link e transmite direto no player embutido, sem baixar nem semear localmente.

**Jogos, não só vídeo.** Torrents de jogos também ganham capa (pelo IGDB). Busque em catálogos de jogos, baixe e depois instale e abra de dentro do app, para que sua biblioteca pirata se comporte um pouco como uma lista da Steam, em vez de uma pasta cheia de instaladores.

**Descobrir.** Uma página inicial navegável no estilo Netflix (pôsteres em alta, um destaque rotativo) para achar algo para baixar sem sair do app.

<p align="center">
  <img src="src/images/shot-discover.jpg" alt="Descobrir — uma página inicial navegável de pôsteres em alta" width="860">
</p>

**Privacidade.** Vincule a uma interface de VPN específica, com um kill switch que corta todo o tráfego se o túnel cair. Modo para trackers privados, preset para Tor, handshake anônimo e bloqueio de clientes sanguessuga (leechers). Há um teste de vazamento de IP embutido para confirmar que está funcionando.

**Buscar e adicionar.** Busca embutida (incluindo fontes abertas da CIS/RuTor, sem login), Smart Paste que reconhece um magnet, um `.torrent`, um link `thunder://` ou um info hash no Ctrl+V, download automático por RSS com filtros regex, uma pasta monitorada, e arrastar e soltar.

<p align="center">
  <img src="src/images/shot-search.jpg" alt="Busca embutida — capas, notas e o melhor resultado na hora" width="860">
</p>

**Controle remoto.** Uma WebUI no navegador com pareamento por QR: escaneie o código pelo celular em vez de digitar endereços de IP. O QR é gerado na sua máquina e o endereço nunca sai dela.

**Organizar.** Extraia arquivos compactados automaticamente ao concluir, organize com categorias e tags, defina limites de proporção e tempo por torrent e globais, e agende a banda por hora e dia.

**Notificações.** Alertas nativos do sistema, mensagens no Telegram e Rich Presence no Discord.

<details>
<summary><b>Lista completa de recursos</b></summary>

Prioridade por arquivo, download sequencial, injeção automática de trackers, controle de layout de conteúdo, regex para excluir arquivos, pasta temporária de download separada, um estado "concluído" com janelas de seed, pausa automática em erros de arquivo, limites globais e por torrent de proporção e tempo, agendador de banda por hora e dia, importação do qBittorrent, criação de arquivos `.torrent`, um inspetor de torrent, listas de bloqueio de IP, criptografia de protocolo, espelho de atualização no Gitee, desligar o computador ao terminar, um ajudante de exclusão no Windows Defender, backup e restauração completos, histórico de itens removidos recentemente, início forçado, um visualizador de log embutido com diagnósticos e teste de vazamento de IP, formatação de acordo com o idioma, e atalhos de teclado.

</details>

## O motor

A maioria dos apps de torrent usa o libtorrent de fábrica. O BATorrent traz um pequeno fork com patches dele, o que permite mudar comportamentos do motor que a API pública não alcança:

- **Ramp de pipeline mais rápido.** Num link de banda larga e latência alta, o pipeline de requisições padrão cresce de um em um; o fork cresce de forma geométrica, então enche um "cano gordo" numa fração das idas e voltas. Medido em cerca de +27% num link rápido no benchmark A/B do próprio projeto, sem os travamentos de execução para execução do padrão, e nunca piora.
- **Preferência por peers do mesmo país.** Um banco GeoIP offline (db-ip Lite) marca cada peer por país, e o ranqueamento de peers do fork prefere peers do seu próprio país quando pode escolher, o que costuma significar menor latência e menos rotas transfronteiriças estranguladas.

Os dois são recursos de tempo de compilação do fork, desligados num build padrão, e aplicados como patches versionados em [`third_party/patches/`](third_party/patches), em vez de uma cópia vendorizada.

## Instalação

| Plataforma | Download | Requisitos |
|---|---|---|
| **Windows** | [Microsoft Store](https://apps.microsoft.com/detail/9n4l3tq24rc6), [Instalador](https://batorrent.com/win) ou [Portátil](https://batorrent.com/portable) | Windows 10 ou mais recente |
| **macOS** | `brew install --cask Mateuscruz19/batorrent/batorrent` ou o [`.dmg`](https://batorrent.com/mac) | macOS 12+, Apple Silicon |
| **Linux** | [AppImage](https://batorrent.com/linux) | glibc 2.35+ |

Com o programa aberto, arraste um arquivo `.torrent` ou um link magnet para a janela.

<sub><b>Nota sobre o macOS:</b> o app ainda não é notarizado (o programa de desenvolvedor da Apple é uma assinatura paga). O Homebrew é o caminho mais tranquilo porque o <code>brew</code> remove a flag de quarentena, então ele abre sem o aviso do Gatekeeper. Se preferir o <code>.dmg</code>, clique com o botão direito no app e escolha <b>Abrir</b> na primeira vez.</sub>

<details>
<summary><b>Compilar a partir do código-fonte</b></summary>

**Requisitos:** C++17, CMake 3.16+, Qt 6 (`Widgets`, `Network`, `Svg`, `Multimedia`), libtorrent-rasterbar 2.0+, Boost e, opcionalmente, Qt6Keychain.

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

- Uma suíte de testes Catch2 (unidade, segurança, memória) roda a cada build de CI; todo comportamento novo do backend vem com um teste.
- O build passa limpo no AddressSanitizer e no UndefinedBehaviorSanitizer.
- Antes de cada release o código é revisado quanto a segurança de memória e de threads, autenticação da WebUI, injeção, path traversal, validação de entrada e tratamento de segredos. Os segredos ficam no chaveiro do sistema, nunca em texto puro, e a WebUI só se abre para a rede depois que você define uma senha.

</details>

## Contribuindo

Issues e pull requests são bem-vindos. Para qualquer coisa não trivial, abra uma issue primeiro para combinarmos a abordagem. Relatos de bug ajudam mais com a sua plataforma e versão (em `Ajuda → Sobre`) e os passos para reproduzir. Traduções são especialmente bem-vindas.

## Licença e marca

O **código** é [MIT](LICENSE), © 2024–2026 Mateus Cruz. Faça fork, construa em cima, distribua.

O **nome "BATorrent" e o logo** são a identidade do projeto e não fazem parte da licença do código. Se você redistribuir um fork, por favor dê a ele um nome próprio, para que os usuários saibam qual é o build oficial. Os detalhes estão em [TRADEMARK.md](TRADEMARK.md). Forks e contribuições de boa-fé são bem-vindos.

Feito no Brasil.
