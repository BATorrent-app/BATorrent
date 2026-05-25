// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef POSTERVIEW_H
#define POSTERVIEW_H

#include <QListView>
#include <QStyledItemDelegate>

class MetadataResolver;
class SessionManager;

class PosterDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit PosterDelegate(MetadataResolver *resolver, SessionManager *session,
                            QObject *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;

private:
    MetadataResolver *m_resolver;
    SessionManager *m_session;
    mutable QHash<QString, QPixmap> m_pixmapCache;
};

class PosterView : public QListView
{
    Q_OBJECT
public:
    explicit PosterView(MetadataResolver *resolver, SessionManager *session,
                        QWidget *parent = nullptr);

signals:
    void torrentSelected(int sourceRow);
    void torrentDoubleClicked(int sourceRow);

protected:
    void selectionChanged(const QItemSelection &selected,
                          const QItemSelection &deselected) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
};

#endif
