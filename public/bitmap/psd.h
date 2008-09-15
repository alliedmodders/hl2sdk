//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Methods relating to saving + loading PSD files (photoshop)
//
// $NoKeywords: $
//===========================================================================//

#ifndef PSD_H
#define PSD_H

#ifdef _WIN32
#pragma once
#endif

#include "bitmap/imageformat.h" //ImageFormat enum definition

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CUtlBuffer;
struct Bitmap_t;


//-----------------------------------------------------------------------------
// Is a file a PSD file?
//-----------------------------------------------------------------------------
bool IsPSDFile( const char *pFileName, const char *pPathID );
bool IsPSDFile( CUtlBuffer &buf );


//-----------------------------------------------------------------------------
// Returns information about the PSD file
//-----------------------------------------------------------------------------
bool PSDGetInfo( const char *pFileName, const char *pPathID, int *pWidth, int *pHeight, ImageFormat *pImageFormat, float *pSourceGamma );
bool PSDGetInfo( CUtlBuffer &buf, int *pWidth, int *pHeight, ImageFormat *pImageFormat, float *pSourceGamma );


//-----------------------------------------------------------------------------
// Reads the PSD file into the specified buffer
//-----------------------------------------------------------------------------
bool PSDReadFileRGBA8888( CUtlBuffer &buf, Bitmap_t &bitmap );
bool PSDReadFileRGBA8888( const char *pFileName, const char *pPathID, Bitmap_t &bitmap );


#endif // PSD_H
