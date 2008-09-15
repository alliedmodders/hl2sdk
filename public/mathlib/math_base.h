//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef MATH_BASE_H
#define MATH_BASE_H

#include <math.h>
#include "basetypes.h"
#include "vector.h"
#include "vector2d.h"
#include "tier0/dbg.h"
#include "minmax.h"

#ifdef _WIN32
#define FORCEINLINE_MATH FORCEINLINE
#else
#define FORCEINLINE_MATH extern __inline__ FORCEINLINE
#endif

// plane_t structure
// !!! if this is changed, it must be changed in asm code too !!!
// FIXME: does the asm code even exist anymore?
// FIXME: this should move to a different file
struct cplane_t
{
	Vector	normal;
	float	dist;
	byte	type;			// for fast side tests
	byte	signbits;		// signx + (signy<<1) + (signz<<1)
	byte	pad[2];

#ifdef VECTOR_NO_SLOW_OPERATIONS
	cplane_t() {}

private:
	// No copy constructors allowed if we're in optimal mode
	cplane_t(const cplane_t& vOther);
#endif
};

// structure offset for asm code
#define CPLANE_NORMAL_X			0
#define CPLANE_NORMAL_Y			4
#define CPLANE_NORMAL_Z			8
#define CPLANE_DIST				12
#define CPLANE_TYPE				16
#define CPLANE_SIGNBITS			17
#define CPLANE_PAD0				18
#define CPLANE_PAD1				19

// 0-2 are axial planes
#define	PLANE_X			0
#define	PLANE_Y			1
#define	PLANE_Z			2

// 3-5 are non-axial planes snapped to the nearest
#define	PLANE_ANYX		3
#define	PLANE_ANYY		4
#define	PLANE_ANYZ		5


//-----------------------------------------------------------------------------
// Frustum plane indices.
// WARNING: there is code that depends on these values
//-----------------------------------------------------------------------------

enum
{
	FRUSTUM_RIGHT		= 0,
	FRUSTUM_LEFT		= 1,
	FRUSTUM_TOP			= 2,
	FRUSTUM_BOTTOM		= 3,
	FRUSTUM_NEARZ		= 4,
	FRUSTUM_FARZ		= 5,
	FRUSTUM_NUMPLANES	= 6
};

extern int SignbitsForPlane( cplane_t *out );

class Frustum_t
{
public:
	void SetPlane( int i, int nType, const Vector &vecNormal, float dist )
	{
		m_Plane[i].normal = vecNormal;
		m_Plane[i].dist = dist;
		m_Plane[i].type = nType;
		m_Plane[i].signbits = SignbitsForPlane( &m_Plane[i] );
		m_AbsNormal[i].Init( fabs(vecNormal.x), fabs(vecNormal.y), fabs(vecNormal.z) );
	}

	inline const cplane_t *GetPlane( int i ) const { return &m_Plane[i]; }
	inline const Vector &GetAbsNormal( int i ) const { return m_AbsNormal[i]; }

private:
	cplane_t	m_Plane[FRUSTUM_NUMPLANES];
	Vector		m_AbsNormal[FRUSTUM_NUMPLANES];
};

// Cull the world-space bounding box to the specified frustum.
extern bool R_CullBox( const Vector& mins, const Vector& maxs, const Frustum_t &frustum );
extern bool R_CullBoxSkipNear( const Vector& mins, const Vector& maxs, const Frustum_t &frustum );

struct matrix3x4_t
{
	matrix3x4_t() {}
	matrix3x4_t( 
		float m00, float m01, float m02, float m03,
		float m10, float m11, float m12, float m13,
		float m20, float m21, float m22, float m23 )
	{
		m_flMatVal[0][0] = m00;	m_flMatVal[0][1] = m01; m_flMatVal[0][2] = m02; m_flMatVal[0][3] = m03;
		m_flMatVal[1][0] = m10;	m_flMatVal[1][1] = m11; m_flMatVal[1][2] = m12; m_flMatVal[1][3] = m13;
		m_flMatVal[2][0] = m20;	m_flMatVal[2][1] = m21; m_flMatVal[2][2] = m22; m_flMatVal[2][3] = m23;
	}

	float *operator[]( int i )				{ Assert(( i >= 0 ) && ( i < 3 )); return m_flMatVal[i]; }
	const float *operator[]( int i ) const	{ Assert(( i >= 0 ) && ( i < 3 )); return m_flMatVal[i]; }
	float *Base()							{ return &m_flMatVal[0][0]; }
	const float *Base() const				{ return &m_flMatVal[0][0]; }

	float m_flMatVal[3][4];
};


#ifndef M_PI
	#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#endif

#define M_PI_F		((float)(M_PI))	// Shouldn't collide with anything.

// NJS: Inlined to prevent floats from being autopromoted to doubles, as with the old system.
#ifndef RAD2DEG
	#define RAD2DEG( x  )  ( (float)(x) * (float)(180.f / M_PI_F) )
#endif

#ifndef DEG2RAD
	#define DEG2RAD( x  )  ( (float)(x) * (float)(M_PI_F / 180.f) )
#endif

// Used to represent sides of things like planes.
#define	SIDE_FRONT	0
#define	SIDE_BACK	1
#define	SIDE_ON		2
#define SIDE_CROSS  -2      // necessary for polylib.c

#define ON_VIS_EPSILON  0.01    // necessary for vvis (flow.c) -- again look into moving later!
#define	EQUAL_EPSILON	0.001   // necessary for vbsp (faces.c) -- should look into moving it there?


extern  const Vector vec3_origin;
extern  const QAngle vec3_angle;
extern const Vector vec3_invalid;
extern	const int nanmask;

#define	IS_NAN(x) (((*(int *)&x)&nanmask)==nanmask)

FORCEINLINE_MATH vec_t DotProduct(const vec_t *v1, const vec_t *v2)
{
	return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

FORCEINLINE_MATH void VectorSubtract(const vec_t *a, const vec_t *b, vec_t *c)
{
	c[0]=a[0]-b[0];
	c[1]=a[1]-b[1];
	c[2]=a[2]-b[2];
}

FORCEINLINE_MATH void VectorAdd(const vec_t *a, const vec_t *b, vec_t *c)
{
	c[0]=a[0]+b[0];
	c[1]=a[1]+b[1];
	c[2]=a[2]+b[2];
}

FORCEINLINE_MATH void VectorCopy(const vec_t *a, vec_t *b)
{
	b[0]=a[0];
	b[1]=a[1];
	b[2]=a[2];
}

FORCEINLINE_MATH void VectorClear(vec_t *a)
{
	a[0]=a[1]=a[2]=0;
}

FORCEINLINE_MATH float VectorMaximum(const vec_t *v)
{
	return max( v[0], max( v[1], v[2] ) );
}

FORCEINLINE_MATH float VectorMaximum(const Vector& v)
{
	return max( v.x, max( v.y, v.z ) );
}

FORCEINLINE_MATH void VectorScale (const float* in, vec_t scale, float* out)
{
	out[0] = in[0]*scale;
	out[1] = in[1]*scale;
	out[2] = in[2]*scale;
}


// Cannot be forceinline as they have overloads:
inline void VectorFill(vec_t *a, float b)
{
	a[0]=a[1]=a[2]=b;
}

inline void VectorNegate(vec_t *a)
{
	a[0]=-a[0];
	a[1]=-a[1];
	a[2]=-a[2];
}


//#define VectorMaximum(a)		( max( (a)[0], max( (a)[1], (a)[2] ) ) )
#define Vector2Clear(x)			{(x)[0]=(x)[1]=0;}
#define Vector2Negate(x)		{(x)[0]=-((x)[0]);(x)[1]=-((x)[1]);}
#define Vector2Copy(a,b)		{(b)[0]=(a)[0];(b)[1]=(a)[1];}
#define Vector2Subtract(a,b,c)	{(c)[0]=(a)[0]-(b)[0];(c)[1]=(a)[1]-(b)[1];}
#define Vector2Add(a,b,c)		{(c)[0]=(a)[0]+(b)[0];(c)[1]=(a)[1]+(b)[1];}
#define Vector2Scale(a,b,c)		{(c)[0]=(b)*(a)[0];(c)[1]=(b)*(a)[1];}

// NJS: Some functions in VBSP still need to use these for dealing with mixing vec4's and shorts with vec_t's.
// remove when no longer needed.
#define VECTOR_COPY( A, B ) do { (B)[0] = (A)[0]; (B)[1] = (A)[1]; (B)[2]=(A)[2]; } while(0)
#define DOT_PRODUCT( A, B ) ( (A)[0]*(B)[0] + (A)[1]*(B)[1] + (A)[2]*(B)[2] )

#ifndef PFN_VECTORMA
void VectorMA( const float *start, float scale, const float *direction, float *dest );
#endif

FORCEINLINE_MATH void VectorMAInline( const float* start, float scale, const float* direction, float* dest )
{
	dest[0]=start[0]+direction[0]*scale;
	dest[1]=start[1]+direction[1]*scale;
	dest[2]=start[2]+direction[2]*scale;
}

FORCEINLINE_MATH void VectorMAInline( const Vector& start, float scale, const Vector& direction, Vector& dest )
{
	dest.x=start.x+direction.x*scale;
	dest.y=start.y+direction.y*scale;
	dest.z=start.z+direction.z*scale;
}


vec_t _DotProduct (const float *v1, const float *v2);

int VectorCompare (const float *v1, const float *v2);

inline float VectorLength(const float *v)
{
	return FastSqrt( v[0]*v[0] + v[1]*v[1] + v[2]*v[2] + FLT_EPSILON );
}

void CrossProduct (const float *v1, const float *v2, float *cross);

qboolean VectorsEqual( const float *v1, const float *v2 );

inline vec_t RoundInt (vec_t in)
{
	return floor(in + 0.5f);
}

int Q_log2(int val);

// Math routines done in optimized assembly math package routines
void inline SinCos( float radians, float *sine, float *cosine )
{
#ifdef _WIN32
	_asm
	{
		fld		DWORD PTR [radians]
		fsincos

		mov edx, DWORD PTR [cosine]
		mov eax, DWORD PTR [sine]

		fstp DWORD PTR [edx]
		fstp DWORD PTR [eax]
	}
#elif _LINUX
	register double __cosr, __sinr;
 	__asm __volatile__
    		("fsincos"
     	: "=t" (__cosr), "=u" (__sinr) : "0" (radians));

  	*sine = __sinr;
  	*cosine = __cosr;
#endif
}

#define SIN_TABLE_SIZE	256
#define FTOIBIAS	12582912.f
extern float SinCosTable[SIN_TABLE_SIZE];


inline float TableCos( float theta )
{
	union
	{
		int i;
		float f;
	} ftmp;

	// ideally, the following should compile down to: theta * constant + constant, changing any of these constants from defines sometimes fubars this.
	ftmp.f = theta * ( float )( SIN_TABLE_SIZE / ( 2.0f * M_PI ) ) + ( FTOIBIAS + ( SIN_TABLE_SIZE / 4 ) );
	return SinCosTable[ ftmp.i & ( SIN_TABLE_SIZE - 1 ) ];
}

inline float TableSin( float theta )
{
	union
	{
		int i;
		float f;
	} ftmp;

	// ideally, the following should compile down to: theta * constant + constant
	ftmp.f = theta * ( float )( SIN_TABLE_SIZE / ( 2.0f * M_PI ) ) + FTOIBIAS;
	return SinCosTable[ ftmp.i & ( SIN_TABLE_SIZE - 1 ) ];
}


// These globals are initialized in GL_RMISC.C: R_SSE(), R_3DNow(), R_MMX()
extern float (*pfInvRSquared)(const float *v);
extern float (*pfSqrt)(float x);
extern float (*pfRSqrt)(float x);
extern float (*pfRSqrtFast)(float x);
extern void  (*pfFastSinCos)(float x, float* s, float* c);
extern float (*pfFastCos)(float x);

// The following are not declared as macros because they are often used in limiting situations,
// and sometimes the compiler simply refuses to inline them for soem reason
#define FastSqrt(x)			(*pfSqrt)(x)
#define	FastRSqrt(x)		(*pfRSqrt)(x)
#define FastRSqrtFast(x)    (*pfRSqrtFast)(x)
#define FastSinCos(x,s,c)   (*pfFastSinCos)(x,s,c)
#define FastCos(x)			(*pfFastCos)(x)

FORCEINLINE_MATH vec_t InvRSquared(float const* v)
{
	return (*pfInvRSquared)(v);
}

FORCEINLINE_MATH vec_t InvRSquared( const Vector& v )
{
	return InvRSquared(&v.x);
}


#ifdef PFN_VECTORMA
extern void  (__cdecl *pfVectorMA)( const float *start, float scale, const float *direction, float *dest );
#define VectorMA(v1,s,v2,d)	(*pfVectorMA)(v1,s,v2,d)
#endif

template<class T>
FORCEINLINE_TEMPLATE T Square( T const &a )
{
	return a * a;
}


// retunrn the largest power of two less than or =  x. will return 0 if passed 0.
FORCEINLINE_MATH int LargestPowerOfTwoLessThanOrEqual(int x)
{
	int ret=1;
	for(;;)
	{
		if (ret==x)
			return ret;
		if (ret>x)
			return ret>>1;
		ret<<=1;
	}
}


// Math routines for optimizing division
void FloorDivMod (double numer, double denom, int *quotient,
		int *rem);
int GreatestCommonDivisor (int i1, int i2);

// Test for FPU denormal mode
bool IsDenormal( const float &val );

// MOVEMENT INFO
enum
{
	PITCH = 0,	// up / down
	YAW,		// left / right
	ROLL		// fall over
};

// Intersect the (infinite) line defined by point vLinePt and direction vLineDir
// with the sphere defined by vSphereCenter and fSphereRadius.
// Returns 0 if the line doesn't intersect the sphere.
// Returns 1 and sets fIntersection1 and fIntersection2 if the line does intersect the sphere.
// (Intersection points will be at vLinePt+vLineDir*fIntersection1 and vLinePt+vLineDir*fIntersection2.
int LineSphereIntersection(
	const Vector &vSphereCenter,
	const float fSphereRadius,
	const Vector &vLinePt,
	const Vector &vLineDir,
	float *fIntersection1,
	float *fIntersection2);

void MatrixAngles( const matrix3x4_t & matrix, float *angles ); // !!!!
void VectorTransform (const float *in1, const matrix3x4_t & in2, float *out);
void VectorITransform (const float *in1, const matrix3x4_t & in2, float *out);
void VectorRotate( const float *in1, const matrix3x4_t & in2, float *out);
void VectorRotate( const Vector &in1, const QAngle &in2, Vector &out );
void VectorIRotate( const float *in1, const matrix3x4_t & in2, float *out);

#ifndef VECTOR_NO_SLOW_OPERATIONS

QAngle TransformAnglesToLocalSpace( const QAngle &angles, const matrix3x4_t &parentMatrix );
QAngle TransformAnglesToWorldSpace( const QAngle &angles, const matrix3x4_t &parentMatrix );

#endif

void MatrixInitialize( matrix3x4_t &mat, const Vector &vecOrigin, const Vector &vecXAxis, const Vector &vecYAxis, const Vector &vecZAxis );
void MatrixCopy( const matrix3x4_t &in, matrix3x4_t &out );
void MatrixInvert( const matrix3x4_t &in, matrix3x4_t &out );

void MatrixGetColumn( const matrix3x4_t &in, int column, Vector &out );
void MatrixSetColumn( const Vector &in, int column, matrix3x4_t &out );

//void DecomposeRotation( const matrix3x4_t &mat, float *out );
void ConcatRotations (const matrix3x4_t &in1, const matrix3x4_t &in2, matrix3x4_t &out);
void ConcatTransforms (const matrix3x4_t &in1, const matrix3x4_t &in2, matrix3x4_t &out);

void QuaternionSlerp( const Quaternion &p, const Quaternion &q, float t, Quaternion &qt );
void QuaternionSlerpNoAlign( const Quaternion &p, const Quaternion &q, float t, Quaternion &qt );
void QuaternionBlend( const Quaternion &p, const Quaternion &q, float t, Quaternion &qt );
void QuaternionBlendNoAlign( const Quaternion &p, const Quaternion &q, float t, Quaternion &qt );
void QuaternionIdentityBlend( const Quaternion &p, float t, Quaternion &qt );
float QuaternionAngleDiff( const Quaternion &p, const Quaternion &q );
void QuaternionScale( const Quaternion &p, float t, Quaternion &q );
void QuaternionAlign( const Quaternion &p, const Quaternion &q, Quaternion &qt );
float QuaternionDotProduct( const Quaternion &p, const Quaternion &q );
void QuaternionConjugate( const Quaternion &p, Quaternion &q );
void QuaternionInvert( const Quaternion &p, Quaternion &q );
float QuaternionNormalize( Quaternion &q );
void QuaternionAdd( const Quaternion &p, const Quaternion &q, Quaternion &qt );
void QuaternionMult( const Quaternion &p,  const Quaternion &q, Quaternion &qt );
void QuaternionMatrix( const Quaternion &q, matrix3x4_t &matrix );
void QuaternionMatrix( const Quaternion &q, const Vector &pos, matrix3x4_t &matrix );
void QuaternionAngles( const Quaternion &q, QAngle &angles );
void AngleQuaternion( const QAngle& angles, Quaternion &qt );
void QuaternionAngles( const Quaternion &q, RadianEuler &angles );
void AngleQuaternion( RadianEuler const &angles, Quaternion &qt );
void QuaternionAxisAngle( const Quaternion &q, Vector &axis, float &angle );
void AxisAngleQuaternion( const Vector &axis, float angle, Quaternion &q );
void BasisToQuaternion( const Vector &vecForward, const Vector &vecRight, const Vector &vecUp, Quaternion &q );

// A couple methods to find the dot product of a vector with a matrix row or column...
inline float MatrixRowDotProduct( const matrix3x4_t &in1, int row, const Vector& in2 )
{
	Assert( (row >= 0) && (row < 3) );
	return DotProduct( in1[row], in2.Base() ); 
}

inline float MatrixColumnDotProduct( const matrix3x4_t &in1, int col, const Vector& in2 )
{
	Assert( (col >= 0) && (col < 4) );
	return in1[0][col] * in2[0] + in1[1][col] * in2[1] + in1[2][col] * in2[2]; 
}

int __cdecl BoxOnPlaneSide (const float *emins, const float *emaxs, const cplane_t *plane);

inline float anglemod(float a)
{
	a = (360.f/65536) * ((int)(a*(65536.f/360.0f)) & 65535);
	return a;
}

// Remap a value in the range [A,B] to [C,D].
inline float RemapVal( float val, float A, float B, float C, float D)
{
	return C + (D - C) * (val - A) / (B - A);
}

inline float RemapValClamped( float val, float A, float B, float C, float D)
{
	float cVal = (val - A) / (B - A);
	cVal = clamp( cVal, 0.0f, 1.0f );

	return C + (D - C) * cVal;
}

// Returns A + (B-A)*flPercent.
// float Lerp( float flPercent, float A, float B );
template <class T>
FORCEINLINE_MATH T Lerp( float flPercent, T const &A, T const &B )
{
	return A + (B - A) * flPercent;
}

// 5-argument floating point linear interpolation.
// FLerp(f1,f2,i1,i2,x)=
//    f1 at x=i1
//    f2 at x=i2
//   smooth lerp between f1 and f2 at x>i1 and x<i2
//   extrapolation for x<i1 or x>i2
//
//   If you know a function f(x)'s value (f1) at position i1, and its value (f2) at position i2,
//   the function can be linearly interpolated with FLerp(f1,f2,i1,i2,x)
//    i2=i1 will cause a divide by zero.
static inline float FLerp(float f1, float f2, float i1, float i2, float x)
{
  return f1+(f2-f1)*(x-i1)/(i2-i1);
}


#ifndef VECTOR_NO_SLOW_OPERATIONS

// YWB:  Specialization for interpolating euler angles via quaternions...
template<> FORCEINLINE_MATH QAngle Lerp<QAngle>( float flPercent, const QAngle& q1, const QAngle& q2 )
{
	// Avoid precision errors
	if ( q1 == q2 )
		return q1;

	Quaternion src, dest;

	// Convert to quaternions
	AngleQuaternion( q1, src );
	AngleQuaternion( q2, dest );

	Quaternion result;

	// Slerp
	QuaternionSlerp( src, dest, flPercent, result );

	// Convert to euler
	QAngle output;
	QuaternionAngles( result, output );
	return output;
}

#else

#pragma error

// NOTE NOTE: I haven't tested this!! It may not work! Check out interpolatedvar.cpp in the client dll to try it
template<> FORCEINLINE_TEMPLATE QAngleByValue Lerp<QAngleByValue>( float flPercent, const QAngleByValue& q1, const QAngleByValue& q2 )
{
	// Avoid precision errors
	if ( q1 == q2 )
		return q1;

	Quaternion src, dest;

	// Convert to quaternions
	AngleQuaternion( q1, src );
	AngleQuaternion( q2, dest );

	Quaternion result;

	// Slerp
	QuaternionSlerp( src, dest, flPercent, result );

	// Convert to euler
	QAngleByValue output;
	QuaternionAngles( result, output );
	return output;
}

#endif // VECTOR_NO_SLOW_OPERATIONS


// Swap two of anything.
template <class T> 
FORCEINLINE_TEMPLATE void swap( T& x, T& y )
{
	T temp = x;
	x = y;
	y = temp;
}

template <class T> FORCEINLINE_TEMPLATE T AVG(T a, T b)
{
	return (a+b)/2;
}

// number of elements in an array of static size
#define NELEMS(x) ((sizeof(x))/sizeof(x[0]))

// XYZ macro, for printf type functions - ex printf("%f %f %f",XYZ(myvector));
#define XYZ(v) (v).x,(v).y,(v).z

//
// Returns a clamped value in the range [min, max].
//
#define clamp(val, min, max) (((val) > (max)) ? (max) : (((val) < (min)) ? (min) : (val)))

inline float Sign( float x )
{
	return (x <0.0f) ? -1.0f : 1.0f;
}

//
// Clamps the input integer to the given array bounds.
// Equivilent to the following, but without using any branches:
//
// if( n < 0 ) return 0;
// else if ( n > maxindex ) return maxindex;
// else return n;
//
// This is not always a clear performance win, but when you have situations where a clamped 
// value is thrashing against a boundary this is a big win. (ie, valid, invalid, valid, invalid, ...)
//
// Note: This code has been run against all possible integers.
//
inline int ClampArrayBounds( int n, unsigned maxindex )
{
	// mask is 0 if less than 4096, 0xFFFFFFFF if greater than
	unsigned int inrangemask = 0xFFFFFFFF + (((unsigned) n) > maxindex );
	unsigned int lessthan0mask = 0xFFFFFFFF + ( n >= 0 );
	
	// If the result was valid, set the result, (otherwise sets zero)
	int result = (inrangemask & n);

	// if the result was out of range or zero.
	result |= ((~inrangemask) & (~lessthan0mask)) & maxindex;

	return result;
}


#define BOX_ON_PLANE_SIDE(emins, emaxs, p)	\
	(((p)->type < 3)?						\
	(										\
		((p)->dist <= (emins)[(p)->type])?	\
			1								\
		:									\
		(									\
			((p)->dist >= (emaxs)[(p)->type])?\
				2							\
			:								\
				3							\
		)									\
	)										\
	:										\
		BoxOnPlaneSide( (emins), (emaxs), (p)))

//-----------------------------------------------------------------------------
// FIXME: Vector versions.... the float versions will go away hopefully soon!
//-----------------------------------------------------------------------------

bool SphereToAABBIntersection( const Vector& sphCenter, float sphRadius, 
			   				   const Vector& boxMin, const Vector& boxMax );

void AngleVectors (const QAngle& angles, Vector *forward);
void AngleVectors (const QAngle& angles, Vector *forward, Vector *right, Vector *up);
void AngleVectorsTranspose (const QAngle& angles, Vector *forward, Vector *right, Vector *up);
void AngleMatrix (const QAngle &angles, matrix3x4_t &mat );
void AngleMatrix( const QAngle &angles, const Vector &position, matrix3x4_t &mat );
void AngleMatrix (const RadianEuler &angles, matrix3x4_t &mat );
void AngleMatrix( RadianEuler const &angles, const Vector &position, matrix3x4_t &mat );
void AngleIMatrix (const QAngle &angles, matrix3x4_t &mat );
void AngleIMatrix (const QAngle &angles, const Vector &position, matrix3x4_t &mat );
void AngleIMatrix (const RadianEuler &angles, matrix3x4_t &mat );
void VectorAngles( const Vector &forward, QAngle &angles );
void VectorAngles( const Vector &forward, const Vector &pseudoup, QAngle &angles );
void VectorMatrix( const Vector &forward, matrix3x4_t &mat );
void VectorVectors( const Vector &forward, Vector &right, Vector &up );
void SetIdentityMatrix( matrix3x4_t &mat );

inline void PositionMatrix( const Vector &position, matrix3x4_t &mat )
{
	MatrixSetColumn( position, 3, mat );
}

inline void MatrixPosition( const matrix3x4_t &matrix, Vector &position )
{
	MatrixGetColumn( matrix, 3, position );
}

inline void VectorRotate( const Vector& in1, const matrix3x4_t &in2, Vector &out)
{
	VectorRotate( &in1.x, in2, &out.x );
}

inline void VectorIRotate( const Vector& in1, const matrix3x4_t &in2, Vector &out)
{
	VectorIRotate( &in1.x, in2, &out.x );
}

inline void MatrixAngles( const matrix3x4_t &matrix, QAngle &angles )
{
	MatrixAngles( matrix, &angles.x );
}

inline void MatrixAngles( const matrix3x4_t &matrix, QAngle &angles, Vector &position )
{
	MatrixAngles( matrix, angles );
	MatrixPosition( matrix, position );
}

inline void MatrixAngles( const matrix3x4_t &matrix, RadianEuler &angles )
{
	MatrixAngles( matrix, &angles.x );

	angles.Init( DEG2RAD( angles.z ), DEG2RAD( angles.x ), DEG2RAD( angles.y ) );
}

void MatrixAngles( const matrix3x4_t &mat, RadianEuler &angles, Vector &position );

void MatrixAngles( const matrix3x4_t &mat, Quaternion &q, Vector &position );

inline int VectorCompare (const Vector& v1, const Vector& v2)
{
	return v1 == v2;
}

inline void VectorTransform (const Vector& in1, const matrix3x4_t &in2, Vector &out)
{
	VectorTransform( &in1.x, in2, &out.x );
}

inline void VectorITransform (const Vector& in1, const matrix3x4_t &in2, Vector &out)
{
	VectorITransform( &in1.x, in2, &out.x );
}

/*
inline void DecomposeRotation( const matrix3x4_t &mat, Vector &out )
{
	DecomposeRotation( mat, &out.x );
}
*/

inline int BoxOnPlaneSide (const Vector& emins, const Vector& emaxs, const cplane_t *plane )
{
	return BoxOnPlaneSide( &emins.x, &emaxs.x, plane );
}

inline void VectorFill(Vector& a, float b)
{
	a[0]=a[1]=a[2]=b;
}

inline void VectorNegate(Vector& a)
{
	a[0] = -a[0];
	a[1] = -a[1];
	a[2] = -a[2];
}

inline vec_t VectorAvg(Vector& a)
{
	return ( a[0] + a[1] + a[2] ) / 3;
}

//-----------------------------------------------------------------------------
// Box/plane test (slow version)
//-----------------------------------------------------------------------------
inline int FASTCALL BoxOnPlaneSide2 (const Vector& emins, const Vector& emaxs, const cplane_t *p, float tolerance = 0.f )
{
	Vector	corners[2];

	if (p->normal[0] < 0)
	{
		corners[0][0] = emins[0];
		corners[1][0] = emaxs[0];
	}
	else
	{
		corners[1][0] = emins[0];
		corners[0][0] = emaxs[0];
	}

	if (p->normal[1] < 0)
	{
		corners[0][1] = emins[1];
		corners[1][1] = emaxs[1];
	}
	else
	{
		corners[1][1] = emins[1];
		corners[0][1] = emaxs[1];
	}

	if (p->normal[2] < 0)
	{
		corners[0][2] = emins[2];
		corners[1][2] = emaxs[2];
	}
	else
	{
		corners[1][2] = emins[2];
		corners[0][2] = emaxs[2];
	}

	int sides = 0;

	float dist1 = DotProduct (p->normal, corners[0]) - p->dist;
	if (dist1 >= tolerance)
		sides = 1;

	float dist2 = DotProduct (p->normal, corners[1]) - p->dist;
	if (dist2 < -tolerance)
		sides |= 2;

	return sides;
}

//-----------------------------------------------------------------------------
// Helpers for bounding box construction
//-----------------------------------------------------------------------------

void ClearBounds (Vector& mins, Vector& maxs);
void AddPointToBounds (const Vector& v, Vector& mins, Vector& maxs);

//
// COLORSPACE/GAMMA CONVERSION STUFF
//
void BuildGammaTable( float gamma, float texGamma, float brightness, int overbright );
void BuildExponentTable( );

// convert texture to linear 0..1 value
inline float TexLightToLinear( int c, int exponent )
{
	extern float power2_n[256]; 
	Assert( exponent >= -128 && exponent <= 127 );
	return ( float )c * power2_n[exponent+128];
}


// convert texture to linear 0..1 value
int LinearToTexture( float f );
// converts 0..1 linear value to screen gamma (0..255)
int LinearToScreenGamma( float f );
float TextureToLinear( int c );

// compressed color format 
struct ColorRGBExp32
{
	byte r, g, b;
	signed char exponent;
};

void ColorRGBExp32ToVector( const ColorRGBExp32& in, Vector& out );
void VectorToColorRGBExp32( const Vector& v, ColorRGBExp32 &c );

// solve for "x" where "a x^2 + b x + c = 0", return true if solution exists
bool SolveQuadratic( float a, float b, float c, float &root1, float &root2 );

// solves for "a, b, c" where "a x^2 + b x + c = y", return true if solution exists
bool SolveInverseQuadratic( float x1, float y1, float x2, float y2, float x3, float y3, float &a, float &b, float &c );

// solves for a,b,c specified as above, except that it always creates a monotonically increasing or
// decreasing curve if the data is monotonically increasing or decreasing. In order to enforce the
// monoticity condition, it is possible that the resulting quadratic will only approximate the data
// instead of interpolating it. This code is not especially fast.
bool SolveInverseQuadraticMonotonic( float x1, float y1, float x2, float y2, 
									 float x3, float y3, float &a, float &b, float &c );




// solves for "a, b, c" where "1/(a x^2 + b x + c ) = y", return true if solution exists
bool SolveInverseReciprocalQuadratic( float x1, float y1, float x2, float y2, float x3, float y3, float &a, float &b, float &c );

// rotate a vector around the Z axis (YAW)
void VectorYawRotate( const Vector& in, float flYaw, Vector &out);


// Bias takes an X value between 0 and 1 and returns another value between 0 and 1
// The curve is biased towards 0 or 1 based on biasAmt, which is between 0 and 1.
// Lower values of biasAmt bias the curve towards 0 and higher values bias it towards 1.
//
// For example, with biasAmt = 0.2, the curve looks like this:
//
// 1
// |				  *
// |				  *
// |			     *
// |			   **
// |			 **
// |	  	 ****
// |*********
// |___________________
// 0                   1
//
//
// With biasAmt = 0.8, the curve looks like this:
//
// 1
// | 	**************
// |  **
// | * 
// | *
// |* 
// |* 
// |*  
// |___________________
// 0                   1
//
// With a biasAmt of 0.5, Bias returns X.
float Bias( float x, float biasAmt );


// Gain is similar to Bias, but biasAmt biases towards or away from 0.5.
// Lower bias values bias towards 0.5 and higher bias values bias away from it.
//
// For example, with biasAmt = 0.2, the curve looks like this:
//
// 1
// | 				  *
// | 				 *
// | 				**
// |  ***************
// | **
// | *
// |*
// |___________________
// 0                   1
//
//
// With biasAmt = 0.8, the curve looks like this:
//
// 1
// |  		    *****
// |  		 ***
// |  		*
// | 		*
// | 		*
// |   	 ***
// |*****
// |___________________
// 0                   1
float Gain( float x, float biasAmt );


// SmoothCurve maps a 0-1 value into another 0-1 value based on a cosine wave
// where the derivatives of the function at 0 and 1 (and 0.5) are 0. This is useful for
// any fadein/fadeout effect where it should start and end smoothly.
//
// The curve looks like this:
//
// 1
// |  		**
// | 	   *  *
// | 	  *	   *
// | 	  *	   *
// | 	 *		*
// |   **		 **
// |***			   ***
// |___________________
// 0                   1
//
float SmoothCurve( float x );


// This works like SmoothCurve, with two changes:
//
// 1. Instead of the curve peaking at 0.5, it will peak at flPeakPos.
//    (So if you specify flPeakPos=0.2, then the peak will slide to the left).
//
// 2. flPeakSharpness is a 0-1 value controlling the sharpness of the peak.
//    Low values blunt the peak and high values sharpen the peak.
float SmoothCurve_Tweak( float x, float flPeakPos=0.5, float flPeakSharpness=0.5 );


//float ExponentialDecay( float halflife, float dt );
//float ExponentialDecay( float decayTo, float decayTime, float dt );

// halflife is time for value to reach 50%
inline float ExponentialDecay( float halflife, float dt )
{
	// log(0.5) == -0.69314718055994530941723212145818
	return expf( -0.69314718f / halflife * dt);
}

// decayTo is factor the value should decay to in decayTime
inline float ExponentialDecay( float decayTo, float decayTime, float dt )
{
	return expf( logf( decayTo ) / decayTime * dt);
}


// hermite basis function for smooth interpolation
// Similar to Gain() above, but very cheap to call
// value should be between 0 & 1 inclusive
inline float SimpleSpline( float value )
{
	float valueSquared = value * value;

	// Nice little ease-in, ease-out spline-like curve
	return (3 * valueSquared - 2 * valueSquared * value);
}

// remaps a value in [startInterval, startInterval+rangeInterval] from linear to
// spline using SimpleSpline
inline float SimpleSplineRemapVal( float val, float A, float B, float C, float D)
{
	return C + (D - C) * SimpleSpline( (val - A) / (B - A) );
}

inline int RoundFloatToInt(float f)
{
	int nResult;
#ifdef _WIN32
	__asm
	{
		fld f
		fistp nResult
	}
#elif _LINUX
	__asm __volatile__ (
		"fistpl %0;": "=m" (nResult): "t" (f) : "st"
	);
#endif
	return nResult;
}

inline unsigned char RoundFloatToByte(float f)
{
	int nResult;

#ifdef _WIN32
	__asm
	{
		fld f
		fistp nResult
	}
#elif _LINUX
	__asm __volatile__ (
		"fistpl %0;": "=m" (nResult): "t" (f) : "st"
	);
#endif

#ifdef Assert
	Assert( nResult >= 0 && nResult <= 255 );
#endif 
	return nResult;
}

inline unsigned long RoundFloatToUnsignedLong(float f)
{
	unsigned char nResult[8];

#ifdef _WIN32
	__asm
	{
		fld f
		fistp       qword ptr nResult
	}
#elif _LINUX
	__asm __volatile__ (
		"fistpl %0;": "=m" (nResult): "t" (f) : "st"
	);
#endif

	return *((unsigned long*)nResult);
}

// Fast, accurate ftol:
inline int Float2Int( float a )
{
   int CtrlwdHolder;
   int CtrlwdSetter;
   int RetVal;

#ifdef _WIN32
   __asm 
   {
      fld    a					// push 'a' onto the FP stack
      fnstcw CtrlwdHolder		// store FPU control word
      movzx  eax, CtrlwdHolder	// move and zero extend word into eax
      and    eax, 0xFFFFF3FF	// set all bits except rounding bits to 1
      or     eax, 0x00000C00	// set rounding mode bits to round down
      mov    CtrlwdSetter, eax	// Prepare to set the rounding mode -- prepare to enter plaid!
      fldcw  CtrlwdSetter		// Entering plaid!
      fistp  RetVal				// Store and converted (to int) result
      fldcw  CtrlwdHolder		// Restore control word
   }
#elif _LINUX
	RetVal = static_cast<int>( a );
#endif

	return RetVal;
}

// Over 15x faster than: (int)floor(value)
inline int Floor2Int( float a )
{
   int CtrlwdHolder;
   int CtrlwdSetter;
   int RetVal;

#ifdef _WIN32
   __asm 
   {
      fld    a					// push 'a' onto the FP stack
      fnstcw CtrlwdHolder		// store FPU control word
      movzx  eax, CtrlwdHolder	// move and zero extend word into eax
      and    eax, 0xFFFFF3FF	// set all bits except rounding bits to 1
      or     eax, 0x00000400	// set rounding mode bits to round down
      mov    CtrlwdSetter, eax	// Prepare to set the rounding mode -- prepare to enter plaid!
      fldcw  CtrlwdSetter		// Entering plaid!
      fistp  RetVal				// Store floored and converted (to int) result
      fldcw  CtrlwdHolder		// Restore control word
   }
#elif _LINUX
	RetVal = static_cast<int>( floor(a) );
#endif

	return RetVal;
}

//-----------------------------------------------------------------------------
// Fast color conversion from float to unsigned char
//-----------------------------------------------------------------------------
FORCEINLINE_MATH unsigned char FastFToC( float c )
{
	volatile float dc;
	dc = c * 255.0f + ( float )( 1 << 23 );
	return *(unsigned char*)&dc;
}

//-----------------------------------------------------------------------------
// Purpose: Bound input float to .001 (millisecond) boundary
// Input  : in - 
// Output : inline float
//-----------------------------------------------------------------------------
inline float ClampToMsec( float in )
{
	int msec = Floor2Int( in * 1000.0f + 0.5f );
	return msec / 1000.0f;
}

// Over 15x faster than: (int)ceil(value)
inline int Ceil2Int( float a )
{
   int CtrlwdHolder;
   int CtrlwdSetter;
   int RetVal;

#ifdef _WIN32
   __asm 
   {
      fld    a					// push 'a' onto the FP stack
      fnstcw CtrlwdHolder		// store FPU control word
      movzx  eax, CtrlwdHolder	// move and zero extend word into eax
      and    eax, 0xFFFFF3FF	// set all bits except rounding bits to 1
      or     eax, 0x00000800	// set rounding mode bits to round down
      mov    CtrlwdSetter, eax	// Prepare to set the rounding mode -- prepare to enter plaid!
      fldcw  CtrlwdSetter		// Entering plaid!
      fistp  RetVal				// Store floored and converted (to int) result
      fldcw  CtrlwdHolder		// Restore control word
   }
#elif _LINUX
	RetVal = static_cast<int>( ceil(a) );
#endif

	return RetVal;
}


// Regular signed area of triangle
#define TriArea2D( A, B, C ) \
	( 0.5f * ( ( B.x - A.x ) * ( C.y - A.y ) - ( B.y - A.y ) * ( C.x - A.x ) ) )

// This version doesn't premultiply by 0.5f, so it's the area of the rectangle instead
#define TriArea2DTimesTwo( A, B, C ) \
	( ( ( B.x - A.x ) * ( C.y - A.y ) - ( B.y - A.y ) * ( C.x - A.x ) ) )


// Get the barycentric coordinates of "pt" in triangle [A,B,C].
inline void GetBarycentricCoords2D( 
	Vector2D const &A,
	Vector2D const &B,
	Vector2D const &C,
	Vector2D const &pt,
	float bcCoords[3] )
{
	// Note, because to top and bottom are both x2, the issue washes out in the composite
	float invTriArea = 1.0f / TriArea2DTimesTwo( A, B, C );

	// NOTE: We assume here that the lightmap coordinate vertices go counterclockwise.
	// If not, TriArea2D() is negated so this works out right.
	bcCoords[0] = TriArea2DTimesTwo( B, C, pt ) * invTriArea;
	bcCoords[1] = TriArea2DTimesTwo( C, A, pt ) * invTriArea;
	bcCoords[2] = TriArea2DTimesTwo( A, B, pt ) * invTriArea;
}


// Return true of the sphere might touch the box (the sphere is actually treated
// like a box itself, so this may return true if the sphere's bounding box touches
// a corner of the box but the sphere itself doesn't).
inline bool QuickBoxSphereTest( 
	const Vector& vOrigin,
	float flRadius,
	const Vector& bbMin,
	const Vector& bbMax )
{
	return vOrigin.x - flRadius < bbMax.x && vOrigin.x + flRadius > bbMin.x &&
		vOrigin.y - flRadius < bbMax.y && vOrigin.y + flRadius > bbMin.y && 
		vOrigin.z - flRadius < bbMax.z && vOrigin.z + flRadius > bbMin.z;
}


// Return true of the boxes intersect (but not if they just touch).
inline bool QuickBoxIntersectTest( 
	const Vector& vBox1Min,
	const Vector& vBox1Max,
	const Vector& vBox2Min,
	const Vector& vBox2Max )
{
	return 
		vBox1Min.x < vBox2Max.x && vBox1Max.x > vBox2Min.x &&
		vBox1Min.y < vBox2Max.y && vBox1Max.y > vBox2Min.y && 
		vBox1Min.z < vBox2Max.z && vBox1Max.z > vBox2Min.z;
}


extern float GammaToLinear( float gamma );
extern float LinearToGamma( float linear );
extern float GammaToLinearFullRange( float gamma );
extern float LinearToGammaFullRange( float linear );

// linear (0..4) to screen corrected vertex space (0..1?)
FORCEINLINE_MATH float LinearToVertexLight( float f )
{
	extern float lineartovertex[4096];	

	// Gotta clamp before the multiply; could overflow...
	int i = RoundFloatToInt(f * 1024.f);	// assume 0..4 range

	// Presumably the comman case will be not to clamp, so check that first:
	if( (unsigned)i > 4095 )
	{
		if( i < 0 ) i = 0;		// Compare to zero instead of 4095 to save 4 bytes in the instruction stream
		else		i = 4095;
	}

	return lineartovertex[i];
}


FORCEINLINE_MATH unsigned char LinearToLightmap( float f )
{
	extern unsigned char lineartolightmap[4096];	

	// Gotta clamp before the multiply; could overflow...
	int i = RoundFloatToInt( f * 1024.f );	// assume 0..4 range

	// Presumably the comman case will be not to clamp, so check that first:
	if( (unsigned)i > 4095 )
	{
		if( i < 0 ) i = 0;		// Compare to zero instead of 4095 to save 4 bytes in the instruction stream
		else		i = 4095;
	}

	return lineartolightmap[i];
}

FORCEINLINE_MATH void ColorClamp( Vector& color )
{
	float max = VectorMaximum( color );
	if( max > 1.0f )
	{
		float ooMax = 1.0f / max;
		color.x *= ooMax;
		color.y *= ooMax;
		color.z *= ooMax;
	}

	#if 1
		if( color[0] < 0.f ) color[0] = 0.f;
		if( color[1] < 0.f ) color[1] = 0.f;
		if( color[2] < 0.f ) color[2] = 0.f;
	#else
		#ifdef Assert
			Assert( color[0] >= 0.0f && color[0] <= 1.0f );
			Assert( color[1] >= 0.0f && color[1] <= 1.0f );
			Assert( color[2] >= 0.0f && color[2] <= 1.0f );
		#endif
	#endif
}

inline void ColorClampTruncate( Vector& color )
{
	if (color[0] > 1.0f) color[0] = 1.0f; else if (color[0] < 0.0f) color[0] = 0.0f;
	if (color[1] > 1.0f) color[1] = 1.0f; else if (color[1] < 0.0f) color[1] = 0.0f;
	if (color[2] > 1.0f) color[2] = 1.0f; else if (color[2] < 0.0f) color[2] = 0.0f;
}

// Interpolate a Catmull-Rom spline.
// t is a [0,1] value and interpolates a curve between p2 and p3.
void Catmull_Rom_Spline(
	const Vector &p1,
	const Vector &p2,
	const Vector &p3,
	const Vector &p4,
	float t, 
	Vector &output );

// Interpolate a Catmull-Rom spline.
// Returns the tangent of the point at t of the spline
void Catmull_Rom_Spline_Tangent( 
	const Vector &p1,
	const Vector &p2,
	const Vector &p3,
	const Vector &p4,
	float t, 
	Vector &output );

// Interpolate a Catmull-Rom spline.
// Normalize p2->p1 and p3->p4 to be the same length as p2->p3
void Catmull_Rom_Spline_Normalize(
	const Vector &p1,
	const Vector &p2,
	const Vector &p3,
	const Vector &p4,
	float t, 
	Vector &output );

// Interpolate a Catmull-Rom spline.
// Normalize p2.x->p1.x and p3.x->p4.x to be the same length as p2.x->p3.x
void Catmull_Rom_Spline_NormalizeX(
	const Vector &p1,
	const Vector &p2,
	const Vector &p3,
	const Vector &p4,
	float t, 
	Vector &output );


// Interpolate a Hermite spline.
// t is a [0,1] value and interpolates a curve between p1 and p2 with the deltas d1 and d2.
void Hermite_Spline(
	const Vector &p1,
	const Vector &p2,
	const Vector &d1,
	const Vector &d2,
	float t, 
	Vector& output );

float Hermite_Spline(
	float p1,
	float p2,
	float d1,
	float d2,
	float t );

// t is a [0,1] value and interpolates a curve between p1 and p2 with the slopes p0->p1 and p1->p2
void Hermite_Spline(
	const Vector &p0,
	const Vector &p1,
	const Vector &p2,
	float t, 
	Vector& output );

float Hermite_Spline(
	float p0,
	float p1,
	float p2,
	float t );


void Hermite_SplineBasis( float t, float basis[] );

void Hermite_Spline( 
	const Quaternion &q0, 
	const Quaternion &q1, 
	const Quaternion &q2, 
	float t, 
	Quaternion &output );


// See http://en.wikipedia.org/wiki/Kochanek-Bartels_curves
// 
// Tension:  -1 = Round -> 1 = Tight
// Bias:     -1 = Pre-shoot (bias left) -> 1 = Post-shoot (bias right)
// Continuity: -1 = Box corners -> 1 = Inverted corners
//
// If T=B=C=0 it's the same matrix as Catmull-Rom.
// If T=1 & B=C=0 it's the same as Cubic.
// If T=B=0 & C=-1 it's just linear interpolation
// 
// See http://news.povray.org/povray.binaries.tutorials/attachment/%3CXns91B880592482seed7@povray.org%3E/Splines.bas.txt
// for example code and descriptions of various spline types...
// 
void Kochanek_Bartels_Spline(
	float tension, 
	float bias, 
	float continuity,
	const Vector &p1,
	const Vector &p2,
	const Vector &p3,
	const Vector &p4,
	float t, 
	Vector& output );

void Kochanek_Bartels_Spline_NormalizeX(
	float tension, 
	float bias, 
	float continuity,
	const Vector &p1,
	const Vector &p2,
	const Vector &p3,
	const Vector &p4,
	float t, 
	Vector& output );

// See link at Kochanek_Bartels_Spline for info on the basis matrix used
void Cubic_Spline(
	const Vector &p1,
	const Vector &p2,
	const Vector &p3,
	const Vector &p4,
	float t, 
	Vector& output );

void Cubic_Spline_NormalizeX(
	const Vector &p1,
	const Vector &p2,
	const Vector &p3,
	const Vector &p4,
	float t, 
	Vector& output );

// See link at Kochanek_Bartels_Spline for info on the basis matrix used
void BSpline(
	const Vector &p1,
	const Vector &p2,
	const Vector &p3,
	const Vector &p4,
	float t, 
	Vector& output );

void BSpline_NormalizeX(
	const Vector &p1,
	const Vector &p2,
	const Vector &p3,
	const Vector &p4,
	float t, 
	Vector& output );

// See link at Kochanek_Bartels_Spline for info on the basis matrix used
void Parabolic_Spline(
	const Vector &p1,
	const Vector &p2,
	const Vector &p3,
	const Vector &p4,
	float t, 
	Vector& output );

void Parabolic_Spline_NormalizeX(
	const Vector &p1,
	const Vector &p2,
	const Vector &p3,
	const Vector &p4,
	float t, 
	Vector& output );

float RangeCompressor( float flValue, float flMin, float flMax, float flBase );

// Get the minimum distance from vOrigin to the bounding box defined by [mins,maxs]
// using voronoi regions.
// 0 is returned if the origin is inside the box.
float CalcSqrDistanceToAABB( const Vector &mins, const Vector &maxs, const Vector &point );
void CalcClosestPointOnAABB( const Vector &mins, const Vector &maxs, const Vector &point, Vector &closestOut );

inline float CalcDistanceToAABB( const Vector &mins, const Vector &maxs, const Vector &point )
{
	float flDistSqr = CalcSqrDistanceToAABB( mins, maxs, point );
	return sqrt(flDistSqr);
}

// Get the closest point from P to the (infinite) line through vLineA and vLineB and
// calculate the shortest distance from P to the line.
// If you pass in a value for t, it will tell you the t for (A + (B-A)t) to get the closest point.
// If the closest point lies on the segment between A and B, then 0 <= t <= 1.
void  CalcClosestPointOnLine( const Vector &P, const Vector &vLineA, const Vector &vLineB, Vector &vClosest, float *t=0 );
float CalcDistanceToLine( const Vector &P, const Vector &vLineA, const Vector &vLineB, float *t=0 );
float CalcDistanceSqrToLine( const Vector &P, const Vector &vLineA, const Vector &vLineB, float *t=0 );

// The same three functions as above, except now the line is closed between A and B.
void  CalcClosestPointOnLineSegment( const Vector &P, const Vector &vLineA, const Vector &vLineB, Vector &vClosest, float *t=0 );
float CalcDistanceToLineSegment( const Vector &P, const Vector &vLineA, const Vector &vLineB, float *t=0 );
float CalcDistanceSqrToLineSegment( const Vector &P, const Vector &vLineA, const Vector &vLineB, float *t=0 );

// A function to compute the closes line segment connnection two lines (or false if the lines are parallel, etc.)
bool CalcLineToLineIntersectionSegment(
   const Vector& p1,const Vector& p2,const Vector& p3,const Vector& p4,Vector *s1,Vector *s2,
   float *t1, float *t2 );

// The above functions in 2D
void  CalcClosestPointOnLine2D( Vector2D const &P, Vector2D const &vLineA, Vector2D const &vLineB, Vector2D &vClosest, float *t=0 );
float CalcDistanceToLine2D( Vector2D const &P, Vector2D const &vLineA, Vector2D const &vLineB, float *t=0 );
float CalcDistanceSqrToLine2D( Vector2D const &P, Vector2D const &vLineA, Vector2D const &vLineB, float *t=0 );
void  CalcClosestPointOnLineSegment2D( Vector2D const &P, Vector2D const &vLineA, Vector2D const &vLineB, Vector2D &vClosest, float *t=0 );
float CalcDistanceToLineSegment2D( Vector2D const &P, Vector2D const &vLineA, Vector2D const &vLineB, float *t=0 );
float CalcDistanceSqrToLineSegment2D( Vector2D const &P, Vector2D const &vLineA, Vector2D const &vLineB, float *t=0 );

// Init the mathlib
void MathLib_Init( float gamma, float texGamma, float brightness, int overbright, bool bAllow3DNow = true, bool bAllowSSE = true, bool bAllowSSE2 = true, bool bAllowMMX = true );
bool MathLib_3DNowEnabled( void );
bool MathLib_MMXEnabled( void );
bool MathLib_SSEEnabled( void );
bool MathLib_SSE2Enabled( void );

float Approach( float target, float value, float speed );
float ApproachAngle( float target, float value, float speed );
float AngleDiff( float destAngle, float srcAngle );
float AngleDistance( float next, float cur );
float AngleNormalize( float angle );

// ensure that 0 <= angle <= 360
float AngleNormalizePositive( float angle );

bool AnglesAreEqual( float a, float b, float tolerance = 0.0f );


void RotationDeltaAxisAngle( const QAngle &srcAngles, const QAngle &destAngles, Vector &deltaAxis, float &deltaAngle );
void RotationDelta( const QAngle &srcAngles, const QAngle &destAngles, QAngle *out );

void ComputeTrianglePlane( const Vector& v1, const Vector& v2, const Vector& v3, Vector& normal, float& intercept );
int PolyFromPlane( Vector *outVerts, const Vector& normal, float dist, float fHalfScale = 9000.0f );
int ClipPolyToPlane( Vector *inVerts, int vertCount, Vector *outVerts, const Vector& normal, float dist, float fOnPlaneEpsilon = 0.1f );
int ClipPolyToPlane_Precise( double *inVerts, int vertCount, double *outVerts, const double *normal, double dist, double fOnPlaneEpsilon = 0.1 );

//-----------------------------------------------------------------------------
// Transforms a AABB into another space; which will inherently grow the box.
//-----------------------------------------------------------------------------
void TransformAABB( const matrix3x4_t &in1, const Vector &vecMinsIn, const Vector &vecMaxsIn, Vector &vecMinsOut, Vector &vecMaxsOut );

//-----------------------------------------------------------------------------
// Uses the inverse transform of in1
//-----------------------------------------------------------------------------
void ITransformAABB( const matrix3x4_t &in1, const Vector &vecMinsIn, const Vector &vecMaxsIn, Vector &vecMinsOut, Vector &vecMaxsOut );

//-----------------------------------------------------------------------------
// Rotates a AABB into another space; which will inherently grow the box. 
// (same as TransformAABB, but doesn't take the translation into account)
//-----------------------------------------------------------------------------
void RotateAABB( const matrix3x4_t &in1, const Vector &vecMinsIn, const Vector &vecMaxsIn, Vector &vecMinsOut, Vector &vecMaxsOut );

//-----------------------------------------------------------------------------
// Uses the inverse transform of in1
//-----------------------------------------------------------------------------
void IRotateAABB( const matrix3x4_t &in1, const Vector &vecMinsIn, const Vector &vecMaxsIn, Vector &vecMinsOut, Vector &vecMaxsOut );

//-----------------------------------------------------------------------------
// Transform a plane
//-----------------------------------------------------------------------------
inline void MatrixTransformPlane( const matrix3x4_t &src, const cplane_t &inPlane, cplane_t &outPlane )
{
	// What we want to do is the following:
	// 1) transform the normal into the new space.
	// 2) Determine a point on the old plane given by plane dist * plane normal
	// 3) Transform that point into the new space
	// 4) Plane dist = DotProduct( new normal, new point )

	// An optimized version, which works if the plane is orthogonal.
	// 1) Transform the normal into the new space
	// 2) Realize that transforming the old plane point into the new space
	// is given by [ d * n'x + Tx, d * n'y + Ty, d * n'z + Tz ]
	// where d = old plane dist, n' = transformed normal, Tn = translational component of transform
	// 3) Compute the new plane dist using the dot product of the normal result of #2

	// For a correct result, this should be an inverse-transpose matrix
	// but that only matters if there are nonuniform scale or skew factors in this matrix.
	VectorRotate( inPlane.normal, src, outPlane.normal );
	outPlane.dist = inPlane.dist * DotProduct( outPlane.normal, outPlane.normal );
	outPlane.dist += outPlane.normal.x * src[0][3] + outPlane.normal.y * src[1][3] + outPlane.normal.z * src[2][3];
}

inline void MatrixITransformPlane( const matrix3x4_t &src, const cplane_t &inPlane, cplane_t &outPlane )
{
	// The trick here is that Tn = translational component of transform,
	// but for an inverse transform, Tn = - R^-1 * T
	Vector vecTranslation;
	MatrixGetColumn( src, 3, vecTranslation );

	Vector vecInvTranslation;
	VectorIRotate( vecTranslation, src, vecInvTranslation );

	VectorIRotate( inPlane.normal, src, outPlane.normal );
	outPlane.dist = inPlane.dist * DotProduct( outPlane.normal, outPlane.normal );
	outPlane.dist -= outPlane.normal.x * vecInvTranslation[0] + outPlane.normal.y * vecInvTranslation[1] + outPlane.normal.z * vecInvTranslation[2];
}

int CeilPow2( int in );
int FloorPow2( int in );

inline float *UnpackNormal( unsigned int *pPackedNormal, float *pNormal )
{
	int temp[3];
	temp[0] = ((*pPackedNormal >> 0L) & 0x7ff);
	if ( temp[0] & 0x400 )
	{
		temp[0] = 2048 - temp[0];
	}
	temp[1] = ((*pPackedNormal >> 11L) & 0x7ff);
	if ( temp[1] & 0x400 )
	{
		temp[1] = 2048 - temp[1];
	}
	temp[2] = ((*pPackedNormal >> 22L) & 0x3ff);
	if ( temp[2] & 0x200 )
	{
		temp[2] = 1024 - temp[2];
	}
	pNormal[0] = (float)temp[0] * 1.0f/1023.0f;
	pNormal[1] = (float)temp[1] * 1.0f/1023.0f;
	pNormal[2] = (float)temp[2] * 1.0f/511.0f;
	return pNormal;
}

FORCEINLINE_MATH unsigned int *PackNormal( float *pNormal, unsigned int *pPackedNormal )
{
	int temp[3];

	temp[0] = Float2Int( pNormal[0] * 1023.0f );
	temp[1] = Float2Int( pNormal[1] * 1023.0f );
	temp[2] = Float2Int( pNormal[2] * 511.0f );

	// the normal is out of bounds, determine the source and fix
	// clamping would be even more of a slowdown here
	Assert( temp[0] >= -1023 && temp[0] <= 1023 );
	Assert( temp[1] >= -1023 && temp[1] <= 1023 );
	Assert( temp[2] >= -511 && temp[2] <= 511 );
	
	*pPackedNormal = ( ( temp[2] & 0x3ff ) << 22L ) |
                     ( ( temp[1] & 0x7ff ) << 11L ) |
                     ( ( temp[0] & 0x7ff ) << 0L );
	return pPackedNormal;
}

FORCEINLINE_MATH unsigned int *PackNormal( float nx, float ny, float nz, unsigned int *pPackedNormal )
{
	int temp[3];

	temp[0] = Float2Int( nx * 1023.0f );
	temp[1] = Float2Int( ny * 1023.0f );
	temp[2] = Float2Int( nz * 511.0f );

	// the normal is out of bounds, determine the source and fix
	// clamping would be even more of a slowdown here
	Assert( temp[0] >= -1023 && temp[0] <= 1023 );
	Assert( temp[1] >= -1023 && temp[1] <= 1023 );
	Assert( temp[2] >= -511 && temp[2] <= 511 );
	
	*pPackedNormal = ( ( temp[2] & 0x3ff ) << 22L ) |
                     ( ( temp[1] & 0x7ff ) << 11L ) |
                     ( ( temp[0] & 0x7ff ) << 0L );
	return pPackedNormal;
}

#endif			// MATH_BASE_H

