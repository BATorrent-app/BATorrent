pragma Singleton
import QtQuick

QtObject {
    property string current: "dark"

    readonly property color bg: {
        if (current === "sakura") return "#FDE6EF"
        if (current === "light") return "#ece4d2"
        if (current === "midnight") return "#08070d"
        return "#0e0a0a"
    }

    readonly property color surface: {
        if (current === "sakura") return "#FFFFFF"
        if (current === "light") return "#f5eed9"
        if (current === "midnight") return "#12121c"
        return "#15110f"
    }

    readonly property color panel: {
        if (current === "sakura") return "#FCE7F0"
        if (current === "light") return "#f1e9d4"
        if (current === "midnight") return "#181425"
        return "#14100f"
    }

    readonly property color surfaceAlt: {
        if (current === "sakura") return "#FBD5E1"
        if (current === "light") return "#e2d9bf"
        if (current === "midnight") return "#0f0e18"
        return "#100c0b"
    }

    readonly property color accent: {
        if (current === "sakura") return "#d6336c"
        return "#dc2626"
    }

    readonly property color accentDark: {
        if (current === "sakura") return "#BE185D"
        return "#991b1b"
    }

    readonly property color accentLight: {
        if (current === "sakura") return "#F472B6"
        return "#ef4444"
    }

    readonly property color text: {
        if (current === "sakura") return "#3F1D2E"
        if (current === "light") return "#1a1614"
        if (current === "midnight") return "#eceafb"
        return "#f0f0f0"
    }

    readonly property color muted: {
        if (current === "sakura") return "#7E4862"
        if (current === "light") return "#6d5f4d"
        if (current === "midnight") return "#9a95c8"
        return "#b0b0b8"
    }

    readonly property color dim: {
        if (current === "sakura") return "#8a5a70"
        if (current === "light") return "#6e6355"
        if (current === "midnight") return "#7a75a0"
        return "#8a8a94"
    }

    readonly property color border: {
        if (current === "sakura") return "#F9C2D2"
        if (current === "light") return "#d9cfb8"
        if (current === "midnight") return "#22203a"
        return "#2a2018"
    }

    readonly property color borderStrong: {
        if (current === "sakura") return "#F19BB3"
        if (current === "light") return "#c2b59b"
        if (current === "midnight") return "#322e50"
        return "#3a2e22"
    }

    readonly property color stateDownloading: {
        if (current === "sakura") return "#EC4899"
        return "#dc2626"
    }

    readonly property color stateSeeding: {
        if (current === "sakura") return "#F59E0B"
        if (current === "light") return "#7f1d1d"
        return "#991b1b"
    }

    readonly property color stateCompleted: {
        if (current === "sakura") return "#B98AA0"
        if (current === "light") return "#a85252"
        return "#6b2a2a"
    }

    readonly property color statePaused: {
        if (current === "sakura") return "#EBC8D4"
        if (current === "light") return "#cfcac0"
        if (current === "midnight") return "#2d2a42"
        return "#3a3035"
    }

    readonly property color stateError: {
        if (current === "sakura") return "#9F1239"
        if (current === "light") return "#7f1d1d"
        return "#991b1b"
    }

    readonly property color accentTint: {
        if (current === "sakura") return Qt.rgba(236/255, 72/255, 153/255, 0.10)
        if (current === "light") return Qt.rgba(220/255, 38/255, 38/255, 0.08)
        return Qt.rgba(220/255, 38/255, 38/255, 0.10)
    }

    readonly property int fontDisplay: 18
    readonly property int fontHeading: 15
    readonly property int fontBody: 11
    readonly property int fontCaption: 9

    readonly property int spacingXs: 4
    readonly property int spacingSm: 8
    readonly property int spacingMd: 12
    readonly property int spacingLg: 16
    readonly property int spacingXl: 24

    readonly property int radiusSm: 6
    readonly property int radiusMd: 8
    readonly property int radiusLg: 12
}
