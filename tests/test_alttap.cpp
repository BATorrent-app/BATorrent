// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// AltTap: Alt pressed and let go on its own shows the menu bar. Alt used with
// something else (Alt+Left, Alt+Tab, Alt+click) must not.

#include <catch2/catch_test_macros.hpp>

#include "app/alttap.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QSignalSpy>
#include <QWindow>

namespace {

void ensureApp()
{
    static int argc = 1;
    static char arg0[] = "test_alttap";
    static char *argv[] = { arg0, nullptr };
    if (!QCoreApplication::instance()) {
        qputenv("QT_QPA_PLATFORM", "offscreen");
        new QGuiApplication(argc, argv);
    }
}

void key(QObject *to, QEvent::Type t, int k, Qt::KeyboardModifiers m)
{
    QKeyEvent e(t, k, m);
    QCoreApplication::sendEvent(to, &e);
}
void altDown(QObject *to) { key(to, QEvent::KeyPress, Qt::Key_Alt, Qt::AltModifier); }
void altUp(QObject *to) { key(to, QEvent::KeyRelease, Qt::Key_Alt, Qt::NoModifier); }

} // namespace

TEST_CASE("AltTap: Alt on its own is a tap", "[alttap]")
{
    ensureApp();
    QWindow w;
    AltTap tap;
    QSignalSpy spy(&tap, &AltTap::tapped);

    altDown(&w);
    altUp(&w);
    CHECK(spy.count() == 1);

    altDown(&w);
    altUp(&w);
    CHECK(spy.count() == 2);
}

TEST_CASE("AltTap: Alt with another key or a click is not", "[alttap]")
{
    ensureApp();
    QWindow w;
    AltTap tap;
    QSignalSpy spy(&tap, &AltTap::tapped);

    altDown(&w);   // Alt+Left: back
    key(&w, QEvent::KeyPress, Qt::Key_Left, Qt::AltModifier);
    key(&w, QEvent::KeyRelease, Qt::Key_Left, Qt::AltModifier);
    altUp(&w);
    CHECK(spy.count() == 0);

    altDown(&w);   // Alt+click
    QMouseEvent press(QEvent::MouseButtonPress, QPointF(5, 5), QPointF(5, 5), QPointF(5, 5),
                      Qt::LeftButton, Qt::LeftButton, Qt::AltModifier);
    QCoreApplication::sendEvent(&w, &press);
    altUp(&w);
    CHECK(spy.count() == 0);

    altDown(&w);   // Alt+Tab: the window loses focus, the release never comes here
    QEvent deact(QEvent::WindowDeactivate);
    QCoreApplication::sendEvent(&w, &deact);
    altUp(&w);
    CHECK(spy.count() == 0);
}

TEST_CASE("AltTap: a key handed on to an item is counted once", "[alttap]")
{
    ensureApp();
    QWindow w;
    QObject item;   // what the window hands the key to next
    AltTap tap;
    QSignalSpy spy(&tap, &AltTap::tapped);

    altDown(&w);
    altDown(&item);
    altUp(&w);
    altUp(&item);
    CHECK(spy.count() == 1);
}

