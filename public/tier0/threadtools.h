//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A collection of utility classes to simplify thread handling, and
//			as much as possible contain portability problems. Here avoiding 
//			including windows.h.
//
//=============================================================================

#ifndef THREADTOOLS_H
#define THREADTOOLS_H

#include <limits.h>

#include <limits>
#include <atomic>
#include <mutex>
#include <shared_mutex>

#include "tier0/platform.h"
#include "tier0/dbg.h"
#include "tier0/vcrmode.h"

#ifdef PLATFORM_WINDOWS_PC
#include <intrin.h>
#endif

#ifdef POSIX
#include <pthread.h>
#include <errno.h>
#define WAIT_OBJECT_0 0
#define WAIT_TIMEOUT 0x00000102
#define WAIT_FAILED -1
#define THREAD_PRIORITY_HIGHEST 2
#endif

#if defined( _WIN32 )
#pragma once
#pragma warning(push)
#pragma warning(disable:4251)
#endif

// #define THREAD_PROFILER 1

#ifndef _RETAIL
#define THREAD_MUTEX_TRACING_SUPPORTED
#if defined(_WIN32) && defined(_DEBUG)
#define THREAD_MUTEX_TRACING_ENABLED
#endif
#endif

#ifdef _WIN32
typedef void *HANDLE;
#endif

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

const unsigned TT_INFINITE = 0xffffffff;

#ifndef NO_THREAD_LOCAL

#ifndef THREAD_LOCAL
#ifdef _WIN32
#define THREAD_LOCAL __declspec(thread)
#elif POSIX
#define THREAD_LOCAL __thread
#endif
#endif

#endif // NO_THREAD_LOCAL

typedef unsigned long ThreadId_t;

//-----------------------------------------------------------------------------
//
// Simple thread creation. Differs from VCR mode/CreateThread/_beginthreadex
// in that it accepts a standard C function rather than compiler specific one.
//
//-----------------------------------------------------------------------------
FORWARD_DECLARE_HANDLE( ThreadHandle_t );
typedef unsigned (*ThreadFunc_t)( void *pParam );

PLATFORM_OVERLOAD ThreadHandle_t CreateSimpleThread( ThreadFunc_t, void *pParam, ThreadId_t *pID, unsigned stackSize = 0 );
PLATFORM_INTERFACE ThreadHandle_t CreateSimpleThread( ThreadFunc_t, void *pParam, unsigned stackSize = 0 );
PLATFORM_INTERFACE bool ReleaseThreadHandle( ThreadHandle_t );


//-----------------------------------------------------------------------------

PLATFORM_INTERFACE void ThreadSleep(unsigned duration = 0);
PLATFORM_INTERFACE uint ThreadGetCurrentId();
PLATFORM_INTERFACE ThreadHandle_t ThreadGetCurrentHandle();
PLATFORM_INTERFACE int ThreadGetPriority( ThreadHandle_t hThread = NULL );
PLATFORM_INTERFACE bool ThreadSetPriority( ThreadHandle_t hThread, int priority );
inline		 bool ThreadSetPriority( int priority ) { return ThreadSetPriority( NULL, priority ); }
PLATFORM_INTERFACE bool ThreadInMainThread();
PLATFORM_INTERFACE void DeclareCurrentThreadIsMainThread();

// NOTE: ThreadedLoadLibraryFunc_t needs to return the sleep time in milliseconds or TT_INFINITE
typedef int (*ThreadedLoadLibraryFunc_t)(); 
PLATFORM_INTERFACE void SetThreadedLoadLibraryFunc( ThreadedLoadLibraryFunc_t func );
PLATFORM_INTERFACE ThreadedLoadLibraryFunc_t GetThreadedLoadLibraryFunc();

#if defined( _WIN32 ) && !defined( _WIN64 ) && !defined( _X360 )
extern "C" unsigned long __declspec(dllimport) __stdcall GetCurrentThreadId();
#define ThreadGetCurrentId GetCurrentThreadId
#endif

inline void ThreadPause()
{
#if defined( PLATFORM_WINDOWS_PC )
	// Intrinsic for __asm pause; from <intrin.h>
	_mm_pause();
#elif POSIX
	__asm __volatile( "pause" );
#elif defined( _X360 )
#else
#error "implement me"
#endif
}

PLATFORM_INTERFACE bool ThreadJoin( ThreadHandle_t, unsigned timeout = TT_INFINITE );
// If you're not calling ThreadJoin, you need to call ThreadDetach so pthreads on Linux knows it can
//	free the memory for this thread. Otherwise you wind up leaking threads until you run out and
//	CreateSimpleThread() will fail.
PLATFORM_INTERFACE void ThreadDetach( ThreadHandle_t );

PLATFORM_INTERFACE void ThreadSetDebugName( ThreadId_t id, const char *pszName );
inline		 void ThreadSetDebugName( const char *pszName ) { ThreadSetDebugName( (ThreadId_t)-1, pszName ); }

PLATFORM_INTERFACE void ThreadSetAffinity( ThreadHandle_t hThread, int nAffinityMask );

//-----------------------------------------------------------------------------

enum ThreadWaitResult_t
{
	TW_FAILED = 0xffffffff, // WAIT_FAILED
	TW_TIMEOUT = 0x00000102, // WAIT_TIMEOUT
};

#ifdef _WIN32
PLATFORM_INTERFACE int ThreadWaitForObjects( int nEvents, const HANDLE *pHandles, bool bWaitAll = true, unsigned timeout = TT_INFINITE );
inline int ThreadWaitForObject( HANDLE handle, bool bWaitAll = true, unsigned timeout = TT_INFINITE ) { return ThreadWaitForObjects( 1, &handle, bWaitAll, timeout ); }
#endif

//-----------------------------------------------------------------------------
//
// Interlock methods. These perform very fast atomic thread
// safe operations. These are especially relevant in a multi-core setting.
//
//-----------------------------------------------------------------------------

#ifdef _WIN32
#define NOINLINE
#elif POSIX
#define NOINLINE __attribute__ ((noinline))
#endif

// ThreadMemoryBarrier is a fence/barrier sufficient for most uses. It prevents reads
// from moving past reads, and writes moving past writes. It is sufficient for
// read-acquire and write-release barriers. It is not a full barrier and it does
// not prevent reads from moving past writes -- that would require a full __sync()
// on PPC and is significantly more expensive.
#if defined( _X360 ) || defined( _PS3 )
	#define ThreadMemoryBarrier() __lwsync()

#elif defined(_MSC_VER)
	// Prevent compiler reordering across this barrier. This is
	// sufficient for most purposes on x86/x64.

	#if _MSC_VER < 1500
		// !KLUDGE! For VC 2005
		// http://connect.microsoft.com/VisualStudio/feedback/details/100051
		#pragma intrinsic(_ReadWriteBarrier)
	#endif
	#define ThreadMemoryBarrier() _ReadWriteBarrier()
#elif defined(GNUC)
	// Prevent compiler reordering across this barrier. This is
	// sufficient for most purposes on x86/x64.
	// http://preshing.com/20120625/memory-ordering-at-compile-time
	#define ThreadMemoryBarrier() asm volatile("" ::: "memory")
#else
	#error Every platform needs to define ThreadMemoryBarrier to at least prevent compiler reordering
#endif

#if defined(_WIN32) && !defined(_X360)
	#if ( _MSC_VER >= 1310 )
		#define USE_INTRINSIC_INTERLOCKED
	#endif
#endif

#ifdef USE_INTRINSIC_INTERLOCKED
extern "C"
{
	long __cdecl _InterlockedIncrement(volatile long*);
	long __cdecl _InterlockedDecrement(volatile long*);
	long __cdecl _InterlockedExchange(volatile long*, long);
	long __cdecl _InterlockedExchangeAdd(volatile long*, long);
	long __cdecl _InterlockedCompareExchange(volatile long*, long, long);
}

#pragma intrinsic( _InterlockedCompareExchange )
#pragma intrinsic( _InterlockedDecrement )
#pragma intrinsic( _InterlockedExchange )
#pragma intrinsic( _InterlockedExchangeAdd ) 
#pragma intrinsic( _InterlockedIncrement )

inline long ThreadInterlockedIncrement( long volatile *p )										{ Assert( (size_t)p % 4 == 0 ); return _InterlockedIncrement( p ); }
inline long ThreadInterlockedDecrement( long volatile *p )										{ Assert( (size_t)p % 4 == 0 ); return _InterlockedDecrement( p ); }
inline long ThreadInterlockedExchange( long volatile *p, long value )							{ Assert( (size_t)p % 4 == 0 ); return _InterlockedExchange( p, value ); }
inline long ThreadInterlockedExchangeAdd( long volatile *p, long value )						{ Assert( (size_t)p % 4 == 0 ); return _InterlockedExchangeAdd( p, value ); }
inline long ThreadInterlockedCompareExchange( long volatile *p, long value, long comperand )	{ Assert( (size_t)p % 4 == 0 ); return _InterlockedCompareExchange( p, value, comperand ); }
inline bool ThreadInterlockedAssignIf( long volatile *p, long value, long comperand )			{ Assert( (size_t)p % 4 == 0 ); return ( _InterlockedCompareExchange( p, value, comperand ) == comperand ); }
#else
PLATFORM_INTERFACE long ThreadInterlockedIncrement( long volatile * );
PLATFORM_INTERFACE long ThreadInterlockedDecrement( long volatile * );
PLATFORM_INTERFACE long ThreadInterlockedExchange( long volatile *, long value );
PLATFORM_INTERFACE long ThreadInterlockedExchangeAdd( long volatile *, long value );
PLATFORM_INTERFACE long ThreadInterlockedCompareExchange( long volatile *, long value, long comperand );
PLATFORM_INTERFACE bool ThreadInterlockedAssignIf( long volatile *, long value, long comperand );
#endif

inline unsigned ThreadInterlockedExchangeSubtract( long volatile *p, long value )	{ return ThreadInterlockedExchangeAdd( (long volatile *)p, -value ); }

#if defined( USE_INTRINSIC_INTERLOCKED ) && !defined( _WIN64 )
#define TIPTR()
inline void *ThreadInterlockedExchangePointer( void * volatile *p, void *value )							{ return (void *)_InterlockedExchange( reinterpret_cast<long volatile *>(p), reinterpret_cast<long>(value) ); }
inline void *ThreadInterlockedCompareExchangePointer( void * volatile *p, void *value, void *comperand )	{ return (void *)_InterlockedCompareExchange( reinterpret_cast<long volatile *>(p), reinterpret_cast<long>(value), reinterpret_cast<long>(comperand) ); }
inline bool ThreadInterlockedAssignPointerIf( void * volatile *p, void *value, void *comperand )			{ return ( _InterlockedCompareExchange( reinterpret_cast<long volatile *>(p), reinterpret_cast<long>(value), reinterpret_cast<long>(comperand) ) == reinterpret_cast<long>(comperand) ); }
#else
PLATFORM_INTERFACE void *ThreadInterlockedExchangePointer( void * volatile *, void *value ) NOINLINE;
PLATFORM_INTERFACE void *ThreadInterlockedCompareExchangePointer( void * volatile *, void *value, void *comperand ) NOINLINE;
PLATFORM_INTERFACE bool ThreadInterlockedAssignPointerIf( void * volatile *, void *value, void *comperand ) NOINLINE;
#endif

inline void const *ThreadInterlockedExchangePointerToConst( void const * volatile *p, void const *value )							{ return ThreadInterlockedExchangePointer( const_cast < void * volatile * > ( p ), const_cast < void * > ( value ) );  }
inline void const *ThreadInterlockedCompareExchangePointerToConst( void const * volatile *p, void const *value, void const *comperand )	{ return ThreadInterlockedCompareExchangePointer( const_cast < void * volatile * > ( p ), const_cast < void * > ( value ), const_cast < void * > ( comperand ) ); }
inline bool ThreadInterlockedAssignPointerToConstIf( void const * volatile *p, void const *value, void const *comperand )			{ return ThreadInterlockedAssignPointerIf( const_cast < void * volatile * > ( p ), const_cast < void * > ( value ), const_cast < void * > ( comperand ) ); }

#if defined( PLATFORM_64BITS )
#if defined (_WIN32) 
typedef __m128i int128;
inline int128 int128_zero()	{ return _mm_setzero_si128(); }
#else
typedef __int128_t int128;
#define int128_zero() 0
#endif

PLATFORM_INTERFACE bool ThreadInterlockedAssignIf128( volatile int128 *pDest, const int128 &value, const int128 &comperand ) NOINLINE;

#endif

PLATFORM_INTERFACE int64 ThreadInterlockedIncrement64( int64 volatile * ) NOINLINE;
PLATFORM_INTERFACE int64 ThreadInterlockedDecrement64( int64 volatile * ) NOINLINE;
PLATFORM_INTERFACE int64 ThreadInterlockedCompareExchange64( int64 volatile *, int64 value, int64 comperand ) NOINLINE;
PLATFORM_INTERFACE int64 ThreadInterlockedExchange64( int64 volatile *, int64 value ) NOINLINE;
PLATFORM_INTERFACE int64 ThreadInterlockedExchangeAdd64( int64 volatile *, int64 value ) NOINLINE;
PLATFORM_INTERFACE bool ThreadInterlockedAssignIf64(volatile int64 *pDest, int64 value, int64 comperand ) NOINLINE;

inline unsigned ThreadInterlockedExchangeSubtract( unsigned volatile *p, unsigned value )	{ return ThreadInterlockedExchangeAdd( (long volatile *)p, value ); }
inline unsigned ThreadInterlockedIncrement( unsigned volatile *p )	{ return ThreadInterlockedIncrement( (long volatile *)p ); }
inline unsigned ThreadInterlockedDecrement( unsigned volatile *p )	{ return ThreadInterlockedDecrement( (long volatile *)p ); }
inline unsigned ThreadInterlockedExchange( unsigned volatile *p, unsigned value )	{ return ThreadInterlockedExchange( (long volatile *)p, value ); }
inline unsigned ThreadInterlockedExchangeAdd( unsigned volatile *p, unsigned value )	{ return ThreadInterlockedExchangeAdd( (long volatile *)p, value ); }
inline unsigned ThreadInterlockedCompareExchange( unsigned volatile *p, unsigned value, unsigned comperand )	{ return ThreadInterlockedCompareExchange( (long volatile *)p, value, comperand ); }
inline bool ThreadInterlockedAssignIf( unsigned volatile *p, unsigned value, unsigned comperand )	{ return ThreadInterlockedAssignIf( (long volatile *)p, value, comperand ); }

inline int ThreadInterlockedExchangeSubtract( int volatile *p, int value )	{ return ThreadInterlockedExchangeAdd( (long volatile *)p, value ); }
inline int ThreadInterlockedIncrement( int volatile *p )	{ return ThreadInterlockedIncrement( (long volatile *)p ); }
inline int ThreadInterlockedDecrement( int volatile *p )	{ return ThreadInterlockedDecrement( (long volatile *)p ); }
inline int ThreadInterlockedExchange( int volatile *p, int value )	{ return ThreadInterlockedExchange( (long volatile *)p, value ); }
inline int ThreadInterlockedExchangeAdd( int volatile *p, int value )	{ return ThreadInterlockedExchangeAdd( (long volatile *)p, value ); }
inline int ThreadInterlockedCompareExchange( int volatile *p, int value, int comperand )	{ return ThreadInterlockedCompareExchange( (long volatile *)p, value, comperand ); }
inline bool ThreadInterlockedAssignIf( int volatile *p, int value, int comperand )	{ return ThreadInterlockedAssignIf( (long volatile *)p, value, comperand ); }

//-----------------------------------------------------------------------------
// Access to VTune thread profiling
//-----------------------------------------------------------------------------
#if defined(_WIN32) && defined(THREAD_PROFILER)
PLATFORM_INTERFACE void ThreadNotifySyncPrepare(void *p);
PLATFORM_INTERFACE void ThreadNotifySyncCancel(void *p);
PLATFORM_INTERFACE void ThreadNotifySyncAcquired(void *p);
PLATFORM_INTERFACE void ThreadNotifySyncReleasing(void *p);
#else
#define ThreadNotifySyncPrepare(p)		((void)0)
#define ThreadNotifySyncCancel(p)		((void)0)
#define ThreadNotifySyncAcquired(p)		((void)0)
#define ThreadNotifySyncReleasing(p)	((void)0)
#endif

//-----------------------------------------------------------------------------
// Encapsulation of a thread local datum (needed because THREAD_LOCAL doesn't
// work in a DLL loaded with LoadLibrary()
//-----------------------------------------------------------------------------

#ifndef NO_THREAD_LOCAL

#if defined(_LINUX) && !defined(OSX)
// linux totally supports compiler thread locals, even across dll's.
#define PLAT_COMPILER_SUPPORTED_THREADLOCALS 1
#define CTHREADLOCALINTEGER( typ ) __thread int
#define CTHREADLOCALINT __thread int
#define CTHREADLOCALPTR( typ ) __thread typ *
#define CTHREADLOCAL( typ ) __thread typ
#define GETLOCAL( x ) ( x )
#endif // _LINUX && !OSX

#if defined(WIN32) || defined(OSX)
#ifndef __AFXTLS_H__ // not compatible with some Windows headers
#define CTHREADLOCALINT CThreadLocalInt<int>
#define CTHREADLOCALINTEGER( typ ) CThreadLocalInt<typ>
#define CTHREADLOCALPTR( typ ) CThreadLocalPtr<typ>
#define CTHREADLOCAL( typ ) CThreadLocal<typ>
#define GETLOCAL( x ) ( x.Get() )
#endif
#endif // WIN32 || OSX

#endif // NO_THREAD_LOCALS

#ifndef __AFXTLS_H__ // not compatible with some Windows headers
#ifndef NO_THREAD_LOCAL

class PLATFORM_CLASS CThreadLocalBase
	{
public:
		CThreadLocalBase();
		~CThreadLocalBase();

		void * Get() const;
		void   Set(void *);

private:
#ifdef _WIN32
	uint32 m_index;
#elif POSIX
		pthread_key_t m_index;
#endif
	};

	//---------------------------------------------------------

#ifndef __AFXTLS_H__

	template <class T>
	class CThreadLocal : public CThreadLocalBase
	{
	public:
		CThreadLocal()
		{
			COMPILE_TIME_ASSERT( sizeof(T) == sizeof(void *) );
		}

		T Get() const
		{
			return reinterpret_cast<T>( CThreadLocalBase::Get() );
		}

		void Set(T val)
		{
			CThreadLocalBase::Set( reinterpret_cast<void *>(val) );
		}
	};

#endif

	//---------------------------------------------------------

template <class T = intp>
	class CThreadLocalInt : public CThreadLocal<T>
	{
	public:
	CThreadLocalInt()
	{
		COMPILE_TIME_ASSERT( sizeof(T) >= sizeof(int) );
	}

	operator int() const { return (int)this->Get(); }
	int	operator=( int i ) { this->Set( (intp)i ); return i; }

	int operator++()					{ T i = this->Get(); this->Set( ++i ); return (int)i; }
	int operator++(int)				{ T i = this->Get(); this->Set( i + 1 ); return (int)i; }

	int operator--()					{ T i = this->Get(); this->Set( --i ); return (int)i; }
	int operator--(int)				{ T i = this->Get(); this->Set( i - 1 ); return (int)i; }
	};


	//---------------------------------------------------------

	template <class T>
	class CThreadLocalPtr : private CThreadLocalBase
	{
	public:
		CThreadLocalPtr() {}

	operator const void *() const          					{ return (T *)Get(); }
		operator void *()                      					{ return (T *)Get(); }

	operator const T *() const							    { return (T *)Get(); }
	operator const T *()          							{ return (T *)Get(); }
		operator T *()											{ return (T *)Get(); }

	int			operator=( int i )							{ AssertMsg( i == 0, "Only NULL allowed on integer assign" ); Set( NULL ); return 0; }
		T *			operator=( T *p )							{ Set( p ); return p; }

		bool        operator !() const							{ return (!Get()); }
		bool        operator!=( int i ) const					{ AssertMsg( i == 0, "Only NULL allowed on integer compare" ); return (Get() != NULL); }
		bool        operator==( int i ) const					{ AssertMsg( i == 0, "Only NULL allowed on integer compare" ); return (Get() == NULL); }
		bool		operator==( const void *p ) const			{ return (Get() == p); }
		bool		operator!=( const void *p ) const			{ return (Get() != p); }
	bool		operator==( const T *p ) const				{ return operator==((void*)p); }
	bool		operator!=( const T *p ) const				{ return operator!=((void*)p); }

		T *  		operator->()								{ return (T *)Get(); }
		T &  		operator *()								{ return *((T *)Get()); }

	const T *   operator->() const							{ return (T *)Get(); }
	const T &   operator *() const							{ return *((T *)Get()); }

	const T &	operator[]( int i ) const					{ return *((T *)Get() + i); }
		T &			operator[]( int i )							{ return *((T *)Get() + i); }

	private:
		// Disallowed operations
		CThreadLocalPtr( T *pFrom );
		CThreadLocalPtr( const CThreadLocalPtr<T> &from );
		T **operator &();
		T * const *operator &() const;
		void operator=( const CThreadLocalPtr<T> &from );
		bool operator==( const CThreadLocalPtr<T> &p ) const;
		bool operator!=( const CThreadLocalPtr<T> &p ) const;
	};

#endif // NO_THREAD_LOCAL
#endif // !__AFXTLS_H__

//-----------------------------------------------------------------------------
//
// A super-fast thread-safe integer A simple class encapsulating the notion of an 
// atomic integer used across threads that uses the built in and faster 
// "interlocked" functionality rather than a full-blown mutex. Useful for simple 
// things like reference counts, etc.
//
//-----------------------------------------------------------------------------

template <typename T>
class CInterlockedIntT
{
public:
	CInterlockedIntT() : m_value( 0 ) 				{ COMPILE_TIME_ASSERT( sizeof(T) == sizeof(long) ); }
	CInterlockedIntT( T value ) : m_value( value ) 	{}

	T GetRaw() const				{ return m_value; }

	operator T() const				{ return m_value; }

	bool operator!() const			{ return ( m_value == 0 ); }
	bool operator==( T rhs ) const	{ return ( m_value == rhs ); }
	bool operator!=( T rhs ) const	{ return ( m_value != rhs ); }

	T operator++()					{ return (T)ThreadInterlockedIncrement( (long *)&m_value ); }
	T operator++(int)				{ return operator++() - 1; }

	T operator--()					{ return (T)ThreadInterlockedDecrement( (long *)&m_value ); }
	T operator--(int)				{ return operator--() + 1; }

	bool AssignIf( T conditionValue, T newValue )	{ return ThreadInterlockedAssignIf( (long *)&m_value, (long)newValue, (long)conditionValue ); }

	T operator=( T newValue )		{ ThreadInterlockedExchange((long *)&m_value, newValue); return m_value; }

	void operator+=( T add )		{ ThreadInterlockedExchangeAdd( (long *)&m_value, (long)add ); }
	void operator-=( T subtract )	{ operator+=( -subtract ); }
	void operator*=( T multiplier )	{ 
		T original, result; 
		do 
		{ 
			original = m_value; 
			result = original * multiplier; 
		} while ( !AssignIf( original, result ) );
	}
	void operator/=( T divisor )	{ 
		T original, result; 
		do 
		{ 
			original = m_value; 
			result = original / divisor;
		} while ( !AssignIf( original, result ) );
	}

	T operator+( T rhs ) const		{ return m_value + rhs; }
	T operator-( T rhs ) const		{ return m_value - rhs; }

private:
	volatile T m_value;
};

typedef CInterlockedIntT<int> CInterlockedInt;
typedef CInterlockedIntT<unsigned> CInterlockedUInt;

//-----------------------------------------------------------------------------

template <typename T>
class CInterlockedPtr
{
public:
	CInterlockedPtr() : m_value( 0 ) 				{}
	CInterlockedPtr( T *value ) : m_value( value ) 	{}

	operator T *() const			{ return m_value; }

	bool operator!() const			{ return ( m_value == 0 ); }
	bool operator==( T *rhs ) const	{ return ( m_value == rhs ); }
	bool operator!=( T *rhs ) const	{ return ( m_value != rhs ); }

#if defined( PLATFORM_64BITS )
	T *operator++()					{ return ((T *)ThreadInterlockedExchangeAdd64( (int64 *)&m_value, sizeof(T) )) + 1; }
	T *operator++(int)				{ return (T *)ThreadInterlockedExchangeAdd64( (int64 *)&m_value, sizeof(T) ); }

	T *operator--()					{ return ((T *)ThreadInterlockedExchangeAdd64( (int64 *)&m_value, -sizeof(T) )) - 1; }
	T *operator--(int)				{ return (T *)ThreadInterlockedExchangeAdd64( (int64 *)&m_value, -sizeof(T) ); }

	bool AssignIf( T *conditionValue, T *newValue )	{ return ThreadInterlockedAssignPointerToConstIf( (void const **) &m_value, (void const *) newValue, (void const *) conditionValue ); }

	T *operator=( T *newValue )		{ ThreadInterlockedExchangePointerToConst( (void const **) &m_value, (void const *) newValue ); return newValue; }

	void operator+=( int add )		{ ThreadInterlockedExchangeAdd64( (int64 *)&m_value, add * sizeof(T) ); }
#else
	T *operator++()					{ return ((T *)ThreadInterlockedExchangeAdd( (long *)&m_value, sizeof(T) )) + 1; }
	T *operator++(int)				{ return (T *)ThreadInterlockedExchangeAdd( (long *)&m_value, sizeof(T) ); }

	T *operator--()					{ return ((T *)ThreadInterlockedExchangeAdd( (long *)&m_value, -sizeof(T) )) - 1; }
	T *operator--(int)				{ return (T *)ThreadInterlockedExchangeAdd( (long *)&m_value, -sizeof(T) ); }

	bool AssignIf( T *conditionValue, T *newValue )	{ return ThreadInterlockedAssignPointerToConstIf( (void const **) &m_value, (void const *) newValue, (void const *) conditionValue ); }

	T *operator=( T *newValue )		{ ThreadInterlockedExchangePointerToConst( (void const **) &m_value, (void const *) newValue ); return newValue; }

	void operator+=( int add )		{ ThreadInterlockedExchangeAdd( (long *)&m_value, add * sizeof(T) ); }
#endif

	void operator-=( int subtract )	{ operator+=( -subtract ); }

	T *operator+( int rhs ) const		{ return m_value + rhs; }
	T *operator-( int rhs ) const		{ return m_value - rhs; }
	T *operator+( unsigned rhs ) const	{ return m_value + rhs; }
	T *operator-( unsigned rhs ) const	{ return m_value - rhs; }
	size_t operator-( T *p ) const		{ return m_value - p; }
	size_t operator-( const CInterlockedPtr<T> &p ) const	{ return m_value - p.m_value; }

private:
	T * volatile m_value;
};

//-----------------------------------------------------------------------------
// 
// Platform independent verification that multiple threads aren't getting into the same code at the same time. 
// Note: This is intended for use to identify problems, it doesn't provide any sort of thread safety.
// 
//-----------------------------------------------------------------------------
class ReentrancyVerifier
{
public:
	inline ReentrancyVerifier(CInterlockedInt* counter, int sleepTimeMS)
	: mCounter(counter)
	{
		Assert(mCounter != NULL);

		if (++(*mCounter) != 1) {
			DebuggerBreakIfDebugging_StagingOnly();
		}

		if (sleepTimeMS > 0)
		{
			ThreadSleep(sleepTimeMS);
		}
	}

	inline ~ReentrancyVerifier()
	{
		if (--(*mCounter) != 0) {
			DebuggerBreakIfDebugging_StagingOnly();
		}
	}

private:
	CInterlockedInt* mCounter;
};


//-----------------------------------------------------------------------------
//
// Platform independent for critical sections management
//
//-----------------------------------------------------------------------------

class CThreadMutex
{
	std::mutex mutex;

public:
	bool TryLock() { return mutex.try_lock(); }
	void Lock() { mutex.lock(); }
	void Unlock() { mutex.unlock(); }
};

class CThreadMutexRecursive
{
	std::recursive_mutex mutex;

public:
	bool TryLock() { return mutex.try_lock(); }
	void Lock() { mutex.lock(); }
	void Unlock() { mutex.unlock(); }
};

//-----------------------------------------------------------------------------
//
// An alternative mutex that is useful for cases when thread contention is 
// rare, but a mutex is required. Instances should be declared volatile.
// Sleep of 0 may not be sufficient to keep high priority threads from starving 
// lesser threads. This class is not a suitable replacement for a critical
// section if the resource contention is high.
//
//-----------------------------------------------------------------------------

#if !defined(THREAD_PROFILER)

// Just make these the same thing, if we really need to, we can recreate these using atomics
// But honestly the standard mutex objects will likely be superior anyway
// Implementations already behave as spinlocks for brief periods before sleeping like our old one
using CThreadFastMutex = CThreadMutex;
using CThreadFastMutexRecursive = CThreadMutexRecursive;

#else
typedef CThreadMutex CThreadFastMutex;
#endif

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

class CThreadNullMutex
{
public:
	static void Lock()				{}
	static void Unlock()			{}

	static bool TryLock()			{ return true; }
	static bool AssertOwnedByCurrentThread() { return true; }
	static void SetTrace( bool b )	{}

	static uint32 GetOwnerId() 		{ return 0;	}
	static int	GetDepth() 			{ return 0; }
};

//-----------------------------------------------------------------------------
//
// A mutex decorator class used to control the use of a mutex, to make it
// less expensive when not multithreading
//
//-----------------------------------------------------------------------------

template <class BaseClass, bool *pCondition>
class CThreadConditionalMutex : public BaseClass
{
public:
	void Lock()				{ if ( *pCondition ) BaseClass::Lock(); }
	void Lock() const 		{ if ( *pCondition ) BaseClass::Lock(); }
	void Unlock()			{ if ( *pCondition ) BaseClass::Unlock(); }
	void Unlock() const		{ if ( *pCondition ) BaseClass::Unlock(); }

	bool TryLock()			{ if ( *pCondition ) return BaseClass::TryLock(); else return true; }
	bool TryLock() const 	{ if ( *pCondition ) return BaseClass::TryLock(); else return true; }
	bool AssertOwnedByCurrentThread() { if ( *pCondition ) return BaseClass::AssertOwnedByCurrentThread(); else return true; }
	void SetTrace( bool b ) { if ( *pCondition ) BaseClass::SetTrace( b ); }
};

//-----------------------------------------------------------------------------
// Mutex decorator that blows up if another thread enters
//-----------------------------------------------------------------------------

template <class BaseClass>
class CThreadTerminalMutex : public BaseClass
{
public:
	bool TryLock()			{ if ( !BaseClass::TryLock() ) { DebuggerBreak(); return false; } return true; }
	bool TryLock() const 	{ if ( !BaseClass::TryLock() ) { DebuggerBreak(); return false; } return true; }
	void Lock()				{ if ( !TryLock() ) BaseClass::Lock(); }
	void Lock() const 		{ if ( !TryLock() ) BaseClass::Lock(); }

};

//-----------------------------------------------------------------------------
//
// Class to Lock a critical section, and unlock it automatically
// when the lock goes out of scope
//
//-----------------------------------------------------------------------------

template <class MUTEX_TYPE = CThreadMutex>
class CAutoLockT
{
public:
	FORCEINLINE CAutoLockT( MUTEX_TYPE &lock)
		: m_lock(lock)
	{
		m_lock.Lock();
	}

	FORCEINLINE CAutoLockT(const MUTEX_TYPE &lock)
		: m_lock(const_cast<MUTEX_TYPE &>(lock))
	{
		m_lock.Lock();
	}

	FORCEINLINE ~CAutoLockT()
	{
		m_lock.Unlock();
	}


private:
	MUTEX_TYPE &m_lock;

	// Disallow copying
	CAutoLockT( const CAutoLockT<MUTEX_TYPE> & );
	CAutoLockT<MUTEX_TYPE> &operator=( const CAutoLockT<MUTEX_TYPE> & );
};

typedef CAutoLockT<CThreadMutex> CAutoLock;

//---------------------------------------------------------

template <int size>	struct CAutoLockTypeDeducer {};
template <> struct CAutoLockTypeDeducer<sizeof(CThreadMutex)> {	typedef CThreadMutex Type_t; };
template <> struct CAutoLockTypeDeducer<sizeof(CThreadNullMutex)> {	typedef CThreadNullMutex Type_t; };
#if !defined(THREAD_PROFILER)
#endif

#define AUTO_LOCK_( type, mutex ) \
	CAutoLockT< type > UNIQUE_ID( static_cast<const type &>( mutex ) )

#if defined(GNUC)

template<typename T> T strip_cv_quals_for_mutex(T&);
template<typename T> T strip_cv_quals_for_mutex(const T&);
template<typename T> T strip_cv_quals_for_mutex(volatile T&);
template<typename T> T strip_cv_quals_for_mutex(const volatile T&);

#define AUTO_LOCK( mutex ) \
    AUTO_LOCK_( __typeof__(::strip_cv_quals_for_mutex(mutex)), mutex )

#else // GNUC

#define AUTO_LOCK( mutex ) \
	AUTO_LOCK_( CAutoLockTypeDeducer<sizeof(mutex)>::Type_t, mutex )

#endif

#define AUTO_LOCK_FM( mutex ) \
	AUTO_LOCK_( CThreadFastMutex, mutex )

#define LOCAL_THREAD_LOCK_( tag ) \
	; \
	static CThreadFastMutex autoMutex_##tag; \
	AUTO_LOCK( autoMutex_##tag )

#define LOCAL_THREAD_LOCK() \
	LOCAL_THREAD_LOCK_(_)

//-----------------------------------------------------------------------------
//
// Base class for event, semaphore and mutex objects.
//
//-----------------------------------------------------------------------------

class PLATFORM_CLASS CThreadSyncObject
{
public:
	~CThreadSyncObject();

	//-----------------------------------------------------
	// Query if object is useful
	//-----------------------------------------------------
	bool operator!() const;

	//-----------------------------------------------------
	// Access handle
	//-----------------------------------------------------
#ifdef _WIN32
	operator HANDLE() { return GetHandle(); }
	const HANDLE GetHandle() const { return m_hSyncObject; }
#endif
	//-----------------------------------------------------
	// Wait for a signal from the object
	//-----------------------------------------------------
	bool Wait( uint32 dwTimeout = TT_INFINITE );

protected:
	CThreadSyncObject();
	void AssertUseable();

#ifdef _WIN32
	HANDLE m_hSyncObject;
	bool m_bCreatedHandle;
#elif defined(POSIX)
	pthread_mutex_t	m_Mutex;
	pthread_cond_t	m_Condition;
	bool m_bInitalized;
	int m_cSet;
	bool m_bManualReset;
	bool m_bWakeForEvent;
#else
#error "Implement me"
#endif

private:
	CThreadSyncObject( const CThreadSyncObject & );
	CThreadSyncObject &operator=( const CThreadSyncObject & );
};


//-----------------------------------------------------------------------------
//
// Wrapper for unnamed event objects
//
//-----------------------------------------------------------------------------

#if defined( _WIN32 )

//-----------------------------------------------------------------------------
//
// CThreadSemaphore
//
//-----------------------------------------------------------------------------

class PLATFORM_CLASS CThreadSemaphore : public CThreadSyncObject
{
public:
	CThreadSemaphore(long initialValue, long maxValue);

	//-----------------------------------------------------
	// Increases the count of the semaphore object by a specified
	// amount.  Wait() decreases the count by one on return.
	//-----------------------------------------------------
	bool Release(long releaseCount = 1, long * pPreviousCount = NULL );

private:
	CThreadSemaphore(const CThreadSemaphore &);
	CThreadSemaphore &operator=(const CThreadSemaphore &);
};


//-----------------------------------------------------------------------------
//
// A mutex suitable for out-of-process, multi-processor usage
//
//-----------------------------------------------------------------------------

class PLATFORM_CLASS CThreadFullMutex : public CThreadSyncObject
{
public:
	CThreadFullMutex( bool bEstablishInitialOwnership = false, const char * pszName = NULL );

	//-----------------------------------------------------
	// Release ownership of the mutex
	//-----------------------------------------------------
	bool Release();

	// To match regular CThreadMutex:
	void Lock()							{ Wait(); }
	void Lock( unsigned timeout )		{ Wait( timeout ); }
	void Unlock()						{ Release(); }
	bool AssertOwnedByCurrentThread()	{ return true; }
	void SetTrace( bool )				{}

private:
	CThreadFullMutex( const CThreadFullMutex & );
	CThreadFullMutex &operator=( const CThreadFullMutex & );
};
#endif


class PLATFORM_CLASS CThreadEvent : public CThreadSyncObject
{
public:
	CThreadEvent( bool fManualReset = false );
#ifdef WIN32
	CThreadEvent( HANDLE hHandle );
#endif
	//-----------------------------------------------------
	// Set the state to signaled
	//-----------------------------------------------------
	bool Set();

	//-----------------------------------------------------
	// Set the state to nonsignaled
	//-----------------------------------------------------
	bool Reset();

	//-----------------------------------------------------
	// Check if the event is signaled
	//-----------------------------------------------------
	bool Check();

	bool Wait( uint32 dwTimeout = TT_INFINITE );

private:
	CThreadEvent( const CThreadEvent & );
	CThreadEvent &operator=( const CThreadEvent & );
};

// Hard-wired manual event for use in array declarations
class CThreadManualEvent : public CThreadEvent
{
public:
	CThreadManualEvent()
	 :	CThreadEvent( true )
	{
	}
};

inline int ThreadWaitForEvents( int nEvents, CThreadEvent * const *pEvents, bool bWaitAll = true, unsigned timeout = TT_INFINITE )
{
#ifdef POSIX
  Assert( nEvents == 1);
  if ( pEvents[0]->Wait( timeout ) )
	  return WAIT_OBJECT_0;
  else
	return WAIT_TIMEOUT;
#else
	HANDLE handles[64];
	for ( unsigned int i = 0; i < min( nEvents, ARRAYSIZE(handles) ); i++ )
		handles[i] = pEvents[i]->GetHandle();
	return ThreadWaitForObjects( nEvents, handles, bWaitAll, timeout );
#endif
}

//-----------------------------------------------------------------------------
//
// CThreadRWLock
//
//-----------------------------------------------------------------------------

class CThreadRWLock
{
	std::shared_timed_mutex mutex{};
public:
	void LockForRead() { mutex.lock_shared(); }
	void UnlockRead() { mutex.unlock_shared(); }

	void LockForWrite() { mutex.lock(); }
	void UnlockWrite() { mutex.unlock(); }
};

//-----------------------------------------------------------------------------
//
// CThreadSpinRWLock
//
//-----------------------------------------------------------------------------

class CThreadSpinRWLock
{
	std::atomic<uint32_t> lock{};
	std::atomic_bool writePending{};

	static constexpr uint32_t WRITE_MODE = (std::numeric_limits<uint32_t>::max)();
	static constexpr uint32_t READ_MAX = WRITE_MODE - 1;
public:

	bool TryLockForWrite()
	{
		uint32_t was = lock;

		if ( was == 0 )
		{
			return lock.compare_exchange_strong( was, WRITE_MODE );
		}

		return false;
	}

	bool TryLockForRead()
	{
		uint32_t was = lock;

		if ( was == 0 && writePending.load() )
			return false;
		
		if ( was < READ_MAX )
			return lock.compare_exchange_strong( was, was + 1 );
		
		return false;
	}

	void LockForRead()
	{
		while ( true )
		{
			uint32_t was = lock;

			if ( was == 0 && writePending.load() )
				continue;

			if ( was < READ_MAX )
			{
				if ( lock.compare_exchange_weak( was, was + 1 ) )
					return;
			}
		}
	}

	void UnlockRead()
	{
		while ( true )
		{
			uint32_t was = lock;

			if ( was == 0 )
				return;
			
			if ( was <= READ_MAX )
			{
				if ( lock.compare_exchange_weak( was, was - 1 ) )
					return;
			}
		}
	}

	void LockForWrite()
	{
		writePending.exchange(true);

		while ( true )
		{
			uint32_t was = lock;

			if ( was == 0 )
			{
				if ( lock.compare_exchange_weak( was, WRITE_MODE ) )
					return;
			}
		}
	}

	void UnlockWrite()
	{
		writePending.store(false);

		while ( true )
		{
			uint32_t was = lock;

			if ( was != WRITE_MODE )
				return;
			
			if ( lock.compare_exchange_weak( was, 0 ) )
				return;
		}
	}
};

//-----------------------------------------------------------------------------
//
// A thread wrapper similar to a Java thread.
//
//-----------------------------------------------------------------------------

class PLATFORM_CLASS CThread
{
public:
	CThread();
	virtual ~CThread();

	//-----------------------------------------------------

	const char *GetName();
	void SetName( const char * );

	size_t CalcStackDepth( void *pStackVariable )		{ return ((byte *)m_pStackBase - (byte *)pStackVariable); }

	//-----------------------------------------------------
	// Functions for the other threads
	//-----------------------------------------------------

	// Start thread running  - error if already running
	virtual bool Start( unsigned nBytesStack = 0 );

	// Returns true if thread has been created and hasn't yet exited
	bool IsAlive();

	// This method causes the current thread to wait until this thread
	// is no longer alive.
	bool Join( unsigned timeout = TT_INFINITE );

#ifdef _WIN32
	// Access the thread handle directly
	HANDLE GetThreadHandle();
	uint GetThreadId();
#elif defined( LINUX )
	uint GetThreadId();
#endif

	//-----------------------------------------------------

	int GetResult();

	//-----------------------------------------------------
	// Functions for both this, and maybe, and other threads
	//-----------------------------------------------------

	// Forcibly, abnormally, but relatively cleanly stop the thread
	void Stop( int exitCode = 0 );

	// Get the priority
	int GetPriority() const;

	// Set the priority
	bool SetPriority( int );

	// Request a thread to suspend, this must ONLY be called from the thread itself, not the main thread
	// This suspend variant causes the thread in question to suspend at a known point in its execution
	// which means you don't risk the global deadlocks/hangs potentially caused by the raw Suspend() call
	void SuspendCooperative();

	// Resume a previously suspended thread from the Cooperative call
	void ResumeCooperative();

	// wait for a thread to execute its SuspendCooperative call 
	void BWaitForThreadSuspendCooperative();

#ifndef LINUX
	// forcefully Suspend a thread
	unsigned int Suspend();

	// forcefully Resume a previously suspended thread
	unsigned int Resume();
#endif

	// Force hard-termination of thread.  Used for critical failures.
	bool Terminate( int exitCode = 0 );

	//-----------------------------------------------------
	// Global methods
	//-----------------------------------------------------

	// Get the Thread object that represents the current thread, if any.
	// Can return NULL if the current thread was not created using
	// CThread
	static CThread *GetCurrentCThread();

	// Offer a context switch. Under Win32, equivalent to Sleep(0)
#ifdef Yield
#undef Yield
#endif
	static void Yield();

	// This method causes the current thread to yield and not to be
	// scheduled for further execution until a certain amount of real
	// time has elapsed, more or less.
	static void Sleep( unsigned duration );

protected:

	// Optional pre-run call, with ability to fail-create. Note Init()
	// is forced synchronous with Start()
	virtual bool Init();

	// Thread will run this function on startup, must be supplied by
	// derived class, performs the intended action of the thread.
	virtual int Run() = 0;

	// Called when the thread is about to exit, by the about-to-exit thread.
	virtual void OnExit();

	// Called after OnExit when a thread finishes or is killed. Not virtual because no inherited classes
	// override it and we don't want to change the vtable from the published SDK version.
	void Cleanup();

	bool WaitForCreateComplete( CThreadEvent *pEvent );

	// "Virtual static" facility
	typedef unsigned (__stdcall *ThreadProc_t)( void * );
	virtual ThreadProc_t GetThreadProc();
	virtual bool IsThreadRunning();

	CThreadMutex m_Lock;

#ifdef WIN32
	ThreadHandle_t GetThreadID() const { return (ThreadHandle_t)m_hThread; }
#else
	ThreadId_t GetThreadID() const { return (ThreadId_t)m_threadId; }
#endif

private:
	enum Flags
	{
		SUPPORT_STOP_PROTOCOL = 1 << 0
	};

	// Thread initially runs this. param is actually 'this'. function
	// just gets this and calls ThreadProc
	struct ThreadInit_t
	{
		CThread *     pThread;
		CThreadEvent *pInitCompleteEvent;
		bool *        pfInitSuccess;
	};

	static unsigned __stdcall ThreadProc( void * pv );

	// make copy constructor and assignment operator inaccessible
	CThread( const CThread & );
	CThread &operator=( const CThread & );

#ifdef _WIN32
	HANDLE 	m_hThread;
	ThreadId_t m_threadId;
#elif defined(POSIX)
	pthread_t m_threadId;
#endif
	CInterlockedInt m_nSuspendCount;
	CThreadEvent m_SuspendEvent;
	CThreadEvent m_SuspendEventSignal;
	int		m_result;
	char	m_szName[32];
	void *	m_pStackBase;
	unsigned m_flags;
};

//-----------------------------------------------------------------------------
//
// A helper class to let you sleep a thread for memory validation, you need to handle
//	 m_bSleepForValidate in your ::Run() call and set m_bSleepingForValidate when sleeping
//
//-----------------------------------------------------------------------------
class PLATFORM_CLASS CValidatableThread : public CThread
{
public:
	CValidatableThread()
	{
		m_bSleepForValidate = false;
		m_bSleepingForValidate = false;
	}

#ifdef DBGFLAG_VALIDATE
	virtual void SleepForValidate() { m_bSleepForValidate = true; }
	bool BSleepingForValidate() { return m_bSleepingForValidate; }
	virtual void WakeFromValidate() { m_bSleepForValidate = false; }
#endif
protected:
	bool m_bSleepForValidate;
	bool m_bSleepingForValidate;
};

//-----------------------------------------------------------------------------
// Simple thread class encompasses the notion of a worker thread, handing
// synchronized communication.
//-----------------------------------------------------------------------------


// These are internal reserved error results from a call attempt
enum WTCallResult_t
{
	WTCR_FAIL			= -1,
	WTCR_TIMEOUT		= -2,
	WTCR_THREAD_GONE	= -3,
};

class CFunctor;
class PLATFORM_CLASS CWorkerThread : public CThread
{
public:
	CWorkerThread();

	//-----------------------------------------------------
	//
	// Inter-thread communication
	//
	// Calls in either direction take place on the same "channel."
	// Seperate functions are specified to make identities obvious
	//
	//-----------------------------------------------------

	// Master: Signal the thread, and block for a response
	int CallWorker( unsigned, unsigned timeout = TT_INFINITE, bool fBoostWorkerPriorityToMaster = true, CFunctor *pParamFunctor = NULL );

	// Worker: Signal the thread, and block for a response
	int CallMaster( unsigned, unsigned timeout = TT_INFINITE );

	// Wait for the next request
	bool WaitForCall( unsigned dwTimeout, unsigned *pResult = NULL );
	bool WaitForCall( unsigned *pResult = NULL );

	// Is there a request?
	bool PeekCall( unsigned *pParam = NULL, CFunctor **ppParamFunctor = NULL );

	// Reply to the request
	void Reply( unsigned );

	// Wait for a reply in the case when CallWorker() with timeout != TT_INFINITE
	int WaitForReply( unsigned timeout = TT_INFINITE );

	// If you want to do WaitForMultipleObjects you'll need to include
	// this handle in your wait list or you won't be responsive
	CThreadEvent &GetCallHandle();
	// Find out what the request was
	unsigned GetCallParam( CFunctor **ppParamFunctor = NULL ) const;

	// Boost the worker thread to the master thread, if worker thread is lesser, return old priority
	int BoostPriority();

protected:
#ifndef _WIN32
#define __stdcall
#endif
	typedef uint32 (__stdcall *WaitFunc_t)( int nEvents, CThreadEvent * const *pEvents, int bWaitAll, uint32 timeout );
	
	int Call( unsigned, unsigned timeout, bool fBoost, WaitFunc_t = NULL, CFunctor *pParamFunctor = NULL );
	int WaitForReply( unsigned timeout, WaitFunc_t );

private:
	CWorkerThread( const CWorkerThread & );
	CWorkerThread &operator=( const CWorkerThread & );

	CThreadEvent	m_EventSend;
	CThreadEvent	m_EventComplete;

	unsigned        m_Param;
	CFunctor		*m_pParamFunctor;
	int				m_ReturnVal;
};


// a unidirectional message queue. A queue of type T. Not especially high speed since each message
// is malloced/freed. Note that if your message class has destructors/constructors, they MUST be
// thread safe!
template<class T> class CMessageQueue
{
	CThreadEvent SignalEvent;								// signals presence of data
	CThreadMutex QueueAccessMutex;

	// the parts protected by the mutex
	struct MsgNode
	{
		MsgNode *Next;
		T Data;
	};

	MsgNode *Head;
	MsgNode *Tail;

public:
	CMessageQueue( void )
	{
		Head = Tail = NULL;
	}

	// check for a message. not 100% reliable - someone could grab the message first
	bool MessageWaiting( void ) 
	{
		return ( Head != NULL );
	}

	void WaitMessage( T *pMsg )
	{
		for(;;)
		{
			while( ! MessageWaiting() )
				SignalEvent.Wait();
			QueueAccessMutex.Lock();
			if (! Head )
			{
				// multiple readers could make this null
				QueueAccessMutex.Unlock();
				continue;
			}
			*( pMsg ) = Head->Data;
			MsgNode *remove_this = Head;
			Head = Head->Next;
			if (! Head)										// if empty, fix tail ptr
				Tail = NULL;
			QueueAccessMutex.Unlock();
			delete remove_this;
			break;
		}
	}

	void QueueMessage( T const &Msg)
	{
		MsgNode *new1=new MsgNode;
		new1->Data=Msg;
		new1->Next=NULL;
		QueueAccessMutex.Lock();
		if ( Tail )
		{
			Tail->Next=new1;
			Tail = new1;
		}
		else
		{
			Head = new1;
			Tail = new1;
		}
		SignalEvent.Set();
		QueueAccessMutex.Unlock();
	}
};

// read data from a memory address
template<class T> FORCEINLINE T ReadVolatileMemory( T const *pPtr )
{
	volatile const T * pVolatilePtr = ( volatile const T * ) pPtr;
	return *pVolatilePtr;
}

//-----------------------------------------------------------------------------

#if defined( _WIN32 )
#pragma warning(pop)
#endif

#endif // THREADTOOLS_H
