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
	virtual void ConnectSocket( int sock, const netadr_t &adr ) = 0;
	virtual bool IsSocketOpen( int sock ) = 0;
	virtual void CloseSocket( int sock ) = 0;
	virtual void EnableLoopbackBetweenSockets( int sock1, int sock2 ) = 0;
	virtual void SetDefaultBroadcastPort( int port ) = 0;
	virtual void PollSocket( int sock, IConnectionlessPacketHandler * ) = 0;

	virtual void unk001() = 0;

	virtual INetChannel *CreateNetChannel( int sock, const ns_address *adr, uint32 steam_handle, const char *, uint32, uint32 ) = 0;
	virtual void RemoveNetChannel( INetChannel *netchan, bool ) = 0;
	virtual bool RemoveNetChannelByAddress( int, const ns_address *adr ) = 0;

	virtual void PrintNetworkStats() = 0;

	virtual void unk101() = 0;
	virtual void unk102() = 0;

	virtual const char *DescribeSocket( int sock ) = 0;
	virtual bool IsValidSocket( int sock ) = 0;

	virtual void BufferToBufferCompress( uint8 *pDest, int &nDestSize, uint8 *pIn, unsigned int nInSize ) = 0;
	virtual void BufferToBufferDecompress( uint8 *pDest, int &nDestSize, uint8 *pIn, unsigned int nInSize ) = 0;

	virtual netadr_t &GetPublicAdr() = 0;
	virtual netadr_t &GetLocalAdr() = 0;
	virtual float GetFakeLag( int sock ) = 0;
	virtual uint16 GetUDPPort( int sock ) = 0;

	virtual void unk201() = 0;
	virtual void unk202() = 0;

	virtual void CloseAllSockets() = 0;

	virtual NetScratchBuffer_t *GetScratchBuffer( void ) = 0;
	virtual void PutScratchBuffer( NetScratchBuffer_t * ) = 0;

	// Returns SteamNetworkingUtils004 interface
	virtual void *GetSteamNetworkUtils() = 0;

	// Returns SteamApi SteamNetworkingSockets012 interface
	virtual void *GetSteamUserNetworkingSockets() = 0;

	// Returns GameServer SteamNetworkingSockets012 interface
	virtual void *GetSteamGameServerNetworkingSockets() = 0;

	// Returns either User or GameServer SteamNetworkingSockets012 interface
	virtual void *GetSteamNetworkingSockets() = 0;

	// Returns SteamNetworkingMessages002 interface
	virtual void *GetSteamNetworkingMessages() = 0;

	virtual void unk301() = 0;
	virtual void unk302() = 0;
	virtual void unk303() = 0;
	virtual void unk304() = 0;
	virtual void unk305() = 0;

	virtual void InitNetworkSystem() = 0;

	virtual void unk401() = 0;
	virtual void unk402() = 0;

	virtual ~INetworkSystem() = 0;
};

DECLARE_TIER2_INTERFACE( INetworkSystem, g_pNetworkSystem );

#endif // INETWORKSYSTEM_H
