//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef NETWORKSTRINGTABLEDEFS_H
#define NETWORKSTRINGTABLEDEFS_H
#ifdef _WIN32
#pragma once
#endif

#include "appframework/IAppSystem.h"
#include "utlstring.h"

typedef int TABLEID;

#define INVALID_STRING_TABLE -1
const unsigned int INVALID_STRING_INDEX = -1;

// table index is sent in log2(MAX_TABLES) bits
#define MAX_TABLES	32  // Table id is 4 bits

#define INTERFACENAME_NETWORKSTRINGTABLESERVER "Source2EngineToServerStringTable001"
#define INTERFACENAME_NETWORKSTRINGTABLECLIENT "Source2EngineToClientStringTable001"

class StringTableInit_t;
class INetworkStringTable;

//-----------------------------------------------------------------------------
// Purpose: Game .dll shared string table interfaces
//-----------------------------------------------------------------------------
class INetworkStringTable
{
public:

	virtual					~INetworkStringTable( void ) {};
		
	// Table Info
	virtual const char		*GetTableName( void ) const = 0;
	virtual TABLEID			GetTableId( void ) const = 0;
	virtual int				GetNumStrings( void ) const = 0;

	// Networking
	virtual int				SetTick( int tick, void *unknown ) = 0;
	virtual int				GetTick( void ) = 0;
	virtual bool			ChangedBetweenTicks(int tickA, int tickB ) const = 0;

	virtual int				AddString( bool bIsServer, const char *value, const void *userdata = 0 ) = 0; 

	virtual const char		*GetString( int stringNumber ) const = 0;
	virtual bool			SetStringUserData( int stringNumber, const void *userdata, bool unknown ) = 0;
	virtual const void		*GetStringUserData( int stringNumber ) const = 0;
	virtual int				FindStringIndex( char const *string ) = 0; // returns INVALID_STRING_INDEX if not found
	virtual void			unk001() = 0;
	virtual void			SetAllowClientSideAddString( bool state ) = 0;
	virtual void			unk003() = 0;
	virtual void			unk004( const char *string ) const = 0; // all stringtables in engine/server set this to "[server]".
	virtual void			unk005() = 0; // likely SetStringChangedCallback
};

enum ENetworkStringtableFlags
{
	NSF_NONE = 0,
	NSF_DICTIONARY_ENABLED  = (1<<0), // Uses pre-calculated per map dictionaries to reduce bandwidth
};

class INetworkStringTableContainer: public IAppSystem
{
public:
	
	virtual					~INetworkStringTableContainer( void ) {};
	
	// table creation/destruction
	virtual INetworkStringTable	*CreateStringTable( StringTableInit_t *initClass ) = 0;
	virtual void				RemoveAllTables( void ) = 0;
	
	// table infos
	virtual INetworkStringTable	*FindTable( const char *tableName ) const = 0;
	virtual INetworkStringTable	*GetTable( TABLEID stringTable ) const = 0;
	virtual int					GetNumTables( void ) const = 0;
};

#endif // NETWORKSTRINGTABLEDEFS_H
