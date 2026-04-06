// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef GEOIP_H
#define GEOIP_H

#include <QObject>
#include <QHash>
#include <QQueue>
#include <QTimer>

class QNetworkAccessManager;
class QNetworkReply;

class GeoIpResolver : public QObject
{
    Q_OBJECT
public:
    explicit GeoIpResolver(QObject *parent = nullptr);

    void resolve(const QString &ip);
    QString cachedCountry(const QString &ip) const;

signals:
    void resolved(const QString &ip, const QString &countryCode);

private slots:
    void processQueue();

private:
    QNetworkAccessManager *m_nam;
    QHash<QString, QString> m_cache; // IP -> country code
    QQueue<QString> m_queue;
    QTimer m_rateLimiter;
    bool m_requestInFlight = false;
};

// Convert a 2-letter country code to a Unicode flag emoji string
inline QString countryCodeToFlag(const QString &code)
{
    if (code.length() != 2)
        return {};
    const QChar c0 = code.at(0).toUpper();
    const QChar c1 = code.at(1).toUpper();
    // Regional indicator symbols: U+1F1E6 ('A') to U+1F1FF ('Z')
    char32_t ri0 = 0x1F1E6 + (c0.unicode() - u'A');
    char32_t ri1 = 0x1F1E6 + (c1.unicode() - u'A');
    return QString::fromUcs4(&ri0, 1) + QString::fromUcs4(&ri1, 1);
}

#endif
