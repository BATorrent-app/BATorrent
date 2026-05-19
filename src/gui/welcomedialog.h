// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef WELCOMEDIALOG_H
#define WELCOMEDIALOG_H

#include <QDialog>

class QCheckBox;

class WelcomeDialog : public QDialog
{
    Q_OBJECT
public:
    explicit WelcomeDialog(QWidget *parent = nullptr);
    bool dontShowAgain() const;

signals:
    // Emitted when the user clicks an action card. The receiver should also
    // close the dialog (the card click itself only emits).
    void openFileRequested();
    void pasteMagnetRequested();
    void openSearchRequested();
    void openRssRequested();

private:
    QCheckBox *m_dontShowCheck;
};

#endif
