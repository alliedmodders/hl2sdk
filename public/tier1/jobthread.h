//========== Copyright © 2005, Valve Corporation, All rights reserved. ========
//
// Purpose:	A utility for a discrete job-oriented worker thread.
//
//			The class CAsyncJobFuliller is both the job queue, and the 
//			worker thread. Except when the main thread attempts to 
//			synchronously execute a job, most of the inter-thread locking 
//			on the queue. 
//
//			The queue threading model uses a manual reset event for optimal 
//			throughput. Adding to the queue is guarded by a semaphore that 
//			will block the inserting thread if the queue has overflown. 
//			This prevents the worker thread from being starved out even if 
//			not running at a higher priority than the master thread.
//
//			The thread function waits for jobs, services jobs, and manages 
//			communication between the worker and master threads. The nature 
//			of the work is opaque to the fulfiller.
//
//			CAsyncJob instances actually do the work. The base class 
//			calls virtual methods for job primitives, so derivations don't 
//			need to worry about threading models. All of the variants of 
//			job and OS can be expressed in this hierarchy. Instances of 
//			CAsyncJob are the items placed in the queue, and by 
//			overriding the job primitives they are the manner by which 
//			users of the fulfiller control the state of the job. 
//
//=============================================================================

#include <limits.h>
#include "tier0/threadtools.h"
#include "tier1/refcount.h"
#include "tier1/utllinkedlist.h"

#ifndef JOBTHREAD_H
#define JOBTHREAD_H

#ifdef AddJob  // windows.h print function collisions
#undef AddJob
#undef GetJob
#endif

#if defined( _WIN32 )
#pragma once
#endif

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------

class CAsyncJob;

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
enum AsyncStatusEnum_t
{
	// Use negative for errors
	ASYNC_OK,						// operation is successful
	ASYNC_STATUS_PENDING,			// file is properly queued, waiting for service
	ASYNC_STATUS_INPROGRESS,		// file is being accessed
	ASYNC_STATUS_ABORTED,			// file was aborted by caller
	ASYNC_STATUS_UNSERVICED,		// file is not yet queued
};

typedef int AsyncStatus_t;

enum AsyncJobFlags_t
{
	AJF_IO				= ( 1 << 0 ),	// The job primarily blocks on IO or hardware
	AJF_BOOST_THREAD	= ( 1 << 1 ),	// Up the thread priority to max allowed while processing task
};

enum AsyncJobPriority_t
{
	AJP_LOW,
	AJP_NORMAL,
	AJP_HIGH
};

//-----------------------------------------------------------------------------
//
// CAsyncJobFuliller
//
//-----------------------------------------------------------------------------

//---------------------------------------------------------
// Messages supported through the CallWorker() method
//---------------------------------------------------------
enum AsyncFulfillerMessages_t
{
	AF_EXIT,		// Exit the thread
	AF_SUSPEND,		// Suspend after next operation
};

class CAsyncJobFuliller : public CWorkerThread
{
public:
	CAsyncJobFuliller( int maxJobs );
	CAsyncJobFuliller();
	~CAsyncJobFuliller();

	//-----------------------------------------------------
	// Thread functions
	//-----------------------------------------------------
	bool Start( unsigned nBytesStack = 0 );

	//-----------------------------------------------------
	// Functions for any thread
	//-----------------------------------------------------
	unsigned GetJobCount()							{ AUTO_LOCK( m_mutex ); return m_queue.Count(); }

	bool IsUsingSemaphore()							{ AUTO_LOCK( m_mutex ); return m_bUseSemaphore; }

	//-----------------------------------------------------
	// Pause/resume processing jobs
	//-----------------------------------------------------
	int SuspendExecution();
	int ResumeExecution();

	//-----------------------------------------------------
	// Execute a job in the context of the worker thread
	//-----------------------------------------------------
	AsyncStatus_t ExecuteImmediate( CAsyncJob * );

	//-----------------------------------------------------
	// Add a job to the queue (master thread)
	//-----------------------------------------------------
	void AddJob( CAsyncJob * );

	//-----------------------------------------------------
	// Remove a job from the queue (master thread)
	//-----------------------------------------------------
	void ReprioritizedJob( CAsyncJob *p )			{ if ( RemoveJob( p ) ) AddJob( p ); }

	//-----------------------------------------------------
	// Remove a job from the queue (master thread)
	//-----------------------------------------------------
	bool RemoveJob( CAsyncJob * );

	//-----------------------------------------------------
	// Bulk job manipulation (blocking)
	//-----------------------------------------------------
	int ExecuteAll()								{ return ExecuteToPriority( AJP_LOW ); }
	int ExecuteToPriority( AsyncJobPriority_t toPriority );
	int AbortAll();

	//-----------------------------------------------------
	// Queue lock
	//-----------------------------------------------------
	CThreadMutex &GetQueueLock()					{ return m_mutex; }

	//-----------------------------------------------------
	// Remove the next job from the queue (worker thread, or master when worker suspended)
	//-----------------------------------------------------
	CAsyncJob *GetJob();

	//-----------------------------------------------------
	// Raw access to queue. Should lock queue or suspend worker before using
	//-----------------------------------------------------
	CUtlLinkedList<CAsyncJob *> &AccessQueue()	{ return m_queue; }

protected:
	//-----------------------------------------------------
	// Thread functions
	//-----------------------------------------------------
	int Run();

#if defined( _WIN32 )
	//-----------------------------------------------------
	// Get handle to include in WaitForMultipleObjects wait list (worker thread)
	//-----------------------------------------------------
	HANDLE GetJobSignalHandle()						{ return m_JobSignal; }
#endif

private:
	void WaitPut();
	void ReleasePut();

	CUtlLinkedList<CAsyncJob *>	m_queue;
	CThreadMutex				m_mutex;
	CThreadEvent				m_JobSignal;
#ifdef _WIN32
	CThreadSemaphore			m_PutSemaphore;
#endif
	bool						m_bUseSemaphore;
	int							m_nSuspend;
};

//-----------------------------------------------------------------------------
// Class to combine the metadata for an operation and the ability to perform
// the operation
//-----------------------------------------------------------------------------
abstract_class CAsyncJob : public CRefCounted1<IRefCounted, CRefCountServiceMT>
{
public:
	CAsyncJob( AsyncJobPriority_t priority = AJP_NORMAL )
	  : m_status( ASYNC_STATUS_UNSERVICED ),
		m_queueID( -1 ),
		m_pFulfiller( NULL ),
		m_priority( priority )
	{
	}

	//-----------------------------------------------------
	// Priority (not thread safe)
	//-----------------------------------------------------
	void SetPriority( AsyncJobPriority_t priority )	{ m_priority = priority; }
	AsyncJobPriority_t GetPriority() const			{ return m_priority; }

	//-----------------------------------------------------
	// Fast queries
	//-----------------------------------------------------
	bool Executed() const				{ return ( m_status == ASYNC_OK );	}
	bool IsFinished() const				{ return ( m_status != ASYNC_STATUS_PENDING && m_status != ASYNC_STATUS_INPROGRESS && m_status != ASYNC_STATUS_UNSERVICED ); }
	AsyncStatus_t GetStatus() const		{ return m_status; }
	
	//-----------------------------------------------------
	// Try to acquire ownership (to satisfy). If you take the lock, you must either execute or abort.
	//-----------------------------------------------------
	bool TryLock() const	{ return m_mutex.TryLock(); }
	void Lock() const 		{ m_mutex.Lock(); }
	void Unlock() const		{ m_mutex.Unlock(); }

	//-----------------------------------------------------
	// Perform the job
	//-----------------------------------------------------
	AsyncStatus_t Execute();
	AsyncStatus_t TryExecute();

	//-----------------------------------------------------
	// Terminate the job, discard if partially or wholly fulfilled
	//-----------------------------------------------------
	AsyncStatus_t Abort( bool bDiscard = true );

	virtual char const			*Describe() { return "AsyncJob"; }

protected:
	//-----------------------------------------------------
	friend class CAsyncJobFuliller;

	AsyncStatus_t		m_status;
	AsyncJobPriority_t	m_priority;
	int					m_queueID;
	CThreadMutex		m_mutex;
	CAsyncJobFuliller *	m_pFulfiller;

private:
	//-----------------------------------------------------
	CAsyncJob( const CAsyncJob &fromRequest );
	void operator=(const CAsyncJob &fromRequest );

	virtual AsyncStatus_t DoExecute() = 0;
	virtual AsyncStatus_t DoAbort( bool bDiscard ) { return ASYNC_STATUS_ABORTED; }
	virtual void DoCleanup() {}
};

//-----------------------------------------------------------------------------

#endif // JOBTHREAD_H
