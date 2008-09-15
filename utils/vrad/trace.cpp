//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// trace.c

//=============================================================================
#ifdef _WIN32
#include <windows.h>
#endif

#include "vrad.h"
#include "trace.h"
#include "Cmodel.h"

//=============================================================================

//
// Trace Nodes
//
typedef struct tnode_s
{
	int		type;
	Vector	normal;
	float	dist;
	int		children[2];
	int		pad;
} tnode_t;

tnode_t		*tnodes, *tnode_p;

class CToolTrace : public CBaseTrace
{
public:
	CToolTrace() {}

	Vector		mins;
	Vector		maxs;
	Vector		extents;

	texinfo_t	*surface;

	qboolean	ispoint;

private:
	 CToolTrace( const CToolTrace& );
};


// 1/32 epsilon to keep floating point happy
#define	DIST_EPSILON	(0.03125)

// JAYHL2: This used to be -1, but that caused lots of epsilon issues
// around slow sloping planes.  Perhaps Quake2 limited maps to a certain
// slope / angle on walkable ground.  It has to be a negative number
// so that the tests work out.
#define		NEVER_UPDATED		-9999

//=============================================================================

bool DM_RayDispIntersectTest( CVRADDispColl *pTree, Vector& rayStart, Vector& rayEnd, CToolTrace *pTrace );
void DM_ClipBoxToBrush( CToolTrace *trace, const Vector & mins, const Vector & maxs, const Vector& p1, const Vector& p2, dbrush_t *brush );

//=============================================================================

/*
==============
MakeTnode

Converts the disk node structure into the efficient tracing structure
==============
*/
// UNDONE: Detect SKY surfaces somehow?
void MakeTnode (int nodenum)
{
	tnode_t			*t;
	dplane_t		*plane;
	int				i;
	dnode_t 		*node;
	
	t = tnode_p++;

	node = dnodes + nodenum;
	plane = dplanes + node->planenum;

	t->type = plane->type;
	VectorCopy (plane->normal, t->normal);
	t->dist = plane->dist;
	
	for (i=0 ; i<2 ; i++)
	{
		if (node->children[i] < 0)
		{
            // set child node value -- this is needed for displacement collisions and detail faces
			// this could be made more efficient with a more complete tree
            t->children[i] = node->children[i];
		}
		else
		{
			t->children[i] = tnode_p - tnodes;
			MakeTnode (node->children[i]);
		}
	}			
}


/*
=============
MakeTnodes

Loads the node structure out of a .bsp file to be used for light occlusion
=============
*/
void MakeTnodes (dmodel_t *bm)
{
	// 32 byte align the structs
	tnodes = ( tnode_t* )calloc( 1, (numnodes+1) * sizeof(tnode_t));
	tnodes = (tnode_t *)(((int)tnodes + 31)&~31);
	tnode_p = tnodes;

	MakeTnode (0);
}


//==========================================================


// UNDONE: Should return CONTENTS_SKY or some sky marker
// when sky surfaces are hit
int TestLine_r (int node, const Vector& start, const Vector& stop, Ray_t& ray, PropTested_t& propTested,
				DispTested_t &dispTested )
{
	tnode_t	*tnode;
	float	front, back;
	Vector	mid;
	float	frac;
	int		side;
	int		r;

    if( node < 0 )
    {
        int leafNumber = -node - 1;
        dleaf_t *pLeaf = dleafs + leafNumber;
        
        if( pLeaf->contents & MASK_OPAQUE )
            return 1;

		//
		// check solids in each leaf
		//
		if ( pLeaf->numleafbrushes )
		{
			//
			// set the initial trace state
			//
			CToolTrace trace;
			memset( &trace, 0, sizeof(trace) );
			trace.ispoint = true;
			trace.startsolid = false;
			trace.fraction = 1.0;

			for ( int i = 0; i < pLeaf->numleafbrushes; i++ )
			{
				int brushnum = dleafbrushes[pLeaf->firstleafbrush+i];
				dbrush_t *b = &dbrushes[brushnum];
				if ( !(b->contents & MASK_OPAQUE))
					continue;

				Vector zeroExtents = vec3_origin;
				DM_ClipBoxToBrush( &trace, zeroExtents, zeroExtents, start, stop, b);
				if ( trace.fraction != 1.0 || trace.startsolid )
					return b->contents;
			}
		}
        
		// Try displacement surfaces
		if( StaticDispMgr()->ClipRayToDispInLeaf( dispTested, ray, leafNumber ) )
			return CONTENTS_SOLID;

		// Try static props...
		if ( ! g_bStaticPropPolys )
			if (StaticPropMgr()->ClipRayToStaticPropsInLeaf( propTested, ray, leafNumber ))
				return CONTENTS_SOLID;

        // no occlusion
        return 0;
    }

	tnode = &tnodes[node];
	if ( tnode->type <= PLANE_Z )
	{
		front = start[tnode->type] - tnode->dist;
		back = stop[tnode->type] - tnode->dist;
	}
	else
	{
		front = (start[0]*tnode->normal[0] + start[1]*tnode->normal[1] + start[2]*tnode->normal[2]) - tnode->dist;
		back = (stop[0]*tnode->normal[0] + stop[1]*tnode->normal[1] + stop[2]*tnode->normal[2]) - tnode->dist;
	}

	if (front >= -ON_VIS_EPSILON && back >= -ON_VIS_EPSILON)
		return TestLine_r (tnode->children[0], start, stop, ray, propTested, dispTested);
	
	if (front < ON_VIS_EPSILON && back < ON_VIS_EPSILON)
		return TestLine_r (tnode->children[1], start, stop, ray, propTested, dispTested);

	side = front < 0;
	
	frac = front / (front-back);

	mid[0] = start[0] + (stop[0] - start[0])*frac;
	mid[1] = start[1] + (stop[1] - start[1])*frac;
	mid[2] = start[2] + (stop[2] - start[2])*frac;

	r = TestLine_r (tnode->children[side], start, mid, ray, propTested, dispTested);
	if (r)
		return r;
	return TestLine_r (tnode->children[!side], mid, stop, ray, propTested, dispTested);
}

PropTested_t s_PropTested[MAX_TOOL_THREADS+1];
DispTested_t s_DispTested[MAX_TOOL_THREADS+1];

int TestLine (const Vector& start, const Vector& stop, int node, int iThread, 
			  int static_prop_index_to_ignore )
{
	// Compute a bitfield, one per prop and disp...
	StaticPropMgr()->StartRayTest( s_PropTested[iThread] );
	StaticDispMgr()->StartRayTest( s_DispTested[iThread] );
	Ray_t ray;
	ray.Init( start, stop, vec3_origin, vec3_origin );
	int hit=TestLine_r( node, start, stop, ray, s_PropTested[iThread], s_DispTested[iThread] );
	if (hit == 0)
	{
		// check against our triangle soup list
		FourRays myrays;
		myrays.origin.DuplicateVector(start); //Vector(80,-179,65)); //start);
		myrays.direction.DuplicateVector(stop); //Vector(80,-179,681)); //stop);
		myrays.direction-=myrays.origin;
		__m128 len=myrays.direction.length();
		myrays.direction *= MMReciprocal( len );
		RayTracingResult rt_result;
		g_RtEnv.Trace4Rays(myrays, Four_Zeros, len, &rt_result, static_prop_index_to_ignore );
		if ( (rt_result.HitIds[0] != -1) &&
			 (rt_result.HitDistance.m128_f32[0] < len.m128_f32[0] ) )
		{
			return CONTENTS_SOLID;
		}
		else
			return 0;		
	}
	return hit;
}


/*
================
DM_ClipBoxToBrush
================
*/
void DM_ClipBoxToBrush( CToolTrace *trace, const Vector& mins, const Vector& maxs, const Vector& p1, const Vector& p2,
						dbrush_t *brush)
{
	dplane_t	*plane, *clipplane;
	float		dist;
	Vector		ofs;
	float		d1, d2;
	float		f;
	dbrushside_t	*side, *leadside;

	if (!brush->numsides)
		return;

	float enterfrac = NEVER_UPDATED;
	float leavefrac = 1.f;
	clipplane = NULL;

	bool getout = false;
	bool startout = false;
	leadside = NULL;

	// Loop interchanged, so we don't have to check trace->ispoint every side.
	if ( !trace->ispoint )
	{
		for (int i=0 ; i<brush->numsides ; ++i)
		{
			side = &dbrushsides[brush->firstside+i];
			plane = dplanes + side->planenum;

			// FIXME: special case for axial

			// general box case
			// push the plane out apropriately for mins/maxs

			// FIXME: use signbits into 8 way lookup for each mins/maxs
			ofs.x = (plane->normal.x < 0) ? maxs.x : mins.x;
			ofs.y = (plane->normal.y < 0) ? maxs.y : mins.y;
			ofs.z = (plane->normal.z < 0) ? maxs.z : mins.z;
//			for (j=0 ; j<3 ; j++)
//			{
				// Set signmask to either 0 if the sign is negative, or 0xFFFFFFFF is the sign is positive:
				//int signmask = (((*(int *)&(plane->normal[j]))&0x80000000) >> 31) - 1;

				//float temp = maxs[j];
				//*(int *)&(ofs[j]) =    (~signmask) & (*(int *)&temp);
				//float temp1 = mins[j];
				//*(int *)&(ofs[j]) |=   (signmask) & (*(int *)&temp1);
//			}
			dist = DotProduct (ofs, plane->normal);
			dist = plane->dist - dist;

			d1 = DotProduct (p1, plane->normal) - dist;
			d2 = DotProduct (p2, plane->normal) - dist;

			// if completely in front of face, no intersection
			if (d1 > 0 && d2 > 0)
				return;

			if (d2 > 0)
				getout = true;	// endpoint is not in solid
			if (d1 > 0)
				startout = true;

			if (d1 <= 0 && d2 <= 0)
				continue;

			// crosses face
			if (d1 > d2)
			{	// enter
				f = (d1-DIST_EPSILON) / (d1-d2);
				if (f > enterfrac)
				{
					enterfrac = f;
					clipplane = plane;
					leadside = side;
				}
			}
			else
			{	// leave
				f = (d1+DIST_EPSILON) / (d1-d2);
				if (f < leavefrac)
					leavefrac = f;
			}
		}
	}
	else
	{
		for (int i=0 ; i<brush->numsides ; ++i)
		{
			side = &dbrushsides[brush->firstside+i];
			plane = dplanes + side->planenum;

			// FIXME: special case for axial

			// special point case
			// don't ray trace against bevel planes
			if( side->bevel == 1 )
				continue;

			dist = plane->dist;
			d1 = DotProduct (p1, plane->normal) - dist;
			d2 = DotProduct (p2, plane->normal) - dist;

			// if completely in front of face, no intersection
			if (d1 > 0 && d2 > 0)
				return;

			if (d2 > 0)
				getout = true;	// endpoint is not in solid
			if (d1 > 0)
				startout = true;

			if (d1 <= 0 && d2 <= 0)
				continue;

			// crosses face
			if (d1 > d2)
			{	// enter
				f = (d1-DIST_EPSILON) / (d1-d2);
				if (f > enterfrac)
				{
					enterfrac = f;
					clipplane = plane;
					leadside = side;
				}
			}
			else
			{	// leave
				f = (d1+DIST_EPSILON) / (d1-d2);
				if (f < leavefrac)
					leavefrac = f;
			}
		}
	}



	if (!startout)
	{	// original point was inside brush
		trace->startsolid = true;
		if (!getout)
			trace->allsolid = true;
		return;
	}
	if (enterfrac < leavefrac)
	{
		if (enterfrac > NEVER_UPDATED && enterfrac < trace->fraction)
		{
			if (enterfrac < 0)
				enterfrac = 0;
			trace->fraction = enterfrac;
			trace->plane.dist = clipplane->dist;
			trace->plane.normal = clipplane->normal;
			trace->plane.type = clipplane->type;
			if (leadside->texinfo!=-1)
				trace->surface = &texinfo[leadside->texinfo];
			else
				trace->surface = 0;
			trace->contents = brush->contents;
		}
	}
}

/*
================
DM_TraceToLeaf
================
*/
void DM_TraceToLeaf (CToolTrace *trace, int leafnum)
{
	int			k;
	int			brushnum;
	dleaf_t		*leaf;
	dbrush_t	*b;

	leaf = &dleafs[leafnum];

	// trace line against all brushes in the leaf
	for (k=0 ; k<leaf->numleafbrushes ; k++)
	{
		brushnum = dleafbrushes[leaf->firstleafbrush+k];
		b = &dbrushes[brushnum];
		if ( !(b->contents & trace->contents))
			continue;
		DM_ClipBoxToBrush (trace, trace->mins, trace->maxs, trace->startpos, trace->endpos, b);
		if (!trace->fraction)
			return;
	}
}


/*
==================
DM_RecursiveHullCheck

==================
*/

void DM_RecursiveHullCheck( CToolTrace *trace, int num, float p1f, float p2f, const Vector& p1, const Vector& p2 )
{
	dnode_t		*node = NULL;
	dplane_t	*plane;
	float		t1 = 0, t2 = 0, offset = 0;
	float		frac, frac2;
	float		idist;
	Vector		mid;
	int			side;
	float		midf;

	if (trace->fraction <= p1f)
		return;		// already hit something nearer

	// While loop here is to avoid recursion overhead
	while( num >= 0)
	{
		//
		// find the point distances to the seperating plane
		// and the offset for the size of the box
		//
		node = dnodes + num;
		plane = dplanes + node->planenum;

		if (plane->type < 3)
		{
			t1 = p1[plane->type] - plane->dist;
			t2 = p2[plane->type] - plane->dist;
			offset = trace->extents[plane->type];
		}
		else
		{
			t1 = DotProduct (plane->normal, p1) - plane->dist;
			t2 = DotProduct (plane->normal, p2) - plane->dist;
			if (trace->ispoint)
			{
				offset = 0;
			}
			else
			{
				offset = fabs(trace->extents[0]*plane->normal[0]) +
					fabs(trace->extents[1]*plane->normal[1]) +
					fabs(trace->extents[2]*plane->normal[2]);
			}
		}

		// see which sides we need to consider
		if (t1 >= offset && t2 >= offset)
		{
			num = node->children[0];
		}
		else if (t1 < -offset && t2 < -offset)
		{
			num = node->children[1];
		}
		else
		{
			// In this case, we have to split
			break;
		}
	}

	// if < 0, we are in a leaf node
	if (num < 0)
	{
		DM_TraceToLeaf (trace, -1-num);
		return;
	}

	// put the crosspoint DIST_EPSILON pixels on the near side
	if (t1 < t2)
	{
		idist = 1.0/(t1-t2);
		side = 1;
		frac2 = (t1 + offset + DIST_EPSILON)*idist;
		frac = (t1 - offset + DIST_EPSILON)*idist;
	}
	else if (t1 > t2)
	{
		idist = 1.0/(t1-t2);
		side = 0;
		frac2 = (t1 - offset - DIST_EPSILON)*idist;
		frac = (t1 + offset + DIST_EPSILON)*idist;
	}
	else
	{
		side = 0;
		frac = 1;
		frac2 = 0;
	}

	// move up to the node
	frac = clamp( frac, 0, 1 );
	midf = p1f + (p2f - p1f)*frac;
	VectorLerp( p1, p2, frac, mid );

	DM_RecursiveHullCheck (trace, node->children[side], p1f, midf, p1, mid);

	// go past the node
	frac2 = clamp( frac2, 0, 1 );		
	midf = p1f + (p2f - p1f)*frac2;
	VectorLerp( p1, p2, frac2, mid );

	DM_RecursiveHullCheck (trace, node->children[side^1], midf, p2f, mid, p2);
}

texinfo_t *TestLine_Surface( int node, const Vector& start, const Vector& stop, int iThread, 
							 bool canRecurse, int static_prop_index_to_ignore )
{
	Assert( start.IsValid() && stop.IsValid() );

	CToolTrace trace;
	// fill in a default trace
	memset (&trace, 0, sizeof(trace));
	trace.fraction = 1;
	trace.surface = NULL;

	if (!numnodes)	// map not loaded
		return NULL;

	trace.contents = MASK_OPAQUE;
	VectorCopy (start, trace.startpos);
	VectorCopy (stop, trace.endpos);
	VectorCopy (vec3_origin, trace.mins);
	VectorCopy (vec3_origin, trace.maxs);

	trace.ispoint = true;
	VectorClear (trace.extents);

	//
	// general sweeping through world
	//
 	DM_RecursiveHullCheck( &trace, node, 0, 1, start, stop );

	if ( trace.startsolid )
		return 0;

	FourRays myrays;
	myrays.origin.DuplicateVector(start);
	myrays.direction.DuplicateVector(stop);
	myrays.direction-=myrays.origin;
	__m128 len=myrays.direction.length();
	myrays.direction *= MMReciprocal( len );
	RayTracingResult rt_result;
	g_RtEnv.Trace4Rays(myrays, Four_Zeros, len, &rt_result, static_prop_index_to_ignore );
	if ( (rt_result.HitIds[0] != -1) &&
		 (rt_result.HitDistance.m128_f32[0] < len.m128_f32[0] ) )
		return 0;

	// Now clip the ray to the displacement surfaces
	Vector end;
	VectorSubtract( stop, start, end );
	VectorMA( start, trace.fraction, end, end );

	Ray_t ray;
	ray.Init( start, end );
	if ( StaticDispMgr()->ClipRayToDisp( s_DispTested[iThread], ray ) )
		return 0;

 	// Now clip the ray to the static props
	if ( ( trace.fraction != 1.0 ) && (! g_bStaticPropPolys) )
	{
		Vector end;
		VectorSubtract( stop, start, end );
		VectorMA( start, trace.fraction, end, end );

		Ray_t ray;
		ray.Init( start, end, vec3_origin, vec3_origin );
		if ( StaticPropMgr()->ClipRayToStaticProps( s_PropTested[iThread], ray ))
			return 0;
	}
	
	// if we hit sky, and we're not in a sky camera's area, try clipping into the 3D sky boxes
	if (canRecurse && (trace.fraction != 1.0) && (trace.surface) && (trace.surface->flags & SURF_SKY))
	{
		Vector dir = stop-start;
		VectorNormalize(dir);

		int leafIndex = PointLeafnum(start);
		if ( leafIndex >= 0 )
		{
			int area = dleafs[leafIndex].area;
			if (area >= 0 && area < numareas)
			{
				if (area_sky_cameras[area] < 0)
				{
					int cam;
					for (cam = 0; cam < num_sky_cameras; ++cam)
					{
						Vector skystart, skystop;
						VectorMA( sky_cameras[cam].origin, sky_cameras[cam].world_to_sky, start, skystart );
						skystop = skystart + dir*MAX_TRACE_LENGTH;
						texinfo_t *skycamsurf = TestLine_Surface( 0, skystart, skystop, iThread, false );
						if (!skycamsurf || !(skycamsurf->flags & SURF_SKY))
						{
							return skycamsurf;
						}
					}
				}
			}
		}
	}

	return trace.surface;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int PointLeafnum_r( const Vector &point, int ndxNode )
{
	// while loop here is to avoid recursion overhead
	while( ndxNode >= 0 )
	{
		dnode_t *pNode = dnodes + ndxNode;
		dplane_t *pPlane = dplanes + pNode->planenum;

		float dist;
		if( pPlane->type < 3 )
		{
			dist = point[pPlane->type] - pPlane->dist;
		}
		else
		{
			dist = DotProduct( pPlane->normal, point ) - pPlane->dist;
		}

		if( dist < 0.0f )
		{
			ndxNode = pNode->children[1];
		}
		else
		{
			ndxNode = pNode->children[0];
		}
	}

	return ( -1 - ndxNode );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int PointLeafnum( const Vector &point )
{
	return PointLeafnum_r( point, 0 );
}
