// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/platform/soundplayer.h"

#include <QAudioFormat>
#include <QAudioSink>
#include <QBuffer>
#include <QMediaDevices>
#include <cmath>

void SoundPlayer::play(SoundSynth::Sound sound)
{
    const QAudioDevice dev = QMediaDevices::defaultAudioOutput();
    if (dev.isNull()) return;

    QAudioFormat format;
    format.setSampleRate(44100);
    format.setSampleFormat(QAudioFormat::Int16);
    format.setChannelCount(2);
    if (!dev.isFormatSupported(format)) format.setChannelCount(1);
    if (!dev.isFormatSupported(format)) return;

    const QVector<float> samples = SoundSynth::render(sound, format.sampleRate(), format.channelCount());
    if (samples.isEmpty()) return;
    QByteArray pcm;
    pcm.resize(samples.size() * qsizetype(sizeof(qint16)));
    auto *out = reinterpret_cast<qint16 *>(pcm.data());
    for (qsizetype i = 0; i < samples.size(); ++i) out[i] = qint16(std::lround(samples[i] * 32767.f));

    auto *sink = new QAudioSink(dev, format);
    auto *buf = new QBuffer(sink);
    buf->setData(pcm);
    buf->open(QIODevice::ReadOnly);
    QObject::connect(sink, &QAudioSink::stateChanged, sink, [sink](QAudio::State state) {
        if (state == QAudio::IdleState || state == QAudio::StoppedState)
            sink->deleteLater();   // buf is a child of sink: goes with it
    });
    sink->start(buf);
}
