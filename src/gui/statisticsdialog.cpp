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
#include <QPushButton>
#include <QSettings>
#include <QDateTime>
#include <QFontInfo>

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
    setFixedSize(560, 480);

    const auto &tm = ThemeManager::instance();

    setStyleSheet(QString(
        "QDialog {"
        "  background: qradialgradient(cx:0.5, cy:0, radius:0.7,"
        "      stop:0 rgba(220,38,38,0.10),"
        "      stop:1 %1);"
        "  color: %2;"
        "}"
        "QLabel { background: transparent; color: %2; }"
        "#ghostBtn {"
        "  background: transparent; color: %2;"
        "  border: 1px solid %3; border-radius: 6px;"
        "  padding: 8px 22px; font-size: 11px; font-weight: 500;"
        "}"
        "#ghostBtn:hover { background: %4; }"
        ).arg(tm.bgColor(), tm.textColor(), tm.borderColor(), tm.surfaceColor()));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(36, 32, 36, 24);
    root->setSpacing(0);

    auto *eyebrow = new QLabel(tr_("stats_eyebrow"));
    {
        QFont f; f.setPointSize(8); f.setWeight(QFont::Bold);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 1.2);
        eyebrow->setFont(f);
        eyebrow->setStyleSheet(QString("color: %1;").arg(tm.accentColor()));
    }
    root->addWidget(eyebrow);
    root->addSpacing(6);

    auto *heading = new QLabel(tr_("stats_heading"));
    {
        QFont f; f.setPointSize(18); f.setWeight(QFont::Bold);
        f.setLetterSpacing(QFont::AbsoluteSpacing, -0.3);
        heading->setFont(f);
        heading->setStyleSheet(QString("color: %1;").arg(tm.textColor()));
    }
    root->addWidget(heading);
    root->addSpacing(28);

    auto makeSectionEyebrow = [&](const QString &text) {
        auto *lbl = new QLabel(text.toUpper());
        QFont f; f.setPointSize(8); f.setWeight(QFont::Bold);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 1.4);
        lbl->setFont(f);
        lbl->setStyleSheet(QString("color: %1;").arg(tm.dimColor()));
        return lbl;
    };

    auto makeKV = [&](const QString &k, const QString &v, bool mono = false) {
        auto *row = new QWidget;
        auto *rl = new QHBoxLayout(row);
        rl->setContentsMargins(0, 6, 0, 6);
        rl->setSpacing(12);

        auto *key = new QLabel(k);
        QFont kf; kf.setPointSize(9); kf.setWeight(QFont::DemiBold);
        key->setFont(kf);
        key->setStyleSheet(QString("color: %1;").arg(tm.mutedColor()));
        key->setFixedWidth(110);
        rl->addWidget(key);

        auto *val = new QLabel(v);
        QFont vf; vf.setPointSize(11);
        if (mono) {
            vf.setFamily(QStringLiteral("Menlo"));
            if (!QFontInfo(vf).fixedPitch())
                vf.setFamily(QStringLiteral("Consolas"));
            vf.setStyleHint(QFont::Monospace);
        }
        val->setFont(vf);
        val->setStyleSheet(QString("color: %1;").arg(tm.textColor()));
        val->setTextInteractionFlags(Qt::TextSelectableByMouse);
        rl->addWidget(val);
        rl->addStretch();
        return row;
    };

    auto makeColumn = [&](const QString &eyebrowText,
                          std::initializer_list<QWidget *> rows) {
        auto *col = new QWidget;
        auto *cl = new QVBoxLayout(col);
        cl->setContentsMargins(0, 0, 0, 0);
        cl->setSpacing(0);
        cl->addWidget(makeSectionEyebrow(eyebrowText));
        cl->addSpacing(8);
        for (QWidget *r : rows)
            cl->addWidget(r);
        cl->addStretch();
        return col;
    };

    const qint64 totalDown = session->globalDownloaded();
    const qint64 totalUp = session->globalUploaded();
    const float totalRatio = session->globalRatio();
    const int totalTorrents = session->totalTorrentsAdded();

    const qint64 sessionDown = session->sessionDownloaded();
    const qint64 sessionUp = session->sessionUploaded();

    QSettings settings("BATorrent", "BATorrent");
    const qint64 startTime = settings.value("sessionStartTime", 0).toLongLong();
    const qint64 sessionUptime = startTime > 0
        ? QDateTime::currentSecsSinceEpoch() - startTime
        : 0;

    const float sessionRatio = sessionDown > 0
        ? static_cast<float>(sessionUp) / static_cast<float>(sessionDown)
        : 0.0f;

    auto *body = new QHBoxLayout;
    body->setContentsMargins(0, 0, 0, 0);
    body->setSpacing(32);

    body->addWidget(makeColumn(tr_("stats_alltime"), {
        makeKV(tr_("stats_downloaded"),      formatBytes(totalDown), true),
        makeKV(tr_("stats_uploaded"),        formatBytes(totalUp), true),
        makeKV(tr_("stats_ratio"),           QString::number(totalRatio, 'f', 3), true),
        makeKV(tr_("stats_torrents_added"),  QString::number(totalTorrents), true),
        makeKV(tr_("stats_uptime"),          formatDuration(sessionUptime), true),
    }), 1);

    body->addWidget(makeColumn(tr_("stats_session"), {
        makeKV(tr_("stats_downloaded"), formatBytes(sessionDown), true),
        makeKV(tr_("stats_uploaded"),   formatBytes(sessionUp), true),
        makeKV(tr_("stats_ratio"),      QString::number(sessionRatio, 'f', 3), true),
        makeKV(tr_("stats_uptime"),     formatDuration(sessionUptime), true),
    }), 1);

    root->addLayout(body);
    root->addStretch();

    auto *footer = new QHBoxLayout;
    footer->addStretch();
    auto *closeBtn = new QPushButton(tr_("welcome_close"));
    closeBtn->setObjectName(QStringLiteral("ghostBtn"));
    closeBtn->setCursor(Qt::PointingHandCursor);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    footer->addWidget(closeBtn);
    root->addLayout(footer);
}
