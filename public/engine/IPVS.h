//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose:
//
//===========================================================================//

#ifndef IPVS_H
#define IPVS_H

#ifdef _WIN32
#pragma once
#endif

#include "mathlib/vector.h"
#include "tier0/basetypes.h"

class CFrustum;
class OBB_t;
struct AABB_t;

enum PVSCluster_t
{
	PVS_CLUSTER_VISIBLE_EVERYWHERE = 0,
};

abstract_class IPVS
{
public:
	virtual int GetClustersForOrigin( uint32 *pList, int nListMax, const Vector &vOrigin ) = 0;
	virtual int GetClustersForBounds( uint32 *pList, int nListMax, const Vector &vMins, const Vector &vMaxs, bool bIsStatic ) = 0;
	virtual int GetClustersForOrientedBounds( uint32 *pList, int nListMax, const OBB_t &bounds, bool bIsStatic ) = 0;
	virtual int GetClustersForFrustum( uint32 *pList, int nListMax, const CFrustum *pFrustum, bool bIsStatic ) = 0;
	virtual int GetVisibleEverywhereClusterList( uint32 *pList, int nListMax ) = 0;
	virtual int FilterClustersInRadius( uint32 *pList, int nList, const Vector &vOrigin, float flRadius ) = 0;
	virtual int GetClusterCount() = 0;
	virtual int GetAllClusterBounds( AABB_t *pBBoxList, int nMaxBBox ) = 0;
	virtual int GetClusterForPosition( const Vector &vPosition ) const = 0;
};

#endif // IPVS_H
