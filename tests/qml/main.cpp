// SPDX-License-Identifier: MIT
// Qt Quick Test runner. Discovers tst_*.qml under QUICK_TEST_SOURCE_DIR and runs
// them against the app's QML (linked via the resources qrc, so qrc:/src/qml/* —
// including the Theme singleton and the widgets module — resolves).
//
// The setup hook injects a minimal `i18n` context object so widgets that bind to
// i18n.t(...) without a typeof guard (PathFld, ToastHost) render cleanly under
// test instead of logging ReferenceErrors. t() echoes the key back.
#include <QtQuickTest/quicktest.h>
#include <QObject>
#include <QQmlEngine>
#include <QQmlContext>
#include <QString>

class I18nStub : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int language MEMBER m_language CONSTANT)
public:
    using QObject::QObject;
    Q_INVOKABLE QString t(const QString &key) const { return key; }
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
        engine->rootContext()->setContextProperty(
            QStringLiteral("i18n"), new I18nStub(engine));
    }
};

QUICK_TEST_MAIN_WITH_SETUP(batorrent_qml, TestSetup)

#include "main.moc"
