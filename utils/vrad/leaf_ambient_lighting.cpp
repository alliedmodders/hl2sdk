//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//
#ifdef _WIN32
#include <windows.h>
#endif
#include "vrad.h"
#include "leaf_ambient_lighting.h"
#include "bsplib.h"
#include "vraddetailprops.h"
#include "anorms.h"
#include "pacifier.h"


static TableVector g_BoxDirections[6] = 
{
	{  1,  0,  0 }, 
	{ -1,  0,  0 },
	{  0,  1,  0 }, 
	{  0, -1,  0 }, 
	{  0,  0,  1 }, 
	{  0,  0, -1 }, 
};



static void ComputeAmbientFromSurface( dface_t *surfID, dworldlight_t* pSkylight, 
									   Vector& radcolor )
{
	if ( !surfID )
		return;

	texinfo_t *pTexInfo = &texinfo[surfID->texinfo];

	// If we hit the sky, use the sky ambient
	if ( pTexInfo->flags & SURF_SKY )
	{
		if ( pSkylight )
		{
			// add in sky ambient
			VectorCopy( pSkylight->intensity, radcolor );
		}
	}
	else
	{
		Vector reflectivity = dtexdata[pTexInfo->texdata].reflectivity;
		VectorMultiply( radcolor, reflectivity, radcolor );
	}
}


// TODO: it's CRAZY how much lighting code we share with the engine. It should all be shared code.
float Engine_WorldLightAngle( const dworldlight_t *wl, const Vector& lnormal, const Vector& snormal, const Vector& delta )
{
	float dot, dot2;

	Assert( wl->type == emit_surface );

	dot = DotProduct( snormal, delta );
	if (dot < 0)
		return 0;

	dot2 = -DotProduct (delta, lnormal);
	if (dot2 <= ON_EPSILON/10)
		return 0; // behind light surface

	return dot * dot2;
}


// TODO: it's CRAZY how much lighting code we share with the engine. It should all be shared code.
float Engine_WorldLightDistanceFalloff( const dworldlight_t *wl, const Vector& delta )
{
	Assert( wl->type == emit_surface );

	// Cull out stuff that's too far
	if (wl->radius != 0)
	{
		if ( DotProduct( delta, delta ) > (wl->radius * wl->radius))
			return 0.0f;
	}

	return InvRSquared(delta);
}


void AddEmitSurfaceLights( const Vector &vStart, Vector lightBoxColor[6] )
{
	int iThread = 0;

	for ( int iLight=0; iLight < *pNumworldlights; iLight++ )
	{
		dworldlight_t *wl = &dworldlights[iLight];

		// Should this light even go in the ambient cubes?
		if ( !( wl->flags & DWL_FLAGS_INAMBIENTCUBE ) )
			continue;

		Assert( wl->type == emit_surface );

		// Can this light see the point?
		if ( TestLine( vStart, wl->origin, 0, iThread ) != CONTENTS_EMPTY )
			continue;

		// Add this light's contribution.
		Vector vDelta = wl->origin - vStart;
		float flDistanceScale = Engine_WorldLightDistanceFalloff( wl, vDelta );

		Vector vDeltaNorm = vDelta;
		VectorNormalize( vDeltaNorm );
		float flAngleScale = Engine_WorldLightAngle( wl, wl->normal, vDeltaNorm, vDeltaNorm );

		float ratio = flDistanceScale * flAngleScale;
		if ( ratio == 0 )
			continue;

		for ( int i=0; i < 6; i++ )
		{
			float t = DotProduct( g_BoxDirections[i], vDeltaNorm );
			if ( t > 0 )
			{
				lightBoxColor[i] += wl->intensity * (t * ratio);
			}
		}
	}	
}


void ComputeAmbientFromSphericalSamples( const Vector &vStart, Vector lightBoxColor[6] )
{
	// Figure out the color that rays hit when shot out from this position.
	Vector radcolor[NUMVERTEXNORMALS];
	for ( int i = 0; i < NUMVERTEXNORMALS; i++ )
	{
		Vector vEnd = vStart + g_anorms[i] * (COORD_EXTENT * 1.74);

		// Now that we've got a ray, see what surface we've hit
		Vector lightStyleColors[MAX_LIGHTSTYLES];
		lightStyleColors[0].Init();	// We only care about light style 0 here.
		Vector colorSum;
		CalcRayAmbientLighting( vStart, vEnd, lightStyleColors, colorSum );
	
		radcolor[i] = lightStyleColors[0];
	}

	// accumulate samples into radiant box
	for ( int j = 6; --j >= 0; )
	{
		float t = 0;

		lightBoxColor[j].Init();

		for (int i = 0; i < NUMVERTEXNORMALS; i++)
		{
			float c = DotProduct( g_anorms[i], g_BoxDirections[j] );
			if (c > 0)
			{
				t += c;
				lightBoxColor[j] += radcolor[i] * c;
			}
		}
		
		lightBoxColor[j] *= 1/t;
	}

	// Now add direct light from the emit_surface lights. These go in the ambient cube because
	// there are a ton of them and they are often so dim that they get filtered out by r_worldlightmin.
	AddEmitSurfaceLights( vStart, lightBoxColor );
}


bool IsLeafAmbientSurfaceLight( dworldlight_t *wl )
{
	static const float g_flWorldLightMinEmitSurface = 0.005f;
	static const float g_flWorldLightMinEmitSurfaceDistanceRatio = ( InvRSquared( Vector( 0, 0, 512 ) ) );

	if ( wl->type != emit_surface )
		return false;

	if ( wl->style != 0 )
		return false;

	float intensity = max( wl->intensity[0], wl->intensity[1] );
	intensity = max( intensity, wl->intensity[2] );
	
	return (intensity * g_flWorldLightMinEmitSurfaceDistanceRatio) < g_flWorldLightMinEmitSurface;
}


// There are problems with water lighting that make bobbing objects look bad.  the same problems
// show up for other poorly sampled areas of the world, but floating in water makes it look the
// worst because of oscillating back and forth and because water always acts as a bsp splitting
// plane.  The hack turned on if this #define is set makes all per leaf ambient for water exactly
// match the per leaf above the water, and does solve the specific problems seen with d1_canals_11.
// It was decided to punt it for hl2 because of effecting the underwater lighting of every object
// in all levels at the last minute. This solution isn't good anyway - what is needed is either
// some sort of adaptive sampling strategy, or distance-based interpolation or some other sort of
// smoothly spacially varying lighting.
#define MAKE_LIGHT_BELOW_WATER_MATCH_LIGHT_ABOVE_WATER 0

void ComputePerLeafAmbientLighting()
{
	// Figure out which lights should go in the per-leaf ambient cubes.
	int nInAmbientCube = 0;
	int nSurfaceLights = 0;
	for ( int i=0; i < *pNumworldlights; i++ )
	{
		dworldlight_t *wl = &dworldlights[i];
		
		if ( IsLeafAmbientSurfaceLight( wl ) )
			wl->flags |= DWL_FLAGS_INAMBIENTCUBE;
		else
			wl->flags &= ~DWL_FLAGS_INAMBIENTCUBE;
	
		if ( wl->type == emit_surface )
			++nSurfaceLights;

		if ( wl->flags & DWL_FLAGS_INAMBIENTCUBE )
			++nInAmbientCube;
	}

	Msg( "%d of %d (%d%% of) surface lights went in leaf ambient cubes.\n", nInAmbientCube, nSurfaceLights, nSurfaceLights ? ((nInAmbientCube*100) / nSurfaceLights) : 0 );

	g_pLeafAmbientLighting->SetCount( numleafs );

	StartPacifier( "ComputePerLeafAmbientLighting: " );
	
	for ( int leafID = 0; leafID < numleafs; leafID++ )
	{
		dleaf_t *pLeaf = &dleafs[leafID];

		Vector cube[6];
		Vector center = ( Vector( pLeaf->mins[0], pLeaf->mins[1], pLeaf->mins[2] ) + Vector( pLeaf->maxs[0], pLeaf->maxs[1], pLeaf->maxs[2] ) ) * 0.5f;
#if MAKE_LIGHT_BELOW_WATER_MATCH_LIGHT_ABOVE_WATER
		if (pLeaf->contents & CONTENTS_WATER)
		{
			center.z=pLeaf->maxs[2]+1;
			int above_leaf=PointLeafnum( center);
			dleaf_t *pLeaf = &dleafs[above_leaf];
			
			center = ( Vector( pLeaf->mins[0], pLeaf->mins[1], pLeaf->mins[2] ) + Vector( pLeaf->maxs[0], pLeaf->maxs[1], pLeaf->maxs[2] ) ) * 0.5f;
		}
#endif
		
		ComputeAmbientFromSphericalSamples( center, cube );
		for ( int i = 0; i < 6; i++ )
		{
			VectorToColorRGBExp32( cube[i], (*g_pLeafAmbientLighting)[leafID].m_Color[i] );
		}

		UpdatePacifier( (float)leafID / numleafs );
	}

	EndPacifier( true );
}



