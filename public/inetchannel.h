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
#include "tier1/bitbuf.h"
#include "tier1/netadr.h"
#include "tier1/utldelegate.h"
#include <eiface.h>

class	IDemoRecorderBase;
class IInstantReplayIntercept;
class	IInstantReplayIntercept;
class	INetMessage;
class	INetChannelHandler;
class	INetChannelInfo;
class	INetMessageBinder;
class	INetworkMessageProcessingPreFilter;
class	INetMessageDispatcher;
class	InstantReplayMessage_t;
class	CUtlSlot;

DECLARE_HANDLE_32BIT(CSplitScreenPlayerSlot);
DECLARE_POINTER_HANDLE(NetMessageHandle_t);


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

enum NetChannelBufType_t
{
	BUF_RELIABLE = 0,
	BUF_UNRELIABLE,
	BUF_VOICE,
};

abstract_class INetChannel : public INetChannelInfo
{
public:
	virtual	~INetChannel( void ) {};

	virtual size_t 	GetTotalPacketBytes( int, int ) const = 0;
	virtual size_t 	GetTotalPacketReliableBytes( int, int ) const = 0;
	virtual void	SetTimeout(float seconds, bool bForceExact = false) = 0;
	
	virtual void	Reset( void ) = 0;
	virtual void	Clear( void ) = 0;
	virtual void	Shutdown(/* ENetworkDisconnectionReason */ int reason) = 0;
	
	virtual bool	SendData( bf_write &msg, NetChannelBufType_t bufferType ) = 0;
	virtual void	SetChoked( void ) = 0;
	virtual bool	Transmit( const char *, bf_write * ) = 0;

	virtual const netadr_t	&GetRemoteAddress( void ) const = 0;
	virtual int				GetDropNumber( void ) const = 0;
		
	virtual void	UpdateMessageStats( int msggroup, int bits, bool ) = 0;
	virtual bool	CanPacket( void ) const = 0;
	virtual bool	IsOverflowed( void ) const = 0;
	virtual bool	HasPendingReliableData( void ) = 0;

	virtual void	SetMaxBufferSize( NetChannelBufType_t bufferType, int nBytes ) = 0;

	// For routing messages to a different handler
	virtual bool	SetActiveChannel( INetChannel *pNewChannel ) = 0;
	virtual void	AttachSplitPlayer( CSplitScreenPlayerSlot nSplitPlayerSlot, INetChannel *pChannel ) = 0;
	virtual void	DetachSplitPlayer( CSplitScreenPlayerSlot nSplitPlayerSlot ) = 0;

	virtual void	SetUsesMaxRoutablePlayload(bool useMax) = 0;
	virtual void	SetDataRate(float rate) = 0;
	virtual void	SetUpdateRate( int rate ) = 0;
	virtual void	SetCommandRate( int rate ) = 0;
	
	virtual bool	IsTimedOut( void ) const  = 0;
	
	virtual void	SetRemoteFramerate( float flFrameTime, float flFrameTimeStdDeviation, float flFrameStartTimeStdDeviation ) = 0;
	virtual bool	IsRemoteDisconnected() const = 0;
	
	virtual void	SetNetMessageDispatcher( INetMessageDispatcher *pDispatcher ) = 0;
	virtual INetMessageDispatcher *GetNetMessageDispatcher( void ) const = 0;
	virtual void	SendNetMessage( NetMessageHandle_t msg, const void *pData, NetChannelBufType_t bufType ) = 0;
	virtual void	StartRegisteringMessageHandlers( void ) = 0;
	virtual void	FinishRegisteringMessageHandlers( void ) = 0;
	virtual void	RegisterNetMessageHandlerAbstract( CUtlSlot *, const CUtlAbstractDelegate &, int, NetMessageHandle_t , int ) = 0;
	virtual void	UnregisterNetMessageHandlerAbstract( CUtlSlot *, const CUtlAbstractDelegate &, NetMessageHandle_t ) = 0;
	virtual void	SetChallengeNr( unsigned int challenge ) = 0;
	virtual void			GetSequenceData( int &nOutSequenceNr, int &nInSequenceNr, int &nOutSequenceNrAck ) = 0;
	virtual void			SetSequenceData( int nOutSequenceNr, int nInSequenceNr, int nOutSequenceNrAck ) = 0;
	virtual int		GetNumBitsWritten(NetChannelBufType_t bufferType) const = 0;
	virtual void	SetCompressionMode( bool ) = 0;
	// Max # of payload bytes before we must split/fragment the packet
	virtual void	SetMaxRoutablePayloadSize( int nSplitSize ) = 0;
	virtual int		GetMaxRoutablePayloadSize() = 0;
	virtual void	ProcessPacket( bf_read &packet, bool bHasHeader ) = 0;
	virtual void	SetDemoRecorder( IDemoRecorderBase * ) = 0;
	virtual void	SetInstantReplayIntercept( IInstantReplayIntercept * ) = 0;
	virtual void	SetInterpolationAmount( float flInterpolationAmount ) = 0;
	virtual void	SetFileTransmissionMode( bool ) = 0;
	virtual bool	IsNull() const = 0;
	virtual bool	ProcessDemoPacket( NetPacket_t* packet ) = 0;
	virtual bool	WasLastMessageReliable() const = 0;
	virtual void	InstallMessageFilter( INetworkMessageProcessingPreFilter * ) = 0;
	virtual void	UninstallMessageFilter( INetworkMessageProcessingPreFilter * ) = 0;
	virtual void	PostReceivedNetMessage(NetMessageHandle_t, const void *, const NetChannelBufType_t* ) = 0;
	virtual bool	InsertReplayMessage( InstantReplayMessage_t &msg ) = 0;
	virtual bool	HasQueuedPackets( void ) const = 0;
  
	virtual void	SetPendingDisconnect( /*ENetworkDisconnectReason*/ int reason ) = 0;
	virtual int		GetPendingDisconnect() const = 0;
	
	virtual bool	IsSuppressingTransmit() const = 0;
	virtual void	SuppressTransmit( bool ) = 0;
};


#endif // INETCHANNEL_H
