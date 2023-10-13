#ifndef EVENTDISPATCHER_H
#define EVENTDISPATCHER_H

#ifdef _WIN32
#pragma once
#endif

#include <tier1/utldelegate.h>
#include <tier1/utlmap.h>

class ISchemaBinding;

struct CEventDispatcher_Base
{
	struct EventListenerInfo_t
	{
		CUtlAbstractDelegate m_Delegate;
		const char *m_pszName;
		int32 m_nPriority;
		uint8 m_nDelegateParamCount;
		bool m_bDelegateReturnsVoid;
	};

	struct DelegateIterator_Base_t
	{
		const CUtlVector< EventListenerInfo_t > *pListeners;
		CUtlVectorFixedGrowable< int, 4 > skipListeners;
		int nCurrent;
		DelegateIterator_Base_t *pNext;
		bool bIteratingForward;
		bool bIsInListenerTelemetryScope;
	};

	CThreadFastMutex m_Lock;
	DelegateIterator_Base_t *m_pActiveIterators;
};

struct CEventID_SchemaBinding
{
	int8 unused;
};

struct CEventIDManager_SchemaBinding : CEventID_SchemaBinding
{
};

struct CEventIDManager_Default : CEventIDManager_SchemaBinding
{
};

template <typename T>
struct CEventDispatcher_Identified : CEventDispatcher_Base
{
	CUtlMap< const ISchemaBinding*, CCopyableUtlVector<CEventDispatcher_Base::EventListenerInfo_t>, unsigned int, bool (*)(const ISchemaBinding* const&, const ISchemaBinding* const&)> m_EventListenerMap;
};

template <typename T>
struct CEventDispatcher : CEventDispatcher_Identified<T>
{
};

#endif // EVENTDISPATCHER_H