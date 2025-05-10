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
#include <string.h>
#include "bufferstring.h"
#include "tier1/generichash.h"

#define DEBUG_STRINGTOKENS 0
#define STRINGTOKEN_MURMURHASH_SEED 0x31415926

// Macros are intended to be used between CUtlStringToken (always lowercase)
#define MAKE_STRINGTOKEN_HASH(pstr) CUtlStringToken::Hash( (pstr), strlen(pstr), STRINGTOKEN_MURMURHASH_SEED )
#define MAKE_STRINGTOKEN_HASH_UTL(containerName) CUtlStringToken::Hash( (containerName).Get(), (containerName).Length(), STRINGTOKEN_MURMURHASH_SEED )

class IFormatOutputStream;
class CFormatStringElement;


// See VStringTokenSystem001
// Interact with stringtokendatabase.txt
PLATFORM_INTERFACE bool g_bUpdateStringTokenDatabase;
PLATFORM_INTERFACE void RegisterStringToken( uint32 nHashCode, const char *pStart, const char *pEnd = NULL, bool bExtraAddToDatabase = true );

#define TOLOWERU( c ) ( ( uint32 ) ( ( ( c >= 'A' ) && ( c <= 'Z' ) ) ? ( c | ( 1 << 5 ) ) : c ) )
class CUtlStringToken
{
public:
	static constexpr uint32 sm_nMagic    = 0x5bd1e995;
	static constexpr uint32 sm_nRotation = 24;

	template< bool CASEINSENSITIVE = true >
	FORCEINLINE static constexpr uint32 Hash( const char *pString, int n, uint32 nSeed = STRINGTOKEN_MURMURHASH_SEED )
	{
		// They're not really 'magic', they just happen to work well.

		constexpr uint32 m = sm_nMagic;
		constexpr uint32 r = sm_nRotation;

		// Initialize the hash to a 'random' value

		uint32 nLength = static_cast< uint32 >( n );

		uint32 h = nSeed ^ nLength;

		// Mix 4 bytes at a time into the hash

		while ( nLength >= 4 )
		{
			uint32 k = CASEINSENSITIVE ? ( TOLOWERU( pString[ 0 ] ) | ( TOLOWERU( pString[ 1 ] ) << 8 ) | ( TOLOWERU( pString[ 2 ] ) << 16 ) | ( TOLOWERU( pString[ 3 ] ) << 24 ) ) : LittleDWord( *(uint32 *)pString );

			k *= m;
			k ^= k >> r;
			k *= m;

			h *= m;
			h ^= k;

			pString += 4;
			nLength -= 4;
		}

		// Handle the last few bytes of the input array

		switch ( nLength )
		{
		case 3: h ^= ( CASEINSENSITIVE ? TOLOWERU( pString[ 2 ] ) : pString[ 2 ] ) << 16; [[fallthrough]];
		case 2: h ^= ( CASEINSENSITIVE ? TOLOWERU( pString[ 1 ] ) : pString[ 1 ] ) << 8;  [[fallthrough]];
		case 1: h ^= ( CASEINSENSITIVE ? TOLOWERU( pString[ 0 ] ) : pString[ 0 ] );
			h *= m;
		};

		// Do a few final mixes of the hash to ensure the last few
		// bytes are well-incorporated.

		h ^= h >> 13;
		h *= m;
		h ^= h >> 15;

		return h;
	}

	CUtlStringToken( uint32 nHashCode = 0 ) : m_nHashCode( nHashCode ) {}
	template < uintp N > FORCEINLINE constexpr CUtlStringToken( const char (&str)[N] ) : m_nHashCode( Hash( str, N - 1 ) ) {}
	CUtlStringToken( const char *pString, int nLen ) : m_nHashCode( Hash( pString, nLen ) ) {}
	CUtlStringToken( const CBufferString &buffer ) : CUtlStringToken( buffer.Get(), buffer.Length() ) {}

	// operator==
	bool operator==( const uint32 nHash ) const { return m_nHashCode == nHash; }
	bool operator==( const CUtlStringToken &other ) const { return operator==( other.GetHashCode() ); }
	bool operator==( const char *pString ) const { return operator==( MAKE_STRINGTOKEN_HASH( pString ) ); }
	bool operator==( const CBufferString &buffer ) const { return operator==( MAKE_STRINGTOKEN_HASH_UTL( buffer ) ); }

	// operator!=
	bool operator!=( const uint32 nHash ) const { return !operator==( nHash ); }
	bool operator!=( const CUtlStringToken &other ) const { return !operator==( other ); }
	bool operator!=( const char *pString ) const { return !operator==( pString ); }
	bool operator!=( const CBufferString &buffer ) const { return !operator==( buffer ); }

	// opertator<
	bool operator<( const uint32 nHash ) const { return ( m_nHashCode < nHash ); }
	bool operator<( CUtlStringToken const &other ) const { return operator<( other.GetHashCode() ); }
	bool operator<( const char *pString ) const { return operator<( MAKE_STRINGTOKEN_HASH( pString ) ); }
	bool operator<( const CBufferString &buffer ) const { return !operator<( MAKE_STRINGTOKEN_HASH_UTL( buffer ) ); }

	/// access to the hash code for people who need to store thse as 32-bits, regardless of the
	operator uint32() const { return m_nHashCode; }
	uint32 GetHashCode() const { return m_nHashCode; }

	DLL_CLASS_IMPORT void FormatTo( IFormatOutputStream* pOutputStream, CFormatStringElement pElement ) const;
	DLL_CLASS_IMPORT static bool TrackTokenCreation( const char *s1, const char *s2 );

private:
	uint32 m_nHashCode;
};

FORCEINLINE bool TrackStringToken( uint32 nHash, const char *pString )
{
	if ( g_bUpdateStringTokenDatabase )
	{
		RegisterStringToken( nHash, pString );

		return true;
	}

	return false;
}

template< bool CASEINSENSITIVE = true, bool TRACKCREATION = true >
FORCEINLINE uint32 MakeStringToken( const char *pString, int nLen )
{
	uint32 nHash = CASEINSENSITIVE ? MurmurHash2LowerCase( pString, nLen, STRINGTOKEN_MURMURHASH_SEED ) : MurmurHash2( pString, nLen, STRINGTOKEN_MURMURHASH_SEED );

	if constexpr ( TRACKCREATION )
		TrackStringToken( nHash, pString );

	return nHash;
}

template< bool CASEINSENSITIVE = true, bool TRACKCREATION = true >
FORCEINLINE uint32 MakeStringToken( const char *pString )
{
	return MakeStringToken< CASEINSENSITIVE, TRACKCREATION >( pString, strlen(pString) );
}

template< bool CASEINSENSITIVE = true, uintp SIZE = 128 >
FORCEINLINE uint32 HashStringWithBuffer( const char *pString, int nLen = -1 )
{
	CBufferStringN< SIZE > buffer( pString, nLen );

	if constexpr ( CASEINSENSITIVE )
		buffer.ToLowerFast();

	return CUtlStringToken::Hash< CASEINSENSITIVE >( buffer.Get(), buffer.Length() );
}

template< bool CASEINSENSITIVE = true, bool TRACKCREATION = true >
FORCEINLINE CUtlStringToken MakeStringToken2( const char *pString, int nLen = -1 )
{
	CUtlStringToken nHashToken = HashStringWithBuffer< CASEINSENSITIVE >( pString, nLen );

	if constexpr ( TRACKCREATION )
		TrackStringToken( nHashToken, pString );

	return nHashToken;
}

#endif // UTLSTRINGTOKEN_H
