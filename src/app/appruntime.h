// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef BATORRENT_APPRUNTIME_H
#define BATORRENT_APPRUNTIME_H

#include <QString>

class QApplication;
class QQmlImageProviderBase;

// Pre-QML runtime: graphics API, fonts, migrations, logging, Sentry, logo provider.
namespace AppRuntime {

void applyGraphicsApiPreference();
void runStartupMigrations();
void loadFonts(QApplication &app);
void logDependencyVersions();
void showQmlLoadFailure(const QString &logHint);
void installQtMessageHandler();

#ifdef BAT_HAVE_SENTRY
void initSentry(const QString &role);
#endif

QQmlImageProviderBase *createAppLogoProvider();

} // namespace AppRuntime

#endif
