//========= Copyright � 1996-2006, Valve LLC, All rights reserved. ============
//
// Purpose: StudioMDL byteswapping functions.
//
// $NoKeywords: $
//=============================================================================
#ifndef STUDIOBYTESWAP_H
#define STUDIOBYTESWAP_H

#if defined(_WIN32)
#pragma once
#endif

#include "cbyteswap.h"
struct studiohdr_t;
class IPhysicsCollision;

namespace StudioByteSwap
{
typedef bool (*CompressFunc_t)( const void *pInput, int inputSize, void **pOutput, int *pOutputSize );

//void SetTargetBigEndian( bool bigEndian );
void	ActivateByteSwapping( bool bActivate );
void	SourceIsNative( bool bActivate );
void	SetVerbose( bool bVerbose );
void	SetCollisionInterface( IPhysicsCollision *pPhysicsCollision );

int		ByteswapStudioFile( const char *pFilename, void *pOutBase, int outBaseSize, const void *pFileBase, int fileSize, studiohdr_t *pHdr, CompressFunc_t pCompressFunc = NULL );
int		ByteswapPHY( void *pOutBase, int outBaseSize, const void *pFileBase, int fileSize );
int		ByteswapANI( studiohdr_t* pHdr, void *pOutBase, int outBaseSize, const void *pFileBase, int filesize );
int		ByteswapVVD( void *pOutBase, int outBaseSize, const void *pFileBase, int fileSize );
int		ByteswapVTX( void *pOutBase, int outBaseSize, const void *pFileBase, int fileSize );
int		ByteswapMDL( void *pOutBase, int OutBaseSize, const void *pFileBase, int fileSize );

#define BYTESWAP_ALIGNMENT_PADDING		4096
#define ERROR_MISALIGNED_DATA			-1
#define ERROR_VERSION					-2
}

#endif // STUDIOBYTESWAP_H
