#ifndef NETMESSAGE_H
#define NETMESSAGE_H

#ifdef _WIN32
#pragma once
#endif

#include "networksystem/inetworkserializer.h"

template<typename PROTO_TYPE>
class CNetMessagePB;

class CNetMessage
{
public:
	~CNetMessage() {}

	// Returns the underlying proto object
	virtual void *AsProto() const = 0;
	virtual void *AsProto2() const = 0;

	virtual INetworkMessageInternal *GetNetMessage() const
	{
		// This shouldn't ever be called and the game provided vtable should be called instead!
		Assert(0);
		return nullptr;
	}

	// Helper function to cast up the abstract message to a concrete CNetMessagePB<T> type.
	// Doesn't do any validity checks itself!
	template<typename T>
	const CNetMessagePB<T> *ToPB() const
	{
		return static_cast<const CNetMessagePB<T> *>(this);
	}

	template<typename T>
	CNetMessagePB<T> *ToPB()
	{
		return static_cast<CNetMessagePB<T> *>(this);
	}
};

// AMNOTE: This is a stub class over real CNetMessagePB!
// This is mainly to access the game constructed objects, and not for direct initialization of them
// since this misses the CNetMessage implementation which requires supplying other proto related info like
// proto binding object, proto msg id/group, etc.
// So to allocate the message yourself use INetworkMessageInternal::AllocateMessage() or INetworkMessages::AllocateNetMessageAbstract()
// functions instead of direct initialization (they both are equivalent)!
// Example usage:
// CNetMessagePB<ProtoClass> *msg = INetworkMessageInternal::AllocateMessage()->ToPB<ProtoClass>();
// msg->field1( 2 );
// msg->field2( 3 );
// IGameEventSystem::PostEventAbstract( ..., msg, ... );
template<typename PROTO_TYPE>
class CNetMessagePB : public CNetMessage, public PROTO_TYPE
{
private:
	// Prevent manual construction of this object as per the comment above
	CNetMessagePB() { };
	CNetMessagePB( const CNetMessagePB & ) { };
};

#endif // NETMESSAGE_H

