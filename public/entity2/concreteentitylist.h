#ifndef CONCRETEENTITYLIST_H
#define CONCRETEENTITYLIST_H

#include "entityidentity.h"

class CConcreteEntityList
{
	struct CList
	{
		CEntityIdentity* m_pHead;
		CEntityIdentity* m_pTail;
		uint64_t unknown;
	};
public:
	CEntityIdentities *m_pIdentityChunks[MAX_ENTITY_LISTS];
	uint8_t unknown[16];
	CEntityIdentity* m_pFirstActiveEntity; // 528
	CConcreteEntityList::CList m_usedList;
	CConcreteEntityList::CList m_dormantList;
	CConcreteEntityList::CList m_freeNetworkableList;
	CConcreteEntityList::CList m_freeNonNetworkableList;
	uint32_t m_nNetworkableEntityLimit; // 0x268
	uint32_t m_nNonNetworkableEntityLimit; // 0x26c
	uint32_t m_nMaxPlayers;
	CBitVec<16384> m_PVSBits;
};

#endif // CONCRETEENTITYLIST_H