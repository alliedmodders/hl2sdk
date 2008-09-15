// $Id$

#include <ssemath.h>
#include <lightdesc.h>

void LightDesc_t::RecalculateDerivedValues(void)
{
	m_Flags=0;
	if (m_Attenuation0)
		m_Flags|=LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION0;
	if (m_Attenuation1)
		m_Flags|=LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION1;
	if (m_Attenuation2)
		m_Flags|=LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION2;
	
	if (m_Type==MATERIAL_LIGHT_SPOT)
	{
		m_ThetaDot=cos(m_Theta);
		m_PhiDot=cos(m_Phi);
		float spread=m_ThetaDot-m_PhiDot;
		if (spread>1.0e-10)
		{
			// note - this quantity is very sensitive to round off error. the sse
			// reciprocal approximation won't cut it here.
			OneOver_ThetaDot_Minus_PhiDot=1.0/spread;
		}
		else
		{
			// hard falloff instead of divide by zero
			OneOver_ThetaDot_Minus_PhiDot=1.0;
		}				
	}	
	m_RangeSquared=m_Range*m_Range;

}

void LightDesc_t::ComputeLightAtPoints( const FourVectors &pos, const FourVectors &normal,
										FourVectors &color, bool DoHalfLambert ) const
{
	FourVectors delta;
	Assert((m_Type==MATERIAL_LIGHT_POINT) || (m_Type==MATERIAL_LIGHT_SPOT) || (m_Type==MATERIAL_LIGHT_DIRECTIONAL));
	switch (m_Type)
	{
		case MATERIAL_LIGHT_POINT:
		case MATERIAL_LIGHT_SPOT:
			delta.DuplicateVector(m_Position);
			delta-=pos;
			break;
				
		case MATERIAL_LIGHT_DIRECTIONAL:
			delta.DuplicateVector(m_Direction);
			delta*=-1.0;
			break;
	}

	__m128 dist2 = delta*delta;

	__m128 falloff;

	if( m_Flags & LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION0 )
	{
		falloff = MMReplicate(m_Attenuation0);
	}
	else
		falloff= Four_Epsilons;

	if( m_Flags & LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION1 )
	{
		falloff=_mm_add_ps(falloff,_mm_mul_ps(MMReplicate(m_Attenuation1),_mm_sqrt_ps(dist2)));
	}

	if( m_Flags & LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION2 )
	{
		falloff=_mm_add_ps(falloff,_mm_mul_ps(MMReplicate(m_Attenuation2),dist2));
	}

	falloff=_mm_rcp_ps(falloff);
	// Cull out light beyond this radius
	// now, zero out elements for which dist2 was > range^2. !!speed!! lights should store dist^2 in sse format
	if (m_Range != 0.f)
	{
		__m128 RangeSquared=MMReplicate(m_RangeSquared); // !!speed!!
		falloff=_mm_and_ps(falloff,_mm_cmplt_ps(dist2,RangeSquared));
	}

	delta.VectorNormalizeFast();
	__m128 strength=delta*normal;
	if (DoHalfLambert)
	{
		strength=_mm_add_ps(_mm_mul_ps(strength,Four_PointFives),Four_PointFives);
	}
	else
		strength=_mm_max_ps(Four_Zeros,delta*normal);
		
	switch(m_Type)
	{
		case MATERIAL_LIGHT_POINT:
			// half-lambert
			break;
				
		case MATERIAL_LIGHT_SPOT:
		{
			__m128 dot2=_mm_sub_ps(Four_Zeros,delta*m_Direction); // dot position with spot light dir for cone falloff


			__m128 cone_falloff_scale=_mm_mul_ps(MMReplicate(OneOver_ThetaDot_Minus_PhiDot),
												 _mm_sub_ps(dot2,MMReplicate(m_PhiDot)));
			cone_falloff_scale=_mm_min_ps(cone_falloff_scale,Four_Ones);
			
			if ((m_Falloff!=0.0) && (m_Falloff!=1.0))
			{
				// !!speed!! could compute integer exponent needed by powsse and store in light
				cone_falloff_scale=PowSSE(cone_falloff_scale,m_Falloff);
			}
			strength=_mm_mul_ps(cone_falloff_scale,strength);

			// now, zero out lighting where dot2<phidot. This will mask out any invalid results
			// from pow function, etc
			__m128 OutsideMask=_mm_cmpgt_ps(dot2,MMReplicate(m_PhiDot)); // outside light cone?
			strength=_mm_and_ps(OutsideMask,strength);
		}
		break;
			
		case MATERIAL_LIGHT_DIRECTIONAL:
			break;

	}
	strength=_mm_mul_ps(strength,falloff);
	color.x=_mm_add_ps(color.x,_mm_mul_ps(strength,MMReplicate(m_Color.x)));
	color.y=_mm_add_ps(color.y,_mm_mul_ps(strength,MMReplicate(m_Color.y)));
	color.z=_mm_add_ps(color.z,_mm_mul_ps(strength,MMReplicate(m_Color.z)));
}
