// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "pairingdialog.h"
#include "../app/qrcodegen.h"
#include "../app/translator.h"
#include "thememanager.h"

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QHBoxLayout>
#include <QLabel>
#include <QNetworkInterface>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

PairingDialog::PairingDialog(int port, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr_("pairing_title"));
    setMinimumSize(480, 320);

    const auto &tm = ThemeManager::instance();
    setStyleSheet(QString(
        "QDialog { background: %1; color: %2; }"
        "QLabel { background: transparent; color: %2; }"
        "QPushButton { background: transparent; color: %2; border: 1px solid %3;"
        "  border-radius: 6px; padding: 8px 18px; font-size: 11px; font-weight: 500; }"
        "QPushButton:hover { background: %4; }"
        "#primaryBtn { background: %5; color: #ffffff; border-color: %5; }"
        "#primaryBtn:hover { background: %6; border-color: %6; }"
        ).arg(tm.bgColor(), tm.textColor(), tm.borderColor(), tm.surfaceColor(),
              tm.accentColor(), tm.accentLightColor()));

    const QString ip = detectLanIp();
    const QString url = ip.isEmpty()
        ? QString()
        : QStringLiteral("http://%1:%2/").arg(ip).arg(port);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(28, 24, 28, 24);
    root->setSpacing(14);

    auto *eyebrow = new QLabel(tr_("pairing_title").toUpper());
    {
        QFont f; f.setPointSize(8); f.setWeight(QFont::Bold);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 1.4);
        eyebrow->setFont(f);
        eyebrow->setStyleSheet(QString("color: %1;").arg(tm.accentColor()));
    }
    root->addWidget(eyebrow);

    auto *heading = new QLabel(tr_("pairing_heading"));
    {
        QFont f; f.setPointSize(15); f.setWeight(QFont::Bold);
        heading->setFont(f);
    }
    root->addWidget(heading);

    auto *desc = new QLabel(tr_("pairing_desc"));
    desc->setWordWrap(true);
    desc->setStyleSheet(QString("color: %1; font-size: 12px;").arg(tm.mutedColor()));
    root->addWidget(desc);

    root->addSpacing(12);

    if (url.isEmpty()) {
        // No usable interface — most often a sleeping Wi-Fi or a machine on
        // Ethernet that's currently unplugged. Tell the user what to fix.
        auto *err = new QLabel(tr_("pairing_no_lan"));
        err->setWordWrap(true);
        err->setStyleSheet(QString("color: %1; font-size: 12px;").arg(tm.accentColor()));
        root->addWidget(err);
        root->addStretch();
    } else {
        // QR code first — most phones scan via the camera app and open the
        // URL directly, no typing needed. Generated in pure Qt (no external
        // service, so the LAN URL never leaks off the machine).
        QPixmap qr = qrgen::renderQR(url, 240, Qt::black, Qt::white, 2);
        if (!qr.isNull()) {
            auto *qrLabel = new QLabel;
            qrLabel->setPixmap(qr);
            qrLabel->setAlignment(Qt::AlignCenter);
            qrLabel->setStyleSheet(
                "QLabel { background: white; border-radius: 8px; padding: 12px; }");
            auto *qrRow = new QHBoxLayout;
            qrRow->addStretch();
            qrRow->addWidget(qrLabel);
            qrRow->addStretch();
            root->addLayout(qrRow);
            root->addSpacing(8);
        }

        // Big monospace URL — easy to read across the room. Boxed bg so it
        // visually reads as "this is the thing you want to type or scan".
        auto *urlBox = new QLabel(url);
        urlBox->setAlignment(Qt::AlignCenter);
        urlBox->setTextInteractionFlags(Qt::TextSelectableByMouse);
        {
            QFont f("Menlo"); f.setStyleHint(QFont::Monospace);
            f.setPointSize(16); f.setWeight(QFont::Bold);
            urlBox->setFont(f);
            urlBox->setStyleSheet(QString(
                "QLabel { background: %1; color: %2; border: 1px solid %3;"
                "  border-radius: 8px; padding: 22px; }")
                .arg(tm.surfaceColor(), tm.textColor(), tm.borderColor()));
        }
        root->addWidget(urlBox);

        auto *hint = new QLabel(tr_("pairing_hint"));
        hint->setWordWrap(true);
        hint->setStyleSheet(QString("color: %1; font-size: 11px;").arg(tm.dimColor()));
        root->addWidget(hint);

        root->addStretch();

        auto *btnRow = new QHBoxLayout;
        btnRow->addStretch();
        auto *copyBtn = new QPushButton(tr_("pairing_copy"));
        connect(copyBtn, &QPushButton::clicked, this, [url, copyBtn]() {
            QApplication::clipboard()->setText(url);
            copyBtn->setText(tr_("pairing_copied"));
        });
        btnRow->addWidget(copyBtn);
        auto *openBtn = new QPushButton(tr_("pairing_open_browser"));
        openBtn->setObjectName(QStringLiteral("primaryBtn"));
        connect(openBtn, &QPushButton::clicked, this, [url]() {
            QDesktopServices::openUrl(QUrl(url));
        });
        btnRow->addWidget(openBtn);
        root->addLayout(btnRow);
    }

    auto *closeRow = new QHBoxLayout;
    closeRow->addStretch();
    auto *closeBtn = new QPushButton(tr_("welcome_close"));
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    closeRow->addWidget(closeBtn);
    root->addLayout(closeRow);
}

QString PairingDialog::detectLanIp()
{
    // Pick the first IPv4 on an active, non-loopback, non-virtual interface.
    // Prefer wifi-like names if multiple match (rough heuristic — works for
    // most laptops).
    QString preferred;
    QString fallback;
    for (const auto &iface : QNetworkInterface::allInterfaces()) {
        const auto flags = iface.flags();
        if (!flags.testFlag(QNetworkInterface::IsUp)) continue;
        if (!flags.testFlag(QNetworkInterface::IsRunning)) continue;
        if (flags.testFlag(QNetworkInterface::IsLoopBack)) continue;
        const QString name = iface.name();
        for (const auto &entry : iface.addressEntries()) {
            if (entry.ip().protocol() != QAbstractSocket::IPv4Protocol) continue;
            const QString ip = entry.ip().toString();
            if (ip.startsWith("169.254.")) continue; // link-local
            // Prefer en0 (macOS Wi-Fi) / wlan / wlp, otherwise use the first.
            if (name.startsWith("en") || name.contains("wlan") || name.contains("wlp"))
                preferred = ip;
            else if (fallback.isEmpty())
                fallback = ip;
        }
    }
    return preferred.isEmpty() ? fallback : preferred;
}
