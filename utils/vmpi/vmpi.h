//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef VMPI_H
#define VMPI_H
#ifdef _WIN32
#pragma once
#endif


#include "vmpi_defs.h"
#include "messbuf.h"


// These are called to handle incoming messages. 
// Return true if you handled the message and false otherwise.
// Note: the first byte in each message is the packet ID.
typedef bool (*VMPIDispatchFn)( MessageBuffer *pBuf, int iSource, int iPacketID );

typedef void (*VMPI_Disconnect_Handler)( int procID, const char *pReason );


// Which machine is the master.
#define VMPI_MASTER_ID 0

#define VMPI_SEND_TO_ALL	-2
#define VMPI_PERSISTENT		-3	// If this is set as the destination for a packet, it is sent to all
								// workers, and also to new workers that connect.

#define MAX_VMPI_PACKET_IDS		32


#define VMPI_TIMEOUT_INFINITE	0xFFFFFFFF


// Instantiate one of these to register a dispatch.
class CDispatchReg
{
public:
	CDispatchReg( int iPacketID, VMPIDispatchFn fn );
};


// Shared by all the tools.
extern bool	g_bUseMPI;
extern bool g_bMPIMaster;	// Set to true if we're the master in a VMPI session.
extern int g_iVMPIVerboseLevel; // Higher numbers make it spit out more data.
extern bool g_bMPI_NoStats;

// These can be watched or modified to check bandwidth statistics.
extern int g_nBytesSent;
extern int g_nMessagesSent;
extern int g_nBytesReceived;
extern int g_nMessagesReceived;

extern int g_nMulticastBytesSent;
extern int g_nMulticastBytesReceived;


enum VMPIRunMode
{
	VMPI_RUN_NETWORKED_MULTICAST,	// Multicast out, find workers, have them do work.
	VMPI_RUN_NETWORKED_BROADCAST,	// Broadcast out, find workers, have them do work.
	VMPI_RUN_LOCAL		// Just make a local process and have it do the work.
};


// It's good to specify a disconnect handler here immediately. If you don't have a handler
// and the master disconnects, you'll lockup forever inside a dispatch loop because you
// never handled the master disconnecting.
//
// Note: runMode is only relevant for the VMPI master. The worker always connects to the master
// the same way.
bool VMPI_Init( 
	int argc, 
	char **argv, 
	const char *pDependencyFilename, 
	VMPI_Disconnect_Handler handler, 
	VMPIRunMode runMode // Networked or local?
	);
void VMPI_Finalize();

VMPIRunMode VMPI_GetRunMode();

// Note: this number can change on the master.
int VMPI_GetCurrentNumberOfConnections();


// Dispatch messages until it gets one with the specified packet ID.
// If subPacketID is not set to -1, then the second byte must match that as well.
//
// Note: this WILL dispatch packets with matching packet IDs and give them a chance to handle packets first.
//
// If bWait is true, then this function either succeeds or Error() is called. If it's false, then if the first available message
// is handled by a dispatch, this function returns false.
bool VMPI_DispatchUntil( MessageBuffer *pBuf, int *pSource, int packetID, int subPacketID = -1, bool bWait = true );

// This waits for the next message and dispatches it.
// You can specify a timeout in milliseconds. If the timeout expires, the function returns false.
bool VMPI_DispatchNextMessage( unsigned long timeout=VMPI_TIMEOUT_INFINITE );

// This should be called periodically in modal loops that don't call other VMPI functions. This will
// check for disconnected sockets and call disconnect handlers so the app can error out if
// it loses all of its connections. 
//
// This can be used in place of a Sleep() call by specifying a timeout value.
void VMPI_HandleSocketErrors( unsigned long timeout=0 );



// Use these to send data to one of the machines.
// If iDest is VMPI_SEND_TO_ALL, then the message goes to all the machines.
bool VMPI_SendData( void *pData, int nBytes, int iDest );
bool VMPI_SendChunks( void const * const *pChunks, const int *pChunkLengths, int nChunks, int iDest );
bool VMPI_Send2Chunks( const void *pChunk1, int chunk1Len, const void *pChunk2, int chunk2Len, int iDest );	// for convenience..
bool VMPI_Send3Chunks( const void *pChunk1, int chunk1Len, const void *pChunk2, int chunk2Len, const void *pChunk3, int chunk3Len, int iDest );

// This registers a function that gets called when a connection is terminated ungracefully.
void VMPI_AddDisconnectHandler( VMPI_Disconnect_Handler handler );

// Returns false if the process has disconnected ungracefully (disconnect handlers
// would have been called for it too).
bool VMPI_IsProcConnected( int procID );

// Simple wrapper for Sleep() so people can avoid including windows.h
void VMPI_Sleep( unsigned long ms );

// VMPI sends machine names around first thing.
const char* VMPI_GetLocalMachineName();
const char* VMPI_GetMachineName( int iProc );
bool VMPI_HasMachineNameBeenSet( int iProc );

// Returns 0xFFFFFFFF if the ID hasn't been set.
unsigned long VMPI_GetJobWorkerID( int iProc );
void VMPI_SetJobWorkerID( int iProc, unsigned long jobWorkerID );

// Search a command line to find arguments. Looks for pName, and if it finds it, returns the
// argument following it. If pName is the last argument, it returns pDefault. If it doesn't
// find pName, returns NULL.
const char* VMPI_FindArg( int argc, char **argv, const char *pName, const char *pDefault = "" );

// (Threadsafe) get and set the current stage. This info winds up in the VMPI database.
void VMPI_GetCurrentStage( char *pOut, int strLen );
void VMPI_SetCurrentStage( const char *pCurStage );

#endif // VMPI_H
