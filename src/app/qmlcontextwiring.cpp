// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "app/qmlcontextwiring.h"

#include "bridges/qmlposterbridge.h"
#include "services/discovery/discoveryservice.h"
#include "services/integrations/debridmanager.h"
#include "services/vpn/vpnmanager.h"

#include <QQmlContext>
#include <cstdlib>

namespace QmlContextWiring {

void registerProperties(QQmlContext *ctx, const QmlContextObjects &o)
{
    ctx->setContextProperty("torrentModel", o.torrentFilter);
    ctx->setContextProperty("torrentFilter", o.torrentFilter);
    ctx->setContextProperty("themeBridge", o.themeBridge);
    ctx->setContextProperty("session", o.session);
    ctx->setContextProperty("rss", o.rss);
    ctx->setContextProperty("settings", o.settings);
    ctx->setContextProperty("addons", o.addons);
    ctx->setContextProperty("search", o.search);
    ctx->setContextProperty("discovery", o.discovery);
#ifdef BAT_STORE_BUILD
    ctx->setContextProperty("isStoreBuild", true);
#else
    ctx->setContextProperty("isStoreBuild", false);
#endif
    ctx->setContextProperty("logs", o.logs);
    ctx->setContextProperty("subsearch", o.subsearch);
    ctx->setContextProperty("pairing", o.pairing);
    ctx->setContextProperty("debrid", o.debrid);
    ctx->setContextProperty("notifications", o.notifications);
    ctx->setContextProperty("i18n", o.i18n);
    ctx->setContextProperty("batSmokeLoaders", qEnvironmentVariableIsSet("BAT_SMOKE_LOADERS"));
#ifndef BAT_STORE_BUILD
    ctx->setContextProperty("updater", o.updater);
    ctx->setContextProperty("vpn", o.vpn);
#else
    Q_UNUSED(o.updater);
    Q_UNUSED(o.vpn);
    ctx->setContextProperty("updater", nullptr);
#endif
}

} // namespace QmlContextWiring
