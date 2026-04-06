// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef SPLASHWIDGET_H
#define SPLASHWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QPixmap>
#include <QVector>
#include <QPointF>

class QMediaPlayer;
class QAudioOutput;

struct Particle {
    QPointF pos;
    QPointF vel;
    float life;
    float maxLife;
    float size;
    float angle;
};

class SplashWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SplashWidget(QWidget *parent = nullptr);
    ~SplashWidget();
    void start();

signals:
    void finished();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void spawnParticles(int count);

    QTimer m_timer;
    QPixmap m_logo;
    int m_frame = 0;
    int m_totalFrames = 120; // ~4s at 30fps
    bool m_running = false;
    QVector<Particle> m_particles;
    QMediaPlayer *m_player = nullptr;
    QAudioOutput *m_audioOutput = nullptr;
};

#endif
