#ifndef NETWORKMESSAGES_H
#define NETWORKMESSAGES_H

#ifdef _WIN32
#pragma once
#endif

#include <eiface.h>

#include <networksystem/inetworkserializer.h>
#include <networksystem/netmessage.h>
#include <tier1/bitbuf.h>
#include <tier1/utlstring.h>
#include <tier1/utlsymbol.h>
#include <tier1/utldelegate.h>
#include <tier0/logging.h>
#include "Color.h"

class CSchemaClassBindingBase;
class INetworkSerializerBindingBuildFilter;
struct NetworkableDataType_t;
struct NetworkFieldResult_t;
struct NetworkSerializerInfo_t;
struct NetworkUnserializerInfo_t;
struct NetworkableData_t;
struct NetworkFieldInfo_t;
struct FieldMetaInfo_t;
class CSchemaType;

enum NetworkFieldChangeCallbackPerformType_t
{
	NETWORK_FIELD_CHANGE_CALLBACK_PERFORM_ON_CHANGE,
	NETWORK_FIELD_CHANGE_CALLBACK_PERFORM_ON_CHANGE_OR_CREATE
};

enum NetworkFieldChangedDelegateType_t
{
	NETWORK_FIELD_CHANGED_NO_VALUE,
	NETWORK_FIELD_CHANGED_WITH_VALUE,
	NETWORK_FIELD_CHANGED_ARRAY_WITH_VALUE,
	NETWORK_FIELD_CHANGED_NO_VALUE_NO_CONTEXT,
	NETWORK_FIELD_CHANGED_TYPE_NONE,
	NETWORK_FIELD_CHANGED_COUNT_TYPE
};

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
	virtual void RegisterNetworkCategory(NetworkCategoryId nCategoryId, char const *szDebugName) = 0;

	virtual void AssociateNetMessageWithChannelCategoryAbstract(INetworkMessageInternal *pNetMessage, NetworkCategoryId nCategoryId, bool) = 0;

	// Passing nMessasgeId as -1 would auto-assign the id even if bAutoAssignId is false based on the message name hash.
	virtual INetworkMessageInternal *FindOrCreateNetMessage(int nMessageId, IProtobufBinding const *pProtoBinding, uint nMessageSize, INetworkSerializerBindingBuildFilter *pUnused, bool bCreateIfNotFound = true, bool bAutoAssignId = false) = 0;

	virtual bool Serialize(bf_write &pBuf, const CNetMessage *pData) = 0;

	virtual bool UnserializeMessageInternal( bf_read &pBuf, CNetMessage *pData ) = 0;
	virtual bool SerializeMessageInternal( bf_write &pBuf, const CNetMessage *pData ) = 0;

	// Returns nullptr if failed to unserialize, reason is written to err_reason
	virtual CNetMessage *UnserializeFromStream(bf_read &pBuf, CUtlString &err_reason) = 0;
	virtual bool SerializeAbstract(bf_write &pBuf, INetworkMessageInternal *pNetMessage, const CNetMessage *pData) = 0;

	virtual CNetMessage *AllocateAndCopyConstructNetMessageAbstract(INetworkMessageInternal *pNetMessage, const CNetMessage *pFrom) = 0;

	virtual void DeallocateNetMessageAbstract(INetworkMessageInternal *pNetMessage, CNetMessage *pData) = 0;

	virtual void *RegisterNetworkFieldSerializer(char const *, NetworkSerializationMode_t, NetworkableDataType_t, int, NetworkFieldSerializeCB, NetworkFieldUnserializeCB, NetworkFieldInfoCB, 
		NetworkFieldMetaInfoCB, NetworkableDataCB, NetworkUnkCB001, NetworkFieldSerializeCB, NetworkFieldUnserializeCB) = 0;
	virtual void *RegisterNetworkArrayFieldSerializer(char const *, NetworkSerializationMode_t, NetworkFieldSerializeBufferCB, NetworkFieldUnserializeBufferCB, NetworkFieldInfoCB,
		NetworkFieldMetaInfoCB, NetworkFieldSerializeBufferCB, NetworkFieldUnserializeBufferCB) = 0;

	virtual NetMessageInfo_t *GetNetMessageInfo(INetworkMessageInternal *pNetMessage) = 0;

	virtual INetworkMessageInternal* FindNetworkMessage(char const *szName) = 0;
	virtual INetworkMessageInternal* FindNetworkMessagePartial(char const *szPartialName) = 0;

	virtual NetworkGroupId FindNetworkGroup(char const *szGroup, bool bCreateIfNotFound = false) = 0;
	virtual int GetNetworkGroupCount() = 0;
	virtual const char *GetNetworkGroupName(NetworkGroupId nGroupId) = 0;
	virtual Color GetNetworkGroupColor(NetworkGroupId nGroupId) = 0;

	virtual void AssociateNetMessageGroupIdWithChannelCategory(NetworkCategoryId nCategoryId, char const *szGroup) = 0;

	virtual void RegisterSchemaAtomicTypeOverride(uint, CSchemaType *) = 0;

	virtual void SetNetworkSerializationContextData(char const *szContext, NetworkSerializationMode_t, void *) = 0;
	virtual void *GetNetworkSerializationContextData(char const *szContext) = 0;

	virtual void unk101() = 0;
	virtual void unk102() = 0;

	// Doesn't support duplicated callbacks per field
	virtual void RegisterNetworkFieldChangeCallbackInternal(char const *szFieldName, uint64, NetworkFieldChangedDelegateType_t fieldType, CUtlAbstractDelegate pCallback, NetworkFieldChangeCallbackPerformType_t cbPerformType, int unkflag ) = 0;

	virtual void AllowAdditionalMessageRegistration(bool bAllow) = 0;
	virtual bool IsAdditionalMessageRegistrationAllowed() = 0;

	virtual int GetFieldChangeCallbackOrderCount() = 0;
	virtual void GetFieldChangeCallbackPriorities(int nCount, int *nPriorities) = 0;

	virtual void RegisterFieldChangeCallbackPriority(int nPriority) = 0;

	virtual INetworkMessageInternal *FindNetworkMessageById(int nMessageId) = 0;

	virtual void SetIsForServer(bool bIsForServer) = 0;
	virtual bool GetIsForServer() = 0;

	virtual void RegisterSchemaTypeOverride(uint, char const *) = 0;

	virtual int ComputeOrderForPriority(int nPriority) = 0;

	virtual CLoggingSystem::LoggingChannel_t *GetLoggingChannel() = 0;

	virtual ~INetworkMessages() = 0;
};

DECLARE_TIER2_INTERFACE(INetworkMessages, g_pNetworkMessages);

#endif /* NETWORKMESSAGES_H */