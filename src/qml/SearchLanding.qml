// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Branding hero of the search landing state (logo + name + prompt). Shown on
// the cold start — and, once the catalog browse exists, as the store-build
// fallback where discovery content is unavailable.
import QtQuick
import QtQuick.Layouts
import "theme"

ColumnLayout {
    spacing: 8
    Image {
        Layout.alignment: Qt.AlignHCenter
        Layout.preferredWidth: 58; Layout.preferredHeight: 58
        source: "qrc:/images/logo.svg"
        sourceSize: Qt.size(116, 116)
        fillMode: Image.PreserveAspectFit
    }
    Text {
        Layout.alignment: Qt.AlignHCenter
        text: "BATorrent"
        color: Theme.t1; font.pixelSize: 26; font.weight: Font.Bold
        font.letterSpacing: -0.5; font.family: Theme.fontSans
    }
    Text {
        Layout.alignment: Qt.AlignHCenter
        text: (i18n.language, i18n.t("search_prompt_sub"))
        color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans
    }
}
