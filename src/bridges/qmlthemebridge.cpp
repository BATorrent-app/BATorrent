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
#include <QScreen>
#include <QWindow>
#include <QEvent>
#ifdef Q_OS_WIN
#  include <windows.h>
#  include <dwmapi.h>
#  include <shellapi.h>
#  ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#    define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#  endif
#endif
#include <QCoreApplication>
#include <QStyleHints>
#include <QFont>
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
    m_followSystem = s.value(QStringLiteral("qmlFollowSystem"), false).toBool();
    loadProfiles();
    migrateThemes();
    m_themeName = s.value(QStringLiteral("qmlThemeName"), QStringLiteral("default")).toString();
    m_backgroundMode = s.value(QStringLiteral("qmlBackgroundMode"), QStringLiteral("theme")).toString();
    m_userImage = s.value(QStringLiteral("qmlUserImage")).toString();
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

namespace {
// Process-wide: every IconImg reads the same counter, and the hot-reload watcher
// is the only thing that moves it. Not persisted: it exists for one dev session.
int g_iconEpoch = 0;
}

int QmlThemeBridge::iconEpoch() { return g_iconEpoch; }
void QmlThemeBridge::bumpIconEpoch() { ++g_iconEpoch; }

QString QmlThemeBridge::devIconDir() const
{
    const QString qmlDir = qEnvironmentVariable("BAT_QML_DIR");
    if (qmlDir.isEmpty()) return {};
    const QDir icons(QDir(qmlDir).filePath(QStringLiteral("../icons")));
    if (!icons.exists()) return {};
    return QUrl::fromLocalFile(icons.absolutePath()).toString();
}

QString QmlThemeBridge::themeName() const
{
    // When following the OS, the effective theme tracks the system scheme so the
    // whole palette (Theme.qml binds to this) swaps light/dark automatically.
    // The dark side is the user's own dark theme, Default when the one picked
    // by hand is light (or a theme of their own that is gone).
    QString picked = m_themeName;
    if (picked == QLatin1String("user") && m_profiles.isEmpty()) picked = QStringLiteral("default");
    if (m_followSystem) {
        if (m_osLight) return QStringLiteral("light");
        return isLightTheme(picked) ? QStringLiteral("default") : picked;
    }
    return picked;
}
void QmlThemeBridge::setThemeName(const QString &n)
{
    // Picking a theme by hand turns off "follow system": otherwise the choice
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
    return !isLightTheme(themeName());   // effective (accounts for follow-system)
}

void QmlThemeBridge::applyTitleBar(QWindow *w, bool dark)
{
#ifdef Q_OS_WIN
    if (!w) return;
    const BOOL on = dark ? TRUE : FALSE;
    DwmSetWindowAttribute(reinterpret_cast<HWND>(w->winId()),
                          DWMWA_USE_IMMERSIVE_DARK_MODE, &on, sizeof(on));
    // A neutral edge: with "accent colour on title bars and borders" on,
    // Windows 11 would draw the user's accent round a window whose title bar
    // is our own. Older Windows ignores the attribute.
    constexpr DWORD kBorderColor = 34;   // DWMWA_BORDER_COLOR
    const COLORREF edge = dark ? RGB(43, 43, 46) : RGB(200, 200, 204);
    DwmSetWindowAttribute(reinterpret_cast<HWND>(w->winId()), kBorderColor, &edge, sizeof(edge));
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

QString QmlThemeBridge::backgroundMode() const { return m_backgroundMode; }
void QmlThemeBridge::setBackgroundMode(const QString &mode)
{
    const QString m = (mode == QLatin1String("clean") || mode == QLatin1String("user")) ? mode : QStringLiteral("theme");
    if (m == m_backgroundMode) return;
    m_backgroundMode = m;
    QSettings().setValue(QStringLiteral("qmlBackgroundMode"), m);
    emit changed();
}

QString QmlThemeBridge::userImage() const { return m_userImage; }
void QmlThemeBridge::setUserImage(const QString &path)
{
    if (path == m_userImage) return;
    m_userImage = path;
    QSettings().setValue(QStringLiteral("qmlUserImage"), path);
    emit changed();
}

bool QmlThemeBridge::isLightTheme(const QString &name) const
{
    if (name == QLatin1String("light") || name == QLatin1String("sakura")) return true;
    if (name == QLatin1String("user"))
        return activeMap().value(QStringLiteral("base")).toString() == QLatin1String("light");
    return false;
}

void QmlThemeBridge::setAppFontFamily(const QString &family)
{
    if (family.isEmpty()) return;
    QFont f = QGuiApplication::font();
    if (f.family() == family) return;
    f.setFamily(family);
    QGuiApplication::setFont(f);
}

// ---- the user's own themes ----

QVariantMap QmlThemeBridge::defaultProfile(const QString &name)
{
    QVariantMap p;
    p["name"]      = name;
    p["base"]      = QStringLiteral("dark");
    p["corners"]   = QStringLiteral("soft");
    p["font"]      = QStringLiteral("IBM Plex Sans");
    p["bg"]        = QStringLiteral("#0e0e10");
    p["panel"]     = QStringLiteral("#18181b");
    p["text"]      = QStringLiteral("#f3f3f4");
    p["primary"]   = QStringLiteral("#e5332b");
    p["secondary"] = QStringLiteral("#d99a2b");
    p["tertiary"]  = QStringLiteral("#3fb950");
    p["image"]     = QString();
    p["fit"]       = QStringLiteral("fill");
    p["zoom"]      = 100;
    p["dim"]       = 55;
    p["blur"]      = 0;
    return p;
}

void QmlThemeBridge::loadProfiles()
{
    QSettings s;
    m_profiles.clear();
    const QByteArray json = s.value(QStringLiteral("qmlCustomProfiles")).toByteArray();
    if (json.isEmpty()) return;
    const auto doc = QJsonDocument::fromJson(json);
    if (!doc.isArray()) return;
    for (const auto &v : doc.array()) {
        // backfill any missing keys from the default so a partial/old profile
        // never collapses the palette to empty colors
        QVariantMap p = defaultProfile(QString());
        const QVariantMap stored = v.toObject().toVariantMap();
        for (auto it = stored.cbegin(); it != stored.cend(); ++it)
            p[it.key()] = it.value();
        // the old Custom profile had the picture's opacity; the new one dims
        // the picture with the background colour instead, the other way round
        if (stored.contains(QStringLiteral("opacity")) && !stored.contains(QStringLiteral("dim")))
            p["dim"] = qBound(0, 100 - stored.value(QStringLiteral("opacity")).toInt(), 90);
        p.remove(QStringLiteral("opacity"));
        m_profiles << p;
    }
}

void QmlThemeBridge::saveProfiles()
{
    QJsonArray arr;
    for (const auto &v : m_profiles)
        arr.append(QJsonObject::fromVariantMap(v.toMap()));
    QSettings().setValue(QStringLiteral("qmlCustomProfiles"),
                         QJsonDocument(arr).toJson(QJsonDocument::Compact));
}

void QmlThemeBridge::migrateThemes()
{
    QSettings s;
    if (s.value(QStringLiteral("qmlThemesV2"), false).toBool()) return;
    const bool existing = s.contains(QStringLiteral("qmlThemeName")) || s.contains(QStringLiteral("qmlAnime"));
    QString name = s.value(QStringLiteral("qmlThemeName"), QStringLiteral("default")).toString();
    // "dark" was the app as it looks now; "custom" is now a theme of one's own
    if (name == QLatin1String("dark")) name = QStringLiteral("default");
    if (name == QLatin1String("custom")) name = m_profiles.isEmpty() ? QStringLiteral("default") : QStringLiteral("user");
    // Nobody ever chose the placeholder "Custom 1" unless they were on Custom:
    // left in, it would show up as a theme they never made.
    if (name != QLatin1String("user") && m_profiles.size() == 1) {
        QVariantMap untouched = defaultProfile(QStringLiteral("Custom 1"));
        QVariantMap p = m_profiles.first().toMap();
        bool same = true;
        for (const char *k : {"bg", "text", "primary", "secondary", "tertiary", "image"})
            if (p.value(QLatin1String(k)) != untouched.value(QLatin1String(k))) same = false;
        if (same && p.value(QStringLiteral("name")).toString() == QLatin1String("Custom 1")) {
            m_profiles.clear();
            saveProfiles();
        }
    }
    s.setValue(QStringLiteral("qmlThemeName"), name);
    // the anime art is gone; whoever had it on gets the theme's own art, and
    // an existing install that had it off keeps a clean background
    if (!s.contains(QStringLiteral("qmlBackgroundMode")))
        s.setValue(QStringLiteral("qmlBackgroundMode"),
                   existing && !s.value(QStringLiteral("qmlAnime"), false).toBool() ? QStringLiteral("clean")
                                                                                     : QStringLiteral("theme"));
    s.remove(QStringLiteral("qmlAnime"));
    s.setValue(QStringLiteral("qmlThemesV2"), true);
}

QVariantList QmlThemeBridge::customProfiles() const { return m_profiles; }

QVariantMap QmlThemeBridge::activeMap() const
{
    if (m_activeProfile >= 0 && m_activeProfile < m_profiles.size())
        return m_profiles.at(m_activeProfile).toMap();
    return defaultProfile(QStringLiteral("My theme"));
}

int QmlThemeBridge::activeProfile() const { return m_activeProfile; }
void QmlThemeBridge::setActiveProfile(int i)
{
    if (i == m_activeProfile || i < 0 || i >= m_profiles.size()) return;
    m_activeProfile = i;
    QSettings().setValue(QStringLiteral("qmlActiveProfile"), i);
    emit changed();
}

void QmlThemeBridge::selectProfile(int i)
{
    if (i < 0 || i >= m_profiles.size()) return;
    m_activeProfile = i;
    QSettings().setValue(QStringLiteral("qmlActiveProfile"), i);
    setThemeName(QStringLiteral("user"));   // emits changed()
}

int QmlThemeBridge::addProfile()
{
    m_profiles << defaultProfile(QStringLiteral("My theme %1").arg(m_profiles.size() + 1));
    saveProfiles();
    emit profilesChanged();
    return m_profiles.size() - 1;
}

int QmlThemeBridge::duplicateProfile(int i)
{
    if (i < 0 || i >= m_profiles.size()) return -1;
    QVariantMap p = m_profiles.at(i).toMap();
    p["name"] = p.value(QStringLiteral("name")).toString() + QStringLiteral(" copy");
    m_profiles.insert(i + 1, p);
    if (m_activeProfile > i) ++m_activeProfile;
    saveProfiles();
    emit profilesChanged();
    return i + 1;
}

void QmlThemeBridge::removeProfile(int i)
{
    if (i < 0 || i >= m_profiles.size()) return;
    const bool wasInUse = (i == m_activeProfile && m_themeName == QLatin1String("user"));
    m_profiles.removeAt(i);
    // keep the active selection pointing at the SAME profile: shift down if an
    // earlier one was removed, clamp if the active one (or past-end) was removed.
    if (i < m_activeProfile)
        --m_activeProfile;
    if (m_activeProfile >= m_profiles.size())
        m_activeProfile = qMax(0, int(m_profiles.size()) - 1);
    QSettings().setValue(QStringLiteral("qmlActiveProfile"), m_activeProfile);
    saveProfiles();
    emit profilesChanged();
    if (wasInUse || (m_profiles.isEmpty() && m_themeName == QLatin1String("user")))
        setThemeName(QStringLiteral("default"));
    else
        emit changed();
}

void QmlThemeBridge::setProfileValue(int i, const QString &key, const QVariant &value)
{
    if (i < 0 || i >= m_profiles.size()) return;
    QVariantMap p = m_profiles.at(i).toMap();
    if (p.value(key) == value) return;
    p[key] = value;
    m_profiles[i] = p;
    saveProfiles();
    emit profilesChanged();
    if (i == m_activeProfile) {
        emit changed();
        if (key == QLatin1String("base")) refreshTitleBars();
    }
}

void QmlThemeBridge::renameProfile(int i, const QString &name)
{
    setProfileValue(i, QStringLiteral("name"), name);
}

void QmlThemeBridge::setProfileColor(int i, const QString &role, const QString &hex)
{
    setProfileValue(i, role, hex);
}

void QmlThemeBridge::setProfileImage(int i, const QString &path)
{
    setProfileValue(i, QStringLiteral("image"), path);
}

bool QmlThemeBridge::osLight() const { return m_osLight; }

QString QmlThemeBridge::appVersion() const { return QCoreApplication::applicationVersion(); }

QPointF QmlThemeBridge::cursorPos() const
{
    return QCursor::pos();
}

QRect QmlThemeBridge::noticeArea() const
{
    QScreen *screen = QGuiApplication::primaryScreen();
#ifdef Q_OS_WIN
    // The monitor is matched by its device name ("\\.\DISPLAY2"), which QScreen
    // reports too: Win32 rectangles are physical pixels and Qt's are scaled
    // per monitor, so comparing coordinates would go wrong on mixed DPI.
    if (HWND fg = GetForegroundWindow()) {
        MONITORINFOEXW mi = {};
        mi.cbSize = sizeof(mi);
        if (GetMonitorInfoW(MonitorFromWindow(fg, MONITOR_DEFAULTTOPRIMARY), &mi)) {
            const QString device = QString::fromWCharArray(mi.szDevice);
            for (QScreen *s : QGuiApplication::screens())
                if (s->name().compare(device, Qt::CaseInsensitive) == 0) { screen = s; break; }
        }
    }
#endif
    return screen ? screen->availableGeometry() : QRect();
}

bool QmlThemeBridge::exclusiveFullscreenInFront() const
{
#ifdef Q_OS_WIN
    QUERY_USER_NOTIFICATION_STATE state = QUNS_ACCEPTS_NOTIFICATIONS;
    return SUCCEEDED(SHQueryUserNotificationState(&state)) && state == QUNS_RUNNING_D3D_FULL_SCREEN;
#else
    return false;
#endif
}

void QmlThemeBridge::markBootHealthy() const
{
    QSettings st;
    st.setValue(QStringLiteral("bootInProgress"), false);
    st.setValue(QStringLiteral("bootCrashes"), 0);
    st.sync();
}

void QmlThemeBridge::setSoftwareRenderer(bool on)
{
    if (m_softwareRenderer == on) return;
    m_softwareRenderer = on;
    emit changed();
}

QString QmlThemeBridge::releaseNotes() const
{
    QFile f(QStringLiteral(":/CHANGELOG.md"));   // the real source of truth
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();
    return QString::fromUtf8(f.readAll());
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

