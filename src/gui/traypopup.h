// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef TRAYPOPUP_H
#define TRAYPOPUP_H

#include <QDialog>
#include <QFrame>
#include <QString>

class SessionManager;
class QLabel;
class QWidget;
class QVBoxLayout;

// Mirrors primitives.jsx Toggle: pill shape, 28×16, knob slides between
// off and on positions, bg switches between surfaceAlt and accent.
class ToggleSwitch : public QFrame
{
    Q_OBJECT
public:
    explicit ToggleSwitch(QWidget *parent = nullptr);
    bool isOn() const { return m_on; }
    void setOn(bool on);

signals:
    void toggled(bool on);

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void paintEvent(QPaintEvent *) override;

private:
    bool m_on = false;
};

// Tray dropdown widget invoked by a left-click on the system tray icon.
// Replicates canvas/misc.jsx TrayMenu — a frameless popup that's drawn
// in pure Qt (the native context menu can't render this design on Windows).
class TrayPopup : public QDialog
{
    Q_OBJECT
public:
    explicit TrayPopup(SessionManager *session, QWidget *parent = nullptr);

    // Show the popup anchored near `anchor` (typically the cursor on click),
    // clamped to the current screen so it never spills off-edge.
    void showAt(const QPoint &anchor);

    void setVpnInterface(const QString &iface) { m_vpnIface = iface; }
    void setKillSwitchActive(bool on) { m_killSwitchOn = on; }
    void setAutoShutdown(bool on) { m_autoShutdownOn = on; }

signals:
    void showWindowRequested();
    void openFileRequested();
    void pasteMagnetRequested();
    void pauseAllRequested();
    void resumeAllRequested();
    void openSettingsRequested();
    void autoShutdownToggled(bool on);
    void quitRequested();

private:
    void refresh();
    void rebuildActiveList(QVBoxLayout *into);

    SessionManager *m_session;
    QLabel *m_headerCount = nullptr;
    QLabel *m_statusDot = nullptr;
    QLabel *m_downValue = nullptr;
    QLabel *m_downUnit  = nullptr;
    QLabel *m_upValue   = nullptr;
    QLabel *m_upUnit    = nullptr;
    QVBoxLayout *m_activeLayout = nullptr;
    QWidget *m_activeSection = nullptr;
    QLabel *m_vpnLabel = nullptr;
    QLabel *m_vpnSub = nullptr;
    ToggleSwitch *m_autoShutdownSwitch = nullptr;

    QString m_vpnIface;
    bool m_killSwitchOn = false;
    bool m_autoShutdownOn = false;
};

#endif
