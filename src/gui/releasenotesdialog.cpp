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

ReleaseNotesDialog::ReleaseNotesDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr_("release_notes_title"));
    setFixedSize(560, 520);

    const auto &tm = ThemeManager::instance();
    QString bg = tm.bgColor();
    QString sf = tm.surfaceColor();
    QString tx = tm.textColor();
    QString mt = tm.mutedColor();
    QString ac = tm.accentColor();
    QString bd = tm.borderColor();

    setStyleSheet(QString(R"(
        QDialog { background-color: %1; color: %2; }
        QLabel { color: %2; }
        QTextBrowser {
            background-color: %3; color: %2;
            border: 1px solid %6; border-radius: 8px;
            padding: 16px; font-size: 13px;
            selection-background-color: %5;
        }
        QPushButton {
            background-color: %5; color: #ffffff;
            border: none; border-radius: 8px;
            padding: 10px 32px; font-size: 13px; font-weight: 600;
        }
        QPushButton:hover { background-color: %7; }
    )").arg(bg, tx, sf, mt, ac, bd, tm.accentLightColor()));

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(32, 24, 32, 24);
    mainLayout->setSpacing(12);

    // Logo + title row
    auto *headerLayout = new QHBoxLayout;
    auto *logoLabel = new QLabel;
    QPixmap logo(":/images/logo1.png");
    logoLabel->setPixmap(logo.scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    headerLayout->addWidget(logoLabel);

    auto *titleLabel = new QLabel(
        QString("%1 v%2").arg(tr_("release_notes_title"),
                              QApplication::applicationVersion()));
    titleLabel->setStyleSheet(QString("font-size: 18px; font-weight: 700; color: %1; margin-left: 10px;").arg(tx));
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);

    auto *subtitleLabel = new QLabel(tr_("release_notes_subtitle"));
    subtitleLabel->setStyleSheet(QString("font-size: 12px; color: %1;").arg(mt));
    mainLayout->addWidget(subtitleLabel);

    // Release notes content
    auto *browser = new QTextBrowser;
    browser->setOpenExternalLinks(true);
    browser->setHtml(releaseNotes());
    mainLayout->addWidget(browser, 1);

    // Close button
    auto *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    auto *closeBtn = new QPushButton(tr_("release_notes_close"));
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    mainLayout->addLayout(btnLayout);
}

QString ReleaseNotesDialog::releaseNotes()
{
    return QStringLiteral(
        "<h3 style='color:#c43030'>v2.3.0</h3>"
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
        "<h3 style='color:#c43030'>v2.2.0</h3>"
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
        "<h3 style='color:#c43030'>v2.1.0</h3>"
        "<ul>"
        "<li><b>RSS Manager</b> &mdash; subscribe to RSS/Atom feeds and auto-download matching torrents</li>"
        "<li><b>Addon System</b> &mdash; search and install community addons</li>"
        "<li><b>Streaming</b> &mdash; stream media files directly from active torrents</li>"
        "<li><b>Media Server Integration</b> &mdash; notify Plex/Jellyfin when downloads complete</li>"
        "<li><b>Bandwidth Scheduler</b> &mdash; set alternative speed limits on a schedule</li>"
        "<li><b>VPN / Interface Binding</b> &mdash; bind to specific network interfaces with kill switch</li>"
        "</ul>"
        "<h3 style='color:#c43030'>v2.0.0</h3>"
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
