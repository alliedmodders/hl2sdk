//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#ifndef _MATH_PFNS_H_
#define _MATH_PFNS_H_

#include <limits>

#if defined( _X360 )
#include <xboxmath.h>
#endif

#if !defined( _X360 )

#include <xmmintrin.h>

// These globals are initialized by mathlib and redirected based on available fpu features

// The following are not declared as macros because they are often used in limiting situations,
// and sometimes the compiler simply refuses to inline them for some reason
FORCEINLINE float FastSqrt( float x )
{
	__m128 root = _mm_sqrt_ss( _mm_load_ss( &x ) );
	return *( reinterpret_cast<float *>( &root ) );
}

FORCEINLINE float FastRSqrtFast( float x )
{
	// use intrinsics
	__m128 rroot = _mm_rsqrt_ss( _mm_load_ss( &x ) );
	return *( reinterpret_cast<float *>( &rroot ) );
}
// Single iteration NewtonRaphson reciprocal square root:
// 0.5 * rsqrtps * (3 - x * rsqrtps(x) * rsqrtps(x)) 	
// Very low error, and fine to use in place of 1.f / sqrtf(x).	
FORCEINLINE float FastRSqrt( float x )
{
	float rroot = FastRSqrtFast( x );
	return (0.5f * rroot) * (3.f - (x * rroot) * rroot);
}

void FastSinCos( float x, float* s, float* c );  // any x
float FastCos( float x );



inline float FastRecip(float x) {return 1.0f / x;}
// Simple SSE rsqrt.  Usually accurate to around 6 (relative) decimal places 
// or so, so ok for closed transforms.  (ie, computing lighting normals)
inline float FastSqrtEst(float x) { return FastRSqrtFast(x) * x; }


#endif // !_X360

#if defined( _X360 )

FORCEINLINE float _VMX_Sqrt( float x )
{
	return __fsqrts( x );
}

FORCEINLINE float _VMX_RSqrt( float x )
{
	float rroot = __frsqrte( x );

	// Single iteration NewtonRaphson on reciprocal square root estimate
	return (0.5f * rroot) * (3.0f - (x * rroot) * rroot);
}

FORCEINLINE float _VMX_RSqrtFast( float x )
{
	return __frsqrte( x );
}

FORCEINLINE void _VMX_SinCos( float a, float *pS, float *pC )
{
	XMScalarSinCos( pS, pC, a );
}

FORCEINLINE float _VMX_Cos( float a )
{
	return XMScalarCos( a );
}

// the 360 has fixed hw and calls directly
#define FastSqrt(x)			_VMX_Sqrt(x)
#define	FastRSqrt(x)		_VMX_RSqrt(x)
#define FastRSqrtFast(x)	_VMX_RSqrtFast(x)
#define FastSinCos(x,s,c)	_VMX_SinCos(x,s,c)
#define FastCos(x)			_VMX_Cos(x)

inline float FastRecip(float x) {return __fres(x);}
inline float FastSqrtEst(float x) { return __frsqrte(x) * x; }

#endif // _X360

// if x is infinite, return FLT_MAX
inline float FastClampInfinity( float x )
{
#ifdef _X360
	return fsel( std::numeric_limits<float>::infinity() - x, x, FLT_MAX );
#else
	return ( x > FLT_MAX ? FLT_MAX : x );
#endif
}

#endif // _MATH_PFNS_H_
