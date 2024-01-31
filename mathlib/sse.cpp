//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: SSE Math primitives.
//
//=====================================================================================//

#include <math.h>
#include <float.h>	// Needed for FLT_EPSILON
#include "basetypes.h"
#include <memory.h>
#include "tier0/dbg.h"
#include "mathlib/mathlib.h"
#include "mathlib/vector.h"
#include "sse.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#if defined ( _WIN32 ) && !defined ( _WIN64 )
static const uint32 _sincos_masks[]	  = { (uint32)0x0,  (uint32)~0x0 };
static const uint32 _sincos_inv_masks[] = { (uint32)~0x0, (uint32)0x0 };
#endif

//-----------------------------------------------------------------------------
// Macros and constants required by some of the SSE assembly:
//-----------------------------------------------------------------------------

#ifdef _WIN32
	#define _PS_EXTERN_CONST(Name, Val) \
		const __declspec(align(16)) float _ps_##Name[4] = { Val, Val, Val, Val }

	#define _PS_EXTERN_CONST_TYPE(Name, Type, Val) \
		const __declspec(align(16)) Type _ps_##Name[4] = { Val, Val, Val, Val }; \

	#define _EPI32_CONST(Name, Val) \
		static const __declspec(align(16)) __int32 _epi32_##Name[4] = { Val, Val, Val, Val }

	#define _PS_CONST(Name, Val) \
		static const __declspec(align(16)) float _ps_##Name[4] = { Val, Val, Val, Val }
#elif defined _LINUX || defined __APPLE__
	#define _PS_EXTERN_CONST(Name, Val) \
		const __attribute__((aligned(16))) float _ps_##Name[4] = { Val, Val, Val, Val }

	#define _PS_EXTERN_CONST_TYPE(Name, Type, Val) \
		const __attribute__((aligned(16))) Type _ps_##Name[4] = { Val, Val, Val, Val }; \

	#define _EPI32_CONST(Name, Val) \
		static const __attribute__((aligned(16))) int32 _epi32_##Name[4] = { Val, Val, Val, Val }

	#define _PS_CONST(Name, Val) \
		static const __attribute__((aligned(16))) float _ps_##Name[4] = { Val, Val, Val, Val }
#endif

#if defined ( _WIN32 ) && !defined ( _WIN64 )
_PS_EXTERN_CONST(am_0, 0.0f);
_PS_EXTERN_CONST(am_1, 1.0f);
_PS_EXTERN_CONST(am_m1, -1.0f);
_PS_EXTERN_CONST(am_0p5, 0.5f);
_PS_EXTERN_CONST(am_1p5, 1.5f);
_PS_EXTERN_CONST(am_pi, (float)M_PI);
_PS_EXTERN_CONST(am_pi_o_2, (float)(M_PI / 2.0));
_PS_EXTERN_CONST(am_2_o_pi, (float)(2.0 / M_PI));
_PS_EXTERN_CONST(am_pi_o_4, (float)(M_PI / 4.0));
_PS_EXTERN_CONST(am_4_o_pi, (float)(4.0 / M_PI));
_PS_EXTERN_CONST_TYPE(am_sign_mask, int32, (int32)0x80000000);
_PS_EXTERN_CONST_TYPE(am_inv_sign_mask, int32, ~0x80000000);
_PS_EXTERN_CONST_TYPE(am_min_norm_pos,int32, 0x00800000);
_PS_EXTERN_CONST_TYPE(am_mant_mask, int32, 0x7f800000);
_PS_EXTERN_CONST_TYPE(am_inv_mant_mask, int32, ~0x7f800000);

_EPI32_CONST(1, 1);
_EPI32_CONST(2, 2);

_PS_CONST(sincos_p0, 0.15707963267948963959e1f);
_PS_CONST(sincos_p1, -0.64596409750621907082e0f);
_PS_CONST(sincos_p2, 0.7969262624561800806e-1f);
_PS_CONST(sincos_p3, -0.468175413106023168e-2f);
#endif

#ifdef PFN_VECTORMA
void  __cdecl _SSE_VectorMA( const float *start, float scale, const float *direction, float *dest );
#endif

//-----------------------------------------------------------------------------
// SSE implementations of optimized routines:
//-----------------------------------------------------------------------------
float _SSE_Sqrt(float x)
{
#if defined( _WIN64 )
	return std::sqrt(x);
#else
	Assert( s_bMathlibInitialized );
	float	root = 0.f;
#ifdef _WIN32
	_asm
	{
		sqrtss		xmm0, x
		movss		root, xmm0
	}
#elif defined _LINUX || defined __APPLE__
	__asm__ __volatile__(
		"movss %1,%%xmm2\n"
		"sqrtss %%xmm2,%%xmm1\n"
		"movss %%xmm1,%0"
       	: "=m" (root)
		: "m" (x)
	);
#endif
	return root;
#endif // _WIN64
}

// Single iteration NewtonRaphson reciprocal square root:
// 0.5 * rsqrtps * (3 - x * rsqrtps(x) * rsqrtps(x)) 	
// Very low error, and fine to use in place of 1.f / sqrtf(x).	
#if 0
float _SSE_RSqrtAccurate(float x)
{
	Assert( s_bMathlibInitialized );

	float rroot;
	_asm
	{
		rsqrtss	xmm0, x
		movss	rroot, xmm0
	}

	return (0.5f * rroot) * (3.f - (x * rroot) * rroot);
}
#else
// Intel / Kipps SSE RSqrt.  Significantly faster than above.
float _SSE_RSqrtAccurate(float a)
{
#if defined( _WIN64 )
	return std::sqrt(a);
#else
	float x;
	float half = 0.5f;
	float three = 3.f;

#ifdef _WIN32
	__asm
	{
		movss   xmm3, a;
		movss   xmm1, half;
		movss   xmm2, three;
		rsqrtss xmm0, xmm3;

		mulss   xmm3, xmm0;
		mulss   xmm1, xmm0;
		mulss   xmm3, xmm0;
		subss   xmm2, xmm3;
		mulss   xmm1, xmm2;

		movss   x,    xmm1;
	}
#elif defined _LINUX || defined __APPLE__
	__asm__ __volatile__(
		"movss   %1, %%xmm3 \n\t"
        "movss   %2, %%xmm1 \n\t"
        "movss   %3, %%xmm2 \n\t"
        "rsqrtss %%xmm3, %%xmm0 \n\t"
        "mulss   %%xmm0, %%xmm3 \n\t"
        "mulss   %%xmm0, %%xmm1 \n\t"
        "mulss   %%xmm0, %%xmm3 \n\t"
        "subss   %%xmm3, %%xmm2 \n\t"
        "mulss   %%xmm2, %%xmm1 \n\t"
        "movss   %%xmm1, %0 \n\t"
		: "=m" (x)
		: "m" (a), "m" (half), "m" (three)
);
#else
	#error "Not Implemented"
#endif

	return x;
#endif // _WIN64
}
#endif

// Simple SSE rsqrt.  Usually accurate to around 6 (relative) decimal places 
// or so, so ok for closed transforms.  (ie, computing lighting normals)
float _SSE_RSqrtFast(float x)
{
#if defined( _WIN64 )
	return std::sqrt(x);
#else
	Assert( s_bMathlibInitialized );

	float rroot = 0.0f;
#ifdef _WIN32
	_asm
	{
		rsqrtss	xmm0, x
		movss	rroot, xmm0
	}
#elif defined _LINUX || defined __APPLE__
	 __asm__ __volatile__(
		"rsqrtss %1, %%xmm0 \n\t"
		"movss %%xmm0, %0 \n\t"
		: "=m" (x)
		: "m" (rroot)
		: "%xmm0"
	);
#else
#error
#endif

	return rroot;
#endif // _WIN64
}

float FASTCALL _SSE_VectorNormalize (Vector& vec)
{
#if defined( _WIN64 )
	float l = std::sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
	vec.x /= l;
	vec.y /= l;
	vec.z /= l;
	return l;
#else
	Assert( s_bMathlibInitialized );

	// NOTE: This is necessary to prevent an memory overwrite...
	// sice vec only has 3 floats, we can't "movaps" directly into it.
#ifdef _WIN32
	__declspec(align(16)) float result[4];
#elif defined _LINUX || defined __APPLE__
	__attribute__((aligned(16))) float result[4];
#endif

	float *v = &vec[0];

	float	radius = 0.f;
	// Blah, get rid of these comparisons ... in reality, if you have all 3 as zero, it shouldn't 
	// be much of a performance win, considering you will very likely miss 3 branch predicts in a row.
	if ( v[0] || v[1] || v[2] )
	{
#ifdef _WIN32
	float *r = &result[0];
	_asm
		{
			mov			eax, v
			mov			edx, r
#ifdef ALIGNED_VECTOR
			movaps		xmm4, [eax]			// r4 = vx, vy, vz, X
			movaps		xmm1, xmm4			// r1 = r4
#else
			movups		xmm4, [eax]			// r4 = vx, vy, vz, X
			movaps		xmm1, xmm4			// r1 = r4
#endif
			mulps		xmm1, xmm4			// r1 = vx * vx, vy * vy, vz * vz, X
			movhlps		xmm3, xmm1			// r3 = vz * vz, X, X, X
			movaps		xmm2, xmm1			// r2 = r1
			shufps		xmm2, xmm2, 1		// r2 = vy * vy, X, X, X
			addss		xmm1, xmm2			// r1 = (vx * vx) + (vy * vy), X, X, X
			addss		xmm1, xmm3			// r1 = (vx * vx) + (vy * vy) + (vz * vz), X, X, X
			sqrtss		xmm1, xmm1			// r1 = sqrt((vx * vx) + (vy * vy) + (vz * vz)), X, X, X
			movss		radius, xmm1		// radius = sqrt((vx * vx) + (vy * vy) + (vz * vz))
			rcpss		xmm1, xmm1			// r1 = 1/radius, X, X, X
			shufps		xmm1, xmm1, 0		// r1 = 1/radius, 1/radius, 1/radius, X
			mulps		xmm4, xmm1			// r4 = vx * 1/radius, vy * 1/radius, vz * 1/radius, X
			movaps		[edx], xmm4			// v = vx * 1/radius, vy * 1/radius, vz * 1/radius, X
		}
#elif defined _LINUX || defined __APPLE__
		__asm__ __volatile__(
#ifdef ALIGNED_VECTOR
            "movaps          %2, %%xmm4 \n\t"
            "movaps          %%xmm4, %%xmm1 \n\t"
#else
            "movups          %2, %%xmm4 \n\t"
            "movaps          %%xmm4, %%xmm1 \n\t"
#endif
            "mulps           %%xmm4, %%xmm1 \n\t"
            "movhlps         %%xmm1, %%xmm3 \n\t"
            "movaps          %%xmm1, %%xmm2 \n\t"
            "shufps          $1, %%xmm2, %%xmm2 \n\t"
            "addss           %%xmm2, %%xmm1 \n\t"
            "addss           %%xmm3, %%xmm1 \n\t"
            "sqrtss          %%xmm1, %%xmm1 \n\t"
            "movss           %%xmm1, %0 \n\t"
            "rcpss           %%xmm1, %%xmm1 \n\t"
            "shufps          $0, %%xmm1, %%xmm1 \n\t"
            "mulps           %%xmm1, %%xmm4 \n\t"
            "movaps          %%xmm4, %1 \n\t"
            : "=m" (radius), "=m" (result)
            : "m" (*v)
 		);
#else
	#error "Not Implemented"
#endif
		vec.x = result[0];
		vec.y = result[1];
		vec.z = result[2];

	}

	return radius;
#endif // _WIN64
}

void FASTCALL _SSE_VectorNormalizeFast (Vector& vec)
{
	float ool = _SSE_RSqrtAccurate( FLT_EPSILON + vec.x * vec.x + vec.y * vec.y + vec.z * vec.z );

	vec.x *= ool;
	vec.y *= ool;
	vec.z *= ool;
}

float _SSE_InvRSquared(const float* v)
{
#if defined( _WIN64 )
	float	r2 = DotProduct(v, v);
	return r2 < 1.f ? 1.f : 1/r2;
#else
	float	inv_r2 = 1.f;
#ifdef _WIN32
	_asm { // Intel SSE only routine
		mov			eax, v
		movss		xmm5, inv_r2		// x5 = 1.0, 0, 0, 0
#ifdef ALIGNED_VECTOR
		movaps		xmm4, [eax]			// x4 = vx, vy, vz, X
#else
		movups		xmm4, [eax]			// x4 = vx, vy, vz, X
#endif
		movaps		xmm1, xmm4			// x1 = x4
		mulps		xmm1, xmm4			// x1 = vx * vx, vy * vy, vz * vz, X
		movhlps		xmm3, xmm1			// x3 = vz * vz, X, X, X
		movaps		xmm2, xmm1			// x2 = x1
		shufps		xmm2, xmm2, 1		// x2 = vy * vy, X, X, X
		addss		xmm1, xmm2			// x1 = (vx * vx) + (vy * vy), X, X, X
		addss		xmm1, xmm3			// x1 = (vx * vx) + (vy * vy) + (vz * vz), X, X, X
		maxss		xmm1, xmm5			// x1 = MAX( 1.0, x1 )
		rcpss		xmm0, xmm1			// x0 = 1 / MAX( 1.0, x1 )
		movss		inv_r2, xmm0		// inv_r2 = x0
	}
#elif defined _LINUX || defined __APPLE__
		__asm__ __volatile__(
#ifdef ALIGNED_VECTOR
		"movaps          %1, %%xmm4 \n\t"
#else
		"movups          %1, %%xmm4 \n\t"
#endif
        "movaps          %%xmm4, %%xmm1 \n\t"
        "mulps           %%xmm4, %%xmm1 \n\t"
		"movhlps         %%xmm1, %%xmm3 \n\t"
		"movaps          %%xmm1, %%xmm2 \n\t"
        "shufps          $1, %%xmm2, %%xmm2 \n\t"
        "addss           %%xmm2, %%xmm1 \n\t"
        "addss           %%xmm3, %%xmm1 \n\t"
		"maxss           %%xmm5, %%xmm1 \n\t"
        "rcpss           %%xmm1, %%xmm0 \n\t"
		"movss           %%xmm0, %0 \n\t" 
        : "=m" (inv_r2)
        : "m" (*v), "m" (inv_r2)
 		);
#else
	#error "Not Implemented"
#endif

	return inv_r2;
#endif // _WIN64
}

void _SSE_SinCos(float x, float* s, float* c)
{
#if defined( _WIN64 )
	*s = std::sin(x);
	*c = std::cos(x);
#elif defined( _WIN32 )
	float t4, t8, t12;

	__asm
	{
		movss	xmm0, x
		movss	t12, xmm0
		movss	xmm1, _ps_am_inv_sign_mask
		mov		eax, t12
		mulss	xmm0, _ps_am_2_o_pi
		andps	xmm0, xmm1
		and		eax, 0x80000000

		cvttss2si	edx, xmm0
		mov		ecx, edx
		mov		t12, esi
		mov		esi, edx
		add		edx, 0x1	
		shl		ecx, (31 - 1)
		shl		edx, (31 - 1)

		movss	xmm4, _ps_am_1
		cvtsi2ss	xmm3, esi
		mov		t8, eax
		and		esi, 0x1

		subss	xmm0, xmm3
		movss	xmm3, _sincos_inv_masks[esi * 4]
		minss	xmm0, xmm4

		subss	xmm4, xmm0

		movss	xmm6, xmm4
		andps	xmm4, xmm3
		and		ecx, 0x80000000
		movss	xmm2, xmm3
		andnps	xmm3, xmm0
		and		edx, 0x80000000
		movss	xmm7, t8
		andps	xmm0, xmm2
		mov		t8, ecx
		mov		t4, edx
		orps	xmm4, xmm3

		mov		eax, s     //mov eax, [esp + 4 + 16]
		mov		edx, c //mov edx, [esp + 4 + 16 + 4]

		andnps	xmm2, xmm6
		orps	xmm0, xmm2

		movss	xmm2, t8
		movss	xmm1, xmm0
		movss	xmm5, xmm4
		xorps	xmm7, xmm2
		movss	xmm3, _ps_sincos_p3
		mulss	xmm0, xmm0
		mulss	xmm4, xmm4
		movss	xmm2, xmm0
		movss	xmm6, xmm4
		orps	xmm1, xmm7
		movss	xmm7, _ps_sincos_p2
		mulss	xmm0, xmm3
		mulss	xmm4, xmm3
		movss	xmm3, _ps_sincos_p1
		addss	xmm0, xmm7
		addss	xmm4, xmm7
		movss	xmm7, _ps_sincos_p0
		mulss	xmm0, xmm2
		mulss	xmm4, xmm6
		addss	xmm0, xmm3
		addss	xmm4, xmm3
		movss	xmm3, t4
		mulss	xmm0, xmm2
		mulss	xmm4, xmm6
		orps	xmm5, xmm3
		mov		esi, t12
		addss	xmm0, xmm7
		addss	xmm4, xmm7
		mulss	xmm0, xmm1
		mulss	xmm4, xmm5

		// use full stores since caller might reload with full loads
		movss	[eax], xmm0
		movss	[edx], xmm4
	}
#elif defined _LINUX || defined __APPLE__
//	#warning "_SSE_sincos NOT implemented!"
#else
	#error "Not Implemented"
#endif
}

float _SSE_cos( float x )
{
#if defined ( _WIN64 )
	return std::cos(x);
#elif defined( _WIN32 )
	float temp;
	__asm
	{
		movss	xmm0, x
		movss	xmm1, _ps_am_inv_sign_mask
		andps	xmm0, xmm1
		addss	xmm0, _ps_am_pi_o_2
		mulss	xmm0, _ps_am_2_o_pi

		cvttss2si	ecx, xmm0
		movss	xmm5, _ps_am_1
		mov		edx, ecx
		shl		edx, (31 - 1)
		cvtsi2ss	xmm1, ecx
		and		edx, 0x80000000
		and		ecx, 0x1

		subss	xmm0, xmm1
		movss	xmm6, _sincos_masks[ecx * 4]
		minss	xmm0, xmm5

		movss	xmm1, _ps_sincos_p3
		subss	xmm5, xmm0

		andps	xmm5, xmm6
		movss	xmm7, _ps_sincos_p2
		andnps	xmm6, xmm0
		mov		temp, edx
		orps	xmm5, xmm6
		movss	xmm0, xmm5

		mulss	xmm5, xmm5
		movss	xmm4, _ps_sincos_p1
		movss	xmm2, xmm5
		mulss	xmm5, xmm1
		movss	xmm1, _ps_sincos_p0
		addss	xmm5, xmm7
		mulss	xmm5, xmm2
		movss	xmm3, temp
		addss	xmm5, xmm4
		mulss	xmm5, xmm2
		orps	xmm0, xmm3
		addss	xmm5, xmm1
		mulss	xmm0, xmm5
		
		movss   x,    xmm0

	}
#elif defined _LINUX || defined __APPLE__
//	#warning "_SSE_cos NOT implemented!"
#else
	#error "Not Implemented"
#endif

	return x;
}

//-----------------------------------------------------------------------------
// SSE2 implementations of optimized routines:
//-----------------------------------------------------------------------------
void _SSE2_SinCos(float x, float* s, float* c)  // any x
{
#if defined( _WIN64 )
	*s = std::sin(x);
	*c = std::cos(x);
#elif defined( _WIN32 )
	__asm
	{
		movss	xmm0, x
		movaps	xmm7, xmm0
		movss	xmm1, _ps_am_inv_sign_mask
		movss	xmm2, _ps_am_sign_mask
		movss	xmm3, _ps_am_2_o_pi
		andps	xmm0, xmm1
		andps	xmm7, xmm2
		mulss	xmm0, xmm3

		pxor	xmm3, xmm3
		movd	xmm5, _epi32_1
		movss	xmm4, _ps_am_1

		cvttps2dq	xmm2, xmm0
		pand	xmm5, xmm2
		movd	xmm1, _epi32_2
		pcmpeqd	xmm5, xmm3
		movd	xmm3, _epi32_1
		cvtdq2ps	xmm6, xmm2
		paddd	xmm3, xmm2
		pand	xmm2, xmm1
		pand	xmm3, xmm1
		subss	xmm0, xmm6
		pslld	xmm2, (31 - 1)
		minss	xmm0, xmm4

		mov		eax, s     // mov eax, [esp + 4 + 16]
		mov		edx, c	   // mov edx, [esp + 4 + 16 + 4]

		subss	xmm4, xmm0
		pslld	xmm3, (31 - 1)

		movaps	xmm6, xmm4
		xorps	xmm2, xmm7
		movaps	xmm7, xmm5
		andps	xmm6, xmm7
		andnps	xmm7, xmm0
		andps	xmm0, xmm5
		andnps	xmm5, xmm4
		movss	xmm4, _ps_sincos_p3
		orps	xmm6, xmm7
		orps	xmm0, xmm5
		movss	xmm5, _ps_sincos_p2

		movaps	xmm1, xmm0
		movaps	xmm7, xmm6
		mulss	xmm0, xmm0
		mulss	xmm6, xmm6
		orps	xmm1, xmm2
		orps	xmm7, xmm3
		movaps	xmm2, xmm0
		movaps	xmm3, xmm6
		mulss	xmm0, xmm4
		mulss	xmm6, xmm4
		movss	xmm4, _ps_sincos_p1
		addss	xmm0, xmm5
		addss	xmm6, xmm5
		movss	xmm5, _ps_sincos_p0
		mulss	xmm0, xmm2
		mulss	xmm6, xmm3
		addss	xmm0, xmm4
		addss	xmm6, xmm4
		mulss	xmm0, xmm2
		mulss	xmm6, xmm3
		addss	xmm0, xmm5
		addss	xmm6, xmm5
		mulss	xmm0, xmm1
		mulss	xmm6, xmm7

		// use full stores since caller might reload with full loads
		movss	[eax], xmm0
		movss	[edx], xmm6
	}
#elif defined _LINUX || defined __APPLE__
//	#warning "_SSE2_SinCos NOT implemented!"
#else
	#error "Not Implemented"
#endif
}

float _SSE2_cos(float x)  
{
#if defined ( _WIN64 )
	return std::cos(x);
#elif defined( _WIN32 )
	__asm
	{
		movss	xmm0, x
		movss	xmm1, _ps_am_inv_sign_mask
		movss	xmm2, _ps_am_pi_o_2
		movss	xmm3, _ps_am_2_o_pi
		andps	xmm0, xmm1
		addss	xmm0, xmm2
		mulss	xmm0, xmm3

		pxor	xmm3, xmm3
		movd	xmm5, _epi32_1
		movss	xmm4, _ps_am_1
		cvttps2dq	xmm2, xmm0
		pand	xmm5, xmm2
		movd	xmm1, _epi32_2
		pcmpeqd	xmm5, xmm3
		cvtdq2ps	xmm6, xmm2
		pand	xmm2, xmm1
		pslld	xmm2, (31 - 1)

		subss	xmm0, xmm6
		movss	xmm3, _ps_sincos_p3
		minss	xmm0, xmm4
		subss	xmm4, xmm0
		andps	xmm0, xmm5
		andnps	xmm5, xmm4
		orps	xmm0, xmm5

		movaps	xmm1, xmm0
		movss	xmm4, _ps_sincos_p2
		mulss	xmm0, xmm0
		movss	xmm5, _ps_sincos_p1
		orps	xmm1, xmm2
		movaps	xmm7, xmm0
		mulss	xmm0, xmm3
		movss	xmm6, _ps_sincos_p0
		addss	xmm0, xmm4
		mulss	xmm0, xmm7
		addss	xmm0, xmm5
		mulss	xmm0, xmm7
		addss	xmm0, xmm6
		mulss	xmm0, xmm1
		movss   x,    xmm0
	}
#elif defined _LINUX || defined __APPLE__
//	#warning "_SSE2_cos NOT implemented!"
#else
	#error "Not Implemented"
#endif

	return x;
}

// SSE Version of VectorTransform
void VectorTransformSSE(const float *in1, const matrix3x4_t& in2, float *out1)
{
	Assert( s_bMathlibInitialized );
	Assert( in1 != out1 );
#if defined ( _WIN64 )
	out1[0] = DotProduct(in1, in2[0]) + in2[0][3];
	out1[1] = DotProduct(in1, in2[1]) + in2[1][3];
	out1[2] = DotProduct(in1, in2[2]) + in2[2][3];
#elif defined( _WIN32 )
	__asm
	{
		mov eax, in1;
		mov ecx, in2;
		mov edx, out1;

		movss xmm0, [eax];
		mulss xmm0, [ecx];
		movss xmm1, [eax+4];
		mulss xmm1, [ecx+4];
		movss xmm2, [eax+8];
		mulss xmm2, [ecx+8];
		addss xmm0, xmm1;
		addss xmm0, xmm2;
		addss xmm0, [ecx+12]
 		movss [edx], xmm0;
		add ecx, 16;

		movss xmm0, [eax];
		mulss xmm0, [ecx];
		movss xmm1, [eax+4];
		mulss xmm1, [ecx+4];
		movss xmm2, [eax+8];
		mulss xmm2, [ecx+8];
		addss xmm0, xmm1;
		addss xmm0, xmm2;
		addss xmm0, [ecx+12]
		movss [edx+4], xmm0;
		add ecx, 16;

		movss xmm0, [eax];
		mulss xmm0, [ecx];
		movss xmm1, [eax+4];
		mulss xmm1, [ecx+4];
		movss xmm2, [eax+8];
		mulss xmm2, [ecx+8];
		addss xmm0, xmm1;
		addss xmm0, xmm2;
		addss xmm0, [ecx+12]
		movss [edx+8], xmm0;
	}
#elif defined _LINUX || defined __APPLE__
//	#warning "VectorTransformSSE C implementation only"
		out1[0] = DotProduct(in1, in2[0]) + in2[0][3];
		out1[1] = DotProduct(in1, in2[1]) + in2[1][3];
		out1[2] = DotProduct(in1, in2[2]) + in2[2][3];
#else
	#error "Not Implemented"
#endif
}

void VectorRotateSSE( const float *in1, const matrix3x4_t& in2, float *out1 )
{
	Assert( s_bMathlibInitialized );
	Assert( in1 != out1 );
#if defined ( _WIN64 )
	out1[0] = DotProduct( in1, in2[0] );
	out1[1] = DotProduct( in1, in2[1] );
	out1[2] = DotProduct( in1, in2[2] );
#elif defined( _WIN32 )
	__asm
	{
		mov eax, in1;
		mov ecx, in2;
		mov edx, out1;

		movss xmm0, [eax];
		mulss xmm0, [ecx];
		movss xmm1, [eax+4];
		mulss xmm1, [ecx+4];
		movss xmm2, [eax+8];
		mulss xmm2, [ecx+8];
		addss xmm0, xmm1;
		addss xmm0, xmm2;
 		movss [edx], xmm0;
		add ecx, 16;

		movss xmm0, [eax];
		mulss xmm0, [ecx];
		movss xmm1, [eax+4];
		mulss xmm1, [ecx+4];
		movss xmm2, [eax+8];
		mulss xmm2, [ecx+8];
		addss xmm0, xmm1;
		addss xmm0, xmm2;
		movss [edx+4], xmm0;
		add ecx, 16;

		movss xmm0, [eax];
		mulss xmm0, [ecx];
		movss xmm1, [eax+4];
		mulss xmm1, [ecx+4];
		movss xmm2, [eax+8];
		mulss xmm2, [ecx+8];
		addss xmm0, xmm1;
		addss xmm0, xmm2;
		movss [edx+8], xmm0;
	}
#elif defined _LINUX || defined __APPLE__
//	#warning "VectorRotateSSE C implementation only"
		out1[0] = DotProduct( in1, in2[0] );
		out1[1] = DotProduct( in1, in2[1] );
		out1[2] = DotProduct( in1, in2[2] );
#else
	#error "Not Implemented"
#endif
}

#if defined( _WIN32 ) && !defined( _WIN64 )
void _declspec(naked) _SSE_VectorMA( const float *start, float scale, const float *direction, float *dest )
{
	// FIXME: This don't work!! It will overwrite memory in the write to dest
	Assert(0);

	Assert( s_bMathlibInitialized );
	_asm {  // Intel SSE only routine
		mov	eax, DWORD PTR [esp+0x04]	; *start, s0..s2
		mov ecx, DWORD PTR [esp+0x0c]	; *direction, d0..d2
		mov edx, DWORD PTR [esp+0x10]	; *dest
		movss	xmm2, [esp+0x08]		; x2 = scale, 0, 0, 0
#ifdef ALIGNED_VECTOR
		movaps	xmm3, [ecx]				; x3 = dir0,dir1,dir2,X
		pshufd	xmm2, xmm2, 0			; x2 = scale, scale, scale, scale
		movaps	xmm1, [eax]				; x1 = start1, start2, start3, X
		mulps	xmm3, xmm2				; x3 *= x2
		addps	xmm3, xmm1				; x3 += x1
		movaps	[edx], xmm3				; *dest = x3
#else
		movups	xmm3, [ecx]				; x3 = dir0,dir1,dir2,X
		pshufd	xmm2, xmm2, 0			; x2 = scale, scale, scale, scale
		movups	xmm1, [eax]				; x1 = start1, start2, start3, X
		mulps	xmm3, xmm2				; x3 *= x2
		addps	xmm3, xmm1				; x3 += x1
		movups	[edx], xmm3				; *dest = x3
#endif
	}
}
#endif

#if defined( _WIN32 ) && !defined( _WIN64 )
#ifdef PFN_VECTORMA
void _declspec(naked) __cdecl _SSE_VectorMA( const Vector &start, float scale, const Vector &direction, Vector &dest )
{
	// FIXME: This don't work!! It will overwrite memory in the write to dest
	Assert(0);

	Assert( s_bMathlibInitialized );
	_asm 
	{  
		// Intel SSE only routine
		mov	eax, DWORD PTR [esp+0x04]	; *start, s0..s2
		mov ecx, DWORD PTR [esp+0x0c]	; *direction, d0..d2
		mov edx, DWORD PTR [esp+0x10]	; *dest
		movss	xmm2, [esp+0x08]		; x2 = scale, 0, 0, 0
#ifdef ALIGNED_VECTOR
		movaps	xmm3, [ecx]				; x3 = dir0,dir1,dir2,X
		pshufd	xmm2, xmm2, 0			; x2 = scale, scale, scale, scale
		movaps	xmm1, [eax]				; x1 = start1, start2, start3, X
		mulps	xmm3, xmm2				; x3 *= x2
		addps	xmm3, xmm1				; x3 += x1
		movaps	[edx], xmm3				; *dest = x3
#else
		movups	xmm3, [ecx]				; x3 = dir0,dir1,dir2,X
		pshufd	xmm2, xmm2, 0			; x2 = scale, scale, scale, scale
		movups	xmm1, [eax]				; x1 = start1, start2, start3, X
		mulps	xmm3, xmm2				; x3 *= x2
		addps	xmm3, xmm1				; x3 += x1
		movups	[edx], xmm3				; *dest = x3
#endif
	}
}
float (__cdecl *pfVectorMA)(Vector& v) = _VectorMA;
#endif
#endif


// SSE DotProduct -- it's a smidgen faster than the asm DotProduct...
//   Should be validated too!  :)
//   NJS: (Nov 1 2002) -NOT- faster.  may time a couple cycles faster in a single function like 
//   this, but when inlined, and instruction scheduled, the C version is faster.  
//   Verified this via VTune
/*
vec_t DotProduct (const vec_t *a, const vec_t *c)
{
	vec_t temp;

	__asm
	{
		mov eax, a;
		mov ecx, c;
		mov edx, DWORD PTR [temp]
		movss xmm0, [eax];
		mulss xmm0, [ecx];
		movss xmm1, [eax+4];
		mulss xmm1, [ecx+4];
		movss xmm2, [eax+8];
		mulss xmm2, [ecx+8];
		addss xmm0, xmm1;
		addss xmm0, xmm2;
		movss [edx], xmm0;
		fld DWORD PTR [edx];
		ret
	}
}
*/