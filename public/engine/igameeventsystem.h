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
	virtual void RegisterGameEvent(INetworkSerializable *pEvent ) = 0;

	virtual void RegisterGameEventHandlerAbstract( CUtlSlot *nSlot, const CUtlAbstractDelegate &delegate, INetworkSerializable *pEvent, int ) = 0;
	virtual void UnregisterGameEventHandlerAbstract( CUtlSlot *nSlot, const CUtlAbstractDelegate &delegate, INetworkSerializable *pEvent ) = 0;

	virtual void PostEventAbstract_Local( CSplitScreenSlot nSlot, INetworkSerializable *, const void *pData, unsigned long nSize ) = 0;

	virtual void PostEventAbstract( CSplitScreenSlot nSlot, bool bLocalOnly, int nClientCount, const unsigned char *clients,
		INetworkSerializable *pEvent, const void *pData, unsigned long nSize, NetChannelBufType_t bufType ) = 0;
	virtual void PostEventAbstract( CSplitScreenSlot nSlot, bool bLocalOnly, IRecipientFilter *pFilter,
		INetworkSerializable *pEvent, const void *pData, unsigned long nSize ) = 0;

	virtual void PostEntityEventAbstract( const CBaseHandle &hndl, INetworkSerializable *pEvent, const void *pData, unsigned long nSize, NetChannelBufType_t bufType ) = 0;

	virtual void ProcessQueuedEvents() = 0;
	virtual int GetEventSource() const = 0;
	virtual void PurgeQueuedEvents() = 0;
};

#endif // GAMEEVENTSYSTEM_H