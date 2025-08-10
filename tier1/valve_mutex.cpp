#include "valve_mutex.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <new>

namespace Internal
{
#ifdef _WIN32
    // Compile-time asserts to ensure these are the actual sizes we have specified.
    // See notes in the header file for why we use them.
    static_assert(sizeof(SRWLOCK) == TT_SIZEOF_SRWLOCK);
    static_assert(sizeof(CRITICAL_SECTION) == TT_SIZEOF_CRITICALSECTION);

    // Regular Mutex

    CThreadMutexWindows::CThreadMutexWindows()
    {
        SRWLOCK* srw_lock = new (m_srwLock) SRWLOCK; // Placement new constructs in place
        InitializeSRWLock(srw_lock); // NOTE: There is no corresponding de-initialize or delete function.
    }

    void CThreadMutexWindows::Lock()
    {
        AcquireSRWLockExclusive(reinterpret_cast<SRWLOCK*>(m_srwLock));
    }

    void CThreadMutexWindows::Unlock()
    {
        ReleaseSRWLockExclusive(reinterpret_cast<SRWLOCK*>(m_srwLock));
    }

    bool CThreadMutexWindows::TryLock()
    {
        return TryAcquireSRWLockExclusive(reinterpret_cast<SRWLOCK*>(m_srwLock));
    }

    // Recursive Mutex

    CThreadMutexRecursiveWindows::CThreadMutexRecursiveWindows()
    {
        CRITICAL_SECTION* critical_section = new (m_criticalSection) CRITICAL_SECTION; // Placement new constructs in place
        InitializeCriticalSection(critical_section);
    }

    CThreadMutexRecursiveWindows::~CThreadMutexRecursiveWindows()
    {
        DeleteCriticalSection(reinterpret_cast<CRITICAL_SECTION*>(m_criticalSection));
    }

    void CThreadMutexRecursiveWindows::Lock()
    {
        EnterCriticalSection(reinterpret_cast<CRITICAL_SECTION*>(m_criticalSection));
    }

    void CThreadMutexRecursiveWindows::Unlock()
    {
        LeaveCriticalSection(reinterpret_cast<CRITICAL_SECTION*>(m_criticalSection));
    }

    bool CThreadMutexRecursiveWindows::TryLock()
    {
        return TryEnterCriticalSection(reinterpret_cast<CRITICAL_SECTION*>(m_criticalSection));
    }

    CThreadRWLockWindows::CThreadRWLockWindows()
    {
        SRWLOCK* srw_lock = new (m_srwLock) SRWLOCK; // Placement new constructs in place
        InitializeSRWLock(srw_lock); // NOTE: There is no corresponding de-initialize or delete function.
    }

    // Shared Mutex (RW Lock)
    void CThreadRWLockWindows::LockForRead()
    {
        AcquireSRWLockShared(reinterpret_cast<SRWLOCK*>(m_srwLock));
    }

    void CThreadRWLockWindows::UnlockRead()
    {
        ReleaseSRWLockShared(reinterpret_cast<SRWLOCK*>(m_srwLock));
    }

    void CThreadRWLockWindows::LockForWrite()
    {
        AcquireSRWLockExclusive(reinterpret_cast<SRWLOCK*>(m_srwLock));
    }

    void CThreadRWLockWindows::UnlockWrite()
    {
        ReleaseSRWLockExclusive(reinterpret_cast<SRWLOCK*>(m_srwLock));
    }

#else
    // Regular Mutex
    CThreadMutexPosix::CThreadMutexPosix()
    {
        pthread_mutex_init(&m_mutex, nullptr);
    }

    CThreadMutexPosix::~CThreadMutexPosix()
    {
        pthread_mutex_destroy(&m_mutex);
    }

    void CThreadMutexPosix::Lock()
    {
        pthread_mutex_lock(&m_mutex);
    }

    void CThreadMutexPosix::Unlock()
    {
        pthread_mutex_unlock(&m_mutex);
    }

    bool CThreadMutexPosix::TryLock()
    {
        return pthread_mutex_trylock(&m_mutex) == 0;
    }

    // Recursive Mutex

    CThreadMutexRecursivePosix::CThreadMutexRecursivePosix()
    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

        pthread_mutex_init(&m_mutex, &attr);
    }

    CThreadMutexRecursivePosix::~CThreadMutexRecursivePosix()
    {
        pthread_mutex_destroy(&m_mutex);
    }

    void CThreadMutexRecursivePosix::Lock()
    {
        pthread_mutex_lock(&m_mutex);
    }

    void CThreadMutexRecursivePosix::Unlock()
    {
        pthread_mutex_unlock(&m_mutex);
    }

    bool CThreadMutexRecursivePosix::TryLock()
    {
        return pthread_mutex_trylock(&m_mutex) == 0;
    }

    // Shared Mutex (RW Lock)
    CThreadRWLockPosix::CThreadRWLockPosix()
    {
        pthread_rwlock_init(&m_rwlock, nullptr);
    }

    CThreadRWLockPosix::~CThreadRWLockPosix()
    {
        pthread_rwlock_destroy(&m_rwlock);
    }

    void CThreadRWLockPosix::LockForRead()
    {
        pthread_rwlock_rdlock(&m_rwlock);
    }

    void CThreadRWLockPosix::UnlockRead()
    {
        pthread_rwlock_unlock(&m_rwlock);
    }

    void CThreadRWLockPosix::LockForWrite()
    {
        pthread_rwlock_wrlock(&m_rwlock);
    }

    void CThreadRWLockPosix::UnlockWrite()
    {
        pthread_rwlock_unlock(&m_rwlock);
    }

#endif // _WIN32
}
