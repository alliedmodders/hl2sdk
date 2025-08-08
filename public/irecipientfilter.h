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

#include "const.h"
#include "eiface.h"
#include "inetchannel.h"

//-----------------------------------------------------------------------------
// Purpose: Generic interface for routing messages to users
//-----------------------------------------------------------------------------
class IRecipientFilter
{
public:
	virtual			~IRecipientFilter() {}

	virtual NetChannelBufType_t	GetNetworkBufType( void ) const = 0;
	virtual bool	IsInitMessage( void ) const = 0;

	virtual const CPlayerBitVec &GetRecipients( void ) const = 0;
};

#endif // IRECIPIENTFILTER_H
