// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef QMLSETTINGSBRIDGE_H
#define QMLSETTINGSBRIDGE_H

#include "bridges/bridgecommon.h"

class QmlSettingsBridge : public QObject
{
    Q_OBJECT
public:
    // The ~80-method settings surface stays bound to SessionManager (config
    // control plane, settings_pack wrappers). In IPC mode `session` is null and
    // get/set fall back to QSettings (the shared store the engine child reads at
    // startup). `engine` is the always-valid IEngine, used only to drive the
    // WebUI server so it works in split mode too.
    explicit QmlSettingsBridge(SessionManager *session, IEngine *engine, QObject *parent = nullptr);
    Q_INVOKABLE QVariant get(const QString &key) const;
    Q_INVOKABLE void set(const QString &key, const QVariant &v);
    // Register BATorrent as the default .torrent / magnet handler. Returns success.
    Q_INVOKABLE bool setAsDefaultApp();
    // Post a test message to the configured Telegram chat; result via telegramTestResult.
    Q_INVOKABLE void testTelegram();
    // Windows-only: add the save folder to Windows Defender's exclusion list (UAC prompt).
    Q_INVOKABLE bool excludeFromDefender();
    // Settings export/import (JSON, secrets excluded) + full backup/restore
    // (.bat archive: settings + resume data). Each returns a localized result.
    Q_INVOKABLE QString exportSettings(const QString &path);
    Q_INVOKABLE QString importSettings(const QString &path);
    Q_INVOKABLE QString fullBackup(const QString &path);
    Q_INVOKABLE QString fullRestore(const QString &path);
    // Up/active network interfaces by name (index 0 = "Any"), for the VPN bind select.
    Q_INVOKABLE QStringList networkInterfaces() const;
    // One-tap phone pairing (the recommended "scan and connect" flow): enable the
    // WebUI, generate a strong password, allow LAN access — secure but zero manual
    // setup. enablePairing returns the plaintext password to display.
    Q_INVOKABLE QString enablePairing();
    Q_INVOKABLE void disablePairing();
    Q_INVOKABLE bool pairingActive() const;
    Q_INVOKABLE QString webUiUser() const;
    Q_INVOKABLE QString webUiPassword() const;   // plaintext from the secure store, for display
    // One-click proxy presets for SOCKS5-capable tunnels (e.g. Mullvad's local
    // SOCKS5). Fills type/host/port; the user still toggles it on.
    Q_INVOKABLE void applyProxyPreset(const QString &name);
    // Verify the tunnel actually carries traffic: compares the external IP seen
    // directly vs through the proxy. Result via proxyLeakTestResult.
    Q_INVOKABLE void proxyLeakTest();
    void setTelegramNotifier(TelegramNotifier *n) { m_telegram = n; }
signals:
    void changed();
    void telegramTestResult(bool ok, const QString &message);
    void proxyLeakTestResult(bool ok, const QString &message);
private:
    static int telegramEventBit(const QString &key);   // toggle key → Events bit (0 if none)
    void applyWebUi();                                  // (re)start the WebUI server from settings
    SessionManager *m_session;
    IEngine *m_engine;                                  // always valid; drives the WebUI in split mode
    TelegramNotifier *m_telegram = nullptr;
    WebServer *m_webServer = nullptr;
};

#endif // QMLSETTINGSBRIDGE_H
