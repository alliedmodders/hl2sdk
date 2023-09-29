#ifndef NETWORKSERIALIZER_H
#define NETWORKSERIALIZER_H

#ifdef _WIN32
#pragma once
#endif

#include <platform.h>

#include <networksystem/iprotobufbinding.h>
#include <tier1/utlstring.h>
#include <tier1/bitbuf.h>
#include "Color.h"

enum NetworkValidationMode_t
{

};

enum NetworkSerializationMode_t
{
	NET_SERIALIZATION_MODE_0 = 0x0,
	NET_SERIALIZATION_MODE_1 = 0x1,
	NET_SERIALIZATION_MODE_COUNT = 0x2,
	NET_SERIALIZATION_MODE_DEFAULT = 0x0,
	NET_SERIALIZATION_MODE_SERVER = 0x0,
	NET_SERIALIZATION_MODE_CLIENT = 0x1,
};

typedef uint16 NetworkMessageId;
typedef uint8 NetworkGroupId;
typedef uint NetworkCategoryId;

struct NetMessageInfo_t
{
	int m_nCategories;
	IProtobufBinding *m_pBinding;
	CUtlString m_szGroup;
	NetworkMessageId m_MessageId;
	NetworkGroupId m_GroupId;

	// (1 << 0) - FLAG_RELIABLE
	// (1 << 6) - FLAG_AUTOASSIGNEDID
	// (1 << 7) - FLAG_UNK001
	uint8 m_nFlags;

	int m_unk001;
	int m_unk002;
	bool m_bOkayToRedispatch;
};

abstract_class INetworkSerializable
{
public:
	virtual ~INetworkSerializable() = 0;

	virtual const char *GetUnscopedName() = 0;
	virtual NetMessageInfo_t *GetNetMessageInfo() = 0;

	virtual void SetMessageId(unsigned short nMessageId) = 0;

	virtual void AddCategoryMask(int nMask, bool) = 0;

	virtual void SwitchMode(NetworkValidationMode_t nMode) = 0;

	virtual void *AllocateMessage() = 0;
	virtual void DeallocateMessage(void *pMsg) = 0;
	virtual void *AllocateAndCopyConstructNetMessage(void const *pOther) = 0;

	virtual bool Serialize(bf_write &pBuf, void const *pData, NetworkSerializationMode_t unused) = 0;
	virtual bool Unserialize(bf_read &pBuf, void *pData, NetworkSerializationMode_t unused) = 0;
};

#endif /* NETWORKSERIALIZER_H */