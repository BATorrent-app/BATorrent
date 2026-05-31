// Source: BATorrent Settings.html + batorrent-settings.css (+ settings-data.js)
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

Window {
    id: win
    width: 884
    height: 624
    minimumWidth: 740
    minimumHeight: 480
    color: Theme.bg
    title: "Preferências"

    property int sec: 0

    // ---- nav metadata ----
    readonly property var navs: [
        { nav: "Geral",               icon: "qrc:/icons/set-general.svg" },
        { nav: "Velocidade",          icon: "qrc:/icons/set-speed.svg" },
        { nav: "Rede",                icon: "qrc:/icons/set-network.svg" },
        { nav: "VPN / Kill Switch",   icon: "qrc:/icons/set-vpn.svg" },
        { nav: "Proxy & Filtro de IP",icon: "qrc:/icons/set-proxy.svg" },
        { nav: "WebUI & Pareamento",  icon: "qrc:/icons/set-webui.svg" },
        { nav: "Notificações",        icon: "qrc:/icons/set-notif.svg" },
        { nav: "Addons & Mídia",      icon: "qrc:/icons/set-addons.svg" },
        { nav: "Avançado",            icon: "qrc:/icons/set-advanced.svg" }
    ]

    readonly property var heads: [
        { eyebrow: "PREFERÊNCIAS", h: "Geral", sub: "Pastas de download, idioma, aparência e comportamento na bandeja do sistema." },
        { eyebrow: "PREFERÊNCIAS", h: "Limites de Velocidade", sub: "Limites globais e alternativos, regras de seeding e agendamento por horário." },
        { eyebrow: "PREFERÊNCIAS", h: "Rede", sub: "Porta de escuta, descoberta de peers, criptografia e privacidade do protocolo." },
        { eyebrow: "PREFERÊNCIAS", h: "VPN / Kill Switch", sub: "Vincule o tráfego a uma interface e proteja contra vazamentos fora da VPN." },
        { eyebrow: "PREFERÊNCIAS", h: "Proxy & Filtro de IP", sub: "Roteie peers e trackers por um proxy e descarte faixas de IP por blocklist." },
        { eyebrow: "PREFERÊNCIAS", h: "WebUI & Pareamento", sub: "Controle o BATorrent pelo navegador e pareie o celular na mesma rede." },
        { eyebrow: "PREFERÊNCIAS", h: "Notificações", sub: "Receba avisos via Telegram, mostre atividade no Discord e ative sons." },
        { eyebrow: "PREFERÊNCIAS", h: "Addons & Mídia", sub: "Trackers automáticos, busca de torrents, servidores de mídia e extração." },
        { eyebrow: "PREFERÊNCIAS", h: "Avançado", sub: "Ajuste fino de I/O de disco, conexões, algoritmos de choke e APIs de metadados." }
    ]

    // ---- fields per section (settings-data.js, verbatim) ----
    readonly property var sections: [
        // 0 Geral
        [
            { type: "group", label: "Downloads" },
            { type: "path", label: "Pasta padrão para salvar", value: "/Users/voce/Downloads/BATorrent" },
            { type: "toggle", label: "Sempre usar pasta padrão (pular diálogo de pasta)", on: true },
            { type: "path", label: "Caminho temporário de download", placeholder: "Baixar para esta pasta primeiro, mover ao concluir", note: "Downloads incompletos vão para cá primeiro, depois movem para o destino quando terminam." },
            { type: "toggle", label: "Mover downloads concluídos automaticamente" },
            { type: "path", label: "Mover para", placeholder: "Pasta de destino final" },
            { type: "group", label: "Aparência" },
            { type: "select", label: "Idioma", options: ["Português", "English", "Español", "日本語", "Русский", "中文", "Deutsch"], value: 0 },
            { type: "theme", label: "Tema", options: ["Escuro", "Claro", "Midnight", "Sakura", "Dark Star"], value: 0 },
            { type: "anime", label: "Modo Anime (arte de fundo)" },
            { type: "group", label: "Sistema" },
            { type: "toggle", label: "Iniciar minimizado na bandeja" },
            { type: "toggle", label: "Fechar para a bandeja ao invés de sair", on: true },
            { type: "toggle", label: "Tocar som nas notificações", on: true },
            { type: "toggle", label: "Tocar som de morcego ao iniciar" },
            { type: "button", label: "Aplicativo padrão", btn: "Definir BATorrent como app padrão de torrent" }
        ],
        // 1 Velocidade
        [
            { type: "group", label: "Limites globais" },
            { type: "number", key: "downloadLimit", label: "Máximo de download", value: "0", suffix: "KB/s", note: "0 = ilimitado" },
            { type: "number", key: "uploadLimit", label: "Máximo de upload", value: "200", suffix: "KB/s", note: "0 = ilimitado" },
            { type: "number", key: "maxActiveDownloads", label: "Downloads ativos simultâneos", value: "5", note: "0 = ilimitado" },
            { type: "group", label: "Seeding" },
            { type: "number", key: "seedRatioLimit", label: "Parar de semear na proporção", value: "2.0", note: "Pausa o seeding quando upload ÷ download alcança essa razão. 0 = sem limite." },
            { type: "number", key: "maxSeedDays", label: "Parar de semear após", value: "0", suffix: "dias" },
            { type: "toggle", key: "stopAfterDownload", label: "Parar de semear ao concluir o download" },
            { type: "select", label: "Marcar como Concluído após", options: ["Nunca", "1 dia", "3 dias", "7 dias", "14 dias", "30 dias"], value: 0, note: "Depois desse tempo contínuo de seeding, marca o torrent como Concluído (verde, pausado)." },
            { type: "group", label: "Agendamento de Velocidade" },
            { type: "toggle", key: "schedulerEnabled", label: "Habilitar agendamento de velocidade", note: "Troca para os limites alternativos em uma janela de horário e dias configurados." },
            { type: "number", key: "altDownloadLimit", label: "Limite alt. download", value: "50", suffix: "KB/s" },
            { type: "number", key: "altUploadLimit", label: "Limite alt. upload", value: "20", suffix: "KB/s" },
            { type: "timerange", label: "Ativo das", from: "08:00", to: "18:00" },
            { type: "days", label: "Dias", value: [1,2,3,4,5] }
        ],
        // 2 Rede
        [
            { type: "group", label: "Conexão" },
            { type: "number", key: "listenPort", label: "Porta de escuta", value: "6881", note: "Conexões de peers chegam por aqui. Combine com a regra de port-forward do seu roteador." },
            { type: "toggle", key: "randomPort", label: "Usar porta aleatória a cada inicialização" },
            { type: "number", key: "maxConnections", label: "Máximo de conexões", value: "200" },
            { type: "group", label: "Protocolo" },
            { type: "toggle", key: "dhtEnabled", label: "Habilitar DHT (descoberta de peers sem tracker)", on: true, note: "Descobre peers sem precisar de tracker. Desabilite em trackers privados (BEP-27)." },
            { type: "toggle", key: "utpEnabled", label: "Habilitar µTP (transporte de peer via UDP)", on: true },
            { type: "segmented", key: "encryptionMode", label: "Criptografia do protocolo", options: ["Habilitada", "Forçada", "Desabilitada"], value: 0, note: "Forçado = só criptografado. Muitos provedores limitam BitTorrent em texto plano." },
            { type: "segmented", key: "speedUnit", label: "Exibir velocidades em", options: ["Bytes (KB/s)", "Bits (Kbps)"], value: 0 },
            { type: "group", label: "Privacidade" },
            { type: "toggle", key: "anonymousMode", label: "Modo anônimo (ocultar ID do cliente)", note: "Oculta nome/versão do cliente nos handshakes e desativa anúncios UPnP/NAT-PMP." },
            { type: "toggle", key: "forceIpv4", label: "Forçar somente IPv4" },
            { type: "toggle", key: "ptMode", label: "Modo PT (compatível com private trackers)", note: "Desliga DHT/PEX/LSD, força handshake anônimo e anuncia em todas as tiers." },
            { type: "toggle", key: "blockLeechers", label: "Bloquear clientes vampiros (Xunlei, QQDownload, Baidu)", on: true }
        ],
        // 3 VPN
        [
            { type: "group", label: "Vínculo de Interface" },
            { type: "select", label: "Interface de rede", options: ["Qualquer (padrão)", "en0 — 192.168.0.12", "utun4 — 10.2.0.2 (VPN)"], value: 2 },
            { type: "toggle", key: "killSwitchEnabled", label: "Pausar torrents se a interface cair (Kill Switch)", on: true, note: "Pausa todo torrent ativo no instante em que a interface ligada cair." },
            { type: "toggle", key: "autoResumeOnReconnect", label: "Retomar automaticamente quando a interface voltar", on: true, note: "Retoma apenas os torrents que o kill switch pausou — não os que você pausou manualmente." },
            { type: "toggle", label: "Usar Tor (SOCKS5 127.0.0.1:9050)" },
            { type: "group", label: "Energia" },
            { type: "toggle", label: "Desligar PC quando todos os downloads terminarem" }
        ],
        // 4 Proxy
        [
            { type: "group", label: "Proxy" },
            { type: "select", key: "proxyType", label: "Tipo de proxy", options: ["Nenhum", "SOCKS5", "HTTP"], value: 0, note: "Roteia todo o tráfego de peers + trackers através de proxy SOCKS5 ou HTTP." },
            { type: "text", key: "proxyHost", label: "Host", mono: true, placeholder: "127.0.0.1", w: "w-md" },
            { type: "number", key: "proxyPort", label: "Porta", value: "1080" },
            { type: "text", key: "proxyUser", label: "Usuário", placeholder: "Opcional", w: "w-md" },
            { type: "password", key: "proxyPass", label: "Senha", w: "w-md" },
            { type: "group", label: "Filtragem de IP" },
            { type: "path", label: "Arquivo de blocklist", placeholder: "Blocklist P2P (.txt, .p2p, .dat)", note: "Faixas de IP carregadas são descartadas antes do handshake." }
        ],
        // 5 WebUI
        [
            { type: "group", label: "Servidor Web" },
            { type: "toggle", label: "Habilitar WebUI" },
            { type: "number", label: "Porta", value: "8080" },
            { type: "text", label: "Usuário", value: "admin", w: "w-md" },
            { type: "password", label: "Senha", placeholder: "Deixe vazio para manter a atual", w: "w-md" },
            { type: "toggle", label: "Permitir acesso remoto (bind 0.0.0.0)" },
            { type: "warning", text: "Habilitar acesso remoto expõe a WebUI para sua rede. Use uma VPN ou proxy reverso com HTTPS para acesso remoto seguro." },
            { type: "group", label: "Mobile" },
            { type: "button", label: "Parear celular", btn: "Parear celular…", note: "Mostra uma URL LAN copiável pra você abrir a WebUI no celular." }
        ],
        // 6 Notificações
        [
            { type: "group", label: "Telegram" },
            { type: "text", label: "Token do bot", mono: true, placeholder: "123456:ABC-DEF…", w: "grow", note: "Crie um bot via @BotFather no Telegram e cole o HTTP API token aqui." },
            { type: "text", label: "Chat ID", mono: true, placeholder: "@seucanal ou ID numérico", w: "w-md" },
            { type: "toggle", label: "Notificar ao concluir download", on: true },
            { type: "toggle", label: "Notificar ao acionar kill switch" },
            { type: "toggle", label: "Notificar em auto-download RSS" },
            { type: "toggle", label: "Notificar em erro de torrent" },
            { type: "button", label: "Teste", btn: "Enviar mensagem teste" },
            { type: "group", label: "Discord" },
            { type: "toggle", label: "Mostrar atividade no perfil do Discord", note: "Discord Rich Presence — mostra seu status de download/seeding. Requer o app Discord rodando." },
            { type: "group", label: "Sistema" },
            { type: "toggle", label: "Tocar som nas notificações", on: true }
        ],
        // 7 Addons
        [
            { type: "group", label: "Trackers" },
            { type: "toggle", label: "Adicionar trackers públicos automaticamente em novos torrents", on: true, badge: "1.243 carregados" },
            { type: "group", label: "Busca de Torrents" },
            { type: "toggle", label: "Habilitar busca de torrents" },
            { type: "text", label: "URL da API", mono: true, placeholder: "URL de uma API de busca compatível", w: "grow" },
            { type: "group", label: "Servidor de Mídia" },
            { type: "toggle", label: "Notificar Plex ao concluir download" },
            { type: "toggle", label: "Notificar Jellyfin / Emby ao concluir download" },
            { type: "text", label: "Chave API", mono: true, w: "w-md" },
            { type: "group", label: "Extração" },
            { type: "toggle", label: "Extrair arquivos automaticamente (RAR/ZIP/7z) ao concluir", note: "Requer 7-Zip (Windows) ou unrar/unzip (macOS/Linux) instalado." },
            { type: "toggle", label: "Excluir arquivos compactados após extração" },
            { type: "text", label: "Senhas de arquivos", placeholder: "senha1; senha2; online-fix.me", w: "grow" }
        ],
        // 8 Avançado
        [
            { type: "group", label: "E/S de Disco" },
            { type: "number", label: "Threads de I/O assíncrono", value: "4" },
            { type: "number", label: "Threads de hash", value: "2" },
            { type: "number", label: "Tamanho do pool de arquivos", value: "40" },
            { type: "number", label: "Memória de verificação", value: "512", suffix: "×16 KiB" },
            { type: "number", label: "Buffer de envio", value: "500", suffix: "KiB" },
            { type: "group", label: "Conexões" },
            { type: "number", label: "Limite global de conexões", value: "200" },
            { type: "number", label: "Velocidade de conexão", value: "30", suffix: "/s" },
            { type: "number", label: "Slots de unchoke", value: "8" },
            { type: "number", label: "Máx. uploads por torrent", value: "4" },
            { type: "number", label: "Máx. conexões por torrent", value: "60" },
            { type: "group", label: "Algoritmos" },
            { type: "segmented", label: "Algoritmo de choke", options: ["Slots fixos", "Baseado em taxa"], value: 1 },
            { type: "select", label: "Choke de seed", options: ["Alternado", "Upload mais rápido", "Anti-leech"], value: 2 },
            { type: "toggle", label: "Incluir overhead IP nos limites de taxa" },
            { type: "toggle", label: "Isentar peers LAN dos limites de velocidade", on: true },
            { type: "group", label: "API de Metadados" },
            { type: "text", key: "tmdbApiKey", label: "TMDB API key", mono: true, w: "grow", note: "Key gratuita de themoviedb.org — posters de filmes/séries." },
            { type: "text", key: "igdbClientId", label: "IGDB Client ID", mono: true, w: "w-md" },
            { type: "text", key: "igdbClientSecret", label: "IGDB Client secret", mono: true, w: "w-md" },
            { type: "group", label: "Diagnóstico" },
            { type: "toggle", key: "verboseLogging", label: "Logs detalhados (nível Debug)", note: "Captura detalhes extras de eventos do libtorrent. Use ao reportar um bug; desligue depois." },
            { type: "text", label: "Executar ao concluir", mono: true, placeholder: "notify-send \"%N concluído\"", w: "grow" },
            { type: "path", label: "Pasta monitorada", placeholder: "Pasta para monitorar arquivos .torrent" }
        ]
    ]

    // group consecutive fields into { label, rows[] } blocks (a "group" field starts a new card)
    function buildBlocks(fields) {
        var blocks = []
        var cur = null
        for (var i = 0; i < fields.length; i++) {
            var f = fields[i]
            if (f.type === "group") {
                cur = { label: f.label, rows: [] }
                blocks.push(cur)
            } else {
                if (!cur) { cur = { label: "", rows: [] }; blocks.push(cur) }
                cur.rows.push(f)
            }
        }
        return blocks
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ===== titlebar =====
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            color: Theme.elev
            Text { anchors.centerIn: parent; text: "Preferências"; color: Theme.t2; font.pointSize: 12.5; font.weight: Font.DemiBold; font.family: Theme.fontSans }
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
        }

        // ===== sbody =====
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // ----- sidebar (.sside 226) -----
            Rectangle {
                Layout.preferredWidth: 226
                Layout.fillHeight: true
                color: Theme.panel
                Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: Theme.hair }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.topMargin: Theme.sp3
                    anchors.bottomMargin: Theme.sp3
                    anchors.leftMargin: Theme.sp2
                    anchors.rightMargin: Theme.sp2
                    spacing: 0

                    // .ssearch
                    TFld {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 32
                        Layout.bottomMargin: Theme.sp3
                        icon: "qrc:/icons/search.svg"
                        placeholder: "Buscar configuração…"
                    }

                    // .snav
                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 1
                        model: win.navs
                        boundsBehavior: Flickable.StopAtBounds

                        delegate: Rectangle {
                            width: ListView.view.width
                            height: 34
                            radius: 7
                            color: win.sec === index ? Theme.sel : (nrMa.containsMouse ? Theme.hover : "transparent")

                            // .nrow.on::before left bar
                            Rectangle {
                                visible: win.sec === index
                                anchors.left: parent.left
                                anchors.leftMargin: -2
                                anchors.verticalCenter: parent.verticalCenter
                                width: 2
                                height: 20
                                radius: 1
                                color: Theme.accent
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: 10
                                spacing: Theme.sp3
                                IconImg {
                                    Layout.alignment: Qt.AlignVCenter
                                    src: modelData.icon
                                    tint: win.sec === index ? Theme.accent : Theme.t3
                                    s: 16
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.nav
                                    color: win.sec === index ? Theme.t1 : (nrMa.containsMouse ? Theme.t1 : Theme.t2)
                                    font.pointSize: 12.5
                                    font.family: Theme.fontSans
                                    elide: Text.ElideRight
                                }
                            }
                            MouseArea { id: nrMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: win.sec = index }
                        }
                    }
                }
            }

            // ----- content (.scontent) -----
            Flickable {
                id: contentScroll
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                contentWidth: width
                contentHeight: contentCol.implicitHeight + 2 * Theme.sp5
                boundsBehavior: Flickable.StopAtBounds

                ColumnLayout {
                    id: contentCol
                    x: Theme.sp5
                    y: Theme.sp5
                    width: contentScroll.width - 2 * Theme.sp5
                    spacing: 0

                    // .shead
                    Eyebrow { text: win.heads[win.sec].eyebrow }
                    Text {
                        Layout.topMargin: 6
                        text: win.heads[win.sec].h
                        color: Theme.t1
                        font.pointSize: 19
                        font.weight: Font.DemiBold
                        font.letterSpacing: -0.3
                        font.family: Theme.fontSans
                    }
                    Text {
                        Layout.topMargin: 8
                        Layout.maximumWidth: 540
                        Layout.bottomMargin: Theme.sp5
                        wrapMode: Text.WordWrap
                        text: win.heads[win.sec].sub
                        color: Theme.t3
                        font.pointSize: 12
                        font.family: Theme.fontSans
                        lineHeight: 1.5
                    }

                    // blocks (glabel + card)
                    Repeater {
                        model: win.buildBlocks(win.sections[win.sec])
                        delegate: ColumnLayout {
                            required property var modelData
                            required property int index
                            Layout.fillWidth: true
                            spacing: 0

                            Text {
                                visible: modelData.label !== ""
                                Layout.topMargin: index === 0 ? 0 : Theme.sp5
                                Layout.bottomMargin: Theme.sp2
                                Layout.leftMargin: 2
                                text: modelData.label
                                color: Theme.t4
                                font.pointSize: 10
                                font.weight: Font.Bold
                                font.letterSpacing: 0.8
                                font.family: Theme.fontSans
                                font.capitalization: Font.AllUppercase
                            }

                            // .card
                            Rectangle {
                                Layout.fillWidth: true
                                radius: 11
                                color: Theme.panel
                                border.color: Theme.hair
                                border.width: 1
                                implicitHeight: rowsCol.implicitHeight

                                ColumnLayout {
                                    id: rowsCol
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.leftMargin: Theme.sp4
                                    anchors.rightMargin: Theme.sp4
                                    spacing: 0

                                    Repeater {
                                        model: modelData.rows
                                        delegate: SettingsRow {
                                            required property var modelData
                                            required property int index
                                            Layout.fillWidth: true
                                            field: modelData
                                            isLast: index === (rowsCol.parent.parent.modelData ? 0 : 0)  // border handled inside
                                            showDivider: true
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Item { Layout.preferredHeight: Theme.sp5 }
                }
            }
        }

        // ===== footer (.sfoot) =====
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            color: Theme.elev
            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hair }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp5
                anchors.rightMargin: 20
                Text { text: "As alterações são aplicadas ao confirmar"; color: Theme.t4; font.pointSize: 10.5; font.family: Theme.fontSans }
                Item { Layout.fillWidth: true }
                Row {
                    spacing: Theme.sp2
                    BtnFlat { text: "Cancelar"; onClicked: win.close() }
                    BtnFlat { primary: true; text: "OK"; onClicked: win.close() }
                }
            }
        }
    }

    // ---- single settings row (one field) ----
    component SettingsRow: ColumnLayout {
        property var field
        property bool showDivider: true
        property bool isLast: false
        spacing: 0

        // .srow
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 13
            Layout.bottomMargin: 13
            spacing: Theme.sp4

            // .smeta
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4
                RowLayout {
                    spacing: Theme.sp2
                    Text {
                        text: field.label
                        color: Theme.t1
                        font.pointSize: 12.5
                        font.family: Theme.fontSans
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: field.type === "warning"
                    }
                    // badge
                    Rectangle {
                        visible: field.badge !== undefined
                        implicitWidth: bdg.implicitWidth + 14
                        implicitHeight: 18
                        radius: 999
                        color: Theme.field
                        border.color: Theme.hair
                        border.width: 1
                        Text { id: bdg; anchors.centerIn: parent; text: field.badge || ""; color: Theme.t3; font.pointSize: 9.5; font.family: Theme.fontMono }
                    }
                }
                Text {
                    visible: field.note !== undefined
                    Layout.maximumWidth: 440
                    text: field.note || ""
                    color: Theme.t4
                    font.pointSize: 10.5
                    font.family: Theme.fontSans
                    wrapMode: Text.WordWrap
                    lineHeight: 1.5
                }
            }

            // .sctrl — control by type
            Loader {
                Layout.alignment: Qt.AlignVCenter
                visible: field.type !== "warning" && field.type !== "path" && field.type !== "timerange" && field.type !== "days"
                sourceComponent: {
                    switch (field.type) {
                    case "toggle": return cToggle
                    case "anime": return cAnime
                    case "number": return cNumber
                    case "text": return cText
                    case "password": return cText
                    case "select": return cSelect
                    case "segmented": return cSegmented
                    case "theme": return cTheme
                    case "button": return cButton
                    case "iface": return cSelect
                    }
                    return null
                }
            }
        }

        // full-width controls below (path / warning / timerange / days are stacked)
        // (handled inline above via type; for brevity path uses its own row)

        Rectangle { visible: showDivider; Layout.fillWidth: true; height: 1; color: Theme.hairSoft }

        // ---- control components ----
        Component { id: cToggle; TToggle {
            on: (typeof settings !== "undefined" && field.key !== undefined) ? settings.get(field.key) === true : field.on === true
            onToggled: function(v) { if (typeof settings !== "undefined" && field.key !== undefined) settings.set(field.key, v) }
        } }
        Component {
            id: cAnime
            TToggle {
                on: Theme.anime
                onToggled: function(v) { Theme.setAnime(v) }
            }
        }
        Component {
            id: cTheme
            TSelect {
                // options index ↔ Theme.name
                readonly property var names: ["dark", "light", "midnight", "sakura", "darkstar"]
                implicitWidth: 180
                model: field.options || []
                currentIndex: Math.max(0, names.indexOf(Theme.name))
                onActivated: function(i) { Theme.setName(names[i]) }
            }
        }
        Component {
            id: cNumber
            RowLayout {
                spacing: Theme.sp2
                Rectangle {
                    implicitWidth: 92; implicitHeight: 30; radius: 7
                    color: Theme.field; border.color: Theme.hair; border.width: 1
                    TextInput {
                        anchors.fill: parent; anchors.rightMargin: 10
                        text: (typeof settings !== "undefined" && field.key !== undefined) ? settings.get(field.key) : (field.value || "")
                        color: Theme.t1
                        font.pointSize: 12; font.family: Theme.fontMono
                        horizontalAlignment: TextInput.AlignRight
                        verticalAlignment: TextInput.AlignVCenter
                        onEditingFinished: if (typeof settings !== "undefined" && field.key !== undefined) settings.set(field.key, text)
                    }
                }
                Text { visible: field.suffix !== undefined; text: field.suffix || ""; color: Theme.t4; font.pointSize: 10.5; font.family: Theme.fontMono }
            }
        }
        Component {
            id: cText
            TFld {
                implicitWidth: field.w === "grow" ? 320 : field.w === "w-md" ? 210 : field.w === "w-sm" ? 120 : 180
                implicitHeight: 30
                mono: field.mono === true
                text: (typeof settings !== "undefined" && field.key !== undefined) ? settings.get(field.key) : (field.value || "")
                placeholder: field.placeholder || ""
                onEdited: function(t) { if (typeof settings !== "undefined" && field.key !== undefined) settings.set(field.key, t) }
            }
        }
        Component {
            id: cSelect
            TSelect {
                implicitWidth: 180
                model: field.options || []
                currentIndex: (typeof settings !== "undefined" && field.key !== undefined) ? settings.get(field.key) : (field.value || 0)
                onActivated: function(i) { if (typeof settings !== "undefined" && field.key !== undefined) settings.set(field.key, i) }
            }
        }
        Component {
            id: cSegmented
            Rectangle {
                implicitWidth: segR.implicitWidth + 4
                implicitHeight: 29
                radius: 8
                color: Theme.field
                border.color: Theme.hair
                border.width: 1
                property int curIdx: (typeof settings !== "undefined" && field.key !== undefined) ? settings.get(field.key) : (field.value || 0)
                Row {
                    id: segR
                    anchors.centerIn: parent
                    spacing: 2
                    Repeater {
                        model: field.options || []
                        delegate: Rectangle {
                            height: 25
                            implicitWidth: segLbl.implicitWidth + 22
                            radius: 6
                            color: index === parent.parent.parent.curIdx ? Theme.sel : "transparent"
                            Text {
                                id: segLbl
                                anchors.centerIn: parent
                                text: modelData
                                color: index === parent.parent.parent.curIdx ? Theme.accentText : Theme.t3
                                font.pointSize: 11
                                font.weight: Font.Medium
                                font.family: Theme.fontSans
                            }
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { parent.parent.parent.curIdx = index; if (typeof settings !== "undefined" && field.key !== undefined) settings.set(field.key, index) } }
                        }
                    }
                }
            }
        }
        Component { id: cButton; BtnFlat { text: field.btn || ""; sm: false } }
    }
}
