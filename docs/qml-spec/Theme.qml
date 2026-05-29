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
// ============================================================
QtObject {
    id: theme

    property string name: "dark"      // dark | light | midnight | sakura | darkstar
    property bool anime: false

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
    readonly property string fontSans: "-apple-system, BlinkMacSystemFont, SF Pro Text, Segoe UI, system-ui, sans-serif"
    readonly property string fontMono: "SF Mono, ui-monospace, JetBrains Mono, Menlo, Consolas, monospace"

    // ---------- surfaces ----------
    readonly property color bg:
        name === "light"    ? "#ffffff" :
        name === "midnight" ? "#08070d" :
        name === "sakura"   ? "#fde6ef" :
        name === "darkstar" ? "#0b0612" : "#0e0e10"

    readonly property color panel:
        name === "light"    ? "#f4f5f7" :
        name === "midnight" ? "#181425" :
        name === "sakura"   ? "#ffffff" :
        name === "darkstar" ? "#190f2e" : "#141416"

    readonly property color elev:
        name === "light"    ? "#eef0f2" :
        name === "midnight" ? "#12121c" :
        name === "sakura"   ? "#fcdcea" :
        name === "darkstar" ? "#130b22" : "#18181b"

    // input/recessed field (--c-field nos diálogos; home não usa, mas mantém coerência)
    readonly property color field:
        name === "light"    ? "#ffffff" :
        name === "midnight" ? "#0f0e18" :
        name === "sakura"   ? "#ffffff" :
        name === "darkstar" ? "#0a0717" : "#0b0c0d"

    // ---------- hairlines ----------
    readonly property color hair:
        name === "light"    ? Qt.rgba(0,0,0,0.11) :
        name === "midnight" ? Qt.rgba(150/255,160/255,225/255,0.13) :
        name === "sakura"   ? Qt.rgba(95/255,29/255,46/255,0.13) :
        name === "darkstar" ? Qt.rgba(185/255,160/255,255/255,0.14) : Qt.rgba(1,1,1,0.08)

    readonly property color hairSoft:
        name === "light"    ? Qt.rgba(0,0,0,0.06) :
        name === "midnight" ? Qt.rgba(150/255,160/255,225/255,0.07) :
        name === "sakura"   ? Qt.rgba(95/255,29/255,46/255,0.07) :
        name === "darkstar" ? Qt.rgba(185/255,160/255,255/255,0.08) : Qt.rgba(1,1,1,0.05)

    readonly property color hover:
        name === "light"    ? Qt.rgba(0,0,0,0.045) :
        name === "sakura"   ? Qt.rgba(214/255,51/255,108/255,0.06) :
        name === "midnight" ? Qt.rgba(160/255,170/255,235/255,0.05) :
        name === "darkstar" ? Qt.rgba(168/255,85/255,247/255,0.10) : Qt.rgba(1,1,1,0.035)

    // .sel / .pill.on tint (--c-sel)
    readonly property color sel:
        name === "darkstar" ? Qt.rgba(168/255,85/255,247/255,0.20) :
        name === "sakura"   ? Qt.rgba(214/255,51/255,108/255,0.13) :
        name === "light"    ? Qt.rgba(229/255,51/255,43/255,0.10) :
        name === "midnight" ? Qt.rgba(220/255,38/255,38/255,0.13) : Qt.rgba(229/255,51/255,43/255,0.09)

    // .pill.on bg (home usa rgba(229,51,43,0.12); reaproveitado p/ accent tint forte)
    readonly property color accentTint:
        name === "darkstar" ? Qt.rgba(168/255,85/255,247/255,0.12) :
        name === "sakura"   ? Qt.rgba(214/255,51/255,108/255,0.12) : Qt.rgba(229/255,51/255,43/255,0.12)

    readonly property color track:
        name === "light"    ? Qt.rgba(0,0,0,0.10) :
        name === "midnight" ? Qt.rgba(150/255,160/255,225/255,0.14) :
        name === "sakura"   ? Qt.rgba(95/255,29/255,46/255,0.12) :
        name === "darkstar" ? Qt.rgba(185/255,160/255,255/255,0.15) : Qt.rgba(1,1,1,0.09)

    // ---------- text ----------
    readonly property color t1:
        name === "light"    ? "#16171a" :
        name === "midnight" ? "#eceafb" :
        name === "sakura"   ? "#3f1d2e" :
        name === "darkstar" ? "#efeaff" : "#f3f3f4"

    readonly property color t2:
        name === "light"    ? "#44464d" :
        name === "midnight" ? "#9a95c8" :
        name === "sakura"   ? "#7e4862" :
        name === "darkstar" ? "#b6aae0" : "#b4b5ba"

    readonly property color t3:
        name === "light"    ? "#6c6e76" :
        name === "midnight" ? "#7a75a0" :
        name === "sakura"   ? "#8a5a70" :
        name === "darkstar" ? "#8a7eb8" : "#818288"

    readonly property color t4:
        name === "light"    ? "#9a9da6" :
        name === "midnight" ? "#565178" :
        name === "sakura"   ? "#b58aa0" :
        name === "darkstar" ? "#645889" : "#5b5c63"

    // ---------- accents (functional) ----------
    // --red (fill)
    readonly property color accent:
        name === "sakura"   ? "#d6336c" :
        name === "darkstar" ? "#a855f7" : "#e5332b"
    // --red-d
    readonly property color accentDark:
        name === "sakura"   ? "#be185d" :
        name === "darkstar" ? "#7e22ce" : "#c01f18"
    // --red-t (accent legível sobre o bg)
    readonly property color accentText:
        name === "light"    ? "#cf2a22" :
        name === "sakura"   ? "#be185d" :
        name === "darkstar" ? "#c084fc" :
        name === "midnight" ? "#ef6a64" : "#ec6a64"

    // --amber (fill: seeding/upload)
    readonly property color amber:
        name === "darkstar" ? "#22d3ee" :
        name === "sakura"   ? "#c8881f" : "#d99a2b"
    // --amber-t (.v-up text)
    readonly property color up:
        name === "darkstar" ? "#67e8f9" :
        (name === "light" || name === "sakura") ? "#9a6710" : "#e0b454"

    // .fl-pa (paused fill) — dark/midnight/darkstar #54555c; light/sakura #6a6c73
    readonly property color pausedFill: isLight ? "#6a6c73" : "#54555c"

    // ---------- anime accent art (per theme) ----------
    readonly property string animeSource:
        name === "dark"     ? "images/eyes-dark.png" :
        name === "midnight" ? "images/eyes-midnight.png" :
        name === "sakura"   ? "images/eyes-sakura.png" :
        name === "darkstar" ? "images/spider.jpg" : ""   // light: nenhuma
    readonly property bool animeBottom: name === "darkstar"   // aranha no canto inferior-direito
    readonly property bool hasAnime: anime && animeSource !== ""

    // troca de tema (chamada pela SettingsWindow — NUNCA por demo control na subbar)
    function cycle() {
        var order = ["dark", "light", "midnight", "sakura", "darkstar"]
        name = order[(order.indexOf(name) + 1) % order.length]
    }
}
