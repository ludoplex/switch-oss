/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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

#include "config.h"
#include "ExecutableAllocator.h"

#include "JSCInlines.h"

#if ENABLE(EXECUTABLE_ALLOCATOR_DEMAND)
#include "CodeProfiling.h"
#include <wtf/HashSet.h>
#include <wtf/MetaAllocator.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/PageReservation.h>
#include <wtf/ThreadingPrimitives.h>
#include <wtf/VMTags.h>
#endif

#if PLATFORM(WKC)
#include <wkc/wkcmpeer.h>
#include <wkc/wkcheappeer.h>
#endif

// Uncomment to create an artificial executable memory usage limit. This limit
// is imperfect and is primarily useful for testing the VM's ability to handle
// out-of-executable-memory situations.
// #define EXECUTABLE_MEMORY_LIMIT 1000000

#if ENABLE(ASSEMBLER)

using namespace WTF;

namespace JSC {

#if ENABLE(EXECUTABLE_ALLOCATOR_DEMAND)

class DemandExecutableAllocator : public MetaAllocator {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
#endif
public:
    DemandExecutableAllocator()
        : MetaAllocator(jitAllocationGranule)
    {
        std::lock_guard<std::mutex> lock(allocatorsMutex());
        allocators().add(this);
        // Don't preallocate any memory here.
    }
    
    virtual ~DemandExecutableAllocator()
    {
        {
            std::lock_guard<std::mutex> lock(allocatorsMutex());
            allocators().remove(this);
        }
        for (unsigned i = 0; i < reservations.size(); ++i)
            reservations.at(i).deallocate();
    }

    static size_t bytesAllocatedByAllAllocators()
    {
        size_t total = 0;
        std::lock_guard<std::mutex> lock(allocatorsMutex());
        for (HashSet<DemandExecutableAllocator*>::const_iterator allocator = allocators().begin(); allocator != allocators().end(); ++allocator)
            total += (*allocator)->bytesAllocated();
        return total;
    }

    static size_t bytesCommittedByAllocactors()
    {
        size_t total = 0;
        std::lock_guard<std::mutex> lock(allocatorsMutex());
        for (HashSet<DemandExecutableAllocator*>::const_iterator allocator = allocators().begin(); allocator != allocators().end(); ++allocator)
            total += (*allocator)->bytesCommitted();
        return total;
    }

#if ENABLE(META_ALLOCATOR_PROFILE)
    static void dumpProfileFromAllAllocators()
    {
        std::lock_guard<std::mutex> lock(allocatorsMutex());
        for (HashSet<DemandExecutableAllocator*>::const_iterator allocator = allocators().begin(); allocator != allocators().end(); ++allocator)
            (*allocator)->dumpProfile();
    }
#endif

protected:
    virtual void* allocateNewSpace(size_t& numPages)
    {
        size_t newNumPages = (((numPages * pageSize() + JIT_ALLOCATOR_LARGE_ALLOC_SIZE - 1) / JIT_ALLOCATOR_LARGE_ALLOC_SIZE * JIT_ALLOCATOR_LARGE_ALLOC_SIZE) + pageSize() - 1) / pageSize();
        
        ASSERT(newNumPages >= numPages);
        
        numPages = newNumPages;
        
#ifdef EXECUTABLE_MEMORY_LIMIT
        if (bytesAllocatedByAllAllocators() >= EXECUTABLE_MEMORY_LIMIT)
            return 0;
#endif
        
        PageReservation reservation = PageReservation::reserve(numPages * pageSize(), OSAllocator::JSJITCodePages, EXECUTABLE_POOL_WRITABLE, true);
        RELEASE_ASSERT(reservation);
        
        reservations.append(reservation);
        
        return reservation.base();
    }
    
    virtual void notifyNeedPage(void* page)
    {
        OSAllocator::commit(page, pageSize(), EXECUTABLE_POOL_WRITABLE, true);
    }
    
    virtual void notifyPageIsFree(void* page)
    {
        OSAllocator::decommit(page, pageSize());
    }

private:
    Vector<PageReservation, 16> reservations;
    static HashSet<DemandExecutableAllocator*>& allocators()
    {
        DEPRECATED_DEFINE_STATIC_LOCAL(HashSet<DemandExecutableAllocator*>, sAllocators, ());
        return sAllocators;
    }

    static std::mutex& allocatorsMutex()
    {
#if !PLATFORM(WKC)
        static NeverDestroyed<std::mutex> mutex;

        return mutex;
#else
        WKC_DEFINE_STATIC_PTR(std::mutex*, mutex, 0);
        if (!mutex) {
            mutex = (std::mutex *)fastMalloc(sizeof(std::mutex));
            mutex = new (mutex) std::mutex();
        }
        return *mutex;
#endif
    }
};

#if ENABLE(ASSEMBLER_WX_EXCLUSIVE)
void ExecutableAllocator::initializeAllocator()
{
}
#else
#if !PLATFORM(WKC)
static DemandExecutableAllocator* gAllocator;
#else
WKC_DEFINE_GLOBAL_PTR(DemandExecutableAllocator*, gAllocator, 0);
#endif

namespace {
static inline DemandExecutableAllocator* allocator()
{
    return gAllocator;
}
}

void ExecutableAllocator::initializeAllocator()
{
    ASSERT(!gAllocator);
    gAllocator = new DemandExecutableAllocator();
    CodeProfiling::notifyAllocator(gAllocator);
}
#endif

ExecutableAllocator::ExecutableAllocator(VM&)
#if ENABLE(ASSEMBLER_WX_EXCLUSIVE)
    : m_allocator(std::make_unique<DemandExecutableAllocator>())
#endif
{
    ASSERT(allocator());
}

ExecutableAllocator::~ExecutableAllocator()
{
}

bool ExecutableAllocator::isValid() const
{
    return true;
}

bool ExecutableAllocator::underMemoryPressure()
{
#ifdef EXECUTABLE_MEMORY_LIMIT
    return DemandExecutableAllocator::bytesAllocatedByAllAllocators() > EXECUTABLE_MEMORY_LIMIT / 2;
#else
#if PLATFORM(WKC)
    size_t avail_bytes = wkcHeapGetHeapAvailableSizeForExecutableRegionPeer();
    size_t total_bytes = wkcHeapGetHeapTotalSizeForExecutableRegionPeer();
    size_t allocated_bytes = total_bytes - avail_bytes;

    return allocated_bytes > total_bytes / 2;
#else
    return false;
#endif // PLATFORM(WKC)
#endif
}

double ExecutableAllocator::memoryPressureMultiplier(size_t addedMemoryUsage)
{
    double result;
#ifdef EXECUTABLE_MEMORY_LIMIT
    size_t bytesAllocated = DemandExecutableAllocator::bytesAllocatedByAllAllocators() + addedMemoryUsage;
    if (bytesAllocated >= EXECUTABLE_MEMORY_LIMIT)
        bytesAllocated = EXECUTABLE_MEMORY_LIMIT;
    result = static_cast<double>(EXECUTABLE_MEMORY_LIMIT) /
        (EXECUTABLE_MEMORY_LIMIT - bytesAllocated);
#else
    UNUSED_PARAM(addedMemoryUsage);
    result = 1.0;
#endif
    if (result < 1.0)
        result = 1.0;
    return result;

}

RefPtr<ExecutableMemoryHandle> ExecutableAllocator::allocate(VM&, size_t sizeInBytes, void* ownerUID, JITCompilationEffort effort)
{
    RefPtr<ExecutableMemoryHandle> result = allocator()->allocate(sizeInBytes, ownerUID);
    RELEASE_ASSERT(result || effort != JITCompilationMustSucceed);
    return result;
}

size_t ExecutableAllocator::committedByteCount()
{
    return DemandExecutableAllocator::bytesCommittedByAllocactors();
}

#if ENABLE(META_ALLOCATOR_PROFILE)
void ExecutableAllocator::dumpProfile()
{
    DemandExecutableAllocator::dumpProfileFromAllAllocators();
}
#endif

#endif // ENABLE(EXECUTABLE_ALLOCATOR_DEMAND)

#if ENABLE(ASSEMBLER_WX_EXCLUSIVE)

#if PLATFORM(WKC)
void ExecutableAllocator::reprotectRegion(void* start, size_t size, ProtectionSetting setting)
{
    wkcMemorySetExecutablePeer(start, size, setting == Executable);
}

#elif OS(WINDOWS) || OS(WINDOWS_WKC)
#error "ASSEMBLER_WX_EXCLUSIVE not yet suported on this platform."
#else

void ExecutableAllocator::reprotectRegion(void* start, size_t size, ProtectionSetting setting)
{
    size_t pageSize = WTF::pageSize();

    // Calculate the start of the page containing this region,
    // and account for this extra memory within size.
    intptr_t startPtr = reinterpret_cast<intptr_t>(start);
    intptr_t pageStartPtr = startPtr & ~(pageSize - 1);
    void* pageStart = reinterpret_cast<void*>(pageStartPtr);
    size += (startPtr - pageStartPtr);

    // Round size up
    size += (pageSize - 1);
    size &= ~(pageSize - 1);

    mprotect(pageStart, size, (setting == Writable) ? PROTECTION_FLAGS_RW : PROTECTION_FLAGS_RX);
}
#endif

#endif

}

#endif // HAVE(ASSEMBLER)
