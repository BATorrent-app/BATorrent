// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/vpn/vpnmanager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QSaveFile>
#include <QFileDevice>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QSettings>
#include <QUuid>

VpnManager::VpnManager(WgTunnel *tunnel, QObject *parent)
    : QObject(parent), m_tunnel(tunnel ? tunnel : new StubWgTunnel(this))
{
    if (m_tunnel->parent() != this) m_tunnel->setParent(this);

    connect(m_tunnel, &WgTunnel::connected, this, [this](const QString &iface) {
        if (m_state != State::Connecting) return;   // stale (disconnected mid-connect)
        m_iface = iface;
        setState(State::Connected);
        emit interfaceUp(iface);
    });
    connect(m_tunnel, &WgTunnel::disconnected, this, [this]() {
        m_iface.clear();
        setState(State::Disconnected);
        const bool deliberate = m_userDown;
        m_userDown = false;
        emit interfaceDown(deliberate);
    });
    connect(m_tunnel, &WgTunnel::failed, this, [this](const QString &why) {
        m_error = why;
        m_iface.clear();
        setState(State::Failed);
        m_userDown = false;
        emit interfaceDown(false);
    });

    load();
}

QString VpnManager::vpnDir() const
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                         + QStringLiteral("/vpn");
    QDir().mkpath(base);
    return base;
}

QString VpnManager::confPath(const QString &id) const
{
    return vpnDir() + QLatin1Char('/') + id + QStringLiteral(".conf");
}

// Short basename on purpose: the Windows client names the tunnel after the conf
// file and caps that name at 32 chars — a full 32-char id + suffix won't fit.
QString VpnManager::splitConfPath(const QString &id) const
{
    return vpnDir() + QLatin1Char('/') + id.left(12) + QStringLiteral("-split.conf");
}

int VpnManager::indexOf(const QString &id) const
{
    for (int i = 0; i < m_profiles.size(); ++i)
        if (m_profiles[i].id == id) return i;
    return -1;
}

void VpnManager::setState(State s)
{
    if (m_state == s) return;
    m_state = s;
    emit stateChanged();
}

bool VpnManager::tunnelIsReal() const { return m_tunnel && m_tunnel->isReal(); }

QString VpnManager::importConfig(const QString &name, const QString &confText)
{
    const bat::WgConfig cfg = bat::parseWireguardConfig(confText);
    if (!cfg.valid) { m_error = cfg.error; return QString(); }

    const QString id = QUuid::createUuid().toString(QUuid::Id128);
    // The .conf holds a private key — write it owner-only, like wg-quick does.
    QSaveFile f(confPath(id));
    if (!f.open(QIODevice::WriteOnly)) { m_error = QStringLiteral("cannot write config"); return QString(); }
    f.write(confText.toUtf8());
    if (!f.commit()) { m_error = QStringLiteral("cannot save config"); return QString(); }
    QFile::setPermissions(confPath(id), QFileDevice::ReadOwner | QFileDevice::WriteOwner);

    m_profiles.push_back(Profile{ id,
                                  name.trimmed().isEmpty() ? QStringLiteral("WireGuard") : name.trimmed(),
                                  cfg.peers.first().endpoint });
    saveIndex();
    emit profilesChanged();
    return id;
}

QString VpnManager::importFromFile(const QString &fileUrlOrPath, const QString &name)
{
    QString path = fileUrlOrPath;
    if (path.startsWith(QStringLiteral("file:")))
        path = QUrl(path).toLocalFile();
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) { m_error = QStringLiteral("cannot read ") + path; return QString(); }
    const QString providedName = name.trimmed().isEmpty()
        ? QFileInfo(path).completeBaseName() : name;
    return importConfig(providedName, QString::fromUtf8(f.readAll()));
}

void VpnManager::removeProfile(const QString &id)
{
    const int i = indexOf(id);
    if (i < 0) return;
    if (m_activeId == id) disconnectVpn();
    QFile::remove(confPath(id));
    QFile::remove(splitConfPath(id));
    m_profiles.remove(i);
    saveIndex();
    emit profilesChanged();
}

QVariantList VpnManager::profiles() const
{
    QVariantList out;
    for (const Profile &p : m_profiles)
        out.append(QVariantMap{ {QStringLiteral("id"), p.id},
                                {QStringLiteral("name"), p.name},
                                {QStringLiteral("endpoint"), p.endpoint} });
    return out;
}

void VpnManager::connectProfile(const QString &id)
{
    if (m_state == State::Connecting || m_state == State::Connected) return;
    if (indexOf(id) < 0) { m_error = QStringLiteral("unknown profile"); setState(State::Failed); return; }

    QFile f(confPath(id));
    if (!f.open(QIODevice::ReadOnly)) { m_error = QStringLiteral("config missing on disk"); setState(State::Failed); return; }
    const QString confText = QString::fromUtf8(f.readAll());
    const bat::WgConfig cfg = bat::parseWireguardConfig(confText);
    if (!cfg.valid) { m_error = cfg.error; setState(State::Failed); return; }

    QString tunnelConf = confPath(id);
    // Split tunnel: bring the tunnel up from a Table=off copy so it takes no
    // default route — only the torrent session (bound to the interface) uses it.
    if (QSettings().value(QStringLiteral("vpnSplitTunnel"), false).toBool()) {
        const QString staged = splitConfPath(id);
        QSaveFile sf(staged);
        if (!sf.open(QIODevice::WriteOnly)
            || sf.write(bat::splitTunnelConf(confText).toUtf8()) < 0
            || !sf.commit()) {
            m_error = QStringLiteral("cannot stage split-tunnel config");
            setState(State::Failed);
            return;
        }
        QFile::setPermissions(staged, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        tunnelConf = staged;
    }

    m_activeId = id;
    m_error.clear();
    QSettings().setValue(QStringLiteral("vpnLastProfileId"), id);
    setState(State::Connecting);
    m_tunnel->up(tunnelConf, cfg);
}

void VpnManager::connectLastUsed()
{
    const QString id = QSettings().value(QStringLiteral("vpnLastProfileId")).toString();
    if (!id.isEmpty() && indexOf(id) >= 0) connectProfile(id);
}

void VpnManager::disconnectVpn()
{
    if (m_state == State::Disconnected) return;
    m_activeId.clear();
    m_userDown = true;
    m_tunnel->down();          // disconnected() slot flips state + emits interfaceDown
    if (m_state == State::Failed) { m_iface.clear(); setState(State::Disconnected); emit interfaceDown(true); }
}

void VpnManager::saveIndex() const
{
    QJsonArray arr;
    for (const Profile &p : m_profiles)
        arr.append(QJsonObject{ {QStringLiteral("id"), p.id}, {QStringLiteral("name"), p.name} });
    QSaveFile f(vpnDir() + QStringLiteral("/profiles.json"));
    if (!f.open(QIODevice::WriteOnly)) return;
    f.write(QJsonDocument(arr).toJson(QJsonDocument::Compact));
    f.commit();
}

void VpnManager::load()
{
    QFile f(vpnDir() + QStringLiteral("/profiles.json"));
    if (!f.open(QIODevice::ReadOnly)) return;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isArray()) return;
    for (const QJsonValue &v : doc.array()) {
        const QJsonObject o = v.toObject();
        const QString id = o.value(QStringLiteral("id")).toString();
        // drop index entries whose .conf vanished, so the list never lies
        if (id.isEmpty() || !QFile::exists(confPath(id))) continue;
        // endpoint comes from the .conf, not the index — it can't go stale
        QString endpoint;
        QFile cf(confPath(id));
        if (cf.open(QIODevice::ReadOnly)) {
            const bat::WgConfig cfg = bat::parseWireguardConfig(QString::fromUtf8(cf.readAll()));
            if (cfg.valid) endpoint = cfg.peers.first().endpoint;
        }
        m_profiles.push_back(Profile{ id, o.value(QStringLiteral("name")).toString(), endpoint });
    }
}
