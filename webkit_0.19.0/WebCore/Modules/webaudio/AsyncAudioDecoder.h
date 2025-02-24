/*
 * Copyright (C) 2011, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef AsyncAudioDecoder_h
#define AsyncAudioDecoder_h

#include <memory>
#include <wtf/Forward.h>
#include <wtf/MessageQueue.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Threading.h>

namespace JSC {
class ArrayBuffer;
}

namespace WebCore {

class AudioBuffer;
class AudioBufferCallback;

// AsyncAudioDecoder asynchronously decodes audio file data from an ArrayBuffer in a worker thread.
// Upon successful decoding, a completion callback will be invoked with the decoded PCM data in an AudioBuffer.

class AsyncAudioDecoder {
    WTF_MAKE_NONCOPYABLE(AsyncAudioDecoder);
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
#endif
public:
    AsyncAudioDecoder();
    ~AsyncAudioDecoder();

    // Must be called on the main thread.
    void decodeAsync(JSC::ArrayBuffer* audioData, float sampleRate, PassRefPtr<AudioBufferCallback> successCallback, PassRefPtr<AudioBufferCallback> errorCallback);

#if !PLATFORM(WKC)
private:
#endif
    class DecodingTask {
        WTF_MAKE_NONCOPYABLE(DecodingTask);
#if PLATFORM(WKC)
        WTF_MAKE_FAST_ALLOCATED;
#endif
    public:
        DecodingTask(JSC::ArrayBuffer* audioData, float sampleRate, PassRefPtr<AudioBufferCallback> successCallback, PassRefPtr<AudioBufferCallback> errorCallback);
        void decode();
        
    private:
        JSC::ArrayBuffer* audioData() { return m_audioData.get(); }
        float sampleRate() const { return m_sampleRate; }
        AudioBufferCallback* successCallback() { return m_successCallback.get(); }
        AudioBufferCallback* errorCallback() { return m_errorCallback.get(); }
        AudioBuffer* audioBuffer() { return m_audioBuffer.get(); }

        static void notifyCompleteDispatch(void* userData);
        void notifyComplete();

        RefPtr<JSC::ArrayBuffer> m_audioData;
        float m_sampleRate;
        RefPtr<AudioBufferCallback> m_successCallback;
        RefPtr<AudioBufferCallback> m_errorCallback;
        RefPtr<AudioBuffer> m_audioBuffer;
    };

#if PLATFORM(WKC)
private:
#endif
    
    static void threadEntry(void* threadData);
    void runLoop();

#if !PLATFORM(WKC) // The members below are unnecessary because we create only one AudioDecoder thread.
    WTF::ThreadIdentifier m_threadID;
    Mutex m_threadCreationMutex;
    MessageQueue<DecodingTask> m_queue;
#endif
};

} // namespace WebCore

#endif // AsyncAudioDecoder_h
