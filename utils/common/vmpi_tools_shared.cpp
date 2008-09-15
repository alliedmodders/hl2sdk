//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include <windows.h>
#include "vmpi.h"
#include "cmdlib.h"
#include "vmpi_tools_shared.h"
#include "vstdlib/strtools.h"
#include "mpi_stats.h"
#include "iphelpers.h"


// ----------------------------------------------------------------------------- //
// Globals.
// ----------------------------------------------------------------------------- //

static bool g_bReceivedDirectoryInfo = false;	// Have we gotten the qdir info yet?

static bool g_bReceivedDBInfo = false;
static CDBInfo g_DBInfo;
static unsigned long g_JobPrimaryID;

static int g_nDisconnects = 0;	// Tracks how many remote processes have disconnected ungracefully.


// ----------------------------------------------------------------------------- //
// Shared dispatch code.
// ----------------------------------------------------------------------------- //

bool SharedDispatch( MessageBuffer *pBuf, int iSource, int iPacketID )
{
	char *pInPos = &pBuf->data[2];

	switch ( pBuf->data[1] )
	{
		case VMPI_SUBPACKETID_DIRECTORIES:
		{
			Q_strncpy( gamedir, pInPos, sizeof( gamedir ) );
			pInPos += strlen( pInPos ) + 1;

			Q_strncpy( qdir, pInPos, sizeof( qdir ) );
			
			g_bReceivedDirectoryInfo = true;
		}
		return true;

		case VMPI_SUBPACKETID_DBINFO:
		{
			g_DBInfo = *((CDBInfo*)pInPos);
			pInPos += sizeof( CDBInfo );
			g_JobPrimaryID = *((unsigned long*)pInPos);

			g_bReceivedDBInfo = true;
		}
		return true;

		case VMPI_SUBPACKETID_CRASH:
		{
			Warning( "\nWorker '%s' dead: %s\n", VMPI_GetMachineName( iSource ), pInPos );
		}
		return true;
	}

	return false;
}

CDispatchReg g_SharedDispatchReg( VMPI_SHARED_PACKET_ID, SharedDispatch );



// ----------------------------------------------------------------------------- //
// Module interfaces.
// ----------------------------------------------------------------------------- //

void SendQDirInfo()
{
	char cPacketID[2] = { VMPI_SHARED_PACKET_ID, VMPI_SUBPACKETID_DIRECTORIES };

	MessageBuffer mb;
	mb.write( cPacketID, 2 );
	mb.write( gamedir, strlen( gamedir ) + 1 );
	mb.write( qdir, strlen( qdir ) + 1 );

	VMPI_SendData( mb.data, mb.getLen(), VMPI_PERSISTENT );
}


void RecvQDirInfo()
{
	while ( !g_bReceivedDirectoryInfo )
		VMPI_DispatchNextMessage();
}


void SendDBInfo( const CDBInfo *pInfo, unsigned long jobPrimaryID )
{
	char cPacketInfo[2] = { VMPI_SHARED_PACKET_ID, VMPI_SUBPACKETID_DBINFO };
	const void *pChunks[] = { cPacketInfo, pInfo, &jobPrimaryID };
	int chunkLengths[] = { 2, sizeof( CDBInfo ), sizeof( jobPrimaryID ) };
	
	VMPI_SendChunks( pChunks, chunkLengths, ARRAYSIZE( pChunks ), VMPI_PERSISTENT );
}


void RecvDBInfo( CDBInfo *pInfo, unsigned long *pJobPrimaryID )
{
	while ( !g_bReceivedDBInfo )
		VMPI_DispatchNextMessage();

	*pInfo = g_DBInfo;
	*pJobPrimaryID = g_JobPrimaryID;
}


void VMPI_HandleCrash( const char *pMessage, bool bAssert )
{
	static LONG crashHandlerCount = 0;
	if ( InterlockedIncrement( &crashHandlerCount ) == 1 )
	{
		Msg( "\nFAILURE: '%s' (assert: %d)\n", pMessage, bAssert );

		// Send a message to the master.
		char crashMsg[2] = { VMPI_SHARED_PACKET_ID, VMPI_SUBPACKETID_CRASH };

		VMPI_Send2Chunks( 
			crashMsg, 
			sizeof( crashMsg ), 
			pMessage,
			strlen( pMessage ) + 1,
			VMPI_MASTER_ID );

		// Let the messages go out.
		Sleep( 500 );
	}

	InterlockedDecrement( &crashHandlerCount );
}


// This is called if we crash inside our crash handler. It just terminates the process immediately.
LONG __stdcall VMPI_SecondExceptionFilter( struct _EXCEPTION_POINTERS *ExceptionInfo )
{
	TerminateProcess( GetCurrentProcess(), 2 );
	return EXCEPTION_EXECUTE_HANDLER; // (never gets here anyway)
}


void VMPI_ExceptionFilter( unsigned long code )
{
	// This is called if we crash inside our crash handler. It just terminates the process immediately.
	SetUnhandledExceptionFilter( VMPI_SecondExceptionFilter );

	//DWORD code = ExceptionInfo->ExceptionRecord->ExceptionCode;

	#define ERR_RECORD( name ) { name, #name }
	struct
	{
		int code;
		char *pReason;
	} errors[] =
	{
		ERR_RECORD( EXCEPTION_ACCESS_VIOLATION ),
		ERR_RECORD( EXCEPTION_ARRAY_BOUNDS_EXCEEDED ),
		ERR_RECORD( EXCEPTION_BREAKPOINT ),
		ERR_RECORD( EXCEPTION_DATATYPE_MISALIGNMENT ),
		ERR_RECORD( EXCEPTION_FLT_DENORMAL_OPERAND ),
		ERR_RECORD( EXCEPTION_FLT_DIVIDE_BY_ZERO ),
		ERR_RECORD( EXCEPTION_FLT_INEXACT_RESULT ),
		ERR_RECORD( EXCEPTION_FLT_INVALID_OPERATION ),
		ERR_RECORD( EXCEPTION_FLT_OVERFLOW ),
		ERR_RECORD( EXCEPTION_FLT_STACK_CHECK ),
		ERR_RECORD( EXCEPTION_FLT_UNDERFLOW ),
		ERR_RECORD( EXCEPTION_ILLEGAL_INSTRUCTION ),
		ERR_RECORD( EXCEPTION_IN_PAGE_ERROR ),
		ERR_RECORD( EXCEPTION_INT_DIVIDE_BY_ZERO ),
		ERR_RECORD( EXCEPTION_INT_OVERFLOW ),
		ERR_RECORD( EXCEPTION_INVALID_DISPOSITION ),
		ERR_RECORD( EXCEPTION_NONCONTINUABLE_EXCEPTION ),
		ERR_RECORD( EXCEPTION_PRIV_INSTRUCTION ),
		ERR_RECORD( EXCEPTION_SINGLE_STEP ),
		ERR_RECORD( EXCEPTION_STACK_OVERFLOW ),
		ERR_RECORD( EXCEPTION_ACCESS_VIOLATION ),
	};

	int nErrors = sizeof( errors ) / sizeof( errors[0] );
	int i = 0;
	for ( i = 0; i < nErrors; i++ )
	{
		if ( errors[i].code == code )
			VMPI_HandleCrash( errors[i].pReason, true );
	}

	if ( i == nErrors )
	{
		VMPI_HandleCrash( "Unknown reason", true );
	}

	TerminateProcess( GetCurrentProcess(), 1 );
}


void HandleMPIDisconnect( int procID, const char *pReason )
{
	int nLiveWorkers = VMPI_GetCurrentNumberOfConnections() - g_nDisconnects - 1;

	bool bOldSuppress = g_bSuppressPrintfOutput;
	g_bSuppressPrintfOutput = true;

		Warning( "\n\n--- WARNING: lost connection to '%s' (%s).\n", VMPI_GetMachineName( procID ), pReason );
		
		if ( g_bMPIMaster )
		{
			Warning( "%d workers remain.\n\n", nLiveWorkers );

			++g_nDisconnects;
			/*
			if ( VMPI_GetCurrentNumberOfConnections() - g_nDisconnects <= 1 )
			{
				Error( "All machines disconnected!" );
			}
			*/
		}
		else
		{
			Error( "Worker quitting." );
		}
	
	g_bSuppressPrintfOutput = bOldSuppress;
}


