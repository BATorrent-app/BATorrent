// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef BATORRENT_NOTIFIER_H
#define BATORRENT_NOTIFIER_H

#include <QObject>
#include <QString>

class QNetworkAccessManager;

// Outbound notification mirror — broadcasts the same events the in-app Toast
// raises (download finished, kill switch, RSS auto-download, errors) to a
// Telegram chat via the Bot API.
//
// Token + chat ID live in QSettings (token via SecretStore so it doesn't sit
// in plaintext). All sends are fire-and-forget; failures are logged but never
// surfaced as UI errors — a broken webhook should never block the app.
class TelegramNotifier : public QObject
{
    Q_OBJECT
public:
    enum Event {
        EventFinished = 1 << 0,
        EventKillSwitch = 1 << 1,
        EventRssAuto = 1 << 2,
        EventError = 1 << 3,
    };
    Q_DECLARE_FLAGS(Events, Event)

    explicit TelegramNotifier(QObject *parent = nullptr);

    void reload();                // re-read settings (called after Settings save)
    bool isConfigured() const;    // token + chat ID both non-empty

    // Send arbitrary text. Used by the "Test connection" button and by the
    // internal handlers. parse_mode = Markdown so * and _ format.
    void send(const QString &text);

    // Subscribed-to-event check used by the slot handlers. Events the user
    // unchecked in Settings are silently dropped here.
    bool wantsEvent(Event e) const;

public slots:
    // Wire these directly to the matching signals from MainWindow's ctor.
    void onTorrentFinished(const QString &name, const QString &infoHash);
    void onKillSwitchTriggered();
    void onRssAutoDownloaded(const QString &feedName, const QString &itemTitle);
    void onTorrentError(const QString &message);

private:
    QNetworkAccessManager *m_nam;
    QString m_token;
    QString m_chatId;
    Events m_events;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(TelegramNotifier::Events)

#endif
