//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// netadr.h
#ifndef NETADR_H
#define NETADR_H
#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h"
#include "bitbuf.h"
#undef SetPort

typedef enum
{ 
	NA_NULL = 0,
	NA_LOOPBACK,
	NA_BROADCAST,
	NA_IP,
} netadrtype_t;

struct netadr_t
{
public:
	netadr_t() { SetIP( 0 ); SetPort( 0 ); SetType( NA_IP ); }
	netadr_t( uint unIP, uint16 usPort ) { SetIP( unIP ); SetPort( usPort ); SetType( NA_IP ); }
	netadr_t( const char *pch ) { SetFromString( pch ); }

	// invalids Address
	DLL_CLASS_IMPORT void	Clear();

	DLL_CLASS_IMPORT void	SetType( netadrtype_t type );
	DLL_CLASS_IMPORT void	SetPort( unsigned short port );
	DLL_CLASS_IMPORT bool	SetFromSockadr( const struct sockaddr *s );

	DLL_CLASS_IMPORT void	SetIP( uint8 b1, uint8 b2, uint8 b3, uint8 b4 );
	// unIP is in host order (little-endian)
	DLL_CLASS_IMPORT void	SetIP( uint unIP );
	void   SetIPAndPort( uint unIP, unsigned short usPort ) { SetIP( unIP ); SetPort( usPort ); }

	// if bUseDNS is true then do a DNS lookup if needed
	DLL_CLASS_IMPORT void	SetFromString( const char *pch, bool bUseDNS = false );

	DLL_CLASS_IMPORT bool	CompareAdr( const netadr_t &a, bool onlyBase = false ) const;
	DLL_CLASS_IMPORT bool	CompareClassBAdr( const netadr_t &a ) const;
	DLL_CLASS_IMPORT bool	CompareClassCAdr( const netadr_t &a ) const;

	DLL_CLASS_IMPORT netadrtype_t	GetType() const;
	DLL_CLASS_IMPORT unsigned short	GetPort() const;

	// returns xxx.xxx.xxx.xxx:ppppp
	DLL_CLASS_IMPORT const char *ToString( bool onlyBase = false ) const;
	DLL_CLASS_IMPORT void ToString( char *pchBuffer, uint32 unBufferSize, bool onlyBase = false ) const;

	DLL_CLASS_IMPORT void			ToSockadr( struct sockaddr *s ) const;

	DLL_CLASS_IMPORT unsigned int	GetIPHostByteOrder() const;
	DLL_CLASS_IMPORT unsigned int	GetIPNetworkByteOrder() const;
	inline unsigned int	GetIP() const { return GetIPNetworkByteOrder(); }

	// true, if this is the localhost IP 
	DLL_CLASS_IMPORT bool	IsLocalhost() const;
	// true if engine loopback buffers are used
	DLL_CLASS_IMPORT bool	IsLoopback() const;
	// true, if this is a private LAN IP
	DLL_CLASS_IMPORT bool	IsReservedAdr() const;
	// ip & port != 0
	DLL_CLASS_IMPORT bool	IsValid() const;
	// ip != 0
	DLL_CLASS_IMPORT bool	IsBaseAdrValid() const;

	DLL_CLASS_IMPORT void    SetFromSocket( int hSocket );

	DLL_CLASS_IMPORT bool    Serialize( bf_write &writeBuf );
	DLL_CLASS_IMPORT bool    Unserialize( bf_read &readBuf );

	bool operator==( const netadr_t &netadr ) const { return (CompareAdr( netadr )); }
	DLL_CLASS_IMPORT bool operator<( const netadr_t &netadr ) const;

public:	// members are public to avoid to much changes

	netadrtype_t	type;
	unsigned char	ip[4];
	unsigned short	port;
};

#endif // NETADR_H
