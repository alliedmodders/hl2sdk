#ifndef VALVE_MUTEX_H
#define VALVE_MUTEX_H

//-----------------------------------------------------------------------------
//
// Platform independent for critical sections management
//
//-----------------------------------------------------------------------------

#ifdef POSIX
#include <pthread.h>
#else
#include <cstddef>
#endif // POSIX

// By manually specifying the size and constructing these ourselves,
// we can avoid the inclusion of the windows header here. Even if we
// fudged some definitions and only included synchapi.h, we wouldn't
// be able to include that without causing issues with the macro CreateEvent
// and the like.
#ifdef _WIN64
	#define TT_SIZEOF_CRITICALSECTION 40
#else
	#define TT_SIZEOF_CRITICALSECTION 24
#endif // _WIN64

#ifdef _WIN32
    #define TT_SIZEOF_SRWLOCK sizeof(void*)
#endif // _WIN32

namespace Internal
{
#ifdef _WIN32
    class CThreadMutexWindows
    {
        std::byte m_srwLock[TT_SIZEOF_SRWLOCK];
    public:
        CThreadMutexWindows();

        CThreadMutexWindows(const CThreadMutexWindows&) = delete;
        CThreadMutexWindows& operator=(const CThreadMutexWindows&) = delete;

        CThreadMutexWindows(CThreadMutexWindows&&) noexcept = delete;
        CThreadMutexWindows& operator=(CThreadMutexWindows&&) noexcept = delete;

        void Lock();
        void Unlock();
        bool TryLock();
    };

    class CThreadMutexRecursiveWindows
    {
        std::byte m_criticalSection[TT_SIZEOF_CRITICALSECTION];
    public:
        CThreadMutexRecursiveWindows();
        ~CThreadMutexRecursiveWindows();

        CThreadMutexRecursiveWindows(const CThreadMutexRecursiveWindows&) = delete;
        CThreadMutexRecursiveWindows& operator=(const CThreadMutexRecursiveWindows&) = delete;

        CThreadMutexRecursiveWindows(CThreadMutexRecursiveWindows&&) noexcept = delete;
        CThreadMutexRecursiveWindows& operator=(CThreadMutexRecursiveWindows&&) noexcept = delete;

        void Lock();
        void Unlock();
        bool TryLock();
    };

    class CThreadRWLockWindows
    {
        std::byte m_srwLock[TT_SIZEOF_SRWLOCK];
    public:
        CThreadRWLockWindows();

        CThreadRWLockWindows(const CThreadRWLockWindows&) = delete;
        CThreadRWLockWindows& operator=(const CThreadRWLockWindows&) = delete;

        CThreadRWLockWindows(CThreadRWLockWindows&&) noexcept = delete;
        CThreadRWLockWindows& operator=(CThreadRWLockWindows&&) noexcept = delete;

        void LockForRead();
	    void UnlockRead();

	    void LockForWrite();
	    void UnlockWrite();
    };
#else
    class CThreadMutexPosix
    {
        pthread_mutex_t m_mutex;
    public:
        CThreadMutexPosix();
        ~CThreadMutexPosix();

        CThreadMutexPosix(const CThreadMutexPosix&) = delete;
        CThreadMutexPosix& operator=(const CThreadMutexPosix&) = delete;

        CThreadMutexPosix(CThreadMutexPosix&&) noexcept = delete;
        CThreadMutexPosix& operator=(CThreadMutexPosix&&) noexcept = delete;

        void Lock();
        void Unlock();
        bool TryLock();
    };

    class CThreadMutexRecursivePosix
    {
        pthread_mutex_t m_mutex;
    public:
        CThreadMutexRecursivePosix();
        ~CThreadMutexRecursivePosix();

        CThreadMutexRecursivePosix(const CThreadMutexRecursivePosix&) = delete;
        CThreadMutexRecursivePosix& operator=(const CThreadMutexRecursivePosix&) = delete;

        CThreadMutexRecursivePosix(CThreadMutexRecursivePosix&&) noexcept = delete;
        CThreadMutexRecursivePosix& operator=(CThreadMutexRecursivePosix&&) noexcept = delete;

        void Lock();
        void Unlock();
        bool TryLock();
    };

    class CThreadRWLockPosix
    {
        pthread_rwlock_t m_rwlock;
    public:
        CThreadRWLockPosix();
        ~CThreadRWLockPosix();

        CThreadRWLockPosix(const CThreadRWLockPosix&) = delete;
        CThreadRWLockPosix& operator=(const CThreadRWLockPosix&) = delete;

        CThreadRWLockPosix(CThreadRWLockPosix&&) noexcept = delete;
        CThreadRWLockPosix& operator=(CThreadRWLockPosix&&) noexcept = delete;

        void LockForRead();
        void UnlockRead();

        void LockForWrite();
        void UnlockWrite();
    };
#endif

}

#ifdef _WIN32
    class CThreadMutex : public Internal::CThreadMutexWindows {};
    class CThreadMutexRecursive : public Internal::CThreadMutexRecursiveWindows {};
    class CThreadRWLock : public Internal::CThreadRWLockWindows {};
#else
    class CThreadMutex : public Internal::CThreadMutexPosix {};
    class CThreadMutexRecursive : public Internal::CThreadMutexRecursivePosix {};
    class CThreadRWLock : public Internal::CThreadRWLockPosix {};
#endif // _WIN32

#endif // VALVE_MUTEX_H