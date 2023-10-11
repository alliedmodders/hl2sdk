//========= Copyright ?1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef IRECIPIENTFILTER_H
#define IRECIPIENTFILTER_H
#ifdef _WIN32
#pragma once
#endif

#include "eiface.h"

//-----------------------------------------------------------------------------
// Purpose: Generic interface for routing messages to users
//-----------------------------------------------------------------------------
class IRecipientFilter
{
public:
	virtual			~IRecipientFilter() {}

	virtual bool	IsReliable( void ) const = 0;
	virtual bool	IsInitMessage( void ) const = 0;

	virtual int		GetRecipientCount( void ) const = 0;
	virtual CPlayerSlot	GetRecipientIndex( int slot ) const = 0;
};

#endif // IRECIPIENTFILTER_H
