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

#include <cctype>
#include <type_traits>

#include "platform.h"

//-----------------------------------------------------------------------------

// AMNOTE: To not pollute global namespace, these are hidden under namespace
namespace GenericHash
{
	namespace detail
	{
		// Branchless constexpr ascii tolower method.
		// Checks if value is in range of A-Z
		// if so adds 32 to the base char symbol
		constexpr char to_lower( char c )
		{
			return c + (((c >= 'A') & (c <= 'Z')) << 5);
		}

		static constexpr uint32 kRandVals[256] =
		{
			238,	164,	191,	168,	115,	 16,	142,	 11,	213,	214,	 57,	151,	248,	252,	 26,	198,
			 13,	105,	102,	 25,	 43,	 42,	227,	107,	210,	251,	 86,	 66,	 83,	193,	126,	108,
			131,	  3,	 64,	186,	192,	 81,	 37,	158,	 39,	244,	 14,	254,	 75,	 30,	  2,	 88,
			172,	176,	255,	 69,	  0,	 45,	116,	139,	 23,	 65,	183,	148,	 33,	 46,	203,	 20,
			143,	205,	 60,	197,	118,	  9,	171,	 51,	233,	135,	220,	 49,	 71,	184,	 82,	109,
			 36,	161,	169,	150,	 63,	 96,	173,	125,	113,	 67,	224,	 78,	232,	215,	 35,	219,
			 79,	181,	 41,	229,	149,	153,	111,	217,	 21,	 72,	120,	163,	133,	 40,	122,	140,
			208,	231,	211,	200,	160,	182,	104,	110,	178,	237,	 15,	101,	 27,	 50,	 24,	189,
			177,	130,	187,	 92,	253,	136,	100,	212,	 19,	174,	 70,	 22,	170,	206,	162,	 74,
			247,	  5,	 47,	 32,	179,	117,	132,	195,	124,	123,	245,	128,	236,	223,	 12,	 84,
			 54,	218,	146,	228,	157,	 94,	106,	 31,	 17,	 29,	194,	 34,	 56,	134,	239,	246,
			241,	216,	127,	 98,	  7,	204,	154,	152,	209,	188,	 48,	 61,	 87,	 97,	225,	 85,
			 90,	167,	155,	112,	145,	114,	141,	 93,	250,	  4,	201,	156,	 38,	 89,	226,	196,
			  1,	235,	 44,	180,	159,	121,	119,	166,	190,	144,	 10,	 91,	 76,	230,	221,	 80,
			207,	 55,	 58,	 53,	175,	  8,	  6,	 52,	 68,	242,	 18,	222,	103,	249,	147,	129,
			138,	243,	 28,	185,	 62,	 59,	240,	202,	234,	 99,	 77,	 73,	199,	137,	 95,	165,
		};
	};
};

inline uint32 HashStringFastCaselessConventional( const char *pszKey )
{
	uint32 hash = 0xAAAAAAAA; // Alternating 1's and 0's to maximize the effect of the later multiply and add

	for(; *pszKey; pszKey++)
	{
		hash = ((hash << 5) + hash) + (uint8)tolower( *pszKey );
	}

	return hash;
}

constexpr uint32 HashStringConventional( const char *pszKey )
{
	uint32 hash = 0xAAAAAAAA; // Alternating 1's and 0's to maximize the effect of the later multiply and add

	for(; *pszKey; pszKey++)
	{
		hash = ((hash << 5) + hash) + (uint8)*pszKey;
	}

	return hash;
}

constexpr uint32 HashInt( const int n )
{
	uint32 even = 0, odd = 0;
	even  = GenericHash::detail::kRandVals[n & 0xff];
	odd   = GenericHash::detail::kRandVals[((n >> 8) & 0xff)];

	even  = GenericHash::detail::kRandVals[odd ^ (n >> 24)];
	odd   = GenericHash::detail::kRandVals[even ^ ((n >> 16) & 0xff)];
	even  = GenericHash::detail::kRandVals[odd ^ ((n >> 8) & 0xff)];
	odd   = GenericHash::detail::kRandVals[even ^ (n & 0xff)];

	return (even << 8) | odd;
}

inline uint32 Hash4( const void *pKey )
{
	const uint32 *p = (const uint32 *)pKey;
	uint32		even = 0, odd = 0, n = 0;

	n     = *p;
	even  = GenericHash::detail::kRandVals[n & 0xff];
	odd   = GenericHash::detail::kRandVals[((n >> 8) & 0xff)];

	even  = GenericHash::detail::kRandVals[odd ^ (n >> 24)];
	odd   = GenericHash::detail::kRandVals[even ^ ((n >> 16) & 0xff)];
	even  = GenericHash::detail::kRandVals[odd ^ ((n >> 8) & 0xff)];
	odd   = GenericHash::detail::kRandVals[even ^ (n & 0xff)];

	return (even << 8) | odd;
}

inline uint32 Hash8( const void *pKey )
{
	const uint32 *p = (const uint32 *)pKey;
	uint32		even = 0, odd = 0, n = 0;

	n     = *p;
	even  = GenericHash::detail::kRandVals[n & 0xff];
	odd   = GenericHash::detail::kRandVals[((n >> 8) & 0xff)];

	even  = GenericHash::detail::kRandVals[odd ^ (n >> 24)];
	odd   = GenericHash::detail::kRandVals[even ^ ((n >> 16) & 0xff)];
	even  = GenericHash::detail::kRandVals[odd ^ ((n >> 8) & 0xff)];
	odd   = GenericHash::detail::kRandVals[even ^ (n & 0xff)];

	n     = *(p + 1);
	even  = GenericHash::detail::kRandVals[odd ^ (n >> 24)];
	odd   = GenericHash::detail::kRandVals[even ^ ((n >> 16) & 0xff)];
	even  = GenericHash::detail::kRandVals[odd ^ ((n >> 8) & 0xff)];
	odd   = GenericHash::detail::kRandVals[even ^ (n & 0xff)];

	return (even << 8) | odd;
}

inline uint32 Hash12( const void *pKey )
{
	const uint32 *p = (const uint32 *)pKey;
	uint32		even = 0, odd = 0, n = 0;

	n     = *p;
	even  = GenericHash::detail::kRandVals[n & 0xff];
	odd   = GenericHash::detail::kRandVals[((n >> 8) & 0xff)];

	even  = GenericHash::detail::kRandVals[odd ^ (n >> 24)];
	odd   = GenericHash::detail::kRandVals[even ^ ((n >> 16) & 0xff)];
	even  = GenericHash::detail::kRandVals[odd ^ ((n >> 8) & 0xff)];
	odd   = GenericHash::detail::kRandVals[even ^ (n & 0xff)];

	n     = *(p + 1);
	even  = GenericHash::detail::kRandVals[odd ^ (n >> 24)];
	odd   = GenericHash::detail::kRandVals[even ^ ((n >> 16) & 0xff)];
	even  = GenericHash::detail::kRandVals[odd ^ ((n >> 8) & 0xff)];
	odd   = GenericHash::detail::kRandVals[even ^ (n & 0xff)];

	n     = *(p + 2);
	even  = GenericHash::detail::kRandVals[odd ^ (n >> 24)];
	odd   = GenericHash::detail::kRandVals[even ^ ((n >> 16) & 0xff)];
	even  = GenericHash::detail::kRandVals[odd ^ ((n >> 8) & 0xff)];
	odd   = GenericHash::detail::kRandVals[even ^ (n & 0xff)];

	return (even << 8) | odd;
}

inline uint32 Hash16( const void *pKey )
{
	const uint32 *p = (const uint32 *)pKey;
	uint32		even = 0, odd = 0, n = 0;

	n     = *p;
	even  = GenericHash::detail::kRandVals[n & 0xff];
	odd   = GenericHash::detail::kRandVals[((n >> 8) & 0xff)];

	even  = GenericHash::detail::kRandVals[odd ^ (n >> 24)];
	odd   = GenericHash::detail::kRandVals[even ^ ((n >> 16) & 0xff)];
	even  = GenericHash::detail::kRandVals[odd ^ ((n >> 8) & 0xff)];
	odd   = GenericHash::detail::kRandVals[even ^ (n & 0xff)];

	n     = *(p + 1);
	even  = GenericHash::detail::kRandVals[odd ^ (n >> 24)];
	odd   = GenericHash::detail::kRandVals[even ^ ((n >> 16) & 0xff)];
	even  = GenericHash::detail::kRandVals[odd ^ ((n >> 8) & 0xff)];
	odd   = GenericHash::detail::kRandVals[even ^ (n & 0xff)];

	n     = *(p + 2);
	even  = GenericHash::detail::kRandVals[odd ^ (n >> 24)];
	odd   = GenericHash::detail::kRandVals[even ^ ((n >> 16) & 0xff)];
	even  = GenericHash::detail::kRandVals[odd ^ ((n >> 8) & 0xff)];
	odd   = GenericHash::detail::kRandVals[even ^ (n & 0xff)];

	n     = *(p + 3);
	even  = GenericHash::detail::kRandVals[odd ^ (n >> 24)];
	odd   = GenericHash::detail::kRandVals[even ^ ((n >> 16) & 0xff)];
	even  = GenericHash::detail::kRandVals[odd ^ ((n >> 8) & 0xff)];
	odd   = GenericHash::detail::kRandVals[even ^ (n & 0xff)];

	return (even << 8) | odd;
}

inline uint32 HashBlock( const void *pKey, uint32 size )
{
	const uint8 *k    = (const uint8 *)pKey;
	uint32 		even = 0, odd  = 0, n = 0;

	while(size)
	{
		--size;
		n    = *k++;
		even = GenericHash::detail::kRandVals[odd ^ n];
		if(size)
		{
			--size;
			n   = *k++;
			odd = GenericHash::detail::kRandVals[even ^ n];
		}
		else
			break;
	}

	return (even << 8) | odd;
}

// hash a uint32 into a uint32
constexpr uint32 HashIntAlternate( uint32 n)
{
	n = ( n + 0x7ed55d16 ) + ( n << 12 );
	n = ( n ^ 0xc761c23c ) ^ ( n >> 19 );
	n = ( n + 0x165667b1 ) + ( n << 5 );
	n = ( n + 0xd3a2646c ) ^ ( n << 9 );
	n = ( n + 0xfd7046c5 ) + ( n << 3 );
	n = ( n ^ 0xb55a4f09 ) ^ ( n >> 16 );
	return n;
}

constexpr uint32 HashIntConventional( const int n ) // faster but less effective
{
	// first byte
	uint32 hash = 0xAAAAAAAA + (n & 0xFF);
	// second byte
	hash = ( hash << 5 ) + hash + ( (n >> 8) & 0xFF );
	// third byte
	hash = ( hash << 5 ) + hash + ( (n >> 16) & 0xFF );
	// fourth byte
	hash = ( hash << 5 ) + hash + ( (n >> 24) & 0xFF );

	return hash;

	/* this is the old version, which would cause a load-hit-store on every
	   line on a PowerPC, and therefore took hundreds of clocks to execute!
	  
	byte *p = (byte *)&n;
	uint32 hash = 0xAAAAAAAA + *p++;
	hash = ( ( hash << 5 ) + hash ) + *p++;
	hash = ( ( hash << 5 ) + hash ) + *p++;
	return ( ( hash << 5 ) + hash ) + *p;
	*/
}

//-----------------------------------------------------------------------------

template <typename T>
inline uint32 HashItem( const T &item )
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

template <> constexpr uint32 HashItem<int>(const int &key )
{
	return HashInt( key );
}

template <> constexpr uint32 HashItem<uint32>(const uint32 &key )
{
	return HashInt( (int)key );
}

//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// MurmurHash variants
//-----------------------------------------------------------------------------

// AMNOTE: Special case for const string arrays to make compiler happy about const folding the hash value
// during compilation, in turn this results in a hash compiled in instead of byte code to compute the hash.
// Very picky about its internal code so edit with care and little of changes could break constant folding.
// In complex cases might still result in a bytecode being emitted instead of direct hash.
template <bool CASELESS = false, size_t N>
constexpr uint32 MurmurHash2( const char (&key)[N], uint32 seed )
{
	// 'm' and 'r' are mixing constants generated offline.
	// They're not really 'magic', they just happen to work well.

	const uint32 m = 0x5bd1e995;
	const int r = 24;

	// Initialize the hash to a 'random' value

	uint32 len = N - 1;
	uint32 h = seed ^ len;

	// Mix 4 bytes at a time into the hash
	// AMNOTE: ptr casting is not allowed during const folding, so can't use that here
	uint32 offset = 0;

	while(len >= 4)
	{
		uint32 k = 0;
		if constexpr(CASELESS)
		{
			// AMNOTE: Double casting is required to prevent sign extension bugs that could propagate
			// since initial buffer is a signed char, while in original code it casted buffer to unsigned char first
			k = static_cast<uint32>(static_cast<uint8>(GenericHash::detail::to_lower( key[offset + 0] ))) |
				(static_cast<uint32>(static_cast<uint8>(GenericHash::detail::to_lower( key[offset + 1] ))) << 8) |
				(static_cast<uint32>(static_cast<uint8>(GenericHash::detail::to_lower( key[offset + 2] ))) << 16) |
				(static_cast<uint32>(static_cast<uint8>(GenericHash::detail::to_lower( key[offset + 3] ))) << 24);
		}
		else
		{
			k = static_cast<uint32>(static_cast<uint8>(key[offset + 0])) |
				(static_cast<uint32>(static_cast<uint8>(key[offset + 1])) << 8) |
				(static_cast<uint32>(static_cast<uint8>(key[offset + 2])) << 16) |
				(static_cast<uint32>(static_cast<uint8>(key[offset + 3])) << 24);
		}

		k *= m;
		k ^= k >> r;
		k *= m;

		h *= m;
		h ^= k;

		len -= 4;
		offset += 4;
	}

	// AMNOTE: switch case breaks const folding during inlining, so can't use it here
	if constexpr(CASELESS)
	{
		if constexpr(((N - 1) % 4) >= 3) h ^= static_cast<uint32>(static_cast<uint8>(GenericHash::detail::to_lower( key[offset + 2] ))) << 16;
		if constexpr(((N - 1) % 4) >= 2) h ^= static_cast<uint32>(static_cast<uint8>(GenericHash::detail::to_lower( key[offset + 1] ))) << 8;
		if constexpr(((N - 1) % 4) >= 1)
		{
			h ^= static_cast<uint32>(static_cast<uint8>(GenericHash::detail::to_lower( key[offset + 0] )));
			h *= m;
		}
	}
	else
	{
		if constexpr(((N - 1) % 4) >= 3) h ^= static_cast<uint32>(static_cast<uint8>(key[offset + 2])) << 16;
		if constexpr(((N - 1) % 4) >= 2) h ^= static_cast<uint32>(static_cast<uint8>(key[offset + 1])) << 8;
		if constexpr(((N - 1) % 4) >= 1)
		{
			h ^= static_cast<uint32>(static_cast<uint8>(key[offset]));
			h *= m;
		}
	}

	// Do a few final mixes of the hash to ensure the last few
	// bytes are well-incorporated.2

	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;

	return h;
}

// AMNOTE: MurmurHash2 variant for any arbitrary type buffer,
// also supports hashing caseless version of strings,
// and is mostly relevant for runtime strings if so.
// Prefer the const string array variant if possible due to constant folding optimizations.
template <bool CASELESS = false>
inline uint32 MurmurHash2( const void *key, int len, uint32 seed )
{
	// 'm' and 'r' are mixing constants generated offline.
	// They're not really 'magic', they just happen to work well.

	const uint32 m = 0x5bd1e995;
	const int r = 24;

	// Initialize the hash to a 'random' value

	uint32 h = seed ^ len;

	// Mix 4 bytes at a time into the hash

	const unsigned char *data = (const unsigned char *)key;

	while(len >= 4)
	{
		uint32 k = 0;
		if constexpr (CASELESS)
		{
			k = static_cast<uint32>(static_cast<uint8>(GenericHash::detail::to_lower( data[0] ))) |
				(static_cast<uint32>(static_cast<uint8>(GenericHash::detail::to_lower( data[1] ))) << 8) |
				(static_cast<uint32>(static_cast<uint8>(GenericHash::detail::to_lower( data[2] ))) << 16) |
				(static_cast<uint32>(static_cast<uint8>(GenericHash::detail::to_lower( data[3] ))) << 24);
		}
		else
		{
			k = LittleDWord( *(uint32 *)data );
		}

		k *= m;
		k ^= k >> r;
		k *= m;

		h *= m;
		h ^= k;

		data += 4;
		len -= 4;
	}

	// Handle the last few bytes of the input array

	if constexpr (CASELESS)
	{
		switch(len)
		{
			case 3: h ^= static_cast<uint32>(static_cast<uint8>(GenericHash::detail::to_lower( data[2] ))) << 16;
			case 2: h ^= static_cast<uint32>(static_cast<uint8>(GenericHash::detail::to_lower( data[1] ))) << 8;
			case 1: h ^= static_cast<uint32>(static_cast<uint8>(GenericHash::detail::to_lower( data[0] )));
				h *= m;
		};
	}
	else
	{
		switch(len)
		{
			case 3: h ^= data[2] << 16;
			case 2: h ^= data[1] << 8;
			case 1: h ^= data[0];
				h *= m;
		};
	}

	// Do a few final mixes of the hash to ensure the last few
	// bytes are well-incorporated.

	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;

	return h;
}

template <size_t N>
constexpr uint32 MurmurHash2LowerCase( const char (&key)[N], uint32 seed )
{
	return MurmurHash2<true>( key, seed );
}

inline uint32 MurmurHash2LowerCase( const char *key, int len, uint32 seed )
{
	return MurmurHash2<true>( key, len, seed );
}

// AMNOTE: Template is required to enforce compiler to pick the correct overload when inlining
// as otherwise non templated overload would always win the pick thus no const folding would happen
template <typename T, std::enable_if_t<std::is_same_v<T, const char *>, int> = 0>
inline uint32 MurmurHash2LowerCase( T key, uint32 seed ) { return MurmurHash2LowerCase( key, strlen( key ), seed ); }

inline uint32 HashString( const char *pszKey )
{
	return MurmurHash2( pszKey, strlen( pszKey ), 0x3501A674 );
}

inline uint32 HashStringCaseless( const char *pszKey )
{
	return MurmurHash2LowerCase( pszKey, 0x3501A674 );
}

template<> inline uint32 HashItem<const char *>( const char *const &pszKey )
{
	return HashString( pszKey );
}

template<> inline uint32 HashItem<char *>( char *const &pszKey )
{
	return HashString( pszKey );
}

#endif /* !GENERICHASH_H */
