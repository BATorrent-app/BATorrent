// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef QMLPAIRINGBRIDGE_H
#define QMLPAIRINGBRIDGE_H

#include "bridges/bridgecommon.h"

class QmlPairingBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString url READ url NOTIFY changed)
    Q_PROPERTY(bool available READ available NOTIFY changed)
public:
    explicit QmlPairingBridge(QObject *parent = nullptr);

    QString url() const { return m_url; }
    bool available() const { return !m_url.isEmpty(); }

    // Recompute the URL from current settings (WebUI port + LAN IP). The dialog
    // calls this on open so enabling the WebUI / changing the port reflects
    // without a restart.
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void copyUrl();
    Q_INVOKABLE void openBrowser();
    Q_INVOKABLE void copyText(const QString &t);   // copy an arbitrary string (the password)
    // QR as a list of row strings ("0101…"), one char per module. Empty on failure.
    Q_INVOKABLE QStringList qrRows() const;
    Q_INVOKABLE QStringList qrRowsForUrl(const QString &url) const;   // QR for a URL with creds

signals:
    void changed();

private:
    static QString detectLanIp();
    QString m_url;
};

#endif // QMLPAIRINGBRIDGE_H
