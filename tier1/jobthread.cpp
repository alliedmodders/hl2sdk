//========== Copyright © 2005, Valve Corporation, All rights reserved. ========
//
// Purpose:
//
//=============================================================================

#if defined( _WIN32 )
#ifdef _XBOX
#include "xbox/xbox_platform.h"
#include "xbox/xbox_win32stubs.h"
#include "xbox/xbox_core.h"
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#endif // _WIN32

#include "tier0/dbg.h"
#include "tier1/jobthread.h"

#include "tier1/utlvector.h"

//-----------------------------------------------------------------------------

#define NO_THREADPOOL

#ifdef _LINUX
#define NO_THREADPOOL
#endif


#ifndef NO_THREADPOOL
class CThreadPool
{
public:
	CThreadPool()
	 :	m_pPendingJob( NULL ),
		m_Exit( true )
	{
	}

	void Init( int nThreads, int stackSize, int priority, bool bDistribute )
	{
		while ( nThreads-- )
		{
			int iThread = m_Threads.AddToTail();
			m_IdleEvents.AddToTail();
			m_Threads[iThread] = CreateSimpleThread( PoolThreadFunc, this, stackSize );
			m_IdleEvents[iThread].Wait();
		}

		if ( bDistribute )
		{
			// TODO
		}
	}

	void Execute( CAsyncJob *pJob )
	{
		if ( m_Threads.Count() )
		{
			pJob->AddRef();
			m_pPendingJob = pJob;

			m_JobAccepted.Reset();
			m_JobAvailable.Set();
			m_JobAccepted.Wait();
		}
		else
		{
			pJob->TryExecute();
		}
	}

	void WaitForIdle()
	{
		WaitForMultipleObjects( m_IdleEvents.Count(), (HANDLE *)m_IdleEvents.Base(), TRUE, 60000 );
	}

	void Term()
	{
		m_Exit.Set();
		WaitForMultipleObjects( m_Threads.Count(), (HANDLE *)m_Threads.Base(), TRUE, 60000 );
	}

private:
	static unsigned PoolThreadFunc( void *pParam )
	{
		CThreadPool *pOwner = (CThreadPool *)pParam;
		int iThread = pOwner->m_Threads.Count() - 1;
		pOwner->m_IdleEvents[iThread].Set();

		HANDLE waitHandles[] =
		{
			pOwner->m_JobAvailable,
			pOwner->m_Exit
		};

		DWORD waitResult;

		while ( ( waitResult = WaitForMultipleObjects( ARRAYSIZE(waitHandles), waitHandles, FALSE, INFINITE ) ) != WAIT_FAILED )
		{
			switch ( waitResult - WAIT_OBJECT_0 )
			{
			case 0:
				{
					pOwner->m_IdleEvents[iThread].Reset();
					CAsyncJob *pJob = pOwner->m_pPendingJob;
					pOwner->m_pPendingJob = NULL;
					pOwner->m_JobAccepted.Set();

					pJob->TryExecute();
					pJob->Release();

					pOwner->m_IdleEvents[iThread].Set();
					break;
				}

			case 1:
				{
					return 0;
				}
			}
		}

		return 1;
	}

private:
	CAsyncJob *m_pPendingJob;

	CThreadEvent m_JobAvailable;
	CThreadEvent m_JobAccepted;
	CThreadEvent m_Exit;
	CInterlockedInt m_IdleCount;

	CUtlVector<ThreadHandle_t> m_Threads;
	CUtlVector<CThreadManualEvent> m_IdleEvents;
};

#else

class CThreadPool
{
public:
	void Init( int nThreads, int stackSize, int priority, bool bDistribute )
	{
	}

	void Execute( CAsyncJob *pJob )
	{
		pJob->TryExecute();
	}

	void WaitForIdle()
	{
	}

	void Term()
	{
	}

};

#endif

CThreadPool g_TestThreadPool;


//-----------------------------------------------------------------------------
//
// CAsyncJobFuliller
//
//-----------------------------------------------------------------------------

CAsyncJobFuliller::CAsyncJobFuliller( int maxJobs )
  :	m_JobSignal( true ),
#ifdef _WIN32
	m_PutSemaphore( maxJobs, maxJobs ),
	m_bUseSemaphore( true ),
#elif _LINUX
	m_bUseSemaphore( false ), // no semaphore support
#endif
	m_nSuspend( 0 )
{
	Assert( maxJobs < USHRT_MAX - 1 );
}

//---------------------------------------------------------

CAsyncJobFuliller::CAsyncJobFuliller()
  : m_JobSignal( true ),
#ifdef _WIN32
	m_PutSemaphore( 0, 0 ),
#endif
	m_bUseSemaphore( false ),
	m_nSuspend( 0 )
{
}

//---------------------------------------------------------

CAsyncJobFuliller::~CAsyncJobFuliller()
{
}

//---------------------------------------------------------

inline void CAsyncJobFuliller::WaitPut()
{
#ifdef _WIN32
	if ( m_bUseSemaphore )
		m_PutSemaphore.Wait();
#endif
}

//---------------------------------------------------------

inline void CAsyncJobFuliller::ReleasePut()
{
#ifdef _WIN32
	if ( m_bUseSemaphore )
		m_PutSemaphore.Release();
#endif
}

//---------------------------------------------------------
// Pause/resume processing jobs
//---------------------------------------------------------
int CAsyncJobFuliller::SuspendExecution()
{
#ifdef _WIN32
	if ( !ThreadInMainThread() )
	{
		Assert( 0 );
		return 0;
	}

	// If not already suspended
	if ( m_nSuspend == 0 )
	{
		// Make sure state is correct
		int curCount = Suspend();
		Resume();
		Assert( curCount == 0 );

		if ( curCount == 0 )
		{
			CallWorker( AF_SUSPEND );

			// Because worker must signal before suspending, we could reach
			// here with the thread not actually suspended
			while ( Suspend() == 0 )
			{
				Resume();
				ThreadSleep();
			}
			Resume();
		}

#ifdef _DEBUG
		curCount = Suspend();
		Resume();
		Assert( curCount > 0 );
#endif
	}

	return m_nSuspend++;
#else
	return 1;
#endif
}

//---------------------------------------------------------

int CAsyncJobFuliller::ResumeExecution()
{
#ifdef _WIN32
	if ( !ThreadInMainThread() )
	{
		Assert( 0 );
		return 0;
	}

	AssertMsg( m_nSuspend >= 1, "Attempted resume when not suspended");
	int result = m_nSuspend--;
	if (m_nSuspend == 0 )
	{
		Resume();
	}
	return result;
#else
	return 0;
#endif
}

//---------------------------------------------------------
// Add a job to the queue
//---------------------------------------------------------

void CAsyncJobFuliller::AddJob( CAsyncJob *pJob )
{
	if ( !pJob )
	{
		return;
	}

#ifdef _WIN32
	BoostPriority();

	// If queue is full, wait for worker to get something from our queue
	WaitPut();

	m_mutex.Lock();

	pJob->AddRef();

	unsigned i = m_queue.Tail();
	int priority = pJob->GetPriority();

	while ( i != m_queue.InvalidIndex() && priority > m_queue[i]->GetPriority() )
	{
		i = m_queue.Previous( i );
	}

	if ( i != m_queue.InvalidIndex() )
	{
		pJob->m_queueID = m_queue.InsertAfter( i, pJob );
	}
	else
	{
		pJob->m_queueID = m_queue.AddToHead( pJob );
	}

	pJob->m_pFulfiller = this;
	pJob->m_status = ASYNC_STATUS_PENDING;

	if ( m_queue.Count() == 1 )
	{
		// Release worker to remove an object from our queue
		m_JobSignal.Set();
	}

	m_mutex.Unlock();
#else
	pJob->Execute();
#endif
}

//---------------------------------------------------------
// Remove a job from the queue
//---------------------------------------------------------

bool CAsyncJobFuliller::RemoveJob( CAsyncJob *pJob )
{
#ifdef _WIN32
	if ( !pJob )
	{
		return false;
	}

	AUTO_LOCK( m_mutex );

	if ( !m_queue.IsValidIndex( pJob->m_queueID ) )
	{
		return false;
	}

	// Take the job out
	m_queue.Remove( pJob->m_queueID );
	pJob->m_queueID = m_queue.InvalidIndex();
	pJob->m_pFulfiller = NULL;
	pJob->Release();

	// Release master to put more in
	ReleasePut();

	// If we're transitioning to empty...
	if ( m_queue.Count() == 0 )
	{
		// Block the worker until there's something to do...
		m_JobSignal.Reset();
	}
#endif

	return true;
}

//---------------------------------------------------------
// Execute to a specified priority
//---------------------------------------------------------

int CAsyncJobFuliller::ExecuteToPriority( AsyncJobPriority_t iToPriority )
{
	int nExecuted = 0;

#ifdef _WIN32
	if ( CThread::GetCurrentCThread() != this )
	{
		SuspendExecution();
	}

	if ( m_queue.Count() )
	{
		CAsyncJob *pJob = NULL;

		while ( ( pJob = GetJob() ) != NULL )
		{
			if ( pJob->GetPriority() >= iToPriority )
			{
				pJob->Execute();
				pJob->Release();
				pJob = NULL;
				nExecuted++;
			}
			else
			{
				break;
			}
		}

		// Extracted one of lower priority, so reinsert it...
		if ( pJob )
		{
			AddJob( pJob/*, true !!!!!!!! */ );
			pJob->Release();
		}
	}
	
	if ( CThread::GetCurrentCThread() != this )
	{
		ResumeExecution();
	}
#endif
	return nExecuted;
}

//---------------------------------------------------------
//
//---------------------------------------------------------

int CAsyncJobFuliller::AbortAll()
{
	int nExecuted = 0;
#ifdef _WIN32
	if ( CThread::GetCurrentCThread() != this )
	{
		SuspendExecution();
	}

	if ( m_queue.Count() )
	{
		CAsyncJob *pJob = NULL;

		while ( ( pJob = GetJob() ) != NULL )
		{
			pJob->Abort();
			pJob->Release();
		}
	}

	if ( CThread::GetCurrentCThread() != this )
	{
		ResumeExecution();
	}
#endif

	return nExecuted;
}

//---------------------------------------------------------
// Get the next job from the queue
//---------------------------------------------------------

CAsyncJob *CAsyncJobFuliller::GetJob()
{
	CAsyncJob *pReturn = NULL;
#ifdef _WIN32
	m_mutex.Lock();
	unsigned i = m_queue.Head();

	if ( i != m_queue.InvalidIndex() )
	{
		pReturn = m_queue[i];
		pReturn->AddRef();
		RemoveJob(pReturn);
	}

	m_mutex.Unlock();
#endif
	return pReturn;
}

//---------------------------------------------------------
// CAsyncJobFuliller thread functions
//---------------------------------------------------------

bool CAsyncJobFuliller::Start( unsigned nBytesStack )
{
#ifdef _WIN32
	if ( CWorkerThread::Start( nBytesStack ) )
	{
		BoostPriority();
		return true;
	}
#endif
	return false;
}

//---------------------------------------------------------

int CAsyncJobFuliller::Run()
{
#if defined( _WIN32 )
	enum FulfillerEvent_t
	{
		CALL_FROM_MASTER,
		JOB_REQUEST
	};

	g_TestThreadPool.Init( 4, 0, 2, false );

	// Wait for either a call from the master thread, or an item in the queue...
	DWORD	waitResult;
	bool	bExit = false;
	HANDLE	waitHandles[2];

	waitHandles[CALL_FROM_MASTER]	= GetCallHandle();
	waitHandles[JOB_REQUEST]  		= GetJobSignalHandle();

	while (!bExit &&
		( waitResult = WaitForMultipleObjects( 2, waitHandles, FALSE, INFINITE ) ) != WAIT_FAILED )
	{
		switch ( waitResult - WAIT_OBJECT_0 )
		{
			// It's a call from the master thread...
		case CALL_FROM_MASTER:
			{
				switch ( GetCallParam() )
				{
				case AF_EXIT:
					Reply( true );
					bExit = TRUE;
					break;

				case AF_SUSPEND:
					g_TestThreadPool.WaitForIdle();
					Reply( true );
					Suspend();
					break;

				default:
					AssertMsg( 0, "Unknown call async fulfiller" );
					Reply( false);
					break;
				}
				break;
			}

			// Otherwise, if there's a read request to service...
		case JOB_REQUEST:
			{
				// Get the request
				CAsyncJob *pJob;

				while ( ( pJob = GetJob() ) != NULL )
				{
					// Job can be NULL if the main thread may have preempted and fulfilled 
					// the job on its own.
					g_TestThreadPool.Execute( pJob );
					pJob->Release();
				}

				break;
			}

		default:
			AssertMsg( 0, "There was nothing to do!" );
		}
	}

#endif
	g_TestThreadPool.Term();
	return 0;
}

//-----------------------------------------------------------------------------
//
// CAsyncJob
//
//-----------------------------------------------------------------------------
AsyncStatus_t CAsyncJob::Execute()
{
	AUTO_LOCK( m_mutex );
	AddRef();

	AsyncStatus_t result;

	switch (m_status)
	{
	case ASYNC_STATUS_UNSERVICED:
	case ASYNC_STATUS_PENDING:
		{
			if ( m_pFulfiller ) // Jobs can exist on thier own
			{
				CAutoLock autoLock( m_pFulfiller->GetQueueLock() );
				if ( m_pFulfiller )
				{
					m_pFulfiller->RemoveJob( this );
				}
			}

			// Service it
			m_status = ASYNC_STATUS_INPROGRESS;
			ThreadSleep( 0 );
			result = m_status = DoExecute();
			DoCleanup();
			ThreadSleep( 0 );
			break;
		}

	case ASYNC_STATUS_INPROGRESS:
		AssertMsg(0, "Mutex Should have protected use while processing");
		// fall through...

	case ASYNC_OK:
	case ASYNC_STATUS_ABORTED:
		result = m_status;
		break;

	default:
		AssertMsg( m_status < ASYNC_OK, "Unknown async job state");
		result = m_status;
	}

	Release();
	return result;
}


//---------------------------------------------------------

AsyncStatus_t CAsyncJob::TryExecute()
{
	// TryLock() would only fail if another thread has entered
	// Execute() or Abort()
	if ( TryLock() )
	{
		// ...service the request
		Execute();
		Unlock();
	}
	return m_status;
}

//---------------------------------------------------------

AsyncStatus_t CAsyncJob::Abort( bool bDiscard )
{
	AUTO_LOCK(m_mutex);
	AddRef();

	AsyncStatus_t result;

	switch (m_status)
	{
	case ASYNC_STATUS_UNSERVICED:
	case ASYNC_STATUS_PENDING:
		{
			if ( m_pFulfiller ) // Jobs can exist on thier own
			{
				CAutoLock autoLock( m_pFulfiller->GetQueueLock() );
				if ( m_pFulfiller )
				{
					m_pFulfiller->RemoveJob( this );
				}
			}

			result = m_status = DoAbort( bDiscard );
			if ( bDiscard )
				DoCleanup();
		}
		break;

	case ASYNC_STATUS_ABORTED:
	case ASYNC_STATUS_INPROGRESS:
	case ASYNC_OK:
		result = m_status;
		break;

	default:
		AssertMsg( m_status < ASYNC_OK, "Unknown async job state");
		result = m_status;
	}

	Release();
	return result;
}

//-----------------------------------------------------------------------------
