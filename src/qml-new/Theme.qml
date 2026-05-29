pragma Singleton
import QtQuick

QtObject {
    id: theme

    // "dark" | "light" | "midnight" | "sakura" | "darkstar"
    property string name: "dark"
    property bool anime: true

    readonly property bool isDark: name !== "light" && name !== "sakura"

    // ---- surfaces ----
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

    readonly property color field:
        name === "light"    ? "#ffffff" :
        name === "midnight" ? "#0f0e18" :
        name === "sakura"   ? "#ffffff" :
        name === "darkstar" ? "#0a0717" : "#0b0c0d"

    // ---- hairlines ----
    readonly property color hair:
        name === "light"    ? Qt.rgba(0,0,0,0.11) :
        name === "sakura"   ? Qt.rgba(0.37,0.11,0.18,0.13) :
        name === "midnight" ? Qt.rgba(0.59,0.63,0.88,0.13) :
        name === "darkstar" ? Qt.rgba(0.73,0.63,1,0.14) : Qt.rgba(1,1,1,0.08)

    readonly property color hairSoft:
        name === "light"    ? Qt.rgba(0,0,0,0.06) :
        name === "sakura"   ? Qt.rgba(0.37,0.11,0.18,0.07) :
        name === "midnight" ? Qt.rgba(0.59,0.63,0.88,0.07) :
        name === "darkstar" ? Qt.rgba(0.73,0.63,1,0.08) : Qt.rgba(1,1,1,0.05)

    readonly property color hover:
        isDark ? Qt.rgba(1,1,1,0.04) : Qt.rgba(0,0,0,0.045)

    readonly property color sel:
        name === "darkstar" ? Qt.rgba(0.66,0.33,0.97,0.20) :
        name === "sakura"   ? Qt.rgba(0.84,0.20,0.42,0.13) :
                              Qt.rgba(0.90,0.20,0.17,0.13)

    // ---- text ----
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

    // ---- functional accents ----
    readonly property color accent:
        name === "sakura"   ? "#d6336c" :
        name === "darkstar" ? "#a855f7" : "#e5332b"

    readonly property color accentDark:
        name === "sakura"   ? "#be185d" :
        name === "darkstar" ? "#7e22ce" : "#c01f18"

    readonly property color accentText:   // accent legible on the bg
        name === "light"    ? "#cf2a22" :
        name === "sakura"   ? "#be185d" :
        name === "darkstar" ? "#c084fc" :
        name === "midnight" ? "#ef6a64" : "#ec6a64"

    // upload / seeding (amber, or cyan on dark star)
    readonly property color up:
        name === "darkstar" ? "#67e8f9" :
        (name === "light" || name === "sakura") ? "#9a6710" : "#e0b454"

    readonly property color track:
        isDark ? Qt.rgba(1,1,1,0.09) : Qt.rgba(0,0,0,0.10)

    // ---- anime accent art per theme ----
    readonly property string animeSource:
        name === "dark"     ? "images/eyes-dark.png" :
        name === "midnight" ? "images/eyes-midnight.png" :
        name === "sakura"   ? "images/eyes-sakura.png" :
        name === "darkstar" ? "images/spider.jpg" : ""   // light: none

    readonly property bool animeBottom: name === "darkstar"   // spider sits bottom-right
    readonly property bool hasAnime: anime && animeSource !== ""

    function cycle() {
        var order = ["dark", "light", "midnight", "sakura", "darkstar"];
        name = order[(order.indexOf(name) + 1) % order.length];
    }
}
