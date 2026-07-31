// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/discovery/addoncatalog.h"

namespace AddonCatalog {

QList<CuratedAddon> curatedCatalog()
{
    using L = QStringList;
    const L movieSeries = {QStringLiteral("movie"), QStringLiteral("series")};
    const L movieSeriesAnime = {QStringLiteral("movie"), QStringLiteral("series"), QStringLiteral("anime")};
    const L stream = {QStringLiteral("stream")};
    const L catalogMeta = {QStringLiteral("catalog"), QStringLiteral("meta")};
    const L catalogStream = {QStringLiteral("catalog"), QStringLiteral("stream")};
    const L animeMeta = {QStringLiteral("catalog"), QStringLiteral("meta"), QStringLiteral("subtitles")};

    return {
        {QStringLiteral("com.linvo.cinemeta"), QStringLiteral("Cinemeta"),
         QStringLiteral("addon_sug_cinemeta"),
         QStringLiteral("Official catalogs for movies and series"),
         QStringLiteral("https://v3-cinemeta.strem.io"), {}, {},
         movieSeries, catalogMeta, true, true, false, false},

        {QStringLiteral("com.stremio.torrentio.addon"), QStringLiteral("Torrentio"),
         QStringLiteral("addon_sug_torrentio"),
         QStringLiteral("Torrent streams from many indexers (respects Content language)"),
         QStringLiteral("https://torrentio.strem.fun"), {}, {},
         movieSeries, stream, true, true, false, false},

        {QStringLiteral("com.stremio.brazuca.addon"), QStringLiteral("Brazuca Torrents"),
         QStringLiteral("addon_sug_brazuca"),
         QStringLiteral("Brazilian dubbed movies & series — BaixaFilmes, RedeTorrent, VacaTorrent…"),
         QStringLiteral("https://94c8cb9f702d-brazuca-torrents.baby-beamup.club"), {},
         QStringLiteral("pt"), movieSeriesAnime, stream, true, true, false, false},

        {QStringLiteral("community.anime.kitsu"), QStringLiteral("Anime Kitsu"),
         QStringLiteral("addon_sug_anime_kitsu"),
         QStringLiteral("Anime catalogs & episode meta (Kitsu / MAL / AniList ids)"),
         QStringLiteral("https://anime-kitsu.strem.fun"), {},
         QStringLiteral("anime"), movieSeriesAnime, animeMeta, true, true, false, false},

        {{}, QStringLiteral("Torrentio · Anime (Nyaa)"), QStringLiteral("addon_sug_torrentio_anime"),
         QStringLiteral("Nyaa.si, TokyoTosho, AniDex, HorribleSubs — anime torrents"),
         QStringLiteral("https://torrentio.strem.fun/providers=nyaasi,tokyotosho,anidex,horriblesubs,nekobt"),
         {}, QStringLiteral("anime"), movieSeriesAnime, stream, true, true, false, false},

        {{}, QStringLiteral("Torrentio · Português"), QStringLiteral("addon_sug_torrentio_pt"),
         QStringLiteral("Torrentio locked to Portuguese / dubbed BR indexers"),
         QStringLiteral("https://torrentio.strem.fun/providers=comando,bludv,micoleaodublado,yts,eztv,rarbg,1337x|language=portuguese"),
         {}, QStringLiteral("pt"), movieSeries, stream, true, false, false, false},
        {{}, QStringLiteral("Torrentio · Español"), QStringLiteral("addon_sug_torrentio_es"),
         QStringLiteral("Spanish / LATAM — Cinecalidad, MejorTorrent, Wolfmax4k…"),
         QStringLiteral("https://torrentio.strem.fun/providers=cinecalidad,mejortorrent,wolfmax4k,bludv,comando,yts,eztv,rarbg,1337x|language=spanish"),
         {}, QStringLiteral("es"), movieSeries, stream, true, false, false, false},
        {{}, QStringLiteral("Torrentio · Русский"), QStringLiteral("addon_sug_torrentio_ru"),
         QStringLiteral("Russian — Rutor, RuTracker and language filter"),
         QStringLiteral("https://torrentio.strem.fun/providers=rutor,rutracker,yts,eztv,rarbg,1337x,thepiratebay|language=russian"),
         {}, QStringLiteral("ru"), movieSeries, stream, true, false, false, false},
        {{}, QStringLiteral("Torrentio · 中文"), QStringLiteral("addon_sug_torrentio_zh"),
         QStringLiteral("Chinese — language-first Torrentio results"),
         QStringLiteral("https://torrentio.strem.fun/language=chinese"),
         {}, QStringLiteral("zh"), movieSeries, stream, true, false, false, false},
        {{}, QStringLiteral("Torrentio · 日本語"), QStringLiteral("addon_sug_torrentio_ja"),
         QStringLiteral("Japanese — language-first Torrentio results"),
         QStringLiteral("https://torrentio.strem.fun/language=japanese"),
         {}, QStringLiteral("ja"), movieSeries, stream, true, false, false, false},
        {{}, QStringLiteral("Torrentio · Deutsch"), QStringLiteral("addon_sug_torrentio_de"),
         QStringLiteral("German — language-first Torrentio results"),
         QStringLiteral("https://torrentio.strem.fun/language=german"),
         {}, QStringLiteral("de"), movieSeries, stream, true, false, false, false},
        {{}, QStringLiteral("Torrentio · Türkçe"), QStringLiteral("addon_sug_torrentio_tr"),
         QStringLiteral("Turkish — language-first Torrentio results"),
         QStringLiteral("https://torrentio.strem.fun/language=turkish"),
         {}, QStringLiteral("tr"), movieSeries, stream, true, false, false, false},
        {{}, QStringLiteral("Torrentio · Українська"), QStringLiteral("addon_sug_torrentio_uk"),
         QStringLiteral("Ukrainian — language-first Torrentio results"),
         QStringLiteral("https://torrentio.strem.fun/language=ukrainian"),
         {}, QStringLiteral("uk"), movieSeries, stream, true, false, false, false},

        {QStringLiteral("org.reptilia.aradeb"), QStringLiteral("Aradeb"),
         QStringLiteral("addon_sug_aradeb"),
         QStringLiteral("Arabic catalogs & streams — needs a free trial or donor key + debrid"),
         {}, QStringLiteral("https://aradeb.518878.xyz/configure"),
         QStringLiteral("ar"), movieSeries, catalogStream, false, false, true, true},

        {{}, QStringLiteral("Torrentio · Configure"), QStringLiteral("addon_sug_torrentio_cfg"),
         QStringLiteral("Open Torrentio’s config page — pick providers, language, optional debrid — then paste the install link"),
         {}, QStringLiteral("https://torrentio.strem.fun/configure"),
         {}, movieSeries, stream, false, false, true, false},
    };
}

QList<ProviderPreset> providerCatalog()
{
    auto torApi = [](const QString &id, const QString &name, const QString &tracker,
                     const QString &region, const QString &note, bool selfHost = false) {
        ProviderPreset ps;
        SearchProvider &p = ps.provider;
        p.id = id;
        p.name = name;
        p.urlTemplate = QStringLiteral("https://torapi.vercel.app/api/search/title/%1?query={query}").arg(tracker);
        p.arrayPath = QString();
        p.namePath = QStringLiteral("Name");
        p.hashPath = QStringLiteral("Hash");
        p.sizePath = QStringLiteral("Size");
        p.seedersPath = QStringLiteral("Seeds");
        p.leechersPath = QStringLiteral("Peers");
        p.builtIn = true;
        p.region = region;
        ps.note = note;
        ps.needsConfig = selfHost;
        return ps;
    };
    auto json = [](const QString &id, const QString &name, const QString &url,
                   const QString &arr, const QString &nm, const QString &hash,
                   const QString &sz, const QString &seed, const QString &leech,
                   const QString &region, const QString &note, bool needsConfig = false) {
        ProviderPreset ps;
        SearchProvider &p = ps.provider;
        p.id = id; p.name = name; p.urlTemplate = url;
        p.arrayPath = arr; p.namePath = nm; p.hashPath = hash;
        p.sizePath = sz; p.seedersPath = seed; p.leechersPath = leech;
        p.builtIn = true; p.region = region;
        ps.note = note; ps.needsConfig = needsConfig;
        return ps;
    };

    QList<ProviderPreset> cat;

    {
        ProviderPreset ps;
        SearchProvider &p = ps.provider;
        p.id = QStringLiteral("torrentindexer_ptbr");
        p.name = QStringLiteral("Comando/BluDV… (torrent-indexer)");
        p.urlTemplate = QStringLiteral("http://127.0.0.1:7006/search?q={query}");
        p.arrayPath = QStringLiteral("results");
        p.namePath = QStringLiteral("title");
        p.hashPath = QStringLiteral("info_hash");
        p.sizePath = QStringLiteral("size");
        p.seedersPath = QStringLiteral("seed_count");
        p.leechersPath = QStringLiteral("leech_count");
        p.builtIn = true;
        p.region = QStringLiteral("ptbr");
        ps.note = QStringLiteral("Filmes/séries PT-BR. Exige rodar o torrent-indexer localmente (Docker) — edite a URL.");
        ps.needsConfig = true;
        cat << ps;
    }

    cat << torApi(QStringLiteral("rutor_torapi"), QStringLiteral("RuTor"), QStringLiteral("rutor"),
                  QStringLiteral("cis"),
                  QStringLiteral("Tracker russo público. Consulta via instância TorAPI de terceiros (editável)."));
    cat << torApi(QStringLiteral("rutracker_torapi"), QStringLiteral("RuTracker (self-host)"),
                  QStringLiteral("rutracker"), QStringLiteral("cis"),
                  QStringLiteral("Exige login → só funciona apontando para uma TorAPI própria com sua conta."),
                  /*selfHost*/ true);

    cat << json(QStringLiteral("jackett_local"), QStringLiteral("Jackett (todos os seus indexers)"),
                QStringLiteral("http://127.0.0.1:9117/api/v2.0/indexers/all/results?apikey=API_KEY&Query={query}"),
                QStringLiteral("Results"), QStringLiteral("Title"), QStringLiteral("InfoHash"),
                QStringLiteral("Size"), QStringLiteral("Seeders"), QStringLiteral("Peers"),
                QStringLiteral("self"),
                QStringLiteral("Uma fonte cobre TODOS os indexers localizados do seu Jackett (PT-BR, ES…). Troque API_KEY."),
                /*needsConfig*/ true);

    return cat;
}

QList<DefaultProvider> defaultProviders()
{
    return {
        {QStringLiteral("apibay"), QStringLiteral("The Pirate Bay (apibay)"),
         QStringLiteral("https://apibay.org/q.php?q={query}&cat={category}"),
         QString(), QStringLiteral("name"), QStringLiteral("info_hash"),
         QStringLiteral("size"), QStringLiteral("seeders"), QStringLiteral("leechers"),
         QStringLiteral("global"), true},
        {QStringLiteral("nyaa_api"), QStringLiteral("Nyaa.si"),
         QStringLiteral("https://nyaa.si/api/v2?q={query}&limit=50"),
         QStringLiteral("torrents"), QStringLiteral("title"), QStringLiteral("info_hash"),
         QStringLiteral("total_size"), QStringLiteral("seeders"), QStringLiteral("leechers"),
         QStringLiteral("anime"), true},
        {QStringLiteral("torrents_csv"), QStringLiteral("Torrents-CSV"),
         QStringLiteral("https://torrents-csv.com/service/search?q={query}&size=50"),
         QStringLiteral("torrents"), QStringLiteral("name"), QStringLiteral("infohash"),
         QStringLiteral("size_bytes"), QStringLiteral("seeders"), QStringLiteral("leechers"),
         QStringLiteral("global"), true},
        {QStringLiteral("bitsearch"), QStringLiteral("BitSearch (multi-idioma)"),
         QStringLiteral("https://bitsearch.eu/api/v1/search?q={query}&limit=50"),
         QStringLiteral("results"), QStringLiteral("title"), QStringLiteral("infohash"),
         QStringLiteral("size"), QStringLiteral("seeders"), QStringLiteral("leechers"),
         QStringLiteral("global"), true},
        {QStringLiteral("rutor_torapi"), QStringLiteral("RuTor"),
         QStringLiteral("https://torapi.vercel.app/api/search/title/rutor?query={query}"),
         QString(), QStringLiteral("Name"), QStringLiteral("Hash"),
         QStringLiteral("Size"), QStringLiteral("Seeds"), QStringLiteral("Peers"),
         QStringLiteral("cis"), true},
    };
}

} // namespace AddonCatalog
