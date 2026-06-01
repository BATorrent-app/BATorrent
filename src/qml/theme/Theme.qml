pragma Singleton
import QtQuick

// ============================================================
//  BATorrent — Theme singleton (ground-truth tokens)
//  Cada token abaixo é LEITURA LITERAL dos :root dos CSS:
//    dark      ← batorrent-home.css
//    light     ← batorrent-home-light.css
//    midnight  ← batorrent-home-midnight.css
//    sakura    ← batorrent-home-sakura.css
//    darkstar  ← batorrent-home-darkstar.css
//  Nada de hex/rgba hardcoded fora deste arquivo.
//  Theme.name chaveia tudo; Theme.anime liga a arte de acento.
//  Persistência via QmlThemeBridge (QSettings: qmlThemeName, qmlAnime).
// ============================================================
QtObject {
    id: theme

    property string name: typeof themeBridge !== "undefined" ? themeBridge.themeName : "dark"
    property bool anime: typeof themeBridge !== "undefined" ? themeBridge.anime : false

    function setName(n) {
        if (typeof themeBridge !== "undefined") themeBridge.themeName = n
        else theme.name = n
    }
    function setAnime(on) {
        if (typeof themeBridge !== "undefined") themeBridge.anime = on
        else theme.anime = on
    }

    // light-mode flag (sakura e light são claros → color-scheme: light)
    readonly property bool isLight: name === "light" || name === "sakura"
    readonly property bool isDark: !isLight

    // ---------- spacing (--s1..--s6) ----------
    readonly property int sp1: 4
    readonly property int sp2: 8
    readonly property int sp3: 12
    readonly property int sp4: 16
    readonly property int sp5: 24
    readonly property int sp6: 32

    // ---------- fonts (--sys / --mono, strings idênticas) ----------
    readonly property string fontSans: Qt.platform.os === "windows" ? "Segoe UI" : (Qt.platform.os === "osx" ? ".AppleSystemUIFont" : "sans-serif")
    readonly property string fontMono: Qt.platform.os === "windows" ? "Consolas" : (Qt.platform.os === "osx" ? "Menlo" : "monospace")

    // ---------- surfaces ----------
    readonly property color bg:
        name === "custom"   ? customBgColor :
        name === "light"    ? "#ffffff" :
        name === "midnight" ? "#05080f" :
        name === "sakura"   ? "#fde6ef" :
        name === "darkstar" ? "#0b0612" : "#0e0e10"

    readonly property color panel:
        name === "custom"   ? customPanelColor :
        name === "light"    ? "#f4f5f7" :
        name === "midnight" ? "#0f1830" :
        name === "sakura"   ? "#ffffff" :
        name === "darkstar" ? "#190f2e" : "#141416"

    // elevated/recessed surfaces derive from the user's panel so they track it
    readonly property color elev:
        name === "custom"   ? Qt.darker(customPanelColor, 1.12) :
        name === "light"    ? "#eef0f2" :
        name === "midnight" ? "#0b1426" :
        name === "sakura"   ? "#fcdcea" :
        name === "darkstar" ? "#130b22" : "#18181b"

    // input/recessed field (--c-field nos diálogos; home não usa, mas mantém coerência)
    readonly property color field:
        name === "custom"   ? Qt.darker(customBgColor, 1.18) :
        name === "light"    ? "#ffffff" :
        name === "midnight" ? "#080f1f" :
        name === "sakura"   ? "#ffffff" :
        name === "darkstar" ? "#0a0717" : "#0b0c0d"

    // ---------- hairlines (custom: derived from the user's text color) ----------
    readonly property color hair:
        name === "custom"   ? Qt.rgba(customTextColor.r, customTextColor.g, customTextColor.b, 0.14) :
        name === "light"    ? Qt.rgba(0,0,0,0.11) :
        name === "midnight" ? Qt.rgba(90/255,140/255,240/255,0.15) :
        name === "sakura"   ? Qt.rgba(95/255,29/255,46/255,0.13) :
        name === "darkstar" ? Qt.rgba(185/255,160/255,255/255,0.14) : Qt.rgba(1,1,1,0.08)

    readonly property color hairSoft:
        name === "custom"   ? Qt.rgba(customTextColor.r, customTextColor.g, customTextColor.b, 0.07) :
        name === "light"    ? Qt.rgba(0,0,0,0.06) :
        name === "midnight" ? Qt.rgba(90/255,140/255,240/255,0.08) :
        name === "sakura"   ? Qt.rgba(95/255,29/255,46/255,0.07) :
        name === "darkstar" ? Qt.rgba(185/255,160/255,255/255,0.08) : Qt.rgba(1,1,1,0.05)

    readonly property color hover:
        name === "custom"   ? Qt.rgba(customTextColor.r, customTextColor.g, customTextColor.b, 0.05) :
        name === "light"    ? Qt.rgba(0,0,0,0.045) :
        name === "sakura"   ? Qt.rgba(214/255,51/255,108/255,0.06) :
        name === "midnight" ? Qt.rgba(80/255,140/255,245/255,0.07) :
        name === "darkstar" ? Qt.rgba(168/255,85/255,247/255,0.10) : Qt.rgba(1,1,1,0.035)

    // .sel / .pill.on tint (--c-sel)
    readonly property color sel:
        name === "custom"   ? Qt.rgba(customPrimaryColor.r, customPrimaryColor.g, customPrimaryColor.b, 0.13) :
        name === "darkstar" ? Qt.rgba(168/255,85/255,247/255,0.20) :
        name === "sakura"   ? Qt.rgba(214/255,51/255,108/255,0.13) :
        name === "light"    ? Qt.rgba(229/255,51/255,43/255,0.10) :
        name === "midnight" ? Qt.rgba(220/255,38/255,38/255,0.13) : Qt.rgba(229/255,51/255,43/255,0.09)

    // .pill.on bg (home usa rgba(229,51,43,0.12); reaproveitado p/ accent tint forte)
    readonly property color accentTint:
        name === "custom"   ? Qt.rgba(customPrimaryColor.r, customPrimaryColor.g, customPrimaryColor.b, 0.12) :
        name === "darkstar" ? Qt.rgba(168/255,85/255,247/255,0.12) :
        name === "sakura"   ? Qt.rgba(214/255,51/255,108/255,0.12) : Qt.rgba(229/255,51/255,43/255,0.12)

    readonly property color track:
        name === "custom"   ? Qt.rgba(customTextColor.r, customTextColor.g, customTextColor.b, 0.10) :
        name === "light"    ? Qt.rgba(0,0,0,0.10) :
        name === "midnight" ? Qt.rgba(90/255,140/255,240/255,0.16) :
        name === "sakura"   ? Qt.rgba(95/255,29/255,46/255,0.12) :
        name === "darkstar" ? Qt.rgba(185/255,160/255,255/255,0.15) : Qt.rgba(1,1,1,0.09)

    // ---------- text (custom: t1 = user color, t2-t4 fade by opacity) ----------
    readonly property color t1:
        name === "custom"   ? customTextColor :
        name === "light"    ? "#16171a" :
        name === "midnight" ? "#e4edff" :
        name === "sakura"   ? "#3f1d2e" :
        name === "darkstar" ? "#efeaff" : "#f3f3f4"

    readonly property color t2:
        name === "custom"   ? Qt.rgba(customTextColor.r, customTextColor.g, customTextColor.b, 0.72) :
        name === "light"    ? "#44464d" :
        name === "midnight" ? "#8aa3da" :
        name === "sakura"   ? "#7e4862" :
        name === "darkstar" ? "#b6aae0" : "#b4b5ba"

    readonly property color t3:
        name === "custom"   ? Qt.rgba(customTextColor.r, customTextColor.g, customTextColor.b, 0.52) :
        name === "light"    ? "#6c6e76" :
        name === "midnight" ? "#6a82b8" :
        name === "sakura"   ? "#8a5a70" :
        name === "darkstar" ? "#8a7eb8" : "#818288"

    readonly property color t4:
        name === "custom"   ? Qt.rgba(customTextColor.r, customTextColor.g, customTextColor.b, 0.36) :
        name === "light"    ? "#9a9da6" :
        name === "midnight" ? "#47588a" :
        name === "sakura"   ? "#b58aa0" :
        name === "darkstar" ? "#645889" : "#5b5c63"

    // ---------- custom theme (name === "custom"): active profile colors ----------
    // Six user colors (bg/panel/text + 3 accents). No legibility auto-fixing —
    // the user owns the contrast. Secondary text tones (t2-t4) and hairlines are
    // derived from the text/bg colors by opacity so they track the user's palette.
    readonly property color customBgColor:        typeof themeBridge !== "undefined" ? themeBridge.cBg        : "#0e0e10"
    readonly property color customPanelColor:     typeof themeBridge !== "undefined" ? themeBridge.cPanel     : "#141416"
    readonly property color customTextColor:      typeof themeBridge !== "undefined" ? themeBridge.cText      : "#f3f3f4"
    readonly property color customPrimaryColor:   typeof themeBridge !== "undefined" ? themeBridge.cPrimary   : "#e5332b"
    readonly property color customSecondaryColor: typeof themeBridge !== "undefined" ? themeBridge.cSecondary : "#d99a2b"
    readonly property color customTertiaryColor:  typeof themeBridge !== "undefined" ? themeBridge.cTertiary  : "#3fb950"

    // ---------- accents (functional) ----------
    // --red (fill)
    readonly property color accent:
        name === "custom"   ? customPrimaryColor :
        name === "sakura"   ? "#d6336c" :
        name === "darkstar" ? "#a855f7" : "#e5332b"
    // --red-d
    readonly property color accentDark:
        name === "custom"   ? Qt.darker(customPrimaryColor, 1.35) :
        name === "sakura"   ? "#be185d" :
        name === "darkstar" ? "#7e22ce" : "#c01f18"
    // --red-t (accent text). For custom, just the primary itself — the user
    // owns the contrast (no auto legibility fixing).
    readonly property color accentText:
        name === "custom"   ? customPrimaryColor :
        name === "light"    ? "#cf2a22" :
        name === "sakura"   ? "#be185d" :
        name === "darkstar" ? "#c084fc" :
        name === "midnight" ? "#ef6a64" : "#ec6a64"

    // --amber (fill: seeding/upload)
    readonly property color amber:
        name === "custom"   ? customSecondaryColor :
        name === "darkstar" ? "#22d3ee" :
        name === "sakura"   ? "#c8881f" : "#d99a2b"
    // --amber-t (.v-up text)
    readonly property color up:
        name === "custom"   ? Qt.lighter(customSecondaryColor, 1.2) :
        name === "darkstar" ? "#67e8f9" :
        (name === "light" || name === "sakura") ? "#9a6710" : "#e0b454"

    // .fl-pa (paused fill) — neutral derived from the user's text in custom
    readonly property color pausedFill:
        name === "custom" ? Qt.rgba(customTextColor.r, customTextColor.g, customTextColor.b, 0.42) :
        isLight ? "#6a6c73" : "#54555c"

    // --grn (verde de saúde: seeds em Search, "Instalado" em Addons, badge "Auto" em RSS)
    // bat-dialog*.css: dark/midnight/darkstar #3fb950 ; light/sakura #2e9c40
    readonly property color grn:
        name === "custom" ? customTertiaryColor : (isLight ? "#2e9c40" : "#3fb950")

    // ---------- anime accent art (per theme) ----------
    readonly property string animeSource:
        name === "dark"     ? "qrc:/images/eyes-dark.png" :
        name === "midnight" ? "qrc:/images/eyes-midnight.png" :
        name === "sakura"   ? "qrc:/images/eyes-sakura.png" :
        name === "darkstar" ? "qrc:/images/spider.jpg" : ""   // light: nenhuma
    readonly property bool animeBottom: name === "darkstar"   // aranha no canto inferior-direito
    readonly property bool hasAnime: anime && animeSource !== ""

    // ---------- custom full background image (custom theme only) ----------
    // QSettings stores a plain path; build a file URL (Windows needs file:///).
    readonly property string bgImageSource:
        (name === "custom" && typeof themeBridge !== "undefined" && themeBridge.cBgImage !== "")
            ? (Qt.platform.os === "windows" ? "file:///" : "file://") + encodeURI(themeBridge.cBgImage)
            : ""
    readonly property real bgImageOpacity:
        typeof themeBridge !== "undefined" ? themeBridge.cBgOpacity / 100 : 0.55
    readonly property bool hasBgImage: bgImageSource !== ""

    // troca de tema (chamada pela SettingsWindow — NUNCA por demo control na subbar)
    function cycle() {
        var order = ["dark", "light", "midnight", "sakura", "darkstar", "custom"]
        setName(order[(order.indexOf(name) + 1) % order.length])
    }
}
