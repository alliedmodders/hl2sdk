#ifndef GAMEEVENTSYSTEM_H
#define GAMEEVENTSYSTEM_H

#ifdef _WIN32
#pragma once
#endif

#include <eiface.h>

#include <tier1/utldelegate.h>

class IRecipientFilter;
class CUtlSlot;

namespace google {
	namespace protobuf {
		class Message;
	}
}

enum NetChannelBufType_t
{
};

struct GameEventHandle_t__
{
	void *m_pUnknown;
	const char* m_szMessageType;
	int m_iUnknown2;
	void *m_pUnknown3;
	void *m_pUnknown4;
	void *m_pUnknown5;
	unsigned short m_MessageID;
};

#define GAMEEVENTSYSTEM_INTERFACE_VERSION "GameEventSystemServerV001"

abstract_class IGameEventSystem : public IAppSystem
{
public:
	virtual void RegisterGameEvent( GameEventHandle_t__ *pEvent ) = 0;
	virtual void RegisterGameEventHandlerAbstract( CUtlSlot *slot, const CUtlAbstractDelegate &delegate,
		GameEventHandle_t__ *pEvent, int ) = 0;
	virtual void UnregisterGameEventHandlerAbstract( CUtlSlot *slot, const CUtlAbstractDelegate &delegate,
		GameEventHandle_t__ *pEvent ) = 0;
	virtual void PostEventAbstract_Local( CSplitScreenSlot slot, GameEventHandle_t__ *, const void *pData,
		unsigned long nSize ) = 0;
	virtual void PostEventAbstract( CSplitScreenSlot slot, bool bLocalOnly, int nClientCount, const unsigned char *clients,
		GameEventHandle_t__ *pEvent, const void *pData, unsigned long nSize, NetChannelBufType_t bufType ) = 0;
	virtual void PostEventAbstract( CSplitScreenSlot slot, bool bLocalOnly, IRecipientFilter *pFilter,
		GameEventHandle_t__ *pEvent, const void *pData, unsigned long nSize ) = 0;
	virtual void PostEntityEventAbstract( const CBaseHandle &hndl, GameEventHandle_t__ *pEvent, const void *pData,
		unsigned long nSize, NetChannelBufType_t bufType ) = 0;
	virtual void ProcessQueuedEvents( void ) = 0;
	virtual int GetEventSource( void ) const = 0;
	virtual void PurgeQueuedEvents( void ) = 0;
};


#endif // GAMEEVENTSYSTEM_H