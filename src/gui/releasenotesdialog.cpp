// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "releasenotesdialog.h"
#include "../app/translator.h"
#include "../gui/thememanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextBrowser>
#include <QDesktopServices>
#include <QUrl>
#include <QPixmap>
#include <QApplication>
#include <QGraphicsDropShadowEffect>

ReleaseNotesDialog::ReleaseNotesDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr_("release_notes_title"));
    setFixedSize(620, 580);

    const auto &tm = ThemeManager::instance();

    setStyleSheet(QString(
        "QDialog {"
        "  background: qradialgradient(cx:0.5, cy:0, radius:0.7,"
        "      stop:0 %7,"
        "      stop:1 %1);"
        "  color: %2;"
        "}"
        "QLabel { background: transparent; }"
        "QTextBrowser {"
        "  background: %3; color: %2;"
        "  border: none; border-radius: 8px;"
        "  padding: 18px 22px;"
        "  selection-background-color: %4;"
        "  selection-color: #ffffff;"
        "}"
        "#closeBtn {"
        "  background: transparent; color: %2;"
        "  border: 1px solid %5; border-radius: 6px;"
        "  padding: 8px 22px; font-size: 11px; font-weight: 500;"
        "}"
        "#closeBtn:hover { background: %6; }"
        ).arg(tm.bgColor(), tm.textColor(), tm.panelColor(),
              tm.accentColor(), tm.borderColor(), tm.surfaceColor(),
              tm.accentTintForGradient(12)));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(36, 32, 36, 24);
    root->setSpacing(0);

    // Header: logo with halo + textual cluster (eyebrow + heading + version)
    auto *headerRow = new QHBoxLayout;
    headerRow->setSpacing(16);
    headerRow->setContentsMargins(0, 0, 0, 0);

    auto *logoLabel = new QLabel;
    logoLabel->setPixmap(tm.themedLogo(56, 2.0));
    logoLabel->setFixedSize(56, 56);
    logoLabel->setScaledContents(true);
    auto *halo = new QGraphicsDropShadowEffect;
    halo->setColor(QColor(220, 38, 38, 80));
    halo->setBlurRadius(36);
    halo->setOffset(0, 0);
    logoLabel->setGraphicsEffect(halo);
    headerRow->addWidget(logoLabel, 0, Qt::AlignTop);

    auto *textCol = new QVBoxLayout;
    textCol->setSpacing(4);
    textCol->setContentsMargins(0, 4, 0, 0);

    auto *eyebrow = new QLabel(tr_("release_notes_title").toUpper());
    {
        QFont f; f.setPointSize(8); f.setWeight(QFont::Bold);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 1.2);
        eyebrow->setFont(f);
        eyebrow->setStyleSheet(QString("color: %1;").arg(tm.accentColor()));
    }
    textCol->addWidget(eyebrow);

    auto *heading = new QLabel(tr_("release_notes_heading"));
    {
        QFont f; f.setPointSize(18); f.setWeight(QFont::Bold);
        f.setLetterSpacing(QFont::AbsoluteSpacing, -0.3);
        heading->setFont(f);
        heading->setStyleSheet(QString("color: %1;").arg(tm.textColor()));
    }
    textCol->addWidget(heading);

    auto *versionLbl = new QLabel(QString("v%1").arg(QApplication::applicationVersion()));
    {
        QFont f("Menlo");
        f.setStyleHint(QFont::Monospace);
        f.setPointSize(10);
        versionLbl->setFont(f);
        versionLbl->setStyleSheet(QString("color: %1;").arg(tm.dimColor()));
    }
    textCol->addWidget(versionLbl);

    headerRow->addLayout(textCol, 1);
    root->addLayout(headerRow);
    root->addSpacing(22);

    auto *browser = new QTextBrowser;
    browser->setOpenExternalLinks(true);
    browser->setFrameShape(QFrame::NoFrame);
    browser->document()->setDefaultStyleSheet(QString(
        "h3 {"
        "  color: %1; font-size: 10px; font-weight: 700;"
        "  letter-spacing: 1.4px; text-transform: uppercase;"
        "  margin-top: 18px; margin-bottom: 8px;"
        "}"
        "h3:first-of-type { margin-top: 0; }"
        "ul { margin: 0 0 0 12px; padding: 0; }"
        "li { color: %2; margin-bottom: 6px; line-height: 150%; }"
        "li b { color: %3; font-weight: 600; }"
        ).arg(tm.accentColor(), tm.mutedColor(), tm.textColor()));
    browser->setHtml(releaseNotes());
    root->addWidget(browser, 1);

    root->addSpacing(16);

    auto *bottom = new QHBoxLayout;
    auto *donateBtn = new QPushButton(QStringLiteral("♥  ") + tr_("action_donate"));
    donateBtn->setObjectName(QStringLiteral("closeBtn"));
    donateBtn->setCursor(Qt::PointingHandCursor);
    connect(donateBtn, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://github.com/sponsors/Mateuscruz19")));
    });
    bottom->addWidget(donateBtn);
    bottom->addStretch();
    auto *closeBtn = new QPushButton(tr_("release_notes_close"));
    closeBtn->setObjectName(QStringLiteral("closeBtn"));
    closeBtn->setCursor(Qt::PointingHandCursor);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    bottom->addWidget(closeBtn);
    root->addLayout(bottom);
}

QString ReleaseNotesDialog::releaseNotes()
{
    return QStringLiteral(
        "<h3>v2.5.0</h3>"
        "<ul>"
        "<li><b>PT Mode</b> &mdash; one-toggle compliance for private trackers: disables "
        "DHT/PEX/LSD, forces anonymous handshake, announces to every tier. Required by "
        "RuTracker private, M-Team, TorrentLeech and most ratio-tracked communities.</li>"
        "<li><b>Tor proxy preset</b> &mdash; one-click fill of the proxy fields with "
        "127.0.0.1:9050 (SOCKS5).</li>"
        "<li><b>Telegram webhook notifications</b> &mdash; mirror in-app toasts (download "
        "complete, kill switch, RSS auto-download, errors) into any Telegram chat or "
        "channel via a bot token. Per-event checkboxes + a Test button. Token stored "
        "in the OS keychain via SecretStore.</li>"
        "<li><b>WebUI mobile pairing with QR</b> &mdash; new <i>Pair phone</i> button in "
        "Settings &rarr; WebUI shows a scannable QR + copyable LAN URL so you can open "
        "the WebUI on your phone with one camera scan. QR encoder is implemented in pure "
        "C++ / Qt (no external service contacted — your LAN address never leaks off the "
        "machine). WebUI HTML was already responsive.</li>"
        "<li><b>Gitee update mirror</b> &mdash; Update source combo in Settings: GitHub "
        "(default), Gitee (China mirror), or Disabled. Same JSON schema so no parser "
        "branching needed.</li>"
        "<li><b>RSS feed presets</b> &mdash; one-click add of Nyaa (anime, all + English-"
        "subbed + search template), Sukebei, and the Linux Tracker.</li>"
        "<li><b>Speed unit toggle</b> &mdash; new General &rarr; <i>Display speeds as</i> combo: "
        "Bytes (KB/s, MB/s) or Bits (Kbps, Mbps). Default unchanged.</li>"
        "<li><b>Locale-aware number formatting</b> &mdash; <code>formatSize</code> / "
        "<code>formatSpeed</code> now use <code>QLocale::system()</code> so RU sees "
        "<code>1 234,5</code> instead of <code>1,234.5</code>.</li>"
        "<li><b>No-telemetry guarantee</b> &mdash; explicit privacy statement on the "
        "Welcome dialog, About box, and README.</li>"
        "<li><b>i18n sweep</b> &mdash; all 37 new v2.5.0 strings translated to EN, PT, "
        "ZH, JA, RU, ES, DE (579 keys &times; 7 languages, 100% coverage).</li>"
        "</ul>"
        "<h3>v2.4.2</h3>"
        "<ul>"
        "<li><b>Completed state</b> &mdash; new green-tinted state separate from Seeding. "
        "Stop toolbar button + context menu mark a 100% torrent as completed (paused, "
        "frozen). Resume on a completed torrent unmarks it. Settings &rarr; auto-mark "
        "as completed after 1/3/7/14/30 days of seeding.</li>"
        "<li><b>Light theme rework</b> &mdash; warm cream / beige palette inspired by the "
        "JSX &quot;Comfortable&quot; reference instead of pure white, with darker brown "
        "borders and a softer red accent surface.</li>"
        "<li><b>Dark theme refinement</b> &mdash; panel / surface colors now align with the "
        "body bg (no more brown drift on the details panel) to match the canvas reference.</li>"
        "<li><b>Theme switching fixes</b> &mdash; toolbar, status bar, filter row, search "
        "edit, category combo, details panel, and the About / message popups all re-style "
        "in place on theme change. Previous build left several of these locked to the "
        "startup theme and shrank toolbar icons cumulatively across switches.</li>"
        "<li><b>Logo legibility in light mode</b> &mdash; the white-on-transparent logo is "
        "now tinted to textColor in light theme so it stops disappearing on cream "
        "surfaces (home empty state, toolbar, tray popup, welcome / release notes).</li>"
        "<li><b>Dialog gradients</b> &mdash; replaced translucent red gradient stops with "
        "opaque pre-mixed tints so welcome, release notes, shortcuts, add / create "
        "torrent, RSS, search, statistics, and addons dialogs stop showing a black "
        "streak through the gradient in light mode.</li>"
        "<li><b>Startup &amp; empty-state polish</b> &mdash; no more 1 s flash of the empty "
        "bat widget while resume data loads on launch; status / member init hardened.</li>"
        "</ul>"
        "<h3>v2.4.1</h3>"
        "<ul>"
        "<li><b>Design overhaul</b> &mdash; full dark UI redesign across every dialog: "
        "welcome, settings, add torrent, search, RSS, create torrent, statistics, addons, "
        "release notes, shortcuts. Eyebrow CAPS sections, monospace technical strings, "
        "consistent panel surfaces, primary/ghost button styles.</li>"
        "<li><b>Tray popup</b> &mdash; cross-platform custom tray dropdown replaces the "
        "native context menu on left-click. Live speeds, active-torrent preview with "
        "progress bars, VPN status, auto-shutdown toggle, danger-styled Quit. Right-click "
        "still shows the native menu on Windows/Linux for keyboard navigation.</li>"
        "<li><b>Settings as standalone window</b> &mdash; opens as an independent top-level "
        "window (Qt::Window, non-modal) at 960×720 instead of a cramped child modal of the "
        "main window. Group sections render as panel cards with eyebrow CAPS headers.</li>"
        "<li><b>Auto-pause on file errors</b> &mdash; when libtorrent reports a file_error_alert "
        "on a finished torrent (files deleted from disk, external drive unplugged), the torrent "
        "is automatically paused. Previously it would silently mark the pieces as unavailable "
        "and start re-downloading the whole torrent.</li>"
        "<li><b>Double-click reveal fixed</b> &mdash; resolves the actual on-disk root via "
        "libtorrent's file_path(0) instead of trusting the torrent display name, which can "
        "drift from disk after rename or sanitization. File now lands highlighted in Finder/"
        "Explorer even when the displayed name doesn't match the filesystem.</li>"
        "<li><b>Translator fallback</b> &mdash; non-English locales now fall back to English "
        "when a key is missing instead of rendering raw key names. ~60 new translation keys "
        "added across all 7 supported languages (EN, PT-BR, ZH, JA, RU, ES, DE).</li>"
        "<li><b>Speed test &rarr; fast.com</b> &mdash; the in-app speed test (which Cloudflare "
        "started blocking with 403s) was replaced by a direct fast.com browser redirect. "
        "Real Netflix-backed measurement, no maintenance.</li>"
        "<li><b>No splash screen</b> &mdash; cinematic startup animation removed. App opens "
        "straight to the main window, matching modern productivity apps (Discord, VSCode, "
        "qBittorrent, Transmission).</li>"
        "<li><b>macOS bundle</b> &mdash; build now produces a proper .app on macOS. Fixes a "
        "Qt 6.11 + macOS Sequoia issue where QSystemTrayIcon could register at height 0.</li>"
        "</ul>"
        "<h3>v2.3.4</h3>"
        "<ul>"
        "<li><b>Async resume saves</b> &mdash; the 5-minute periodic save no longer freezes "
        "the GUI; piece-finished events trigger opportunistic per-torrent saves (rate-limited "
        "to 1/min) so a crash mid-download loses at most ~1 minute of progress.</li>"
        "<li><b>Slow torrent demotion</b> &mdash; torrents stuck below 10 KB/s for over 60s "
        "stop counting against the active-downloads limit, so a fresh torrent can start in "
        "their place instead of sitting in the queue forever.</li>"
        "<li><b>µTP toggle + port settings</b> &mdash; Settings → Network exposes Enable µTP, "
        "Listening port, and Use a random port on each launch.</li>"
        "<li><b>HTTPS GeoIP</b> &mdash; peer-country lookups switched from http://ip-api.com "
        "to https://ipinfo.io so peer IPs no longer cross the network in cleartext.</li>"
        "<li><b>Click-to-focus notifications</b> &mdash; clicking a finished-torrent balloon "
        "brings the window forward AND selects the matching row.</li>"
        "<li><b>Async icon cache</b> &mdash; scaled icons are cached instead of re-rendered "
        "from the 1024px source on every paint; multi-size Linux + retina macOS icons.</li>"
        "<li><b>Fix</b> &mdash; new torrents stuck paused (libtorrent default flags), "
        "&ldquo;Open folder&rdquo; landing in Documents, .!bt suffix never stripped on "
        "completion, removeTorrent race re-creating .resume files, killswitch leaving "
        "torrents paused forever after VPN switch.</li>"
        "<li><b>WebUI hardening</b> &mdash; request-smuggling guard, per-request temp file "
        "for uploads, bounded auth-failure cache, 24h session cookie TTL.</li>"
        "<li><b>SecretStore</b> &mdash; heap-allocated QtKeychain jobs with 5s timeout "
        "(stack-allocated jobs were UB if the keychain was cold).</li>"
        "<li><b>Updater</b> &mdash; arch detection via QSysInfo; macOS now points at the "
        "arm64.dmg the CI actually publishes.</li>"
        "</ul>"
        "<h3>v2.3.0</h3>"
        "<ul>"
        "<li><b>Categories/Tags</b> &mdash; organize torrents by type (Movies, Games, Software, Music, Other) with filtering</li>"
        "<li><b>Piece Map</b> &mdash; visual grid showing downloaded vs missing pieces in the details panel</li>"
        "<li><b>GeoIP Flags</b> &mdash; country flag emojis next to peer IPs</li>"
        "<li><b>Speed Test</b> &mdash; built-in internet speed test (ping, download, upload)</li>"
        "<li><b>Download Queue</b> &mdash; limit concurrent active downloads with queue priority</li>"
        "<li><b>Auto-move Completed</b> &mdash; automatically move finished downloads to a configured folder</li>"
        "<li><b>Statistics</b> &mdash; all-time and per-session download/upload statistics</li>"
        "<li><b>Keyboard Shortcuts</b> &mdash; reference dialog for all hotkeys</li>"
        "<li><b>Export/Import Settings</b> &mdash; backup and restore configuration as JSON</li>"
        "<li><b>Splash Animation</b> &mdash; new cinematic startup with sonar rings, particles, and bat sound</li>"
        "<li><b>Dark Installer</b> &mdash; fully themed Windows installer with branding</li>"
        "<li><b>Bug Fix</b> &mdash; app now relaunches after update; tray instance restores on new launch</li>"
        "</ul>"
        "<h3>v2.2.0</h3>"
        "<ul>"
        "<li><b>WebUI Redesign</b> &mdash; completely overhauled web interface with modern dark theme, "
        "filtering, search, drag-and-drop upload, and responsive mobile layout</li>"
        "<li><b>Release Notes</b> &mdash; in-app release notes shown after updates and accessible from Help menu</li>"
        "<li><b>Set as Default App</b> &mdash; option to set BATorrent as default handler for .torrent files "
        "and magnet links, with first-launch prompt</li>"
        "<li><b>Double-click to Open Folder</b> &mdash; double-click any torrent row to open its save directory</li>"
        "<li><b>Default Save Path</b> &mdash; 'Use default path' setting now actually skips the folder picker</li>"
        "<li><b>5 New Languages</b> &mdash; Chinese, Japanese, Russian, Spanish, and German translations</li>"
        "<li><b>Installer Logo</b> &mdash; BATorrent logo now appears in the Windows installer wizard</li>"
        "<li><b>Test Suite</b> &mdash; comprehensive security, memory leak, and unit/integration tests with "
        "Catch2, ASan, CppCheck, MSVC /analyze, CRT Debug Heap, and Dr. Memory</li>"
        "</ul>"
        "<h3>v2.1.0</h3>"
        "<ul>"
        "<li><b>RSS Manager</b> &mdash; subscribe to RSS/Atom feeds and auto-download matching torrents</li>"
        "<li><b>Addon System</b> &mdash; search and install community addons</li>"
        "<li><b>Streaming</b> &mdash; stream media files directly from active torrents</li>"
        "<li><b>Media Server Integration</b> &mdash; notify Plex/Jellyfin when downloads complete</li>"
        "<li><b>Bandwidth Scheduler</b> &mdash; set alternative speed limits on a schedule</li>"
        "<li><b>VPN / Interface Binding</b> &mdash; bind to specific network interfaces with kill switch</li>"
        "</ul>"
        "<h3>v2.0.0</h3>"
        "<ul>"
        "<li><b>WebUI</b> &mdash; remote web interface with authentication</li>"
        "<li><b>Speed Graph</b> &mdash; real-time download/upload speed visualization</li>"
        "<li><b>Proxy Support</b> &mdash; SOCKS4/5 and HTTP proxy configuration</li>"
        "<li><b>IP Filtering</b> &mdash; load blocklists to filter peer connections</li>"
        "<li><b>Themes</b> &mdash; dark, light, and system theme options</li>"
        "<li><b>Auto-Updater</b> &mdash; automatic update checks with one-click install</li>"
        "</ul>"
    );
}
