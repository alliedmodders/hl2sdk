//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef EP1_GAMESTATS_H
#define EP1_GAMESTATS_H
#ifdef _WIN32
#pragma once
#endif

class CUtlBuffer;
#include "tier1/UtlDict.h"

#define GAMESTATS_FILE_VERSION_OLD		001
#define GAMESTATS_FILE_VERSION_OLD2		002
#define GAMESTATS_FILE_VERSION_OLD3		003
#define GAMESTATS_FILE_VERSION_OLD4		004
#define GAMESTATS_FILE_VERSION_OLD5		005
#define GAMESTATS_FILE_VERSION			006

struct GameStatsRecord_t
{
public:
	GameStatsRecord_t() :
	  	m_nCount( 0 ),
		m_nSeconds( 0 ),
		m_nCommentary( 0 ),
		m_nHDR( 0 ),
		m_nCaptions( 0 ),
		m_bSteam( true ),
		m_bCyberCafe( false ),
		m_nDeaths( 0 )
	{
		Q_memset( m_nSkill, 0, sizeof( m_nSkill ) );
	}

	void		Clear();

	void		SaveToBuffer( CUtlBuffer& buf );
	bool		ParseFromBuffer( CUtlBuffer& buf );

// Data
public:
	int			m_nCount;
	int			m_nSeconds;

	int			m_nCommentary;
	int			m_nHDR;
	int			m_nCaptions;
	int			m_nSkill[ 3 ];
	bool		m_bSteam;
	bool		m_bCyberCafe;
	int			m_nDeaths;
};

struct GameStats_t
{
public:
	GameStats_t() :
	  m_nVersion( GAMESTATS_FILE_VERSION ),
	  m_nSecondsToCompleteGame( 0 ),
  		m_nHL2ChaptureUnlocked( 0 ),
		m_bSteam( true ),
		m_bCyberCafe( false ),
		m_nDXLevel( 0 )
	{
		m_szUserID[ 0 ] = 0;
	}

	void						Clear();

	void						SaveToBuffer( CUtlBuffer& buf );
	bool						ParseFromBuffer( CUtlBuffer& buf );

	GameStatsRecord_t			*FindOrAddRecordForMap( char const *mapname );

// Data
public:
	int							m_nVersion;
	char						m_szUserID[ 17 ];	// GUID
	int							m_nSecondsToCompleteGame; // 0 means they haven't finished playing yet

	GameStatsRecord_t			m_Summary;			// Summary record
	CUtlDict< GameStatsRecord_t, unsigned short > m_MapTotals;
	bool						m_bSteam;
	bool						m_bCyberCafe;
	int							m_nHL2ChaptureUnlocked;
	int							m_nDXLevel;
};

#endif // EP1_GAMESTATS_H
