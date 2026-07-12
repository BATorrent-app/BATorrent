// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/platform/soundplayer.h"

#include <QAudioSink>
#include <QAudioFormat>
#include <QMediaDevices>
#include <QBuffer>
#include <QtMath>

namespace {

// Two-note rising chime (A5 -> D6, a clean major-fourth) with a short
// fade-in/out envelope on each note so there's no click at the edges.
QByteArray synthesizeChime(const QAudioFormat &format)
{
    const int sampleRate = format.sampleRate();
    if (sampleRate <= 0) return {};

    const double noteSecs = 0.14;
    const double fadeSecs = 0.02;
    const double freqs[] = {880.0, 1174.66};

    QByteArray pcm;
    const int samplesPerNote = static_cast<int>(sampleRate * noteSecs);
    pcm.reserve(static_cast<int>(sizeof(qint16)) * samplesPerNote * 2);

    for (double freq : freqs) {
        for (int i = 0; i < samplesPerNote; ++i) {
            const double t = double(i) / sampleRate;
            double env = 1.0;
            if (t < fadeSecs) env = t / fadeSecs;
            else if (t > noteSecs - fadeSecs) env = (noteSecs - t) / fadeSecs;
            const double sample = qSin(2.0 * M_PI * freq * t) * env * 0.5;
            const qint16 s16 = static_cast<qint16>(sample * 32767);
            pcm.append(reinterpret_cast<const char *>(&s16), sizeof(s16));
        }
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
