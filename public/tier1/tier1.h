//===== Copyright © 2005-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: A higher level link library for general use in the game and tools.
//
//===========================================================================//


#ifndef TIER1_H
#define TIER1_H

#if defined( _WIN32 )
#pragma once
#endif

#include "appframework/IAppSystem.h"
#include "tier1/convar.h"
#include "icvar.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class ICvar;
class IDataModel;
class IDmElementFramework;


//-----------------------------------------------------------------------------
// These tier1 libraries must be set by any users of this library.
// They can be set by calling ConnectTier1Libraries.
// It is hoped that setting this, and using this library will be the common mechanism for
// allowing link libraries to access tier1 library interfaces
//-----------------------------------------------------------------------------
extern ICvar *cvar;
extern ICvar *g_pCVar;
extern IDataModel *g_pDataModel;
extern IDmElementFramework *g_pDmElementFramework;


//-----------------------------------------------------------------------------
// Call this to connect to/disconnect from all tier 1 libraries.
// It's up to the caller to check the globals it cares about to see if ones are missing
//-----------------------------------------------------------------------------
void ConnectTier1Libraries( CreateInterfaceFn *pFactoryList, int nFactoryCount );
void DisconnectTier1Libraries();


//-----------------------------------------------------------------------------
// Helper empty implementation of an IAppSystem for tier2 libraries
//-----------------------------------------------------------------------------
template< class IInterface, int ConVarFlag = 0 > 
class CTier1AppSystem : public CTier0AppSystem< IInterface >, public IConCommandBaseAccessor
{
	typedef CTier0AppSystem< IInterface > BaseClass;

public:
	CTier1AppSystem( bool bIsPrimaryAppSystem = true ) : BaseClass(	bIsPrimaryAppSystem )
	{
	}

	virtual bool Connect( CreateInterfaceFn factory ) 
	{
		if ( !BaseClass::Connect( factory ) )
			return false;

		if ( IsPrimaryAppSystem() )
		{
			ConnectTier1Libraries( &factory, 1 );
			if ( ConVarFlag && !g_pCVar )
			{
				Warning( "The convar system is needed to run!\n" );
				return false;
			}
		}
		return true;
	}

	virtual void Disconnect() 
	{
		if ( IsPrimaryAppSystem() )
		{
			DisconnectTier1Libraries();
		}
		BaseClass::Disconnect();
	}

	virtual InitReturnVal_t Init()
	{
		InitReturnVal_t nRetVal = BaseClass::Init();
		if ( nRetVal != INIT_OK )
			return nRetVal;

		if ( g_pCVar && ConVarFlag && IsPrimaryAppSystem() )
		{
			ConCommandBaseMgr::OneTimeInit( this );
		}
		return INIT_OK;
	}

	virtual void Shutdown()
	{
		if ( g_pCVar && ConVarFlag && IsPrimaryAppSystem() )
		{
			g_pCVar->UnlinkVariables( ConVarFlag );
		}
		BaseClass::Shutdown( );
	}

	virtual bool RegisterConCommandBase( ConCommandBase *pCommand )
	{
		// Mark for easy removal
		pCommand->AddFlags( ConVarFlag );
		pCommand->SetNext( 0 );

		// Link to engine's list instead
		g_pCVar->RegisterConCommandBase( pCommand );

//		char const *pValue = m_pCVar->GetCommandLineValue( pCommand->GetName() );
//		if( pValue && !pCommand->IsCommand() )
//		{
//			( ( ConVar * )pCommand )->SetValue( pValue );
//		}
		return true;
	}
};


#endif // TIER1_H

