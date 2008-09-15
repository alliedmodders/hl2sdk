#ifndef HL2MP_GAMEINTERFACE_H
#define HL2MP_GAMEINTERFACE_H
#ifdef _WIN32
#pragma once
#endif

#include "utllinkedlist.h"


// These are created for map entities in order as the map entities are spawned.
class CMapEntityRef
{
public:
	int		m_iEdict;			// Which edict slot this entity got. -1 if CreateEntityByName failed.
	int		m_iSerialNumber;	// The edict serial number. TODO used anywhere ?
};

extern CUtlLinkedList<CMapEntityRef, unsigned short> g_MapEntityRefs;

#endif