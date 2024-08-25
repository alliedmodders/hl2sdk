#ifndef SCHEMATYPES_H
#define SCHEMATYPES_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/logging.h"
#include "tier0/threadtools.h"
#include "tier1/generichash.h"
#include "tier1/utlmap.h"
#include "tier1/utlstring.h"

class ISchemaSystemTypeScope;
class CSchemaSystemTypeScope;
class CSchemaClassInfo;
class CSchemaEnumInfo;
class CBufferString;
struct SchemaAtomicTypeInfo_t;
struct datamap_t;

enum SchemaClassFlags1_t
{
	SCHEMA_CF1_HAS_VIRTUAL_MEMBERS = (1 << 0),
	SCHEMA_CF1_IS_ABSTRACT = (1 << 1),
	SCHEMA_CF1_HAS_TRIVIAL_CONSTRUCTOR = (1 << 2),
	SCHEMA_CF1_HAS_TRIVIAL_DESTRUCTOR = (1 << 3),
	SCHEMA_CF1_LIMITED_METADATA = (1 << 4),
	SCHEMA_CF1_INHERITANCE_DEPTH_CALCULATED = (1 << 5),
	SCHEMA_CF1_MODULE_LOCAL_TYPE_SCOPE = (1 << 6),
	SCHEMA_CF1_GLOBAL_TYPE_SCOPE = (1 << 7),
	SCHEMA_CF1_CONSTRUCT_ALLOWED = (1 << 8),
	SCHEMA_CF1_CONSTRUCT_DISALLOWED = (1 << 9),
	SCHEMA_CF1_INFO_TAG_MNetworkAssumeNotNetworkable = (1 << 10),
	SCHEMA_CF1_INFO_TAG_MNetworkNoBase = (1 << 11),
	SCHEMA_CF1_INFO_TAG_MIgnoreTypeScopeMetaChecks = (1 << 12),
	SCHEMA_CF1_INFO_TAG_MDisableDataDescValidation = (1 << 13),
	SCHEMA_CF1_INFO_TAG_MClassHasEntityLimitedDataDesc = (1 << 14),
	SCHEMA_CF1_INFO_TAG_MClassHasCustomAlignedNewDelete = (1 << 15),
	SCHEMA_CF1_UNK016 = (1 << 16),
	SCHEMA_CF1_INFO_TAG_MConstructibleClassBase = (1 << 17),
	SCHEMA_CF1_INFO_TAG_MHasKV3TransferPolymorphicClassname = (1 << 18),
};

enum SchemaClassFlags2_t {};

enum SchemaEnumFlags_t
{
	SCHEMA_EF_IS_REGISTERED = (1 << 0),
	SCHEMA_EF_MODULE_LOCAL_TYPE_SCOPE = (1 << 1),
	SCHEMA_EF_GLOBAL_TYPE_SCOPE = (1 << 2),
};

enum SchemaTypeCategory_t : uint8
{
	SCHEMA_TYPE_BUILTIN = 0,
	SCHEMA_TYPE_POINTER,
	SCHEMA_TYPE_BITFIELD,
	SCHEMA_TYPE_FIXED_ARRAY,
	SCHEMA_TYPE_ATOMIC,
	SCHEMA_TYPE_DECLARED_CLASS,
	SCHEMA_TYPE_DECLARED_ENUM,
	SCHEMA_TYPE_INVALID,
};

enum SchemaAtomicCategory_t : uint8
{
	SCHEMA_ATOMIC_PLAIN = 0,
	SCHEMA_ATOMIC_T,
	SCHEMA_ATOMIC_COLLECTION_OF_T,
	SCHEMA_ATOMIC_TT,
	SCHEMA_ATOMIC_I,
	SCHEMA_ATOMIC_INVALID,
};

enum SchemaBuiltinType_t
{
	SCHEMA_BUILTIN_TYPE_INVALID = 0,
	SCHEMA_BUILTIN_TYPE_VOID,
	SCHEMA_BUILTIN_TYPE_CHAR,
	SCHEMA_BUILTIN_TYPE_INT8,
	SCHEMA_BUILTIN_TYPE_UINT8,
	SCHEMA_BUILTIN_TYPE_INT16,
	SCHEMA_BUILTIN_TYPE_UINT16,
	SCHEMA_BUILTIN_TYPE_INT32,
	SCHEMA_BUILTIN_TYPE_UINT32,
	SCHEMA_BUILTIN_TYPE_INT64,
	SCHEMA_BUILTIN_TYPE_UINT64,
	SCHEMA_BUILTIN_TYPE_FLOAT32,
	SCHEMA_BUILTIN_TYPE_FLOAT64,
	SCHEMA_BUILTIN_TYPE_BOOL,
	SCHEMA_BUILTIN_TYPE_COUNT,
};

enum SchemaClassManipulatorAction_t
{
	SCHEMA_CLASS_MANIPULATOR_ACTION_REGISTER = 0,
	SCHEMA_CLASS_MANIPULATOR_ACTION_REGISTER_PRE,
	SCHEMA_CLASS_MANIPULATOR_ACTION_ALLOCATE,
	SCHEMA_CLASS_MANIPULATOR_ACTION_DEALLOCATE,
	SCHEMA_CLASS_MANIPULATOR_ACTION_CONSTRUCT_IN_PLACE,
	SCHEMA_CLASS_MANIPULATOR_ACTION_DESCTRUCT_IN_PLACE,
	SCHEMA_CLASS_MANIPULATOR_ACTION_GET_SCHEMA_BINDING,
};

enum SchemaCollectionManipulatorAction_t
{
	SCHEMA_COLLECTION_MANIPULATOR_ACTION_GET_COUNT = 0,
	SCHEMA_COLLECTION_MANIPULATOR_ACTION_GET_ELEMENT_CONST,
	SCHEMA_COLLECTION_MANIPULATOR_ACTION_GET_ELEMENT,
	SCHEMA_COLLECTION_MANIPULATOR_ACTION_SWAP_ELEMENTS,
	SCHEMA_COLLECTION_MANIPULATOR_ACTION_INSERT_BEFORE,
	SCHEMA_COLLECTION_MANIPULATOR_ACTION_REMOVE_MULTIPLE,
	SCHEMA_COLLECTION_MANIPULATOR_ACTION_SET_COUNT,
};

typedef void* (*SchemaClassManipulatorFn_t)(SchemaClassManipulatorAction_t, void*, void*);
typedef void* (*SchemaCollectionManipulatorFn_t)(SchemaCollectionManipulatorAction_t, void*, void*, void*);

inline uint32 CSchemaType_Hash( const char *pString, int len )
{
	return MurmurHash2( pString, len, 0xBAADFEED ); 
}

template <class T>
struct SchemaMetaInfoHandle_t
{
	SchemaMetaInfoHandle_t() : m_pObj( NULL ) {}
	SchemaMetaInfoHandle_t( T *obj ) : m_pObj( obj ) {}
	T* Get() const { return m_pObj; }
	bool operator<( const SchemaMetaInfoHandle_t& rhs ) const { return m_pObj < rhs.m_pObj; }

	T* m_pObj;
};

template <class K, class V>
class CSchemaPtrMap
{
public:
	CUtlOrderedMap<K, V> m_Map;
	CThreadFastMutex m_Mutex;
};

class CSchemaType
{
public:
	virtual bool IsValid() { return false; }
	virtual const char* ToString( CBufferString& buff, bool bDontClearBuff ) { return ""; }
	virtual void SpewDescription( LoggingChannelID_t channelID, const char* pszName ) {}
	virtual bool GetSizeAndAlignment( int& nSize, uint8& nAlignment ) { return false; }
	virtual bool CanReinterpretAs( const CSchemaType* pType ) { return false; }
	virtual SchemaMetaInfoHandle_t<CSchemaType> GetInnerType() { return SchemaMetaInfoHandle_t<CSchemaType>(); }
	virtual SchemaMetaInfoHandle_t<CSchemaType> GetInnermostType() { return SchemaMetaInfoHandle_t<CSchemaType>(); }
	virtual bool IsA( const CSchemaType* pType ) { return false; }
	virtual bool InternalMatchInnerAs( SchemaTypeCategory_t eTypeCategory, SchemaAtomicCategory_t eAtomicCategory ) { return false; }
	
	virtual void unk001() {}
	virtual void unk002() {}
	virtual void unk003() {}
	virtual void unk004() {}
	virtual void unk005() {}
	virtual void unk006() {}
	
	virtual bool DependsOnlyOnUnresolvedOrGlobalTypes( ISchemaSystemTypeScope* pTypeScope ) { return false; }
	
	virtual ~CSchemaType() = 0;

public:
	CUtlString m_sTypeName;
	CSchemaSystemTypeScope* m_pTypeScope;
	SchemaTypeCategory_t m_eTypeCategory;
	SchemaAtomicCategory_t m_eAtomicCategory;
};

class CSchemaType_Builtin : public CSchemaType
{
public:
	SchemaBuiltinType_t m_eBuiltinType;
	uint8 m_nSize;
};

class CSchemaType_Ptr : public CSchemaType
{
public:
	CSchemaType* m_pObjectType;
};

class CSchemaType_Atomic : public CSchemaType
{
public:
	SchemaAtomicTypeInfo_t* m_pAtomicInfo;
	int m_nAtomicID;
	uint16 m_nSize;
	uint8 m_nAlignment;
};

class CSchemaType_Atomic_T : public CSchemaType_Atomic
{
public:
	CSchemaType* m_pTemplateType;
};

class CSchemaType_Atomic_CollectionOfT : public CSchemaType_Atomic_T
{
public:
	SchemaCollectionManipulatorFn_t m_pfnManipulator;
	uint16 m_nElementSize;
};

class CSchemaType_Atomic_TT : public CSchemaType_Atomic_T
{
public:
	CSchemaType* m_pTemplateType2;
};

class CSchemaType_Atomic_I : public CSchemaType_Atomic
{
public:
	int m_nInteger;
};

class CSchemaType_DeclaredClass : public CSchemaType
{
public:
	CSchemaClassInfo* m_pClassInfo;
	bool m_bGlobalPromotionRequired;
};

class CSchemaType_DeclaredEnum : public CSchemaType
{
public:
	CSchemaEnumInfo* m_pEnumInfo;
	bool m_bGlobalPromotionRequired;
};

class CSchemaType_FixedArray : public CSchemaType
{
public:
	int m_nElementCount;
	uint16 m_nElementSize;
	uint8 m_nElementAlignment;
	CSchemaType* m_pElementType;
};

class CSchemaType_Bitfield : public CSchemaType
{
public:
	int m_nBitfieldCount;
};

struct SchemaMetadataEntryData_t
{
	const char* m_pszName;
	void* m_pData;
};

struct SchemaClassFieldData_t
{
	const char* m_pszName;
	
	CSchemaType* m_pType;
	
	int m_nSingleInheritanceOffset;
	
	int m_nStaticMetadataCount;
	SchemaMetadataEntryData_t* m_pStaticMetadata;
};

struct SchemaStaticFieldData_t
{
	const char* m_pszName;
	
	CSchemaType* m_pType;
	
	void* m_pInstance;
	
	int m_nStaticMetadataCount;
	SchemaMetadataEntryData_t* m_pStaticMetadata;
};

struct SchemaBaseClassInfoData_t
{
	uint m_nOffset;
	CSchemaClassInfo* m_pClass;
};

struct SchemaClassInfoData_t
{
	CSchemaClassInfo* m_pSchemaBinding;
	
	const char* m_pszName;
	const char* m_pszProjectName;
	
	int m_nSize;
	
	uint16 m_nFieldCount;
	uint16 m_nStaticFieldCount;
	uint16 m_nStaticMetadataCount;
	
	uint8 m_nAlignment;
	uint8 m_nBaseClassCount;
	
	uint16 m_nMultipleInheritanceDepth;
	uint16 m_nSingleInheritanceDepth;
	
	SchemaClassFieldData_t* m_pFields;
	SchemaStaticFieldData_t* m_pStaticFields;
	SchemaBaseClassInfoData_t* m_pBaseClasses;
	datamap_t* m_pDataDescMap;
	SchemaMetadataEntryData_t* m_pStaticMetadata;
	
	CSchemaSystemTypeScope* m_pTypeScope;
	CSchemaType_DeclaredClass* m_pDeclaredClass;
	
	uint m_nFlags1;
	uint m_nFlags2;
	
	SchemaClassManipulatorFn_t m_pfnManipulator;
};

class CSchemaClassInfo : public SchemaClassInfoData_t
{
};

struct SchemaEnumeratorInfoData_t
{
	const char* m_pszName;
	
	int64 m_nValue;
	
	int m_nStaticMetadataCount;
	SchemaMetadataEntryData_t* m_pStaticMetadata;
};

struct SchemaEnumInfoData_t
{
	CSchemaEnumInfo* m_pSchemaBinding;
	
	const char* m_pszName;
	const char* m_pszProjectName;
	
	uint8 m_nSize;
	uint8 m_nAlignment;
	
	uint16 m_nFlags;
	uint16 m_nEnumeratorCount;
	uint16 m_nStaticMetadataCount;
	
	SchemaEnumeratorInfoData_t* m_pEnumerators;
	SchemaMetadataEntryData_t* m_pStaticMetadata;
	
	CSchemaSystemTypeScope* m_pTypeScope;
	
	int64 m_nMinEnumeratorValue;
	int64 m_nMaxEnumeratorValue;
};

class CSchemaEnumInfo : public SchemaEnumInfoData_t
{
};

struct SchemaAtomicTypeInfo_t
{
	const char* m_pszName;
	const char* m_pszTokenName;
	
	int m_nAtomicID;
	
	int m_nStaticMetadataCount;
	SchemaMetadataEntryData_t* m_pStaticMetadata;
};

struct AtomicTypeInfo_T_t
{
	bool operator<( const AtomicTypeInfo_T_t& rhs ) const 
	{ 
		if ( m_nAtomicID != rhs.m_nAtomicID )
			return m_nAtomicID < rhs.m_nAtomicID;
		
		if ( m_pTemplateType != rhs.m_pTemplateType )
			return m_pTemplateType < rhs.m_pTemplateType;
		
		return (void *)m_pfnManipulator < (void *)rhs.m_pfnManipulator;
	}
	
	int m_nAtomicID;
	CSchemaType* m_pTemplateType;
	SchemaCollectionManipulatorFn_t m_pfnManipulator;
};

struct AtomicTypeInfo_TT_t
{
	bool operator<( const AtomicTypeInfo_TT_t& rhs ) const 
	{ 
		if ( m_nAtomicID != rhs.m_nAtomicID )
			return m_nAtomicID < rhs.m_nAtomicID;
		
		if ( m_pTemplateType != rhs.m_pTemplateType )
			return m_pTemplateType < rhs.m_pTemplateType;
		
		return m_pTemplateType2 < rhs.m_pTemplateType2;
	}
	
	int m_nAtomicID;
	CSchemaType* m_pTemplateType;
	CSchemaType* m_pTemplateType2;
};

struct AtomicTypeInfo_I_t
{
	bool operator<( const AtomicTypeInfo_I_t& rhs ) const 
	{ 
		if ( m_nAtomicID != rhs.m_nAtomicID )
			return m_nAtomicID < rhs.m_nAtomicID;

		return m_nInteger < rhs.m_nInteger;
	}
	
	int m_nAtomicID;
	int m_nInteger;
};

struct TypeAndCountInfo_t
{
	bool operator<( const TypeAndCountInfo_t& rhs ) const 
	{ 
		if ( m_nElementCount != rhs.m_nElementCount )
			return m_nElementCount < rhs.m_nElementCount;
		
		return m_pElementType < rhs.m_pElementType;
	}
	
	int m_nElementCount;
	CSchemaType* m_pElementType;
};

#endif // SCHEMATYPES_H
