// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include "services/platform/translator.h"

#include <QString>

// The language the user wants their *content* in: dubs, subtitles, TMDB titles
// which is not the same question as the language of the app chrome. Someone
// can read English menus fine and still need Portuguese audio.
//
// Also the single owner of the per-language codes (TMDB and subtitles), so no
// caller keeps its own table that can fall out of step with the enum.
namespace ContentLanguage {

// The user's choice, or the app language while they haven't made one.
Translator::Language current();
void set(Translator::Language lang);
void followApp();          // clear the explicit choice
bool followsApp();         // no explicit choice stored

// Release-name tag ("PT", "ES", …): what AudioMode classifies against.
QString releaseTag(Translator::Language lang);
QString releaseTag();

// TMDB `language=` value ("pt-BR").
QString tmdb(Translator::Language lang);
QString tmdb();

// ISO 3166 country for TMDB's country-relative rows. With no explicit choice the
// system locale still wins ("pt_BR" → BR), since it's the better guess.
QString region();

// Subtitle-provider language code ("pt").
QString subtitleCode(Translator::Language lang);
QString subtitleCode();

}
