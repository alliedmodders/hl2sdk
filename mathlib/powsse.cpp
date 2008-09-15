// $Id$

#include "mathlib/ssemath.h"

__m128 PowSSE_FixedPoint_Exponent(__m128 x, int exponent)
{
	__m128 rslt=Four_Ones;									// x^0=1.0
	int xp=abs(exponent);
	if (xp & 3)												// fraction present?
	{
		__m128 sq_rt=_mm_sqrt_ps(x);
		if (xp & 1)											// .25?
			rslt=_mm_sqrt_ps(sq_rt);						// x^.25
		if (xp & 2)
			rslt=_mm_mul_ps(rslt,sq_rt);
	}
	xp>>=2;													// strip fraction
	__m128 curpower=x;										// curpower iterates through  x,x^2,x^4,x^8,x^16...

	while(1)
	{
		if (xp & 1)
			rslt=_mm_mul_ps(rslt,curpower);
		xp>>=1;
		if (xp)
			curpower=_mm_mul_ps(curpower,curpower);
		else
			break;
	}
	if (exponent<0)
		return _mm_rcp_ps(rslt);							// pow(x,-b)=1/pow(x,b)
	else
		return rslt;
}

