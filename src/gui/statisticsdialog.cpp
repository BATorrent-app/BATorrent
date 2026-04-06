// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "statisticsdialog.h"
#include "../app/translator.h"
#include "../gui/thememanager.h"
#include "../torrent/sessionmanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSettings>
#include <QApplication>
#include <QFrame>
#include <QDateTime>

static QString formatBytes(qint64 bytes)
{
    if (bytes < 1024)
        return QString("%1 B").arg(bytes);
    if (bytes < 1024 * 1024)
        return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    if (bytes < 1024LL * 1024 * 1024)
        return QString("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 2);
    if (bytes < 1024LL * 1024 * 1024 * 1024)
        return QString("%1 GB").arg(bytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
    return QString("%1 TB").arg(bytes / (1024.0 * 1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
}

static QString formatDuration(qint64 seconds)
{
    int days = static_cast<int>(seconds / 86400);
    int hours = static_cast<int>((seconds % 86400) / 3600);
    int mins = static_cast<int>((seconds % 3600) / 60);
    int secs = static_cast<int>(seconds % 60);
    if (days > 0)
        return QString("%1d %2h %3m").arg(days).arg(hours).arg(mins);
    if (hours > 0)
        return QString("%1h %2m %3s").arg(hours).arg(mins).arg(secs);
    return QString("%1m %2s").arg(mins).arg(secs);
}

StatisticsDialog::StatisticsDialog(SessionManager *session, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr_("stats_title"));
    setFixedSize(460, 420);

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
        QFrame[frameShape="4"] { background-color: %3; max-height: 1px; }
    )").arg(bg, tx, bd));

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(36, 28, 36, 28);
    mainLayout->setSpacing(12);

    // Title
    auto *titleLabel = new QLabel(tr_("stats_title"));
    titleLabel->setStyleSheet(QString("font-size: 20px; font-weight: 700; color: %1;").arg(tx));
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    mainLayout->addSpacing(4);

    // Helper to add a stat row
    auto addStatRow = [&](const QString &label, const QString &value) {
        auto *row = new QHBoxLayout;
        auto *nameLabel = new QLabel(label);
        nameLabel->setStyleSheet(QString("font-size: 13px; color: %1;").arg(mt));
        auto *valueLabel = new QLabel(value);
        valueLabel->setStyleSheet(QString("font-size: 13px; font-weight: 600; color: %1;").arg(tx));
        valueLabel->setAlignment(Qt::AlignRight);
        row->addWidget(nameLabel);
        row->addStretch();
        row->addWidget(valueLabel);
        mainLayout->addLayout(row);
    };

    // Helper to add a section header
    auto addSection = [&](const QString &text) {
        auto *sep = new QFrame;
        sep->setFrameShape(QFrame::HLine);
        mainLayout->addWidget(sep);
        auto *lbl = new QLabel(text);
        lbl->setStyleSheet(QString("font-size: 14px; font-weight: 700; color: %1;").arg(ac));
        mainLayout->addWidget(lbl);
    };

    // --- All-time stats ---
    addSection(tr_("stats_alltime"));

    qint64 totalDown = session->globalDownloaded();
    qint64 totalUp = session->globalUploaded();
    float ratio = session->globalRatio();
    int totalTorrents = session->totalTorrentsAdded();

    addStatRow(tr_("stats_downloaded"), formatBytes(totalDown));
    addStatRow(tr_("stats_uploaded"), formatBytes(totalUp));
    addStatRow(tr_("stats_ratio"), QString::number(ratio, 'f', 3));
    addStatRow(tr_("stats_torrents_added"), QString::number(totalTorrents));

    // --- Session stats ---
    addSection(tr_("stats_session"));

    qint64 sessionDown = session->sessionDownloaded();
    qint64 sessionUp = session->sessionUploaded();

    QSettings settings("BATorrent", "BATorrent");
    qint64 startTime = settings.value("sessionStartTime", 0).toLongLong();
    qint64 uptime = 0;
    if (startTime > 0)
        uptime = QDateTime::currentSecsSinceEpoch() - startTime;

    addStatRow(tr_("stats_uptime"), formatDuration(uptime));
    addStatRow(tr_("stats_downloaded"), formatBytes(sessionDown));
    addStatRow(tr_("stats_uploaded"), formatBytes(sessionUp));

    mainLayout->addSpacing(8);

    // --- Visual bar: download vs upload proportion ---
    if (totalDown > 0 || totalUp > 0) {
        auto *barFrame = new QFrame;
        barFrame->setFixedHeight(20);
        barFrame->setStyleSheet(QString("background-color: %1; border-radius: 4px;").arg(sf));

        auto *barLayout = new QHBoxLayout(barFrame);
        barLayout->setContentsMargins(0, 0, 0, 0);
        barLayout->setSpacing(0);

        double total = static_cast<double>(totalDown + totalUp);
        int downPct = static_cast<int>((totalDown / total) * 100.0);

        auto *downBar = new QFrame;
        downBar->setStyleSheet(QString("background-color: %1; border-radius: 4px;").arg(ac));

        auto *upBar = new QFrame;
        upBar->setStyleSheet(QString("background-color: %1; border-radius: 4px;").arg(tm.warningColor()));

        barLayout->addWidget(downBar, downPct);
        barLayout->addWidget(upBar, 100 - downPct);

        mainLayout->addWidget(barFrame);

        auto *legendLayout = new QHBoxLayout;
        auto *downLegend = new QLabel(QString("%1 %2").arg(tr_("stats_downloaded"), formatBytes(totalDown)));
        downLegend->setStyleSheet(QString("font-size: 10px; color: %1;").arg(ac));
        auto *upLegend = new QLabel(QString("%1 %2").arg(tr_("stats_uploaded"), formatBytes(totalUp)));
        upLegend->setStyleSheet(QString("font-size: 10px; color: %1;").arg(tm.warningColor()));
        upLegend->setAlignment(Qt::AlignRight);
        legendLayout->addWidget(downLegend);
        legendLayout->addStretch();
        legendLayout->addWidget(upLegend);
        mainLayout->addLayout(legendLayout);
    }

    mainLayout->addStretch();
}
