#include <catch2/catch_test_macros.hpp>
#include "services/platform/soundsynth.h"

#include <QFile>
#include <cmath>

using SoundSynth::Sound;

namespace {

// Writes the sound as a 16-bit WAV when BAT_SOUND_DIR is set, so a person can
// listen to what the numbers below only describe.
void maybeExport(const QVector<float> &s, int rate, int ch, const char *name)
{
    const QByteArray dir = qgetenv("BAT_SOUND_DIR");
    if (dir.isEmpty()) return;
    QFile f(QString::fromLocal8Bit(dir) + QLatin1Char('/') + QLatin1String(name));
    if (!f.open(QIODevice::WriteOnly)) return;
    const quint32 data = quint32(s.size() * 2);
    auto u32 = [&f](quint32 v) { f.write(reinterpret_cast<const char *>(&v), 4); };
    auto u16 = [&f](quint16 v) { f.write(reinterpret_cast<const char *>(&v), 2); };
    f.write("RIFF"); u32(36 + data); f.write("WAVEfmt "); u32(16); u16(1); u16(quint16(ch));
    u32(quint32(rate)); u32(quint32(rate * ch * 2)); u16(quint16(ch * 2)); u16(16); f.write("data"); u32(data);
    for (float v : s) { const qint16 x = qint16(std::lround(v * 32767.f)); f.write(reinterpret_cast<const char *>(&x), 2); }
}

}

TEST_CASE("every sound starts and ends on silence, peaks at -6 dBFS and never jumps", "[soundsynth]") {
    const struct { Sound s; const char *file; double minSecs, maxSecs; } all[] = {
        { Sound::Completion, "completion.wav", 1.0, 2.0 },
        { Sound::Achievement, "achievement.wav", 1.2, 2.2 },
        { Sound::AchievementRare, "achievement_rare.wav", 1.5, 2.8 },
    };
    for (const auto &c : all) {
        for (int ch : { 1, 2 }) {
            const int rate = 44100;
            const QVector<float> s = SoundSynth::render(c.s, rate, ch);
            if (ch == 2) maybeExport(s, rate, ch, c.file);
            REQUIRE(!s.isEmpty());
            REQUIRE(s.size() % ch == 0);
            const double secs = double(s.size() / ch) / rate;
            REQUIRE(secs >= c.minSecs);
            REQUIRE(secs <= c.maxSecs);
            float peak = 0, jump = 0;
            for (int i = 0; i < s.size(); ++i) {
                REQUIRE(std::isfinite(s[i]));
                peak = std::max(peak, std::abs(s[i]));
                if (i >= ch) jump = std::max(jump, std::abs(s[i] - s[i - ch]));
            }
            REQUIRE(std::abs(peak - 0.5f) < 0.01f);   // the level is set, not left to chance
            REQUIRE(jump < 0.2f);                      // no step a speaker would turn into a click
            for (int k = 0; k < ch; ++k) {
                REQUIRE(s[k] == 0.f);
                REQUIRE(s[s.size() - 1 - k] == 0.f);
            }
            // The last tenth of a second is near silence: the tail rings out, it isn't cut.
            const int tail = int(0.1 * rate) * ch;
            float tailPeak = 0;
            for (int i = s.size() - tail; i < s.size(); ++i) tailPeak = std::max(tailPeak, std::abs(s[i]));
            REQUIRE(tailPeak < 0.02f);
        }
    }
}

TEST_CASE("the rare achievement is longer and richer than the plain one", "[soundsynth]") {
    const auto plain = SoundSynth::render(Sound::Achievement, 44100, 1);
    const auto rare = SoundSynth::render(Sound::AchievementRare, 44100, 1);
    REQUIRE(rare.size() > plain.size());
}

TEST_CASE("nonsense formats give nothing rather than garbage", "[soundsynth]") {
    REQUIRE(SoundSynth::render(Sound::Achievement, 100, 1).isEmpty());
    REQUIRE(SoundSynth::render(Sound::Achievement, 44100, 6).isEmpty());
}
