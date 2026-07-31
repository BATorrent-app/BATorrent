// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick

Item {
    id: root
    required property var host
    required property var toolbar
    property alias tourOverlay: tourOverlay

    TourOverlay {
        id: tourOverlay
        onPageRequested: function(page) { host.currentPage = page }
        // step 0 is the welcome (centered, no spotlight); the rest spotlight the UI
        // bat/pose vary per step so all 3 candidate SVGs (noto/twemoji/openmoji)
        // and poses (perch/hang, left/right) show in one run — for picking one.
        steps: (i18n.language, [
            { page: 0, title: i18n.t("tour_s1_t"), text: i18n.t(host.layoutClassic ? "tour_s1_d" : "tour_s1_d_top"),
              rectFn: function() { return host.navHost ? host.navHost.itemRect("rail", tourOverlay) : Qt.rect(0,0,0,0) } },
            { page: 0, title: i18n.t("tour_s2_t"), text: i18n.t("tour_s2_d"),
              rectFn: function() { return host.rectIn(toolbar.tbOpen, tourOverlay) } },
            { page: 1, title: i18n.t("tour_s3_t"), text: i18n.t("tour_s3_d"),
              rectFn: function() { return host.navHost ? host.navHost.itemRect(1, tourOverlay) : Qt.rect(0,0,0,0) } },
            { page: 2, title: i18n.t("tour_s5_t"), text: i18n.t("tour_s5_d"),
              rectFn: function() { return host.navHost ? host.navHost.itemRect(2, tourOverlay) : Qt.rect(0,0,0,0) } },
            { page: 0, title: i18n.t("tour_s6_t"), text: i18n.t("tour_s6_d"),
              rectFn: function() { return host.navHost ? host.navHost.itemRect("settings", tourOverlay) : Qt.rect(0,0,0,0) } }
        ])
    }

}
