//===== Copyright (c) 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose:
//
//===========================================================================//

#ifndef INETWORKSYSTEM_H
#define INETWORKSYSTEM_H
#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h"
#include "appframework/IAppSystem.h"
#include "inetchannel.h"
#include "tier1/bitbuf.h"

class IConnectionlessPacketHandler;
class INetworkConfigChanged;
class INetworkPacketFilter;
class INetworkFileDownloadFilter;
class INetworkFileSendCompleted;
class INetworkPrepareStartupParams;

// Reverse engineered interface: return types may be wrong

abstract_class INetworkSystem : public IAppSystem
{
public:
	virtual void SetDedicated( bool enable ) = 0;
	virtual void SetMultiplayer( bool enable ) = 0;
	virtual int CreateSocket( int, int, int, const char * ) = 0;
	virtual void ForceOpenSocket( int sock ) = 0;
	virtual void AddExtraSocket( int, const char * ) = 0;
	virtual void RemoveAllExtraSockets() = 0;
	virtual void EnableLoopbackBetweenSockets( int, int ) = 0;
	virtual void SetDefaultBroadcastPort( int port ) = 0;
	virtual void RunFrame( double ) = 0;
	virtual void SendQueuedPackets() = 0;
	virtual void SleepUntilMessages( int, int ) = 0;
	virtual void InitPostFork() = 0;
	virtual void SetSubProcess( bool ) = 0;
	virtual void SendPacket( INetChannel *netchan, int, const netadr_t &adr, const unsigned char *, int, bf_write *, bool, unsigned int ) = 0;
	virtual void ProcessSocket( int sock, IConnectionlessPacketHandler *handler ) = 0;
	virtual INetChannel *CreateNetChannel( int, netadr_t *adr ,char const *, INetChannelHandler *handler, bool ) = 0;
	virtual void RemoveNetChannel( INetChannel *netchan, bool ) = 0;
	virtual void ListenSocket( int sock, bool ) = 0;
	virtual void ConnectSocket( int sock , const netadr_t &adr ) = 0;
	virtual void CloseSocket( int sock ) = 0;
	virtual void OutOfBandPrintf( int sock, const netadr_t &adr, const char *format, ...) = 0;
	virtual void OutOfBandDelayedPrintf( int sock, const netadr_t &adr, unsigned int delay, const char *format, ...) = 0;
	virtual void SetTime( double time ) = 0;
	virtual void SetTimeScale( float timeScale ) = 0;
	virtual double GetNetTime() = 0;
	virtual bool IsMultiplayer() = 0;
	virtual bool IsDedicated() = 0;
	virtual bool IsDedicatedForXbox() = 0;
	virtual void LogBadPacket( netpacket_t * ) = 0;
	virtual void DescribeSocket( int sock ) = 0;
	virtual void BufferToBufferCompress( char *, unsigned int *, const char *, unsigned int ) = 0;
	virtual void BufferToBufferDecompress( char *, unsigned int *, char const *, unsigned int ) = 0;
	virtual netadr_t& GetPublicAdr() = 0;
	virtual netadr_t& GetLocalAdr() = 0;
	virtual float GetFakeLag() = 0;
	virtual int GetUDPPort( int ) = 0;
	virtual bool IsSafeFileToDownload( const char *file ) = 0;
	virtual bool IsValidFileTransferExtension( const char *extension ) = 0;
	virtual bool CanRedownloadFile( const char *file ) = 0;
	virtual void AddNetworkConfigChangedCallback( INetworkConfigChanged *callback ) = 0;
	virtual void RemoveNetworkConfigChangedCallback( INetworkConfigChanged *callback ) = 0;
	virtual void AddNetworkPacketFilterCallback( INetworkPacketFilter *callback ) = 0;
	virtual void RemoveNetworkPacketFilterCallback( INetworkPacketFilter *callback ) = 0;
	virtual void AddNetworkFileDownloadFilter( INetworkFileDownloadFilter *callback ) = 0;
	virtual void RemoveNetworkFileDownloadFilter( INetworkFileDownloadFilter *callback ) = 0;
	virtual void AddNetworkFileSendCompletedCallback( INetworkFileSendCompleted *callback ) = 0;
	virtual void RemoveNetworkFileSendCompletedCallback( INetworkFileSendCompleted *callback ) = 0;
	virtual void AddNetworkPrepareStartupParamsCallback( INetworkPrepareStartupParams *callback ) = 0;
	virtual void RemoveNetworkPrepareStartupParamsCallback( INetworkPrepareStartupParams *callback ) = 0;
	virtual void CloseAllSockets() = 0;
};

DECLARE_TIER2_INTERFACE( INetworkSystem, g_pNetworkSystem );

#endif // INETWORKSYSTEM_H
