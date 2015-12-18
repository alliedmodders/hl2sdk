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
class IPeerToPeerCallbacks;
class ISteamP2PAllowConnection;
class INetworkChannelNotify;
class NetScratchBuffer_t;
class CMsgSteamDatagramGameServerAuthTicket;
class CUtlStringToken;
class CPeerToPeerAddress;

enum ENSAddressType
{
	kAddressDirect,
	kAddressP2P,
	kAddressProxiedGameServer,
	kAddressProxiedClient,

	kAddressMax
};

class ns_address
{
public:
	const netadr_t &GetAddress() const { return m_Address; }
	const CSteamID& GetSteamID() const { return m_ID; }
	const uint16 GetRemotePort() const { return m_nRemotePort; }
	ENSAddressType GetAddressType() const { return m_AddressType; }
private:
	netadr_t m_Address;
	CSteamID m_ID;
	uint16 m_nRemotePort;
	int m_Unknown;
	ENSAddressType m_AddressType;
};

enum
{
	NS_CLIENT = 0,	// client socket
	NS_SERVER,	// server socket
	NS_HLTV,
	NS_P2P,
	MAX_SOCKETS
};

enum ESteamP2PConnectionOwner {};

// Reverse engineered interface: return types may be wrong

abstract_class INetworkSystem : public IAppSystem
{
public:
	virtual void InitGameServer() = 0;
	virtual void ShutdownGameServer() = 0;
	virtual int CreateSocket( int, int, int, int, int, const char * ) = 0;
	virtual void OpenSocket( int sock ) = 0;
	virtual bool IsOpen ( int sock ) = 0;
	virtual void CloseSocket( int sock ) = 0;
	virtual void ForceReopenSocket( int sock, int ) = 0;
	virtual void SetRemoteStreamChannel( int, int ) = 0;
	virtual void AddExtraSocket( int, const char * ) = 0;
	virtual void RemoveAllExtraSockets() = 0;
	
	virtual int InitPeerToPeerNetworking( IPeerToPeerCallbacks * ) = 0;
	virtual void ShutdownPeerToPeerNetworking( int sock ) = 0;
	virtual void TerminatePeerToPeerSockets( int sock ) = 0;
	virtual void P2PAcceptableConnectionsChanged( ESteamP2PConnectionOwner owner ) = 0;
	
	virtual void EnableLoopbackBetweenSockets( int, int ) = 0;
	virtual void SetDefaultBroadcastPort( int port ) = 0;
	virtual void RunFrame( double ) = 0;
	virtual void SendQueuedPackets() = 0;
	virtual void FlushPeerToPeerChannels( int sock ) = 0;
	virtual void SleepUntilMessages( int, int ) = 0;
	virtual void InitPostFork() = 0;
	virtual void SetSubProcess( bool ) = 0;
	virtual void SendPacket( INetChannel *netchan, int, const ns_address &adr, const unsigned char *, int, bf_write *, bool, unsigned int ) = 0;
	virtual void ProcessSocket( int sock, IConnectionlessPacketHandler *handler ) = 0;
	virtual void ProcessIncomingP2PRequests( ESteamP2PConnectionOwner, ISteamP2PAllowConnection * ) = 0;
	virtual void PollSocket( int, IConnectionlessPacketHandler * ) = 0;
	virtual void ProcessSocketMessages( int ) = 0;
	virtual INetChannel *CreateNetChannel( int, const ns_address *adr, const char *, uint32, uint32 ) = 0;
	virtual INetChannel *CreateNetChannel( int, const CPeerToPeerAddress &, const char * ) = 0;
	virtual void RemoveNetChannel( INetChannel *netchan, bool ) = 0;
	virtual void RemoveNetChannelByAddress(int, const CPeerToPeerAddress &) = 0;
	virtual void ListenSocket( int sock, bool ) = 0;
	virtual void ConnectSocket( int sock , const netadr_t &adr ) = 0;
	virtual void CloseNetworkSocket( int sock, int ) = 0;
	virtual void OutOfBandPrintf( int sock, const ns_address &adr, const char *format, ...) = 0;
	virtual void OutOfBandDelayedPrintf( int sock, const ns_address &adr, unsigned int delay, const char *format, ...) = 0;
	virtual void SetTime( double time ) = 0;
	virtual void SetTimeScale( float timeScale ) = 0;
	virtual double GetNetTime() const = 0;
	virtual void DescribeSocket( int sock ) = 0;
	virtual void BufferToBufferCompress( char *, unsigned int *, const char *, unsigned int ) = 0;
	virtual void BufferToBufferDecompress( char *, unsigned int *, char const *, unsigned int ) = 0;
	virtual netadr_t& GetPublicAdr() = 0;
	virtual netadr_t& GetLocalAdr() = 0;
	virtual float GetFakeLag() = 0;
	virtual uint16 GetUDPPort( int ) = 0;
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
	virtual void AddNetworkChannelNotifyCallback( INetworkChannelNotify *callback ) = 0;
	virtual void RemoveNetworkChannelNotifyCallback( INetworkChannelNotify *callback ) = 0;
	virtual void CloseAllSockets() = 0;
	virtual int FindSocket( const CUtlStringToken & ) = 0;
	virtual NetScratchBuffer_t *GetScratchBuffer( void ) = 0;
	virtual void PutScratchBuffer( NetScratchBuffer_t * ) = 0;
	virtual void ReceivedSteamDatagramTicket( const CMsgSteamDatagramGameServerAuthTicket & ) = 0;
	virtual void *GetSteamDatagramClient( void ) = 0;
};

DECLARE_TIER2_INTERFACE( INetworkSystem, g_pNetworkSystem );

#endif // INETWORKSYSTEM_H
