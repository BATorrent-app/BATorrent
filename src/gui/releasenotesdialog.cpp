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
        "      stop:0 rgba(220,38,38,0.12),"
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
              tm.accentColor(), tm.borderColor(), tm.surfaceColor()));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(36, 32, 36, 24);
    root->setSpacing(0);

    // Header: logo with halo + textual cluster (eyebrow + heading + version)
    auto *headerRow = new QHBoxLayout;
    headerRow->setSpacing(16);
    headerRow->setContentsMargins(0, 0, 0, 0);

    auto *logoLabel = new QLabel;
    QPixmap raw(":/images/logo1.png");
    logoLabel->setPixmap(raw.scaled(56 * 2, 56 * 2,
        Qt::KeepAspectRatio, Qt::SmoothTransformation));
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
