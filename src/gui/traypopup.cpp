// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "traypopup.h"
#include "thememanager.h"
#include "../app/translator.h"
#include "../torrent/sessionmanager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QFrame>
#include <QScreen>
#include <QGuiApplication>
#include <QGraphicsDropShadowEffect>

ToggleSwitch::ToggleSwitch(QWidget *parent) : QFrame(parent)
{
    setFixedSize(28, 16);
    setCursor(Qt::PointingHandCursor);
}

void ToggleSwitch::setOn(bool on)
{
    if (m_on == on) return;
    m_on = on;
    update();
    emit toggled(m_on);
}

void ToggleSwitch::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) setOn(!m_on);
    QFrame::mousePressEvent(e);
}

void ToggleSwitch::paintEvent(QPaintEvent *)
{
    const auto &tm = ThemeManager::instance();
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QColor bg(m_on ? tm.accentColor() : tm.surfaceAltColor());
    QColor border(m_on ? tm.accentColor() : tm.borderColor());
    QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    p.setPen(QPen(border, 1));
    p.setBrush(bg);
    p.drawRoundedRect(r, 8, 8);

    const int knobX = m_on ? 13 : 1;
    QRectF knob(knobX, 1, 12, 12);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#ffffff"));
    p.drawEllipse(knob);
}

namespace {

QLabel *makeStatusDot(const QColor &color, int size, QWidget *parent)
{
    auto *dot = new QLabel(parent);
    dot->setFixedSize(size, size);
    QPixmap pm(size * 2, size * 2);
    pm.fill(Qt::transparent);
    pm.setDevicePixelRatio(2.0);
    {
        QPainter pp(&pm);
        pp.setRenderHint(QPainter::Antialiasing);
        pp.setPen(Qt::NoPen);
        pp.setBrush(color);
        pp.drawEllipse(QRectF(0, 0, size, size));
    }
    dot->setPixmap(pm);
    return dot;
}

class ActiveRow : public QWidget
{
public:
    ActiveRow(const QString &name, float progress,
              int downloadRate, qint64 remainingBytes,
              QWidget *parent = nullptr)
        : QWidget(parent)
    {
        const auto &tm = ThemeManager::instance();
        auto *col = new QVBoxLayout(this);
        col->setContentsMargins(0, 6, 0, 6);
        col->setSpacing(2);

        auto *line = new QHBoxLayout;
        line->setSpacing(6);
        auto *nameLbl = new QLabel(name);
        nameLbl->setStyleSheet(QString("color: %1; font-size: 10px; background: transparent;")
            .arg(tm.textColor()));
        nameLbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        // Truncate long names with ellipsis instead of wrapping.
        QFontMetrics fm(nameLbl->font());
        nameLbl->setText(fm.elidedText(name, Qt::ElideRight, 200));
        line->addWidget(nameLbl, 1);

        auto *pct = new QLabel(QString::number(int(progress * 100)) + "%");
        QFont f("Menlo");
        f.setStyleHint(QFont::Monospace);
        f.setPointSize(9);
        pct->setFont(f);
        pct->setStyleSheet(QString("color: %1; background: transparent;")
            .arg(tm.mutedColor()));
        line->addWidget(pct);
        col->addLayout(line);

        // Sub-line: live download rate + ETA. Skipped when the torrent is
        // either complete or stalled so we don't waste vertical space on
        // "0 B/s · ∞" rows.
        if (progress < 1.0f && downloadRate > 0) {
            auto *meta = new QLabel(QString("↓ %1 · ETA %2")
                .arg(formatSpeed(downloadRate), formatEta(downloadRate, remainingBytes)));
            QFont mf("Menlo");
            mf.setStyleHint(QFont::Monospace);
            mf.setPointSize(8);
            meta->setFont(mf);
            meta->setStyleSheet(QString("color: %1; background: transparent;")
                .arg(tm.dimColor()));
            col->addWidget(meta);
        }

        m_progress = progress;
    }

private:
    static QString formatSpeed(int bps) {
        if (bps < 1024) return QString::number(bps) + " B/s";
        if (bps < 1024 * 1024) return QString::number(bps / 1024) + " KB/s";
        return QString::number(bps / 1048576.0, 'f', 1) + " MB/s";
    }
    static QString formatEta(int rate, qint64 remaining) {
        if (rate <= 0 || remaining <= 0) return QStringLiteral("∞");
        qint64 secs = remaining / rate;
        if (secs < 60)    return QString::number(secs) + "s";
        if (secs < 3600)  return QString::number(secs / 60) + "m";
        if (secs < 86400) return QString::number(secs / 3600) + "h "
                                + QString::number((secs % 3600) / 60) + "m";
        return QString::number(secs / 86400) + "d";
    }
public:

protected:
    void paintEvent(QPaintEvent *) override {
        const auto &tm = ThemeManager::instance();
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        // 3px progress bar at the bottom of the row
        const int barH = 3;
        const QRectF track(0, height() - barH - 2, width(), barH);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(tm.surfaceColor()));
        p.drawRoundedRect(track, 2, 2);
        QRectF fill = track;
        fill.setWidth(track.width() * m_progress);
        p.setBrush(QColor(tm.stateDownloadingColor()));
        p.drawRoundedRect(fill, 2, 2);
    }

private:
    float m_progress;
};

// Clickable menu row mirroring TrayItem in misc.jsx — icon + label + optional
// sub-text + optional shortcut keycap + optional toggle. No real "icon" — we
// keep a simple bullet-style monogram since the existing app icons are full
// color SVGs that would clash at 13px in this density.
class MenuRow : public QFrame
{
public:
    MenuRow(const QString &label, const QString &shortcut,
            bool danger, QWidget *parent = nullptr)
        : QFrame(parent), m_danger(danger)
    {
        const auto &tm = ThemeManager::instance();
        setObjectName(QStringLiteral("trayRow"));
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_StyledBackground);
        applyStyle(false);

        auto *row = new QHBoxLayout(this);
        row->setContentsMargins(14, 7, 12, 7);
        row->setSpacing(10);

        auto *labelLbl = new QLabel(label);
        labelLbl->setStyleSheet(QString("color: %1; font-size: 11px; background: transparent;")
            .arg(danger ? tm.accentLightColor() : tm.textColor()));
        row->addWidget(labelLbl, 1);

        if (!shortcut.isEmpty()) {
            auto *sc = new QLabel(shortcut);
            QFont f("Menlo");
            f.setStyleHint(QFont::Monospace);
            f.setPointSize(9);
            sc->setFont(f);
            sc->setStyleSheet(QString("color: %1; background: transparent;")
                .arg(tm.dimColor()));
            row->addWidget(sc);
        }

        m_row = row;
    }

    QLabel *addSub(const QString &text, const QColor &color)
    {
        auto *sub = new QLabel(text);
        QFont f; f.setPointSize(9); f.setWeight(QFont::DemiBold);
        sub->setFont(f);
        sub->setStyleSheet(QString("color: %1; background: transparent;")
            .arg(color.name()));
        // Insert before the trailing shortcut label if any (or at end).
        m_row->insertWidget(m_row->count() - (m_row->count() > 2 ? 1 : 0), sub);
        return sub;
    }

    void setTrailing(QWidget *w) {
        m_row->addWidget(w);
    }

    void onClick(std::function<void()> handler) { m_handler = std::move(handler); }

protected:
    void enterEvent(QEnterEvent *e) override {
        applyStyle(true);
        QFrame::enterEvent(e);
    }
    void leaveEvent(QEvent *e) override {
        applyStyle(false);
        QFrame::leaveEvent(e);
    }
    void mousePressEvent(QMouseEvent *e) override {
        if (e->button() == Qt::LeftButton && m_handler) m_handler();
        QFrame::mousePressEvent(e);
    }

private:
    void applyStyle(bool hover) {
        const auto &tm = ThemeManager::instance();
        setStyleSheet(QString(
            "QFrame#trayRow { background: %1; border: none; }"
            ).arg(hover ? tm.surfaceAltColor() : QStringLiteral("transparent")));
    }

    QHBoxLayout *m_row;
    bool m_danger;
    std::function<void()> m_handler;
};

QFrame *makeDivider(QWidget *parent)
{
    const auto &tm = ThemeManager::instance();
    auto *line = new QFrame(parent);
    line->setFixedHeight(1);
    line->setStyleSheet(QString("background: %1; border: none;").arg(tm.hairlineColor()));
    return line;
}

} // namespace

TrayPopup::TrayPopup(SessionManager *session, QWidget *parent)
    : QDialog(parent), m_session(session)
{
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedWidth(280);

    // Live-refresh speeds/active count while the popup is open. We only
    // listen while visible — connecting permanently would call refresh()
    // every poll tick (~1s) wasting cycles when the popup is hidden.
    connect(m_session, &SessionManager::torrentsUpdated, this, [this]() {
        if (isVisible()) refresh();
    });

    const auto &tm = ThemeManager::instance();

    // Outer wrapper: rounded panel with shadow. The QDialog itself is
    // transparent (WA_TranslucentBackground); we paint the rounded card via
    // an internal QFrame so the corners render cleanly cross-platform.
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(8, 8, 8, 8);
    outer->setSpacing(0);

    auto *card = new QFrame(this);
    card->setObjectName(QStringLiteral("trayCard"));
    card->setStyleSheet(QString(
        "QFrame#trayCard {"
        "  background: %1;"
        "  border: 1px solid %2;"
        "  border-radius: 10px;"
        "}"
        ).arg(tm.surfaceColor(), tm.borderColor()));
    auto *shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(40);
    shadow->setOffset(0, 16);
    shadow->setColor(QColor(0, 0, 0, 128));
    card->setGraphicsEffect(shadow);
    outer->addWidget(card);

    auto *inner = new QVBoxLayout(card);
    inner->setContentsMargins(0, 0, 0, 0);
    inner->setSpacing(0);

    // ── Header: logo + name + count + status dot ──
    auto *header = new QWidget;
    header->setStyleSheet(QString("background: %1;").arg(tm.bgColor()));
    auto *hRow = new QHBoxLayout(header);
    hRow->setContentsMargins(12, 12, 12, 12);
    hRow->setSpacing(10);

    auto *logo = new QLabel;
    logo->setPixmap(tm.themedLogo(28, 2.0));
    logo->setFixedSize(28, 28);
    logo->setScaledContents(true);
    logo->setStyleSheet(QStringLiteral("background: transparent;"));
    hRow->addWidget(logo);

    auto *headerCol = new QVBoxLayout;
    headerCol->setSpacing(2);
    headerCol->setContentsMargins(0, 0, 0, 0);
    auto *appName = new QLabel(QStringLiteral("BATorrent"));
    {
        QFont f; f.setPointSize(11); f.setWeight(QFont::Bold);
        appName->setFont(f);
        appName->setStyleSheet(QString("color: %1; background: transparent;").arg(tm.textColor()));
    }
    headerCol->addWidget(appName);
    m_headerCount = new QLabel(QStringLiteral("0 torrents"));
    {
        QFont f("Menlo"); f.setStyleHint(QFont::Monospace); f.setPointSize(9);
        m_headerCount->setFont(f);
        m_headerCount->setStyleSheet(QString("color: %1; background: transparent;").arg(tm.mutedColor()));
    }
    headerCol->addWidget(m_headerCount);
    hRow->addLayout(headerCol, 1);

    m_statusDot = makeStatusDot(QColor(tm.dimColor()), 8, header);
    hRow->addWidget(m_statusDot, 0, Qt::AlignTop);

    inner->addWidget(header);

    // ── Speed strip ──
    auto *speed = new QWidget;
    speed->setStyleSheet(QString("background: %1;").arg(tm.surfaceColor()));
    auto *sRow = new QHBoxLayout(speed);
    sRow->setContentsMargins(12, 10, 12, 10);
    sRow->setSpacing(16);

    auto makeSpeedColumn = [&](const QString &label, const QColor &accent,
                                QLabel *&outValue, QLabel *&outUnit) {
        auto *col = new QVBoxLayout;
        col->setSpacing(2);
        col->setContentsMargins(0, 0, 0, 0);

        auto *eyebrow = new QLabel(label);
        QFont ef; ef.setPointSize(8); ef.setWeight(QFont::Bold);
        ef.setLetterSpacing(QFont::AbsoluteSpacing, 1.2);
        eyebrow->setFont(ef);
        eyebrow->setStyleSheet(QString("color: %1; background: transparent;").arg(tm.dimColor()));
        col->addWidget(eyebrow);

        auto *valueRow = new QHBoxLayout;
        valueRow->setSpacing(2);
        outValue = new QLabel(QStringLiteral("0"));
        QFont vf("Menlo"); vf.setStyleHint(QFont::Monospace);
        vf.setPointSize(14); vf.setWeight(QFont::Bold);
        outValue->setFont(vf);
        outValue->setStyleSheet(QString("color: %1; background: transparent;").arg(tm.textColor()));
        valueRow->addWidget(outValue);
        outUnit = new QLabel(QStringLiteral(" KB/s"));
        QFont uf; uf.setPointSize(9);
        outUnit->setFont(uf);
        outUnit->setStyleSheet(QString("color: %1; background: transparent;").arg(tm.mutedColor()));
        outUnit->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
        valueRow->addWidget(outUnit);
        valueRow->addStretch();
        col->addLayout(valueRow);

        Q_UNUSED(accent);
        return col;
    };

    sRow->addLayout(makeSpeedColumn(QStringLiteral("↓ DOWN"),
        QColor(tm.stateDownloadingColor()), m_downValue, m_downUnit), 1);
    sRow->addLayout(makeSpeedColumn(QStringLiteral("↑ UP"),
        QColor(tm.stateSeedingColor()), m_upValue, m_upUnit), 1);
    inner->addWidget(speed);

    // ── Active torrents preview (up to 2) ──
    m_activeSection = new QWidget;
    m_activeSection->setStyleSheet(QString("background: %1;").arg(tm.surfaceAltColor()));
    m_activeLayout = new QVBoxLayout(m_activeSection);
    m_activeLayout->setContentsMargins(12, 4, 12, 4);
    m_activeLayout->setSpacing(0);
    inner->addWidget(m_activeSection);

    // ── Menu items ──
    auto *menu = new QWidget;
    menu->setStyleSheet(QString("background: %1;").arg(tm.surfaceColor()));
    auto *mCol = new QVBoxLayout(menu);
    mCol->setContentsMargins(0, 4, 0, 4);
    mCol->setSpacing(0);

#ifdef Q_OS_MAC
    const QString cmd = QStringLiteral("⌘");
#else
    const QString cmd = QStringLiteral("Ctrl+");
#endif

    // Qt's translation entries use "&X" as keyboard-accelerator hints which
    // QMenu interprets (underlining the next char). Custom widgets render
    // them literally, so strip the ampersand for the tray rows.
    auto strip = [](const QString &s) {
        QString out = s;
        out.remove(QChar('&'));
        return out;
    };

    auto addRow = [&](const QString &label, const QString &shortcut,
                      bool danger, std::function<void()> handler) {
        auto *r = new MenuRow(label, shortcut, danger, menu);
        r->onClick([this, handler]() {
            handler();
            hide();
        });
        mCol->addWidget(r);
        return r;
    };

    addRow(strip(tr_("tray_show")), cmd + "1", false,
           [this]() { emit showWindowRequested(); });
    addRow(strip(tr_("action_open")), cmd + "O", false,
           [this]() { emit openFileRequested(); });
    addRow(strip(tr_("action_magnet")), cmd + "V", false,
           [this]() { emit pasteMagnetRequested(); });
    mCol->addWidget(makeDivider(menu));
    addRow(strip(tr_("action_pause_all")), QString(), false,
           [this]() { emit pauseAllRequested(); });
    addRow(strip(tr_("action_resume_all")), QString(), false,
           [this]() { emit resumeAllRequested(); });
    mCol->addWidget(makeDivider(menu));

    // VPN row with optional "kill switch on" sub-text. We construct it
    // directly so the label/sub-label are reachable for refresh() updates.
    auto *vpnRow = new MenuRow(tr_("tray_vpn_idle"), QString(), false, menu);
    m_vpnLabel = vpnRow->findChild<QLabel *>();
    m_vpnSub = vpnRow->addSub(QStringLiteral(""), QColor(tm.stateSeedingColor()));
    m_vpnSub->setVisible(false);
    vpnRow->onClick([this]() { emit openSettingsRequested(); hide(); });
    mCol->addWidget(vpnRow);

    // Auto-shutdown row with toggle (short label for the tray; the verbose
    // version lives in the settings dialog).
    auto *autoRow = new MenuRow(tr_("tray_auto_shutdown"), QString(), false, menu);
    m_autoShutdownSwitch = new ToggleSwitch(autoRow);
    autoRow->setTrailing(m_autoShutdownSwitch);
    connect(m_autoShutdownSwitch, &ToggleSwitch::toggled, this, [this](bool on) {
        m_autoShutdownOn = on;
        emit autoShutdownToggled(on);
    });
    mCol->addWidget(autoRow);
    mCol->addWidget(makeDivider(menu));

    addRow(strip(tr_("action_settings")), cmd + ",", false,
           [this]() { emit openSettingsRequested(); });
    addRow(strip(tr_("tray_quit")), cmd + "Q", true,
           [this]() { emit quitRequested(); });

    inner->addWidget(menu);
}

void TrayPopup::rebuildActiveList(QVBoxLayout *into)
{
    // Wipe and rebuild — the count and order can change between shows.
    while (auto *item = into->takeAt(0)) {
        if (auto *w = item->widget()) w->deleteLater();
        delete item;
    }

    int added = 0;
    for (int i = 0; i < m_session->torrentCount() && added < 2; ++i) {
        TorrentInfo info = m_session->torrentAt(i);
        if (info.paused) continue;
        if (info.progress >= 1.0f) continue;
        if (info.downloadRate <= 0) continue;
        const qint64 remaining = info.totalSize > 0
            ? info.totalSize - info.totalDone : 0;
        into->addWidget(new ActiveRow(info.name, info.progress,
                                      info.downloadRate, remaining));
        added++;
    }
    m_activeSection->setVisible(added > 0);
}

void TrayPopup::refresh()
{
    const auto &tm = ThemeManager::instance();

    int total = m_session->torrentCount();
    int active = 0;
    qint64 down = 0, up = 0;
    for (int i = 0; i < total; ++i) {
        TorrentInfo info = m_session->torrentAt(i);
        if (!info.paused && info.downloadRate + info.uploadRate > 0)
            active++;
        down += info.downloadRate;
        up += info.uploadRate;
    }
    m_headerCount->setText(tr_("tray_count_format")
        .arg(total).arg(active));

    // Status dot: amber if any activity, dim otherwise.
    QColor dotColor = active > 0
        ? QColor(tm.stateSeedingColor()) : QColor(tm.dimColor());
    // Re-render the dot pixmap (the label was created at construction).
    QPixmap pm(16, 16);
    pm.fill(Qt::transparent);
    pm.setDevicePixelRatio(2.0);
    {
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen);
        p.setBrush(dotColor);
        p.drawEllipse(QRectF(0, 0, 8, 8));
    }
    m_statusDot->setPixmap(pm);

    auto formatSpeedSplit = [](qint64 bps, QString &value, QString &unit) {
        if (bps < 1024) { value = QString::number(bps); unit = QStringLiteral(" B/s"); }
        else if (bps < 1024 * 1024) {
            value = QString::number(bps / 1024.0, 'f', 1);
            unit = QStringLiteral(" KB/s");
        } else {
            value = QString::number(bps / (1024.0 * 1024.0), 'f', 1);
            unit = QStringLiteral(" MB/s");
        }
    };
    QString dVal, dUnit, uVal, uUnit;
    formatSpeedSplit(down, dVal, dUnit);
    formatSpeedSplit(up, uVal, uUnit);
    m_downValue->setText(dVal);
    m_downUnit->setText(dUnit);
    m_upValue->setText(uVal);
    m_upUnit->setText(uUnit);

    rebuildActiveList(m_activeLayout);

    // VPN row — show the bound interface name when set, otherwise an idle
    // placeholder. Sub-label surfaces "kill switch on" only while engaged.
    if (m_vpnLabel) {
        m_vpnLabel->setText(m_vpnIface.isEmpty()
            ? tr_("tray_vpn_idle")
            : QString("VPN — %1").arg(m_vpnIface));
    }
    if (!m_vpnIface.isEmpty()) {
        m_vpnSub->setVisible(m_killSwitchOn);
        if (m_killSwitchOn)
            m_vpnSub->setText(tr_("tray_kill_switch_on"));
    } else {
        m_vpnSub->setVisible(false);
    }

    m_autoShutdownSwitch->blockSignals(true);
    m_autoShutdownSwitch->setOn(m_autoShutdownOn);
    m_autoShutdownSwitch->blockSignals(false);
}

void TrayPopup::showAt(const QPoint &anchor)
{
    refresh();
    adjustSize();

    // Clamp to screen so we don't render off-edge. We try positioning the
    // popup below-right of the anchor first (matches macOS menu-bar feel);
    // if that overflows, mirror to the opposite side.
    QScreen *screen = QGuiApplication::screenAt(anchor);
    if (!screen) screen = QGuiApplication::primaryScreen();
    const QRect avail = screen ? screen->availableGeometry() : QRect(0, 0, 1920, 1080);

    QPoint pos = anchor + QPoint(-width() / 2, 16);
    if (pos.x() + width() > avail.right() - 8) pos.setX(avail.right() - width() - 8);
    if (pos.x() < avail.left() + 8) pos.setX(avail.left() + 8);
    if (pos.y() + height() > avail.bottom() - 8)
        pos.setY(anchor.y() - height() - 16);
    if (pos.y() < avail.top() + 8) pos.setY(avail.top() + 8);

    move(pos);
    show();
    raise();
    activateWindow();
}
