// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/platform/soundplayer.h"

#include <QAudioSink>
#include <QAudioFormat>
#include <QMediaDevices>
#include <QBuffer>
#include <QtMath>
#include <cstddef>
#include <algorithm>
#include <vector>

namespace {

// A warm two-note "ta-da" chime (G5 -> D6, a rising perfect fifth). The old
// version was two pure sine beeps in sequence — which is exactly why it read as
// a BIOS beep. Two things fix that: a struck-bell timbre (fundamental plus a
// few decaying harmonics) and an exponential decay envelope so each note rings
// out and rings into the next instead of stopping dead.
QByteArray synthesizeChime(const QAudioFormat &format)
{
    const int sampleRate = format.sampleRate();
    if (sampleRate <= 0) return {};

    struct Note { double freq; double start; };            // start = onset offset (s)
    const Note notes[] = { {783.99, 0.0}, {1174.66, 0.13} };  // G5, D6 — overlapping
    // harmonic weights: fundamental + octave + fifth + 2nd octave, each softer,
    // give a soft mallet/glass tone rather than a flat sine
    const double harm[] = {1.0, 0.5, 0.28, 0.12};
    const double noteTail = 0.75;                          // seconds a note rings
    const double attack   = 0.006;                         // click-free onset
    const double decayTau = 0.26;                          // exponential ring-out

    const double totalSecs = notes[std::size(notes) - 1].start + noteTail;
    const int totalSamples = static_cast<int>(sampleRate * totalSecs);

    std::vector<double> mix(static_cast<std::size_t>(totalSamples), 0.0);
    for (const Note &n : notes) {
        const int onset = static_cast<int>(n.start * sampleRate);
        const int len   = static_cast<int>(noteTail * sampleRate);
        for (int i = 0; i < len && onset + i < totalSamples; ++i) {
            const double t = double(i) / sampleRate;
            double env = qExp(-t / decayTau);              // struck-bell decay
            if (t < attack) env *= t / attack;             // soft attack, no click
            double s = 0.0;
            for (std::size_t h = 0; h < std::size(harm); ++h)
                s += harm[h] * qSin(2.0 * M_PI * n.freq * (h + 1) * t);
            mix[static_cast<std::size_t>(onset + i)] += s * env;
        }
    }

    QByteArray pcm;
    pcm.reserve(static_cast<int>(sizeof(qint16)) * totalSamples);
    for (double v : mix) {
        v *= 0.22;                                         // headroom for the summed harmonics
        v = std::clamp(v, -1.0, 1.0);
        const qint16 s16 = static_cast<qint16>(v * 32767);
        pcm.append(reinterpret_cast<const char *>(&s16), sizeof(s16));
    }
    return pcm;
}

} // namespace

void SoundPlayer::playCompletionChime()
{
    QAudioFormat format;
    format.setSampleRate(44100);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);

    const QAudioDevice dev = QMediaDevices::defaultAudioOutput();
    if (dev.isNull() || !dev.isFormatSupported(format)) return;

    const QByteArray pcm = synthesizeChime(format);
    if (pcm.isEmpty()) return;

    auto *sink = new QAudioSink(dev, format);
    auto *buf = new QBuffer(sink);
    buf->setData(pcm);
    buf->open(QIODevice::ReadOnly);

    QObject::connect(sink, &QAudioSink::stateChanged, sink, [sink](QAudio::State state) {
        if (state == QAudio::IdleState || state == QAudio::StoppedState)
            sink->deleteLater();   // buf is a child of sink — goes with it
    });
    sink->start(buf);
}
