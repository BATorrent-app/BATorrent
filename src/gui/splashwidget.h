// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef SPLASHWIDGET_H
#define SPLASHWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QPixmap>

class SplashWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SplashWidget(QWidget *parent = nullptr);
    void start();

signals:
    void finished();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QTimer m_timer;
    QPixmap m_logo;
    int m_frame = 0;
    int m_totalFrames = 90; // ~3s at 30fps
    bool m_running = false;
};

#endif
