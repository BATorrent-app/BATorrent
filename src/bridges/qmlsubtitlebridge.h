// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef QMLSUBTITLEBRIDGE_H
#define QMLSUBTITLEBRIDGE_H

#include "bridges/bridgecommon.h"

class QmlSubtitleBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool searching READ searching NOTIFY searchingChanged)
    Q_PROPERTY(QVariantList results READ results NOTIFY resultsChanged)
public:
    explicit QmlSubtitleBridge(IEngine *session, QObject *parent = nullptr);

    bool searching() const { return m_searching; }
    QVariantList results() const;
    Q_INVOKABLE void searchFor(const QString &infoHash, int fileIndex, const QString &mediaTitle,
                               const QStringList &langs = {});   // empty langs ⇒ UI language + English
    Q_INVOKABLE void download(int index);
    Q_INVOKABLE bool hasOpenSubtitlesKey() const;
    void setResolver(MetadataResolver *r) { m_resolver = r; }

signals:
    void searchingChanged();
    void resultsChanged();
    void subtitleReady(const QString &path);
    void searchError(const QString &message);

private:
    IEngine *m_session;
    SubtitleSearch *m_search;
    MetadataResolver *m_resolver = nullptr;
    QString m_targetDir;
    QString m_baseName;
    bool m_searching = false;
};

#endif // QMLSUBTITLEBRIDGE_H
