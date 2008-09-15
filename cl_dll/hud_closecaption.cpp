//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//


#include "cbase.h"
#include <ctype.h>
#include "sentence.h"
#include "hud_closecaption.h"
#include "vstdlib/strtools.h"
#include <vgui_controls/Controls.h>
#include <vgui/IVgui.h>
#include <vgui/ISurface.h>
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include "iclientmode.h"
#include "hud_macros.h"
#include "checksum_crc.h"
#include "filesystem.h"
#include "datacache/idatacache.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define CC_INSET		12

// Marked as FCVAR_USERINFO so that the server can cull CC messages before networking them down to us!!!
ConVar closecaption( "closecaption", "0", FCVAR_ARCHIVE | FCVAR_USERINFO, "Enable close captioning." );
extern ConVar cc_lang;
static ConVar cc_linger_time( "cc_linger_time", "1.0", FCVAR_ARCHIVE, "Close caption linger time." );
static ConVar cc_predisplay_time( "cc_predisplay_time", "0.25", FCVAR_ARCHIVE, "Close caption delay before showing caption." );
static ConVar cc_captiontrace( "cc_captiontrace", "1", 0, "Show missing closecaptions (0 = no, 1 = devconsole, 2 = show in hud)" );
static ConVar cc_subtitles( "cc_subtitles", "0", FCVAR_ARCHIVE | FCVAR_ARCHIVE_XBOX, "If set, don't show sound effect captions, just voice overs (i.e., won't help hearing impaired players)." );
ConVar english( "english", "1", FCVAR_USERINFO, "If set to 1, running the english language set of assets." );

#if defined( _XBOX )
static ConVar cc_smallfontlength( "cc_smallfontlength", "300", 0, "If text stream is this long, force usage of small font size." );
#endif

#define	MAX_CAPTION_CHARACTERS		2048

#define CAPTION_PAN_FADE_TIME		0.5			// The time it takes for a line to fade while panning over a large entry
#define CAPTION_PAN_SLIDE_TIME		0.5			// The time it takes for a line to slide on while panning over a large entry

//-----------------------------------------------------------------------------
// Purpose: Helper for sentence.cpp
// Input  : *ansi - 
//			*unicode - 
//			unicodeBufferSize - 
// Output : int
//-----------------------------------------------------------------------------
#ifndef _XBOX
int ConvertANSIToUnicode(const char *ansi, wchar_t *unicode, int unicodeBufferSize)
{
	return vgui::localize()->ConvertANSIToUnicode( ansi, unicode, unicodeBufferSize );
}
#endif

// A work unit is a pre-processed chunk of CC text to display
// Any state changes (font/color/etc) cause a new work unit to be precomputed
// Moving onto a new line also causes a new Work Unit
// The width and height are stored so that layout can be quickly recomputed each frame
class CCloseCaptionWorkUnit
{
public:
	CCloseCaptionWorkUnit();
	~CCloseCaptionWorkUnit();

	void	SetWidth( int w );
	int		GetWidth() const;

	void	SetHeight( int h );
	int		GetHeight() const;

	void	SetPos( int x, int y );
	void	GetPos( int& x, int &y ) const;

	void	SetFadeStart( float flTime );
	float	GetFadeStart( void ) const;

	void	SetBold( bool bold );
	bool	GetBold() const;

	void	SetItalic( bool ital );
	bool	GetItalic() const;

	void	SetStream( const wchar_t *stream );
	const wchar_t	*GetStream() const;

	void	SetColor( Color& clr );
	Color GetColor() const;

	vgui::HFont		GetFont() const
	{
		return m_hFont;
	}
	
	void		SetFont( vgui::HFont fnt )
	{
		m_hFont = fnt;
	}

	void Dump()
	{
		char buf[ 2048 ];
		vgui::localize()->ConvertUnicodeToANSI( GetStream(), buf, sizeof( buf ) );

		Msg( "x = %i, y = %i, w = %i h = %i text %s\n", m_nX, m_nY, m_nWidth, m_nHeight, buf );
	}

private:

	int				m_nX;
	int				m_nY;
	int				m_nWidth;
	int				m_nHeight;
	float			m_flFadeStartTime;

	bool			m_bBold;
	bool			m_bItalic;
	wchar_t			*m_pszStream;
	vgui::HFont			m_hFont;
	Color			m_Color;
};

CCloseCaptionWorkUnit::CCloseCaptionWorkUnit() :
	m_nWidth(0),
	m_nHeight(0),
	m_bBold(false),
	m_bItalic(false),
	m_pszStream(0),
	m_Color( Color( 255, 255, 255, 255 ) ),
	m_hFont( 0 ),
	m_flFadeStartTime(0)
{
}

CCloseCaptionWorkUnit::~CCloseCaptionWorkUnit()
{
	delete[] m_pszStream;
	m_pszStream = NULL;
}

void CCloseCaptionWorkUnit::SetWidth( int w )
{
	m_nWidth = w;
}

int CCloseCaptionWorkUnit::GetWidth() const
{
	return m_nWidth;
}

void CCloseCaptionWorkUnit::SetHeight( int h )
{
	m_nHeight = h;
}

int CCloseCaptionWorkUnit::GetHeight() const
{
	return m_nHeight;
}

void CCloseCaptionWorkUnit::SetPos( int x, int y )
{
	m_nX = x;
	m_nY = y;
}

void CCloseCaptionWorkUnit::GetPos( int& x, int &y ) const
{
	x = m_nX;
	y = m_nY;
}

void CCloseCaptionWorkUnit::SetFadeStart( float flTime )
{
	m_flFadeStartTime = flTime;
}

float CCloseCaptionWorkUnit::GetFadeStart( void ) const
{
	return m_flFadeStartTime;
}

void CCloseCaptionWorkUnit::SetBold( bool bold )
{
	m_bBold = bold;
}

bool CCloseCaptionWorkUnit::GetBold() const
{
	return m_bBold;
}

void CCloseCaptionWorkUnit::SetItalic( bool ital )
{
	m_bItalic = ital;
}

bool CCloseCaptionWorkUnit::GetItalic() const
{
	return m_bItalic;
}

void CCloseCaptionWorkUnit::SetStream( const wchar_t *stream )
{
	delete[] m_pszStream;
	m_pszStream = NULL;

	int len = wcslen( stream );
	Assert( len < 4096 );
	m_pszStream = new wchar_t[ len + 1 ];
	wcsncpy( m_pszStream, stream, len );
	m_pszStream[ len ] = L'\0';
}

const wchar_t *CCloseCaptionWorkUnit::GetStream() const
{
	return m_pszStream ? m_pszStream : L"";
}

void CCloseCaptionWorkUnit::SetColor( Color& clr )
{
	m_Color = clr;
}

Color CCloseCaptionWorkUnit::GetColor() const
{
	return m_Color;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CCloseCaptionItem
{
public:
	CCloseCaptionItem( 
		const wchar_t	*stream,
		float timetolive,
		float addedtime,
		float predisplay,
		bool valid,
		bool fromplayer
	) :
		m_flTimeToLive( 0.0f ),
		m_flAddedTime( addedtime ),
		m_bValid( false ),
		m_nTotalWidth( 0 ),
		m_nTotalHeight( 0 ),
		m_bSizeComputed( false ),
		m_bFromPlayer( fromplayer )

	{
		SetStream( stream );
		SetTimeToLive( timetolive );
		SetInitialLifeSpan( timetolive );
		SetPreDisplayTime( cc_predisplay_time.GetFloat() + predisplay );
		m_bValid = valid;
	}

	CCloseCaptionItem( const CCloseCaptionItem& src )
	{
		SetStream( src.m_szStream );
		m_flTimeToLive = src.m_flTimeToLive;
		m_bValid = src.m_bValid;
		m_bFromPlayer = src.m_bFromPlayer;
		m_flAddedTime = src.m_flAddedTime;
	}

	~CCloseCaptionItem( void )
	{
		while ( m_Work.Count() > 0 )
		{
			CCloseCaptionWorkUnit *unit = m_Work[ 0 ];
			m_Work.Remove( 0 );
			delete unit;
		}

	}

	void SetStream( const wchar_t *stream)
	{
		wcsncpy( m_szStream, stream, sizeof( m_szStream ) / sizeof( wchar_t ) );
	}

	const wchar_t *GetStream() const
	{
		return m_szStream;
	}

	void SetTimeToLive( float ttl )
	{
		m_flTimeToLive = ttl;
	}

	float GetTimeToLive( void ) const
	{
		return m_flTimeToLive;
	}

	void SetInitialLifeSpan( float t )
	{
		m_flInitialLifeSpan = t;
	}

	float GetInitialLifeSpan() const
	{
		return m_flInitialLifeSpan;
	}

	bool IsValid() const
	{
		return m_bValid;
	}

	void	SetHeight( int h )
	{
		m_nTotalHeight = h;
	}
	int		GetHeight() const
	{
		return m_nTotalHeight;
	}
	void	SetWidth( int w )
	{
		m_nTotalWidth = w;
	}
	int		GetWidth() const
	{
		return m_nTotalWidth;
	}

	void	AddWork( CCloseCaptionWorkUnit *unit )
	{
		m_Work.AddToTail( unit );
	}

	int		GetNumWorkUnits() const
	{
		return m_Work.Count();
	}

	CCloseCaptionWorkUnit *GetWorkUnit( int index )
	{
		Assert( index >= 0 && index < m_Work.Count() );

		return m_Work[ index ];
	}

	void		SetSizeComputed( bool computed )
	{
		m_bSizeComputed = computed;
	}

	bool		GetSizeComputed() const
	{
		return m_bSizeComputed;
	}

	void		SetPreDisplayTime( float t )
	{
		m_flPreDisplayTime = t;
	}

	float		GetPreDisplayTime() const
	{
		return m_flPreDisplayTime;
	}

	float		GetAlpha( float fadeintimehidden, float fadeintime, float fadeouttime )
	{
		float time_since_start = m_flInitialLifeSpan - m_flTimeToLive;
		float time_until_end =  m_flTimeToLive;

		float totalfadeintime = fadeintimehidden + fadeintime;

		if ( totalfadeintime > 0.001f && 
			time_since_start < totalfadeintime )
		{
			if ( time_since_start >= fadeintimehidden )
			{
				float f = 1.0f;
				if ( fadeintime > 0.001f )
				{
					f = ( time_since_start - fadeintimehidden ) / fadeintime;
				}
				f = clamp( f, 0.0f, 1.0f );
				return f;
			}
			
			return 0.0f;
		}

		if ( fadeouttime > 0.001f &&
			time_until_end < fadeouttime )
		{
			float f = time_until_end / fadeouttime;
			f = clamp( f, 0.0f, 1.0f );
			return f;
		}

		return 1.0f;
	}

	float	GetAddedTime() const
	{
		return m_flAddedTime;
	}

	void	SetAddedTime( float addt )
	{
		m_flAddedTime = addt;
	}

	bool	IsFromPlayer() const
	{
		return m_bFromPlayer;
	}

private:
	wchar_t				m_szStream[ MAX_CAPTION_CHARACTERS ];

	float				m_flPreDisplayTime;
	float				m_flTimeToLive;
	float				m_flInitialLifeSpan;
	float				m_flAddedTime;
	bool				m_bValid;
	int					m_nTotalWidth;
	int					m_nTotalHeight;

	bool				m_bSizeComputed;
	bool				m_bFromPlayer;

	CUtlVector< CCloseCaptionWorkUnit * >	m_Work;

};

struct VisibleStreamItem
{
	int					height;
	int					width;
	CCloseCaptionItem	*item;
};

//-----------------------------------------------------------------------------
// Purpose: The only resource manager parameter we currently care about is the name 
//  of the .vcd to cache into memory
//-----------------------------------------------------------------------------
struct asynccaptionparams_t
{
	char const *dbfile;
	int			fileindex;
	int			blocktoload;
	int			blockoffset;
	int			blocksize;
};

// 16K of cache for close caption data
#define MAX_ASYNCCAPTION_MEMORY_CACHE (int)( 16.0 * 1024.0f )

void CaptionAsyncLoaderCallback( const FileAsyncRequest_t &request, int numReadBytes, FSAsyncStatus_t asyncStatus );

struct AsyncCaptionData_t
{
	int					m_nBlockNum;
	byte				*m_pBlockData;
	int					m_nFileIndex;
	int					m_nBlockSize;
	
	bool				m_bLoadPending : 1;
	bool				m_bLoadCompleted : 1;

	FSAsyncControl_t	m_hAsyncControl;

	AsyncCaptionData_t() :
		m_nBlockNum( -1 ),
		m_pBlockData( 0 ),
		m_nFileIndex( -1 ),
		m_nBlockSize( 0 ),
		m_bLoadPending( false ),
		m_bLoadCompleted( false ),
		m_hAsyncControl( NULL )
	{
	}

	// APIS required by CDataManager
	void DestroyResource()
	{
		if ( m_bLoadPending && !m_bLoadCompleted )
		{
			filesystem->AsyncFinish( m_hAsyncControl, true );
		}
		filesystem->AsyncRelease( m_hAsyncControl );

		WipeData();
		delete this;
	}

	void ReleaseData()
	{
		filesystem->AsyncRelease( m_hAsyncControl );
		m_hAsyncControl = 0;
		WipeData();
		m_bLoadCompleted = false;
		Assert( !m_bLoadPending );
	}

	void WipeData()
	{
		delete[] m_pBlockData;
		m_pBlockData = NULL;
	}

	AsyncCaptionData_t		*GetData()
	{ 
		return this; 
	}
	unsigned int	Size()
	{ 
		return sizeof( *this ) + m_nBlockSize; 
	}

	void AsyncLoad( char const *fileName, int blockOffset )
	{
		// Already pending
		Assert ( !m_hAsyncControl );

		// async load the file	
		FileAsyncRequest_t fileRequest;
		fileRequest.pContext    = (void *)this;
		fileRequest.pfnCallback = ::CaptionAsyncLoaderCallback;
		fileRequest.pData       = m_pBlockData;
		fileRequest.pszFilename = fileName;
		fileRequest.nOffset     = blockOffset;
		fileRequest.flags       = FSASYNC_FLAGS_RAWIO;
		fileRequest.nBytes      = m_nBlockSize;
		fileRequest.priority    = -1;
		fileRequest.pszPathID   = IsXbox() ? "XGAME" : "GAME";
		
		// queue for async load
		MEM_ALLOC_CREDIT();
		filesystem->AsyncRead( fileRequest, &m_hAsyncControl );
	}

	// you must implement these static functions for the ResourceManager
	// -----------------------------------------------------------
	static AsyncCaptionData_t *CreateResource( const asynccaptionparams_t &params )
	{
		AsyncCaptionData_t *data = new AsyncCaptionData_t;
		data->m_nBlockNum = params.blocktoload;
		data->m_nFileIndex = params.fileindex;
		data->m_nBlockSize = params.blocksize;
		data->m_pBlockData = new byte[ data->m_nBlockSize ];
		return data;
	}

	static unsigned int EstimatedSize( const asynccaptionparams_t &params )
	{
		// The block size is assumed to be 4K
		return ( sizeof( AsyncCaptionData_t ) + params.blocksize );
	}
};

//-----------------------------------------------------------------------------
// Purpose: This manages the instanced scene memory handles.  We essentially grow a handle list by scene filename where
//  the handle is a pointer to a AsyncCaptionData_t defined above.  If the resource manager uncaches the handle, we reload the
//  .vcd from disk.  Precaching a .vcd calls into FindOrAddBlock which moves the .vcd to the head of the LRU if it's in memory
//  or it reloads it from disk otherwise.
//-----------------------------------------------------------------------------
class CAsyncCaptionResourceManager : public CAutoGameSystem, public CManagedDataCacheClient< AsyncCaptionData_t, asynccaptionparams_t >
{
public:
	CAsyncCaptionResourceManager() : CAutoGameSystem( "CAsyncCaptionResourceManager" )
	{
	}

	void		SetDbInfo( const CUtlVector< AsyncCaption_t > & info )
	{
		m_Db = info;
	}

	virtual bool Init()
	{
		CCacheClientBaseClass::Init( datacache, "Captions", MAX_ASYNCCAPTION_MEMORY_CACHE );
		return true;
	}
	virtual void Shutdown()
	{
		Clear();
		CCacheClientBaseClass::Shutdown();
	}

	//-----------------------------------------------------------------------------
	// Purpose: Spew a cache summary to the console
	//-----------------------------------------------------------------------------
	void SpewMemoryUsage()
	{
		GetCacheSection()->OutputReport();

		DataCacheStatus_t status;
		DataCacheLimits_t limits;
		GetCacheSection()->GetStatus( &status, &limits );
		int bytesUsed = status.nBytes;
		int bytesTotal = limits.nMaxBytes;

		float percent = 100.0f * (float)bytesUsed / (float)bytesTotal;

		int count = 0;
		for ( int i = 0; i < m_Db.Count(); ++i )
		{
			count += m_Db[ i ].m_RequestedBlocks.Count();
		}

		DevMsg( "CAsyncCaptionResourceManager:  %i blocks total %s, %.2f %% of capacity\n", count, Q_pretifymem( bytesUsed, 2 ), percent );
	}

	virtual void LevelInitPostEntity()
	{
	}

	void CaptionAsyncLoaderCallback( const FileAsyncRequest_t &request, int numReadBytes, FSAsyncStatus_t asyncStatus )
	{	
		// get our preserved data
		AsyncCaptionData_t *pData = ( AsyncCaptionData_t * )request.pContext;

		Assert( pData );

		// mark as completed in single atomic operation
		pData->m_bLoadCompleted = true;
	}

	int ComputeBlockOffset( int fileIndex, int blockNum )
	{
		return m_Db[ fileIndex ].m_Header.dataoffset + blockNum * m_Db[ fileIndex ].m_Header.blocksize;
	}

	void GetBlockInfo( int fileIndex, int blockNum, bool& entry, bool& pending, bool& loaded )
	{
		pending = false;
		loaded = false;
		AsyncCaption_t::BlockInfo_t search;
		search.fileindex = fileIndex;
		search.blocknum = blockNum;

		CUtlRBTree< AsyncCaption_t::BlockInfo_t, unsigned short >& requested = m_Db[ fileIndex ].m_RequestedBlocks;

		int idx = requested.Find( search );
		if ( idx == requested.InvalidIndex() )
		{
			entry = false;
			return;
		}
		entry = true;

		DataCacheHandle_t handle = requested[ idx ].handle;
		AsyncCaptionData_t	*pCaptionData = CacheLock( handle );
		if ( pCaptionData )
		{
			if ( pCaptionData->m_bLoadPending )
			{
				pending = true;
			}
			else if ( pCaptionData->m_bLoadCompleted )
			{
				loaded = true;
			}
			CacheUnlock( handle );
		}
	}

	// Either commences async loading or polls for async loading once per frame to wait for it to complete...
	void PollForAsyncLoading( CHudCloseCaption *hudCloseCaption, int dbFileIndex, int blockNum )
	{
		char const *dbname = m_Db[ dbFileIndex ].m_DataBaseFile.String();

		CUtlRBTree< AsyncCaption_t::BlockInfo_t, unsigned short >& requested = m_Db[ dbFileIndex ].m_RequestedBlocks;

		int idx = FindOrAddBlock( dbFileIndex, blockNum );
		if ( idx == requested.InvalidIndex() )
		{
			Assert( 0 );
			return;
		}

		DataCacheHandle_t handle = requested[ idx ].handle;

		AsyncCaptionData_t	*pCaptionData = CacheLock( handle );
		if ( !pCaptionData )
		{
			// Try and reload it
			char fn[ 256 ];
			Q_strncpy( fn, dbname, sizeof( fn ) );
			Q_FixSlashes( fn );
			Q_strlower( fn );

			asynccaptionparams_t params;
			params.dbfile		= fn;
			params.blocktoload	= blockNum;
			params.blocksize	= m_Db[ dbFileIndex ].m_Header.blocksize;
			params.blockoffset	= ComputeBlockOffset( dbFileIndex, blockNum );
			params.fileindex    = dbFileIndex;

			handle = requested[ idx ].handle = CacheCreate( params );
			pCaptionData = CacheLock( handle );
			if ( !pCaptionData )
			{
				Assert( pCaptionData );
				return;
			}
		}

		if ( pCaptionData->m_bLoadCompleted )
		{
			pCaptionData->m_bLoadPending = false;
			// Copy in data at this point
			Assert( hudCloseCaption );
			if ( hudCloseCaption )
			{
				hudCloseCaption->OnFinishAsyncLoad( requested[ idx ].fileindex, requested[ idx ].blocknum, pCaptionData );
			}

			// This finalizes the load (unlocks the handle)
			GetCacheSection()->BreakLock( handle );
			return;
		}

		if ( pCaptionData->m_bLoadPending )
		{
			CacheUnlock( handle );
			return;
		}

		// Commence load (locks handle for entire async load) (unlocked above)
		pCaptionData->m_bLoadPending = true;
		pCaptionData->AsyncLoad( dbname, ComputeBlockOffset( dbFileIndex, blockNum ) );
	}
	
	//-----------------------------------------------------------------------------
	// Purpose: Touch the cache or load the scene into the cache for the first time
	// Input  : *filename - 
	//-----------------------------------------------------------------------------
	int FindOrAddBlock( int dbFileIndex, int blockNum )
	{
		char const *dbname = m_Db[ dbFileIndex ].m_DataBaseFile.String();

		CUtlRBTree< AsyncCaption_t::BlockInfo_t, unsigned short >& requested = m_Db[ dbFileIndex ].m_RequestedBlocks;

		AsyncCaption_t::BlockInfo_t search;
		search.blocknum = blockNum;
		search.fileindex = dbFileIndex;

		int idx = requested.Find( search );
		if ( idx != requested.InvalidIndex() )
		{
			// Move it to head of LRU
			CacheTouch( requested[ idx ].handle );
			return idx;
		}

		char fn[ 256 ];
		Q_strncpy( fn, dbname, sizeof( fn ) );
		Q_FixSlashes( fn );
		Q_strlower( fn );

		asynccaptionparams_t params;
		params.dbfile		= fn;
		params.blocktoload	= blockNum;
		params.blockoffset	= ComputeBlockOffset( dbFileIndex, blockNum );
		params.blocksize	= m_Db[ dbFileIndex ].m_Header.blocksize;
		params.fileindex    = dbFileIndex;

		memhandle_t handle = CacheCreate( params );

		AsyncCaption_t::BlockInfo_t info;
		info.fileindex = dbFileIndex;
		info.blocknum = blockNum;
		info.handle = handle;
        
		// Add scene filename to dictionary
		idx = requested.Insert( info );
		return idx;
	}

	void Flush()
	{
		CacheFlush();
	}

	void Clear()
	{
		for ( int file = 0; file < m_Db.Count(); ++file )
		{
			CUtlRBTree< AsyncCaption_t::BlockInfo_t, unsigned short >& requested = m_Db[ file ].m_RequestedBlocks;

			int c = requested.Count();
			for ( int i = 0; i  < c; ++i )
			{
				memhandle_t dat = requested[ i ].handle;
				CacheRemove( dat );
			}

			requested.RemoveAll();
		}
	}

private:
	
	CUtlVector< AsyncCaption_t >				m_Db;
};

CAsyncCaptionResourceManager g_AsyncCaptionResourceManager;

void CaptionAsyncLoaderCallback( const FileAsyncRequest_t &request, int numReadBytes, FSAsyncStatus_t asyncStatus )
{
	g_AsyncCaptionResourceManager.CaptionAsyncLoaderCallback( request, numReadBytes, asyncStatus );
}

DECLARE_HUDELEMENT( CHudCloseCaption );

DECLARE_HUD_MESSAGE( CHudCloseCaption, CloseCaption );

CHudCloseCaption::CHudCloseCaption( const char *pElementName )
	: CHudElement( pElementName ), 
	vgui::Panel( NULL, "HudCloseCaption" ),
	m_CloseCaptionRepeats( 0, 0, CaptionTokenLessFunc ),
	m_CurrentLanguage( UTL_INVAL_SYMBOL ),
	m_bPaintDebugInfo( false )
#if !defined( _XBOX )
	, m_TokenNameLookup( 0, 0, TokenNameLessFunc )
#endif
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_nGoalHeight = 0;
	m_nCurrentHeight = 0;
	m_flGoalAlpha = 1.0f;
	m_flCurrentAlpha = 1.0f;

	m_flGoalHeightStartTime = 0;
	m_flGoalHeightFinishTime = 0;

	m_bLocked = false;
	m_bVisibleDueToDirect = false;

	SetPaintBorderEnabled( false );
	SetPaintBackgroundEnabled( false );

	vgui::ivgui()->AddTickSignal( GetVPanel(), 0 );

	vgui::localize()->AddFileEx( vgui::filesystem(), "resource/closecaption_%language%.txt", "GAME", true );

	HOOK_HUD_MESSAGE( CHudCloseCaption, CloseCaption );
#if !defined( _XBOX )
	BuildTokenNameLookup();
#endif

	char uilanguage[ 64 ];
	engine->GetUILanguage( uilanguage, sizeof( uilanguage ) );

	if ( !Q_stricmp( uilanguage, "english" ) )
	{
		english.SetValue( 1 );
	}
	else
	{
		english.SetValue( 0 );
	}

	char dbfile [ 512 ];
	Q_snprintf( dbfile, sizeof( dbfile ), "resource/closecaption_%s.dat", uilanguage );
	InitCaptionDictionary( dbfile );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudCloseCaption::~CHudCloseCaption()
{
	m_CloseCaptionRepeats.RemoveAll();
#if !defined( _XBOX )
	m_TokenNameLookup.RemoveAll();
#endif

	ClearAsyncWork();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *newmap - 
//-----------------------------------------------------------------------------
void CHudCloseCaption::LevelInit( void )
{
	CreateFonts();
	// Reset repeat counters per level
	m_CloseCaptionRepeats.RemoveAll();

	// Wipe any stale pending work items...
	ClearAsyncWork();
}

static ConVar cc_minvisibleitems( "cc_minvisibleitems", "1", 0, "Minimum number of caption items to show." );

void CHudCloseCaption::TogglePaintDebug()
{
	m_bPaintDebugInfo = !m_bPaintDebugInfo;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudCloseCaption::Paint( void )
{
	int w, h;
	GetSize( w, h );

	if ( m_bPaintDebugInfo )
	{
		int blockWide = 350;
		int startx = 50;
		
		int y = 0;
		int size = 8;
		int sizewithgap = size + 1;

		for ( int a = 0; a < m_AsyncCaptions.Count(); ++a )
		{
			int x = startx;

			int c = m_AsyncCaptions[ a ].m_Header.numblocks;
			for ( int i = 0 ; i < c; ++i )
			{
				bool entry, pending, loaded;
				g_AsyncCaptionResourceManager.GetBlockInfo( a, i, entry, pending, loaded );

				if ( !entry )
				{
					vgui::surface()->DrawSetColor( Color( 0, 0, 0, 127 ) );
				}
				else if ( pending )
				{
					vgui::surface()->DrawSetColor( Color( 0, 0, 255, 127 ) );
				}
				else if ( loaded )
				{
					vgui::surface()->DrawSetColor( Color( 0, 255, 0, 127 ) );
				}
				else
				{
					vgui::surface()->DrawSetColor( Color( 255, 255, 0, 127 ) );
				}

				vgui::surface()->DrawFilledRect( x, y, x + size, y + size );
				x += sizewithgap;
				if ( x >= startx + blockWide )
				{
					x = startx;
					y += sizewithgap;
				}
			}

			y += sizewithgap;
		}
	}

	wrect_t rcOutput;
	rcOutput.left = 0;
	rcOutput.right = w;
	rcOutput.bottom = h;
	rcOutput.top = m_nTopOffset;

	wrect_t rcText = rcOutput;

	int avail_width = rcText.right - rcText.left - 2 * CC_INSET;
	int avail_height = rcText.bottom - rcText.top - 2 * CC_INSET;

	int totalheight = 0;
	int i;
	CUtlVector< VisibleStreamItem > visibleitems;
	int c = m_Items.Count();
	int maxwidth = 0;

	for  ( i = 0; i < c; i++ )
	{
		CCloseCaptionItem *item = m_Items[ i ];

		// Not ready for display yet.
		if ( item->GetPreDisplayTime() > 0.0f )
		{
			continue;
		}

		if ( !item->GetSizeComputed() )
		{
			ComputeStreamWork( avail_width, item );
		}

		int itemwidth = item->GetWidth();
		int itemheight = item->GetHeight();

		totalheight += itemheight;
		if ( itemwidth > maxwidth )
		{
			maxwidth = itemwidth;
		}

		VisibleStreamItem si;
		si.height = itemheight;
		si.width = itemwidth;
		si.item = item;

		visibleitems.AddToTail( si );

		// Start popping really old items off the stack if we run out of space
		while ( itemheight <= avail_height && 
				totalheight > avail_height && 
				visibleitems.Count() > cc_minvisibleitems.GetInt() )
		{
			VisibleStreamItem & pop = visibleitems[ 0 ];
			totalheight -= pop.height;

			// And make it die right away...
			pop.item->SetTimeToLive( 0.0f );

			visibleitems.Remove( 0 );
		}	
	}

	float desiredAlpha = visibleitems.Count() >= 1 ? 1.0f : 0.0f;

	// Always return at least one line height for drawing the surrounding box
	totalheight = max( totalheight, m_nLineHeight ); 

	// Trigger box growing
	if ( totalheight != m_nGoalHeight )
	{
		m_nGoalHeight = totalheight;
		m_flGoalHeightStartTime = gpGlobals->curtime;
		m_flGoalHeightFinishTime = gpGlobals->curtime + m_flGrowTime;
	}
	if ( desiredAlpha != m_flGoalAlpha )
	{
		m_flGoalAlpha = desiredAlpha;
		m_flGoalHeightStartTime = gpGlobals->curtime;
		m_flGoalHeightFinishTime = gpGlobals->curtime + m_flGrowTime;
	}

	// If shrunk to zero and faded out, nothing left to do
	if ( !visibleitems.Count() &&
		m_nGoalHeight == m_nCurrentHeight &&
		m_flGoalAlpha == m_flCurrentAlpha )
	{
		m_flGoalHeightStartTime = 0;
		m_flGoalHeightFinishTime = 0;
		return;
	}

	bool growingDown = false;

	// Continue growth?
	if ( m_flGoalHeightFinishTime &&
		m_flGoalHeightStartTime &&
		m_flGoalHeightFinishTime > m_flGoalHeightStartTime )
	{
		float togo = m_nGoalHeight - m_nCurrentHeight;
		float alphatogo = m_flGoalAlpha - m_flCurrentAlpha;

		growingDown = togo < 0.0f ? true : false;

		float dt = m_flGoalHeightFinishTime - m_flGoalHeightStartTime;
		float frac = ( gpGlobals->curtime - m_flGoalHeightStartTime ) / dt;
		frac = clamp( frac, 0.0f, 1.0f );
		int newHeight = m_nCurrentHeight + (int)( frac * togo );
		m_nCurrentHeight = newHeight;
		float newAlpha = m_flCurrentAlpha + frac * alphatogo;
		m_flCurrentAlpha = clamp( newAlpha, 0.0f, 1.0f );
	}
	else
	{
		m_nCurrentHeight = m_nGoalHeight;
		m_flCurrentAlpha = m_flGoalAlpha;
	}

	rcText.top = rcText.bottom - m_nCurrentHeight - 2 * CC_INSET;
 
	Color bgColor = GetBgColor();
   	bgColor[3] = m_flBackgroundAlpha;
	DrawBox( rcText.left, max(rcText.top,0), rcText.right - rcText.left, rcText.bottom - max(rcText.top,0), bgColor, m_flCurrentAlpha );

	if ( !visibleitems.Count() )
	{
		return;
	}

	rcText.left += CC_INSET;
	rcText.right -= CC_INSET;

	int textHeight = m_nCurrentHeight;
	if ( growingDown )
	{
		// If growing downward, keep the text locked to the bottom of the window instead of anchored to the top
		textHeight = totalheight;
	}

	rcText.top = rcText.bottom - textHeight - CC_INSET;

	wrect_t rcWindow;
	rcWindow.left = 0;
	rcWindow.top = 0;
	rcWindow.right = avail_width;
	rcWindow.bottom = avail_height;

	// Now draw them
	c = visibleitems.Count();
	for ( i = 0; i < c; i++ )
	{
		VisibleStreamItem *si = &visibleitems[ i ];

		// If the oldest/top item was created with additional time, we can remove that now
		if ( i == 0 )
		{
			if ( si->item->GetAddedTime() > 0.0f )
			{
				float ttl = si->item->GetTimeToLive();
				ttl -= si->item->GetAddedTime();
				ttl = max( 0.0f, ttl );
				si->item->SetTimeToLive( ttl );
				si->item->SetAddedTime( 0.0f );
			}
		}

		int height = si->height;
 		CCloseCaptionItem *item = si->item;
		 
		int iFadeLine = -1;
		float flFadeLineAlpha = 1.0;
	
		// If the height is greater than the total height of the element, 
		// we need to slowly pan over this item. 
		if ( height > avail_height )
		{
			// Figure out how many lines we'll need to move to see the whole caption
			int units = item->GetNumWorkUnits();
 			int iTotalMove = 0;
			for ( int j = 0 ; j < units; j++ )
			{
				CCloseCaptionWorkUnit *wu = item->GetWorkUnit( j );
				iTotalMove += wu->GetHeight();
				if ( iTotalMove >= (height - avail_height) )
				{
					units = j+1;
					break;
				}
			}

			// Figure out the delta between each point where we move the line
			float flMoveDelta = item->GetInitialLifeSpan() / (float)units;
 			float flCurMove = item->GetInitialLifeSpan() - item->GetTimeToLive();
 			int iHeightToMove = 0;

 			int iLinesToMove = clamp( floor( flCurMove / flMoveDelta ), 0, units );
			if ( iLinesToMove )
			{
 				int iCurrentLineHeight = 0;
				for ( int j = 0 ; j < iLinesToMove; j++ )
				{
					iHeightToMove = iCurrentLineHeight;

					CCloseCaptionWorkUnit *wu = item->GetWorkUnit( j );
  					iCurrentLineHeight += wu->GetHeight();
				}

				// Slide to the desired distance, once the fade is done
	 			float flTimePostMove = flCurMove - (flMoveDelta * iLinesToMove);
 				if ( flTimePostMove < CAPTION_PAN_FADE_TIME )
				{
					iFadeLine = iLinesToMove-1;

					// It's time to fade out the top line. If it hasn't started fading yet, start it.
					CCloseCaptionWorkUnit *wu = item->GetWorkUnit(iFadeLine);
					if ( wu->GetFadeStart() == 0 )
					{
						wu->SetFadeStart( gpGlobals->curtime );
					}

					// Fade out quickly
					float flFadeTime = (gpGlobals->curtime - wu->GetFadeStart()) /  CAPTION_PAN_FADE_TIME;
					flFadeLineAlpha = clamp( 1.0 - flFadeTime, 0, 1 );
				}
				else if ( flTimePostMove < (CAPTION_PAN_FADE_TIME+CAPTION_PAN_SLIDE_TIME) )
				{
					flTimePostMove -= CAPTION_PAN_FADE_TIME;
 					float flSlideTime = clamp( flTimePostMove / 0.25, 0, 1 );
 					iHeightToMove += ceil((iCurrentLineHeight - iHeightToMove) * flSlideTime);
				}
				else
				{
					iHeightToMove = iCurrentLineHeight;
				}
			}

			// Minor adjustment to center the caption text within the window.
 			rcText.top = -iHeightToMove + 2;
		}

		rcText.bottom = rcText.top + height;
 
		wrect_t rcOut = rcText;
 
		rcOut.right = rcOut.left + si->width + 6;
		
		DrawStream( rcOut, rcWindow, item, iFadeLine, flFadeLineAlpha );

		rcText.top += height;
		rcText.bottom += height;

		if ( rcText.top >= rcOutput.bottom )
			break;
	}
}

bool CHudCloseCaption::LookupUnicodeText( char const *token, wchar_t *outbuf, size_t count )
{
	wchar_t *outstr = vgui::localize()->Find( token );
	if ( !outstr )
	{
		wcsncpy( outbuf, L"<can't find entry>", count );
		return false;
	}

	wcsncpy( outbuf, outstr, count );

	return true;
}

void CHudCloseCaption::OnTick( void )
{
	// See if any async work has completed
	ProcessAsyncWork();


	float dt = gpGlobals->frametime;

	int c = m_Items.Count();
	int i;

	if ( m_bVisibleDueToDirect )
	{
		SetVisible( true );
		if ( !c )
		{
			m_bVisibleDueToDirect = false;
		}
	}
	else
	{
		SetVisible( closecaption.GetBool() );
	}

	// Pass one decay all timers
	for ( i = 0 ; i < c ; ++i )
	{
		CCloseCaptionItem *item = m_Items[ i ];

		float predisplay = item->GetPreDisplayTime();
		if ( predisplay > 0.0f )
		{
			predisplay -= dt;
			predisplay = max( 0.0f, predisplay );
			item->SetPreDisplayTime( predisplay );
		}
		else
		{
			// remove time from actual playback
			float ttl = item->GetTimeToLive();
			ttl -= dt;
			ttl = max( 0.0f, ttl );
			item->SetTimeToLive( ttl );
		}
	}

	// Pass two, remove from head until we get to first item with time remaining
	bool foundfirstnondeletion = false;
	for ( i = 0 ; i < c ; ++i )
	{
		CCloseCaptionItem *item = m_Items[ i ];

		// Skip items not yet showing...
		float predisplay = item->GetPreDisplayTime();
		if ( predisplay > 0.0f )
		{
			continue;
		}

		float ttl = item->GetTimeToLive();
		if ( ttl > 0.0f )
		{
			foundfirstnondeletion = true;
			continue;
		}

		// Skip the remainder of the items after we find the first/oldest active item
		if ( foundfirstnondeletion )
		{
			continue;
		}

		delete item;
		m_Items.Remove( i );
		--i;
		--c;
	}
}

void CHudCloseCaption::Reset( void )
{
	while ( m_Items.Count() > 0 )
	{
		CCloseCaptionItem *i = m_Items[ 0 ];
		delete i;
		m_Items.Remove( 0 );
	}

	ClearAsyncWork();
	Unlock();
}

bool CHudCloseCaption::SplitCommand( wchar_t const **ppIn, wchar_t *cmd, wchar_t *args ) const
{
	const wchar_t *in = *ppIn;
	const wchar_t *oldin = in;

	if ( in[0] != L'<' )
	{
		*ppIn += ( oldin - in );
		return false;
	}

	args[ 0 ] = 0;
	cmd[ 0 ]= 0;
	wchar_t *out = cmd;
	in++;
	while ( *in != L'\0' && *in != L':' && *in != L'>' && !isspace( *in ) )
	{
		*out++ = *in++;
	}
	*out = L'\0';

	if ( *in != L':' )
	{
		*ppIn += ( in - oldin );
		return true;
	}

	in++;
	out = args;
	while ( *in != L'\0' && *in != L'>' )
	{
		*out++ = *in++;
	}
	*out = L'\0';

	//if ( *in == L'>' )
	//	in++;

	*ppIn += ( in - oldin );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *stream - 
//			*findcmd - 
//			value - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHudCloseCaption::GetFloatCommandValue( const wchar_t *stream, const wchar_t *findcmd, float& value ) const
{
	const wchar_t *curpos = stream;
	
	for ( ; curpos && *curpos != L'\0'; ++curpos )
	{
		wchar_t cmd[ 256 ];
		wchar_t args[ 256 ];

		if ( SplitCommand( &curpos, cmd, args ) )
		{
			if ( !wcscmp( cmd, findcmd ) )
			{
				value = (float)wcstod( args, NULL );
				return true;
			}
			continue;
		}
	}

	return false;
}


bool CHudCloseCaption::StreamHasCommand( const wchar_t *stream, const wchar_t *findcmd ) const
{
	const wchar_t *curpos = stream;
	
	for ( ; curpos && *curpos != L'\0'; ++curpos )
	{
		wchar_t cmd[ 256 ];
		wchar_t args[ 256 ];

		if ( SplitCommand( &curpos, cmd, args ) )
		{
			if ( !wcscmp( cmd, findcmd ) )
			{
				return true;
			}
			continue;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: It's blank or only comprised of whitespace/space characters...
// Input  : *stream - 
// Output : static bool
//-----------------------------------------------------------------------------
static bool IsAllSpaces( const wchar_t *stream )
{
	const wchar_t *p = stream;
	while ( *p != L'\0' )
	{
		if ( !iswspace( *p ) )
			return false;

		p++;
	}

	return true;
}

bool CHudCloseCaption::StreamHasCommand( const wchar_t *stream, const wchar_t *search )
{
	for ( const wchar_t *curpos = stream; curpos && *curpos != L'\0'; ++curpos )
	{
		wchar_t cmd[ 256 ];
		wchar_t args[ 256 ];

		if ( SplitCommand( &curpos, cmd, args ) )
		{
			if ( !wcscmp( cmd, search ) )
			{
				return true;
			}
		}
	}
	return false;
}

void CHudCloseCaption::Process( const wchar_t *stream, float duration, char const *tokenstream, bool fromplayer, bool direct )
{
	if ( !direct )
	{
		if ( !closecaption.GetBool() )
		{
			Reset();
			return;
		}

		// If we're locked, ignore all closecaption commands
		if ( m_bLocked )
			return;
	}

	// Nothing to do...
	if ( IsAllSpaces( stream) )
	{
		return;
	}

	// If subtitling, don't show sfx captions at all
	if ( cc_subtitles.GetBool() && StreamHasCommand( stream, L"sfx" ) )
	{
		return;
	}

	bool valid = true;
	if ( !wcsncmp( stream, L"!!!", wcslen( L"!!!" ) ) )
	{
		// It's in the text file, but hasn't been translated...
		valid = false;
	}

	if ( !wcsncmp( stream, L"-->", wcslen( L"-->" ) ) )
	{
		// It's in the text file, but hasn't been translated...
		valid = false;

		if ( cc_captiontrace.GetInt() < 2 )
		{
			if ( cc_captiontrace.GetInt() == 1 )
			{
				Msg( "Missing caption for '%s'\n", tokenstream );
			}

			return;
		}
	}

	float lifespan = duration + cc_linger_time.GetFloat();
	
	float addedlife = 0.0f;

	if ( m_Items.Count() > 0 )
	{
		// Get the remaining life span of the last item
		CCloseCaptionItem *final = m_Items[ m_Items.Count() - 1 ];
		float prevlife = final->GetTimeToLive();

		if ( prevlife > lifespan )
		{
			addedlife = prevlife - lifespan;
		}

		lifespan = max( lifespan, prevlife );
	}
	
	float delay = 0.0f;
	float override_duration = 0.0f;

	wchar_t phrase[ MAX_CAPTION_CHARACTERS ];
	wchar_t *out = phrase;

	for ( const wchar_t *curpos = stream; curpos && *curpos != L'\0'; ++curpos )
	{
		wchar_t cmd[ 256 ];
		wchar_t args[ 256 ];

		const wchar_t *prevpos = curpos;

		if ( SplitCommand( &curpos, cmd, args ) )
		{
			if ( !wcscmp( cmd, L"delay" ) )
			{

				// End current phrase
				*out = L'\0';

				if ( wcslen( phrase ) > 0 )
				{
					CCloseCaptionItem *item = new CCloseCaptionItem( phrase, lifespan, addedlife, delay, valid, fromplayer );
					m_Items.AddToTail( item );
					if ( StreamHasCommand( phrase, L"sfx" ) )
					{
						// SFX show up instantly.
						item->SetPreDisplayTime( 0.0f );
					}
					
					if ( GetFloatCommandValue( phrase, L"len", override_duration ) )
					{
						item->SetTimeToLive( override_duration );
					}
				}

				// Start new phrase
				out = phrase;

				// Delay must be positive
				delay = max( 0.0f, (float)wcstod( args, NULL ) );

				continue;
			}

			int copychars = curpos - prevpos;
			while ( --copychars >= 0 )
			{
				*out++ = *prevpos++;
			}
		}

		*out++ = *curpos;
	}

	// End final phrase, if any
	*out = L'\0';
	if ( wcslen( phrase ) > 0 )
	{
		CCloseCaptionItem *item = new CCloseCaptionItem( phrase, lifespan, addedlife, delay, valid, fromplayer );
		m_Items.AddToTail( item );

		if ( StreamHasCommand( phrase, L"sfx" ) )
		{
			// SFX show up instantly.
			item->SetPreDisplayTime( 0.0f );
		}

		if ( GetFloatCommandValue( phrase, L"len", override_duration ) )
		{
			item->SetTimeToLive( override_duration );
		}
	}
}

void CHudCloseCaption::CreateFonts( void )
{
	vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( GetScheme() );

	m_hFonts[ CCFONT_NORMAL ]		= pScheme->GetFont( "CloseCaption_Normal" );
#if defined( _XBOX )
	m_hFonts[ CCFONT_SMALL ]		= pScheme->GetFont( "CloseCaption_Small" );
#else
    m_hFonts[ CCFONT_BOLD ]			= pScheme->GetFont( "CloseCaption_Bold" );
	m_hFonts[ CCFONT_ITALIC ]		= pScheme->GetFont( "CloseCaption_Italic" );
	m_hFonts[ CCFONT_ITALICBOLD ]	= pScheme->GetFont( "CloseCaption_BoldItalic" );
#endif
	m_nLineHeight = max( 6, vgui::surface()->GetFontTall( m_hFonts[ CCFONT_NORMAL ] ) );
}

struct WorkUnitParams
{
	WorkUnitParams()
	{
		Q_memset( stream, 0, sizeof( stream ) );
		out = stream;
		x = 0;
		y = 0;
		width = 0;
		bold = italic = false;
		clr = Color( 255, 255, 255, 255 );
		newline = false;
		font = 0;
	}

	~WorkUnitParams()
	{
	}

	void Finalize( int lineheight )
	{
		*out = L'\0';
	}

	void Next( int lineheight )
	{
		// Restart output
		Q_memset( stream, 0, sizeof( stream ) );
		out = stream;

		x += width;

		width = 0;
		// Leave bold, italic and color alone!!!
		if ( newline )
		{
			newline = false;
			x = 0;
			y += lineheight;
		}
	}

#if !defined( _XBOX )
	int GetFontNumber()
	{
		return CHudCloseCaption::GetFontNumber( bold, italic );
	}
#endif

	wchar_t	stream[ MAX_CAPTION_CHARACTERS ];
	wchar_t	*out;

	int		x;
	int		y;
	int		width;
	bool	bold;
	bool	italic;
	Color clr;
	bool	newline;
	vgui::HFont font;
};

void CHudCloseCaption::AddWorkUnit( CCloseCaptionItem *item,
	WorkUnitParams& params )
{
	params.Finalize( vgui::surface()->GetFontTall( params.font ) );

	if ( wcslen( params.stream ) > 0 )
	{
		CCloseCaptionWorkUnit *wu = new CCloseCaptionWorkUnit();

		wu->SetStream( params.stream );
		wu->SetColor( params.clr );
		wu->SetBold( params.bold );
		wu->SetItalic( params.italic );
		wu->SetWidth( params.width );
		wu->SetHeight( vgui::surface()->GetFontTall( params.font ) );
		wu->SetPos( params.x, params.y );
		wu->SetFont( params.font );
		wu->SetFadeStart( 0 );

		int curheight = item->GetHeight();
		int curwidth = item->GetWidth();

		curheight = max( curheight, params.y + wu->GetHeight() );
		curwidth = max( curwidth, params.x + params.width );

		item->SetHeight( curheight );
		item->SetWidth( curwidth );

		// Add it
		item->AddWork( wu );

		params.Next( vgui::surface()->GetFontTall( params.font ) );
	}
}

void CHudCloseCaption::ComputeStreamWork( int available_width, CCloseCaptionItem *item )
{
	// Start with a clean param block
	WorkUnitParams params;

	const wchar_t *curpos = item->GetStream();
#if defined( _XBOX )
	int streamlen = wcslen( curpos );
#endif
	CUtlVector< Color > colorStack;

	const wchar_t *most_recent_space = NULL;
	int				most_recent_space_w = -1;

	for ( ; curpos && *curpos != L'\0'; ++curpos )
	{
		wchar_t cmd[ 256 ];
		wchar_t args[ 256 ];

		if ( SplitCommand( &curpos, cmd, args ) )
		{
			if ( !wcscmp( cmd, L"cr" ) )
			{
				params.newline = true;
				AddWorkUnit( item, params);
			}
			else if ( !wcscmp( cmd, L"clr" ) )
			{
				AddWorkUnit( item, params );

				if ( args[0] == 0 && colorStack.Count()>= 2)
				{
					colorStack.Remove( colorStack.Count() - 1 );
					params.clr = colorStack[ colorStack.Count() - 1 ];
				}
				else
				{
					int r, g, b;
					Color newcolor;
					if ( 3 == swscanf( args, L"%i,%i,%i", &r, &g, &b ) )
					{
						newcolor = Color( r, g, b, 255 );
						colorStack.AddToTail( newcolor );
						params.clr = colorStack[ colorStack.Count() - 1 ];
					}
				}
			}
			else if ( !wcscmp( cmd, L"playerclr" ) )
			{
				AddWorkUnit( item, params );

				if ( args[0] == 0 && colorStack.Count()>= 2)
				{
					colorStack.Remove( colorStack.Count() - 1 );
					params.clr = colorStack[ colorStack.Count() - 1 ];
				}
				else
				{
					// player and npc color selector
					// e.g.,. 255,255,255:200,200,200
					int pr, pg, pb, nr, ng, nb;
					Color newcolor;
					if ( 6 == swscanf( args, L"%i,%i,%i:%i,%i,%i", &pr, &pg, &pb, &nr, &ng, &nb ) )
					{
						newcolor = item->IsFromPlayer() ? Color( pr, pg, pb, 255 ) : Color( nr, ng, nb, 255 );
						colorStack.AddToTail( newcolor );
						params.clr = colorStack[ colorStack.Count() - 1 ];
					}
				}
			}
			else if ( !wcscmp( cmd, L"I" ) )
			{
				AddWorkUnit( item, params );
				params.italic = !params.italic;
			}
			else if ( !wcscmp( cmd, L"B" ) )
			{
				AddWorkUnit( item, params );
				params.bold = !params.bold;
			}

			continue;
		}
#if defined( _XBOX )
		vgui::HFont useF = m_hFonts[ streamlen >= cc_smallfontlength.GetInt() ? CCFONT_SMALL : CCFONT_NORMAL ];
#else
		vgui::HFont useF = m_hFonts[ params.GetFontNumber() ];
#endif	

		params.font = useF;

		int w, h;

		wchar_t sz[2];
		sz[ 0 ] = *curpos;
		sz[ 1 ] = L'\0';
		vgui::surface()->GetTextSize( useF, sz, w, h );

		if ( ( params.x + params.width ) + w > available_width )
		{
			if ( most_recent_space && curpos >= most_recent_space + 1 )
			{
				// Roll back to previous space character if there is one...
				int goback = curpos - most_recent_space - 1;
				params.out -= ( goback + 1 );
				params.width = most_recent_space_w;
				
				wchar_t *extra = new wchar_t[ goback + 1 ];
				wcsncpy( extra, most_recent_space + 1, goback );
				extra[ goback ] = L'\0';

				params.newline = true;
				AddWorkUnit( item, params );

				wcsncpy( params.out, extra, goback );
				params.out += goback;
				int textw, texth;
				vgui::surface()->GetTextSize( useF, extra, textw, texth );

				params.width = textw;

				delete[] extra;

				most_recent_space = NULL;
				most_recent_space_w = -1;
			}
			else
			{
				params.newline = true;
				AddWorkUnit( item, params );
			}
		}
		*params.out++ = *curpos;
		params.width += w;

		if ( iswspace( *curpos ) )
		{
			most_recent_space = curpos;
			most_recent_space_w = params.width;
		}
	}

	// Add the final unit.
	params.newline = true;
	AddWorkUnit( item, params );

	item->SetSizeComputed( true );

	// DumpWork( item );
}

void CHudCloseCaption::	DumpWork( CCloseCaptionItem *item )
{
	int c = item->GetNumWorkUnits();
	for ( int i = 0 ; i < c; ++i )
	{
		CCloseCaptionWorkUnit *wu = item->GetWorkUnit( i );
		wu->Dump();
	}
}

void CHudCloseCaption::DrawStream( wrect_t &rcText, wrect_t &rcWindow, CCloseCaptionItem *item, int iFadeLine, float flFadeLineAlpha )
{
	int c = item->GetNumWorkUnits();

	wrect_t rcOut;

	float alpha = item->GetAlpha( m_flItemHiddenTime, m_flItemFadeInTime, m_flItemFadeOutTime );

	for ( int i = 0 ; i < c; ++i )
	{
		int x = 0;
		int y = 0;

		CCloseCaptionWorkUnit *wu = item->GetWorkUnit( i );
	
		vgui::HFont useF = wu->GetFont();

		wu->GetPos( x, y );

		rcOut.left = rcText.left + x + 3;
		rcOut.right = rcOut.left + wu->GetWidth();
		rcOut.top = rcText.top + y;
   		rcOut.bottom = rcOut.top + wu->GetHeight();

		// Adjust alpha to handle fade in/out at the top & bottom of the element.
		// Used for single commentary entries that are too big to fit into the element.
		float flLineAlpha = alpha;
		if ( i == iFadeLine )
		{
			flLineAlpha *= flFadeLineAlpha;
		}
		else if ( rcOut.top < 0 )
		{
			// We're off the top of the element, so don't draw
			continue;
		}
		else if ( rcOut.top > rcWindow.bottom )
		{
			float flFadeHeight = (float)wu->GetHeight() * 0.25;
			float flDist = (float)(rcOut.top - rcWindow.bottom) / flFadeHeight;
			flDist = Bias( flDist, 0.2 );
			if ( flDist > 1 )
				continue;

			flLineAlpha *= 1.0 - flDist;
		}

		Color useColor = wu->GetColor();

		useColor[ 3 ] *= flLineAlpha;

		if ( !item->IsValid() )
		{
			useColor = Color( 255, 255, 255, 255 * flLineAlpha );
			rcOut.right += 2;
			vgui::surface()->DrawSetColor( Color( 100, 100, 40, 255 * flLineAlpha ) );
			vgui::surface()->DrawFilledRect( rcOut.left, rcOut.top, rcOut.right, rcOut.bottom );
		}

		vgui::surface()->DrawSetTextFont( useF );
		vgui::surface()->DrawSetTextPos( rcOut.left, rcOut.top );
		vgui::surface()->DrawSetTextColor( useColor );
		vgui::surface()->DrawPrintText( wu->GetStream(), wcslen( wu->GetStream() ) );
	}
}

bool CHudCloseCaption::GetNoRepeatValue( const wchar_t *caption, float &retval )
{
	retval = 0.0f;
	const wchar_t *curpos = caption;
	
	for ( ; curpos && *curpos != L'\0'; ++curpos )
	{
		wchar_t cmd[ 256 ];
		wchar_t args[ 256 ];

		if ( SplitCommand( &curpos, cmd, args ) )
		{
			if ( !wcscmp( cmd, L"norepeat" ) )
			{
				retval = (float)wcstod( args, NULL );
				return true;
			}
			continue;
		}
	}
	return false;
}

bool CHudCloseCaption::CaptionTokenLessFunc( const CaptionRepeat &lhs, const CaptionRepeat &rhs )
{ 
	return ( lhs.m_nTokenIndex < rhs.m_nTokenIndex );	
}

#if !defined( _XBOX )
bool CHudCloseCaption::TokenNameLessFunc( const TokenNameLookup &lhs, const TokenNameLookup &rhs )
{
	return (unsigned int)lhs.crc < (unsigned int)rhs.crc;
}

static bool CaptionTrace( char const *token )
{
	static CUtlSymbolTable s_MissingCloseCaptions;

	// Make sure we only show the message once
	if ( UTL_INVAL_SYMBOL == s_MissingCloseCaptions.Find( token ) )
	{
		s_MissingCloseCaptions.AddString( token );
		return true;
	}

	return false;
}
#endif 

static ConVar cc_sentencecaptionnorepeat( "cc_sentencecaptionnorepeat", "4", 0, "How often a sentence can repeat." );

int CRCString( char const *str )
{
	int len = Q_strlen( str );
	CRC32_t crc;
	CRC32_Init( &crc );
	CRC32_ProcessBuffer( &crc, str, len );
	CRC32_Final( &crc );

	return ( int )crc;
}

class CAsyncCaption
{
public:
	CAsyncCaption() : 
		m_flDuration( 0.0f ),
		m_bIsStream( false ),
		m_bFromPlayer( false )
	{
	}

	~CAsyncCaption()
	{
		int c = m_Tokens.Count();
		for ( int i = 0; i < c; ++i )
		{
			delete m_Tokens[ i ];
		}
		m_Tokens.Purge();
	}

	void StartRequesting( CHudCloseCaption *hudCloseCaption, CUtlVector< AsyncCaption_t >& directories )
	{
		// Issue pending async requests for each token in string
		int c = m_Tokens.Count();
		for ( int i = 0; i < c; ++i )
		{
			caption_t *caption = m_Tokens[ i ];
			Assert( !caption->stream );
			Assert( caption->dirindex >= 0 );

			CaptionLookup_t& entry = directories[ caption->fileindex ].m_CaptionDirectory[ caption->dirindex ];

			// Request this block, and if it's there, it'll call OnDataLoaded immediately
			g_AsyncCaptionResourceManager.PollForAsyncLoading( hudCloseCaption, caption->fileindex, entry.blockNum );
		}
	}

	void OnDataArrived( CUtlVector< AsyncCaption_t >& directories, int nFileIndex, int nBlockNum, AsyncCaptionData_t *pData )
	{
		int c = m_Tokens.Count();
		for ( int i = 0; i < c; ++i )
		{
			caption_t *caption = m_Tokens[ i ];
			if ( caption->stream != NULL )
				continue;

			// Lookup the data
			CaptionLookup_t &entry = directories[ nFileIndex ].m_CaptionDirectory[ caption->dirindex ];
			if ( entry.blockNum != nBlockNum )
				continue;

			const wchar_t *pIn = ( const wchar_t *)&pData->m_pBlockData[ entry.offset ];
			caption->stream = new wchar_t[ entry.length >> 1 ];
			memcpy( (void *)caption->stream, pIn, entry.length );
		}
	}

	void ProcessAsyncWork( CHudCloseCaption *hudCloseCaption, CUtlVector< AsyncCaption_t >& directories )
	{
		int c = m_Tokens.Count();
		for ( int i = 0; i < c; ++i )
		{
			caption_t *caption = m_Tokens[ i ];
			if ( caption->stream != NULL )
				continue;

			CaptionLookup_t& entry = directories[ caption->fileindex].m_CaptionDirectory[ caption->dirindex ];

			// Request this block, and if it's there, it'll call OnDataLoaded immediately
			g_AsyncCaptionResourceManager.PollForAsyncLoading( hudCloseCaption, caption->fileindex, entry.blockNum );
		}
	}

	bool GetStream( wchar_t *buf, int bufSizeInBytes )
	{
		buf[ 0 ] = L'\0';

		int c = m_Tokens.Count();
		for ( int i = 0; i < c; ++i )
		{
			caption_t *caption = m_Tokens[ i ];
			if ( caption->stream == NULL )
			{
				return false;
			}
		}

		unsigned int curlen = 0;
		unsigned int maxlen = bufSizeInBytes / sizeof( wchar_t );

		// Compose full stream from tokens
		for ( int i = 0; i < c; ++i )
		{
			caption_t *caption = m_Tokens[ i ];
			int len = wcslen( caption->stream ) + 1;
			if ( curlen + len >= maxlen )
				break;

			wcscat( buf, caption->stream );
			if ( i < c - 1 ) 
			{
				wcscat( buf, L" " );
			}

			curlen += len;
		}

		return true;
	}

	bool					IsStream() const
	{
		return m_bIsStream;
	}

	void					SetIsStream( bool state )
	{
		m_bIsStream = state;
	}

	void	AddRandomToken( CUtlVector< AsyncCaption_t >& directories )
	{
		int dc = directories.Count();
		int fileindex = RandomInt( 0, dc - 1 );

		int c = directories[ fileindex ].m_CaptionDirectory.Count();
		int idx = RandomInt( 0, c - 1 );

		caption_t *caption = new caption_t;
		char foo[ 32 ];
		Q_snprintf( foo, sizeof( foo ), "%d", idx );
		caption->token = strdup( foo );
		caption->dirindex = idx;
		caption->stream = NULL;
		caption->fileindex = fileindex;

		m_Tokens.AddToTail( caption );
	}

	void AddToken
	( 
		CUtlVector< AsyncCaption_t >& directories, 
		char const *token 
	)
	{
		CaptionLookup_t search;
		search.SetHash( token );

		int idx = -1;
		int i;
		int dc = directories.Count();
		for ( i = 0; i < dc; ++i )
		{
            idx = directories[ i ].m_CaptionDirectory.Find( search );
			if ( idx == directories[ i ].m_CaptionDirectory.InvalidIndex() )
				continue;

			break;
		}

		if ( i >= dc || idx == -1 )
			return;

		caption_t *caption = new caption_t;
		caption->token = strdup( token );
		caption->dirindex = idx;
		caption->stream = NULL;
		caption->fileindex = i;

		m_Tokens.AddToTail( caption );
	}

	int						Count() const
	{
		return m_Tokens.Count();
	}

	char const				*GetToken( int index )
	{
		return m_Tokens[ index ]->token;
	}

	void					GetOriginalStream( char *buf, size_t bufsize )
	{
		buf[ 0 ] = 0;
		int c = Count();
		for ( int i = 0 ; i < c; ++i )
		{
			Q_strncat( buf, GetToken( i ), bufsize, COPY_ALL_CHARACTERS );
			if ( i != c - 1 )
			{
				Q_strncat( buf, " ", bufsize, COPY_ALL_CHARACTERS );
			}
		}
	}

	void					SetDuration( float t )
	{
		m_flDuration = t;
	}

	float					GetDuration()
	{
		return m_flDuration;
	}

	bool					IsFromPlayer()
	{
		return m_bFromPlayer;
	}

	void					SetFromPlayer( bool state )
	{
		m_bFromPlayer = state;
	}

	bool					IsDirect()
	{
		return m_bDirect;
	}

	void					SetDirect( bool state )
	{
		m_bDirect = state;
	}
private:
	float					m_flDuration;
	bool					m_bIsStream : 1;
	bool					m_bFromPlayer : 1;
	bool					m_bDirect : 1;

	struct caption_t
	{
		caption_t() :
			token( 0 ),
			dirindex( -1 ),
			fileindex( -1 ),
			stream( 0 )
		{
		}

		~caption_t()
		{
			free( token );
			delete[] stream;
		}

		void		SetStream( const wchar_t *in )
		{
			delete[] stream;
			stream = 0;
			if ( !in )
				return;

			int len = wcslen( in );
			stream = new wchar_t[ len + 1 ];
			wcsncpy( stream, in, len + 1 );
		}
			
		char		*token;
		int			dirindex;
		int			fileindex;
		wchar_t		*stream;
	};

	CUtlVector< caption_t * > m_Tokens;
};

void CHudCloseCaption::ProcessAsyncWork()
{
	int i;
	for( i = m_AsyncWork.Head(); i != m_AsyncWork.InvalidIndex(); i = m_AsyncWork.Next( i ) )
	{
		// check for data arrival
		CAsyncCaption *item = m_AsyncWork[ i ];
		item->ProcessAsyncWork( this, m_AsyncCaptions );
	}
	// Now operate on any new data which arrived
	for( i = m_AsyncWork.Head(); i != m_AsyncWork.InvalidIndex();  )
	{
		int n = m_AsyncWork.Next( i );

		CAsyncCaption *item = m_AsyncWork[ i ];
		wchar_t stream[ 2048 ];

		// If we get to the first item with pending async work, stop processing
		if ( !item->GetStream( stream, sizeof( stream ) ) )
		{
			break;
		}

		if ( stream[ 0 ] != L'\0' )
		{
			char original[ 512 ];
			item->GetOriginalStream( original, sizeof( original ) );

			// Process it now
			if ( item->IsStream() )
			{
				_ProcessSentenceCaptionStream( item->Count(), original, stream );
			}
			else
			{
				_ProcessCaption( stream, original, item->GetDuration(), item->IsFromPlayer(), item->IsDirect() );
			}
		}

		m_AsyncWork.Remove( i );
		delete item;

		i = n;
	}
}

void CHudCloseCaption::ClearAsyncWork()
{
	for ( int i = m_AsyncWork.Head(); i != m_AsyncWork.InvalidIndex(); i = m_AsyncWork.Next( i ) )
	{
        CAsyncCaption *item = m_AsyncWork[ i ];
		delete item;
	}
	m_AsyncWork.Purge();
}

extern void Hack_FixEscapeChars( char *str );

void CHudCloseCaption::ProcessCaptionDirect( char const *tokenname, float duration, bool fromplayer /* = false */ )
{
	m_bVisibleDueToDirect = true;

	char token[ 512 ];
	Q_strncpy( token, tokenname, sizeof( token ) );
	if ( Q_strstr( token, "\\" ) )
	{
		Hack_FixEscapeChars( token );
	}

	ProcessCaption( token, duration, fromplayer, true );
}

void CHudCloseCaption::PlayRandomCaption()
{
	CAsyncCaption *async = new CAsyncCaption;
	async->SetIsStream( false );
	async->AddRandomToken( m_AsyncCaptions );
	async->SetDuration( RandomFloat( 1.0f, 3.0f ) );
	async->SetFromPlayer( RandomInt( 0, 1 ) == 0 ? true : false );
	async->StartRequesting( this, m_AsyncCaptions );
	m_AsyncWork.AddToTail( async );
}

void CHudCloseCaption::AddAsyncWork( char const *tokenstream, bool bIsStream, float duration, bool fromplayer, bool direct /* = false */ )
{
	CAsyncCaption *async = new CAsyncCaption();
	async->SetIsStream( bIsStream );
	async->SetDirect( direct );
	if ( !bIsStream )
	{
		async->AddToken
		( 
			m_AsyncCaptions, 
			tokenstream 
		);
	}
	else
	{
		// The first token from the stream is the name of the sentence
		char tokenname[ 512 ];
		tokenname[ 0 ] = 0;
		char const *p = tokenstream;
		p = nexttoken( tokenname, p, ' ' );
		// p points to reset of sentence tokens, build up a unicode string from them...
		while ( p && Q_strlen( tokenname ) > 0 )
		{
			p = nexttoken( tokenname, p, ' ' );

			if ( Q_strlen( tokenname ) == 0 )
				break;

			async->AddToken
			( 
					m_AsyncCaptions, 
					tokenname 
			);
		}
	}

	m_AsyncWork.AddToTail( async );

	async->SetDuration( duration );
	async->SetFromPlayer( fromplayer );
	// Do this last as the block might be resident already and this will finish immediately...
	async->StartRequesting( this, m_AsyncCaptions );
}


void CHudCloseCaption::ProcessSentenceCaptionStream( char const *tokenstream )
{
	float interval = cc_sentencecaptionnorepeat.GetFloat();
	interval = clamp( interval, 0.1f, 60.0f );

	// The first token from the stream is the name of the sentence
	char tokenname[ 512 ];

	tokenname[ 0 ] = 0;

	char const *p = tokenstream;

	p = nexttoken( tokenname, p, ' ' );

	if ( Q_strlen( tokenname ) > 0 )
	{
		//  Use it to check for "norepeat" rules
		CaptionRepeat entry;
		entry.m_nTokenIndex = CRCString( tokenname );

		int idx = m_CloseCaptionRepeats.Find( entry );
		if ( m_CloseCaptionRepeats.InvalidIndex() == idx )
		{
			entry.m_flLastEmitTime = gpGlobals->curtime;
			entry.m_nLastEmitTick = gpGlobals->tickcount;
			entry.m_flInterval = interval;
			m_CloseCaptionRepeats.Insert( entry );
		}
		else
		{
			CaptionRepeat &entry = m_CloseCaptionRepeats[ idx ];
			if ( gpGlobals->curtime < ( entry.m_flLastEmitTime + entry.m_flInterval ) )
			{
				return;
			}

			entry.m_flLastEmitTime = gpGlobals->curtime;
			entry.m_nLastEmitTick = gpGlobals->tickcount;
		}
	}

	AddAsyncWork( tokenstream, true, 0.0f, false );
}

void CHudCloseCaption::_ProcessSentenceCaptionStream( int wordCount, char const *tokenstream, const wchar_t *caption_full )
{
	if ( wcslen( caption_full ) > 0 )
	{
		Process( caption_full, ( wordCount + 1 ) * 0.75f, tokenstream, false /*never from player!*/ );
	}
}

void CHudCloseCaption::ProcessCaption( char const *tokenname, float duration, bool fromplayer /* = false */, bool direct /* = false */ )
{
	AddAsyncWork( tokenname, false, duration, fromplayer, direct );
}

void CHudCloseCaption::_ProcessCaption( const wchar_t *caption, char const *tokenname, float duration, bool fromplayer, bool direct )
{
	// Get the string for the token
	float interval = 0.0f;
	bool hasnorepeat = GetNoRepeatValue( caption, interval );

	CaptionRepeat entry;
	entry.m_nTokenIndex = CRCString( tokenname );

	int idx = m_CloseCaptionRepeats.Find( entry );
	if ( m_CloseCaptionRepeats.InvalidIndex() == idx )
	{
		entry.m_flLastEmitTime = gpGlobals->curtime;
		entry.m_nLastEmitTick = gpGlobals->tickcount;
		entry.m_flInterval = interval;
		m_CloseCaptionRepeats.Insert( entry );
	}
	else
	{
		CaptionRepeat &entry = m_CloseCaptionRepeats[ idx ];

		// Interval of 0.0 means just don't double emit on same tick #
		if ( entry.m_flInterval <= 0.0f )
		{
			if ( gpGlobals->tickcount <= entry.m_nLastEmitTick )
			{
				return;
			}
		}
		else if ( hasnorepeat )
		{
			if ( gpGlobals->curtime < ( entry.m_flLastEmitTime + entry.m_flInterval ) )
			{
				return;
			}
		}

		entry.m_flLastEmitTime = gpGlobals->curtime;
		entry.m_nLastEmitTick = gpGlobals->tickcount;
	}

	Process( caption, duration, tokenname, fromplayer, direct );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pszName - 
//			iSize - 
//			*pbuf - 
//-----------------------------------------------------------------------------
void CHudCloseCaption::MsgFunc_CloseCaption(bf_read &msg)
{
#if !defined( _XBOX )
	CRC32_t crc = (CRC32_t)msg.ReadLong();
#else
	char tokenname[ 128 ];
	msg.ReadString( tokenname, sizeof( tokenname ) );
#endif
	float duration = msg.ReadShort() * 0.1f;
	byte flagbyte = msg.ReadByte();
	bool warnonmissing = flagbyte & CLOSE_CAPTION_WARNIFMISSING ? true : false;
	bool fromplayer = flagbyte & CLOSE_CAPTION_FROMPLAYER ? true : false;
	bool bIsMale = flagbyte & CLOSE_CAPTION_GENDER_MALE ? true : false;
	bool bIsFemale = flagbyte & CLOSE_CAPTION_GENDER_FEMALE ? true : false;

#if !defined( _XBOX )
	TokenNameLookup lookup;
	lookup.crc = crc;

	int idx = m_TokenNameLookup.Find( lookup );
	if ( idx == m_TokenNameLookup.InvalidIndex() )
	{
		if ( warnonmissing )
		{
			// Send message to server to spew warning
			engine->ClientCmd( VarArgs( "cc_lookup_crc %u\n", (unsigned int)crc ) );
		}
		return;
	}

	char const *tokenname = vgui::localize()->GetNameByIndex( m_TokenNameLookup[ idx ].stringIndex );
	if ( !tokenname )
	{
		Warning( "MsgFunc_CloseCaption to find token name for index %i\n", m_TokenNameLookup[ idx ].stringIndex );
		return;
	}
#endif
	char szTestName[ 512 ];
	if ( bIsMale || bIsFemale )
	{
		Q_snprintf( szTestName, sizeof( szTestName ), "%s_%s", tokenname, bIsMale ? "male" : "female" );
		// see if it exists
		if ( vgui::localize()->Find( szTestName ) )
		{
			// the gender-ified version exists, use it, otherwise fall through and pass the non-gender string to the cc system for processing
			ProcessCaption( szTestName , duration, fromplayer );	
			return;
		}		
	}

	ProcessCaption( tokenname, duration, fromplayer );	
}

int CHudCloseCaption::GetFontNumber( bool bold, bool italic )
{
#if !defined( _XBOX )
	if ( bold || italic )
	{
		if( bold && italic )
		{
			return CHudCloseCaption::CCFONT_ITALICBOLD;
		}

		if ( bold )
		{
			return CHudCloseCaption::CCFONT_BOLD;
		}

		if ( italic )
		{
			return CHudCloseCaption::CCFONT_ITALIC;
		}
	}
#endif
	return CHudCloseCaption::CCFONT_NORMAL;
}

void CHudCloseCaption::Flush()
{
	g_AsyncCaptionResourceManager.Flush();
}

void CHudCloseCaption::InitCaptionDictionary( char const *dbfile )
{
	if ( m_CurrentLanguage.IsValid() && !Q_stricmp( m_CurrentLanguage.String(), dbfile ) )
		return;

	m_CurrentLanguage = dbfile;

	m_AsyncCaptions.Purge();

	g_AsyncCaptionResourceManager.Clear();

	char searchPaths[ 512 ];
	filesystem->GetSearchPath( "GAME", true, searchPaths, sizeof( searchPaths ) );

	char *path = strtok( searchPaths, ";" );
	while ( path )
	{
		char fullpath[ 512 ];
		Q_snprintf( fullpath, sizeof( fullpath ), "%s%s", path, dbfile );
		Q_FixSlashes( fullpath );
		Q_strlower( fullpath );

		path = strtok( NULL, ";" );

        FileHandle_t fh = filesystem->Open( fullpath, "rb" );
		if ( FILESYSTEM_INVALID_HANDLE != fh )
		{
			CUtlBuffer dirbuffer;

			AsyncCaption_t& entry = m_AsyncCaptions[ m_AsyncCaptions.AddToTail() ];

			// Read the header
			filesystem->Read( &entry.m_Header, sizeof( entry.m_Header ), fh );
			if ( entry.m_Header.magic != COMPILED_CAPTION_FILEID )
				Error( "Invalid file id for %s\n", fullpath );
			if ( entry.m_Header.version != COMPILED_CAPTION_VERSION )
				Error( "Invalid file version for %s\n", fullpath );
			if ( entry.m_Header.directorysize < 0 || entry.m_Header.directorysize > 64 * 1024 )
				Error( "Invalid directory size %d for %s\n", entry.m_Header.directorysize, fullpath );
			//if ( entry.m_Header.blocksize != MAX_BLOCK_SIZE )
			//	Error( "Invalid block size %d, expecting %d for %s\n", entry.m_Header.blocksize, MAX_BLOCK_SIZE, fullpath );

			int directoryBytes = entry.m_Header.directorysize * sizeof( CaptionLookup_t );
			entry.m_CaptionDirectory.EnsureCapacity( entry.m_Header.directorysize );
			dirbuffer.EnsureCapacity( directoryBytes );
			
			filesystem->Read( dirbuffer.Base(), directoryBytes, fh );
			filesystem->Close( fh );

			entry.m_CaptionDirectory.CopyArray( (const CaptionLookup_t *)dirbuffer.PeekGet(), entry.m_Header.directorysize );
			entry.m_CaptionDirectory.RedoSort( true );
			
			entry.m_DataBaseFile = fullpath;
		}
	}

	g_AsyncCaptionResourceManager.SetDbInfo( m_AsyncCaptions );
}

void CHudCloseCaption::OnFinishAsyncLoad( int nFileIndex, int nBlockNum, AsyncCaptionData_t *pData )
{
	// Fill in data for all users of pData->m_nBlockNum
	FOR_EACH_LL( m_AsyncWork, i )
	{
		CAsyncCaption *item = m_AsyncWork[ i ];
		item->OnDataArrived( m_AsyncCaptions, nFileIndex, nBlockNum, pData );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudCloseCaption::Lock( void )
{
	if ( !IsXbox() )
		m_bLocked = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudCloseCaption::Unlock( void )
{
	m_bLocked = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#if !defined( _XBOX )
void CHudCloseCaption::BuildTokenNameLookup()
{
	vgui::StringIndex_t i = vgui::localize()->GetFirstStringIndex();
	while ( vgui::INVALID_STRING_INDEX != i )
	{
		TokenNameLookup lookup;

		CRC32_t crc;
		CRC32_Init( &crc );
		char const *tokenName = vgui::localize()->GetNameByIndex( i );

		char lowercase[ 256 ];
		Q_strncpy( lowercase, tokenName, sizeof( lowercase ) );
		Q_strlower( lowercase );

		CRC32_ProcessBuffer( &crc, lowercase, Q_strlen( lowercase ) );
		CRC32_Final( &crc );

		lookup.crc = crc;
		lookup.stringIndex = i;

		if ( m_TokenNameLookup.Find( lookup ) == m_TokenNameLookup.InvalidIndex() )
		{
			m_TokenNameLookup.Insert( lookup );
		}

		i = vgui::localize()->GetNextStringIndex( i );
	}
}
#endif

static int EmitCaptionCompletion( const char *partial, char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ] )
{
	int current = 0;
	if ( !vgui::localize() )
		return current;

	const char *cmdname = "cc_emit";
	char *substring = NULL;
	int substringLen = 0;
	if ( Q_strstr( partial, cmdname ) && strlen(partial) > strlen(cmdname) + 1 )
	{
		substring = (char *)partial + strlen( cmdname ) + 1;
		substringLen = strlen(substring);
	}
	
	vgui::StringIndex_t i = vgui::localize()->GetFirstStringIndex();

	while ( i != vgui::INVALID_STRING_INDEX &&
		 current < COMMAND_COMPLETION_MAXITEMS )
	{
		char const *ccname = vgui::localize()->GetNameByIndex( i );
		if ( ccname )
		{
			if ( !substring || !Q_strncasecmp( ccname, substring, substringLen ) )
			{
				Q_snprintf( commands[ current ], sizeof( commands[ current ] ), "%s %s", cmdname, ccname );
				current++;
			}
		}
		i = vgui::localize()->GetNextStringIndex( i );
	}

	return current;
}

void EmitCaption_f()
{
	if ( engine->Cmd_Argc() != 2 )
	{
		Msg( "usage:  cc_emit tokenname\n" );
		return;
	}

	CHudCloseCaption *hudCloseCaption = GET_HUDELEMENT( CHudCloseCaption );
	if ( hudCloseCaption )
	{
		hudCloseCaption->ProcessCaption( engine->Cmd_Argv(1), 5.0f );	
	}
}

static ConCommand cc_emit( "cc_emit", EmitCaption_f, "Emits a closed caption", 0, EmitCaptionCompletion );

void EmitRandomCaption_f()
{
	int count = 1;
	if ( engine->Cmd_Argc() == 2 )
	{
		count = max( 1, atoi( engine->Cmd_Argv( 1 ) ) );
	}
	CHudCloseCaption *hudCloseCaption = GET_HUDELEMENT( CHudCloseCaption );
	if ( hudCloseCaption )
	{
		for ( int i = 0; i < count; ++i )
		{
			hudCloseCaption->PlayRandomCaption();
		}
	}
}

static ConCommand cc_random( "cc_random", EmitRandomCaption_f, "Emits a random caption" );

CON_COMMAND( cc_flush, "Flushes async'd captions." )
{
	CHudCloseCaption *hudCloseCaption = GET_HUDELEMENT( CHudCloseCaption );
	if ( hudCloseCaption )
	{
		hudCloseCaption->Flush();
	}
}

CON_COMMAND( cc_showblocks, "Toggles showing which blocks are pending/loaded async." )
{
	CHudCloseCaption *hudCloseCaption = GET_HUDELEMENT( CHudCloseCaption );
	if ( hudCloseCaption )
	{
		hudCloseCaption->TogglePaintDebug();
	}
}

void OnCaptionLanguageChanged( ConVar *var, char const *pOldString )
{
	if ( !vgui::localize() )
		return;

	char fn[ 512 ];
	Q_snprintf( fn, sizeof( fn ), "resource/closecaption_%s.txt", var->GetString() );

	// Re-adding the file, even if it's "english" will overwrite the tokens as needed
	vgui::localize()->AddFileEx( vgui::filesystem(), "resource/closecaption_%language%.txt", "GAME", true );

	char uilanguage[ 64 ];
	engine->GetUILanguage( uilanguage, sizeof( uilanguage ) );

	CHudCloseCaption *hudCloseCaption = GET_HUDELEMENT( CHudCloseCaption );

	// If it's not the default, load the language on top of the user's default language
	if ( Q_strlen( var->GetString() ) > 0 && Q_stricmp( var->GetString(), uilanguage ) )
	{
		if ( vgui::filesystem()->FileExists( fn ) )
		{
			vgui::localize()->AddFileEx( vgui::filesystem(), fn, "GAME", true );
		}
		else
		{
			char fallback[ 512 ];
			Q_snprintf( fallback, sizeof( fallback ), "resource/closecaption_%s.txt", uilanguage );

			Msg( "%s not found\n", fn );
			Msg( "%s will be used\n", fallback );
		}

		if ( hudCloseCaption )
		{
			char dbfile [ 512 ];
			Q_snprintf( dbfile, sizeof( dbfile ), "resource/closecaption_%s.dat", var->GetString() );
			hudCloseCaption->InitCaptionDictionary( dbfile );
		}
	}
	else
	{
		if ( hudCloseCaption )
		{
			char dbfile [ 512 ];
			Q_snprintf( dbfile, sizeof( dbfile ), "resource/closecaption_%s.dat", uilanguage );
			hudCloseCaption->InitCaptionDictionary( dbfile );
		}
	}
	DevMsg( "cc_lang = %s\n", var->GetString() );
}



ConVar cc_lang( "cc_lang", "", FCVAR_ARCHIVE, "Current close caption language (emtpy = use game UI language)", OnCaptionLanguageChanged );