//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Real-Time Hierarchical Profiling
//
// $NoKeywords: $
//=============================================================================//

#ifndef VPROF_H
#define VPROF_H

#include "tier0/dbg.h"
#include "tier0/fasttimer.h"
#include "tier0/l2cache.h"
#include "tier0/threadtools.h"

// VProf is enabled by default in all configurations -except- X360 Retail.
#if !( defined( _X360 ) && defined( _CERT ) )
#define VPROF_ENABLED
#endif

#if defined(_X360) && defined(VPROF_ENABLED)
#include "tier0/pmc360.h"
#ifndef USE_PIX
#define VPROF_UNDO_PIX
#undef _PIX_H_
#undef PIXBeginNamedEvent
#undef PIXEndNamedEvent
#undef PIXSetMarker
#undef PIXNameThread
#define USE_PIX
#include <pix.h>
#undef USE_PIX
#else
#include <pix.h>
#endif
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif

// enable this to get detailed nodes beneath budget
// #define VPROF_LEVEL 1

// enable this to use pix (360 only)
// #define VPROF_PIX 1

#if defined(VPROF_PIX)
#pragma comment( lib, "Xapilibi" )
#endif

//-----------------------------------------------------------------------------
//
// Profiling instrumentation macros
//

#define MAXCOUNTERS 256


#ifdef VPROF_ENABLED

#define VPROF_VTUNE_GROUP

#define	VPROF( name )						VPROF_(name, 1, VPROF_BUDGETGROUP_OTHER_UNACCOUNTED, false, 0)
#define	VPROF_ASSERT_ACCOUNTED( name )		VPROF_(name, 1, VPROF_BUDGETGROUP_OTHER_UNACCOUNTED, true, 0)
#define	VPROF_( name, detail, group, bAssertAccounted, budgetFlags )		VPROF_##detail(name,group, bAssertAccounted, budgetFlags)

#define VPROF_BUDGET( name, group )					VPROF_BUDGET_FLAGS(name, group, BUDGETFLAG_OTHER)
#define VPROF_BUDGET_FLAGS( name, group, flags )	VPROF_(name, 0, group, false, flags)

#define VPROF_SCOPE_BEGIN( tag )	do { VPROF( tag )
#define VPROF_SCOPE_END()			} while (0)

#define VPROF_ONLY( expression )	expression

#define VPROF_ENTER_SCOPE( name )			g_VProfCurrentProfile.EnterScope( name, false, VProfBudgetGroupCallSite { VPROF_BUDGETGROUP_OTHER_UNACCOUNTED, 0 }, { __FILE__, __LINE__, __func__ } )
#define VPROF_EXIT_SCOPE()					g_VProfCurrentProfile.ExitScope()

#define VPROF_BUDGET_GROUP_ID_UNACCOUNTED 0


// Budgetgroup flags. These are used with VPROF_BUDGET_FLAGS.
// These control which budget panels the groups show up in.
// If a budget group uses VPROF_BUDGET, it gets the default 
// which is BUDGETFLAG_OTHER.
#define BUDGETFLAG_CLIENT	(1<<0)		// Shows up in the client panel.
#define BUDGETFLAG_SERVER	(1<<1)		// Shows up in the server panel.
#define BUDGETFLAG_OTHER	(1<<2)		// Unclassified (the client shows these but the dedicated server doesn't).
#define BUDGETFLAG_HIDDEN	(1<<15)
#define BUDGETFLAG_ALL		0xFFFF


// NOTE: You can use strings instead of these defines. . they are defined here and added
// in vprof.cpp so that they are always in the same order.
#define VPROF_BUDGETGROUP_OTHER_UNACCOUNTED			_T("Unaccounted")
#define VPROF_BUDGETGROUP_WORLD_RENDERING			_T("World Rendering")
#define VPROF_BUDGETGROUP_DISPLACEMENT_RENDERING	_T("Displacement_Rendering")
#define VPROF_BUDGETGROUP_GAME						_T("Game")
#define VPROF_BUDGETGROUP_NPCS						_T("NPCs")
#define VPROF_BUDGETGROUP_SERVER_ANIM				_T("Server Animation")
#define VPROF_BUDGETGROUP_PHYSICS					_T("Physics")
#define VPROF_BUDGETGROUP_STATICPROP_RENDERING		_T("Static_Prop_Rendering")
#define VPROF_BUDGETGROUP_MODEL_RENDERING			_T("Other_Model_Rendering")
#define VPROF_BUDGETGROUP_MODEL_FAST_PATH_RENDERING _T("Fast Path Model Rendering")
#define VPROF_BUDGETGROUP_BRUSHMODEL_RENDERING		_T("Brush_Model_Rendering")
#define VPROF_BUDGETGROUP_SHADOW_RENDERING			_T("Shadow_Rendering")
#define VPROF_BUDGETGROUP_DETAILPROP_RENDERING		_T("Detail_Prop_Rendering")
#define VPROF_BUDGETGROUP_PARTICLE_RENDERING		_T("Particle/Effect_Rendering")
#define VPROF_BUDGETGROUP_ROPES						_T("Ropes")
#define VPROF_BUDGETGROUP_DLIGHT_RENDERING			_T("Dynamic_Light_Rendering")
#define VPROF_BUDGETGROUP_OTHER_NETWORKING			_T("Networking")
#define VPROF_BUDGETGROUP_CLIENT_ANIMATION			_T("Client_Animation")
#define VPROF_BUDGETGROUP_OTHER_SOUND				_T("Sound")
#define VPROF_BUDGETGROUP_OTHER_VGUI				_T("VGUI")
#define VPROF_BUDGETGROUP_OTHER_FILESYSTEM			_T("FileSystem")
#define VPROF_BUDGETGROUP_PREDICTION				_T("Prediction")
#define VPROF_BUDGETGROUP_INTERPOLATION				_T("Interpolation")
#define VPROF_BUDGETGROUP_SWAP_BUFFERS				_T("Swap_Buffers")
#define VPROF_BUDGETGROUP_PLAYER					_T("Player")
#define VPROF_BUDGETGROUP_OCCLUSION					_T("Occlusion")
#define VPROF_BUDGETGROUP_OVERLAYS					_T("Overlays")
#define VPROF_BUDGETGROUP_TOOLS						_T("Tools")
#define VPROF_BUDGETGROUP_LIGHTCACHE				_T("Light_Cache")
#define VPROF_BUDGETGROUP_DISP_HULLTRACES			_T("Displacement_Hull_Traces")
#define VPROF_BUDGETGROUP_TEXTURE_CACHE				_T("Texture_Cache")
#define VPROF_BUDGETGROUP_PARTICLE_SIMULATION		_T("Particle Simulation")
#define VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING	_T("Flashlight Shadows")
#define VPROF_BUDGETGROUP_CLIENT_SIM				_T("Client Simulation") // think functions, tempents, etc.
#define VPROF_BUDGETGROUP_STEAM						_T("Steam") 
#define VPROF_BUDGETGROUP_CVAR_FIND					_T("Cvar_Find") 
#define VPROF_BUDGETGROUP_CLIENTLEAFSYSTEM			_T("ClientLeafSystem")
#define VPROF_BUDGETGROUP_JOBS_COROUTINES			_T("Jobs/Coroutines")
	
#ifdef _X360
// update flags
#define VPROF_UPDATE_BUDGET				0x01	// send budget data every frame
#define VPROF_UPDATE_TEXTURE_GLOBAL		0x02	// send global texture data every frame
#define VPROF_UPDATE_TEXTURE_PERFRAME	0x04	// send perframe texture data every frame
#endif

//-------------------------------------

#ifndef VPROF_LEVEL
#define VPROF_LEVEL 0
#endif

#define	VPROF_0(name,group,assertAccounted,budgetFlags)	VProfScopeHelper<0, assertAccounted> vprofHelper_(name, group, budgetFlags, { __FILE__, __LINE__, __func__ });

#if VPROF_LEVEL > 0 
#define	VPROF_1(name,group,assertAccounted,budgetFlags)	VProfScopeHelper<1, assertAccounted> vprofHelper_(name, group, budgetFlags, { __FILE__, __LINE__, __func__ });
#else
#define	VPROF_1(name,group,assertAccounted,budgetFlags)	((void)0)
#endif

#if VPROF_LEVEL > 1 
#define	VPROF_2(name,group,assertAccounted,budgetFlags)	VProfScopeHelper<2, assertAccounted> vprofHelper_(name, group, budgetFlags, { __FILE__, __LINE__, __func__ });
#else
#define	VPROF_2(name,group,assertAccounted,budgetFlags)	((void)0)
#endif

#if VPROF_LEVEL > 2 
#define	VPROF_3(name,group,assertAccounted,budgetFlags)	VProfScopeHelper<3, assertAccounted> vprofHelper_(name, group, budgetFlags, { __FILE__, __LINE__, __func__ });
#else
#define	VPROF_3(name,group,assertAccounted,budgetFlags)	((void)0)
#endif

#if VPROF_LEVEL > 3 
#define	VPROF_4(name,group,assertAccounted,budgetFlags)	VProfScopeHelper<4, assertAccounted> vprofHelper_(name, group, budgetFlags, { __FILE__, __LINE__, __func__ });
#else
#define	VPROF_4(name,group,assertAccounted,budgetFlags)	((void)0)
#endif

//-------------------------------------

#ifdef _MSC_VER
#define VProfCode( code ) \
	if ( 0 ) \
		; \
	else \
	{ \
	VPROF( __FUNCTION__ ": " #code ); \
		code; \
	}
#else
#define VProfCode( code ) \
	if ( 0 ) \
		; \
	else \
	{ \
		VPROF( #code ); \
		code; \
	} 
#endif


//-------------------------------------

#define VPROF_INCREMENT_COUNTER(name,amount)			do { static CVProfCounter _counter( name ); _counter.Increment( amount ); } while( 0 )
#define VPROF_INCREMENT_GROUP_COUNTER(name,group,amount)			do { static CVProfCounter _counter( name, group ); _counter.Increment( amount ); } while( 0 )

#else

#define	VPROF( name )									((void)0)
#define	VPROF_ASSERT_ACCOUNTED( name )					((void)0)
#define	VPROF_( name, detail, group, bAssertAccounted, budgetFlags )	((void)0)
#define VPROF_BUDGET( name, group )						((void)0)
#define VPROF_BUDGET_FLAGS( name, group, flags )		((void)0)

#define VPROF_SCOPE_BEGIN( tag )	do {
#define VPROF_SCOPE_END()			} while (0)

#define VPROF_ONLY( expression )	((void)0)

#define VPROF_ENTER_SCOPE( name )
#define VPROF_EXIT_SCOPE()

#define VPROF_INCREMENT_COUNTER(name,amount)			((void)0)
#define VPROF_INCREMENT_GROUP_COUNTER(name,group,amount)	((void)0)

#define VPROF_TEST_SPIKE( msec )	((void)0)

#define VProfCode( code ) code

#endif
 
//-----------------------------------------------------------------------------

#ifdef VPROF_ENABLED

class CUtlSourceLocation
{
public:
	CUtlSourceLocation( const char *file_path, uint64 file_line, const char *method_name ) :
		m_FilePath( file_path ), m_FileLine( file_line ), m_MethodName( method_name )
	{}

private:
	const char *m_FilePath;
	uint64 m_FileLine;
	const char *m_MethodName;
};

class VProfReportSettings_t;
class CVProfSummingContext;
struct VProfBudgetGroupCallSite;

//-----------------------------------------------------------------------------
//
// A node in the call graph hierarchy
//

class PLATFORM_CLASS CVProfNode 
{
	friend class CVProfRecorder;
	friend class CVProfile;

public:
	CVProfNode( const char *pszName, CVProfNode *pParent, VProfBudgetGroupCallSite &pBudgetGroupName, const CUtlSourceLocation &location );
	~CVProfNode();
	
	CVProfNode &operator=( const CVProfNode & );

	CVProfNode *GetVParent();
	const CVProfNode *GetVParent() const;
	CVProfNode *GetVSibling();
	const CVProfNode *GetVPrevSibling() const;
	const CVProfNode *GetVSibling() const;
	CVProfNode *GetVChild();
	const CVProfNode *GetVChild() const;
	
	void MarkFrame();
	void ResetPeak();
	
	void Pause();
	void Resume();
	void Reset();

	void EnterScope();
	bool ExitScope();

	CVProfNode *FindOrCreateChild( const char *, VProfBudgetGroupCallSite &, const CUtlSourceLocation & );

	const char *GetName() const;

	// Only used by the record/playback stuff.
	void SetBudgetGroupID( int id );
	int GetBudgetGroupID() const;

	int	GetCurCalls() const;
	double GetCurTime() const;
	int GetPrevCalls() const;
	double GetPrevTime() const;
	int	GetTotalCalls() const;
	double GetTotalTime() const;
	double GetPeakTime() const;

	const CUtlSourceLocation &GetSourceLocation() const;

	double GetCurTimeLessChildren() const;
	double GetPrevTimeLessChildren() const;
	double GetTotalTimeLessChildren() const;

	void ClearPrevTime();

	// Not used in the common case...
	void SetCurFrameTime( unsigned long milliseconds );
	
	void SetClientData( int iClientData );
	int GetClientData() const;

#ifdef DBGFLAG_VALIDATE
	void Validate( CValidator &validator, tchar *pchName );		// Validate our internal structures
#endif // DBGFLAG_VALIDATE


// Used by vprof record/playback.
private:
	CVProfNode( const char *, VProfBudgetGroupCallSite &, double, const CUtlSourceLocation & );

	void SetUniqueNodeID( int id );
	int GetUniqueNodeID() const;

	static int s_iCurrentUniqueNodeID;
private:
	const tchar *m_pszName;
	CFastTimer	m_Timer;

	// L2 Cache data.
	int			m_iPrevL2CacheMiss;
	int			m_iCurL2CacheMiss;
	int			m_iTotalL2CacheMiss;

#ifndef _X360	
	// L2 Cache data.
	CL2Cache	m_L2Cache;
#else // 360:
	
	unsigned int m_iBitFlags; // see enum below for settings
	CPMCData	m_PMCData;
	int			m_iPrevLoadHitStores;
	int			m_iCurLoadHitStores;
	int			m_iTotalLoadHitStores;

	public:
	enum FlagBits
	{
		kRecordL2 = 0x01,
		kCPUTrace = 0x02, ///< cause a PIX trace inside this node.
	};
	// call w/ true to enable L2 and LHS recording; false to turn it off
	inline void EnableL2andLHS(bool enable)
	{
		if (enable)
			m_iBitFlags |= kRecordL2;
		else
			m_iBitFlags &= (~kRecordL2);
	}

	inline bool IsL2andLHSEnabled( void )
	{
		return (m_iBitFlags & kRecordL2) != 0;
	}

	int GetLoadHitStores();

	private:
	
#endif

	int			m_nRecursions;
	
	unsigned	m_nCurFrameCalls;
	CCycleCount	m_CurFrameTime;
	
	unsigned	m_nPrevFrameCalls;
	CCycleCount	m_PrevFrameTime;

	unsigned	m_nTotalCalls;
	CCycleCount	m_TotalTime;

	CCycleCount	m_PeakTime;

	CVProfNode *m_pParent;
	CVProfNode *m_pChild;
	CVProfNode *m_pSibling;

	int m_BudgetGroupID;
	
	int m_iClientData;
	int m_iUniqueNodeID;
};

//-----------------------------------------------------------------------------
//
// Coordinator and root node of the profile hierarchy tree
//

enum VProfReportType_t
{
	VPRT_SUMMARY									= ( 1 << 0 ),
	VPRT_HIERARCHY									= ( 1 << 1 ),
	VPRT_HIERARCHY_TIME_PER_FRAME_AND_COUNT_ONLY	= ( 1 << 2 ),
	VPRT_LIST_BY_TIME								= ( 1 << 3 ),
	VPRT_LIST_BY_TIME_LESS_CHILDREN					= ( 1 << 4 ),
	VPRT_LIST_BY_AVG_TIME							= ( 1 << 5 ),	
	VPRT_LIST_BY_AVG_TIME_LESS_CHILDREN				= ( 1 << 6 ),
	VPRT_LIST_BY_PEAK_TIME							= ( 1 << 7 ),
	VPRT_LIST_BY_PEAK_OVER_AVERAGE					= ( 1 << 8 ),
	VPRT_LIST_TOP_ITEMS_ONLY						= ( 1 << 9 ),

	VPRT_FULL = (0xffffffff & ~(VPRT_HIERARCHY_TIME_PER_FRAME_AND_COUNT_ONLY|VPRT_LIST_TOP_ITEMS_ONLY)),
};

enum CounterGroup_t
{
	COUNTER_GROUP_DEFAULT=0,
	COUNTER_GROUP_NO_RESET,				// The engine doesn't reset these counters. Usually, they are used 
										// like global variables that can be accessed across modules.
	COUNTER_GROUP_TEXTURE_GLOBAL,		// Global texture usage counters (totals for what is currently in memory).
	COUNTER_GROUP_TEXTURE_PER_FRAME		// Per-frame texture usage counters.
}; 

class PLATFORM_CLASS CVProfile 
{
public:
	CVProfile();
	~CVProfile();

	int AssignNextTimespanId();
	
	void Start();
	void Stop();
	void Term();

	void SetTargetThreadId( unsigned id );
	uint32 GetTargetThreadId();
	bool InTargetThread();

	void CalcBudgetGroupTimes_Recursive( CVProfNode *, unsigned int *, int, float );

	void EnterScope( const char *pszName, bool bAssertAccounted, VProfBudgetGroupCallSite &pBudgetGroupName, const CUtlSourceLocation &location );
	void EnterScopeChecked( const char *pszName, bool bAssertAccounted, VProfBudgetGroupCallSite &pBudgetGroupName, const CUtlSourceLocation &location );
	void EnterScopeNode( CVProfNode *node );
	void ExitScope();
	void ExitScopeChecked();

	void MarkFrame();
	void ResetPeaks();
	
	void Pause();
	void Resume();
	void Reset();
	
	bool IsEnabled() const;

	bool AtRoot() const;

	//
	// Queries
	//

#ifdef VPROF_VTUNE_GROUP
#	define MAX_GROUP_STACK_DEPTH 1024

	void EnableVTuneGroup( const char *pGroupName );
	void DisableVTuneGroup( void );
	
	void PushGroup( int nGroupID );
	void PopGroup( void );
#endif
	
	int NumFramesSampled() const;
	double GetPeakFrameTime() const;
	double GetTotalTimeSampled() const;
	double GetTimeLastFrame() const;
	double GetTotalSecondsSampled() const;
	double GetTotalWallClockSecondsSampled();
	
	void SetOutputStream( void (*)(const char *, ...) );

	CVProfNode *GetRoot();
	CVProfNode *FindNode( CVProfNode *pStartNode, const char *pszNode );

	void OutputReport( const VProfReportSettings_t &, const char *pszStartNode, int budgetGroupID = -1 );
	void OutputReport( const char *pszStartNode = NULL, int budgetGroupID = -1 );

	static int GetNumBudgetGroups( void );
	static const char *GetBudgetGroupName( int budgetGroupID );
	static int GetBudgetGroupFlags( int budgetGroupID );	// Returns a combination of BUDGETFLAG_ defines.
	static void GetBudgetGroupColor( int budgetGroupID, int &r, int &g, int &b, int &a );

	static int BudgetGroupNameToBudgetGroupID( const char *pBudgetGroupName );
	static int BudgetGroupNameToBudgetGroupID( const char *pBudgetGroupName, int budgetFlagsToORIn );
	static int BudgetGroupNameToBudgetGroupIDNoCreate( const char *pBudgetGroupName );

	void HideBudgetGroup( int budgetGroupID, bool bHide = true );
	void HideBudgetGroup( const char *pszName, bool bHide = true );

	uint64_t *FindOrCreateCounter( const char *pName, CounterGroup_t eCounterGroup = COUNTER_GROUP_DEFAULT );
	void ResetCounters( CounterGroup_t eCounterGroup );
	
	int GetNumCounters( void ) const;
	
	uint64_t GetCounterValue( int index ) const;
	const char *GetCounterName( int index ) const;
	const char *GetCounterNameAndValue( int index, uint64_t &val ) const;
	CounterGroup_t GetCounterGroup( int index ) const;

	CVProfNode *GetCurrentNode();

#ifdef DBGFLAG_VALIDATE
	void Validate( CValidator &validator, tchar *pchName );		// Validate our internal structures
#endif // DBGFLAG_VALIDATE

protected:

	void FreeNodes_R( CVProfNode *pNode );

#ifdef VPROF_VTUNE_GROUP
	bool VTuneGroupEnabled();
	int VTuneGroupID();
#endif

	void SumTimes( const char *pszStartNode, int budgetGroupID, CVProfSummingContext & );
	void SumTimes( CVProfNode *pNode, int budgetGroupID, CVProfSummingContext & );
	void DumpNodes( const VProfReportSettings_t &, const CVProfNode *pNode, int indent, bool bAverageAndCountOnly, const CVProfSummingContext &, double, double, bool );
	static int FindBudgetGroupName( const char *pBudgetGroupName );
	static int AddBudgetGroupName( const char *pBudgetGroupName, int budgetFlags );

#ifdef VPROF_VTUNE_GROUP
	bool		m_bVTuneGroupEnabled;
	int			m_nVTuneGroupID;
	int			m_GroupIDStack[MAX_GROUP_STACK_DEPTH];
	int			m_GroupIDStackDepth;
#endif
	int 		m_enabled;
	bool		m_fAtRoot; // tracked for efficiency of the "not profiling" case
	CVProfNode *m_pCurNode;
	CVProfNode	m_Root;
	int			m_nFrames;
	int			m_ProfileDetailLevel;
	int			m_pausedEnabledDepth;

	class CBudgetGroup
	{
	public:
		tchar *m_pName;
		int m_BudgetFlags;
	};
	
	CBudgetGroup	*m_pBudgetGroups;
	int			m_nBudgetGroupNamesAllocated;
	int			m_nBudgetGroupNames;
	void		(*m_pNumBudgetGroupsChangedCallBack)(void);

	// Performance monitoring events.
	bool		m_bPMEInit;
	bool		m_bPMEEnabled;

	int m_Counters[MAXCOUNTERS];
	char m_CounterGroups[MAXCOUNTERS]; // (These are CounterGroup_t's).
	tchar *m_CounterNames[MAXCOUNTERS];
	int m_NumCounters;

#ifdef _X360
	int						m_UpdateMode;
	CPUTraceState			m_iCPUTraceEnabled;
	int						m_nFramesRemaining;
	int						m_nFrameCount;
	int64					m_WorstCycles;
	char					m_WorstTraceFilename[128];
	char					m_CPUTraceFilename[128];
	unsigned int			m_iSuccessiveTraceIndex;
	VXConsoleReportMode_t	m_ReportMode;
	float					m_pReportScale[VXCONSOLE_REPORT_COUNT];
	bool					m_bTraceCompleteEvent;
#endif

	unsigned m_TargetThreadId;
};

//-------------------------------------

PLATFORM_INTERFACE CVProfile g_VProfCurrentProfile;


//-----------------------------------------------------------------------------

PLATFORM_INTERFACE bool g_VProfSignalSpike;

class CVProfSpikeDetector
{
public:
	CVProfSpikeDetector( float spike ) :
		m_timeLast( GetTimeLast() )
	{
		m_spike = spike;
		m_Timer.Start();
	}

	~CVProfSpikeDetector()
	{
		m_Timer.End();
		if ( Plat_FloatTime() - m_timeLast > 2.0 )
		{
			m_timeLast = Plat_FloatTime();
			if ( m_Timer.GetDuration().GetMillisecondsF() > m_spike )
			{
				g_VProfSignalSpike = true;
			}
		}
	}

private:
	static float &GetTimeLast() { static float timeLast = 0; return timeLast; }
	CFastTimer	m_Timer;
	float m_spike;
	float &m_timeLast;
};


// Macro to signal a local spike. Meant as temporary instrumentation, do not leave in code
#define VPROF_TEST_SPIKE( msec ) CVProfSpikeDetector UNIQUE_ID( msec )

//-----------------------------------------------------------------------------

typedef void (*VProfExitScopeCB)();

struct VProfBudgetGroupCallSite
{
	VProfBudgetGroupCallSite( const char *group_name, int flags, int group_id = VPROF_BUDGET_GROUP_ID_UNACCOUNTED ) :
		m_pGroupName( group_name ), m_nFlags( flags ), m_nGroupId( group_id ) { }

	const char *m_pGroupName;
	int m_nFlags;
	int m_nGroupId;
};

template <int DETAIL_LEVEL, bool ASSERT_ACCOUNTED>
class VProfScopeHelper
{
public:
	VProfScopeHelper( const char *pszName, const CUtlSourceLocation &location )
	{
		m_pExitScope = EnterScopeInternal( pszName, location );
	}

	VProfScopeHelper( const char *pszName, const char *pszGroupName, int nFlags, const CUtlSourceLocation &location )
	{
		VProfBudgetGroupCallSite budget_group( pszGroupName, nFlags );
		m_pExitScope = EnterScopeInternalBudgetFlags( pszName, budget_group, location );
	}

	VProfScopeHelper( VProfScopeHelper &other )
	{
		m_pExitScope = other.m_pExitScope;
	}

	~VProfScopeHelper()
	{
		ExitScope();
	}

	void ExitScope()
	{
		if(!m_pExitScope)
			return;

		m_pExitScope();
		m_pExitScope = nullptr;
	}

	DLL_CLASS_IMPORT VProfScopeHelper &operator=( const VProfScopeHelper &other );
	DLL_CLASS_IMPORT VProfScopeHelper &operator=( VProfScopeHelper &&other );

	static DLL_CLASS_IMPORT VProfExitScopeCB EnterScopeInternal( const char *pszName, const CUtlSourceLocation &location );
	static DLL_CLASS_IMPORT VProfExitScopeCB EnterScopeInternalBudgetFlags( const char *pszName, VProfBudgetGroupCallSite &budgetGroup, const CUtlSourceLocation &location );

private:
	VProfExitScopeCB m_pExitScope = nullptr;
};

//-----------------------------------------------------------------------------

class CVProfCounter
{
public:
	CVProfCounter( const char *pName, CounterGroup_t group = COUNTER_GROUP_DEFAULT )
	{
		m_pCounter = g_VProfCurrentProfile.FindOrCreateCounter( pName, group );
		Assert( m_pCounter );
	}
	~CVProfCounter()
	{
	}
	void Increment( uint64_t val )
	{ 
		Assert( m_pCounter );
		*m_pCounter += val; 
	}
private:
	uint64_t *m_pCounter;
};

#endif

#ifdef _X360

#include "xbox/xbox_console.h"
#include "tracerecording.h"
#include  "tier1/fmtstr.h"
#pragma comment( lib, "tracerecording.lib" )
#pragma comment( lib, "xbdm.lib" )

class CPIXRecorder
{
public:
	CPIXRecorder() : m_bActive( false ) {}
	~CPIXRecorder() { Stop(); }

	void Start( const char *pszFilename = "capture" )
	{
		if ( !m_bActive )
		{
			if ( !XTraceStartRecording( CFmtStr( "e:\\%s.pix2", pszFilename ) ) )
			{
				Msg( "XTraceStartRecording failed, error code %d\n", GetLastError() );
			}
			else
			{
				m_bActive = true;
			}
		}
	}

	void Stop()
	{
		if ( m_bActive )
		{
			m_bActive = false;
			if ( XTraceStopRecording() )
			{
				Msg( "CPU trace finished.\n" );
				// signal VXConsole that trace is completed
				XBX_rTraceComplete();
			}
		}
	}

private:
	bool m_bActive;
};

#define VPROF_BEGIN_PIX_BLOCK( convar ) \
	{ \
	bool bRunPix = 0; \
	static CFastTimer PIXTimer; \
	extern ConVar convar; \
	ConVar &PIXConvar = convar; \
	CPIXRecorder PIXRecorder; \
		{ \
		PIXLabel: \
			if ( bRunPix ) \
			{ \
				PIXRecorder.Start(); \
			} \
			else \
			{ \
				if ( PIXConvar.GetBool() ) \
				{ \
					PIXTimer.Start(); \
				} \
			} \
				{


#define VPROF_END_PIX_BLOCK() \
				} \
			\
			if ( !bRunPix ) \
			{ \
				if ( PIXConvar.GetBool() ) \
				{ \
					PIXTimer.End(); \
					if ( PIXTimer.GetDuration().GetMillisecondsF() > PIXConvar.GetFloat() ) \
					{ \
						PIXConvar.SetValue( 0 ); \
						bRunPix = true; \
						goto PIXLabel; \
					} \
				} \
			} \
			else \
			{ \
				PIXRecorder.Stop(); \
			} \
		} \
	}
#else
#define VPROF_BEGIN_PIX_BLOCK( PIXConvar ) {
#define VPROF_END_PIX_BLOCK() }
#endif


#ifdef VPROF_UNDO_PIX
#undef USE_PIX
#undef _PIX_H_
#undef PIXBeginNamedEvent
#undef PIXEndNamedEvent
#undef PIXSetMarker
#undef PIXNameThread
#include <pix.h>
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif

//=============================================================================
