#ifndef NETWORKMESSAGES_H
#define NETWORKMESSAGES_H

#ifdef _WIN32
#pragma once
#endif

#include <eiface.h>
#include <tier1/bitbuf.h>
#include <tier1/utlstring.h>
#include <tier1/utldelegate.h>
#include <tier0/logging.h>

struct NetMessageHandle_t__;
class CSchemaClassBindingBase;
class INetworkSerializerBindingBuildFilter;
class IProtobufBinding;
enum NetworkSerializationMode_t;
struct NetworkableDataType_t;
struct NetworkFieldResult_t;
struct NetworkSerializerInfo_t;
struct NetworkUnserializerInfo_t;
struct NetworkableData_t;
struct NetworkFieldInfo_t;
struct FieldMetaInfo_t;
struct NetworkFieldChangedDelegateType_t;
struct NetworkFieldChangeCallbackPerformType_t;
class CSchemaType;

typedef NetworkFieldResult_t ( *NetworkFieldSerializeCB )(NetworkSerializerInfo_t const&, int, NetworkableData_t *);
typedef NetworkFieldResult_t ( *NetworkFieldUnserializeCB )(NetworkUnserializerInfo_t const&, int, NetworkableData_t *);

typedef NetworkFieldResult_t ( *NetworkFieldSerializeBufferCB )(NetworkSerializerInfo_t const&, bf_write &, int *, void const **, bool);
typedef NetworkFieldResult_t ( *NetworkFieldUnserializeBufferCB )(NetworkUnserializerInfo_t const&, bf_read &, int *, void **, bool, CSchemaClassBindingBase const*);

typedef uint ( *NetworkFieldInfoCB )(NetworkFieldInfo_t const&);
typedef bool ( *NetworkFieldMetaInfoCB )(FieldMetaInfo_t const&, CSchemaClassBindingBase const*, NetworkFieldInfo_t const&, void *);
typedef bool ( *NetworkableDataCB )(NetworkableData_t const*, CUtlString &);
typedef char const* ( *NetworkUnkCB001 )(void);

#define NETWORKMESSAGES_INTERFACE_VERSION "NetworkMessagesVersion001"

abstract_class INetworkMessages
{
public:
	virtual void RegisterNetworkCategory(uint, char const *) = 0;

	virtual void AssociateNetMessageWithChannelCategoryAbstract(NetMessageHandle_t__ *, uint, bool) = 0;

	// virtual void FindOrCreateNetMessageSchema(int, CSchemaClassBindingBase const*, INetworkSerializerBindingBuildFilter *, bool, bool) = 0;
	virtual void FindOrCreateNetMessage(int, IProtobufBinding const*, uint, INetworkSerializerBindingBuildFilter *, bool, bool) = 0;

	virtual void SerializeAbstract(bf_write &, NetMessageHandle_t__ *, void const *) = 0;

	virtual void UnserializeAbstract(bf_read &, NetMessageHandle_t__ **, void **) = 0;
	virtual void UnserializeAbstract(bf_read &, NetMessageHandle_t__ *, void *) = 0;

	virtual void AllocateNetMessageAbstract(NetMessageHandle_t__ *) = 0;
	virtual void AllocateAndCopyConstructNetMessageAbstract(NetMessageHandle_t__ *, void const *) = 0;

	virtual void DeallocateNetMessageAbstract(NetMessageHandle_t__ *, void *) = 0;

	virtual void RegisterNetworkFieldSerializer(char const *, NetworkSerializationMode_t, NetworkableDataType_t, int, NetworkFieldSerializeCB, NetworkFieldUnserializeCB, NetworkFieldInfoCB, 
		NetworkFieldMetaInfoCB, NetworkableDataCB, NetworkUnkCB001, NetworkFieldSerializeCB, NetworkFieldUnserializeCB) = 0;
	virtual void RegisterNetworkArrayFieldSerializer(char const *, NetworkSerializationMode_t, NetworkFieldSerializeBufferCB, NetworkFieldUnserializeBufferCB, NetworkFieldInfoCB,
		NetworkFieldMetaInfoCB, NetworkFieldSerializeBufferCB, NetworkFieldUnserializeBufferCB) = 0;

	virtual void GetNetMessageInfo(NetMessageHandle_t__ *) = 0;

	virtual NetMessageHandle_t__* FindNetworkMessage(char const *) = 0;
	virtual NetMessageHandle_t__* FindNetworkMessagePartial(char const *) = 0;

	virtual void FindNetworkGroup(char const *, bool) = 0;
	virtual void GetNetworkGroupCount(void) = 0;
	virtual void GetNetworkGroupName(int) = 0;

	virtual void unk001() = 0;

	virtual void AssociateNetMessageGroupIdWithChannelCategory(uint, char const *) = 0;

	virtual void RegisterSchemaAtomicTypeOverride(uint, CSchemaType *) = 0;

	virtual void SetNextworkSerializationContextData(char const *, NetworkSerializationMode_t, void *) = 0;

	virtual void unk002() = 0;
	virtual void unk003() = 0;

	virtual void RegisterNetworkFieldChangeCallbackInternal(char const *, NetworkFieldChangedDelegateType_t, CUtlAbstractDelegate, NetworkFieldChangeCallbackPerformType_t, int) = 0;

	virtual void AllowAdditionalMessageRegistration(bool bAllow) = 0;
	virtual bool IsAdditionalMessageRegistrationAllowed() = 0;

	virtual void GetFieldChangeCallbackOrderCount(void) = 0;
	virtual void GetFieldChangeCallbackPriorities(int, int *) = 0;

	virtual void RegisterFieldChangeCallbackPriority(int) = 0;

	virtual NetMessageHandle_t__ *FindNetworkMessageById(int) = 0;

	virtual void SetIsForServer(bool bForServer) = 0;
	virtual bool IsForServer() = 0;

	virtual void RegisterSchemaTypeOverride(uint, char const *) = 0;

	virtual int ComputeOrderForPriority(int nPriority) = 0;

	virtual CLoggingSystem::LoggingChannel_t *GetLoggingChannel() = 0;

	virtual ~INetworkMessages() = 0;
};

DECLARE_TIER2_INTERFACE(INetworkMessages, g_pNetworkMessages);

#endif /* NETWORKMESSAGES_H */