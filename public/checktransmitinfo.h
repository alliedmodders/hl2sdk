#ifndef CHECKTRANSMITINFO_H
#define CHECKTRANSMITINFO_H
#ifdef _WIN32
#pragma once
#endif

#include "bitvec.h"
#include "const.h"

// Entities can span this many clusters before we revert to a slower area checking algorithm
#define	MAX_FAST_ENT_CLUSTERS	4
#define	MAX_ENT_CLUSTERS	64
#define MAX_WORLD_AREAS		8

class CCheckTransmitInfo
{
public:
	CBitVec<MAX_EDICTS>	*m_pTransmitEntity;	// entity n is already marked for transmission
	CBitVec<MAX_EDICTS>	*m_pTransmitAlways; // entity n is always send even if not in PVS (HLTV and Replay only)

	// AMNOTE: This is incomplete and may require further reversing in the future.
};

//-----------------------------------------------------------------------------
// Stores information necessary to perform PVS testing.
//-----------------------------------------------------------------------------
struct PVSInfo_t
{
	// headnode for the entity's bounding box
	short		m_nHeadNode;			

	// number of clusters or -1 if too many
	short		m_nClusterCount;		

	// cluster indices
	unsigned short *m_pClusters;	

	// For dynamic "area portals"
	short		m_nAreaNum;
	short		m_nAreaNum2;

	// current position
	float		m_vCenter[3];

private:
	unsigned short m_pClustersInline[MAX_FAST_ENT_CLUSTERS];
};


#endif // CHECKTRANSMITINFO_H
