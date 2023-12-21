#include "const.h"
#include "entity2/entitysystem.h"

CEntityIdentity* CEntitySystem::GetEntityIdentity(CEntityIndex entnum)
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

	return pIdentity;
}

CEntityIdentity* CEntitySystem::GetEntityIdentity(const CEntityHandle& hEnt)
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

	return pIdentity;
}

CEntityHandle CEntitySystem::FindFirstEntityHandleByName(const char* szName, WorldGroupId_t hWorldGroupId)
{
	if (!szName || !*szName)
		return CEntityHandle();

	EntityInstanceByNameIter_t iter(szName);
	iter.SetWorldGroupId(hWorldGroupId);

	CEntityInstance* pEntity = iter.First();
	if (!pEntity)
		return CEntityHandle();

	return pEntity->GetRefEHandle();
}

CUtlSymbolLarge CEntitySystem::AllocPooledString(const char* pString)
{
	if (!pString || !*pString)
		return CUtlSymbolLarge();

	return m_Symbols.AddString(pString);
}

CUtlSymbolLarge CEntitySystem::FindPooledString(const char* pString)
{
	if (!pString || !*pString)
		return CUtlSymbolLarge();

	return m_Symbols.Find(pString);
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

EntityInstanceIter_t::EntityInstanceIter_t(IEntityFindFilter* pFilter, EntityIterType_t eIterType)
{
	m_pCurrentEnt = nullptr;
	m_pFilter = pFilter;
	m_eIterType = eIterType;
	m_hWorldGroupId = WorldGroupId_t();
}

CEntityInstance* EntityInstanceIter_t::First()
{
	m_pCurrentEnt = nullptr;
	return Next();
}

CEntityInstance* EntityInstanceIter_t::Next()
{
	if (m_pCurrentEnt)
		m_pCurrentEnt = m_pCurrentEnt->m_pNext;
	else
		m_pCurrentEnt = (m_eIterType == ENTITY_ITER_OVER_ACTIVE) ? GameEntitySystem()->m_EntityList.m_pFirstActiveEntity : GameEntitySystem()->m_EntityList.m_dormantList.m_pHead;

	for (; m_pCurrentEnt != nullptr; m_pCurrentEnt = m_pCurrentEnt->m_pNext)
	{
		if ((m_pCurrentEnt->m_flags & EF_MARKED_FOR_DELETE) != 0)
			continue;

		if (m_pFilter && !m_pFilter->ShouldFindEntity(m_pCurrentEnt->m_pInstance))
			continue;

		if (m_hWorldGroupId != WorldGroupId_t() && m_hWorldGroupId != m_pCurrentEnt->m_worldGroupId)
			continue;

		break;
	}

	if (m_pCurrentEnt)
		return m_pCurrentEnt->m_pInstance;

	return nullptr;
}

EntityInstanceByNameIter_t::EntityInstanceByNameIter_t(const char* szName, CEntityInstance* pSearchingEntity, CEntityInstance* pActivator, CEntityInstance* pCaller, IEntityFindFilter* pFilter, EntityIterType_t eIterType)
{
	m_pCurrentEnt = nullptr;
	m_pFilter = pFilter;
	m_eIterType = eIterType;
	m_hWorldGroupId = WorldGroupId_t();

	if (szName[0] == '!')
	{
		m_pszEntityName = nullptr;
		m_pEntityHandles = nullptr;
		m_nCurEntHandle = 0;
		m_nNumEntHandles = 0;
		m_pProceduralEnt = GameEntitySystem()->FindEntityProcedural(szName, pSearchingEntity, pActivator, pCaller);
	}
	else if (strchr(szName, '*') || eIterType == ENTITY_ITER_OVER_DORMANTS)
	{
		m_pszEntityName = szName;
		m_pEntityHandles = nullptr;
		m_nCurEntHandle = 0;
		m_nNumEntHandles = 0;
		m_pProceduralEnt = nullptr;
	}
	else
	{
		m_pszEntityName = nullptr;
		m_pProceduralEnt = nullptr;

		CUtlSymbolLarge nameSymbol = GameEntitySystem()->FindPooledString(szName);

		unsigned short idx = GameEntitySystem()->m_entityNames.Find(nameSymbol);
		if (idx == GameEntitySystem()->m_entityNames.InvalidIndex())
		{
			m_pEntityHandles = nullptr;
			m_nCurEntHandle = 0;
			m_nNumEntHandles = 0;
		}
		else
		{
			m_pEntityHandles = GameEntitySystem()->m_entityNames[ idx ];
			m_nCurEntHandle = 0;

			if (m_pEntityHandles)
			{
				m_nNumEntHandles = m_pEntityHandles->Count();

				if (!m_nNumEntHandles)
					m_pEntityHandles = nullptr;
			}
			else
			{
				m_nNumEntHandles = 0;
			}
		}
	}
}

CEntityInstance* EntityInstanceByNameIter_t::First()
{
	m_pCurrentEnt = nullptr;
	return Next();
}

CEntityInstance* EntityInstanceByNameIter_t::Next()
{
	if (m_pProceduralEnt)
	{
		if (m_pCurrentEnt)
		{
			m_pCurrentEnt = nullptr;
		}
		else
		{
			if (!m_pFilter || m_pFilter->ShouldFindEntity(m_pProceduralEnt))
				m_pCurrentEnt = m_pProceduralEnt->m_pEntity;
		}
	}
	else if (m_pEntityHandles)
	{
		if ( !m_pCurrentEnt )
			m_nCurEntHandle = m_nNumEntHandles;

		for (--m_nCurEntHandle; m_nCurEntHandle >= 0; --m_nCurEntHandle)
		{
			m_pCurrentEnt = GameEntitySystem()->GetEntityIdentity(m_pEntityHandles->Element(m_nCurEntHandle));

			if ((m_pCurrentEnt->m_flags & EF_MARKED_FOR_DELETE) != 0)
				continue;

			if (m_pFilter && !m_pFilter->ShouldFindEntity(m_pCurrentEnt->m_pInstance))
				continue;

			if (m_hWorldGroupId != WorldGroupId_t() && m_hWorldGroupId != m_pCurrentEnt->m_worldGroupId)
				continue;

			break;
		}

		if (m_nCurEntHandle < 0)
			m_pCurrentEnt = nullptr;
	}
	else if (m_pszEntityName)
	{
		if (m_pCurrentEnt)
			m_pCurrentEnt = m_pCurrentEnt->m_pNext;
		else
			m_pCurrentEnt = (m_eIterType == ENTITY_ITER_OVER_ACTIVE) ? GameEntitySystem()->m_EntityList.m_pFirstActiveEntity : GameEntitySystem()->m_EntityList.m_dormantList.m_pHead;

		for (; m_pCurrentEnt != nullptr; m_pCurrentEnt = m_pCurrentEnt->m_pNext)
		{
			if ((m_pCurrentEnt->m_flags & EF_MARKED_FOR_DELETE) != 0)
				continue;

			if (!m_pCurrentEnt->m_name.IsValid())
				continue;

			if (!m_pCurrentEnt->NameMatches(m_pszEntityName))
				continue;

			if (m_pFilter && !m_pFilter->ShouldFindEntity(m_pCurrentEnt->m_pInstance))
				continue;

			if (m_hWorldGroupId != WorldGroupId_t() && m_hWorldGroupId != m_pCurrentEnt->m_worldGroupId)
				continue;

			break;
		}
	}

	if (m_pCurrentEnt)
		return m_pCurrentEnt->m_pInstance;

	return nullptr;
}
