// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef BATORRENT_PAIRINGDIALOG_H
#define BATORRENT_PAIRINGDIALOG_H

#include <QDialog>

// Shown from Settings → WebUI ("Pair phone..."). Displays the LAN URL the
// user's phone should hit so they don't have to find their machine's IP
// manually. QR code rendering is intentionally absent in v2.5.0 — adding it
// would require either a 500-LoC vendored encoder or an external dependency;
// the copy-URL workflow ships the user value without that cost. QR is
// tracked as a v2.5.1 follow-up.
class PairingDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PairingDialog(int port, QWidget *parent = nullptr);

private:
    static QString detectLanIp();
};

#endif
