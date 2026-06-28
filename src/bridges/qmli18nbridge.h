// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef QMLI18NBRIDGE_H
#define QMLI18NBRIDGE_H

#include "bridges/bridgecommon.h"

class QmlI18nBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int language READ language WRITE setLanguage NOTIFY languageChanged)
public:
    explicit QmlI18nBridge(QObject *parent = nullptr) : QObject(parent) {}
    Q_INVOKABLE QString t(const QString &key) const { return tr_(key); }
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
