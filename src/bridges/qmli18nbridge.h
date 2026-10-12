// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef QMLI18NBRIDGE_H
#define QMLI18NBRIDGE_H

#include "bridges/bridgecommon.h"

#include <QDateTime>

class QmlI18nBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int language READ language WRITE setLanguage NOTIFY languageChanged)
public:
    explicit QmlI18nBridge(QObject *parent = nullptr) : QObject(parent) {}
    Q_INVOKABLE QString t(const QString &key) const { return tr_(key); }
    // A day in the app's language ("28 set. 2026"). Qt.formatDate in QML uses
    // the system's locale, so a Portuguese UI showed "28 Sep 2026".
    Q_INVOKABLE QString date(qint64 secsSinceEpoch) const {
        return Translator::instance().locale().toString(
            QDateTime::fromSecsSinceEpoch(secsSinceEpoch).date(), QStringLiteral("d MMM yyyy"));
    }
    int language() const { return static_cast<int>(Translator::instance().language()); }
    Q_INVOKABLE void setLanguage(int lang) {
        if (lang == language()) return;
        Translator::instance().setLanguage(static_cast<Translator::Language>(lang));
        QSettings().setValue("language", lang);
        emit languageChanged();
    }
signals:
    void languageChanged();
};

#endif // QMLI18NBRIDGE_H
