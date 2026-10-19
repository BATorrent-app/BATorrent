// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef ALTTAP_H
#define ALTTAP_H

#include <QObject>

// Alt pressed and let go on its own, as Windows apps use it to show their menu
// bar. Watches the app's windows, not one item: with no field focused, keys go
// to the window's root and no QML handler below it would see them. Any other
// key or a click while Alt is down (Alt+Left, Alt+Tab, Alt+drag) is not a tap.
class AltTap : public QObject
{
    Q_OBJECT
public:
    explicit AltTap(QObject *parent = nullptr);
    ~AltTap() override;

signals:
    void tapped();

protected:
    bool eventFilter(QObject *o, QEvent *e) override;

private:
    bool m_armed = false;
};

#endif
