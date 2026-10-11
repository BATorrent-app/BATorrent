// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef BATORRENT_APPCONTAINER_WIN_H
#define BATORRENT_APPCONTAINER_WIN_H

#include <QProcess>
#include <QString>
#include <memory>

// Windows confinement for the media child (internal/ISOLATED_DECODER_PLAN.md,
// phase 3). Unlike macOS the parent applies it at creation: an AppContainer
// with no capabilities (no network, no loopback, no user files) inside a job
// that forbids further processes and dies with the UI.
class ContainedLaunch
{
public:
    ContainedLaunch();
    ~ContainedLaunch();
    ContainedLaunch(const ContainedLaunch &) = delete;
    ContainedLaunch &operator=(const ContainedLaunch &) = delete;

    // Creates or reuses the profile and lets it read `codeDir`. False = no sandbox:
    // the caller must not start the child.
    bool prepare(const QString &codeDir, QString *error);
    QString sid() const { return m_sid; }
    QProcess::CreateProcessArgumentModifier modifier();

private:
    struct Native;
    std::unique_ptr<Native> d;
    QString m_sid;
};

#endif
