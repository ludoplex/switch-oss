/*
 * Copyright (C) 2005, 2008, 2009, 2014 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef JSLock_h
#define JSLock_h

#if !PLATFORM(WKC)
#include <mutex>
#include <thread>
#else
#include <wkcmutex.h>
#include <wkcthread.h>
#endif
#include <wtf/Assertions.h>
#include <wtf/Noncopyable.h>
#include <wtf/RefPtr.h>
#include <wtf/ThreadSafeRefCounted.h>
#include <wtf/WTFThreadData.h>

namespace JSC {

// To make it safe to use JavaScript on multiple threads, it is
// important to lock before doing anything that allocates a
// JavaScript data structure or that interacts with shared state
// such as the protect count hash table. The simplest way to lock
// is to create a local JSLockHolder object in the scope where the lock 
// must be held and pass it the context that requires protection. 
// The lock is recursive so nesting is ok. The JSLock 
// object also acts as a convenience short-hand for running important
// initialization routines.

// To avoid deadlock, sometimes it is necessary to temporarily
// release the lock. Since it is recursive you actually have to
// release all locks held by your thread. This is safe to do if
// you are executing code that doesn't require the lock, and you
// reacquire the right number of locks at the end. You can do this
// by constructing a locally scoped JSLock::DropAllLocks object. The 
// DropAllLocks object takes care to release the JSLock only if your
// thread acquired it to begin with.

class ExecState;
class VM;

// This class is used to protect the initialization of the legacy single 
// shared VM.
class GlobalJSLock {
    WTF_MAKE_NONCOPYABLE(GlobalJSLock);
public:
    JS_EXPORT_PRIVATE GlobalJSLock();
    JS_EXPORT_PRIVATE ~GlobalJSLock();

    static void initialize();
private:
#if !PLATFORM(WKC)
    static std::mutex* s_sharedInstanceMutex;
#else
    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY(std::mutex*, s_sharedInstanceMutex);
#endif
};

class JSLockHolder {
public:
    JS_EXPORT_PRIVATE JSLockHolder(VM*);
    JS_EXPORT_PRIVATE JSLockHolder(VM&);
    JS_EXPORT_PRIVATE JSLockHolder(ExecState*);

    JS_EXPORT_PRIVATE ~JSLockHolder();
private:
    void init();

    RefPtr<VM> m_vm;
};

class JSLock : public ThreadSafeRefCounted<JSLock> {
    WTF_MAKE_NONCOPYABLE(JSLock);
public:
    JSLock(VM*);
    JS_EXPORT_PRIVATE ~JSLock();

    JS_EXPORT_PRIVATE void lock();
    JS_EXPORT_PRIVATE void unlock();

    static void lock(ExecState*);
    static void unlock(ExecState*);
    static void lock(VM&);
    static void unlock(VM&);

    VM* vm() { return m_vm; }

    bool hasExclusiveThread() const { return m_hasExclusiveThread; }
    std::thread::id exclusiveThread() const
    {
        ASSERT(m_hasExclusiveThread);
        return m_ownerThreadID;
    }
    JS_EXPORT_PRIVATE void setExclusiveThread(std::thread::id);
    JS_EXPORT_PRIVATE bool currentThreadIsHoldingLock();

    void willDestroyVM(VM*);

    class DropAllLocks {
        WTF_MAKE_NONCOPYABLE(DropAllLocks);
    public:
        JS_EXPORT_PRIVATE DropAllLocks(ExecState*);
        JS_EXPORT_PRIVATE DropAllLocks(VM*);
        JS_EXPORT_PRIVATE DropAllLocks(VM&);
        JS_EXPORT_PRIVATE ~DropAllLocks();

        void setDropDepth(unsigned depth) { m_dropDepth = depth; }
        unsigned dropDepth() const { return m_dropDepth; }

    private:
        intptr_t m_droppedLockCount;
        RefPtr<VM> m_vm;
        unsigned m_dropDepth;
    };

private:
    void lock(intptr_t lockCount);
    void unlock(intptr_t unlockCount);

    void didAcquireLock();
    void willReleaseLock();

    unsigned dropAllLocks(DropAllLocks*);
    void grabAllLocks(DropAllLocks*, unsigned lockCount);

    std::mutex m_lock;
    std::thread::id m_ownerThreadID;
    intptr_t m_lockCount;
    unsigned m_lockDropDepth;
    bool m_hasExclusiveThread;
    VM* m_vm;
    AtomicStringTable* m_entryAtomicStringTable; 
};

} // namespace

#endif // JSLock_h
