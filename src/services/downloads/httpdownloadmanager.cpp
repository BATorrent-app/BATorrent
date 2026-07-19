// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/downloads/httpdownloadmanager.h"

#include <QTimer>
#include <QDir>
#include <QSaveFile>
#include <QFile>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QUuid>

namespace {
constexpr int kSaveDebounceMs = 1500;   // coalesce bursts of progress writes
}

HttpDownloadManager::HttpDownloadManager(QObject *parent)
    : QObject(parent)
{
    m_saveTimer = new QTimer(this);
    m_saveTimer->setSingleShot(true);
    m_saveTimer->setInterval(kSaveDebounceMs);
    connect(m_saveTimer, &QTimer::timeout, this, [this]() { save(); });

    m_defaultDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    load();
}

HttpDownloadManager::~HttpDownloadManager()
{
    if (m_saveTimer && m_saveTimer->isActive()) save();   // flush pending checkpoint
}

QString HttpDownloadManager::storePath() const
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(base + QStringLiteral("/downloads"));
    return base + QStringLiteral("/downloads/http.json");
}

HttpDownload *HttpDownloadManager::at(int i) const
{
    return (i >= 0 && i < m_items.size()) ? m_items[i] : nullptr;
}

HttpDownload *HttpDownloadManager::byId(const QString &id) const
{
    for (HttpDownload *d : m_items)
        if (d->id() == id) return d;
    return nullptr;
}

void HttpDownloadManager::wire(HttpDownload *d)
{
    connect(d, &HttpDownload::progressed, this, [this]() { emit changed(); });
    connect(d, &HttpDownload::stateChanged, this, [this]() {
        scheduleSave();
        emit changed();
    });
    connect(d, &HttpDownload::finished, this, [this, d](bool ok) {
        scheduleSave();
        emit downloadFinished(d->id(), d->fileName(), ok);
        emit changed();
    });
}

QString HttpDownloadManager::add(const QUrl &url, const QString &saveDir, const QString &fileName)
{
    const QString id = QUuid::createUuid().toString(QUuid::Id128);
    const QString dir = saveDir.isEmpty() ? m_defaultDir : saveDir;
    auto *d = new HttpDownload(id, url, dir, fileName, this);
    m_items.push_back(d);
    wire(d);
    d->start();
    scheduleSave();
    emit changed();
    return id;
}

void HttpDownloadManager::pause(const QString &id)
{
    if (HttpDownload *d = byId(id)) { d->pause(); emit changed(); }
}

void HttpDownloadManager::resume(const QString &id)
{
    if (HttpDownload *d = byId(id)) { d->resume(); emit changed(); }
}

void HttpDownloadManager::remove(const QString &id, bool deleteFiles)
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i]->id() != id) continue;
        HttpDownload *d = m_items[i];
        d->cancel(deleteFiles);
        if (deleteFiles) QFile::remove(d->finalPath());   // in case it already finalized
        m_items.remove(i);
        d->deleteLater();
        scheduleSave();
        emit changed();
        return;
    }
}

void HttpDownloadManager::scheduleSave()
{
    if (m_saveTimer && !m_saveTimer->isActive()) m_saveTimer->start();
}

void HttpDownloadManager::save() const
{
    QJsonArray arr;
    for (const HttpDownload *d : m_items) {
        QVariantMap m = d->toState();
        arr.append(QJsonObject::fromVariantMap(m));
    }
    QSaveFile f(storePath());
    if (!f.open(QIODevice::WriteOnly)) return;
    f.write(QJsonDocument(arr).toJson(QJsonDocument::Compact));
    f.commit();
}

void HttpDownloadManager::load()
{
    QFile f(storePath());
    if (!f.open(QIODevice::ReadOnly)) return;
    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isArray()) {
        // Corrupt sidecar: quarantine it so the next boot starts clean rather
        // than hitting the same parse failure forever (house pattern).
        f.close();
        QFile::rename(storePath(), storePath() + QStringLiteral(".corrupt"));
        return;
    }
    for (const QJsonValue &v : doc.array()) {
        const QVariantMap m = v.toObject().toVariantMap();
        const QString id = m.value(QStringLiteral("id")).toString();
        const QUrl url(m.value(QStringLiteral("url")).toString());
        const QString saveDir = m.value(QStringLiteral("saveDir")).toString();
        const QString fileName = m.value(QStringLiteral("fileName")).toString();
        if (id.isEmpty() || !url.isValid()) continue;
        auto *d = new HttpDownload(id, url, saveDir, fileName, this);
        d->restoreState(m);   // Completed stays completed; in-flight comes back Paused
        m_items.push_back(d);
        wire(d);
    }
}
