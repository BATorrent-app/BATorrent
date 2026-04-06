// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "splashwidget.h"
#include "thememanager.h"
#include <QPainter>
#include <QRadialGradient>
#include <QLinearGradient>
#include <QtMath>
#include <QRandomGenerator>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QUrl>

SplashWidget::SplashWidget(QWidget *parent)
    : QWidget(parent)
{
    m_logo = QPixmap(":/images/logo1.png");

    // Audio setup
    m_audioOutput = new QAudioOutput(this);
    m_audioOutput->setVolume(0.6f);
    m_player = new QMediaPlayer(this);
    m_player->setAudioOutput(m_audioOutput);
    m_player->setSource(QUrl("qrc:/sounds/batsound.mp3"));

    connect(&m_timer, &QTimer::timeout, this, [this]() {
        if (!m_running) return;
        m_frame++;

        // Spawn particles during specific frames
        if (m_frame == 15)
            spawnParticles(24);  // burst when logo appears
        if (m_frame == 45)
            spawnParticles(12);  // second burst at peak glow
        if (m_frame > 15 && m_frame < 80 && m_frame % 8 == 0)
            spawnParticles(3);   // ambient particles

        // Update particles
        for (auto &pt : m_particles) {
            pt.pos += pt.vel;
            pt.vel *= 0.97f;  // drag
            pt.life -= 1.0f;
            pt.angle += 2.0f;
        }
        // Remove dead particles
        m_particles.erase(
            std::remove_if(m_particles.begin(), m_particles.end(),
                           [](const Particle &p) { return p.life <= 0; }),
            m_particles.end());

        if (m_frame >= m_totalFrames) {
            m_running = false;
            m_timer.stop();
            emit finished();
            return;
        }
        update();
    });
}

SplashWidget::~SplashWidget()
{
    m_player->stop();
}

void SplashWidget::start()
{
    m_frame = 0;
    m_running = true;
    m_particles.clear();
    m_timer.start(33);
    m_player->play();
}

void SplashWidget::spawnParticles(int count)
{
    auto *rng = QRandomGenerator::global();
    float cx = width() / 2.0f;
    float cy = height() / 2.0f - 20.0f;

    for (int i = 0; i < count; i++) {
        Particle p;
        float angle = rng->bounded(360) * (M_PI / 180.0f);
        float speed = 1.5f + rng->bounded(4.0);
        p.pos = QPointF(cx + rng->bounded(40) - 20, cy + rng->bounded(40) - 20);
        p.vel = QPointF(qCos(angle) * speed, qSin(angle) * speed);
        p.maxLife = 30.0f + rng->bounded(40);
        p.life = p.maxLife;
        p.size = 2.0f + rng->bounded(4.0);
        p.angle = rng->bounded(360.0);
        m_particles.append(p);
    }
}

void SplashWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    int w = width();
    int h = height();
    float cx = w / 2.0f;
    float cy = h / 2.0f - 20.0f;

    const auto &tm = ThemeManager::instance();

    // ── Background ──
    float bgAlpha = 1.0f;
    if (m_frame > 95)
        bgAlpha = qMax(0.0f, 1.0f - (m_frame - 95.0f) / 25.0f);

    QColor bgColor(tm.bgColor());
    bgColor.setAlphaF(bgAlpha);
    p.fillRect(rect(), bgColor);

    if (!m_running && m_frame >= m_totalFrames)
        return;

    // ── Radial glow behind logo ──
    // Builds up from frame 10, pulses, then fades
    float glowIntensity = 0.0f;
    if (m_frame < 10) {
        glowIntensity = 0.0f;
    } else if (m_frame < 40) {
        float ft = (m_frame - 10.0f) / 30.0f;
        glowIntensity = ft * ft;
    } else if (m_frame < 80) {
        // Pulse between 0.8 and 1.0
        float pulse = 0.9f + 0.1f * qSin(m_frame * 0.15f);
        glowIntensity = pulse;
    } else {
        float ft = (m_frame - 80.0f) / 40.0f;
        glowIntensity = qMax(0.0f, 1.0f - ft);
    }

    if (glowIntensity > 0) {
        QRadialGradient glow(cx, cy, 200);
        QColor red(220, 38, 38);
        red.setAlphaF(0.25f * glowIntensity * bgAlpha);
        glow.setColorAt(0.0, red);
        red.setAlphaF(0.08f * glowIntensity * bgAlpha);
        glow.setColorAt(0.5, red);
        glow.setColorAt(1.0, Qt::transparent);
        p.setPen(Qt::NoPen);
        p.setBrush(glow);
        p.drawEllipse(QPointF(cx, cy), 200, 200);
    }

    // ── Sonar rings ──
    // Expanding rings from center at key moments
    auto drawRing = [&](int spawnFrame, float maxRadius, float duration) {
        if (m_frame < spawnFrame || m_frame > spawnFrame + duration) return;
        float ft = (m_frame - spawnFrame) / duration;
        float radius = maxRadius * ft;
        float alpha = (1.0f - ft) * 0.3f * bgAlpha;
        QColor ringColor(220, 38, 38);
        ringColor.setAlphaF(alpha);
        p.setPen(QPen(ringColor, 1.5));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QPointF(cx, cy), radius, radius);
    };

    drawRing(15, 300, 40);  // first ring on logo appear
    drawRing(25, 250, 35);  // second ring delayed
    drawRing(45, 280, 38);  // third ring at peak

    // ── Particles ──
    for (const auto &pt : m_particles) {
        float lifeRatio = pt.life / pt.maxLife;
        float alpha = lifeRatio * 0.8f * bgAlpha;
        QColor particleColor(220, 38, 38);

        // Some particles are white/chrome
        if (static_cast<int>(pt.angle) % 3 == 0)
            particleColor = QColor(200, 200, 210);

        particleColor.setAlphaF(alpha);
        p.setPen(Qt::NoPen);
        p.setBrush(particleColor);

        float sz = pt.size * lifeRatio;
        p.drawEllipse(pt.pos, sz, sz);
    }

    // ── Logo ──
    // Phase 1 (0-20): dark, nothing
    // Phase 2 (10-35): zoom in from 0.3 to 1.05 with bounce
    // Phase 3 (35-45): settle to 1.0 with slight overshoot
    // Phase 4 (45-90): hold with subtle breathing
    // Phase 5 (90-120): fade out + slight zoom

    float logoAlpha = 0.0f;
    float logoScale = 1.0f;

    if (m_frame < 10) {
        logoAlpha = 0.0f;
    } else if (m_frame < 35) {
        // Zoom in with elastic ease
        float ft = (m_frame - 10.0f) / 25.0f;
        // Elastic ease out approximation
        float ease = 1.0f - qPow(2.0f, -10.0f * ft) * qCos(ft * 12.0f);
        logoAlpha = qMin(1.0f, ft * 2.0f);
        logoScale = 0.3f + 0.75f * ease;
    } else if (m_frame < 45) {
        // Settle
        float ft = (m_frame - 35.0f) / 10.0f;
        logoAlpha = 1.0f;
        logoScale = 1.05f - 0.05f * ft;
    } else if (m_frame < 90) {
        // Hold with subtle breathing
        logoAlpha = 1.0f;
        float breath = qSin(m_frame * 0.08f) * 0.015f;
        logoScale = 1.0f + breath;
    } else {
        // Fade out with zoom
        float ft = (m_frame - 90.0f) / 30.0f;
        logoAlpha = qMax(0.0f, 1.0f - ft * 1.2f);
        logoScale = 1.0f + 0.15f * ft;
    }

    if (logoAlpha > 0) {
        int logoSize = static_cast<int>(200 * logoScale);
        QPixmap scaled = m_logo.scaled(logoSize, logoSize,
                                        Qt::KeepAspectRatio,
                                        Qt::SmoothTransformation);
        p.setOpacity(logoAlpha * bgAlpha);
        int lx = (w - scaled.width()) / 2;
        int ly = static_cast<int>(cy) - scaled.height() / 2;
        p.drawPixmap(lx, ly, scaled);
    }

    // ── "BATorrent" text ──
    // Slides up from below, appears after logo settles
    float textAlpha = 0.0f;
    float textOffset = 0.0f;

    if (m_frame < 30) {
        textAlpha = 0.0f;
    } else if (m_frame < 50) {
        float ft = (m_frame - 30.0f) / 20.0f;
        float ease = ft * ft * (3.0f - 2.0f * ft);
        textAlpha = ease;
        textOffset = 20.0f * (1.0f - ease);
    } else if (m_frame < 90) {
        textAlpha = 1.0f;
    } else {
        float ft = (m_frame - 90.0f) / 30.0f;
        textAlpha = qMax(0.0f, 1.0f - ft * 1.5f);
        textOffset = -10.0f * ft;
    }

    if (textAlpha > 0) {
        p.setOpacity(textAlpha * bgAlpha);

        // Title
        QColor textColor(tm.textColor());
        p.setPen(textColor);
        QFont f = font();
        f.setPointSize(24);
        f.setWeight(QFont::Black);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 5);
        p.setFont(f);

        int textY = static_cast<int>(cy + 120 + textOffset);
        p.drawText(QRect(0, textY, w, 40), Qt::AlignHCenter, "BATORRENT");

        // Red accent line under title
        if (m_frame >= 38) {
            float lineProgress = qMin(1.0f, (m_frame - 38.0f) / 15.0f);
            int lineW = static_cast<int>(80 * lineProgress);
            QColor lineColor(220, 38, 38);
            lineColor.setAlphaF(textAlpha * bgAlpha);
            p.setPen(Qt::NoPen);
            p.setBrush(lineColor);
            p.drawRoundedRect(static_cast<int>(cx) - lineW / 2, textY + 38,
                             lineW, 3, 1.5, 1.5);
        }

        // Subtitle - slides in later
        float subAlpha = 0.0f;
        float subOffset = 0.0f;
        if (m_frame >= 42 && m_frame < 90) {
            float ft = qMin(1.0f, (m_frame - 42.0f) / 18.0f);
            float ease = ft * ft * (3.0f - 2.0f * ft);
            subAlpha = ease;
            subOffset = 15.0f * (1.0f - ease);
        } else if (m_frame >= 90) {
            float ft = (m_frame - 90.0f) / 25.0f;
            subAlpha = qMax(0.0f, 1.0f - ft * 1.8f);
        }

        if (subAlpha > 0) {
            p.setOpacity(subAlpha * bgAlpha);
            f.setPointSize(10);
            f.setWeight(QFont::Normal);
            f.setLetterSpacing(QFont::AbsoluteSpacing, 2);
            p.setFont(f);
            QColor subColor(tm.mutedColor());
            p.setPen(subColor);
            p.drawText(QRect(0, textY + 48 + static_cast<int>(subOffset), w, 25),
                        Qt::AlignHCenter, "Lightweight BitTorrent Client");
        }
    }

    p.setOpacity(1.0f);
}
