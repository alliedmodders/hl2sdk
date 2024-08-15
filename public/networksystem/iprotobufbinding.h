#ifndef PROTOBUFBINDING_H
#define PROTOBUFBINDING_H

#ifdef _WIN32
#pragma once
#endif

#include <tier1/utlstring.h>
#include <tier1/bitbuf.h>
#include <inetchannel.h>
#include "Color.h"

class CNetMessage;

abstract_class IProtobufBinding
{
public:
	virtual const char *GetName() = 0;
	virtual int GetSize() = 0;

	virtual const char *ToString(const void *pData, CUtlString &sResult) = 0;

	virtual const char *GetGroup() = 0;
	virtual Color GetGroupColor() = 0;
	virtual NetChannelBufType_t GetBufType() = 0;

	virtual CNetMessage *AllocateMessage() = 0;

	virtual bool OkToRedispatch() = 0;
	virtual bool unk001() = 0;
};

#endif /* PROTOBUFBINDING_H */