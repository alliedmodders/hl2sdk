#include "const.h"
#include "entity2/entitysystem.h"

CBaseEntity* CEntitySystem::GetBaseEntity(CEntityIndex entnum)
{
	if (entnum.Get() <= -1 || entnum.Get() >= (MAX_TOTAL_ENTITIES - 1))
		return nullptr;

	CEntityIdentity* pChunkToUse = m_EntityList.m_pIdentityChunks[entnum.Get() / MAX_ENTITIES_IN_LIST];
	if (!pChunkToUse)
		return nullptr;

	CEntityIdentity* pIdentity = &pChunkToUse[entnum.Get() % MAX_ENTITIES_IN_LIST];
	if (!pIdentity)
		return nullptr;

	if (pIdentity->GetEntityIndex() != entnum)
		return nullptr;

	return static_cast<CBaseEntity*>(pIdentity->m_pInstance);
}

CBaseEntity* CEntitySystem::GetBaseEntity(const CEntityHandle& hEnt)
{
	if (!hEnt.IsValid())
		return nullptr;

	CEntityIdentity* pChunkToUse = m_EntityList.m_pIdentityChunks[hEnt.GetEntryIndex() / MAX_ENTITIES_IN_LIST];
	if (!pChunkToUse)
		return nullptr;

	CEntityIdentity* pIdentity = &pChunkToUse[hEnt.GetEntryIndex() % MAX_ENTITIES_IN_LIST];
	if (!pIdentity)
		return nullptr;

	if (pIdentity->GetRefEHandle() != hEnt)
		return nullptr;

	return static_cast<CBaseEntity*>(pIdentity->m_pInstance);
}

void CGameEntitySystem::AddListenerEntity(IEntityListener* pListener)
{
	if (m_entityListeners.Find(pListener) == -1)
	{
		m_entityListeners.AddToTail(pListener);
	}
}

void CGameEntitySystem::RemoveListenerEntity(IEntityListener* pListener)
{
	m_entityListeners.FindAndRemove(pListener);
}
