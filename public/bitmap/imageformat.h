//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//===========================================================================//

#ifndef IMAGEFORMAT_H
#define IMAGEFORMAT_H

#ifdef _WIN32
#pragma once
#endif


#include <stdio.h>

//-----------------------------------------------------------------------------
// The various image format types
//-----------------------------------------------------------------------------


// don't bitch that inline functions aren't used!!!!
#pragma warning(disable : 4514)

enum ImageFormat 
{
	IMAGE_FORMAT_UNKNOWN  = -1,
	IMAGE_FORMAT_RGBA8888 = 0, 
	IMAGE_FORMAT_ABGR8888, 
	IMAGE_FORMAT_RGB888, 
	IMAGE_FORMAT_BGR888,
	IMAGE_FORMAT_RGB565, 
	IMAGE_FORMAT_I8,
	IMAGE_FORMAT_IA88,
	IMAGE_FORMAT_P8,
	IMAGE_FORMAT_A8,
	IMAGE_FORMAT_RGB888_BLUESCREEN,
	IMAGE_FORMAT_BGR888_BLUESCREEN,
	IMAGE_FORMAT_ARGB8888,
	IMAGE_FORMAT_BGRA8888,
	IMAGE_FORMAT_DXT1,
	IMAGE_FORMAT_DXT3,
	IMAGE_FORMAT_DXT5,
	IMAGE_FORMAT_BGRX8888,
	IMAGE_FORMAT_BGR565,
	IMAGE_FORMAT_BGRX5551,
	IMAGE_FORMAT_BGRA4444,
	IMAGE_FORMAT_DXT1_ONEBITALPHA,
	IMAGE_FORMAT_BGRA5551,
	IMAGE_FORMAT_UV88,
	IMAGE_FORMAT_UVWQ8888,
	IMAGE_FORMAT_RGBA16161616F,
	// GR - HDR
	IMAGE_FORMAT_RGBA16161616,
	IMAGE_FORMAT_UVLX8888,
	IMAGE_FORMAT_R32F,	// Single-channel 32-bit floating point
	IMAGE_FORMAT_RGB323232F,
	IMAGE_FORMAT_RGBA32323232F,

#ifdef _XBOX
	// adds support for these linear formats
	// non-linear uncompressed formats are swizzled
	IMAGE_FORMAT_LINEAR_BGRX8888,
	IMAGE_FORMAT_LINEAR_RGBA8888,
	IMAGE_FORMAT_LINEAR_ABGR8888,
	IMAGE_FORMAT_LINEAR_ARGB8888,
	IMAGE_FORMAT_LINEAR_BGRA8888,
	IMAGE_FORMAT_LINEAR_RGB888,
	IMAGE_FORMAT_LINEAR_BGR888,
	IMAGE_FORMAT_LINEAR_BGRX5551,
	IMAGE_FORMAT_LINEAR_I8,
#endif

	NUM_IMAGE_FORMATS
};


//-----------------------------------------------------------------------------
// Color structures
//-----------------------------------------------------------------------------
struct BGRA8888_t
{
	unsigned char b;		// change the order of names to change the 
	unsigned char g;		//  order of the output ARGB or BGRA, etc...
	unsigned char r;		//  Last one is MSB, 1st is LSB.
	unsigned char a;
	inline BGRA8888_t& operator=( const BGRA8888_t& in )
	{
		*( unsigned int * )this = *( unsigned int * )&in;
		return *this;
	}
};

struct RGBA8888_t
{
	unsigned char r;		// change the order of names to change the 
	unsigned char g;		//  order of the output ARGB or BGRA, etc...
	unsigned char b;		//  Last one is MSB, 1st is LSB.
	unsigned char a;
	inline RGBA8888_t& operator=( const BGRA8888_t& in )
	{
		r = in.r;
		g = in.g;
		b = in.b;
		a = in.a;
		return *this;
	}
};

struct RGB888_t
{
	unsigned char r;
	unsigned char g;
	unsigned char b;
	inline RGB888_t& operator=( const BGRA8888_t& in )
	{
		r = in.r;
		g = in.g;
		b = in.b;
		return *this;
	}
	inline bool operator==( const RGB888_t& in ) const
	{
		return ( r == in.r ) && ( g == in.g ) && ( b == in.b );
	}
	inline bool operator!=( const RGB888_t& in ) const
	{
		return ( r != in.r ) || ( g != in.g ) || ( b != in.b );
	}
};

struct BGR888_t
{
	unsigned char b;
	unsigned char g;
	unsigned char r;
	inline BGR888_t& operator=( const BGRA8888_t& in )
	{
		r = in.r;
		g = in.g;
		b = in.b;
		return *this;
	}
};

struct BGR565_t
{
	unsigned short b : 5;		// order of names changes
	unsigned short g : 6;		//  byte order of output to 32 bit
	unsigned short r : 5;
	inline BGR565_t& operator=( const BGRA8888_t& in )
	{
		r = in.r >> 3;
		g = in.g >> 2;
		b = in.b >> 3;
		return *this;
	}
};

struct BGRA5551_t
{
	unsigned short b : 5;		// order of names changes
	unsigned short g : 5;		//  byte order of output to 32 bit
	unsigned short r : 5;
	unsigned short a : 1;
	inline BGRA5551_t& operator=( const BGRA8888_t& in )
	{
		r = in.r >> 3;
		g = in.g >> 3;
		b = in.b >> 3;
		a = in.a >> 7;
		return *this;
	}
};

struct BGRA4444_t
{
	unsigned short b : 4;		// order of names changes
	unsigned short g : 4;		//  byte order of output to 32 bit
	unsigned short r : 4;
	unsigned short a : 4;
	inline BGRA4444_t& operator=( const BGRA8888_t& in )
	{
		r = in.r >> 4;
		g = in.g >> 4;
		b = in.b >> 4;
		a = in.a >> 4;
		return *this;
	}
};

struct RGBX5551_t
{
	unsigned short r : 5;
	unsigned short g : 5;
	unsigned short b : 5;
	unsigned short x : 1;
	inline RGBX5551_t& operator=( const BGRA8888_t& in )
	{
		r = in.r >> 3;
		g = in.g >> 3;
		b = in.b >> 3;
		return *this;
	}
};



//-----------------------------------------------------------------------------
// some important constants
//-----------------------------------------------------------------------------
#define ARTWORK_GAMMA ( 2.2f )
#define IMAGE_MAX_DIM ( 2048 )


//-----------------------------------------------------------------------------
// information about each image format
//-----------------------------------------------------------------------------
struct ImageFormatInfo_t
{
	char* m_pName;
	int m_NumBytes;
	int m_NumRedBits;
	int m_NumGreeBits;
	int m_NumBlueBits;
	int m_NumAlphaBits;
	bool m_IsCompressed;
};


//-----------------------------------------------------------------------------
// Various methods related to pixelmaps and color formats
//-----------------------------------------------------------------------------
namespace ImageLoader
{

bool GetInfo( const char *fileName, int *width, int *height, enum ImageFormat *imageFormat, float *sourceGamma );
int  GetMemRequired( int width, int height, int depth, ImageFormat imageFormat, bool mipmap );
int  GetMipMapLevelByteOffset( int width, int height, enum ImageFormat imageFormat, int skipMipLevels );
void GetMipMapLevelDimensions( int *width, int *height, int skipMipLevels );
int  GetNumMipMapLevels( int width, int height, int depth = 1 );
bool Load( unsigned char *imageData, const char *fileName, int width, int height, enum ImageFormat imageFormat, float targetGamma, bool mipmap );
bool Load( unsigned char *imageData, FILE *fp, int width, int height, 
			enum ImageFormat imageFormat, float targetGamma, bool mipmap );

// convert from any image format to any other image format.
// return false if the conversion cannot be performed.
// Strides denote the number of bytes per each line, 
// by default assumes width * # of bytes per pixel
bool ConvertImageFormat( unsigned char *src, enum ImageFormat srcImageFormat,
		                 unsigned char *dst, enum ImageFormat dstImageFormat, 
						 int width, int height, int srcStride = 0, int dstStride = 0 );

// Flags for ResampleRGBA8888
enum
{
	RESAMPLE_NORMALMAP = 0x1,
	RESAMPLE_ALPHATEST = 0x2,
	RESAMPLE_NICE_FILTER = 0x4,
	RESAMPLE_CLAMPS = 0x8,
	RESAMPLE_CLAMPT = 0x10,
	RESAMPLE_CLAMPU = 0x20,
};

struct ResampleInfo_t
{
	ResampleInfo_t() : m_flColorScale( 1.0f ), m_nFlags(0), m_flAlphaThreshhold(0.4f),
		m_flAlphaHiFreqThreshhold(0.4f), m_nSrcDepth(1), m_nDestDepth(1) {}

	unsigned char *m_pSrc;
	unsigned char *m_pDest;
	int m_nSrcWidth;
	int m_nSrcHeight;
	int m_nSrcDepth;
	int m_nDestWidth;
	int m_nDestHeight;
	int m_nDestDepth;
	float m_flSrcGamma;
	float m_flDestGamma;
	float m_flColorScale;
	float m_flAlphaThreshhold;
	float m_flAlphaHiFreqThreshhold;
	int m_nFlags;
};

bool ResampleRGBA8888( const ResampleInfo_t &info );
bool ResampleRGBA16161616( const ResampleInfo_t &info );
bool ResampleRGB323232F( const ResampleInfo_t &info );

void ConvertNormalMapRGBA8888ToDUDVMapUVLX8888( unsigned char *src, int width, int height,
										                     unsigned char *dst_ );
void ConvertNormalMapRGBA8888ToDUDVMapUVWQ8888( unsigned char *src, int width, int height,
										                     unsigned char *dst_ );
void ConvertNormalMapRGBA8888ToDUDVMapUV88( unsigned char *src, int width, int height,
										                     unsigned char *dst_ );

void ConvertIA88ImageToNormalMapRGBA8888( unsigned char *src, int width, 
														int height, unsigned char *dst,
														float bumpScale );

void NormalizeNormalMapRGBA8888( unsigned char *src, int numTexels );


//-----------------------------------------------------------------------------
// Gamma correction
//-----------------------------------------------------------------------------
void GammaCorrectRGBA8888( unsigned char *src, unsigned char* dst,
					int width, int height, int depth, float srcGamma, float dstGamma );


//-----------------------------------------------------------------------------
// Makes a gamma table
//-----------------------------------------------------------------------------
void ConstructGammaTable( unsigned char* pTable, float srcGamma, float dstGamma );


//-----------------------------------------------------------------------------
// Gamma corrects using a previously constructed gamma table
//-----------------------------------------------------------------------------
void GammaCorrectRGBA8888( unsigned char* pSrc, unsigned char* pDst,
						  int width, int height, int depth, unsigned char* pGammaTable );


//-----------------------------------------------------------------------------
// Generates a number of mipmap levels
//-----------------------------------------------------------------------------
void GenerateMipmapLevels( unsigned char* pSrc, unsigned char* pDst, int width,
	int height,	int depth, ImageFormat imageFormat, float srcGamma, float dstGamma, 
	int numLevels = 0 );


//-----------------------------------------------------------------------------
// operations on square images (src and dst can be the same)
//-----------------------------------------------------------------------------
bool RotateImageLeft( unsigned char *src, unsigned char *dst, 
					  int widthHeight, ImageFormat imageFormat );
bool RotateImage180( unsigned char *src, unsigned char *dst, 
					 int widthHeight, ImageFormat imageFormat );
bool FlipImageVertically( void *pSrc, void *pDst, int nWidth, int nHeight, ImageFormat imageFormat, int nDstStride = 0 );
bool FlipImageHorizontally( void *pSrc, void *pDst, int nWidth, int nHeight, ImageFormat imageFormat, int nDstStride = 0 );
bool SwapAxes( unsigned char *src, 
			  int widthHeight, ImageFormat imageFormat );


//-----------------------------------------------------------------------------
// Returns info about each image format
//-----------------------------------------------------------------------------
ImageFormatInfo_t const& ImageFormatInfo( ImageFormat fmt );


//-----------------------------------------------------------------------------
// Gets the name of the image format
//-----------------------------------------------------------------------------
inline char const* GetName( ImageFormat fmt )
{
	return ImageFormatInfo(fmt).m_pName;
}


//-----------------------------------------------------------------------------
// Gets the size of the image format in bytes
//-----------------------------------------------------------------------------
inline int SizeInBytes( ImageFormat fmt )
{
	return ImageFormatInfo(fmt).m_NumBytes;
}

//-----------------------------------------------------------------------------
// Does the image format support transparency?
//-----------------------------------------------------------------------------
inline bool IsTransparent( ImageFormat fmt )
{
	return ImageFormatInfo(fmt).m_NumAlphaBits > 0;
}


//-----------------------------------------------------------------------------
// Is the image format compressed?
//-----------------------------------------------------------------------------
inline bool IsCompressed( ImageFormat fmt )
{
	return ImageFormatInfo(fmt).m_IsCompressed;
}

} // end namespace ImageLoader

#endif // IMAGEFORMAT_H
