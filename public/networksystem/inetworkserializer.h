#ifndef NETWORKSERIALIZER_H
#define NETWORKSERIALIZER_H

#ifdef _WIN32
#pragma once
#endif

#include <platform.h>

#include <networksystem/iprotobufbinding.h>
#include <tier1/utlstring.h>
#include <tier1/utlcommon.h>
#include <tier1/bitbuf.h>
#include <tier1/utldict.h>
#include <tier1/utlhash.h>
#include "Color.h"

#include <memory>

#define CODEGEN_HASH_TOKEN (0x3501A674)

class CNetMessage;
class CNetworkSerializerCodeGenDatabase;
class CCheckTransmitInfo;
class CEntityInstance;
class CChoreoComponent;
class CNetworkSerializerClassInfo;

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

abstract_class INetworkMessageInternal
{
public:
	virtual ~INetworkMessageInternal() = 0;

	virtual const char *GetUnscopedName() = 0;
	virtual NetMessageInfo_t *GetNetMessageInfo() = 0;

	virtual void SetMessageId( unsigned short nMessageId ) = 0;

	virtual void AddCategoryMask( int nMask, bool ) = 0;

	virtual void SwitchMode( NetworkValidationMode_t nMode ) = 0;

	virtual CNetMessage *AllocateMessage() = 0;

	// Calls to INetworkMessages::SerializeMessageInternal
	virtual bool Serialize( bf_write &pBuf, const CNetMessage *pData ) = 0;
	// Calls to INetworkMessages::UnserializeMessageInternal
	virtual bool Unserialize( bf_read &pBuf, CNetMessage *pData ) = 0;
};

struct NetworkRecipientsFilter_t
{
	using FilterCb = void (*)(CEntityInstance *ent, CCheckTransmitInfo *pInfo, CPlayerBitVec &player_mask);

	void *m_unk001;
	FilterCb m_FilterFn;
	CUtlString m_FilterName;
	int8 m_unk101;
};

struct NetworkChangePointerCallback_t
{
	using ChangeCb = void (*)(CChoreoComponent *component, CEntityInstance *ent, bool);

	CUtlString m_CallbackName;
	CUtlString m_ClassName;
	void *m_unk001;
	ChangeCb m_CallbackFn;
	int8 m_unk101;
};

struct NetworkOverride_t
{
	const char *m_ParentClass;
	const char *m_FieldName;
	const char *m_FieldPriority;
	int m_unk001;
};

struct VarTypeOverride_t
{
	CUtlString m_FieldName;
	CUtlString m_OverrideType;
};

struct SerializedFieldTypeMapping_t
{
	CUtlString m_FieldName;
	CUtlString m_FieldType;
};

struct UserGroupProxy_t
{
	const char *m_ClassName;
	const char *m_UserGroup;
	void *m_ProxyFn1;
	void *m_ProxyFn2;

	int8 m_unk001;
};

struct ReplayCompatField_t
{
	CUtlString m_FieldPath;
	CUtlString m_FieldType;
	CUtlString m_unk001;
	int8 m_unk002;
};

class CNetworkSerializerFieldInfo
{
public:
	CUtlStringToken m_FieldNameHash;
	CUtlString m_pszFieldName;
	CUtlString m_pszTypeName;
	CUtlString m_pszRawType;
	CUtlString m_pszEncodedType;
	CUtlStringToken m_ClassNameHash;
	CUtlString m_pszClassName;
	int32 m_nFieldSize;
	int32 m_nFieldOffset;
	CUtlStringToken m_NetworkAliasHash;
	CUtlString m_NetworkAlias;
	CUtlStringToken m_NetworkTypeAliasHash;
	CUtlString m_NetworkTypeAlias;

	void *m_unk001;

	CUtlString m_NetworkSerializer;
	CUtlString m_NetworkEncoder;

	std::shared_ptr<NetworkRecipientsFilter_t> m_NetworkSendProxyRecipientsFilter;
	std::shared_ptr<NetworkChangePointerCallback_t> m_NetworkChangePointerCallback;

	int8 m_NetworkPriority;

	SchemaCollectionManipulatorFn_t m_CollectionManipulatorFn;
	CUtlVector<CUtlString> m_NetworkIncludeByUserGroup;
	CUtlVector<CUtlString> m_NetworkChangeCb;

	int m_unk101;
	void *m_unk102;
	void *m_unk103;

	int m_NetworkBitCount;
	int m_NetworkEncodeFlags;
	int m_NetworkVarEmbeddedFieldOffsetDelta;
	float m_NetworkMin;
	float m_NetworkMax;

	int m_unk201;
	int8 m_unk202;

	bool m_NetworkPolymorphic;
	CUtlString m_pszCodeGenType;
	CNetworkSerializerClassInfo *m_TypeClassInfo;

	int m_CodeGenValueTypeSize;

	int m_unk401;

	CUtlString m_TypeOverride;
	CUtlString m_BuiltinUnderlyingType;

	char m_ResourceTypeForInfoType[8];
	int m_FixedArraySize;
	char m_IsAtomic;
	char m_IsBuiltIn;
	char m_IsEnum;
	char m_IsCHandle;
	char m_IsPointer;
	char m_IsUtlVector;
	char m_IsFixedArray;
	char m_IsTypeSafeInt;
	char m_IsTypeSafeFloat;
	char m_IsStrongHandle;
	char m_IsWeakHandle;
	char m_IsModifierHandle;
	char m_IsSigned;
	char m_IsNetArray;
};

struct SerializerFieldLookup_t
{
	SerializerFieldLookup_t( const char *fieldname ) : m_FieldName( fieldname ), m_FieldIndex( 0 ) {}

	CUtlString m_FieldName;
	int m_FieldIndex;
};

template<> struct DefaultEqualFunctor<SerializerFieldLookup_t> { bool operator()( SerializerFieldLookup_t a, SerializerFieldLookup_t b ) const { return a.m_FieldName == b.m_FieldName; } };
template<> struct DefaultHashFunctor<SerializerFieldLookup_t> { unsigned int operator()( SerializerFieldLookup_t a ) const { return MurmurHash2LowerCase( a.m_FieldName.String(), CODEGEN_HASH_TOKEN ); } };

class CNetworkSerializerClassInfo
{
public:
	CNetworkSerializerFieldInfo *FindField( const char *field_name ) const
	{
		auto handle = m_FieldLookupTable.Find( field_name );

		if(handle != m_FieldLookupTable.InvalidHandle())
		{
			auto elem = m_FieldLookupTable.Element( handle );
			Assert( elem.m_FieldIndex >= 0 && elem.m_FieldIndex < m_nTotalFieldEntries );
			return m_Fields[elem.m_FieldIndex];
		}

		return nullptr;
	}

public:
	struct ExcludeIncludeFilter_t
	{
		CUtlVector<CUtlString> m_ExcludeList;
		CUtlVector<CUtlString> m_IncludeList;
	};

	CUtlStringToken m_nHash;
	CUtlString m_pszClassName;
	CUtlVector<CNetworkSerializerFieldInfo *> m_Fields;
	ExcludeIncludeFilter_t m_NetworkFilterByUserGroup;
	ExcludeIncludeFilter_t m_NetworkFilterByName;

	CUtlHash<SerializerFieldLookup_t> m_FieldLookupTable;
	int m_nTotalFieldEntries;

	CUtlVector<CNetworkSerializerClassInfo> m_ParentClassInfo;
	CNetworkSerializerClassInfo *m_ParentClassInfoBuffer;
	CUtlVector<int> m_ParentClassOffset;

	struct
	{
		void *m_unk001;
		CUtlLinkedList<void *, int> m_unk002;
	} m_unk001;

	CUtlVector<NetworkOverride_t *> m_NetworkOverrides;
	// Includes this class and parent overreides
	CUtlVector<NetworkOverride_t *> m_NetworkFlattenedOverrides;

	CUtlVector<VarTypeOverride_t *> m_NetworkVarTypeOverrides;
	CUtlVector<SerializedFieldTypeMapping_t *> m_FieldTypeMappings;
	CUtlVector<UserGroupProxy_t *> m_UserGroupProxies;
	CUtlVector<ReplayCompatField_t *> m_NetworkReplayCompatFields;
	CNetworkSerializerCodeGenDatabase *m_pDatabase;

	int32_t m_nClassSize;
	int m_NetworkOutOfPVSUpdates;
	int m_unk101;

	SchemaClassManipulatorFn_t m_pfnManipulator;

	bool m_Initialized;
	bool m_NetworkVarsAtomic;
	bool m_unk201;
	bool m_unk202;
	bool m_NetworkStructNotInNetworkUtlVectorEmbedded;

	CThreadSpinRWLock m_Mutex;
};

class CNetworkSerializerCodeGenDatabase
{
public:
	struct EnumInfo_t
	{
		int32_t m_nValue;
		int8_t m_nFlags;
	};

	CUtlString m_ModuleName;
	CUtlDict<CNetworkSerializerClassInfo *> m_ClassInfos;
	CUtlDict<CNetworkSerializerCodeGenDatabase::EnumInfo_t> m_EnumInfos;
	CUtlDict<CUtlString> m_AtomicTypeMapping;

	bool m_bDebugSpew;

	CUtlString *m_unk001;
	CUtlString *m_unk002;
	CUtlString *m_unk003;
	CUtlString *m_unk004;

	int32_t m_nDuplicateCount;
};

#endif /* NETWORKSERIALIZER_H */