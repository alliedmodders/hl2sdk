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
	virtual	~INetChannel( void ) {};

	virtual void	Reset( void ) = 0;
	virtual void	Clear( void ) = 0;
	virtual void	Shutdown( ENetworkDisconnectionReason reason ) = 0;
	
	virtual HSteamNetConnection GetSteamNetConnection( void ) const = 0;
	
	virtual bool	SendNetMessage( const CNetMessage *pData, NetChannelBufType_t bufType ) = 0;
	virtual bool	SendData( bf_write &msg, NetChannelBufType_t bufferType ) = 0;
	virtual int		Transmit( const char *pDebugName, bf_write *data ) = 0;
	virtual void	SetBitsToSend( void ) = 0;
	virtual int		SendMessages( const char *pDebugName, bf_write *data ) = 0;
	virtual void	ClearBitsToSend( void ) = 0;

	virtual const netadr_t &GetRemoteAddress( void ) const = 0;

	virtual void	UpdateMessageStats( int msggroup, int bits, bool ) = 0;
	
	virtual void	unk001( void ) = 0;
	
	virtual bool	CanPacket( void ) const = 0;
	virtual bool	IsOverflowed( void ) const = 0;
	virtual bool	HasPendingReliableData( void ) = 0;

	// For routing messages to a different handler
	virtual bool	SetActiveChannel( INetChannel *pNewChannel ) = 0;
	
	virtual void	AttachSplitPlayer( CSplitScreenSlot nSlot, INetChannel *pChannel ) = 0;
	virtual void	DetachSplitPlayer( CSplitScreenSlot nSlot ) = 0;

	virtual void	SetMinDataRate( int rate ) = 0;
	virtual void	SetMaxDataRate( int rate ) = 0;

	virtual void	SetTimeout( float seconds, bool bForceExact = false ) = 0;
	virtual bool	IsTimedOut( void ) const = 0;
	virtual void	UpdateLastReceivedTime( void ) = 0;

	virtual void	SetRemoteFramerate( float flFrameTime, float flFrameTimeStdDeviation, float flFrameStartTimeStdDeviation, float flLoss, float flUnfilteredFrameTime ) = 0;					
	virtual bool	IsRemoteDisconnected( ENetworkDisconnectionReason &reason ) const = 0;

	virtual void	SetNetMessageDispatcher( INetMessageDispatcher *pDispatcher ) = 0;
	virtual INetMessageDispatcher *GetNetMessageDispatcher( void ) const = 0;
	
	virtual void	StartRegisteringMessageHandlers( void ) = 0;
	virtual void	FinishRegisteringMessageHandlers( void ) = 0;
	
	virtual void	RegisterNetMessageHandlerAbstract( CUtlSlot *nSlot, const CUtlAbstractDelegate &delegate, int nParamCount, INetworkMessageInternal *pNetMessage, int nPriority ) = 0;
	virtual void	UnregisterNetMessageHandlerAbstract( CUtlSlot *nSlot, const CUtlAbstractDelegate &delegate, INetworkMessageInternal *pNetMessage ) = 0;
	
	virtual void	SetChallengeNr( unsigned int challenge ) = 0;
	virtual int		GetNumBitsWritten( NetChannelBufType_t bufferType ) const = 0;
	virtual void	SetDemoRecorder( IDemoRecorderBase *pDemoRecorder ) = 0;
	virtual void	SetInstantReplayIntercept( IInstantReplayIntercept *pInstantReplayIntercept ) = 0;
	virtual bool	IsNull( void ) const = 0;
	virtual bool	ProcessDemoPacket( NetPacket_t *packet ) = 0;
	virtual bool	WasLastMessageReliable( void ) const = 0;
	
	virtual void	InstallMessageFilter( INetworkMessageProcessingPreFilter *pFilter ) = 0;
	virtual void	UninstallMessageFilter( INetworkMessageProcessingPreFilter *pFilter ) = 0;
	
	virtual void	PostReceivedNetMessage( INetworkMessageInternal *pNetMessage, const CNetMessage *pData, const NetChannelBufType_t *pBufType, int nBits, int nInSequenceNr ) = 0;
	virtual void	InsertReplayMessage( InstantReplayMessage_t &msg ) = 0;
	virtual bool	HasQueuedNetMessages( int nMessageId ) const = 0;

	virtual void	SetPendingDisconnect( ENetworkDisconnectionReason reason ) = 0;
	virtual ENetworkDisconnectionReason GetPendingDisconnect( void ) const = 0;

	virtual void	SuppressTransmit( bool suppress ) = 0;
	virtual bool	IsSuppressingTransmit( void ) const = 0;
	
	virtual EResult	SendRawMessage( const void *pData, uint32 cbData, int nSendFlags ) = 0;
	
	virtual int		GetCurrentNetMessageBits( void ) const = 0;
	virtual int		GetCurrentNetMessageInSequenceNr( void ) const = 0;

	virtual void	unk101( void ) = 0;
	virtual void	unk102( void ) = 0;
};


#endif // INETCHANNEL_H
