//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//
#include "resourcetags.h"
#include "tier0/dbg.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef _XBOX
static int s_resourceTag = RESOURCE_TAG_NONE;

void Sys_SetResourceTag( int tag )
{
	s_resourceTag = tag;
}

int Sys_GetResourceTag( void )
{
#if 0
	// control flow issue, no resource should be instanced during an indeterminate state
	Assert( s_resourceTag != RESOURCE_TAG_NONE );
#endif

	return s_resourceTag;
}
#endif