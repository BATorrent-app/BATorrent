# BATorrent — Plano de Divulgação v2.5.1

## Contexto

BATorrent é um cliente BitTorrent 100% gratuito, open source (MIT), desenvolvido por um brasileiro. O app suporta 7 idiomas (EN/PT/ZH/JA/RU/ES/DE) e foi construído do zero em C++/Qt 6/libtorrent-rasterbar. Roda em Windows, macOS e Linux.

A v2.5.1 inclui features específicas para o mercado chinês (anti-Xunlei, thunder:// links, Gitee mirror, PT mode) e russo (PT mode, Tor preset, Telegram webhook, VPN kill switch).

O objetivo é alcançar 500+ GitHub stars e 3000+ downloads nas próximas 4 semanas.

---

## Cronograma (4 semanas)

### Semana -1 (PREPARAÇÃO — antes de postar qualquer coisa)

**Aquecer contas:** postar comentários genuínos (NÃO promocionais) nos fóruns/subs onde vai divulgar. Contas com histórico de 2+ semanas de atividade genuína têm posts aceitos; contas novas são removidas automaticamente.

| Plataforma | Ação de aquecimento | Link |
|---|---|---|
| r/torrenting | Comentar em 5+ posts sobre clientes/VPN/privacy | https://reddit.com/r/torrenting |
| r/opensource | Comentar em 3+ posts sobre projetos C++/Qt | https://reddit.com/r/opensource |
| r/linux | Comentar em 3+ posts sobre apps nativos | https://reddit.com/r/linux |
| r/selfhosted | Comentar em 3+ posts sobre WebUI/docker | https://reddit.com/r/selfhosted |
| r/privacy | Comentar em 3+ posts sobre VPN/kill switch | https://reddit.com/r/privacy |
| Habr.com | Criar conta, ler artigos, deixar 5+ comentários construtivos | https://habr.com/ru/ |
| V2EX | Registrar via GitHub OAuth, postar em 2-3 threads técnicas | https://v2ex.com/ |
| Pikabu | Criar conta, upvotar, comentar em 3+ posts | https://pikabu.ru/ |

**Outras preparações:**
- [ ] Criar conta no 52pojie.cn (requer acumular reputation points antes de postar na Original Release)
- [ ] Criar mirror no Gitee: https://gitee.com/ (se ainda não feito)
- [ ] Submeter ao OSCHINA software catalog: https://www.oschina.net/
- [ ] Submeter ao AlternativeTo: https://alternativeto.net/ (menu do usuário → Suggest new application, listar como alternativa a qBittorrent/Transmission/Deluge/uTorrent)
- [ ] Tirar 4-6 screenshots atualizados (Dark + Light theme, Settings, Diagnostics, Torrent Inspector, QR Pairing)
- [ ] Gravar GIF de 15s mostrando: Smart Paste → magnet adicionado → download inicia → Discord RPC atualiza

---

### Semana 1 — Lançamento principal

**Dia 1 (Terça, 14:00 UTC):** Reddit EN — r/opensource + r/torrenting

**r/torrenting** — https://reddit.com/r/torrenting

Título:
```
I'm a solo dev from Brazil and I built a free, open-source torrent client with VPN kill switch, anti-Xunlei blocking, Tor, Telegram alerts, and Discord activity — no ads, no telemetry, 7 languages
```

Post body:
```
I wanted a torrent client that takes privacy seriously without sacrificing features, so I built one from scratch in C++ and Qt 6. It's called BATorrent, it's 100% free and open source (MIT), and I've been pouring every spare hour into it for months.

I'm from Brazil and I've been working hard to make this useful not just for English speakers but for communities worldwide — there's full support for Chinese (Simplified), Japanese, Russian, Spanish, German, and Portuguese. The app auto-detects your system language on first launch.

**What sets it apart:**

Privacy & VPN:
- VPN kill switch — auto-pauses every torrent if your VPN interface drops
- PT mode — one toggle for private tracker compliance (disables DHT/PEX/LSD, anonymous handshake)
- Tor proxy preset — one click fills SOCKS5 127.0.0.1:9050
- Anonymous mode — hides client ID in handshake
- No telemetry, no analytics, no calls home

Anti-leecher:
- Blocks Xunlei/Thunder, QQDownload, Baidu peers that download without seeding back
- Detects by peer_id prefix, auto-bans via IP filter

Integrations:
- Telegram notifications — download complete, kill switch, RSS, errors → straight to your chat
- Discord Rich Presence — shows download status on your profile with "Download BATorrent" button
- WebUI with QR code pairing — scan from your phone, no IP typing
- RSS auto-download with Nyaa anime presets built in

Power tools:
- Smart Paste (Ctrl+V detects magnets + thunder:// links)
- Torrent inspector (preview .torrent metadata before adding)
- Multi-tag system, Force Start, Recently Removed history
- Full backup/restore of all settings + torrents
- Log viewer with export for bug reports

Runs on Windows, macOS, and Linux. ~40 source files, MIT licensed.

GitHub: https://github.com/Mateuscruz19/BAT-Torrent
Download: https://github.com/Mateuscruz19/BAT-Torrent/releases/latest

I'd love to hear what you think. Feature requests, bug reports, and criticism all welcome — this is how the app got to where it is.
```

---

**r/opensource** — https://reddit.com/r/opensource

Título:
```
BATorrent — open-source BitTorrent client with VPN kill switch, anti-leecher blocking, Tor, WebUI, Discord RPC, and 7 languages (C++/Qt6/libtorrent, MIT)
```

Post: versão mais técnica do mesmo conteúdo, focando no stack e convite pra contribuir traduções.

---

**Dia 3 (Quinta, 14:00 UTC):** Hacker News

**Show HN** — https://news.ycombinator.com/submit

Título:
```
Show HN: BATorrent – Open-source torrent client with VPN kill switch and anti-Xunlei (C++/Qt6)
```

Link: `https://github.com/Mateuscruz19/BAT-Torrent`

Primeiro comentário (imediato):
```
Hi HN, I'm a developer from Brazil. I built BATorrent because I wanted a torrent client where VPN isolation isn't an afterthought.

The core idea: you bind torrent traffic to a network interface, and the kill switch auto-pauses everything if that interface drops. No traffic ever leaks outside your tunnel. Combined with Tor proxy and anonymous mode, it's designed for users who care about where their packets go.

A few technical decisions I'm happy to discuss:

- Pure C++ QR code encoder (~700 LoC, ISO/IEC 18004) for the mobile WebUI pairing — no external service, so the LAN address never leaves the machine
- Discord Rich Presence via raw IPC (no SDK) — just a local socket handshake + JSON frames
- Anti-leecher blocking detects Xunlei/QQDownload peers by peer_id prefix and auto-bans via libtorrent's IP filter. This is the #1 requested feature in Chinese BT communities
- 7-language i18n with 622 keys and English fallback, all in a single translator.cpp

Stack: C++17, Qt 6 Widgets, libtorrent-rasterbar 2.x. ~40 source files, MIT licensed.
```

REGRA: responder CADA comentário nas primeiras 2 horas.

---

**Dia 4-5:** Rússia

**RuTracker** — https://rutracker.org/forum/viewforum.php?f=1649

Post em BB-code, obrigatoriamente em russo. Usar o template já preparado (ver seção anterior). Título: `BATorrent v2.5.1 — бесплатный торрент-клиент с VPN kill switch и блокировкой Xunlei`.

**Habr Sandbox** — https://habr.com/ru/sandbox/start/

Artigo em russo (~1500 palavras). Estrutura:
1. Проблема (uTorrent/Xunlei — реклама/майнеры; qBittorrent — нет VPN/Tor интеграции)
2. Решение (BATorrent + features)
3. Сравнение (tabela BATorrent vs qBittorrent vs Transmission)
4. Скриншоты
5. Ссылки

Hubs: "Open Source", "Information Security", "Software"

IMPORTANTE: Habr proíbe conteúdo AI-generated. Escrever/editar pessoalmente.

**Pikabu** — https://pikabu.ru/

Post casual com screenshots. Tom: "Привет, я разработчик из Бразилии и создал торрент-клиент..." (pessoal, não corporativo).

---

**Dia 5-6:** China

**52pojie.cn** — https://www.52pojie.cn/forum-2-1.html (Original Release)

Usar o template chinês já preparado. HEADLINE: "支持屏蔽迅雷吸血" (Suporta bloqueio de Xunlei vampiro). Essa frase sozinha viraliza.

IMPORTANTE: incluir disclaimer "本工具仅用于合法文件分享" (ferramenta apenas para compartilhamento legal).
IMPORTANTE: NÃO mencionar VPN/翻墙. Focar em: anti-Xunlei + PT mode + Thunder:// links.

**V2EX** — https://v2ex.com/go/share

Post curto e direto. Angle: "来自巴西的开发者，为中国用户做了这些：屏蔽迅雷吸血、支持thunder://链接、Gitee更新源、完整中文界面" (Dev brasileiro que fez isso pro público chinês: bloqueia Xunlei, suporta thunder://, Gitee mirror, interface ZH completa).

---

### Semana 2 — Expansão

**Dia 8 (Terça):** Reddit EN round 2

- r/linux — angle: native Qt6, AppImage, no Electron
- r/selfhosted — angle: WebUI + QR pairing + RSS + Telegram
- r/privacy — angle: VPN kill switch + Tor + no telemetry + anonymous mode

**Dia 10:** Listagens

- AlternativeTo (se não feito na semana -1)
- Product Hunt — lançar terça/quarta 00:01 PT
- OSCHINA catalog (cross-promo com Gitee)

**Dia 12-14:** Consolidação

- Zhihu — responder pergunta existente "BT下载软件哪个好用？": https://www.zhihu.com/question/309769458
- CSDN Blog — post tutorial pra SEO Baidu
- 4PDA — https://4pda.to/forum/index.php?showforum=285
- Appinn — https://meta.appinn.net/
- Telegram: postar em @open_source_friend (https://t.me/open_source_friend)

---

### Semana 3-4 — Conteúdo técnico + comunidade

- Dev.to artigo "How I built a torrent client in C++/Qt6" (~2000 palavras)
- Bilibili vídeo demo (2-3 min, em chinês, com legendas)
- YouTube vídeo demo (em inglês + legendas PT)
- Criar canal Telegram @batorrent_updates pra releases
- Se HN viralizar → GitHub Trending segue automaticamente (star velocity)

---

## Pontos fortes pra cada público

### Todos os mercados
- 100% gratuito — sem tiers, sem premium, sem ads
- Open source MIT — código auditável
- Desenvolvido por um brasileiro independente — sem corporação, sem agenda oculta
- 7 idiomas com auto-detect

### China (CN)
- **HEADLINE: 屏蔽迅雷/QQ下载吸血** (bloqueia leechers Xunlei/QQ)
- Suporte a links thunder:// (Smart Paste decoda automaticamente)
- PT mode pra private trackers chineses (M-Team, HDChina, OurBits)
- Gitee como update mirror (GitHub lento/instável na China)
- Interface 100% em chinês simplificado
- Sem bundleware/spyware (contraste direto com Xunlei)
- NÃO mencionar VPN/翻墙 nos posts

### Rússia (RU)
- PT mode pra trackers privados (RuTracker private, etc.)
- VPN kill switch + Tor proxy preset
- Telegram webhook pra notificações
- Interface 100% em russo
- Sem crypto miners (contraste com uTorrent)
- Developer independente, sem corporação americana

### Japão (JP)
- Nyaa RSS presets (anime, English-subbed, search template)
- Anonymous mode (privacidade cultural forte no JP)
- No telemetry — statement explícito (comunidade JP espera isso escrito)
- Interface 100% em japonês
- Sequential download pra streaming de anime

### Internacional (EN)
- Discord Rich Presence com botões de download (marketing orgânico viral)
- Smart Paste (workflow killer — Ctrl+V com magnet)
- WebUI + QR pairing (self-hosted angle)
- C++/Qt6 nativo — não é Electron

---

## Tom dos posts

| Plataforma | Tom | Idioma |
|---|---|---|
| Reddit | Pessoal, "I built", transparente | Inglês |
| Hacker News | Técnico, trade-offs, "Show HN" | Inglês |
| RuTracker | BB-code formal, changelog completo | Russo |
| Habr | Artigo técnico, comparação detalhada | Russo |
| Pikabu | Casual, screenshots, "привет, я из Бразилии" | Russo |
| 52pojie | Formato rigoroso (Original Release), screenshots, disclaimer | Chinês simplificado |
| V2EX | Curto, direto, "share & create" | Chinês simplificado |
| Zhihu | Resposta detalhada a pergunta existente | Chinês simplificado |

---

## Angle brasileiro nos posts

Para cada plataforma (adaptado ao idioma):
- "I'm a solo developer from Brazil"
- "100% free — no premium tiers, no subscriptions. I built this because I believe good software should be accessible to everyone"
- "I worked hard to support [language] because I want BATorrent to be genuinely useful for [country] users, not just a translated English app"

Isso humaniza o projeto (solo dev vs corporação), sinaliza que é genuinamente grátis (não é modelo freemium disfarçado), e demonstra respeito pelo público local (não é Google Translate jogado).

---

## Regras de ouro

1. NÃO postar tudo no mesmo dia — filtro de spam
2. NÃO usar linguagem de marketing ("revolutionary", "game-changing")
3. NÃO pedir upvotes, stars, ou downloads
4. Responder CADA comentário nas primeiras 2 horas
5. Screenshots/GIFs obrigatórias em TODO post
6. Cada sub/forum precisa de conta com histórico genuíno
7. China: NUNCA mencionar 翻墙/VPN bypass do GFW — risco legal real
8. Rússia: SEMPRE em russo, tom técnico mas acessível
9. HN: primeiro comentário imediato explicando decisões técnicas
10. Staggering de 2-3 dias entre posts em plataformas diferentes

---

## Métricas de sucesso (4 semanas)

| Métrica | Meta |
|---|---|
| GitHub stars | 500+ (de ~43 atual) |
| Downloads totais | 3000+ |
| Forks | 10+ |
| Issues abertas (feature requests) | 20+ |
| Contribuidores externos | 3+ |

---

## Links de referência

### Reddit
- r/torrenting: https://reddit.com/r/torrenting
- r/opensource: https://reddit.com/r/opensource
- r/linux: https://reddit.com/r/linux
- r/selfhosted: https://reddit.com/r/selfhosted
- r/privacy: https://reddit.com/r/privacy
- r/software: https://reddit.com/r/software

### Hacker News
- Submit: https://news.ycombinator.com/submit

### Rússia
- RuTracker (torrent clients): https://rutracker.org/forum/viewforum.php?f=1649
- Habr Sandbox: https://habr.com/ru/sandbox/start/
- 4PDA Software: https://4pda.to/forum/index.php?showforum=285
- Pikabu: https://pikabu.ru/
- Linux.org.ru: https://www.linux.org.ru/news/opensource/
- Telegram @open_source_friend: https://t.me/open_source_friend
- TGStat (buscar canais): https://tgstat.ru/en/apps

### China
- 52pojie.cn Original Release: https://www.52pojie.cn/forum-2-1.html
- V2EX Share: https://v2ex.com/go/share
- Zhihu BT question: https://www.zhihu.com/question/309769458
- OSCHINA: https://www.oschina.net/
- Appinn: https://meta.appinn.net/
- CSDN: https://blog.csdn.net/
- Baidu Tieba qBittorrent: https://tieba.baidu.com/f?kw=qbittorrent
- Baidu Tieba PT: https://tieba.baidu.com/f?kw=pt
- SegmentFault OSS: https://segmentfault.com/channel/oss

### Listagens
- AlternativeTo: https://alternativeto.net/ (menu → Suggest new application)
- Product Hunt: https://producthunt.com/
- Dev.to: https://dev.to/
- Lobste.rs: https://lobste.rs/ (invite-only)
