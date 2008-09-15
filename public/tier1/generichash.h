//======= Copyright © 2005, , Valve Corporation, All rights reserved. =========
//
// Purpose: Variant Pearson Hash general purpose hashing algorithm described
//			by Cargill in C++ Report 1994. Generates a 16-bit result.
//
//=============================================================================

#ifndef GENERICHASH_H
#define GENERICHASH_H

#if defined(_WIN32)
#pragma once
#endif

///////////////////////////////////////////////////////////////////////////////

unsigned HashString( const char *pszKey );
unsigned HashStringCaseless( const char *pszKey );
unsigned HashStringCaselessConventional( const char *pszKey );
unsigned Hash4( const void *pKey );
unsigned Hash8( const void *pKey );
unsigned Hash12( const void *pKey );
unsigned Hash16( const void *pKey );
unsigned HashBlock( const void *pKey, unsigned size );

///////////////////////////////////////////////////////////////////////////////

template <typename T>
inline unsigned HashItem( const T &item )
{
	// TODO: Confirm comiler optimizes out unused paths
	if ( sizeof(item) == 4 )
		return Hash4( &item );
	else if ( sizeof(item) == 8 )
		return Hash8( &item );
	else if ( sizeof(item) == 12 )
		return Hash12( &item );
	else if ( sizeof(item) == 16 )
		return Hash16( &item );
	else
		return HashBlock( &item, sizeof(item) );
}

template<> inline unsigned HashItem<const char *>(const char * const &pszKey )
{
	return HashString( pszKey );
}

template<> inline unsigned HashItem<char *>(char * const &pszKey )
{
	return HashString( pszKey );
}

///////////////////////////////////////////////////////////////////////////////

#endif /* !GENERICHASH_H */
