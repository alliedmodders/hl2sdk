//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include <windows.h>
#include "materialsystem/IMaterialSystem.h"
#include "materialsystem/MaterialSystem_Config.h"
#include <cmdlib.h>
#include "tier0/dbg.h"
#include "FileSystem.h"
#include "cmdlib.h"
#include "tier2/tier2.h"

extern void MdlError( char const *pMsg, ... );

CreateInterfaceFn g_MatSysFactory = NULL;
CreateInterfaceFn g_ShaderAPIFactory = NULL;

static void LoadMaterialSystem( void )
{
	if( g_pMaterialSystem )
		return;
	
	const char *pDllName = "materialsystem.dll";
	CSysModule *materialSystemDLLHInst;
	materialSystemDLLHInst = g_pFullFileSystem->LoadModule( pDllName );
	if( !materialSystemDLLHInst )
	{
		MdlError( "Can't load MaterialSystem.dll\n" );
	}

	g_MatSysFactory = Sys_GetFactory( materialSystemDLLHInst );
	if ( g_MatSysFactory )
	{
		g_pMaterialSystem = (IMaterialSystem *)g_MatSysFactory( MATERIAL_SYSTEM_INTERFACE_VERSION, NULL );
		if ( !g_pMaterialSystem )
		{
			MdlError( "Could not get the material system interface from materialsystem.dll" );
		}
	}
	else
	{
		MdlError( "Could not find factory interface in library MaterialSystem.dll" );
	}

	if (!( g_ShaderAPIFactory = g_pMaterialSystem->Init( "shaderapiempty.dll", 0, CmdLib_GetFileSystemFactory() )) )
	{
		MdlError( "Could not start the empty shader (shaderapiempty.dll)!" );
	}
}

void InitMaterialSystem( const char *materialBaseDirPath )
{
	LoadMaterialSystem();
	MaterialSystem_Config_t config;
	g_pMaterialSystem->OverrideConfig( config, false );
}
