// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef RELEASENOTESDIALOG_H
#define RELEASENOTESDIALOG_H

#include <QDialog>

class QTextBrowser;

class ReleaseNotesDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ReleaseNotesDialog(QWidget *parent = nullptr);

    static QString releaseNotes();
};

#endif
