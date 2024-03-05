//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: This header should never be used directly from leaf code!!!
// Instead, just add the file memoverride.cpp into your project and all this
// will automagically be used
//
// $NoKeywords: $
//=============================================================================//

#ifndef TIER0_MEMALLOC_H
#define TIER0_MEMALLOC_H

#ifdef _WIN32
#pragma once
#endif

// These memory debugging switches aren't relevant under Linux builds since memoverride.cpp
// isn't built into Linux projects
#ifndef POSIX
// Define this in release to get memory tracking even in release builds
//#define USE_MEM_DEBUG 1

// Define this in release to get light memory debugging
//#define USE_LIGHT_MEM_DEBUG

// Define this to require -uselmd to turn light memory debugging on
//#define LIGHT_MEM_DEBUG_REQUIRES_CMD_LINE_SWITCH
#endif

#if defined( _MEMTEST )
#ifdef _WIN32
#define USE_MEM_DEBUG 1
#endif
#endif

// Undefine this if using a compiler lacking threadsafe RTTI (like vc6)
#define MEM_DEBUG_CLASSNAME 1

// GAMMACASE: This forces any debug usage to be disabled, due to the IMemAlloc structure being incorrect
// which causes random crashes if used! Added until proper solution is made.
#define PREVENT_DEBUG_USAGE

#include "platform.h"
#include <stddef.h>
#if defined( OSX )
#include <malloc/malloc.h>
#endif

#ifdef TIER0_DLL_EXPORT
#  define MEM_INTERFACE DLL_EXPORT
#else
#  define MEM_INTERFACE DLL_IMPORT
#endif

#if !defined(STEAM) && !defined(NO_MALLOC_OVERRIDE)

#define MEMALLOC_REGION_ALLOC_1		'4'
#define MEMALLOC_REGION_ALLOC_2		'6'
#define MEMALLOC_REGION_ALLOC_3		'8'
#define MEMALLOC_REGION_ALLOC_4		':'
#define MEMALLOC_REGION_ALLOC_5		'<'
#define MEMALLOC_REGION_ALLOC_6		'>'

#define MEMALLOC_REGION_FREE_1		'5'
#define MEMALLOC_REGION_FREE_2		'7'
#define MEMALLOC_REGION_FREE_3		'9'
#define MEMALLOC_REGION_FREE_4		';'
#define MEMALLOC_REGION_FREE_5		'='
#define MEMALLOC_REGION_FREE_6		'?'

enum MemAllocAttribute_t
{
	MemAllocAttribute_Unk0 = 0,
	MemAllocAttribute_Unk1 = 1,
	MemAllocAttribute_Unk2 = 2
};

enum MemoryState
{
	MemoryState_UnexpectedlyAllocated = 0,
	MemoryState_UnexpectedlyFreed = 1,
	MemoryState_UnexpectedlyUnrecognized = 2,
	MemoryState_Corrupt = 3,
	MemoryState_Invalid = 4,
	MemoryState_Operational = 5,
	MemoryState_Unknown = 6
};

enum MemoryDebugType : int16
{
	MemoryDebugType_None = 0,
	MemoryDebugType_Light = 1,
	MemoryDebugType_Full = 2
};

struct MemoryInfoState
{
	MemoryInfoState( MemoryDebugType type, bool debug = false ) : m_MemType( type ), m_bDebug( debug ) { }

	MemoryDebugType m_MemType;
	bool m_bDebug;
};

struct _CrtMemState;
class CBufferString;

#define MEMALLOC_VERSION 1

typedef size_t (*MemAllocFailHandler_t)( size_t );

PLATFORM_INTERFACE void CMemAllocSystemInitialize();

//-----------------------------------------------------------------------------
// NOTE! This should never be called directly from leaf code
// Just use new,delete,malloc,free etc. They will call into this eventually
//-----------------------------------------------------------------------------
abstract_class IMemAlloc
{
	// AMNOTE: A lot of functions in here might be stubbed out and not do what their description tells
	// this heavily depends on the allocator implementation and should be taken into account when using this directly!

private:
	virtual ~IMemAlloc() = 0;

	// Release versions
	virtual void *Alloc( size_t nSize ) = 0;
public:
	virtual void *Realloc( void *pMem, size_t nSize ) = 0;
	virtual void Free( void *pMem ) = 0;

private:
	virtual void *AllocAligned( size_t nSize, size_t align ) = 0;
public:
	virtual void *ReallocAligned( void *pMem, size_t nSize, size_t align ) = 0;
	virtual void FreeAligned( void *pMem ) = 0;

	inline void *IndirectAlloc( size_t nSize ) { return Alloc( nSize ); }
	inline void *IndirectAllocAligned( size_t nSize, size_t align ) { return AllocAligned( nSize, align ); }

	// AMNOTE: It's unclear if the functions below are actually a debug variants, as in binaries they are
	// absolutely the same to the above functions, but with a different name as they aren't merged together
	// in the vtable in the win binaries. So it's mostly a guess, same goes to their params.
	// ===============================================================
private:
	virtual void *AllocDbg( size_t nSize ) = 0;
public:
	virtual void *ReallocDbg( void *pMem, size_t nSize ) = 0;
	virtual void FreeDbg( void *pMem ) = 0;

private:
	virtual void *AllocAlignedDbg( size_t nSize, size_t align ) = 0;
public:
	virtual void *ReallocAlignedDbg( void *pMem, size_t nSize, size_t align ) = 0;
	virtual void FreeAlignedDbg( void *pMem ) = 0;
	// ===============================================================

	// Region-based allocations
	// Use MEMALLOC_REGION_* defines for the region arg
	// AMNOTE: Region name is mostly a guess!
	virtual void *RegionAlloc( uint8 region, size_t nSize ) = 0;
	virtual void RegionFree( uint8 region, void *pMem ) = 0;

	virtual void *RegionAllocAligned( uint8 region, size_t nSize, size_t align ) = 0;
	virtual void RegionFreeAligned( uint8 region, void *pMem ) = 0;

	// Returns size of a particular allocation
	virtual size_t GetSize( void *pMem ) = 0;
	virtual size_t GetSizeAligned( void *pMem ) = 0;

	// If out arg is NULL or "<stdout>" it would output all the info to console,
	// otherwise a file with that name would be created at the game root folder
	virtual void DumpStats( const char *out_path ) = 0;

	// AMNOTE: Stub, returns -1
	virtual int unk001() = 0;

	// AMNOTE: Stub
	virtual void unk002() = 0;

	// AMNOTE: Stub, returns false and writes -1 to the ret_out
	virtual bool unk003( int *ret_out ) = 0;

	// AMNOTE: Stub, returns false
	virtual bool unk004() = 0;

	// AMNOTE: Stub
	virtual void unk005() = 0;

	virtual void CompactOnFail() = 0;

	// Logs the out of memory message, breaks in the debugger and exits the process
	virtual void ReportFailedAllocation( int nSize ) = 0;

	// memset's at the pMem location with nSize bytes of specified type, where:
	// MemoryDebugType_None - would do nothing;
	// MemoryDebugType_Light - would memset with 0xDD bytes;
	// MemoryDebugType_Full - would memset with 0xD8 bytes;
	// the input pMem is returned
	// 
	// NOTE: This would do nothing if the allocator is not in the debug memory mode or if
	// state has the m_bDebug set to false
	virtual void *MemSetDbg( void *pMem, size_t nSize, MemoryInfoState state ) = 0;

	// Returns true if the underlying allocator is using DebugMemoryType different to DebugMemoryType::DebugMemoryType_None
	// would also return true if the state argument has m_bDebug set as true
	virtual bool IsInDebugMode( MemoryInfoState state ) = 0;

	// If memory is not in the debug mode, returns either MemoryState_Invalid or MemoryState_Operational
	// Otherwise a more deep check would be performed
	virtual MemoryState GetMemoryState( void *pMem ) = 0;

	// Logs a warning and breaks in a debugger if the pMem state doesn't match provided state
	virtual void ReportBadMemory( void *pMem, MemoryState state = MemoryState_Operational ) = 0;

	// Returns memory debug type of this allocator
	virtual MemoryDebugType GetMemoryDebugType() = 0;

	// Returns previous total allocation size that was used
	// Directly limits how much bytes could be allocated with (Re)Alloc* functions
	virtual size_t SetTotalAllocationSize( size_t new_total_alloc_size ) = 0;

	// Returns previous allocation limit size that was used
	// Directly limits how much bytes could be allocated with (Re)Alloc* functions
	virtual size_t SetAllocationLimitSize( size_t new_alloc_limit_size ) = 0;

	// Writes detailed info about this allocator and settings used
	// Example output of a non debug allocator:
	// Heap: standard allocator pass-through to low-level
	// Low - level allocator : jemalloc
	//
	// Example output of a debug allocator with custom settings:
	// Heap: standard allocator + mem init + stackstats + light verifier + full mem debug
	// Low - level allocator : jemalloc
	virtual void GetAllocatorDescription( CBufferString &buf ) = 0;

	// Returns true if stackstats is enabled and -memstackstats_disable_pools launch option was used
	virtual bool IsStackStatsPoolsDisabled() = 0;

	// Returns true if stackstats is enabled for this allocator
	virtual bool IsStackStatsEnabled() = 0;

	// AMNOTE: Stub, returns 0
	virtual int unk101() = 0;

	// AMNOTE: Stub
	virtual void unk102( void *pMem, MemAllocAttribute_t alloc_attribute, int unk ) = 0;

	// AMNOTE: Copies data to an unknown struct of byte size 56
	// Returns true if data was written, false otherwise
	virtual bool unk103( void *out_val ) = 0;

	// Calls the lower-level allocator functions directly
	virtual void *AllocRaw( size_t nSize ) = 0;
	virtual void *ReallocRaw( void *pMem, size_t nSize ) = 0;
	virtual void FreeRaw( void *pMem ) = 0;
	virtual size_t GetSizeRaw( void *pMem ) = 0;
};

//-----------------------------------------------------------------------------
// Singleton interface
//-----------------------------------------------------------------------------
MEM_INTERFACE IMemAlloc *g_pMemAlloc;

//-----------------------------------------------------------------------------

#ifdef MEMALLOC_REGIONS
#ifndef MEMALLOC_REGION
#define MEMALLOC_REGION 0
#endif
inline void *MemAlloc_Alloc( size_t nSize )
{ 
	return g_pMemAlloc->RegionAlloc( MEMALLOC_REGION, nSize );
}

inline void *MemAlloc_Alloc( size_t nSize, const char *pFileName, int nLine )
{ 
	return g_pMemAlloc->RegionAlloc( MEMALLOC_REGION, nSize, pFileName, nLine );
}
#else
#undef MEMALLOC_REGION
inline void *MemAlloc_Alloc( size_t nSize )
{ 
	return g_pMemAlloc->IndirectAlloc( nSize );
}

inline void *MemAlloc_Alloc( size_t nSize, const char *pFileName, int nLine )
{ 
	return g_pMemAlloc->IndirectAlloc( nSize /*, pFileName, nLine*/ );
}
#endif
inline void MemAlloc_Free( void *ptr )
{
	g_pMemAlloc->Free( ptr );
}
inline void MemAlloc_Free( void *ptr, const char *pFileName, int nLine )
{
	g_pMemAlloc->Free( ptr /*, pFileName, nLine*/ );
}

//-----------------------------------------------------------------------------

#ifdef MEMALLOC_REGIONS
#else
#endif


inline bool ValueIsPowerOfTwo( size_t value )			// don't clash with mathlib definition
{
	return (value & ( value - 1 )) == 0;
}


inline void *MemAlloc_AllocAlignedUnattributed( size_t size, size_t align )
{
	return g_pMemAlloc->IndirectAllocAligned( size, align );
}

inline void *MemAlloc_AllocAlignedFileLine( size_t size, size_t align, const char *pszFile, int nLine )
{
	return g_pMemAlloc->IndirectAllocAligned( size, align /*, pszFile, nLine*/ );
}

#ifdef USE_MEM_DEBUG
#define MemAlloc_AllocAligned( s, a )	MemAlloc_AllocAlignedFileLine( s, a, __FILE__, __LINE__ )
#elif defined(USE_LIGHT_MEM_DEBUG)
extern const char *g_pszModule; 
#define MemAlloc_AllocAligned( s, a )	MemAlloc_AllocAlignedFileLine( s, a, g_pszModule, 0 )
#else
#define MemAlloc_AllocAligned( s, a )	MemAlloc_AllocAlignedUnattributed( s, a )
#endif

inline void *MemAlloc_ReallocAligned( void *ptr, size_t size, size_t align )
{
	return g_pMemAlloc->ReallocAligned( ptr, size, align );
}

inline void MemAlloc_FreeAligned( void *pMemBlock )
{
	g_pMemAlloc->FreeAligned( pMemBlock );
}

inline void MemAlloc_FreeAligned( void *pMemBlock, const char *pszFile, int nLine )
{
	g_pMemAlloc->FreeAligned( pMemBlock /*, pszFile, nLine*/ );
}

inline size_t MemAlloc_GetSizeAligned( void *pMemBlock )
{
	return g_pMemAlloc->GetSizeAligned( pMemBlock );
}

struct aligned_tmp_t
{
	// empty base class
};

// template here to allow adding alignment at levels of hierarchy that aren't the base
template< int bytesAlignment = 16, class T = aligned_tmp_t >
class CAlignedNewDelete : public T
{
public:
	void *operator new( size_t nSize )
	{
		return MemAlloc_AllocAligned( nSize, bytesAlignment );
	}

	void* operator new( size_t nSize, int nBlockUse, const char *pFileName, int nLine )
	{
		return MemAlloc_AllocAlignedFileLine( nSize, bytesAlignment, pFileName, nLine );
	}

	void operator delete(void *pData)
	{
		if ( pData )
		{
			MemAlloc_FreeAligned( pData );
		}
	}

	void operator delete( void* pData, int nBlockUse, const char *pFileName, int nLine )
	{
		if ( pData )
		{
			MemAlloc_FreeAligned( pData, pFileName, nLine );
		}
	}
};

//-----------------------------------------------------------------------------

#if (defined(_DEBUG) || defined(USE_MEM_DEBUG)) && !defined(PREVENT_DEBUG_USAGE)
#define MEM_ALLOC_CREDIT_(tag)	CMemAllocAttributeAlloction memAllocAttributeAlloction( tag, __LINE__ )
#define MemAlloc_PushAllocDbgInfo( pszFile, line ) g_pMemAlloc->PushAllocDbgInfo( pszFile, line )
#define MemAlloc_PopAllocDbgInfo() g_pMemAlloc->PopAllocDbgInfo()
#define MemAlloc_RegisterAllocation( pFileName, nLine, nLogicalSize, nActualSize, nTime ) g_pMemAlloc->RegisterAllocation( pFileName, nLine, nLogicalSize, nActualSize, nTime )
#define MemAlloc_RegisterDeallocation( pFileName, nLine, nLogicalSize, nActualSize, nTime ) g_pMemAlloc->RegisterDeallocation( pFileName, nLine, nLogicalSize, nActualSize, nTime )
#else
#define MEM_ALLOC_CREDIT_(tag)	((void)0)
#define MemAlloc_PushAllocDbgInfo( pszFile, line ) ((void)0)
#define MemAlloc_PopAllocDbgInfo() ((void)0)
#define MemAlloc_RegisterAllocation( pFileName, nLine, nLogicalSize, nActualSize, nTime ) ((void)0)
#define MemAlloc_RegisterDeallocation( pFileName, nLine, nLogicalSize, nActualSize, nTime ) ((void)0)
#endif

//-----------------------------------------------------------------------------

class CMemAllocAttributeAlloction
{
public:
	CMemAllocAttributeAlloction( const char *pszFile, int line ) 
	{
		MemAlloc_PushAllocDbgInfo( pszFile, line );
	}
	
	~CMemAllocAttributeAlloction()
	{
		MemAlloc_PopAllocDbgInfo();
	}
};

#define MEM_ALLOC_CREDIT()	MEM_ALLOC_CREDIT_(__FILE__)

//-----------------------------------------------------------------------------

#if defined(MSVC) && ( defined(_DEBUG) || defined(USE_MEM_DEBUG) ) && !defined(PREVENT_DEBUG_USAGE)

	#pragma warning(disable:4290)
	#pragma warning(push)
	#include <typeinfo.h>

	// MEM_DEBUG_CLASSNAME is opt-in.
	// Note: typeid().name() is not threadsafe, so if the project needs to access it in multiple threads
	// simultaneously, it'll need a mutex.
	#if defined(_CPPRTTI) && defined(MEM_DEBUG_CLASSNAME)

		template <typename T> const char *MemAllocClassName( T *p )
		{
			static const char *pszName = typeid(*p).name(); // @TODO: support having debug heap ignore certain allocations, and ignore memory allocated here [5/7/2009 tom]
			return pszName;
		}

		#define MEM_ALLOC_CREDIT_CLASS()	MEM_ALLOC_CREDIT_( MemAllocClassName( this ) )
		#define MEM_ALLOC_CLASSNAME(type) (typeid((type*)(0)).name())
	#else
		#define MEM_ALLOC_CREDIT_CLASS()	MEM_ALLOC_CREDIT_( __FILE__ )
		#define MEM_ALLOC_CLASSNAME(type) (__FILE__)
	#endif

	// MEM_ALLOC_CREDIT_FUNCTION is used when no this pointer is available ( inside 'new' overloads, for example )
	#ifdef _MSC_VER
		#define MEM_ALLOC_CREDIT_FUNCTION()		MEM_ALLOC_CREDIT_( __FUNCTION__ )
	#else
		#define MEM_ALLOC_CREDIT_FUNCTION() (__FILE__)
	#endif

	#pragma warning(pop)
#else
	#define MEM_ALLOC_CREDIT_CLASS()
	#define MEM_ALLOC_CLASSNAME(type) NULL
	#define MEM_ALLOC_CREDIT_FUNCTION() 
#endif

//-----------------------------------------------------------------------------

#if (defined(_DEBUG) || defined(USE_MEM_DEBUG)) && !defined(PREVENT_DEBUG_USAGE)
struct MemAllocFileLine_t
{
	const char *pszFile;
	int line;
};

#define MEMALLOC_DEFINE_EXTERNAL_TRACKING( tag ) \
	static CUtlMap<void *, MemAllocFileLine_t, int> s_##tag##Allocs( DefLessFunc( void *) ); \
	CUtlMap<void *, MemAllocFileLine_t, int> * g_p##tag##Allocs = &s_##tag##Allocs; \
	const char * g_psz##tag##Alloc = strcpy( (char *)MemAlloc_Alloc( strlen( #tag "Alloc" ) + 1, "intentional leak", 0 ), #tag "Alloc" );

#define MEMALLOC_DECLARE_EXTERNAL_TRACKING( tag ) \
	extern CUtlMap<void *, MemAllocFileLine_t, int> * g_p##tag##Allocs; \
	extern const char * g_psz##tag##Alloc;

#define MemAlloc_RegisterExternalAllocation( tag, p, size ) \
	if ( !p ) \
		; \
	else \
	{ \
		MemAllocFileLine_t fileLine = { g_psz##tag##Alloc, 0 }; \
		g_pMemAlloc->GetActualDbgInfo( fileLine.pszFile, fileLine.line ); \
		if ( fileLine.pszFile != g_psz##tag##Alloc ) \
		{ \
			g_p##tag##Allocs->Insert( p, fileLine ); \
		} \
		\
		MemAlloc_RegisterAllocation( fileLine.pszFile, fileLine.line, size, size, 0); \
	}

#define MemAlloc_RegisterExternalDeallocation( tag, p, size ) \
	if ( !p ) \
		; \
	else \
	{ \
		MemAllocFileLine_t fileLine = { g_psz##tag##Alloc, 0 }; \
		CUtlMap<void *, MemAllocFileLine_t, int>::IndexType_t iRecordedFileLine = g_p##tag##Allocs->Find( p ); \
		if ( iRecordedFileLine !=  g_p##tag##Allocs->InvalidIndex() ) \
		{ \
			fileLine = (*g_p##tag##Allocs)[iRecordedFileLine]; \
			g_p##tag##Allocs->RemoveAt( iRecordedFileLine ); \
		} \
		\
		MemAlloc_RegisterDeallocation( fileLine.pszFile, fileLine.line, size, size, 0); \
	}

#else

#define MEMALLOC_DEFINE_EXTERNAL_TRACKING( tag )
#define MEMALLOC_DECLARE_EXTERNAL_TRACKING( tag )
#define MemAlloc_RegisterExternalAllocation( tag, p, size ) ((void)0)
#define MemAlloc_RegisterExternalDeallocation( tag, p, size ) ((void)0)

#endif

//-----------------------------------------------------------------------------

#elif defined( POSIX )

#if defined( OSX )
// Mac always aligns allocs, don't need to call posix_memalign which doesn't exist in 10.5.8 which TF2 still needs to run on
//inline void *memalign(size_t alignment, size_t size) {void *pTmp=NULL; posix_memalign(&pTmp, alignment, size); return pTmp;}
inline void *memalign(size_t alignment, size_t size) {void *pTmp=NULL; pTmp = malloc(size); return pTmp;}
#endif

inline void *_aligned_malloc( size_t nSize, size_t align )															{ return memalign( align, nSize ); }
inline void _aligned_free( void *ptr )																				{ free( ptr ); }

inline void *MemAlloc_Alloc( size_t nSize, const char *pFileName = NULL, int nLine = 0 )							{ return malloc( nSize ); }
inline void MemAlloc_Free( void *ptr, const char *pFileName = NULL, int nLine = 0 )									{ free( ptr ); }

inline void *MemAlloc_AllocAligned( size_t size, size_t align, const char *pszFile = NULL, int nLine = 0  )	        { return memalign( align, size ); }
inline void *MemAlloc_AllocAlignedFileLine( size_t size, size_t align, const char *pszFile = NULL, int nLine = 0 )	{ return memalign( align, size ); }
inline void MemAlloc_FreeAligned( void *pMemBlock, const char *pszFile = NULL, int nLine = 0 ) 						{ free( pMemBlock ); }

#if defined( OSX )
inline size_t _msize( void *ptr )																					{ return malloc_size( ptr ); }
#else
inline size_t _msize( void *ptr )																					{ return malloc_usable_size( ptr ); }
#endif

inline void *MemAlloc_ReallocAligned( void *ptr, size_t size, size_t align )
{
	void *ptr_new_aligned = memalign( align, size );

	if( ptr_new_aligned )
	{
		size_t old_size = _msize( ptr );
		size_t copy_size = ( size < old_size ) ? size : old_size;

		memcpy( ptr_new_aligned, ptr, copy_size );
		free( ptr );
	}

	return ptr_new_aligned;
}
#else
#define MemAlloc_GetDebugInfoSize() g_pMemAlloc->GetDebugInfoSize()
#define MemAlloc_SaveDebugInfo( pvDebugInfo ) g_pMemAlloc->SaveDebugInfo( pvDebugInfo )
#define MemAlloc_RestoreDebugInfo( pvDebugInfo ) g_pMemAlloc->RestoreDebugInfo( pvDebugInfo )
#define MemAlloc_InitDebugInfo( pvDebugInfo, pchRootFileName, nLine ) g_pMemAlloc->InitDebugInfo( pvDebugInfo, pchRootFileName, nLine )

#endif // !STEAM && !NO_MALLOC_OVERRIDE

//-----------------------------------------------------------------------------

#if !defined(STEAM) && defined(NO_MALLOC_OVERRIDE)
#define MEM_ALLOC_CREDIT_(tag)	((void)0)
#define MEM_ALLOC_CREDIT()	MEM_ALLOC_CREDIT_(__FILE__)
#define MEM_ALLOC_CREDIT_FUNCTION()
#define MEM_ALLOC_CREDIT_CLASS()
#define MEM_ALLOC_CLASSNAME(type) NULL

#define MemAlloc_PushAllocDbgInfo( pszFile, line )
#define MemAlloc_PopAllocDbgInfo()

#define MEMALLOC_DEFINE_EXTERNAL_TRACKING( tag )
#define MemAlloc_RegisterExternalAllocation( tag, p, size ) ((void)0)
#define MemAlloc_RegisterExternalDeallocation( tag, p, size ) ((void)0)

#endif // !STEAM && NO_MALLOC_OVERRIDE

//-----------------------------------------------------------------------------



// linux memory tracking via hooks.
#ifdef _POSIX
PLATFORM_INTERFACE void MemoryLogMessage( char const *s );						// throw a message into the memory log
PLATFORM_INTERFACE void EnableMemoryLogging( bool bOnOff );
PLATFORM_INTERFACE void DumpMemoryLog( int nThresh );
PLATFORM_INTERFACE void DumpMemorySummary( void );
PLATFORM_INTERFACE void SetMemoryMark( void );
PLATFORM_INTERFACE void DumpChangedMemory( int nThresh );

// ApproximateProcessMemoryUsage returns the approximate memory footprint of this process.
PLATFORM_INTERFACE size_t ApproximateProcessMemoryUsage( void );
#else
inline void MemoryLogMessage( char const * )
{
}
inline void EnableMemoryLogging( bool )
{
}
inline void DumpMemoryLog( int )
{
}
inline void DumpMemorySummary( void )
{
}
inline void SetMemoryMark( void )
{
}
inline void DumpChangedMemory( int )
{
}
inline size_t ApproximateProcessMemoryUsage( void )
{
	return 0;
}
#endif


#endif /* TIER0_MEMALLOC_H */
