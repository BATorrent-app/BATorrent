// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "app/macfileopen.h"
#include "app/singleinstance.h"

#include <QCoreApplication>
#include <QEvent>
#include <QFileOpenEvent>
#include <QObject>
#include <QUrl>

namespace {

class Filter : public QObject
{
public:
    Filter(SingleInstance *single, QObject *parent)
        : QObject(parent), m_single(single) {}

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override
    {
        if (ev->type() != QEvent::FileOpen)
            return QObject::eventFilter(obj, ev);

        auto *fe = static_cast<QFileOpenEvent *>(ev);
        // file() for a double-clicked .torrent; url() for a magnet: or
        // bittorrent: link handed over by the browser, where file() is empty.
        QString target = fe->file();
        if (target.isEmpty())
            target = fe->url().toString();
        if (!target.isEmpty() && m_single)
            m_single->deliver(target);
        return true;
    }

private:
    SingleInstance *m_single;
};

} // namespace

namespace MacFileOpen {

void install(QCoreApplication *app, SingleInstance *single)
{
    if (!app || !single) return;
    app->installEventFilter(new Filter(single, app));
}

} // namespace MacFileOpen
