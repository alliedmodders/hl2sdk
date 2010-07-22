//========= Copyright c 1996-2008, Valve Corporation, All rights reserved. ============
// This will contain things like helper functions to measure tick count of small pieces
// of code precisely; or measure performance counters (L2 misses, mispredictions etc.)
// on different hardware.
//=====================================================================================

// this class defines a section of code to measure

#ifndef L4D_TIER0_MINIPROFILER_HDR
#define L4D_TIER0_MINIPROFILER_HDR

#include <tier0/cache_hints.h>

#ifdef IS_WINDOWS_PC
#include <intrin.h>	// get __rdtsc
#endif

#if !defined(_CERT) && ( defined( WIN32 ) || defined( _X360 ) ) //	&& !defined(_LINUX)
#define ENABLE_HARDWARE_PROFILER 1
#else
#define ENABLE_HARDWARE_PROFILER 0
#endif

#if ENABLE_HARDWARE_PROFILER

#ifdef _LINUX
inline int GetHardwareClockFast( void )
{
	unsigned long long int nRet;
	__asm__ volatile (".byte 0x0f, 0x31" : "=A" (nRet));
	return ( int ) nRet;
}

#else
inline int GetHardwareClockFast()
{
#ifdef _X360
	return __mftb32();
#else
	return __rdtsc();
#endif
}
#endif
#endif

class CMiniProfiler;
class CLinkedMiniProfiler;

#ifdef TIER0_DLL_EXPORT
extern "C"
{
	DLL_EXPORT CMiniProfiler *g_pLastMiniProfiler;
	DLL_EXPORT uint32 g_nMiniProfilerFrame;
}
#else
DLL_IMPORT void PublishAll(CLinkedMiniProfiler*pList, uint32 nHistoryMax);
#if ENABLE_HARDWARE_PROFILER
DLL_IMPORT CMiniProfiler *g_pLastMiniProfiler;
#endif
#endif


class CMiniProfilerSample
{
#if ENABLE_HARDWARE_PROFILER
	int m_nTimeBaseBegin;       // time base is kept here instead of using -= and += for better reliability and to avoid a cache miss at the beginning of the profiled section
#endif

public:
	CMiniProfilerSample()
	{
#if ENABLE_HARDWARE_PROFILER
		m_nTimeBaseBegin = GetHardwareClockFast();
#endif
	}

	int GetElapsed()const
	{
#if ENABLE_HARDWARE_PROFILER
		return GetHardwareClockFast() - m_nTimeBaseBegin;
#else
		return 0;
#endif
	}
};



class CMiniProfiler
{
public:
#if ENABLE_HARDWARE_PROFILER
	// numTicks is 
	uint32 m_numTimeBaseTicks;	 // this will be totally screwed between Begin() and End()
	uint32 m_numCalls;
	uint32 m_numTimeBaseTicksInCallees; // this is the time to subtract from m_numTimeBaseTicks to get the "exclusive" time in this block
#endif
public:
	CMiniProfiler()
	{
#if ENABLE_HARDWARE_PROFILER
		Reset();
#endif
	}


	void Begin()
	{
#if ENABLE_HARDWARE_PROFILER
		m_numTimeBaseTicks -= GetHardwareClockFast();
#endif
	}

	void End()
	{
#if ENABLE_HARDWARE_PROFILER
		m_numTimeBaseTicks += GetHardwareClockFast();
		m_numCalls ++;
#endif
	}


	void Add(int numTimeBaseTicks, int numCalls = 1)
	{
#if ENABLE_HARDWARE_PROFILER
		m_numTimeBaseTicks += numTimeBaseTicks;
		m_numCalls += numCalls;
#endif
	}

	void Add(const CMiniProfilerSample &sample)
	{
		Add(sample.GetElapsed());
	}

	void SubCallee(int numTimeBaseTicks)
	{
#if ENABLE_HARDWARE_PROFILER
		m_numTimeBaseTicksInCallees += numTimeBaseTicks;
#endif
	}
	
	
	void Reset()
	{
#if ENABLE_HARDWARE_PROFILER
		m_numTimeBaseTicks = 0;
		m_numCalls = 0;
		m_numTimeBaseTicksInCallees = 0;
#endif
	}

	void Damp(int shift = 1)
	{
#if ENABLE_HARDWARE_PROFILER
		m_numTimeBaseTicks >>= shift;
		m_numCalls >>= shift;
#endif
	}

	DLL_CLASS_EXPORT  void Publish(const char *szMessage, ...);
};



class CMiniProfilerGuard: public CMiniProfilerSample
{
#if ENABLE_HARDWARE_PROFILER
	CMiniProfiler *m_pProfiler, *m_pLastProfiler;
	int m_numCallsToAdd;
#endif
public:
	CMiniProfilerGuard(CMiniProfiler *pProfiler, int numCallsToAdd = 1)
	{
#if ENABLE_HARDWARE_PROFILER
		PREFETCH_CACHE_LINE(pProfiler,0);
		m_numCallsToAdd = numCallsToAdd;
		m_pProfiler = pProfiler;
		m_pLastProfiler = g_pLastMiniProfiler;
		g_pLastMiniProfiler = pProfiler;
#endif
	}
	~CMiniProfilerGuard()
	{
#if ENABLE_HARDWARE_PROFILER
		int nElapsed = GetElapsed();
		m_pProfiler->Add(nElapsed, m_numCallsToAdd);
		m_pLastProfiler->SubCallee(nElapsed);
		g_pLastMiniProfiler = m_pLastProfiler;
#endif
	}
};

class CMiniProfilerAntiGuard: public CMiniProfilerSample
{
#if ENABLE_HARDWARE_PROFILER
	CMiniProfiler *m_pProfiler;
#endif
public:
	CMiniProfilerAntiGuard(CMiniProfiler *pProfiler)
	{
#if ENABLE_HARDWARE_PROFILER
		m_pProfiler = pProfiler;
#endif
	}
	~CMiniProfilerAntiGuard()
	{
#if ENABLE_HARDWARE_PROFILER
		m_pProfiler->Add(-GetElapsed());
#endif
	}
};



class CLinkedMiniProfiler: public CMiniProfiler
{
public:
#if ENABLE_HARDWARE_PROFILER
	CLinkedMiniProfiler *m_pNext, **m_ppPrev;
	const char *m_szName;
	uint m_nHistoryMax, m_nHistoryLength;
	CMiniProfiler *m_pHistory;
	uint32 m_nFrameHistoryBegins;
#endif
public:
	CLinkedMiniProfiler(const char *szName, CLinkedMiniProfiler**ppList)
	{
#if ENABLE_HARDWARE_PROFILER
		m_nFrameHistoryBegins = 0;
		m_pNext = *ppList;
		m_ppPrev = ppList;
		*ppList = this;
		m_szName = szName;
		m_pHistory = NULL;
		m_nHistoryLength = 0;
		m_nHistoryMax = 0;
#endif
	}


	~CLinkedMiniProfiler()
	{
#if ENABLE_HARDWARE_PROFILER
		// We need to remove miniprofiler from the list properly. This is an issue because we unload DLLs sometimes. 
		*m_ppPrev = m_pNext; // that's it: we just remove this object from the list by linking previous object with the next
		if(m_pNext)
			m_pNext->m_ppPrev = m_ppPrev;
#endif
	}

	void Publish(uint nHistoryMax);
	void PurgeHistory();
};

#endif
