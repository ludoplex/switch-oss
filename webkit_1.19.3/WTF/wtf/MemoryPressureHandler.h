/*
 * Copyright (C) 2011-2017 Apple Inc. All Rights Reserved.
 * Copyright (C) 2014 Raspberry Pi Foundation. All Rights Reserved.
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

#include <atomic>
#include <ctime>
#include <wtf/FastMalloc.h>
#include <wtf/Forward.h>
#include <wtf/Function.h>
#include <wtf/Optional.h>
#include <wtf/RunLoop.h>

#if USE(GLIB)
#include <wtf/glib/GRefPtr.h>
#endif

#if OS(WINDOWS)
#include <wtf/win/Win32Handle.h>
#endif

namespace WTF {

enum class MemoryUsagePolicy {
    Unrestricted, // Allocate as much as you want
    Conservative, // Maybe you don't cache every single thing
    Strict, // Time to start pinching pennies for real
};

enum class WebsamProcessState {
    Active,
    Inactive,
};

enum class Critical { No, Yes };
enum class Synchronous { No, Yes };

typedef WTF::Function<void(Critical, Synchronous)> LowMemoryHandler;

class MemoryPressureHandler {
    friend class WTF::NeverDestroyed<MemoryPressureHandler>;
public:
    WTF_EXPORT_PRIVATE static MemoryPressureHandler& singleton();

    WTF_EXPORT_PRIVATE void install();

    WTF_EXPORT_PRIVATE void setShouldUsePeriodicMemoryMonitor(bool);

#if OS(LINUX)
    WTF_EXPORT_PRIVATE void triggerMemoryPressureEvent(bool isCritical);
#endif

    void setMemoryKillCallback(WTF::Function<void()>&& function) { m_memoryKillCallback = WTFMove(function); }
    void setMemoryPressureStatusChangedCallback(WTF::Function<void(bool)>&& function) { m_memoryPressureStatusChangedCallback = WTFMove(function); }
    void setDidExceedInactiveLimitWhileActiveCallback(WTF::Function<void()>&& function) { m_didExceedInactiveLimitWhileActiveCallback = WTFMove(function); }

    void setLowMemoryHandler(LowMemoryHandler&& handler)
    {
        m_lowMemoryHandler = WTFMove(handler);
    }

    bool isUnderMemoryPressure() const
    {
        return m_underMemoryPressure
#if PLATFORM(MAC)
            || m_memoryUsagePolicy >= MemoryUsagePolicy::Strict
#endif
            || m_isSimulatingMemoryPressure;
    }
    void setUnderMemoryPressure(bool);

    WTF_EXPORT_PRIVATE static MemoryUsagePolicy currentMemoryUsagePolicy();

    class ReliefLogger {
    public:
        explicit ReliefLogger(const char *log)
            : m_logString(log)
            , m_initialMemory(loggingEnabled() ? platformMemoryUsage() : MemoryUsage { })
        {
        }

        ~ReliefLogger()
        {
            if (loggingEnabled())
                logMemoryUsageChange();
        }


        const char* logString() const { return m_logString; }
        static void setLoggingEnabled(bool enabled) { s_loggingEnabled = enabled; }
        static bool loggingEnabled()
        {
#if RELEASE_LOG_DISABLED
            return s_loggingEnabled;
#else
            return true;
#endif
        }

    private:
        struct MemoryUsage {
            MemoryUsage() = default;
            MemoryUsage(size_t resident, size_t physical)
                : resident(resident)
                , physical(physical)
            {
            }
            size_t resident { 0 };
            size_t physical { 0 };
        };
        std::optional<MemoryUsage> platformMemoryUsage();
        void logMemoryUsageChange();

        const char* m_logString;
        std::optional<MemoryUsage> m_initialMemory;

#if !PLATFORM(WKC)
        WTF_EXPORT_PRIVATE static bool s_loggingEnabled;
#else
        WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY_ZERO(bool, s_loggingEnabled);
#endif
    };

    WTF_EXPORT_PRIVATE void releaseMemory(Critical, Synchronous = Synchronous::No);

    WTF_EXPORT_PRIVATE void beginSimulatedMemoryPressure();
    WTF_EXPORT_PRIVATE void endSimulatedMemoryPressure();

    WTF_EXPORT_PRIVATE void setProcessState(WebsamProcessState);
    WebsamProcessState processState() const { return m_processState; }

    WTF_EXPORT_PRIVATE static void setPageCount(unsigned);

private:
    size_t thresholdForMemoryKill();
    void memoryPressureStatusChanged();

    void uninstall();

    void holdOff(Seconds);

    MemoryPressureHandler();
    ~MemoryPressureHandler() = delete;

    void respondToMemoryPressure(Critical, Synchronous = Synchronous::No);
    void platformReleaseMemory(Critical);
    void platformInitialize();

    void measurementTimerFired();
    void shrinkOrDie();
    void setMemoryUsagePolicyBasedOnFootprint(size_t);
    void doesExceedInactiveLimitWhileActive();
    void doesNotExceedInactiveLimitWhileActive();

    WebsamProcessState m_processState { WebsamProcessState::Inactive };

    unsigned m_pageCount { 0 };

    bool m_installed { false };
    LowMemoryHandler m_lowMemoryHandler;

    std::atomic<bool> m_underMemoryPressure;
    bool m_isSimulatingMemoryPressure { false };

    std::unique_ptr<RunLoop::Timer<MemoryPressureHandler>> m_measurementTimer;
    MemoryUsagePolicy m_memoryUsagePolicy { MemoryUsagePolicy::Unrestricted };
    WTF::Function<void()> m_memoryKillCallback;
    WTF::Function<void(bool)> m_memoryPressureStatusChangedCallback;
    WTF::Function<void()> m_didExceedInactiveLimitWhileActiveCallback;
    bool m_hasInvokedDidExceedInactiveLimitWhileActiveCallback { false };

#if OS(WINDOWS)
    void windowsMeasurementTimerFired();
    RunLoop::Timer<MemoryPressureHandler> m_windowsMeasurementTimer;
    Win32Handle m_lowMemoryHandle;
#endif

#if OS(LINUX)
    RunLoop::Timer<MemoryPressureHandler> m_holdOffTimer;
    void holdOffTimerFired();
#endif
};

extern WTFLogChannel LogMemoryPressure;

} // namespace WTF

using WTF::Critical;
using WTF::MemoryPressureHandler;
using WTF::Synchronous;
using WTF::WebsamProcessState;
