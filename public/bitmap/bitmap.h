//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//===========================================================================//

#ifndef BITMAP_H
#define BITMAP_H

#ifdef _WIN32
#pragma once
#endif


#include "bitmap/imageformat.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CUtlBuffer;


//-----------------------------------------------------------------------------
// A Bitmap
//-----------------------------------------------------------------------------
struct Bitmap_t
{
	Bitmap_t();
	~Bitmap_t();
	void Init( int nWidth, int nHeight, ImageFormat imageFormat );
	unsigned char *GetPixel( int x, int y );

	int m_nWidth;
	int m_nHeight;
	ImageFormat m_ImageFormat;
	unsigned char *m_pBits;
};

inline Bitmap_t::Bitmap_t()
{
	m_nWidth = 0;
	m_nHeight = 0;
	m_ImageFormat = IMAGE_FORMAT_UNKNOWN;
	m_pBits = NULL;
}

inline Bitmap_t::~Bitmap_t()
{
	if ( m_pBits )
	{
		delete[] m_pBits;
		m_pBits = NULL;
	}
}

inline void Bitmap_t::Init( int nWidth, int nHeight, ImageFormat imageFormat )
{
	if ( m_pBits )
	{
		delete[] m_pBits;
		m_pBits = NULL;
	}

	m_nWidth = nWidth;
	m_nHeight = nHeight;
	m_ImageFormat = imageFormat;
	m_pBits = new unsigned char[ nWidth * nHeight * ImageLoader::SizeInBytes( m_ImageFormat ) ];
}

inline unsigned char *Bitmap_t::GetPixel( int x, int y )
{
	if ( !m_pBits )
		return NULL;

	int nPixelSize = ImageLoader::SizeInBytes( m_ImageFormat );
	return &m_pBits[ ( m_nWidth * y + x ) * nPixelSize ];
}


//-----------------------------------------------------------------------------
// Loads a bitmap from an arbitrary file: could be a TGA, PSD, or PFM.
// LoadBitmap autodetects which type, and returns it
//-----------------------------------------------------------------------------
enum BitmapFileType_t 
{ 
	BITMAP_FILE_TYPE_UNKNOWN = -1, 
	BITMAP_FILE_TYPE_PSD = 0, 
	BITMAP_FILE_TYPE_TGA, 
	BITMAP_FILE_TYPE_PFM, 
}; 

BitmapFileType_t LoadBitmapFile( CUtlBuffer &buf, Bitmap_t *pBitmap );


//-----------------------------------------------------------------------------
// PFM file loading related methods
//-----------------------------------------------------------------------------
bool PFMReadFileR32F( CUtlBuffer &fileBuffer, Bitmap_t &bitmap, float pfmScale );
bool PFMReadFileRGB323232F( CUtlBuffer &fileBuffer, Bitmap_t &bitmap, float pfmScale );
bool PFMReadFileRGBA32323232F( CUtlBuffer &fileBuffer, Bitmap_t &bitmap, float pfmScale );
bool PFMGetInfo_AndAdvanceToTextureBits( CUtlBuffer &pfmBuffer, int &nWidth, int &nHeight, ImageFormat &imageFormat );


#endif // BITMAP_H
