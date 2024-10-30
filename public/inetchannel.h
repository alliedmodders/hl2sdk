//========= Copyright 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef INETCHANNEL_H
#define INETCHANNEL_H
#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h"
#include "inetchannelinfo.h"
#include "steam/steamnetworkingtypes.h"
#include "tier1/bitbuf.h"
#include "tier1/netadr.h"
#include "tier1/utldelegate.h"
#include <eiface.h>

class	IDemoRecorderBase;
class	IInstantReplayIntercept;
class	CNetMessage;
class	INetChannelHandler;
class	INetChannel;
class	INetChannelInfo;
class	INetMessageBinder;
class	INetworkMessageProcessingPreFilter;
class	INetworkMessageInternal;
class	INetMessageDispatcher;
class	InstantReplayMessage_t;
class	CUtlSlot;

#ifndef NET_PACKET_ST_DEFINED
#define NET_PACKET_ST_DEFINED
struct NetPacket_t
{
	netadr_t		from;		// sender IP
	int				source;		// received source
	double			received;	// received time
	unsigned char	*data;		// pointer to raw packet data
	bf_read			message;	// easy bitbuf data access
	int				size;		// size in bytes
	int				wiresize;   // size in bytes before decompression
	bool			stream;		// was send as stream
	struct NetPacket_t *pNext;	// for internal use, should be NULL in public
};
#endif // NET_PACKET_ST_DEFINED

enum NetChannelBufType_t : int8
{
	BUF_DEFAULT = -1,
	BUF_UNRELIABLE = 0,
	BUF_RELIABLE,
	BUF_VOICE,
};

abstract_class INetworkMessageProcessingPreFilter
{
public:
	// Filter incoming messages from the netchan, return true to filter out (block) the further processing of the message
	virtual bool FilterMessage( INetworkMessageInternal *pNetMessage, const CNetMessage *pData, INetChannel *pChannel ) = 0;
};

abstract_class INetChannel : public INetChannelInfo
{
public:
	virtual	~INetChannel( void ) {};																	//31

	virtual void	Reset( void ) = 0;																	//32
	virtual void	Clear( void ) = 0;																	//33
	
	virtual void	Shutdown( ENetworkDisconnectionReason reason ) = 0;									//34
	
	virtual float	sub_180095190_GetUnknownFloat() const = 0;											//35
	
	virtual bool	SendNetMessage(void* messageHandle, google::protobuf::Message* message, NetChannelBufType_t type) = 0;//36
	virtual bool	SendData(bf_write& msg, NetChannelBufType_t bufferType) = 0;						//37
	
	virtual int		Transmit(const char* pDebugName, bf_write* data) = 0;								//38
	virtual void	SetBitsToSend() = 0;																//39
	virtual int		SendMessages(const char* pDebugName, bf_write* data) = 0;							//40
	virtual void	ClearBitsToSend() = 0;																//41

	virtual const netadr_t& GetRemoteAddress() = 0;														//42
	
	virtual void	UpdateMessageStats(int msggroup, int bits, bool) = 0;								//43
	
	virtual void	sub_18009DA80_Unknown(void) = 0;													//44
	
	virtual bool	CanPacket(void) const = 0;															//45
	virtual bool	IsOverflowed(void) const = 0;														//46
	virtual bool	HasPendingReliableData(void) = 0;													//47
	
	// For routing messages to a different handler
	virtual void	SetActiveChannel(INetChannel * pNewChannel) = 0;									//48

	virtual void	AttachSplitPlayer(int CSplitScreenSlot, INetChannel*) = 0;							//49
	virtual void	DetachSplitPlayer(int CSplitScreenSlot) = 0;										//50

	virtual void	Setup(void) = 0;																	//51
	virtual void	Setup2(void) = 0;																	//52

	virtual void	SetTimeout(float seconds, bool bForceExact = false) = 0;							//53
	virtual bool	IsTimedOut(void) const = 0;															//54
	virtual void	UpdateLastReceivedTime(void) = 0;													//55

	virtual float	sub_180096C00_GetUnknownFloat() const = 0;											//56
	//idk, can this be SetRemoteFramerate?
	virtual void	sub_180097DA0_SetUnknown(void *pUnknownStruct) = 0;									//57

	virtual bool	IsRemoteDisconnected(ENetworkDisconnectionReason& reason) const = 0;				//58

	virtual void	SetNetMessageDispatcher(INetMessageDispatcher* pDispatcher) = 0;					//59
	virtual INetMessageDispatcher* GetNetMessageDispatcher(void) const = 0;								//60

	virtual void	StartRegisteringMessageHandlers( void ) = 0;										//61
	virtual void	FinishRegisteringMessageHandlers( void ) = 0;										//62

	virtual void	RegisterNetMessageHandlerAbstract( CUtlSlot *nSlot, const CUtlAbstractDelegate &delegate, int nParamCount, INetworkMessageInternal *pNetMessage, int nPriority ) = 0;//63
	virtual void	UnregisterNetMessageHandlerAbstract( CUtlSlot *nSlot, const CUtlAbstractDelegate &delegate, INetworkMessageInternal *pNetMessage ) = 0;//64
	
	virtual int		GetNumBitsWritten( NetChannelBufType_t bufferType ) const = 0;						//65

	virtual void	SetDemoRecorder( IDemoRecorderBase *pDemoRecorder ) = 0;							//66
	virtual void	SetInstantReplayIntercept( IInstantReplayIntercept *pInstantReplayIntercept ) = 0;	//67
	
	virtual bool	IsNull( void ) const = 0;															//68
	
	virtual bool	ProcessDemoPacket( NetPacket_t *packet ) = 0;										//69

	virtual void	InstallMessageFilter( INetworkMessageProcessingPreFilter *pFilter ) = 0;			//70
	virtual void	UninstallMessageFilter( INetworkMessageProcessingPreFilter *pFilter ) = 0;			//71
	
	virtual void	PostReceivedNetMessage( INetworkMessageInternal *pNetMessage, const CNetMessage *pData, const NetChannelBufType_t *pBufType, int nBits, int nInSequenceNr ) = 0;//72
	
	virtual void	InsertReplayMessage( InstantReplayMessage_t &msg ) = 0;								//73
	
	virtual bool	HasQueuedNetMessages( unsigned short nMessageId ) const = 0;						//74
	
	virtual void	SetPendingDisconnect( ENetworkDisconnectionReason reason ) = 0;						//75
	virtual ENetworkDisconnectionReason GetPendingDisconnect( void ) const = 0;							//76
	
	virtual void	SuppressTransmit( bool suppress ) = 0;												//77
	virtual bool	IsSuppressingTransmit( void ) const = 0;											//78
	
	virtual EResult	SendRawMessage( const void *pData, uint32 cbData, int nSendFlags ) = 0;				//79
	
	virtual void	sub_18009CB70_SetUnknown(int a1) = 0;												//80
	virtual void	sub_18009DAE0_GetUnknown() = 0;														//81
	virtual void	sub_180097E50_SetUnknown(void *pUnknownStruct) = 0;									//82
};


#endif // INETCHANNEL_H
