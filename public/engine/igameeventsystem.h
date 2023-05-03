#ifndef GAMEEVENTSYSTEM_H
#define GAMEEVENTSYSTEM_H

#ifdef _WIN32
#pragma once
#endif

#include <eiface.h>

#include <networksystem/inetworkserializer.h>
#include <tier1/utldelegate.h>
#include <tier1/utlsymbol.h>
#include <irecipientfilter.h>
#include <inetchannel.h>

class CUtlSlot;

#define GAMEEVENTSYSTEM_INTERFACE_VERSION "GameEventSystemServerV001"

abstract_class IGameEventSystem : public IAppSystem
{
public:
	virtual void RegisterGameEvent( INetworkSerializable *pEvent ) = 0;

	virtual void RegisterGameEventHandlerAbstract( CUtlSlot *nSlot, const CUtlAbstractDelegate &delegate, INetworkSerializable *pEvent ) = 0;
	virtual void UnregisterGameEventHandlerAbstract( CUtlSlot *nSlot, const CUtlAbstractDelegate &delegate, INetworkSerializable *pEvent ) = 0;

	// Providing nSize has no effect and is unused.
	virtual void PostEventAbstract_Local( CSplitScreenSlot nSlot, INetworkSerializable *pEvent, const void *pData, unsigned long nSize ) = 0;

	// Providing nSlot as -1 would select 0nth slot.
	// clients pointer is a masked uint64 value where (client index - 1) is mapped to each bit.
	// Providing nClientCount as -1 and clients pointer as NULL would post event to all available clients.
	// Providing nSize has no effect and is unused.
	virtual void PostEventAbstract( CSplitScreenSlot nSlot, bool bLocalOnly, int nClientCount, const uint64 *clients,
		INetworkSerializable *pEvent, const void *pData, unsigned long nSize, NetChannelBufType_t bufType ) = 0;
	virtual void PostEventAbstract( CSplitScreenSlot nSlot, bool bLocalOnly, IRecipientFilter *pFilter,
		INetworkSerializable *pEvent, const void *pData, unsigned long nSize ) = 0;

	// Posts the event to all clients, even tho the function name tells otherwise
	// Providing nSize has no effect and is unused.
	virtual void PostEntityEventAbstract( const CBaseHandle &hndl, INetworkSerializable *pEvent, const void *pData, unsigned long nSize, NetChannelBufType_t bufType ) = 0;

	virtual void ProcessQueuedEvents() = 0;
	virtual int GetEventSource() const = 0;
	virtual void PurgeQueuedEvents() = 0;
};

#endif // GAMEEVENTSYSTEM_H