// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef SUBTITLESEARCH_H
#define SUBTITLESEARCH_H

#include <QList>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QStringList>

struct SubtitleResult {
    QString provider;      // "OpenSubtitles" | "Gestdown"
    QString name;          // release / version label
    QString language;      // display code ("pt-BR", "en")
    QString downloadRef;   // provider-specific handle
};

// Subtitle auto-download for the built-in player.
// Providers: OpenSubtitles.com REST (app API key + search by parsed title/S/E,
// downloads count against the anonymous 5/day/IP quota unless the user logs in
// later) and Gestdown (keyless Addic7ed proxy, series only). Results merge as
// each provider answers; download saves a language-suffixed sidecar next to
// the video so the player's auto-detect finds it on the next open too.
class SubtitleSearch : public QObject
{
    Q_OBJECT
public:
    explicit SubtitleSearch(QObject *parent = nullptr);

    void search(const QString &videoName, const QStringList &languages);
    void download(int index, const QString &targetDir, const QString &videoBaseName);
    QList<SubtitleResult> results() const { return m_results; }
    bool busy() const { return m_pending > 0; }

signals:
    void resultsChanged();
    void searchFinished();
    void downloadFinished(const QString &path);
    void errorOccurred(const QString &message);

private:
    void searchOpenSubtitles(const QString &title, int season, int episode, const QStringList &langs);
    void searchGestdown(const QString &title, int season, int episode, const QStringList &langs);
    void downloadOpenSubtitles(const SubtitleResult &r, const QString &savePath);
    void downloadGestdown(const SubtitleResult &r, const QString &savePath);
    void providerDone();
    static QString openSubtitlesKey();

    QNetworkAccessManager m_nam;
    QList<SubtitleResult> m_results;
    int m_pending = 0;
};

#endif
