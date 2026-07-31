// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSessionBridge — queue reorder for the current selection.

#include "bridges/qmlsessionbridge.h"
#include "torrent/sessionmanager.h"

#include <QList>

// Resolve the active rows (multi-select, falling back to the focus index).
static QList<int> resolveRows(const QList<int> &rows, int idx)
{
    if (!rows.isEmpty()) return rows;
    return idx >= 0 ? QList<int>{idx} : QList<int>{};
}
void QmlSessionBridge::queueUpSelected()
{
    QList<int> rows = m_selectedRows.isEmpty()
        ? (m_selectedIndex >= 0 ? QList<int>{m_selectedIndex} : QList<int>{})
        : m_selectedRows;
    if (rows.isEmpty()) return;
    std::sort(rows.begin(), rows.end());
    QList<int> newRows;
    for (int r : rows) {
        if (r > 0 && !newRows.contains(r - 1)) {
            m_session->setTorrentQueuePosition(r, r - 1);
            emit queueMoved(r, r - 1);
            newRows << (r - 1);
        } else {
            newRows << r;
        }
    }
    m_selectedRows = newRows;
    m_selectedIndex = newRows.isEmpty() ? -1 : newRows.last();
    // queueMoved already drives QmlPosterModel::moveRow (a real beginMoveRows),
    // so the rows slide smoothly. A queueRefreshNeeded here would re-emit
    // dataChanged for the whole list and reload every cover → full-screen flash.
    emit selectionChanged(); emit selectionListsChanged();
}

void QmlSessionBridge::queueDownSelected()
{
    QList<int> rows = m_selectedRows.isEmpty()
        ? (m_selectedIndex >= 0 ? QList<int>{m_selectedIndex} : QList<int>{})
        : m_selectedRows;
    if (rows.isEmpty()) return;
    std::sort(rows.begin(), rows.end(), std::greater<int>());
    int lastIdx = m_session->torrentCount() - 1;
    QList<int> newRows;
    for (int r : rows) {
        if (r < lastIdx && !newRows.contains(r + 1)) {
            m_session->setTorrentQueuePosition(r, r + 1);
            emit queueMoved(r, r + 1);
            newRows << (r + 1);
        } else {
            newRows << r;
        }
    }
    m_selectedRows = newRows;
    m_selectedIndex = newRows.isEmpty() ? -1 : newRows.first();
    // see queueUpSelected: moveRow handles the visual move; no full refresh.
    emit selectionChanged(); emit selectionListsChanged();
}

void QmlSessionBridge::queueTopSelected()
{
    QList<int> rows = resolveRows(m_selectedRows, m_selectedIndex);
    if (rows.size() == 1 && rows.first() > 0) {
        int r = rows.first();
        m_session->setTorrentQueuePosition(r, 0);
        emit queueMoved(r, 0);                 // smooth move, no flash
        m_selectedRows = {0};
        m_selectedIndex = 0;
    } else {
        for (int r : rows) m_session->setTorrentQueuePosition(r, 0);
        emit queueRefreshNeeded();
    }
    emit selectionChanged(); emit selectionListsChanged();
}

void QmlSessionBridge::queueBottomSelected()
{
    const int last = m_session->torrentCount() - 1;
    QList<int> rows = resolveRows(m_selectedRows, m_selectedIndex);
    if (rows.size() == 1 && rows.first() < last) {
        int r = rows.first();
        m_session->setTorrentQueuePosition(r, last);
        emit queueMoved(r, last);              // smooth move, no flash
        m_selectedRows = {last};
        m_selectedIndex = last;
    } else {
        for (int r : rows) m_session->setTorrentQueuePosition(r, last);
        emit queueRefreshNeeded();
    }
    emit selectionChanged(); emit selectionListsChanged();
}

