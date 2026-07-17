//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#ifndef UTLSTRINGTOKEN_H
#define UTLSTRINGTOKEN_H

#ifdef _WIN32
#pragma once
#endif

#include <limits.h>
#include "tier1/generichash.h"

#include <type_traits>

#define STRINGTOKEN_MURMURHASH_SEED 0x31415926

class CUtlString;
class IFormatOutputStream;
class CFormatStringElement;

// AMNOTE: See VStringTokenSystem001
// Interact with stringtokendatabase.txt
PLATFORM_INTERFACE void RegisterStringToken( uint32 nHashCode, const char *pStart, const char *pEnd = NULL, bool bExtraAddToDatabase = true );

class CUtlStringToken
{
public:
	FORCEINLINE CUtlStringToken( uint32 nHashCode = 0 ) : m_nHashCode( nHashCode ) {}

	template <size_t N>
	FORCEINLINE CUtlStringToken( const char (&str)[N] ) : m_nHashCode( MurmurHash2LowerCase( str, STRINGTOKEN_MURMURHASH_SEED ) ) { }

	// AMNOTE: Template is required to enforce compiler to pick the correct overload when inlining
	// as otherwise non templated overload would always win the pick thus no const folding would happen
	template <typename T, std::enable_if_t<std::is_same_v<T, const char *>, int> = 0>
	FORCEINLINE CUtlStringToken( T str ) : m_nHashCode( 0 )
	{
		if(str && *str)
			m_nHashCode = MurmurHash2LowerCase( str, STRINGTOKEN_MURMURHASH_SEED );
	};

	FORCEINLINE bool operator==( CUtlStringToken const &other ) const { return ( other.m_nHashCode == m_nHashCode ); }
	FORCEINLINE bool operator!=( CUtlStringToken const &other ) const { return !operator==( other ); }
	FORCEINLINE bool operator<( CUtlStringToken const &other ) const { return ( m_nHashCode < other.m_nHashCode ); }

	FORCEINLINE bool IsValid() const { return m_nHashCode != 0; }
	FORCEINLINE uint32 GetHashCode() const { return m_nHashCode; }
	FORCEINLINE void SetHashCode( uint32 hash ) { m_nHashCode = hash; }

	DLL_CLASS_IMPORT void FormatTo( IFormatOutputStream* pOutputStream, CFormatStringElement pElement ) const;
	DLL_CLASS_IMPORT static bool TrackTokenCreation( const char *s1, const char *s2 );

private:
	uint32 m_nHashCode;
};

#endif // UTLSTRINGTOKEN_H
