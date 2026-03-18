#include "splashwidget.h"
#include "thememanager.h"
#include <QPainter>
#include <QRadialGradient>
#include <QtMath>

SplashWidget::SplashWidget(QWidget *parent)
    : QWidget(parent)
{
    m_logo = QPixmap(":/images/logo1.png");

    connect(&m_timer, &QTimer::timeout, this, [this]() {
        if (!m_running) return;
        m_frame++;

        if (m_frame >= m_totalFrames) {
            m_running = false;
            m_timer.stop();
            emit finished();
            return;
        }
        update();
    });
}

void SplashWidget::start()
{
    m_frame = 0;
    m_running = true;
    m_timer.start(33);
}

void SplashWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    int w = width();
    int h = height();

    const auto &tm = ThemeManager::instance();

    // --- Background ---
    // Solid during intro, fades out at the end
    float bgAlpha = 1.0f;
    if (m_frame > 70)
        bgAlpha = qMax(0.0f, 1.0f - (m_frame - 70.0f) / 20.0f);

    QColor bg(tm.bgColor());
    bg.setAlphaF(bgAlpha);
    p.fillRect(rect(), bg);

    if (!m_running && m_frame >= m_totalFrames)
        return;

    // --- Logo ---
    // Phase 1 (0-30): fade in + scale from 0.7 to 1.0
    // Phase 2 (30-60): hold + subtle glow
    // Phase 3 (60-90): fade out everything

    float logoAlpha = 0.0f;
    float logoScale = 1.0f;

    if (m_frame < 30) {
        // Fade in
        float ft = m_frame / 30.0f;
        float ease = ft * ft * (3.0f - 2.0f * ft); // smoothstep
        logoAlpha = ease;
        logoScale = 0.7f + 0.3f * ease;
    } else if (m_frame < 60) {
        // Hold
        logoAlpha = 1.0f;
        logoScale = 1.0f;
    } else {
        // Fade out
        float ft = (m_frame - 60.0f) / 30.0f;
        logoAlpha = 1.0f - ft;
        logoScale = 1.0f + 0.05f * ft;
    }

    if (logoAlpha <= 0) return;

    // Draw logo PNG centered
    int logoSize = static_cast<int>(180 * logoScale);
    QPixmap scaled = m_logo.scaled(logoSize, logoSize,
                                    Qt::KeepAspectRatio,
                                    Qt::SmoothTransformation);
    p.setOpacity(logoAlpha);
    int lx = (w - scaled.width()) / 2;
    int ly = (h - scaled.height()) / 2 - 20;
    p.drawPixmap(lx, ly, scaled);

    // Draw "BATorrent" text below logo
    float textAlpha = logoAlpha;
    if (m_frame < 35)
        textAlpha = qMax(0.0f, (m_frame - 20.0f) / 15.0f);

    if (textAlpha > 0) {
        p.setOpacity(textAlpha);
        QColor textColor(tm.textColor());
        p.setPen(textColor);
        QFont f = font();
        f.setPointSize(22);
        f.setWeight(QFont::Bold);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 3);
        p.setFont(f);
        p.drawText(QRect(0, ly + logoSize + 8, w, 40),
                    Qt::AlignHCenter, "BATorrent");

        // Subtitle
        f.setPointSize(10);
        f.setWeight(QFont::Normal);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 1);
        p.setFont(f);
        QColor subColor(tm.mutedColor());
        p.setPen(subColor);
        p.drawText(QRect(0, ly + logoSize + 42, w, 25),
                    Qt::AlignHCenter, "Lightweight BitTorrent Client");
    }

    p.setOpacity(1.0f);
}
