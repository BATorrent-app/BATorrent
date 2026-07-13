// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "bridges/qmlposterbridge.h"
#include "torrent/sessionmanager.h"
#include "services/metadata/metadataresolver.h"
#include "services/discovery/discoveryservice.h"
#include "services/metadata/nameparser.h"
#include "services/integrations/rssmanager.h"
#include "services/discovery/addonmanager.h"
#include "services/platform/logger.h"
#include "services/platform/qrcodegen.h"
#include "services/platform/utils.h"
#include "services/platform/translator.h"
#include "services/integrations/geoip.h"
#include "services/integrations/discordrpc.h"
#include "services/integrations/updater.h"
#include "services/integrations/notifier.h"
#include "services/security/secretstore.h"
#include "webui/webserver.h"
#include <QCryptographicHash>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDateTime>

#include <QNetworkInterface>

#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QProcess>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QApplication>
#include <QWindow>
#include <QEvent>
#ifdef Q_OS_WIN
#  include <windows.h>
#  include <dwmapi.h>
#  ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#    define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#  endif
#endif
#include <QCoreApplication>
#include <QStyleHints>
#include <QPainter>
#include <QSvgRenderer>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <cstring>
#include <algorithm>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>

#include <libtorrent/torrent_info.hpp>
#include <libtorrent/file_storage.hpp>
#include <libtorrent/create_torrent.hpp>
#include <libtorrent/bencode.hpp>
#include <libtorrent/version.hpp>
#include <openssl/opensslv.h>
#include <boost/version.hpp>
#include <memory>
#include <sstream>

QmlThemeBridge::QmlThemeBridge(QObject *parent) : QObject(parent)
{
    qApp->installEventFilter(this);   // paint each window's native title bar (Windows)
    QSettings s;
    m_themeName = s.value(QStringLiteral("qmlThemeName"), QStringLiteral("dark")).toString();
    m_followSystem = s.value(QStringLiteral("qmlFollowSystem"), false).toBool();
    m_anime = s.value(QStringLiteral("qmlAnime"), false).toBool();
    loadProfiles();
    m_activeProfile = s.value(QStringLiteral("qmlActiveProfile"), 0).toInt();
    if (m_activeProfile < 0 || m_activeProfile >= m_profiles.size())
        m_activeProfile = 0;
    m_appIconChoice = s.value(QStringLiteral("appIconChoice"), QStringLiteral("default")).toString();

    if (auto *hints = QGuiApplication::styleHints()) {
        m_osLight = (hints->colorScheme() == Qt::ColorScheme::Light);
        connect(hints, &QStyleHints::colorSchemeChanged, this, [this](Qt::ColorScheme scheme) {
            const bool light = (scheme == Qt::ColorScheme::Light);
            if (light == m_osLight) return;
            m_osLight = light;
            // re-tint the scheme-aware logo, but keep a user-chosen custom icon
            applyAppIcon(m_appIconChoice);
            emit osSchemeChanged();                        // QML tray re-binds
            if (m_followSystem) {                          // OS flipped → swap theme
                emit changed();
                refreshTitleBars();
            }
        });
    }
}

QString QmlThemeBridge::themeName() const
{
    // When following the OS, the effective theme tracks the system scheme so the
    // whole palette (Theme.qml binds to this) swaps light/dark automatically.
    if (m_followSystem) return m_osLight ? QStringLiteral("light") : QStringLiteral("dark");
    return m_themeName;
}
void QmlThemeBridge::setThemeName(const QString &n)
{
    // Picking a theme by hand turns off "follow system" — otherwise the choice
    // would be silently overridden on the next OS scheme change.
    if (m_followSystem) {
        m_followSystem = false;
        QSettings().setValue(QStringLiteral("qmlFollowSystem"), false);
    }
    if (n == m_themeName) { emit changed(); return; }
    m_themeName = n;
    QSettings().setValue(QStringLiteral("qmlThemeName"), n);
    emit changed();
    refreshTitleBars();   // recolor native title bars to match the new theme
}

bool QmlThemeBridge::followSystem() const { return m_followSystem; }
void QmlThemeBridge::setFollowSystem(bool on)
{
    if (on == m_followSystem) return;
    m_followSystem = on;
    QSettings().setValue(QStringLiteral("qmlFollowSystem"), on);
    emit changed();
    refreshTitleBars();
}

// ---- native title bar (Windows leaves it light regardless of theme) ----

bool QmlThemeBridge::eventFilter(QObject *o, QEvent *e)
{
    if (e->type() == QEvent::Show) {
        if (auto *w = qobject_cast<QWindow *>(o))
            applyTitleBar(w, darkTitleBar());
    }
    return QObject::eventFilter(o, e);
}

bool QmlThemeBridge::darkTitleBar() const
{
    // light-ish themes → light title bar; dark/midnight/custom → dark.
    const QString n = themeName();   // effective (accounts for follow-system)
    return !(n == QLatin1String("light") || n == QLatin1String("sakura"));
}

void QmlThemeBridge::applyTitleBar(QWindow *w, bool dark)
{
#ifdef Q_OS_WIN
    if (!w) return;
    const BOOL on = dark ? TRUE : FALSE;
    DwmSetWindowAttribute(reinterpret_cast<HWND>(w->winId()),
                          DWMWA_USE_IMMERSIVE_DARK_MODE, &on, sizeof(on));
#else
    Q_UNUSED(w); Q_UNUSED(dark);
#endif
}

void QmlThemeBridge::refreshTitleBars()
{
    const bool dark = darkTitleBar();
    const auto windows = QGuiApplication::topLevelWindows();
    for (QWindow *w : windows) applyTitleBar(w, dark);
}

bool QmlThemeBridge::anime() const { return m_anime; }
void QmlThemeBridge::setAnime(bool on)
{
    if (on == m_anime) return;
    m_anime = on;
    QSettings().setValue(QStringLiteral("qmlAnime"), on);
    emit changed();
}

// ---- custom profiles ----

QVariantMap QmlThemeBridge::defaultProfile(const QString &name)
{
    QVariantMap p;
    p["name"]      = name;
    p["bg"]        = QStringLiteral("#0e0e10");
    p["panel"]     = QStringLiteral("#141416");
    p["text"]      = QStringLiteral("#f3f3f4");
    p["primary"]   = QStringLiteral("#e5332b");
    p["secondary"] = QStringLiteral("#d99a2b");
    p["tertiary"]  = QStringLiteral("#3fb950");
    p["image"]     = QString();
    p["opacity"]   = 55;
    return p;
}

void QmlThemeBridge::loadProfiles()
{
    QSettings s;
    const QByteArray json = s.value(QStringLiteral("qmlCustomProfiles")).toByteArray();
    if (!json.isEmpty()) {
        const auto doc = QJsonDocument::fromJson(json);
        if (doc.isArray()) {
            m_profiles.clear();
            for (const auto &v : doc.array()) {
                // backfill any missing keys from the default so a partial/old
                // profile never collapses the palette to empty colors
                QVariantMap p = defaultProfile(QString());
                const QVariantMap stored = v.toObject().toVariantMap();
                for (auto it = stored.cbegin(); it != stored.cend(); ++it)
                    p[it.key()] = it.value();
                m_profiles << p;
            }
        }
    }
    if (m_profiles.isEmpty())
        m_profiles << defaultProfile(QStringLiteral("Custom 1"));
}

void QmlThemeBridge::saveProfiles()
{
    QJsonArray arr;
    for (const auto &v : m_profiles)
        arr.append(QJsonObject::fromVariantMap(v.toMap()));
    QSettings().setValue(QStringLiteral("qmlCustomProfiles"),
                         QJsonDocument(arr).toJson(QJsonDocument::Compact));
}

QVariantList QmlThemeBridge::customProfiles() const { return m_profiles; }

QVariantMap QmlThemeBridge::activeMap() const
{
    if (m_activeProfile >= 0 && m_activeProfile < m_profiles.size())
        return m_profiles.at(m_activeProfile).toMap();
    return defaultProfile(QStringLiteral("Custom 1"));
}

int QmlThemeBridge::activeProfile() const { return m_activeProfile; }
void QmlThemeBridge::setActiveProfile(int i)
{
    if (i == m_activeProfile || i < 0 || i >= m_profiles.size()) return;
    m_activeProfile = i;
    QSettings().setValue(QStringLiteral("qmlActiveProfile"), i);
    emit changed();
}

int QmlThemeBridge::addProfile()
{
    m_profiles << defaultProfile(QStringLiteral("Custom %1").arg(m_profiles.size() + 1));
    saveProfiles();
    emit profilesChanged();
    return m_profiles.size() - 1;
}

void QmlThemeBridge::removeProfile(int i)
{
    if (i < 0 || i >= m_profiles.size() || m_profiles.size() <= 1) return;
    m_profiles.removeAt(i);
    // keep the active selection pointing at the SAME profile: shift down if an
    // earlier one was removed, clamp if the active one (or past-end) was removed.
    if (i < m_activeProfile)
        --m_activeProfile;
    if (m_activeProfile >= m_profiles.size())
        m_activeProfile = m_profiles.size() - 1;
    QSettings().setValue(QStringLiteral("qmlActiveProfile"), m_activeProfile);
    saveProfiles();
    emit profilesChanged();
    emit changed();
}

void QmlThemeBridge::renameProfile(int i, const QString &name)
{
    if (i < 0 || i >= m_profiles.size()) return;
    QVariantMap p = m_profiles.at(i).toMap();
    if (p.value("name").toString() == name) return;
    p["name"] = name;
    m_profiles[i] = p;
    saveProfiles();
    emit profilesChanged();
}

void QmlThemeBridge::setProfileColor(int i, const QString &role, const QString &hex)
{
    if (i < 0 || i >= m_profiles.size()) return;
    QVariantMap p = m_profiles.at(i).toMap();
    if (p.value(role).toString() == hex) return;
    p[role] = hex;
    m_profiles[i] = p;
    saveProfiles();
    emit profilesChanged();
    if (i == m_activeProfile) emit changed();
}

void QmlThemeBridge::setProfileImage(int i, const QString &path)
{
    if (i < 0 || i >= m_profiles.size()) return;
    QVariantMap p = m_profiles.at(i).toMap();
    if (p.value("image").toString() == path) return;
    p["image"] = path;
    m_profiles[i] = p;
    saveProfiles();
    emit profilesChanged();
    if (i == m_activeProfile) emit changed();
}

void QmlThemeBridge::setProfileOpacity(int i, int pct)
{
    if (i < 0 || i >= m_profiles.size()) return;
    QVariantMap p = m_profiles.at(i).toMap();
    if (p.value("opacity").toInt() == pct) return;
    p["opacity"] = pct;
    m_profiles[i] = p;
    saveProfiles();
    emit profilesChanged();
    if (i == m_activeProfile) emit changed();
}

QString QmlThemeBridge::cBg()        const { return activeMap().value("bg").toString(); }
QString QmlThemeBridge::cPanel()     const { return activeMap().value("panel").toString(); }
QString QmlThemeBridge::cText()      const { return activeMap().value("text").toString(); }
QString QmlThemeBridge::cPrimary()   const { return activeMap().value("primary").toString(); }
QString QmlThemeBridge::cSecondary() const { return activeMap().value("secondary").toString(); }
QString QmlThemeBridge::cTertiary()  const { return activeMap().value("tertiary").toString(); }
QString QmlThemeBridge::cBgImage()   const { return activeMap().value("image").toString(); }
int     QmlThemeBridge::cBgOpacity() const { return activeMap().value("opacity").toInt(); }

bool QmlThemeBridge::osLight() const { return m_osLight; }

QString QmlThemeBridge::appVersion() const { return QCoreApplication::applicationVersion(); }

QPointF QmlThemeBridge::cursorPos() const
{
    return QCursor::pos();
}

void QmlThemeBridge::markBootHealthy() const
{
    QSettings st;
    st.setValue(QStringLiteral("bootInProgress"), false);
    st.setValue(QStringLiteral("bootCrashes"), 0);
    st.sync();
}

QString QmlThemeBridge::releaseNotes() const
{
    QFile f(QStringLiteral(":/CHANGELOG.md"));   // the real source of truth
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();
    return QString::fromUtf8(f.readAll());
}

QVariantList QmlThemeBridge::libraries() const
{
    QVariantList out;
    auto add = [&](const QString &nm, const QString &v) {
        QVariantMap m; m["nm"] = nm; m["v"] = v; out << m;
    };
    add("Qt", QString::fromLatin1(qVersion()));
    add("libtorrent-rasterbar", QStringLiteral(LIBTORRENT_VERSION));
#ifdef OPENSSL_VERSION_STR
    add("OpenSSL", QStringLiteral(OPENSSL_VERSION_STR));
#endif
    add("Boost", QString::fromLatin1(BOOST_LIB_VERSION).replace('_', '.'));
    return out;
}

QPixmap QmlThemeBridge::renderLogo(bool darkBody, int size, qreal dpr)
{
    if (dpr <= 0) dpr = 1.0;
    QFile f(QStringLiteral(":/images/logo.svg"));
    if (!f.open(QIODevice::ReadOnly))
        return QPixmap();
    QByteArray svg = f.readAll();
    // Body is off-white (#E9E9E8); swap to dark text when the background is
    // light so it doesn't vanish. The red wings (#E72134) stay in both cases.
    if (darkBody)
        svg.replace("#E9E9E8", "#16171a");
    QSvgRenderer renderer(svg);
    const int px = int(size * dpr);
    QPixmap pm(px, px);
    pm.fill(Qt::transparent);
    pm.setDevicePixelRatio(dpr);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    renderer.render(&p, QRectF(0, 0, size, size));   // logical extent (retina fix)
    return pm;
}

QIcon QmlThemeBridge::trayIcon() const
{
    QIcon icon;
    for (int sz : {16, 24, 32, 64, 128, 256})
        icon.addPixmap(renderLogo(m_osLight, sz, 1.0));
    return icon;
}

// ---- app-icon picker (theme-independent; issue #15) ----

QString QmlThemeBridge::appIconChoice() const { return m_appIconChoice; }

QVariantList QmlThemeBridge::appIcons() const
{
    QVariantList out;
    const auto add = [&](const QString &key, const QString &preview) {
        QVariantMap m; m["key"] = key; m["preview"] = preview; out << m;
    };
    add(QStringLiteral("default"),  QStringLiteral("qrc:/images/logo1.png"));
    add(QStringLiteral("dark"),     QStringLiteral("qrc:/images/appicons/dark.png"));
    add(QStringLiteral("light"),    QStringLiteral("qrc:/images/appicons/light.png"));
    add(QStringLiteral("midnight"), QStringLiteral("qrc:/images/appicons/midnight.png"));
    add(QStringLiteral("sakura"),   QStringLiteral("qrc:/images/appicons/sakura.png"));
    add(QStringLiteral("darkstar"), QStringLiteral("qrc:/images/appicons/darkstar.png"));
    return out;
}

QIcon QmlThemeBridge::iconForKey(const QString &key)
{
    QString path;
    if (key == QLatin1String("dark"))          path = QStringLiteral(":/images/appicons/dark.png");
    else if (key == QLatin1String("light"))    path = QStringLiteral(":/images/appicons/light.png");
    else if (key == QLatin1String("midnight")) path = QStringLiteral(":/images/appicons/midnight.png");
    else if (key == QLatin1String("sakura"))   path = QStringLiteral(":/images/appicons/sakura.png");
    else if (key == QLatin1String("darkstar")) path = QStringLiteral(":/images/appicons/darkstar.png");
    else return QIcon();

    QPixmap src(path);
    if (src.isNull()) return QIcon();
    QIcon icon;
    for (int sz : {16, 24, 32, 64, 128, 256, 512})
        icon.addPixmap(src.scaled(sz, sz, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    return icon;
}

// Sets the live window icon: macOS Dock + Windows taskbar/window. "default"
// keeps the bundled .icns on macOS (issue #14) and the scheme-aware logo
// elsewhere; a custom pick overrides the Dock on purpose.
void QmlThemeBridge::applyAppIcon(const QString &key)
{
    if (key.isEmpty() || key == QLatin1String("default")) {
#ifndef Q_OS_MACOS
        QGuiApplication::setWindowIcon(trayIcon());
#endif
        return;
    }
    const QIcon icon = iconForKey(key);
    if (!icon.isNull())
        QGuiApplication::setWindowIcon(icon);
}

void QmlThemeBridge::setAppIcon(const QString &key)
{
    const QString k = key.isEmpty() ? QStringLiteral("default") : key;
    if (k != m_appIconChoice) {
        m_appIconChoice = k;
        QSettings().setValue(QStringLiteral("appIconChoice"), k);
        emit appIconChanged();
    }
    applyAppIcon(k);
}

void QmlThemeBridge::applySavedAppIcon()
{
    if (!m_appIconChoice.isEmpty() && m_appIconChoice != QLatin1String("default"))
        applyAppIcon(m_appIconChoice);
}

// RSS bridge

