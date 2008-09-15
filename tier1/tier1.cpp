//===== Copyright © 2005-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: A higher level link library for general use in the game and tools.
//
//===========================================================================//

#include <tier1/tier1.h>
#include "tier0/dbg.h"
#include "datamodel/idatamodel.h"
#include "icvar.h"


//-----------------------------------------------------------------------------
// These tier1 libraries must be set by any users of this library.
// They can be set by calling ConnectTier1Libraries or InitDefaultFileSystem.
// It is hoped that setting this, and using this library will be the common mechanism for
// allowing link libraries to access tier1 library interfaces
//-----------------------------------------------------------------------------
ICvar *cvar = 0;
ICvar *g_pCVar = 0;
IDataModel *g_pDataModel = 0;
IDmElementFramework *g_pDmElementFramework = 0;


//-----------------------------------------------------------------------------
// Call this to connect to all tier 1 libraries.
// It's up to the caller to check the globals it cares about to see if ones are missing
//-----------------------------------------------------------------------------
void ConnectTier1Libraries( CreateInterfaceFn *pFactoryList, int nFactoryCount )
{
	// Don't connect twice..
	Assert( !g_pCVar && !g_pDataModel && !g_pDmElementFramework );

	for ( int i = 0; i < nFactoryCount; ++i )
	{
		if ( !g_pCVar )
		{
			cvar = g_pCVar = ( ICvar * )pFactoryList[i]( VENGINE_CVAR_INTERFACE_VERSION, NULL );
		}
		if ( !g_pDataModel )
		{
			g_pDataModel = ( IDataModel * )pFactoryList[i]( VDATAMODEL_INTERFACE_VERSION, NULL );
		}
		if ( !g_pDmElementFramework )
		{
			g_pDmElementFramework = ( IDmElementFramework * )pFactoryList[i]( VDMELEMENTFRAMEWORK_VERSION, NULL );
		}
	}
}

void DisconnectTier1Libraries()
{
	g_pCVar = cvar = 0;
	g_pDataModel = 0;
	g_pDmElementFramework = 0;
}
