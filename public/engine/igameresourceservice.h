#ifndef IGAMERESOURCESERVICE_H
#define IGAMERESOURCESERVICE_H

#ifdef _WIN32
#pragma once
#endif

#include "eiface.h"
#include "engine/IEngineService.h"
#include "entity2/entityidentity.h"

struct EntitySpawnInfo_t;
class matrix3x4a_t;

abstract_class IGameResourceService : public IEngineService
{
public:
	virtual ~IGameResourceService() = 0;

	virtual void unk001() = 0;
	virtual void unk002() = 0;
	virtual void unk003() = 0;
	virtual void unk004() = 0;
	virtual void unk005() = 0;
	virtual void unk006() = 0;
	virtual void unk007() = 0;
	virtual void unk008() = 0;
	virtual void unk009() = 0;
	virtual void unk010() = 0;
	virtual void unk011() = 0;

	virtual void PrecacheEntitiesAndConfirmResourcesAreLoaded( SpawnGroupHandle_t hSpawnGroup, int nCount, const EntitySpawnInfo_t *pEntities, const matrix3x4a_t *pWorldOffset ) = 0;
};

#endif // IGAMERESOURCESERVICE_H
