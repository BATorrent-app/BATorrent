// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include <QPainter>
#include "app/appruntime.h"

#include "bridges/qmlthemebridge.h"
#include "services/platform/logger.h"
#include "services/platform/utils.h"

#include <QApplication>
#include <QCoreApplication>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QMessageBox>
#include <QQuickImageProvider>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QSettings>
#include <QStandardPaths>
#include <cstdio>
#include <cstdlib>

#include <libtorrent/version.hpp>
#include <boost/version.hpp>

#ifdef BAT_HAVE_SENTRY
#include <sentry.h>
#endif

namespace {

class AppLogoImageProvider : public QQuickImageProvider
{
public:
    AppLogoImageProvider() : QQuickImageProvider(QQuickImageProvider::Pixmap) {}
    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requested) override
    {
        const int sz = requested.width() > 0 ? requested.width() : 256;
        const bool darkBody = id.startsWith("dark");
        QPixmap pm = QmlThemeBridge::renderLogo(darkBody, sz, 1.0);

        // VPN state as a dot under the wing, the way IVPN marks its own tray
        // icon. Only drawn when the caller asks for it: a permanent red badge
        // on a machine with no VPN configured would be an alarm about nothing.
        if (!pm.isNull() && (id.contains(QLatin1String("vpn=on"))
                             || id.contains(QLatin1String("vpn=off")))) {
            const bool up = id.contains(QLatin1String("vpn=on"));
            QPainter p(&pm);
            p.setRenderHint(QPainter::Antialiasing);
            const qreal d = sz * 0.34;
            const QRectF dot(sz - d, sz - d, d, d);
            // Punch a transparent ring first so the dot reads as a badge on top
            // of the wing instead of a blob merged into it.
            p.setCompositionMode(QPainter::CompositionMode_Clear);
            p.setPen(Qt::NoPen);
            p.setBrush(Qt::black);
            p.drawEllipse(dot.adjusted(-sz * 0.05, -sz * 0.05, sz * 0.05, sz * 0.05));
            p.setCompositionMode(QPainter::CompositionMode_SourceOver);
            p.setBrush(up ? QColor(0x36, 0xb3, 0x7e) : QColor(0xe0, 0x57, 0x4b));
            p.drawEllipse(dot);
        }
        if (size) *size = pm.size();
        return pm;
    }
};

bool isQmlError(const QString &m)
{
    if (!m.contains(QLatin1String(".qml:")) && !m.contains(QLatin1String("qrc:")))
        return false;
    static const char *markers[] = {
        "TypeError", "ReferenceError", "is not a function", "is not defined",
        "Cannot read property", "Unable to assign", "Cannot assign",
        "Binding loop detected", "Error:"
    };
    for (const char *mk : markers)
        if (m.contains(QLatin1String(mk))) return true;
    return false;
}

void qtMessageHandler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg)
{
    Logger::Level lvl = Logger::Debug;
    switch (type) {
    case QtDebugMsg:    lvl = Logger::Debug;    break;
    case QtInfoMsg:     lvl = Logger::Info;     break;
    case QtWarningMsg:  lvl = Logger::Warning;  break;
    case QtCriticalMsg: lvl = Logger::Error;    break;
    case QtFatalMsg:    lvl = Logger::Critical; break;
    }
    QString prefix;
    if (ctx.category && qstrcmp(ctx.category, "default") != 0)
        prefix = QStringLiteral("[%1] ").arg(QString::fromUtf8(ctx.category));
    Logger::instance().log(lvl, prefix + msg);
    fprintf(stderr, "%s\n", qPrintable(prefix + msg));

    static const QByteArray qmlStrict = qgetenv("BAT_QML_STRICT");
    if (!qmlStrict.isEmpty() && type == QtWarningMsg && isQmlError(msg)) {
        fprintf(stderr, "\n‼️  [QML ERROR] %s\n\n", qPrintable(msg));
        if (qmlStrict == "fatal") { fflush(stderr); abort(); }
    }
}

} // namespace

namespace AppRuntime {

void applyGraphicsApiPreference()
{
    QString api = QString::fromLocal8Bit(qgetenv("BAT_GRAPHICS_API")).trimmed().toLower();
    if (api.isEmpty()) {
        const int idx = QSettings(QStringLiteral("BATorrent"), QStringLiteral("BATorrent"))
                            .value(QStringLiteral("graphicsApi"), 0).toInt();
        if (idx == 1) api = QStringLiteral("software");
        else if (idx == 2) api = QStringLiteral("opengl");
        else if (idx == 3) api = QStringLiteral("d3d11");
    }
    if (api == QLatin1String("software"))
        QQuickWindow::setGraphicsApi(QSGRendererInterface::Software);
    else if (api == QLatin1String("opengl"))
        QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    else if (api == QLatin1String("d3d11") || api == QLatin1String("direct3d11"))
        QQuickWindow::setGraphicsApi(QSGRendererInterface::Direct3D11);
}

void runStartupMigrations()
{
    setSpeedUnit(QSettings("BATorrent", "BATorrent").value("speedUnit", 0).toInt());

    QSettings st;
    if (!st.contains("postDownloadAction") && st.value("autoShutdown", false).toBool())
        st.setValue("postDownloadAction", 6);
}

void loadFonts(QApplication &app)
{
    QFontDatabase::addApplicationFont(":/fonts/IBMPlexSans-Regular.ttf");
    QFontDatabase::addApplicationFont(":/fonts/IBMPlexSans-Medium.ttf");
    QFontDatabase::addApplicationFont(":/fonts/IBMPlexSans-SemiBold.ttf");
    QFontDatabase::addApplicationFont(":/fonts/IBMPlexSans-Bold.ttf");
    QFontDatabase::addApplicationFont(":/fonts/NewRocker-Regular.ttf");

    if (!QFontDatabase::families().contains(QStringLiteral("IBM Plex Sans")))
        qWarning() << "[font] IBM Plex Sans failed to register — the UI is on a fallback family";

    QFont defaultFont("IBM Plex Sans", 10);
    defaultFont.setStyleStrategy(QFont::PreferAntialias);
    app.setFont(defaultFont);
}

void logDependencyVersions()
{
    qInfo().nospace().noquote() << "[versions] BATorrent " << QCoreApplication::applicationVersion()
                      << " · Qt " << qVersion()
                      << " · libtorrent " << LIBTORRENT_VERSION
                      << " · Boost " << QString::fromLatin1(BOOST_LIB_VERSION).replace('_', '.');
}

void showQmlLoadFailure(const QString &logHint)
{
    qInstallMessageHandler(nullptr);
    QMessageBox box;
    box.setIcon(QMessageBox::Critical);
    box.setWindowTitle(QStringLiteral("BATorrent"));
    box.setText(QStringLiteral("BATorrent couldn't load its interface."));
    box.setInformativeText(
        QStringLiteral("The log may explain why (%1). If the window opens blank or gray, "
                       "reinstall or set graphicsApi=software in settings and restart.")
            .arg(logHint));
    box.addButton(QMessageBox::Ok);
    box.exec();
}

void installQtMessageHandler()
{
    qInstallMessageHandler(qtMessageHandler);
}

#ifdef BAT_HAVE_SENTRY
void initSentry(const QString &role)
{
    sentry_options_t *o = sentry_options_new();
#ifdef BAT_SENTRY_DSN
    sentry_options_set_dsn(o, BAT_SENTRY_DSN);
#endif
    {
        QString handler = QCoreApplication::applicationDirPath()
                          + QStringLiteral("/crashpad_handler");
#ifdef Q_OS_WIN
        handler += QStringLiteral(".exe");
#endif
#ifdef BAT_SENTRY_HANDLER
        if (!QFileInfo::exists(handler)) handler = QStringLiteral(BAT_SENTRY_HANDLER);
#endif
        if (QFileInfo::exists(handler))
            sentry_options_set_handler_path(o, handler.toUtf8().constData());
    }
    const QString db = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                       + QStringLiteral("/sentry-") + role;
    sentry_options_set_database_path(o, db.toUtf8().constData());
    sentry_options_set_release(o, "batorrent@" APP_VERSION);
#ifdef QT_DEBUG
    sentry_options_set_environment(o, "development");
#else
    sentry_options_set_environment(o, "production");
#endif
    if (qEnvironmentVariableIsSet("BAT_SENTRY_TEST"))
        sentry_options_set_debug(o, 1);
    sentry_init(o);
    qAddPostRoutine([]{ sentry_close(); });
}
#endif

QQmlImageProviderBase *createAppLogoProvider()
{
    return new AppLogoImageProvider();
}

} // namespace AppRuntime
