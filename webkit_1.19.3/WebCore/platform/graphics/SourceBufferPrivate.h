/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 * Copyright (C) 2013-2017 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#if ENABLE(MEDIA_SOURCE)

#include "MediaPlayer.h"

namespace WebCore {

class MediaSample;
class SourceBufferPrivateClient;

class SourceBufferPrivate : public RefCounted<SourceBufferPrivate> {
public:
    virtual ~SourceBufferPrivate() = default;

    virtual void setClient(SourceBufferPrivateClient*) = 0;

    virtual void append(Vector<unsigned char>&&) = 0;
    virtual void abort() = 0;
    virtual void resetParserState() = 0;
#if !PLATFORM(WKC)
    virtual void removedFromMediaSource() = 0;
#else
    virtual bool setTimestampOffset(double) = 0;
    virtual bool removedFromMediaSource() = 0;
    virtual void removeCodedFrames(double, double) = 0;
    virtual void evictCodedFrames(MediaTime* out_start, MediaTime* out_end) = 0;
    virtual bool isFull() = 0;
#endif

    virtual MediaPlayer::ReadyState readyState() const = 0;
    virtual void setReadyState(MediaPlayer::ReadyState) = 0;

    virtual void flush(const AtomicString&) { }
    virtual void enqueueSample(Ref<MediaSample>&&, const AtomicString&) { }
    virtual void allSamplesInTrackEnqueued(const AtomicString&) { }
    virtual bool isReadyForMoreSamples(const AtomicString&) { return false; }
    virtual void setActive(bool) { }
    virtual void stopAskingForMoreSamples(const AtomicString&) { }
    virtual void notifyClientWhenReadyForMoreSamples(const AtomicString&) { }

    virtual Vector<String> enqueuedSamplesForTrackID(const AtomicString&) { return { }; }
};

}

#endif
