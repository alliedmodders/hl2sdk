//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Generic MD5 hashing algo
//
//=============================================================================//

#ifndef CHECKSUM_MD5_H
#define CHECKSUM_MD5_H

#ifdef _WIN32
#pragma once
#endif

#include "platform.h"

// 16 bytes == 128 bit digest
#define MD5_DIGEST_LENGTH 16  

// MD5 Hash
typedef struct
{
	unsigned int	buf[4];
    unsigned int	bits[2];
    unsigned char	in[64];
} MD5Context_t;

PLATFORM_INTERFACE void MD5Init( MD5Context_t *context );
PLATFORM_INTERFACE void MD5Update( MD5Context_t *context, unsigned char const *buf, unsigned int len );
PLATFORM_INTERFACE void MD5Final( unsigned char digest[ MD5_DIGEST_LENGTH ], MD5Context_t *context );

PLATFORM_INTERFACE char *MD5_Print(unsigned char *digest, int hashlen );

PLATFORM_INTERFACE unsigned int MD5_PseudoRandom(unsigned int nSeed);

#endif // CHECKSUM_MD5_H
