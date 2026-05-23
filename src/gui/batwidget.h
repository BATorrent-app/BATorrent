// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef BATWIDGET_H
#define BATWIDGET_H

#include <QWidget>
#include <QPixmap>

class QLabel;

class BatWidget : public QWidget
{
    Q_OBJECT
public:
    explicit BatWidget(QWidget *parent = nullptr);
    // Override the default title/body. Pass empty strings to revert to the
    // built-in welcome copy. Used by MainWindow to show context-specific
    // empty states per filter ("No completed torrents yet" etc.).
    void setCustomMessage(const QString &title, const QString &body);

signals:
    void openFileRequested();
    void pasteMagnetRequested();
    void openSearchRequested();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QPixmap m_logo;
    int m_circleSize = 132;
    QLabel *m_titleLabel = nullptr;
    QLabel *m_bodyLabel = nullptr;
    QString m_defaultTitle;
    QString m_defaultBody;
};

#endif
