/*
 * Copyright (C) 2013-2017 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#pragma once

#if ENABLE(MEDIA_SOURCE)

#include "SourceBufferPrivate.h"

namespace WebCore {

class AudioTrackPrivate;
class InbandTextTrackPrivate;
class MockInitializationBox;
class MockMediaSourcePrivate;
class MockSampleBox;
class TimeRanges;
class VideoTrackPrivate;

class MockSourceBufferPrivate final : public SourceBufferPrivate {
public:
    static RefPtr<MockSourceBufferPrivate> create(MockMediaSourcePrivate*);
    virtual ~MockSourceBufferPrivate();

    void clearMediaSource() { m_mediaSource = nullptr; }

    bool hasVideo() const;
    bool hasAudio() const;

    void seekToTime(const MediaTime&);
    MediaTime fastSeekTimeForMediaTime(const MediaTime&, const MediaTime& negativeThreshold, const MediaTime& positiveThreshold);

private:
    explicit MockSourceBufferPrivate(MockMediaSourcePrivate*);

    // SourceBufferPrivate overrides
    void setClient(SourceBufferPrivateClient*) final;
    void append(Vector<unsigned char>&&) final;
    void abort() final;
    void resetParserState() final;
#if PLATFORM(WKC)
    bool setTimestampOffset(double) override final { return true; }
    bool removedFromMediaSource() override final;
    void removeCodedFrames(double, double) override final {}
    void evictCodedFrames(MediaTime* out_start, MediaTime* out_end) final {}
    bool isFull() final { return false; }
#else
    void removedFromMediaSource() final;
#endif
    MediaPlayer::ReadyState readyState() const final;
    void setReadyState(MediaPlayer::ReadyState) final;

    void flush(const AtomicString&) final { m_enqueuedSamples.clear(); }
    void enqueueSample(Ref<MediaSample>&&, const AtomicString&) final;
    bool isReadyForMoreSamples(const AtomicString&) final { return true; }
    void setActive(bool) final;

    Vector<String> enqueuedSamplesForTrackID(const AtomicString&) final;

    void didReceiveInitializationSegment(const MockInitializationBox&);
    void didReceiveSample(const MockSampleBox&);

    MockMediaSourcePrivate* m_mediaSource;
    SourceBufferPrivateClient* m_client;
    Vector<String> m_enqueuedSamples;

    Vector<char> m_inputBuffer;
};

}

#endif // ENABLE(MEDIA_SOURCE) && USE(AVFOUNDATION)
