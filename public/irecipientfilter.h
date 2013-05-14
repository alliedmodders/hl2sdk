//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
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

	// Client index will be -1 for invalid slot
	virtual void	GetRecipientIndex( int &clientIndex, int slot ) const = 0;
};

#endif // IRECIPIENTFILTER_H
