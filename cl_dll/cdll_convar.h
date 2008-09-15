//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef CDLL_CONVAR_H
#define CDLL_CONVAR_H
#pragma once


// This file implements IConVarAccessor to allow access to console variables.



#include "convar.h"
#include "cdll_util.h"
#include "icvar.h"

class CDLL_ConVarAccessor : public IConCommandBaseAccessor
{
public:
	virtual bool	RegisterConCommandBase( ConCommandBase *pCommand )
	{
		// Mark for easy removal
		pCommand->AddFlags( FCVAR_CLIENTDLL );

		// Unlink from client .dll only list
		pCommand->SetNext( 0 );

		// Link to engine's list instead
		cvar->RegisterConCommandBase( pCommand );
		return true;
	}
};



#endif // CDLL_CONVAR_H
