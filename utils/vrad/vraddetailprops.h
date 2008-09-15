//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef VRADDETAILPROPS_H
#define VRADDETAILPROPS_H
#ifdef _WIN32
#pragma once
#endif


#include "bspfile.h"
#include "anorms.h"


// Calculate the lighting at whatever surface the ray hits.
// Note: this ADDS to the values already in color. So if you want absolute
// values in there, then clear the values in color[] first.
void CalcRayAmbientLighting(
	const Vector &vStart,
	const Vector &vEnd,
	Vector color[MAX_LIGHTSTYLES],	// The color contribution from each lightstyle.
	Vector &colorSum				// The contribution from each lightstyle, summed up.
	);

void ComputeDetailPropLighting( int iThread );


#endif // VRADDETAILPROPS_H
