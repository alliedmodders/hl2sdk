//===== Copyright 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//

#ifndef IMAGEFORMAT_H
#define IMAGEFORMAT_H

#ifdef _WIN32
#pragma once
#endif

#include <stdio.h>
#include "tier0/platform.h"
#include "tier0/dbg.h"

enum NormalDecodeMode_t
{
	NORMAL_DECODE_NONE			= 0,
	NORMAL_DECODE_ATI2N			= 1,
	NORMAL_DECODE_ATI2N_ALPHA	= 2
};

// Forward declaration
#ifdef _WIN32
typedef enum _D3DFORMAT D3DFORMAT;
typedef enum DXGI_FORMAT;
#endif

//-----------------------------------------------------------------------------
// The various image format types
//-----------------------------------------------------------------------------

// don't bitch that inline functions aren't used!!!!
#ifdef _MSC_VER
#pragma warning(disable : 4514)
#endif

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
	IMAGE_FORMAT_RGBA16161616,
	IMAGE_FORMAT_UVLX8888,
	IMAGE_FORMAT_R32F,			// Single-channel 32-bit floating point
	IMAGE_FORMAT_RGB323232F,	// NOTE: D3D9 does not have this format
	IMAGE_FORMAT_RGBA32323232F,
	IMAGE_FORMAT_RG1616F,
	IMAGE_FORMAT_RG3232F,
	IMAGE_FORMAT_RGBX8888,

	IMAGE_FORMAT_NULL,			// Dummy format which takes no video memory

	// Compressed normal map formats
	IMAGE_FORMAT_ATI2N,			// One-surface ATI2N / DXN format
	IMAGE_FORMAT_ATI1N,			// Two-surface ATI1N format

	IMAGE_FORMAT_RGBA1010102,	// 10 bit-per component render targets
	IMAGE_FORMAT_BGRA1010102,
	IMAGE_FORMAT_R16F,			// 16 bit FP format

	// Depth-stencil texture formats
	IMAGE_FORMAT_D16,
	IMAGE_FORMAT_D15S1,
	IMAGE_FORMAT_D32,
	IMAGE_FORMAT_D24S8,
	IMAGE_FORMAT_LINEAR_D24S8,
	IMAGE_FORMAT_D24X8,
	IMAGE_FORMAT_D24X4S4,
	IMAGE_FORMAT_D24FS8,
	IMAGE_FORMAT_D16_SHADOW,	// Specific formats for shadow mapping
	IMAGE_FORMAT_D24X8_SHADOW,	// Specific formats for shadow mapping

	// supporting these specific formats as non-tiled for procedural cpu access (360-specific)
	IMAGE_FORMAT_LINEAR_BGRX8888,
	IMAGE_FORMAT_LINEAR_RGBA8888,
	IMAGE_FORMAT_LINEAR_ABGR8888,
	IMAGE_FORMAT_LINEAR_ARGB8888,
	IMAGE_FORMAT_LINEAR_BGRA8888,
	IMAGE_FORMAT_LINEAR_RGB888,
	IMAGE_FORMAT_LINEAR_BGR888,
	IMAGE_FORMAT_LINEAR_BGRX5551,
	IMAGE_FORMAT_LINEAR_I8,
	IMAGE_FORMAT_LINEAR_RGBA16161616,

	IMAGE_FORMAT_LE_BGRX8888,
	IMAGE_FORMAT_LE_BGRA8888,

	NUM_IMAGE_FORMATS
};

#if defined( POSIX  ) || defined( DX_TO_GL_ABSTRACTION )
typedef enum _D3DFORMAT
	{
		D3DFMT_INDEX16,
		D3DFMT_D16,
		D3DFMT_D24S8,
		D3DFMT_A8R8G8B8,
		D3DFMT_A4R4G4B4,
		D3DFMT_X8R8G8B8,
		D3DFMT_R5G6R5,
		D3DFMT_X1R5G5B5,
		D3DFMT_A1R5G5B5,
		D3DFMT_L8,
		D3DFMT_A8L8,
		D3DFMT_A,
		D3DFMT_DXT1,
		D3DFMT_DXT3,
		D3DFMT_DXT5,
		D3DFMT_V8U8,
		D3DFMT_Q8W8V8U8,
		D3DFMT_X8L8V8U8,
		D3DFMT_A16B16G16R16F,
		D3DFMT_A16B16G16R16,
		D3DFMT_R32F,
		D3DFMT_A32B32G32R32F,
		D3DFMT_R8G8B8,
		D3DFMT_D24X4S4,
		D3DFMT_A8,
		D3DFMT_R5G6B5,
		D3DFMT_D15S1,
		D3DFMT_D24X8,
		D3DFMT_VERTEXDATA,
		D3DFMT_INDEX32,

		// adding fake D3D format names for the vendor specific ones (eases debugging/logging)
		
		// NV shadow depth tex
		D3DFMT_NV_INTZ		= 0x5a544e49,	// MAKEFOURCC('I','N','T','Z')
		D3DFMT_NV_RAWZ		= 0x5a574152,	// MAKEFOURCC('R','A','W','Z')

		// NV null tex
		D3DFMT_NV_NULL		= 0x4c4c554e,	// MAKEFOURCC('N','U','L','L')

		// ATI shadow depth tex
		D3DFMT_ATI_D16		= 0x36314644,	// MAKEFOURCC('D','F','1','6')
		D3DFMT_ATI_D24S8	= 0x34324644,	// MAKEFOURCC('D','F','2','4')

		// ATI 1N and 2N compressed tex
		D3DFMT_ATI_2N		= 0x32495441,	// MAKEFOURCC('A', 'T', 'I', '2')
		D3DFMT_ATI_1N		= 0x31495441,	// MAKEFOURCC('A', 'T', 'I', '1')
		
		D3DFMT_UNKNOWN
	} D3DFORMAT;
#endif

//-----------------------------------------------------------------------------
// Color structures
//-----------------------------------------------------------------------------
#ifdef _MSC_VER
#pragma warning ( disable : 4293 )
#endif
template< int nBitCount > FORCEINLINE int ConvertTo10Bit( int x )
{
	switch ( nBitCount )
	{
		case 1:
			return ( x & 0x1 ) ? 0x3FF : 0;
		case 2:
			return ( x << 8 ) | ( x << 6 ) | ( x << 4 ) | ( x << 2 ) | x;
		case 3:
			return ( x << 7 ) | ( x << 4 ) | ( x << 1 ) | ( x >> 2 );
		case 4:
			return ( x << 6 ) | ( x << 2 ) | ( x >> 2 );
		default:
		{
			int rshift = ( nBitCount - ( 10 - nBitCount ) );
			return ( x << ( 10 - nBitCount ) ) | ( x >> rshift );
		}
	}
}
#ifdef _MSC_VER
#pragma warning ( default : 4293 )
#endif

template< int nBitCount > FORCEINLINE int ConvertFrom10Bit( int x )
{
	return ( x >> ( 10 - nBitCount ) );
}

struct R32F_t
{
	float32 r;
};

struct RG3232F_t
{
	float32 r;
	float32 g;
};

struct RGB323232F_t
{
	float32 r;
	float32 g;
	float32 b;
};

struct RGBA32323232F_t
{
	float32 r;
	float32 g;
	float32 b;
	float32 a;
};

struct RGBA1010102_t
{
	uint32 r : 10;	
	uint32 g : 10;	
	uint32 b : 10;	
	uint32 a : 2;
	inline int RTo10Bit( ) const { return ConvertTo10Bit<10>( r ); }
	inline int GTo10Bit( ) const { return ConvertTo10Bit<10>( g ); }
	inline int BTo10Bit( ) const { return ConvertTo10Bit<10>( b ); }
	inline int ATo10Bit( ) const { return ConvertTo10Bit<2>( a ); }
	inline void RFrom10Bit( int r10 ) { r = ConvertFrom10Bit<10>( r10 ); }
	inline void GFrom10Bit( int g10 ) { g = ConvertFrom10Bit<10>( g10 ); }
	inline void BFrom10Bit( int b10 ) { b = ConvertFrom10Bit<10>( b10 ); }
	inline void AFrom10Bit( int a10 ) { a = ConvertFrom10Bit<10>( a10 ); }
};

struct BGRA1010102_t
{
	uint32 b : 10;	
	uint32 g : 10;	
	uint32 r : 10;	
	uint32 a : 2;
	inline int RTo10Bit( ) const { return ConvertTo10Bit<10>( r ); }
	inline int GTo10Bit( ) const { return ConvertTo10Bit<10>( g ); }
	inline int BTo10Bit( ) const { return ConvertTo10Bit<10>( b ); }
	inline int ATo10Bit( ) const { return ConvertTo10Bit<2>( a ); }
	inline void RFrom10Bit( int r10 ) { r = ConvertFrom10Bit<10>( r10 ); }
	inline void GFrom10Bit( int g10 ) { g = ConvertFrom10Bit<10>( g10 ); }
	inline void BFrom10Bit( int b10 ) { b = ConvertFrom10Bit<10>( b10 ); }
	inline void AFrom10Bit( int a10 ) { a = ConvertFrom10Bit<10>( a10 ); }
};

struct BGRA8888_t
{
	uint8 b;		// change the order of names to change the 
	uint8 g;		//  order of the output ARGB or BGRA, etc...
	uint8 r;		//  Last one is MSB, 1st is LSB.
	uint8 a;
	inline BGRA8888_t& operator=( const BGRA8888_t& in )
	{
		*( unsigned int * )this = *( unsigned int * )&in;
		return *this;
	}
	inline int RTo10Bit( ) const { return ConvertTo10Bit<8>( r ); }
	inline int GTo10Bit( ) const { return ConvertTo10Bit<8>( g ); }
	inline int BTo10Bit( ) const { return ConvertTo10Bit<8>( b ); }
	inline int ATo10Bit( ) const { return ConvertTo10Bit<8>( a ); }
	inline void RFrom10Bit( int r10 ) { r = ConvertFrom10Bit<8>( r10 ); }
	inline void GFrom10Bit( int g10 ) { g = ConvertFrom10Bit<8>( g10 ); }
	inline void BFrom10Bit( int b10 ) { b = ConvertFrom10Bit<8>( b10 ); }
	inline void AFrom10Bit( int a10 ) { a = ConvertFrom10Bit<8>( a10 ); }
};

struct ABGR8888_t
{
	uint8 a;
	uint8 b;		// change the order of names to change the 
	uint8 g;		//  order of the output ARGB or BGRA, etc...
	uint8 r;		//  Last one is MSB, 1st is LSB.
	inline ABGR8888_t& operator=( const BGRA8888_t& in )
	{
		r = in.r;
		g = in.g;
		b = in.b;
		a = in.a;
		return *this;
	}
	inline int RTo10Bit( ) const { return ConvertTo10Bit<8>( r ); }
	inline int GTo10Bit( ) const { return ConvertTo10Bit<8>( g ); }
	inline int BTo10Bit( ) const { return ConvertTo10Bit<8>( b ); }
	inline int ATo10Bit( ) const { return ConvertTo10Bit<8>( a ); }
	inline void RFrom10Bit( int r10 ) { r = ConvertFrom10Bit<8>( r10 ); }
	inline void GFrom10Bit( int g10 ) { g = ConvertFrom10Bit<8>( g10 ); }
	inline void BFrom10Bit( int b10 ) { b = ConvertFrom10Bit<8>( b10 ); }
	inline void AFrom10Bit( int a10 ) { a = ConvertFrom10Bit<8>( a10 ); }
};


struct RGBA8888_t
{
	uint8 r;		// change the order of names to change the 
	uint8 g;		//  order of the output ARGB or BGRA, etc...
	uint8 b;		//  Last one is MSB, 1st is LSB.
	uint8 a;
	inline RGBA8888_t& operator=( const BGRA8888_t& in )
	{
		r = in.r;
		g = in.g;
		b = in.b;
		a = in.a;
		return *this;
	}
	inline int RTo10Bit( ) const { return ConvertTo10Bit<8>( r ); }
	inline int GTo10Bit( ) const { return ConvertTo10Bit<8>( g ); }
	inline int BTo10Bit( ) const { return ConvertTo10Bit<8>( b ); }
	inline int ATo10Bit( ) const { return ConvertTo10Bit<8>( a ); }
	inline void RFrom10Bit( int r10 ) { r = ConvertFrom10Bit<8>( r10 ); }
	inline void GFrom10Bit( int g10 ) { g = ConvertFrom10Bit<8>( g10 ); }
	inline void BFrom10Bit( int b10 ) { b = ConvertFrom10Bit<8>( b10 ); }
	inline void AFrom10Bit( int a10 ) { a = ConvertFrom10Bit<8>( a10 ); }
};

struct RGB888_t
{
	uint8 r;
	uint8 g;
	uint8 b;
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
	inline int RTo10Bit( ) const { return ConvertTo10Bit<8>( r ); }
	inline int GTo10Bit( ) const { return ConvertTo10Bit<8>( g ); }
	inline int BTo10Bit( ) const { return ConvertTo10Bit<8>( b ); }
	inline void RFrom10Bit( int r10 ) { r = ConvertFrom10Bit<8>( r10 ); }
	inline void GFrom10Bit( int g10 ) { g = ConvertFrom10Bit<8>( g10 ); }
	inline void BFrom10Bit( int b10 ) { b = ConvertFrom10Bit<8>( b10 ); }
};

struct BGR888_t
{
	uint8 b;
	uint8 g;
	uint8 r;
	inline BGR888_t& operator=( const BGRA8888_t& in )
	{
		r = in.r;
		g = in.g;
		b = in.b;
		return *this;
	}
	inline int RTo10Bit( ) const { return ConvertTo10Bit<8>( r ); }
	inline int GTo10Bit( ) const { return ConvertTo10Bit<8>( g ); }
	inline int BTo10Bit( ) const { return ConvertTo10Bit<8>( b ); }
	inline void RFrom10Bit( int r10 ) { r = ConvertFrom10Bit<8>( r10 ); }
	inline void GFrom10Bit( int g10 ) { g = ConvertFrom10Bit<8>( g10 ); }
	inline void BFrom10Bit( int b10 ) { b = ConvertFrom10Bit<8>( b10 ); }
};

struct BGRX8888_t
{
	uint8 b;
	uint8 g;
	uint8 r;
	uint8 x;
	inline BGRX8888_t& operator=( const BGRA8888_t& in )
	{
		r = in.r;
		g = in.g;
		b = in.b;
		x = 255;
		return *this;
	}
	inline int RTo10Bit( ) const { return ConvertTo10Bit<8>( r ); }
	inline int GTo10Bit( ) const { return ConvertTo10Bit<8>( g ); }
	inline int BTo10Bit( ) const { return ConvertTo10Bit<8>( b ); }
	inline void RFrom10Bit( int r10 ) { r = ConvertFrom10Bit<8>( r10 ); }
	inline void GFrom10Bit( int g10 ) { g = ConvertFrom10Bit<8>( g10 ); }
	inline void BFrom10Bit( int b10 ) { b = ConvertFrom10Bit<8>( b10 ); }
};


// 360 uses this structure for x86 dxt decoding
#if defined( _X360 )
#pragma bitfield_order( push, lsb_to_msb )
#endif
struct BGR565_t
{
	uint16 b : 5;		// order of names changes
	uint16 g : 6;		//  byte order of output to 32 bit
	uint16 r : 5;
	inline BGR565_t& operator=( const BGRA8888_t& in )
	{
		r = in.r >> 3;
		g = in.g >> 2;
		b = in.b >> 3;
		return *this;
	}
	inline BGR565_t &Set( int red, int green, int blue )
	{
		r = red >> 3;
		g = green >> 2;
		b = blue >> 3;
		return *this;
	}
	inline int RTo10Bit( ) const { return ConvertTo10Bit<5>( r ); }
	inline int GTo10Bit( ) const { return ConvertTo10Bit<6>( g ); }
	inline int BTo10Bit( ) const { return ConvertTo10Bit<5>( b ); }
	inline void RFrom10Bit( int r10 ) { r = ConvertFrom10Bit<5>( r10 ); }
	inline void GFrom10Bit( int g10 ) { g = ConvertFrom10Bit<6>( g10 ); }
	inline void BFrom10Bit( int b10 ) { b = ConvertFrom10Bit<5>( b10 ); }
};
#if defined( _X360 )
#pragma bitfield_order( pop )
#endif

struct BGRA5551_t
{
	uint16 b : 5;		// order of names changes
	uint16 g : 5;		//  byte order of output to 32 bit
	uint16 r : 5;
	uint16 a : 1;
	inline BGRA5551_t& operator=( const BGRA8888_t& in )
	{
		r = in.r >> 3;
		g = in.g >> 3;
		b = in.b >> 3;
		a = in.a >> 7;
		return *this;
	}
	inline int RTo10Bit( ) const { return ConvertTo10Bit<5>( r ); }
	inline int GTo10Bit( ) const { return ConvertTo10Bit<5>( g ); }
	inline int BTo10Bit( ) const { return ConvertTo10Bit<5>( b ); }
	inline int ATo10Bit( ) const { return ConvertTo10Bit<1>( a ); }
	inline void RFrom10Bit( int r10 ) { r = ConvertFrom10Bit<5>( r10 ); }
	inline void GFrom10Bit( int g10 ) { g = ConvertFrom10Bit<5>( g10 ); }
	inline void BFrom10Bit( int b10 ) { b = ConvertFrom10Bit<5>( b10 ); }
	inline void AFrom10Bit( int a10 ) { a = ConvertFrom10Bit<1>( a10 ); }
};

struct BGRA4444_t
{
	uint16 b : 4;		// order of names changes
	uint16 g : 4;		//  byte order of output to 32 bit
	uint16 r : 4;
	uint16 a : 4;
	inline BGRA4444_t& operator=( const BGRA8888_t& in )
	{
		r = in.r >> 4;
		g = in.g >> 4;
		b = in.b >> 4;
		a = in.a >> 4;
		return *this;
	}
	inline int RTo10Bit( ) const { return ConvertTo10Bit<4>( r ); }
	inline int GTo10Bit( ) const { return ConvertTo10Bit<4>( g ); }
	inline int BTo10Bit( ) const { return ConvertTo10Bit<4>( b ); }
	inline int ATo10Bit( ) const { return ConvertTo10Bit<4>( a ); }
	inline void RFrom10Bit( int r10 ) { r = ConvertFrom10Bit<4>( r10 ); }
	inline void GFrom10Bit( int g10 ) { g = ConvertFrom10Bit<4>( g10 ); }
	inline void BFrom10Bit( int b10 ) { b = ConvertFrom10Bit<4>( b10 ); }
	inline void AFrom10Bit( int a10 ) { a = ConvertFrom10Bit<4>( a10 ); }
};

struct RGBX5551_t
{
	uint16 r : 5;
	uint16 g : 5;
	uint16 b : 5;
	uint16 x : 1;
	inline RGBX5551_t& operator=( const BGRA8888_t& in )
	{
		r = in.r >> 3;
		g = in.g >> 3;
		b = in.b >> 3;
		return *this;
	}
	inline int RTo10Bit( ) const { return ConvertTo10Bit<5>( r ); }
	inline int GTo10Bit( ) const { return ConvertTo10Bit<5>( g ); }
	inline int BTo10Bit( ) const { return ConvertTo10Bit<5>( b ); }
	inline void RFrom10Bit( int r10 ) { r = ConvertFrom10Bit<5>( r10 ); }
	inline void GFrom10Bit( int g10 ) { g = ConvertFrom10Bit<5>( g10 ); }
	inline void BFrom10Bit( int b10 ) { b = ConvertFrom10Bit<5>( b10 ); }
};

struct UV88_t
{
	int8 u;
	int8 v;
};

struct UVWQ8888_t
{
	int8 u;
	int8 v;
	int8 w;
	int8 q;
};

struct UVLX8888_t
{
	int8 u;
	int8 v;
	uint8 l;
	uint8 x;
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
	const char* m_pName;
	uint8 m_nNumBytes;
	uint8 m_nNumRedBits;
	uint8 m_nNumGreenBits;
	uint8 m_nNumBlueBits;
	uint8 m_nNumAlphaBits;
	uint8 m_nNumDepthBits;
	uint8 m_nNumStencilBits;
	bool m_bIsCompressed:1;
	bool m_bIsFloat:1;
	bool m_bIsDepthFormat : 1;
};


//-----------------------------------------------------------------------------
// Computes nice filters for a compression ratio
//-----------------------------------------------------------------------------
struct KernelInfo_t
{
	float *m_pKernel;
	float *m_pInvKernel;
	int m_nWidth;
	int m_nHeight;
	int m_nDepth;
	int m_nDiameter;
};


//-----------------------------------------------------------------------------
// Various methods related to pixelmaps and color formats
//-----------------------------------------------------------------------------
namespace ImageLoader
{
	bool GetInfo( const char *fileName, int *width, int *height, enum ImageFormat *imageFormat, float *sourceGamma );

	// Generates a nice filter kernel
	void ComputeNiceFilterKernel( float flWRatio, float flHRatio, float flDRatio, KernelInfo_t *pKernel );
	void CleanupNiceFilterKernel( KernelInfo_t *pKernel );

	// adjustedheight received the height adjusted for compression (/4 for dxt)
	int  GetMemRequired( int width, int height, int depth, ImageFormat imageFormat, bool mipmap, int *pAdjustedHeightOut = NULL );
	int  GetMemRequired( int width, int height, int depth, int nMipmapCount, ImageFormat imageFormat, int *pAdjustedHeightOut = NULL );

	// This version is for mipmaps which are stored biggest level to smallest level in memory
	int  GetMipMapLevelByteOffset( int width, int height, ImageFormat imageFormat, int skipMipLevels, int nDepth = 1 );

	// This version is for mipmaps which are stored smallest level to largest level in memory
	int  GetMipMapLevelByteOffsetReverse( int nWidth, int nHeight, int nDepth, int nTotalMipCount, ImageFormat imageFormat, int nMipLevel );

	void GetMipMapLevelDimensions( int *width, int *height, int skipMipLevels );
	void GetMipMapLevelDimensions( int &nWidth, int &nHeight, int &nDepth, int nMipLevel );
	int  GetNumMipMapLevels( int width, int height, int depth = 1 );
	bool Load( unsigned char *imageData, const char *fileName, int width, int height, ImageFormat imageFormat, float targetGamma, bool mipmap );
	bool Load( unsigned char *imageData, FILE *fp, int width, int height, 
			   enum ImageFormat imageFormat, float targetGamma, bool mipmap );

	// convert from any image format to any other image format.
	// return false if the conversion cannot be performed.
	// Strides denote the number of bytes per each line, 
	// by default assumes width * # of bytes per pixel
	bool ConvertImageFormat( const unsigned char *src, enum ImageFormat srcImageFormat,
							 unsigned char *dst, enum ImageFormat dstImageFormat, 
							 int width, int height, int srcStride = 0, int dstStride = 0 );

	// must be used in conjunction with ConvertImageFormat() to pre-swap and post-swap
	void PreConvertSwapImageData( unsigned char *pImageData, int nImageSize, ImageFormat imageFormat, int width = 0, int stride = 0 );
	void PostConvertSwapImageData( unsigned char *pImageData, int nImageSize, ImageFormat imageFormat, int width = 0, int stride = 0 );
	void ByteSwapImageData( unsigned char *pImageData, int nImageSize, ImageFormat imageFormat, int width = 0, int stride = 0 );
	bool IsFormatValidForConversion( ImageFormat fmt );

	//-----------------------------------------------------------------------------
	// convert back and forth from D3D format to ImageFormat, regardless of
	// whether it's supported or not
	// FIXME: Move into shaderapidx8/rendersystemdx9
	//-----------------------------------------------------------------------------
	ImageFormat D3DFormatToImageFormat( D3DFORMAT format );
	D3DFORMAT ImageFormatToD3DFormat( ImageFormat format );

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

		ResampleInfo_t() : m_nSrcDepth(1), m_nDestDepth(1), m_flAlphaThreshhold(0.4f), m_flAlphaHiFreqThreshhold(0.4f), m_nFlags(0)
		{
			m_flColorScale[0] = 1.0f, m_flColorScale[1] = 1.0f, m_flColorScale[2] = 1.0f, m_flColorScale[3] = 1.0f;
			m_flColorGoal[0] = 0.0f, m_flColorGoal[1] = 0.0f, m_flColorGoal[2] = 0.0f, m_flColorGoal[3] = 0.0f;
		}

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
		
		float m_flColorScale[4];	// Color scale factors RGBA
		float m_flColorGoal[4];		// Color goal values RGBA		DestColor = ColorGoal + scale * (SrcColor - ColorGoal)
		
		float m_flAlphaThreshhold;
		float m_flAlphaHiFreqThreshhold;
		
		int m_nFlags;
	};

	bool ResampleRGBA8888( const ResampleInfo_t &info );
	bool ResampleRGBA16161616( const ResampleInfo_t &info );
	bool ResampleRGB323232F( const ResampleInfo_t &info );
	bool ResampleRGBA32323232F( const ResampleInfo_t &info );

	void ConvertNormalMapRGBA8888ToDUDVMapUVLX8888( const unsigned char *src, int width, int height, unsigned char *dst_ );
	void ConvertNormalMapRGBA8888ToDUDVMapUVWQ8888( const unsigned char *src, int width, int height, unsigned char *dst_ );
	void ConvertNormalMapRGBA8888ToDUDVMapUV88( const unsigned char *src, int width, int height, unsigned char *dst_ );
	void ConvertNormalMapARGB8888ToDXT5GA( const unsigned char *src, unsigned char *dst, int width, int height );

	void ConvertIA88ImageToNormalMapRGBA8888( const unsigned char *src, int width, 
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
	bool RotateImageLeft( const unsigned char *src, unsigned char *dst, 
						  int widthHeight, ImageFormat imageFormat );
	bool RotateImage180( const unsigned char *src, unsigned char *dst, 
						 int widthHeight, ImageFormat imageFormat );
	bool FlipImageVertically( void *pSrc, void *pDst, int nWidth, int nHeight, ImageFormat imageFormat, int nDstStride = 0 );
	bool FlipImageHorizontally( void *pSrc, void *pDst, int nWidth, int nHeight, ImageFormat imageFormat, int nDstStride = 0 );
	bool SwapAxes( unsigned char *src, 
				   int widthHeight, ImageFormat imageFormat );


	//-----------------------------------------------------------------------------
	// Returns info about each image format
	//-----------------------------------------------------------------------------
	const ImageFormatInfo_t &ImageFormatInfo( ImageFormat fmt );


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
		return ImageFormatInfo(fmt).m_nNumBytes;
	}

	//-----------------------------------------------------------------------------
	// Does the image format support transparency?
	//-----------------------------------------------------------------------------
	inline bool IsTransparent( ImageFormat fmt )
	{
		return ImageFormatInfo(fmt).m_nNumAlphaBits > 0;
	}


	//-----------------------------------------------------------------------------
	// Is the image format compressed?
	//-----------------------------------------------------------------------------
	inline bool IsCompressed( ImageFormat fmt )
	{
		return ImageFormatInfo(fmt).m_bIsCompressed;
	}


	//-----------------------------------------------------------------------------
	// Is the image format compressed?
	//-----------------------------------------------------------------------------
	inline bool IsDepthFormat( ImageFormat fmt )
	{
		return ImageFormatInfo(fmt).m_bIsDepthFormat;
	}


	//-----------------------------------------------------------------------------
	// Is any channel > 8 bits?
	//-----------------------------------------------------------------------------
	inline bool HasChannelLargerThan8Bits( ImageFormat fmt )
	{
		ImageFormatInfo_t info = ImageFormatInfo(fmt);
		return ( info.m_nNumRedBits > 8 || info.m_nNumGreenBits > 8 || info.m_nNumBlueBits > 8 || info.m_nNumAlphaBits > 8 );
	}

	inline bool IsFloatFormat( ImageFormat fmt )
	{
		return ( fmt == IMAGE_FORMAT_RGBA16161616F ) ||
				( fmt == IMAGE_FORMAT_RG1616F ) ||
				( fmt == IMAGE_FORMAT_R32F ) ||
				( fmt == IMAGE_FORMAT_RG3232F ) ||
				( fmt == IMAGE_FORMAT_RGB323232F ) ||
				( fmt == IMAGE_FORMAT_RGBA32323232F );
	}

} // end namespace ImageLoader


#endif // IMAGEFORMAT_H
