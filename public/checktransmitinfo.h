#ifndef CHECKTRANSMITINFO_H
#define CHECKTRANSMITINFO_H
#ifdef _WIN32
#pragma once
#endif

#include "bitvec.h"
#include "const.h"
#include "entity2/entityidentity.h"
#include "playerslot.h"
#include "tier1/utlvector.h"

// Entities can span this many clusters before we revert to a slower area checking algorithm
#define	MAX_FAST_ENT_CLUSTERS	4
#define	MAX_ENT_CLUSTERS	64
#define MAX_WORLD_AREAS		8

struct vis_info_t
{
	uint32 m_uVisBitsBufSize;
	SpawnGroupHandle_t m_SpawnGroupHandle;
	CBitVec<4096> m_VisBits;
};

class CCheckTransmitInfo
{
public:
	CBitVec<MAX_EDICTS> *m_pTransmitEntity;
	CBitVec<MAX_EDICTS> *m_pTransmitNonPlayers;
	CBitVec<MAX_EDICTS> *m_pTransmitOutOfPVS;
	CBitVec<MAX_EDICTS> *m_pTransmitAlways;
	CUtlVector<CPlayerSlot> m_vecTargetSlots;
	vis_info_t m_VisInfo;
	CPlayerSlot m_nPlayerSlot;
	bool m_bFullUpdate;
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
