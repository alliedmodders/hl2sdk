//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
//
// mpivrad.cpp
//

#include <windows.h>
#include <conio.h>
#include "vrad.h"
#include "physdll.h"
#include "lightmap.h"
#include "vstdlib/strtools.h"
#include "pacifier.h"
#include "messbuf.h"
#include "bsplib.h"
#include "consolewnd.h"
#include "vismat.h"
#include "vmpi_filesystem.h"
#include "vmpi_dispatch.h"
#include "utllinkedlist.h"
#include "vmpi.h"
#include "mpi_stats.h"
#include "vmpi_distribute_work.h"
#include "vmpi_tools_shared.h"




CUtlVector<char> g_LightResultsFilename;


extern int total_transfer;
extern int max_transfer;

extern void BuildVisLeafs(int);
extern void BuildPatchLights( int facenum );


// Handle VRAD packets.
bool VRAD_DispatchFn( MessageBuffer *pBuf, int iSource, int iPacketID )
{
	switch( pBuf->data[1] )
	{
		case VMPI_SUBPACKETID_PLIGHTDATA_RESULTS:
		{
			const char *pFilename = &pBuf->data[2];
			g_LightResultsFilename.CopyArray( pFilename, strlen( pFilename ) + 1 );
			return true;
		}
		
		default:		
			return false;
	}
}
CDispatchReg g_VRADDispatchReg( VMPI_VRAD_PACKET_ID, VRAD_DispatchFn ); // register to handle the messages we want
CDispatchReg g_DistributeWorkReg( VMPI_DISTRIBUTEWORK_PACKETID, DistributeWorkDispatch );



void VRAD_SetupMPI( int argc, char **argv )
{
	CmdLib_AtCleanup( VMPI_Stats_Term );

	//
	// Preliminary check -mpi flag
	//
	if ( !VMPI_FindArg( argc, argv, "-mpi", "" ) )
		return;

	// Force local mode?
	VMPIRunMode mode = VMPI_RUN_NETWORKED_MULTICAST;
	if ( VMPI_FindArg( argc, argv, "-mpi_broadcast", "" ) )
		mode = VMPI_RUN_NETWORKED_BROADCAST;
	if ( VMPI_FindArg( argc, argv, "-mpi_local", "" ) )
		mode = VMPI_RUN_LOCAL;

	VMPI_Stats_InstallSpewHook();

	//
	//  Extract mpi specific arguments
	//
	Msg( "MPI - Establishing connections...\n" );
	if ( !VMPI_Init( 
		argc, 
		argv, 
		"dependency_info_vrad.txt", 
		HandleMPIDisconnect,
		mode
		) )
	{
		Error( "MPI_Init failed." );
	}

	if ( !g_bMPI_NoStats )
		StatsDB_InitStatsDatabase( argc, argv, "dbinfo_vrad.txt" );
}


//-----------------------------------------
//
// Run BuildFaceLights across all available processing nodes
// and collect the results.
//

CCycleCount g_CPUTime;


//--------------------------------------------------
// Serialize face data
void SerializeFace( MessageBuffer * pmb, int facenum )
{
	int i, n;

	dface_t     * f  = &g_pFaces[facenum];
	facelight_t * fl = &facelight[facenum];

	pmb->write(f, sizeof(dface_t));
	pmb->write(fl, sizeof(facelight_t));

	pmb->write(fl->sample, sizeof(sample_t) * fl->numsamples);

	//
	// Write the light information
	// 
	for (i=0; i<MAXLIGHTMAPS; ++i) {
		for (n=0; n<NUM_BUMP_VECTS+1; ++n) {
			if (fl->light[i][n]) {
				pmb->write(fl->light[i][n], sizeof(Vector) * fl->numsamples);
			}
		}
	}

	if (fl->luxel) 	pmb->write(fl->luxel, sizeof(Vector) * fl->numluxels);
	if (fl->luxelNormals) pmb->write(fl->luxelNormals, sizeof(Vector) * fl->numluxels);
}

//--------------------------------------------------
// UnSerialize face data
//
void UnSerializeFace( MessageBuffer * pmb, int facenum )
{
	int i, n;

	dface_t     * f  = &g_pFaces[facenum];
	facelight_t * fl = &facelight[facenum];

	if (pmb->read(f, sizeof(dface_t)) < 0) Error("BS1");

	if (pmb->read(fl, sizeof(facelight_t)) < 0) Error("BS2");
	fl->sample = (sample_t *) calloc(fl->numsamples, sizeof(sample_t));
	if (pmb->read(fl->sample, sizeof(sample_t) * fl->numsamples) < 0) Error("BS3");

	//
	// Read the light information
	// 
	for (i=0; i<MAXLIGHTMAPS; ++i) {
		for (n=0; n<NUM_BUMP_VECTS+1; ++n) {
			if (fl->light[i][n]) {
				fl->light[i][n] = (Vector *) calloc (fl->numsamples, sizeof(Vector));
				if (pmb->read(fl->light[i][n], sizeof(Vector) * fl->numsamples) < 0) Error("BS7");
			}
		}
	}

	if (fl->luxel) {
		fl->luxel = (Vector *) calloc(fl->numluxels, sizeof(Vector));
		if (pmb->read(fl->luxel, sizeof(Vector) * fl->numluxels) < 0) Error("BS8");
	}

	if (fl->luxelNormals) {
		fl->luxelNormals = (Vector *) calloc(fl->numluxels, sizeof(Vector));
		if (pmb->read(fl->luxelNormals, sizeof(Vector) * fl->numluxels) < 0) Error("BS9");
	}

}


void MPI_ReceiveFaceResults( int iWorkUnit, MessageBuffer *pBuf, int iWorker )
{
	UnSerializeFace( pBuf, iWorkUnit );
}


void MPI_ProcessFaces( int iThread, int iWorkUnit, MessageBuffer *pBuf )
{
	// Do BuildFacelights on the face.
	CTimeAdder adder( &g_CPUTime );
		BuildFacelights( iThread, iWorkUnit );
	adder.End();
	
	// Send the results.
	SerializeFace( pBuf, iWorkUnit );
}


void RunMPIBuildFacelights()
{
	g_CPUTime.Init();

    Msg( "%-20s ", "BuildFaceLights:" );
	StartPacifier("");

	VMPI_SetCurrentStage( "RunMPIBuildFaceLights" );
	double elapsed = DistributeWork( 
		numfaces, 
		VMPI_DISTRIBUTEWORK_PACKETID,
		MPI_ProcessFaces, 
		MPI_ReceiveFaceResults );

	EndPacifier(false);
	Msg( " (%d)\n", (int)elapsed );

	if ( g_bMPIMaster )
	{
		//
		// BuildPatchLights is normally called from BuildFacelights(),
		// but in MPI mode we have the master do the calculation
		// We might be able to speed this up by doing while the master
		// is idling in the above loop. Wouldn't want to slow down the
		// handing out of work - maybe another thread?
		//
		for ( int i=0; i < numfaces; ++i )
		{
			BuildPatchLights(i);
		}
	}
	else
	{
		Warning( "\n\nTook %.2f seconds (%d%% CPU utilization)\n\n", 
			elapsed, 
			(int)( g_CPUTime.GetSeconds() * 100 / elapsed ) );
	}
}




//-----------------------------------------
//
// Run BuildVisLeafs across all available processing nodes
// and collect the results.
//

// This function is called when the master receives results back from a worker.
void MPI_ReceiveVisLeafsResults( int iWorkUnit, MessageBuffer *pBuf, int iWorker )
{
	int patchesInCluster = 0;
	
	pBuf->read(&patchesInCluster, sizeof(patchesInCluster));
	
	for ( int k=0; k < patchesInCluster; ++k )
	{
		int patchnum = 0;
		pBuf->read(&patchnum, sizeof(patchnum));
		
		patch_t * patch = &patches[patchnum];
		int numtransfers;
		pBuf->read( &numtransfers, sizeof(numtransfers) );
		patch->numtransfers = numtransfers;
		if (numtransfers) 
		{
			patch->transfers = (transfer_t *) malloc(numtransfers * sizeof(transfer_t));
			pBuf->read(patch->transfers, numtransfers * sizeof(transfer_t));
		}
		
		total_transfer += numtransfers;
		if (max_transfer < numtransfers) 
			max_transfer = numtransfers;
	}
}


// Temporary variables used during callbacks. If we're going to be threadsafe, these 
// should go in a structure and get passed around.
class CVMPIVisLeafsData
{
public:
	MessageBuffer *m_pVisLeafsMB;
	int m_nPatchesInCluster;
};

CVMPIVisLeafsData g_VMPIVisLeafsData[MAX_TOOL_THREADS+1];
transfer_t *g_pBuildVisLeafsTransfers;



// This is called by BuildVisLeafs_Cluster every time it finishes a patch.
// The results are appended to g_VisLeafsMB and sent back to the master when all clusters are done.
void MPI_AddPatchData( int iThread, int patchnum, patch_t *patch )
{
	CVMPIVisLeafsData *pData = &g_VMPIVisLeafsData[iThread];

	// Add in results for this patch
	++pData->m_nPatchesInCluster;
	pData->m_pVisLeafsMB->write(&patchnum, sizeof(patchnum));
	pData->m_pVisLeafsMB->write(&patch->numtransfers, sizeof(patch->numtransfers));
	pData->m_pVisLeafsMB->write( patch->transfers, patch->numtransfers * sizeof(transfer_t) );
}


// This handles a work unit sent by the master. Each work unit here is a 
// list of clusters.
void MPI_ProcessVisLeafs( int iThread, int iWorkUnit, MessageBuffer *pBuf )
{
	CVMPIVisLeafsData *pData = &g_VMPIVisLeafsData[iThread];
	int iCluster = iWorkUnit;

	// Start this cluster.
	pData->m_nPatchesInCluster = 0;

	pData->m_pVisLeafsMB = pBuf;

	// Write a temp value in there. We overwrite it later.
	int iSavePos = pBuf->getLen();
	pBuf->write( &pData->m_nPatchesInCluster, sizeof(pData->m_nPatchesInCluster) );

	// Collect the results in MPI_AddPatchData.
	BuildVisLeafs_Cluster( iThread, g_pBuildVisLeafsTransfers, iCluster, MPI_AddPatchData );

	// Now send the results back..
	pBuf->update( iSavePos, &pData->m_nPatchesInCluster, sizeof(pData->m_nPatchesInCluster) );

	pData->m_pVisLeafsMB = NULL;
}


void RunMPIBuildVisLeafs()
{
    Msg( "%-20s ", "BuildVisLeafs  :" );
	if ( g_bMPIMaster )
	{
		StartPacifier("");
	}
	else
	{
		g_pBuildVisLeafsTransfers = BuildVisLeafs_Start();
	}

	numthreads = 1;

	//
	// Slaves ask for work via GetMPIBuildVisLeafWork()
	// Results are returned in BuildVisRow()
	//
	VMPI_SetCurrentStage( "RunMPIBuildVisLeafs" );
	
	double elapsed = DistributeWork( 
		dvis->numclusters, 
		VMPI_DISTRIBUTEWORK_PACKETID,
		MPI_ProcessVisLeafs, 
		MPI_ReceiveVisLeafsResults );

	if ( g_bMPIMaster )
	{
		EndPacifier(false);
		Msg( " (%d)\n", (int)elapsed );
	}
	else
	{
		BuildVisLeafs_End( g_pBuildVisLeafsTransfers );

		Msg( "%% worker CPU utilization during PortalFlow: %.1f\n", 
			(g_CPUTime.GetSeconds() * 100.0f / elapsed) / numthreads );

		//Msg( "VRAD worker finished. Over and out.\n" );
		//VMPI_SetCurrentStage( "worker done" );
		//CmdLib_Exit( 0 );
	}
}



void VMPI_DistributeLightData()
{
	if ( !g_bUseMPI )
		return;

	if ( g_bMPIMaster )
	{
		const char *pVirtualFilename = "--plightdata--";
		VMPI_FileSystem_CreateVirtualFile( pVirtualFilename, pdlightdata->Base(), pdlightdata->Count() );

		char cPacketID[2] = { VMPI_VRAD_PACKET_ID, VMPI_SUBPACKETID_PLIGHTDATA_RESULTS };
		VMPI_Send2Chunks( cPacketID, sizeof( cPacketID ), pVirtualFilename, strlen( pVirtualFilename ) + 1, VMPI_PERSISTENT );
	}
	else
	{
		VMPI_SetCurrentStage( "VMPI_DistributeLightData" );

		// Wait until we've received the filename from the master.
		while ( g_LightResultsFilename.Count() == 0 )
		{
			VMPI_DispatchNextMessage();
		}

		// Open 
		FileHandle_t fp = g_pFileSystem->Open( g_LightResultsFilename.Base(), "rb", VMPI_VIRTUAL_FILES_PATH_ID );
		if ( !fp )
			Error( "Can't open '%s' to read lighting info.", g_LightResultsFilename.Base() );

		int size = g_pFileSystem->Size( fp );
		pdlightdata->EnsureCount( size );
		g_pFileSystem->Read( pdlightdata->Base(), size, fp );
	
		g_pFileSystem->Close( fp );
	}
}


