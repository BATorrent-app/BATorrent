// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "speedtestdialog.h"
#include "../app/translator.h"
#include "../gui/thememanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QByteArray>

SpeedTestDialog::SpeedTestDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr_("speedtest_title"));
    setFixedSize(420, 380);

    m_nam = new QNetworkAccessManager(this);

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
        QPushButton {
            background-color: %3; color: #ffffff;
            border: none; border-radius: 8px;
            padding: 12px 36px; font-size: 14px; font-weight: 600;
        }
        QPushButton:hover { background-color: %6; }
        QPushButton:disabled { background-color: %5; color: %4; }
        QProgressBar {
            border: 1px solid %5; border-radius: 4px;
            background-color: %7; text-align: center; color: %2;
            height: 8px;
        }
        QProgressBar::chunk {
            background-color: %3; border-radius: 3px;
        }
    )").arg(bg, tx, ac, mt, bd, tm.accentLightColor(), sf));

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(36, 28, 36, 28);
    mainLayout->setSpacing(16);

    // Title
    auto *titleLabel = new QLabel(tr_("speedtest_title"));
    titleLabel->setStyleSheet(QString("font-size: 20px; font-weight: 700; color: %1;").arg(tx));
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    // Phase label
    m_phaseLabel = new QLabel("");
    m_phaseLabel->setStyleSheet(QString("font-size: 13px; color: %1;").arg(mt));
    m_phaseLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_phaseLabel);

    // Progress bar
    m_progressBar = new QProgressBar;
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(false);
    m_progressBar->setFixedHeight(8);
    mainLayout->addWidget(m_progressBar);

    mainLayout->addSpacing(8);

    // Results area
    auto addResultRow = [&](const QString &labelKey, QLabel *&valueLabel) {
        auto *row = new QHBoxLayout;
        auto *nameLabel = new QLabel(tr_(labelKey));
        nameLabel->setStyleSheet(QString("font-size: 14px; font-weight: 600; color: %1;").arg(tx));
        valueLabel = new QLabel("--");
        valueLabel->setStyleSheet(QString("font-size: 22px; font-weight: 700; color: %1;").arg(ac));
        valueLabel->setAlignment(Qt::AlignRight);
        row->addWidget(nameLabel);
        row->addStretch();
        row->addWidget(valueLabel);
        mainLayout->addLayout(row);
    };

    addResultRow("speedtest_ping", m_pingLabel);
    addResultRow("speedtest_download", m_downloadLabel);
    addResultRow("speedtest_upload", m_uploadLabel);

    mainLayout->addStretch();

    // Start button
    m_startBtn = new QPushButton(tr_("speedtest_start"));
    connect(m_startBtn, &QPushButton::clicked, this, &SpeedTestDialog::startTest);
    auto *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    btnLayout->addWidget(m_startBtn);
    btnLayout->addStretch();
    mainLayout->addLayout(btnLayout);
}

void SpeedTestDialog::startTest()
{
    m_startBtn->setEnabled(false);
    m_pingLabel->setText("--");
    m_downloadLabel->setText("--");
    m_uploadLabel->setText("--");
    m_progressBar->setValue(0);
    m_pingMs = 0.0;
    m_downloadMbps = 0.0;
    m_uploadMbps = 0.0;

    testPing();
}

void SpeedTestDialog::setPhase(const QString &text)
{
    m_phaseLabel->setText(tr_("speedtest_testing") + " " + text);
}

void SpeedTestDialog::testPing()
{
    setPhase(tr_("speedtest_ping") + "...");
    m_progressBar->setValue(10);

    QNetworkRequest req(QUrl("https://speed.cloudflare.com"));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);

    m_timer.start();
    QNetworkReply *reply = m_nam->head(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        m_pingMs = m_timer.elapsed();
        reply->deleteLater();

        m_pingLabel->setText(QString("%1 ms").arg(m_pingMs, 0, 'f', 0));
        m_progressBar->setValue(25);

        testDownload();
    });
}

void SpeedTestDialog::testDownload()
{
    setPhase(tr_("speedtest_download") + "...");

    QNetworkRequest req(QUrl("https://speed.cloudflare.com/__down?bytes=25000000"));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);

    m_timer.start();
    QNetworkReply *reply = m_nam->get(req);

    connect(reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        if (total > 0) {
            int pct = 25 + static_cast<int>(35.0 * received / total);
            m_progressBar->setValue(pct);
        }
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        double elapsedSec = m_timer.elapsed() / 1000.0;
        qint64 bytes = reply->bytesAvailable();
        // Also account for already-read bytes
        QByteArray data = reply->readAll();
        qint64 totalBytes = reply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
        if (totalBytes <= 0)
            totalBytes = data.size();
        reply->deleteLater();

        if (elapsedSec > 0.0)
            m_downloadMbps = (totalBytes * 8.0) / (elapsedSec * 1000000.0);

        m_downloadLabel->setText(QString("%1 Mbps").arg(m_downloadMbps, 0, 'f', 2));
        m_progressBar->setValue(60);

        testUpload();
    });
}

void SpeedTestDialog::testUpload()
{
    setPhase(tr_("speedtest_upload") + "...");

    QByteArray payload(10000000, 'x'); // 10 MB of data

    QNetworkRequest req(QUrl("https://speed.cloudflare.com/__up"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);

    m_timer.start();
    QNetworkReply *reply = m_nam->post(req, payload);

    connect(reply, &QNetworkReply::uploadProgress, this, [this](qint64 sent, qint64 total) {
        if (total > 0) {
            int pct = 60 + static_cast<int>(35.0 * sent / total);
            m_progressBar->setValue(pct);
        }
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply, payloadSize = payload.size()]() {
        double elapsedSec = m_timer.elapsed() / 1000.0;
        reply->deleteLater();

        if (elapsedSec > 0.0)
            m_uploadMbps = (payloadSize * 8.0) / (elapsedSec * 1000000.0);

        m_uploadLabel->setText(QString("%1 Mbps").arg(m_uploadMbps, 0, 'f', 2));
        m_progressBar->setValue(100);

        showResults();
    });
}

void SpeedTestDialog::showResults()
{
    m_phaseLabel->setText(tr_("speedtest_complete"));
    m_startBtn->setEnabled(true);
}
