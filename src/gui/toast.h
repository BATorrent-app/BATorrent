// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef TOAST_H
#define TOAST_H

#include <QWidget>
#include <QPixmap>
#include <QList>

class QPropertyAnimation;
class QTimer;

class Toast : public QWidget
{
    Q_OBJECT
public:
    enum Kind { Info, Success, Warning, Error };

    static void notify(const QString &title, const QString &body,
                       Kind kind = Info,
                       const QObject *receiver = nullptr,
                       const char *clickedSlot = nullptr);

signals:
    void clicked();

protected:
    void paintEvent(QPaintEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void enterEvent(QEnterEvent *e) override;
    void leaveEvent(QEvent *e) override;

private:
    Toast(const QString &title, const QString &body, Kind kind);
    void dismiss();
    static void restack();

    QString m_title;
    QString m_body;
    Kind m_kind;
    QPixmap m_logo;
    QTimer *m_dismissTimer;
    QPropertyAnimation *m_fadeAnim;
    static QList<Toast *> s_active;
};

#endif
