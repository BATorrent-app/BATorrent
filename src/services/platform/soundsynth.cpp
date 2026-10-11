// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/platform/soundsynth.h"

#include <QtMath>
#include <algorithm>
#include <cmath>
#include <vector>

namespace SoundSynth {

namespace {

struct Note {
    double freq;
    double at;      // onset, seconds
    double gain;
};

struct Partial {
    double ratio;   // to the fundamental; slightly off-integer is what makes it a bell, not an organ
    double gain;
    double tau;     // its own ring-out: the high ones die first, which is what keeps it soft
};

// A soft struck tone, between a celesta and a kalimba.
const Partial kBell[] = {
    { 1.000, 1.00, 0.55 },
    { 2.005, 0.30, 0.32 },
    { 3.010, 0.10, 0.16 },
    { 4.170, 0.045, 0.07 },
};

struct Spec {
    std::vector<Note> notes;
    double length;   // seconds, tail included
    double shimmer;  // gain of a quiet partial two octaves up, 0 = none
};

Spec spec(Sound s)
{
    // A4 C#5 E5 A5: a major chord rising, and below the shrill register.
    switch (s) {
    case Sound::Completion:
        return { { { 659.26, 0.00, 0.80 }, { 880.00, 0.11, 0.70 } }, 1.65, 0.0 };
    case Sound::Achievement:
        return { { { 440.00, 0.00, 0.75 }, { 554.37, 0.075, 0.70 }, { 659.26, 0.15, 0.85 } }, 1.95, 0.0 };
    case Sound::AchievementRare:
        return { { { 440.00, 0.00, 0.70 }, { 554.37, 0.07, 0.65 }, { 659.26, 0.14, 0.75 },
                   { 880.00, 0.23, 0.85 } }, 2.50, 0.06 };
    }
    return {};
}

// One channel, dry. `detune` in cents spreads the two channels apart.
std::vector<double> voice(const Spec &sp, int rate, double detuneCents)
{
    const int n = int(sp.length * rate);
    std::vector<double> out(size_t(std::max(n, 0)), 0.0);
    const double det = std::pow(2.0, detuneCents / 1200.0);
    const double attack = 0.005;
    for (const Note &note : sp.notes) {
        const int onset = int(note.at * rate);
        for (int i = onset; i < n; ++i) {
            const double t = double(i - onset) / rate;
            double env = t < attack ? 0.5 - 0.5 * std::cos(M_PI * t / attack) : 1.0;
            double v = 0;
            for (const Partial &p : kBell)
                v += p.gain * std::exp(-t / p.tau) * std::sin(2 * M_PI * note.freq * det * p.ratio * t);
            if (sp.shimmer > 0)
                v += sp.shimmer * std::exp(-t / 0.4) * std::sin(2 * M_PI * note.freq * det * 4.0 * t);
            out[size_t(i)] += note.gain * env * v;
        }
    }
    return out;
}

// Gentle one-pole low-pass: takes the glassy edge off without dulling it.
void lowPass(std::vector<double> &x, int rate, double cutoff)
{
    const double a = 1.0 - std::exp(-2 * M_PI * cutoff / rate);
    double y = 0;
    for (double &s : x) { y += a * (s - y); s = y; }
}

// A few short, darkened echoes: enough room that it doesn't sound pasted on.
void room(std::vector<double> &x, int rate, const double (&taps)[3][2])
{
    std::vector<double> wet(x.size(), 0.0);
    for (const auto &tap : taps) {
        const size_t d = size_t(tap[0] * rate);
        for (size_t i = d; i < x.size(); ++i) wet[i] += tap[1] * x[i - d];
    }
    lowPass(wet, rate, 3500);
    for (size_t i = 0; i < x.size(); ++i) x[i] += wet[i];
}

} // namespace

QVector<float> render(Sound sound, int sampleRate, int channels)
{
    if (sampleRate < 8000 || (channels != 1 && channels != 2)) return {};
    const Spec sp = spec(sound);

    static const double tapsL[3][2] = { { 0.023, 0.28 }, { 0.041, 0.18 }, { 0.067, 0.11 } };
    static const double tapsR[3][2] = { { 0.029, 0.26 }, { 0.047, 0.17 }, { 0.073, 0.10 } };
    std::vector<double> l = voice(sp, sampleRate, -1.5);
    std::vector<double> r = channels == 2 ? voice(sp, sampleRate, +1.5) : std::vector<double>{};
    room(l, sampleRate, tapsL);
    lowPass(l, sampleRate, 7000);
    if (channels == 2) { room(r, sampleRate, tapsR); lowPass(r, sampleRate, 7000); }

    // Fade the last stretch to nothing, then set the level: peak at -6 dBFS.
    const size_t n = l.size();
    const size_t fade = std::min(n, size_t(0.35 * sampleRate));
    double peak = 1e-9;
    for (size_t i = 0; i < n; ++i) {
        double g = 1.0;
        if (i >= n - fade) g = 0.5 + 0.5 * std::cos(M_PI * double(i - (n - fade)) / double(fade));
        l[i] *= g;
        peak = std::max(peak, std::abs(l[i]));
        if (channels == 2) { r[i] *= g; peak = std::max(peak, std::abs(r[i])); }
    }
    const double scale = 0.5 / peak;
    QVector<float> out;
    out.reserve(int(n) * channels);
    for (size_t i = 0; i < n; ++i) {
        out.append(float(l[i] * scale));
        if (channels == 2) out.append(float(r[i] * scale));
    }
    if (!out.isEmpty()) {
        out.first() = 0.f;
        out.last() = 0.f;
        if (channels == 2) { out[1] = 0.f; out[out.size() - 2] = 0.f; }
    }
    return out;
}

} // namespace SoundSynth
