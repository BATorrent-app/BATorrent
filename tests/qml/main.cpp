// SPDX-License-Identifier: MIT
// Qt Quick Test runner. Discovers tst_*.qml under QUICK_TEST_SOURCE_DIR and runs
// them against the app's QML (linked via the resources qrc, so qrc:/src/qml/*;
// including the Theme singleton and the widgets module: resolves).
//
// The setup hook injects a minimal `i18n` context object so widgets that bind to
// i18n.t(...) without a typeof guard (PathFld, ToastHost) render cleanly under
// test instead of logging ReferenceErrors. t() echoes the key back.
#include <QtQuickTest/quicktest.h>
#include <QDateTime>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QObject>
#include <QQmlEngine>
#include <QQmlContext>
#include <QString>
#include <QStringList>
#include <cstdio>
#include <QtQml/qqml.h>

#include "bridges/isolatedmediaplayer.h"

class I18nStub : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int language MEMBER m_language CONSTANT)
public:
    using QObject::QObject;
    Q_INVOKABLE QString t(const QString &key) const { return key; }
    // The real bridge formats in the app's language; a fixed ISO date is enough here.
    Q_INVOKABLE QString date(qint64 secs) const {
        return QDateTime::fromSecsSinceEpoch(secs).date().toString(Qt::ISODate);
    }
private:
    int m_language = 0;
};

class TestSetup : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;
public slots:
    void qmlEngineAvailable(QQmlEngine *engine)
    {
        // Same faces the app registers. Without them every Theme.fontSans
        // binding misses and Qt rescans the system font families to resolve
        // it, which on a cold macOS font cache turned a two-second run into
        // seventeen minutes — passing, but indistinguishable from a hang.
        for (const char *face : { ":/fonts/IBMPlexSans-Regular.ttf",
                                  ":/fonts/IBMPlexSans-Medium.ttf",
                                  ":/fonts/IBMPlexSans-SemiBold.ttf",
                                  ":/fonts/IBMPlexSans-Bold.ttf",
                                  ":/fonts/NewRocker-Regular.ttf" })
            QFontDatabase::addApplicationFont(QString::fromLatin1(face));
        // Anything that does not name a family asks for "Sans Serif", which is
        // a generic Qt does not have either, and the same rescan follows.
        QGuiApplication::setFont(QFont(QStringLiteral("IBM Plex Sans")));
        engine->rootContext()->setContextProperty(
            QStringLiteral("i18n"), new I18nStub(engine));
        qmlRegisterType<IsolatedMediaPlayer>("BATorrent.Media", 1, 0, "IsolatedMediaPlayer");
    }
};

// QML reports its errors as warnings and carries on, so a binding loop or a
// TypeError never failed a test: the suite passed while the app logged them.
// They fail the run now. (QString::arg's "argument missing" is left out: the
// i18n stub above hands back keys without placeholders.)
namespace {
QtMessageHandler g_previous = nullptr;
QStringList g_qmlErrors;
void failOnQmlErrors(QtMsgType type, const QMessageLogContext &ctx, const QString &msg)
{
    static const QStringList kinds = {
        QStringLiteral("Binding loop detected"), QStringLiteral("TypeError:"),
        QStringLiteral("ReferenceError:"), QStringLiteral("Unable to assign"),
        QStringLiteral("Cannot read property"), QStringLiteral("is not a function"),
        QStringLiteral("non-existent property"), QStringLiteral("is not a type"),
        QStringLiteral("failed to load component") };
    if (type == QtWarningMsg || type == QtCriticalMsg)
        for (const QString &k : kinds)
            if (msg.contains(k)) { g_qmlErrors << msg; break; }
    if (g_previous) g_previous(type, ctx, msg);
}
}

int main(int argc, char **argv)
{
    QTEST_SET_MAIN_SOURCE_PATH
    g_previous = qInstallMessageHandler(failOnQmlErrors);
    TestSetup setup;
    int rc = quick_test_main_with_setup(argc, argv, "batorrent_qml", QUICK_TEST_SOURCE_DIR, &setup);
    if (!g_qmlErrors.isEmpty()) {
        fprintf(stderr, "\nQML errors during the run (%lld):\n", qint64(g_qmlErrors.size()));
        for (const QString &e : g_qmlErrors) fprintf(stderr, "  %s\n", qPrintable(e));
        if (rc == 0) rc = 1;
    }
    return rc;
}

#include "main.moc"
