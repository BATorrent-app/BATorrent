// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "app/alttap.h"

#include <QCoreApplication>
#include <QKeyEvent>
#include <QWindow>

AltTap::AltTap(QObject *parent) : QObject(parent)
{
    if (QCoreApplication::instance()) QCoreApplication::instance()->installEventFilter(this);
}

AltTap::~AltTap()
{
    if (QCoreApplication::instance()) QCoreApplication::instance()->removeEventFilter(this);
}

bool AltTap::eventFilter(QObject *o, QEvent *e)
{
    // A key reaches the window first and then each item it is handed to: count
    // it once, at the window.
    if (!qobject_cast<QWindow *>(o)) return false;
    switch (e->type()) {
    case QEvent::KeyPress: {
        auto *k = static_cast<QKeyEvent *>(e);
        if (k->isAutoRepeat()) break;
        // Qt flips Alt in the modifiers of Alt's own event, so whether it reads
        // set depends on how it was made: what matters is nothing else is held
        m_armed = k->key() == Qt::Key_Alt && (k->modifiers() & ~Qt::AltModifier) == Qt::NoModifier;
        break;
    }
    case QEvent::KeyRelease: {
        auto *k = static_cast<QKeyEvent *>(e);
        if (k->isAutoRepeat() || k->key() != Qt::Key_Alt) break;
        const bool tap = m_armed;
        m_armed = false;
        if (tap) emit tapped();
        break;
    }
    case QEvent::MouseButtonPress:
    case QEvent::Wheel:
    case QEvent::FocusOut:
    case QEvent::WindowDeactivate:
        m_armed = false;
        break;
    default:
        break;
    }
    return false;
}
