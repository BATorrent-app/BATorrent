// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef ADDONDIALOG_H
#define ADDONDIALOG_H

#include <QDialog>

class QListWidget;
class QLineEdit;
class QCheckBox;

class AddonDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AddonDialog(QWidget *parent = nullptr);

private slots:
    void installAddon();
    void removeSelectedAddon();
    void refreshList();

private:
    QListWidget *m_addonList;
    QLineEdit *m_urlEdit;
    QCheckBox *m_autoTrackersCheck;
    QCheckBox *m_torrentSearchCheck;
    QLineEdit *m_torrentSearchUrlEdit;
};

#endif
