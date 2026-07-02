// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/integrations/notifier.h"
#include "services/security/secretstore.h"
#include "services/platform/logger.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QUrl>

TelegramNotifier::TelegramNotifier(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
    m_nam->setTransferTimeout(15000);
    reload();
}

void TelegramNotifier::reload()
{
    QSettings s("BATorrent", "BATorrent");
    m_token = SecretStore::instance().get("telegramBotToken");
    m_chatId = s.value("telegramChatId").toString();
    // Default: enable all events when configured. Stored as bitmask.
    int mask = s.value("telegramEvents",
        int(EventFinished | EventKillSwitch | EventRssAuto | EventError)).toInt();
    m_events = static_cast<Events>(mask);
}

bool TelegramNotifier::isConfigured() const
{
    return !m_token.isEmpty() && !m_chatId.isEmpty();
}

bool TelegramNotifier::wantsEvent(Event e) const
{
    return m_events.testFlag(e);
}

void TelegramNotifier::send(const QString &text)
{
    if (!isConfigured()) return;
    QUrl url(QStringLiteral("https://api.telegram.org/bot%1/sendMessage").arg(m_token));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setHeader(QNetworkRequest::UserAgentHeader, "BATorrent/notifier");

    QJsonObject body;
    body.insert("chat_id", m_chatId);
    body.insert("text", text);
    body.insert("parse_mode", "Markdown");
    body.insert("disable_web_page_preview", true);

    auto *reply = m_nam->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    // Fire-and-forget: only log failures. Telegram returning 4xx is a config
    // problem the user will discover via the "Test" button — no point in
    // popping a Toast for every torrent that finishes.
    connect(reply, &QNetworkReply::finished, this, [reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            Logger::instance().log(Logger::Warning,
                QStringLiteral("Telegram notifier: %1").arg(reply->errorString()));
        }
        reply->deleteLater();
    });
}

void TelegramNotifier::onTorrentFinished(const QString &name, const QString & /*infoHash*/)
{
    if (!wantsEvent(EventFinished)) return;
    send(QStringLiteral("✅ Download complete: *%1*").arg(name));
}

void TelegramNotifier::onKillSwitchTriggered()
{
    if (!wantsEvent(EventKillSwitch)) return;
    send(QStringLiteral("🚨 Kill switch fired — all torrents paused."));
}

void TelegramNotifier::onRssAutoDownloaded(const QString &feedName, const QString &itemTitle)
{
    if (!wantsEvent(EventRssAuto)) return;
    send(QStringLiteral("📡 RSS auto-added: *%1*\n_from %2_").arg(itemTitle, feedName));
}

void TelegramNotifier::onTorrentError(const QString &message)
{
    if (!wantsEvent(EventError)) return;
    send(QStringLiteral("⚠️ %1").arg(message));
}
