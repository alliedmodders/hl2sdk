// $Id$


// ssemath.h - defines sse-based "structure of arrays" classes and functions.


#ifndef SSEMATH_H
#define SSEMATH_H

//#ifdef _WIN32

#include <xmmintrin.h>
#include <vector.h>

#ifdef _LINUX
typedef int int32;
typedef union __attribute__ ((aligned (16)))
 {
	float m128_f32[4];
 } l_m128;

#endif

// I thought about defining a class/union for the sse packed floats instead of using __m128,
// but decided against it because (a) the nature of sse code which includes comparisons is to blur
// the relationship between packed floats and packed integer types and (b) not sure that the
// compiler would handle generating good code for the intrinsics.


// useful constants in SSE packed float format:
extern __m128 Four_Zeros;									// 0 0 0 0
extern __m128 Four_Ones;									// 1 1 1 1
extern __m128 Four_PointFives;								// .5 .5 .5 .5
extern __m128 Four_Epsilons;								// FLT_EPSILON FLT_EPSILON FLT_EPSILON FLT_EPSILON


/// replicate a single floating point value to all 4 components of an sse packed float
static inline __m128 MMReplicate(float f)
{
	__m128 value=_mm_set_ss(f);
	return _mm_shuffle_ps(value,value,0);
}

/// replicate a single 32 bit integer value to all 4 components of an m128
static inline __m128 MMReplicateI(int i)
{
	__m128 value=_mm_set_ss(*((float *)&i));;
	return _mm_shuffle_ps(value,value,0);
}

/// 1/x for all 4 values. uses reciprocal approximation instruction plus newton iteration.
/// No error checking!
static inline __m128 MMReciprocal(__m128 x)
{
	__m128 ret=_mm_rcp_ps(x);
	// newton iteration is Y(n+1)=2*Y(N)-x*Y(n)^2
	ret=_mm_sub_ps(_mm_add_ps(ret,ret),_mm_mul_ps(x,_mm_mul_ps(ret,ret)));
	return ret;
}
/// 1/x for all 4 values. uses reciprocal approximation instruction plus newton iteration.
/// 1/0 will result in a big but NOT infinite result
static inline __m128 MMReciprocalSaturate(__m128 x)
{
	__m128 zero_mask=_mm_cmpeq_ps(x,Four_Zeros);
	x=_mm_or_ps(x,_mm_and_ps(Four_Epsilons,zero_mask));
	__m128 ret=_mm_rcp_ps(x);
	// newton iteration is Y(n+1)=2*Y(N)-x*Y(n)^2
	ret=_mm_sub_ps(_mm_add_ps(ret,ret),_mm_mul_ps(x,_mm_mul_ps(ret,ret)));
	return ret;
}

/// class FourVectors stores 4 independent vectors for use by sse processing. These vectors are
/// stored in the format x x x x y y y y z z z z so that they can be efficiently accelerated by
/// SSE. 
class FourVectors
{
public:
	__m128 x,y,z;											// x x x x y y y y z z z z

	inline void DuplicateVector(Vector const &v)	  //< set all 4 vectors to the same vector value
	{
		x=MMReplicate(v.x);
		y=MMReplicate(v.y);
		z=MMReplicate(v.z);
	}

	inline __m128 const & operator[](int idx) const
	{
		return *((&x)+idx);
	}

	inline __m128 & operator[](int idx)
	{
		return *((&x)+idx);
	}

	inline void operator+=(FourVectors const &b)			//< add 4 vectors to another 4 vectors
	{
		x=_mm_add_ps(x,b.x);
		y=_mm_add_ps(y,b.y);
		z=_mm_add_ps(z,b.z);
	}
	inline void operator-=(FourVectors const &b)			//< subtract 4 vectors from another 4
	{
		x=_mm_sub_ps(x,b.x);
		y=_mm_sub_ps(y,b.y);
		z=_mm_sub_ps(z,b.z);
	}

	inline void operator*=(FourVectors const &b)			//< scale all four vectors per component scale
	{
		x=_mm_mul_ps(x,b.x);
		y=_mm_mul_ps(y,b.y);
		z=_mm_mul_ps(z,b.z);
	}

	inline void operator*=(float scale)						//< uniformly scale all 4 vectors
	{
		__m128 scalepacked=MMReplicate(scale);
		x=_mm_mul_ps(x,scalepacked);
		y=_mm_mul_ps(y,scalepacked);
		z=_mm_mul_ps(z,scalepacked);
	}
	
	inline void operator*=(__m128 scale)					//< scale 
	{
		x=_mm_mul_ps(x,scale);
		y=_mm_mul_ps(y,scale);
		z=_mm_mul_ps(z,scale);
	}

	inline __m128 operator*(FourVectors const &b) const		//< 4 dot products
	{
		__m128 dot=_mm_mul_ps(x,b.x);
		dot=_mm_add_ps(dot,_mm_mul_ps(y,b.y));
		dot=_mm_add_ps(dot,_mm_mul_ps(z,b.z));
		return dot;
	}

	inline __m128 operator*(Vector const &b) const			//< dot product all 4 vectors with 1 vector
	{
		__m128 dot=_mm_mul_ps(x,MMReplicate(b.x));
		dot=_mm_add_ps(dot,_mm_mul_ps(y,MMReplicate(b.y)));
		dot=_mm_add_ps(dot,_mm_mul_ps(z,MMReplicate(b.z)));
		return dot;
	}


	inline void VProduct(FourVectors const &b)				//< component by component mul
	{
		x=_mm_mul_ps(x,b.x);
		y=_mm_mul_ps(y,b.y);
		z=_mm_mul_ps(z,b.z);
	}
	inline void MakeReciprocal(void)						//< (x,y,z)=(1/x,1/y,1/z)
	{
		x=MMReciprocal(x);
		y=MMReciprocal(y);
		z=MMReciprocal(z);
	}

	inline void MakeReciprocalSaturate(void)				//< (x,y,z)=(1/x,1/y,1/z), 1/0=1.0e23
	{
		x=MMReciprocalSaturate(x);
		y=MMReciprocalSaturate(y);
		z=MMReciprocalSaturate(z);
	}

	// X(),Y(),Z() - get at the desired component of the i'th (0..3) vector.
	inline float const &X(int idx) const
	{
#ifdef _LINUX
		return ((l_m128*)&x)->m128_f32[idx];
#else
		return x.m128_f32[idx];
#endif
	}

	inline float const &Y(int idx) const
	{
#ifdef _LINUX
		return ((l_m128*)&y)->m128_f32[idx];
#else
		return y.m128_f32[idx];
#endif
	}
	inline float const &Z(int idx) const
	{
#ifdef _LINUX
		return ((l_m128*)&z)->m128_f32[idx];
#else
		return z.m128_f32[idx];
#endif
	}

	// X(),Y(),Z() - get at the desired component of the i'th (0..3) vector.
	inline float &X(int idx)
	{
#ifdef _LINUX
		return ((l_m128*)&x)->m128_f32[idx];
#else
		return x.m128_f32[idx];
#endif
	}

	inline float &Y(int idx)
	{
#ifdef _LINUX
		return ((l_m128*)&y)->m128_f32[idx];
#else
		return y.m128_f32[idx];
#endif
	}
	inline float &Z(int idx)
	{
#ifdef _LINUX
		return ((l_m128*)&z)->m128_f32[idx];
#else
		return z.m128_f32[idx];
#endif
	}

	inline Vector Vec(int idx) const						//< unpack one of the vectors
	{
		return Vector(X(idx),Y(idx),Z(idx));
	}
	
	FourVectors(void)
	{
	}

	/// LoadAndSwizzle - load 4 Vectors into a FourVectors, perofrming transpose op
	inline void LoadAndSwizzle(Vector const &a, Vector const &b, Vector const &c, Vector const &d)
	{
		__m128 row0=_mm_loadu_ps(&(a.x));
		__m128 row1=_mm_loadu_ps(&(b.x));
		__m128 row2=_mm_loadu_ps(&(c.x));
		__m128 row3=_mm_loadu_ps(&(d.x));
		// now, matrix is:
		// x y z ?
        // x y z ?
        // x y z ?
        // x y z ?
		_MM_TRANSPOSE4_PS(row0,row1,row2,row3);
		x=row0;
		y=row1;
		z=row2;
	}

	/// LoadAndSwizzleAligned - load 4 Vectors into a FourVectors, perofrming transpose op.
	/// all 4 vectors muyst be 128 bit boundary
	inline void LoadAndSwizzleAligned(Vector const &a, Vector const &b, Vector const &c, Vector const &d)
	{
		__m128 row0=_mm_load_ps(&(a.x));
		__m128 row1=_mm_load_ps(&(b.x));
		__m128 row2=_mm_load_ps(&(c.x));
		__m128 row3=_mm_load_ps(&(d.x));
		// now, matrix is:
		// x y z ?
        // x y z ?
        // x y z ?
        // x y z ?
		_MM_TRANSPOSE4_PS(row0,row1,row2,row3);
		x=row0;
		y=row1;
		z=row2;
	}

	/// return the squared length of all 4 vectors
	inline __m128 length2(void) const
	{
		return (*this)*(*this);
	}

	/// return the approximate length of all 4 vectors. uses the sse sqrt approximation instr
	inline __m128 length(void) const
	{
		return _mm_sqrt_ps(length2());
	}

	/// normalize all 4 vectors in place. not mega-accurate (uses sse reciprocal approximation instr)
	inline void VectorNormalizeFast(void)
	{
		__m128 mag_sq=(*this)*(*this);						// length^2
		(*this) *= _mm_rsqrt_ps(mag_sq);					// *(1.0/sqrt(length^2))
	}

	/// normnalize all 4 vectors in place. uses newton iteration for higher precision results than
	/// from VectorNormalizeFast.
	inline void VectorNormalize(void)
	{
		static __m128 FourHalves={0.5,0.5,0.5,0.5};
		static __m128 FourThrees={3.,3.,3.,3.};
		__m128 mag_sq=(*this)*(*this);						// length^2
		__m128 guess=_mm_rsqrt_ps(mag_sq);
		// newton iteration for 1/sqrt(x) : y(n+1)=1/2 (y(n)*(3-x*y(n)^2));
        guess=_mm_mul_ps(guess,_mm_sub_ps(FourThrees,_mm_mul_ps(mag_sq,_mm_mul_ps(guess,guess))));
		guess=_mm_mul_ps(Four_PointFives,guess);
		(*this) *= guess;
	}

	/// construct a FourVectors from 4 separate Vectors
	inline FourVectors(Vector const &a, Vector const &b, Vector const &c, Vector const &d)
	{
		LoadAndSwizzle(a,b,c,d);
	}
	
};

/// form 4 cross products
inline FourVectors operator ^(const FourVectors &a, const FourVectors &b)
{
	FourVectors ret;
	ret.x=_mm_sub_ps(_mm_mul_ps(a.y,b.z),_mm_mul_ps(a.z,b.y));
	ret.y=_mm_sub_ps(_mm_mul_ps(a.z,b.x),_mm_mul_ps(a.x,b.z));
	ret.z=_mm_sub_ps(_mm_mul_ps(a.x,b.y),_mm_mul_ps(a.y,b.x));
	return ret;
}

/// component-by-componentwise MAX operator
inline FourVectors maximum(const FourVectors &a, const FourVectors &b)
{
	FourVectors ret;
	ret.x=_mm_max_ps(a.x,b.x);
	ret.y=_mm_max_ps(a.y,b.y);
	ret.z=_mm_max_ps(a.z,b.z);
	return ret;
}

/// component-by-componentwise MIN operator
inline FourVectors minimum(const FourVectors &a, const FourVectors &b)
{
	FourVectors ret;
	ret.x=_mm_min_ps(a.x,b.x);
	ret.y=_mm_min_ps(a.y,b.y);
	ret.z=_mm_min_ps(a.z,b.z);
	return ret;
}


/// check an m128 for all zeros. Not sure if this is the best way
inline bool IsAllZeros(__m128 var)
{
#ifdef _WIN32
	__declspec(align(16)) int32 myints[4];
#elif _LINUX
	int32 myints[4] __attribute__ ((aligned (16)));
#endif
	_mm_store_ps((float *) myints,var);
	return (myints[0]|myints[1]|myints[2]|myints[3])==0;
}

/// calculate the absolute value of a packed single
inline __m128 fabs(__m128 x)
{
#ifdef _WIN32
	static __declspec(align(16)) int32 clear_signmask[4]=
		{0x7fffffff,0x7fffffff,0x7fffffff,0x7fffffff};
#elif _LINUX
	static int32 clear_signmask[4]  __attribute__ ((aligned (16)))=
		{0x7fffffff,0x7fffffff,0x7fffffff,0x7fffffff};
#endif
	return _mm_and_ps(x,_mm_load_ps((float *) clear_signmask));
}

/// negate all four components of an sse packed single
inline __m128 fnegate(__m128 x)
{
#ifdef _WIN32
	static __declspec(align(16)) int32 signmask[4]=
		{0x80000000,0x80000000,0x80000000,0x80000000};
#elif _LINUX
	static int32 signmask[4]  __attribute__ ((aligned (16)))=
		{0x80000000,0x80000000,0x80000000,0x80000000};
#endif
	return _mm_xor_ps(x,_mm_load_ps((float *) signmask));
}

__m128 PowSSE_FixedPoint_Exponent(__m128 x, int exponent);

// PowSSE - raise an sse register to a power.  This is analogous to the C pow() function, with some
// restictions: fractional exponents are only handled with 2 bits of precision. Basically,
// fractions of 0,.25,.5, and .75 are handled. PowSSE(x,.30) will be the same as PowSSE(x,.25).
// negative and fractional powers are handled by the SSE reciprocal and square root approximation
// instructions and so are not especially accurate ----Note that this routine does not raise
// numeric exceptions because it uses SSE--- This routine is O(log2(exponent)).
inline __m128 PowSSE(__m128 x, float exponent)
{
	return PowSSE_FixedPoint_Exponent(x,(int) (4.0*exponent));
}

#define SSE1234(x) ((x).m128_f32[0]),((x).m128_f32[1]),((x).m128_f32[2]),((x).m128_f32[3])

#endif // WIN32





//#endif
