//===== Copyright © 1996-2006, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//===========================================================================//


#ifndef COMMANDBUFFER_H
#define COMMANDBUFFER_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h"
#include "tier1/utlstring.h"
#include "tier1/utlvector.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CUtlBuffer;


//-----------------------------------------------------------------------------
// Invalid command handle
//-----------------------------------------------------------------------------
typedef intp CommandHandle_t;
enum
{
	COMMAND_BUFFER_INVALID_COMMAND_HANDLE = 0
};


//-----------------------------------------------------------------------------
// A command buffer class- a queue of argc/argv based commands associated
// with a particular time
//-----------------------------------------------------------------------------
class CCommandBuffer
{
public:
	// Constructor, destructor
	DLL_CLASS_IMPORT CCommandBuffer();
	DLL_CLASS_IMPORT ~CCommandBuffer();

	// Inserts text into the command buffer
	DLL_CLASS_IMPORT bool AddText( const char *pText, int nSource = 0, int nTickDelay = 0, bool unk3 = false, double unk4 = 0.0, uint64 nRequiredFlags = 0 );

	// Used to iterate over all commands appropriate for the current time
	DLL_CLASS_IMPORT void BeginProcessingCommands( int nDeltaTicks );
	DLL_CLASS_IMPORT bool DequeueNextCommand();
	DLL_CLASS_IMPORT int DequeueNextCommand( const char **&ppArgv );
	DLL_CLASS_IMPORT void EndProcessingCommands();

	// Are we in the middle of processing commands?
	DLL_CLASS_IMPORT bool IsProcessingCommands();

	// Delays all queued commands to execute at a later time
	DLL_CLASS_IMPORT void DelayAllQueuedCommands( int nTickDelay );

	// Indicates how long to delay when encountering a 'wait' command
	DLL_CLASS_IMPORT void SetWaitDelayTime( int nTickDelay );

	// Splits pText into individual commands, appending each to pOut.
	DLL_CLASS_IMPORT static void SplitCommands( const char *pText, int nLength, CUtlVector< CUtlString > *pOut );

	// Returns a handle to the next command to process
	// (useful when inserting commands into the buffer during processing
	// of commands to force immediate execution of those commands,
	// most relevantly, to implement a feature where you stream a file
	// worth of commands into the buffer, where the file size is too large
	// to entirely contain in the buffer).
	DLL_CLASS_IMPORT CommandHandle_t GetNextCommandHandle();

	// Specifies a max limit of the args buffer. For unittesting. Size == 0 means use default
	DLL_CLASS_IMPORT void LimitArgumentBufferSize( int nSize );

	// Sets the flag mask a command must satisfy to be dequeued. Returns the previous mask.
	DLL_CLASS_IMPORT uint64 SetRequiredFlags( uint64 nRequiredFlags );

	// Locks/unlocks the command buffer.
	DLL_CLASS_IMPORT void LockCommandBuffer( bool bLock );

private:
	enum
	{
		ARGS_BUFFER_LENGTH = 0x8000,
	};

	char			m_ArgSBuffer[ ARGS_BUFFER_LENGTH ];	// 0x0000
	CommandHandle_t	m_hNextCommand;				// 0x8000
	uint8			m_unk001[ 0x30 ];			// 0x8008
	uint64			m_nRequiredFlags;			// 0x8038
	uint8			m_unk002[ 0x18 ];			// 0x8040
	int			    m_nWaitDelayTicks;			// 0x8058
	int			    m_nMaxArgSBufferLength;	    // 0x805C
	uint8			m_unk003[ 0x02 ];			// 0x8060
	bool			m_bIsLocked;				// 0x8062
	uint8			m_unk004[ 0x15 ];			// 0x8063
	char			*m_pArgSCursor;				// 0x8078
	uint8			m_unk005[ 0x420 ];			// 0x8080  args pool / command list
	int			    m_nArgc;					// 0x84A0
	uint8			m_unk006[ 0x04 ];			// 0x84A4
	const char		**m_ppArgv;					// 0x84A8
	uint8			m_unk007[ 0x208 ];			// 0x84B0
	bool			m_bIsProcessingCommands;	// 0x86B8
	uint8			m_unk008[ 0x07 ];			// 0x86B9
	double			m_unk009;					// 0x86C0
	uint8			m_unk010[ 0x10 ];			// 0x86C8
};												// sizeof == 0x86D8

#endif // COMMANDBUFFER_H
