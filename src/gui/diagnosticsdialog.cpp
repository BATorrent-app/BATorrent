// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "diagnosticsdialog.h"
#include "../app/translator.h"
#include "../app/utils.h"
#include "../torrent/sessionmanager.h"
#include "thememanager.h"

#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkInterface>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QStandardPaths>
#include <QSettings>
#include <QUrl>
#include <QVBoxLayout>

DiagnosticsDialog::DiagnosticsDialog(SessionManager *session, QWidget *parent)
    : QDialog(parent), m_session(session)
{
    setWindowTitle(tr_("diag_title"));
    setMinimumSize(560, 480);

    const auto &tm = ThemeManager::instance();
    setStyleSheet(QString(
        "QDialog { background: %1; color: %2; }"
        "QLabel { background: transparent; color: %2; }"
        "QPushButton { background: transparent; color: %2; border: 1px solid %3;"
        "  border-radius: 6px; padding: 7px 16px; font-size: 11px; font-weight: 500; }"
        "QPushButton:hover { background: %4; }"
        "#primaryBtn { background: %5; color: #ffffff; border-color: %5; }"
        "#primaryBtn:hover { background: %6; border-color: %6; }"
        ).arg(tm.bgColor(), tm.textColor(), tm.borderColor(), tm.surfaceColor(),
              tm.accentColor(), tm.accentLightColor()));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(14);

    auto title = new QLabel(tr_("diag_title"));
    {
        QFont f; f.setPointSize(15); f.setWeight(QFont::Bold);
        title->setFont(f);
    }
    root->addWidget(title);

    // Health check section
    auto *healthHdr = new QLabel(tr_("diag_health_section"));
    {
        QFont f; f.setPointSize(9); f.setWeight(QFont::Black);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 1.2);
        healthHdr->setFont(f);
        healthHdr->setStyleSheet(QString("color: %1;").arg(tm.dimColor()));
    }
    root->addWidget(healthHdr);

    m_healthLabel = new QLabel(tr_("diag_press_run"));
    m_healthLabel->setWordWrap(true);
    m_healthLabel->setTextFormat(Qt::RichText);
    m_healthLabel->setStyleSheet(QString(
        "background: %1; border: 1px solid %2; border-radius: 6px;"
        "padding: 12px; font-family: Menlo, Consolas, monospace; font-size: 11px;")
        .arg(tm.surfaceColor(), tm.borderColor()));
    root->addWidget(m_healthLabel);

    auto *healthBtn = new QPushButton(tr_("diag_run_health"));
    healthBtn->setObjectName(QStringLiteral("primaryBtn"));
    connect(healthBtn, &QPushButton::clicked, this, &DiagnosticsDialog::runHealthCheck);
    auto *healthRow = new QHBoxLayout;
    healthRow->addStretch();
    healthRow->addWidget(healthBtn);
    root->addLayout(healthRow);

    root->addSpacing(8);

    // IP leak test section
    auto *ipHdr = new QLabel(tr_("diag_ip_section"));
    {
        QFont f; f.setPointSize(9); f.setWeight(QFont::Black);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 1.2);
        ipHdr->setFont(f);
        ipHdr->setStyleSheet(QString("color: %1;").arg(tm.dimColor()));
    }
    root->addWidget(ipHdr);

    m_ipLabel = new QLabel(tr_("diag_ip_idle"));
    m_ipLabel->setWordWrap(true);
    m_ipLabel->setTextFormat(Qt::RichText);
    m_ipLabel->setStyleSheet(QString(
        "background: %1; border: 1px solid %2; border-radius: 6px;"
        "padding: 12px; font-family: Menlo, Consolas, monospace; font-size: 11px;")
        .arg(tm.surfaceColor(), tm.borderColor()));
    root->addWidget(m_ipLabel);

    m_ipBtn = new QPushButton(tr_("diag_run_ip"));
    m_ipBtn->setObjectName(QStringLiteral("primaryBtn"));
    connect(m_ipBtn, &QPushButton::clicked, this, &DiagnosticsDialog::runIpLeakTest);
    auto *ipRow = new QHBoxLayout;
    ipRow->addStretch();
    ipRow->addWidget(m_ipBtn);
    root->addLayout(ipRow);

    root->addStretch();

    auto *closeBtn = new QPushButton(tr_("welcome_close"));
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    auto *closeRow = new QHBoxLayout;
    closeRow->addStretch();
    closeRow->addWidget(closeBtn);
    root->addLayout(closeRow);

    m_nam = new QNetworkAccessManager(this);
}

void DiagnosticsDialog::runHealthCheck()
{
    auto pass = [](const QString &s) {
        return QStringLiteral("<span style='color:#22c55e'>✓</span> %1").arg(s);
    };
    auto fail = [](const QString &s) {
        return QStringLiteral("<span style='color:#dc2626'>✗</span> %1").arg(s);
    };
    auto warn = [](const QString &s) {
        return QStringLiteral("<span style='color:#f59e0b'>!</span> %1").arg(s);
    };

    QStringList rows;

    // Save path writable
    QSettings s("BATorrent", "BATorrent");
    QString savePath = s.value("lastSavePath",
        QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)).toString();
    QFileInfo savePathInfo(savePath);
    rows << (savePathInfo.exists() && savePathInfo.isWritable()
             ? pass(tr_("diag_check_save_path").arg(savePath))
             : fail(tr_("diag_check_save_path_fail").arg(savePath)));

    // Listen port
    int port = m_session->listenPort();
    rows << (port >= 1024 && port <= 65535
             ? pass(tr_("diag_check_listen_port").arg(port))
             : warn(tr_("diag_check_listen_port_warn")));

    // DHT
    rows << (m_session->dhtEnabled()
             ? pass(tr_("diag_check_dht_on"))
             : warn(tr_("diag_check_dht_off")));

    // Encryption
    int enc = m_session->encryptionMode();
    rows << (enc <= 1
             ? pass(tr_("diag_check_encryption_on"))
             : warn(tr_("diag_check_encryption_off")));

    // VPN binding
    QString iface = m_session->outgoingInterface();
    if (!iface.isEmpty()) {
        bool found = false;
        for (const auto &ni : QNetworkInterface::allInterfaces()) {
            if (ni.name() == iface && (ni.flags() & QNetworkInterface::IsRunning)) {
                found = true; break;
            }
        }
        rows << (found
                 ? pass(tr_("diag_check_vpn_bound").arg(iface))
                 : fail(tr_("diag_check_vpn_missing").arg(iface)));
    } else {
        rows << warn(tr_("diag_check_vpn_none"));
    }

    // Leecher blocking
    if (m_session->blockLeecherClients())
        rows << pass(tr_("diag_check_leecher_on").arg(m_session->blockedLeecherCount()));
    else
        rows << warn(tr_("diag_check_leecher_off"));

    // Active torrents sanity
    rows << pass(tr_("diag_check_torrent_count").arg(m_session->torrentCount()));

    // Detailed stats
    auto ds = m_session->detailedStats();
    rows << pass(QStringLiteral("DHT nodes: %1").arg(ds.dhtNodes));
    rows << pass(QStringLiteral("Connected peers: %1").arg(ds.peersCount));
    if (ds.totalWasted > 0)
        rows << warn(QStringLiteral("Wasted data: %1").arg(formatSize(ds.totalWasted)));
    rows << (ds.hasIncomingConnections
             ? pass(QStringLiteral("Listening for incoming connections"))
             : warn(QStringLiteral("Not listening — port may be blocked")));

    m_healthLabel->setText(rows.join(QStringLiteral("<br>")));
}

void DiagnosticsDialog::runIpLeakTest()
{
    m_ipBtn->setEnabled(false);
    m_ipLabel->setText(tr_("diag_ip_checking"));
    QNetworkRequest req(QUrl(QStringLiteral("https://api.ipify.org?format=text")));
    req.setHeader(QNetworkRequest::UserAgentHeader, "BATorrent/diagnostics");
    auto *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        m_ipBtn->setEnabled(true);
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            m_ipLabel->setText(QStringLiteral("<span style='color:#dc2626'>✗</span> %1: %2")
                .arg(tr_("diag_ip_fail"), reply->errorString()));
            return;
        }
        const QString ip = QString::fromUtf8(reply->readAll()).trimmed();
        QString boundIface = m_session->outgoingInterface();
        QString msg = QStringLiteral("<b>%1:</b> %2<br>")
            .arg(tr_("diag_ip_visible"), ip);
        if (boundIface.isEmpty()) {
            msg += QStringLiteral("<span style='color:#f59e0b'>!</span> %1")
                       .arg(tr_("diag_ip_no_vpn"));
        } else {
            msg += QStringLiteral("<span style='color:#22c55e'>✓</span> %1: <b>%2</b>")
                       .arg(tr_("diag_ip_bound_iface"), boundIface);
        }
        m_ipLabel->setText(msg);
    });
}
