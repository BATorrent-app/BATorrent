// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSessionBridge — clipboard / smart-paste helpers. Split out of
// qmlsessionbridge.cpp verbatim; no behaviour change.

#include "bridges/session/qmlsessionbridge.h"
#include "torrent/sessionmanager.h"
#include "services/platform/translator.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QThread>
#include <QUrl>
#include <QRegularExpression>
#include <QFileInfo>
#include <QDir>

// On Windows another process (a clipboard manager, the OS) can briefly hold the
// clipboard, and QClipboard::setText then fails silently — reported as "copy
// almost never works". Retry a few times and verify it actually stuck.
static void setClipboardRobust(const QString &text)
{
    if (text.isEmpty()) return;
    QClipboard *cb = QGuiApplication::clipboard();
    for (int i = 0; i < 6; ++i) {
        cb->setText(text);
        if (cb->text() == text) return;
        QThread::msleep(15);
    }
}

void QmlSessionBridge::copyMagnetLink()
{
    if (!hasSelection()) return;
    setClipboardRobust(m_session->torrentMagnetUri(m_selectedIndex));
}

void QmlSessionBridge::copyInfoHash()
{
    if (!hasSelection()) return;
    setClipboardRobust(m_session->torrentHashAt(m_selectedIndex));
}

QString QmlSessionBridge::normalizeClipboardMagnet(const QString &clip)
{
    // Xunlei thunder:// links: base64 of "AA" + the real URL + "ZZ". Decode, then
    // fall through to the normal handling (it usually wraps a magnet).
    QString s = clip;
    if (s.startsWith(QStringLiteral("thunder://"), Qt::CaseInsensitive)) {
        QString dec = QString::fromUtf8(
            QByteArray::fromBase64(s.mid(10).toLatin1())).trimmed();
        if (dec.startsWith(QStringLiteral("AA"), Qt::CaseInsensitive)
            && dec.endsWith(QStringLiteral("ZZ"), Qt::CaseInsensitive))
            dec = dec.mid(2, dec.size() - 4);
        s = dec.trimmed();
        if (s.isEmpty()) return QString();
    }
    if (s.startsWith(QStringLiteral("magnet:"), Qt::CaseInsensitive)
        || s.startsWith(QStringLiteral("bittorrent:"), Qt::CaseInsensitive))
        return s;
    static const QRegularExpression hashRe(QStringLiteral("^[0-9a-fA-F]{40}$"));
    if (hashRe.match(s).hasMatch())
        return QStringLiteral("magnet:?xt=urn:btih:") + s;
    return QString();
}

void QmlSessionBridge::smartPaste()
{
    QString clip = QGuiApplication::clipboard()->text().trimmed();
    if (clip.isEmpty()) { emit toast(tr_("toast_paste_none"), QString()); return; }
    QString magnet = normalizeClipboardMagnet(clip);
    if (!magnet.isEmpty()) {
        addMagnetUri(magnet);
        // confirm always: past the active-download limit the torrent is queued
        // (not visibly at the top), so without this the paste looks ignored
        emit toast(tr_("toast_magnet_added"), QString());
        return;
    }
    if (clip.endsWith(QStringLiteral(".torrent"), Qt::CaseInsensitive)
        && (clip.startsWith(QStringLiteral("http"), Qt::CaseInsensitive)
            || clip.startsWith(QStringLiteral("file:"), Qt::CaseInsensitive))) {
        addTorrentFile(clip);
        return;
    }
    emit toast(tr_("toast_paste_none"), QString());
}

QString QmlSessionBridge::clipboardMagnetIfNew()
{
    QString clip = QGuiApplication::clipboard()->text().trimmed();
    if (clip.isEmpty()) return QString();
    QString magnet = normalizeClipboardMagnet(clip);
    if (magnet.isEmpty() || magnet == m_lastClipboardMagnet) return QString();
    m_lastClipboardMagnet = magnet;
    // never offer a magnet that's already in the session (added moments ago
    // via drop/click while its link still sits on the clipboard)
    static const QRegularExpression btih(QStringLiteral("btih:([0-9A-Fa-f]{40})"));
    const auto m = btih.match(magnet);
    if (m.hasMatch() && m_session->torrentIndexByInfoHash(m.captured(1).toLower()) >= 0)
        return QString();
    return magnet;
}

void QmlSessionBridge::copySelectedName()
{
    if (!hasSelection()) return;
    setClipboardRobust(m_session->torrentAt(m_selectedIndex).name);
}

void QmlSessionBridge::copySelectedContentPath()
{
    if (!hasSelection()) return;
    const QString path = m_session->torrentRootPath(m_selectedIndex);
    if (path.isEmpty()) return;
    setClipboardRobust(QDir::toNativeSeparators(path));
    emit toast(tr_("ctx_path_copied"), QDir::toNativeSeparators(path));
}

void QmlSessionBridge::copyToClipboard(const QString &text)
{
    setClipboardRobust(text);
}

