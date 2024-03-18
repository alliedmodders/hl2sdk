#ifndef SCHEMASYSTEM_H
#define SCHEMASYSTEM_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/threadtools.h"
#include "tier1/convar.h"
#include "tier1/utlstring.h"
#include "tier1/UtlStringMap.h"
#include "tier1/utltshash.h"
#include "tier1/utlvector.h"
#include "appframework/IAppSystem.h"
#include "schemasystem/schematypes.h"

class CBufferString;
class CKeyValues3Context;
struct ResourceManifestDesc_t;

enum SchemaTypeScope_t : uint8
{
	SCHEMA_TYPESCOPE_GLOBAL = 0,
	SCHEMA_TYPESCOPE_LOCAL,
	SCHEMA_TYPESCOPE_DEFAULT,
};

typedef void (*CompleteModuleRegistrationCallbackFn_t)(void*);

abstract_class ISchemaSystemTypeScope
{
public:
	virtual CSchemaClassInfo* InstallSchemaClassBinding( const char* pszModuleName, CSchemaClassInfo* pClassInfo ) = 0;
	virtual CSchemaEnumInfo* InstallSchemaEnumBinding( const char* pszModuleName, CSchemaEnumInfo* pEnumInfo ) = 0;

	virtual SchemaMetaInfoHandle_t<CSchemaClassInfo> FindDeclaredClass( const char* pszClassName ) = 0;
	virtual SchemaMetaInfoHandle_t<CSchemaEnumInfo> FindDeclaredEnum( const char* pszEnumName ) = 0;

	virtual SchemaMetaInfoHandle_t<CSchemaType_Builtin> FindBuiltinTypeByName( const char* pszBuiltinName ) = 0;

	virtual SchemaMetaInfoHandle_t<CSchemaType_Builtin> 		Type_Builtin( SchemaBuiltinType_t eBuiltinType ) = 0;
	virtual SchemaMetaInfoHandle_t<CSchemaType_Ptr>				Type_Ptr( CSchemaType* pObjectType ) = 0;
	virtual SchemaMetaInfoHandle_t<CSchemaType_Atomic>			Type_Atomic( const char* pszAtomicName, uint16 nSize, uint8 nAlignment ) = 0;
	virtual SchemaMetaInfoHandle_t<CSchemaType_Atomic_T>		Type_Atomic_T( const char* pszAtomicName, uint16 nSize, uint8 nAlignment, CSchemaType* pTemplateType ) = 0;
	virtual SchemaMetaInfoHandle_t<CSchemaType_Atomic_CollectionOfT> Type_Atomic_CollectionOfT( const char* pszAtomicName, uint16 nSize, uint8 nAlignment, uint16 nElementSize, CSchemaType* pTemplateType, SchemaAtomicManipulatorFn_t manipulator ) = 0;
	virtual SchemaMetaInfoHandle_t<CSchemaType_Atomic_TF>		Type_Atomic_TF( const char* pszAtomicName, uint16 nSize, uint8 nAlignment, CSchemaType* pTemplateType, int nFuncPtrSize ) = 0;
	virtual SchemaMetaInfoHandle_t<CSchemaType_Atomic_TT>		Type_Atomic_TT( const char* pszAtomicName, uint16 nSize, uint8 nAlignment, CSchemaType* pTemplateType, CSchemaType* pTemplateType2 ) = 0;
	virtual SchemaMetaInfoHandle_t<CSchemaType_Atomic_TTF>		Type_Atomic_TTF( const char* pszAtomicName, uint16 nSize, uint8 nAlignment, CSchemaType* pTemplateType, CSchemaType* pTemplateType2, int nFuncPtrSize ) = 0;
	virtual SchemaMetaInfoHandle_t<CSchemaType_Atomic_I>		Type_Atomic_I( const char* pszAtomicName, uint16 nSize, uint8 nAlignment, int nInterger ) = 0;
	virtual SchemaMetaInfoHandle_t<CSchemaType_DeclaredClass>	Type_DeclaredClass( const char* pszClassName ) = 0;
	virtual SchemaMetaInfoHandle_t<CSchemaType_DeclaredEnum>	Type_DeclaredEnum( const char* pszEnumName ) = 0;
	virtual SchemaMetaInfoHandle_t<CSchemaType_FixedArray>		Type_FixedArray( CSchemaType* pElementType, int nElementCount, uint16 nElementSize, uint8 nElementAlignment ) = 0;
	virtual SchemaMetaInfoHandle_t<CSchemaType_FixedArray>		Type_FixedArray_Multidimensional( CSchemaType* pElementType, uint16 nElementSize, uint8 nElementAlignment, ... ) = 0;
	virtual SchemaMetaInfoHandle_t<CSchemaType_Bitfield>		Type_Bitfield( int nSize ) = 0;

	virtual SchemaMetaInfoHandle_t<CSchemaType_Atomic>			FindType_Atomic( int nAtomicID ) = 0;
	virtual SchemaMetaInfoHandle_t<CSchemaType_Atomic_T>		FindType_Atomic_T( int nAtomicID, CSchemaType* pTemplateType ) = 0;
	virtual SchemaMetaInfoHandle_t<CSchemaType_Atomic_CollectionOfT> FindType_Atomic_CollectionOfT( int nAtomicID, CSchemaType* pTemplateType, SchemaAtomicManipulatorFn_t manipulator ) = 0;
	virtual SchemaMetaInfoHandle_t<CSchemaType_Atomic_TF>		FindType_Atomic_TF( int nAtomicID, CSchemaType* pTemplateType, int nFuncPtrSize ) = 0;
	virtual SchemaMetaInfoHandle_t<CSchemaType_Atomic_TT> 		FindType_Atomic_TT( int nAtomicID, CSchemaType* pTemplateType, CSchemaType* pTemplateType2 ) = 0;
	virtual SchemaMetaInfoHandle_t<CSchemaType_Atomic_TTF> 		FindType_Atomic_TTF( int nAtomicID, CSchemaType* pTemplateType, CSchemaType* pTemplateType2, int nFuncPtrSize ) = 0;
	virtual SchemaMetaInfoHandle_t<CSchemaType_Atomic_I> 		FindType_Atomic_I( int nAtomicID, int nInteger ) = 0;
	
	virtual CSchemaType_DeclaredClass* FindType_DeclaredClass( const char* pszClassName ) = 0;
	virtual CSchemaType_DeclaredEnum* FindType_DeclaredEnum( const char* pszEnumName ) = 0;

	virtual CSchemaClassInfo* FindRawClassBinding( const char* pszClassName ) = 0;
	virtual CSchemaClassInfo* FindRawClassBinding( uint nClassID ) = 0;
	virtual CSchemaEnumInfo* FindRawEnumBinding( const char* pszEnumName ) = 0;
	virtual CSchemaEnumInfo* FindRawEnumBinding( uint nEnumID ) = 0;

	virtual const char* GetScopeName() = 0;
	virtual bool IsGlobalScope() = 0;

	virtual void MarkClassAsRequiringGlobalPromotion( const CSchemaClassInfo* pClassInfo ) = 0;
	virtual void MarkEnumAsRequiringGlobalPromotion( const CSchemaEnumInfo* pEnumInfo ) = 0;

	virtual void ResolveAtomicInfoThreadsafe( const SchemaAtomicTypeInfo_t** ppAtomicInfo, const char* pszAtomicName, int nAtomicID ) = 0;
	virtual void ResolveEnumInfoThreadsafe( const CSchemaEnumInfo** pEnumInfo, const char* pszEnumName ) = 0;
	virtual void ResolveClassInfoThreadsafe( const CSchemaClassInfo** pClassInfo, const char* pszClassName ) = 0;
};

class CSchemaSystemTypeScope : public ISchemaSystemTypeScope
{
public:
	char m_szScopeName[256];
	
	CSchemaSystemTypeScope* m_pGlobalTypeScope;
	
	bool m_bBuiltinTypesInitialized;
	CSchemaType_Builtin m_BuiltinTypes[SCHEMA_BUILTIN_COUNT];
	
	CSchemaPtrMap<CSchemaType*, CSchemaType_Ptr*>					m_Ptrs;
	CSchemaPtrMap<int, CSchemaType_Atomic*>							m_Atomics;
	CSchemaPtrMap<AtomicTypeInfo_T_t, CSchemaType_Atomic_T*>		m_AtomicsT;
	CSchemaPtrMap<AtomicTypeInfo_T_t, CSchemaType_Atomic_CollectionOfT*> m_AtomicsCollectionOfT;
	CSchemaPtrMap<AtomicTypeInfo_TF_t, CSchemaType_Atomic_TF*>		m_AtomicsTF;
	CSchemaPtrMap<AtomicTypeInfo_TT_t, CSchemaType_Atomic_TT*>		m_AtomicsTT;
	CSchemaPtrMap<AtomicTypeInfo_TTF_t, CSchemaType_Atomic_TTF*>	m_AtomicsTTF;
	CSchemaPtrMap<AtomicTypeInfo_I_t, CSchemaType_Atomic_I*>		m_AtomicsI;
	CSchemaPtrMap<uint, CSchemaType_DeclaredClass*>					m_DeclaredClasses;
	CSchemaPtrMap<uint, CSchemaType_DeclaredEnum*> 					m_DeclaredEnums;
	CSchemaPtrMap<int, const SchemaAtomicTypeInfo_t*> 				m_AtomicInfos;
	CSchemaPtrMap<TypeAndCountInfo_t, CSchemaType_FixedArray*> 		m_FixedArrays;
	CSchemaPtrMap<int, CSchemaType_Bitfield*> 						m_Bitfields;
	
	CUtlTSHash<CSchemaClassInfo*, 256, uint> m_ClassBindings;
	CUtlTSHash<CSchemaEnumInfo*, 256, uint> m_EnumBindings;
};

#define SCHEMASYSTEM_INTERFACE_VERSION "SchemaSystem_001"

abstract_class ISchemaSystem : public IAppSystem
{
public:
	virtual CSchemaSystemTypeScope* GlobalTypeScope() = 0;
	virtual CSchemaSystemTypeScope* FindOrCreateTypeScopeForModule( const char* pszModuleName, const char** ppszBindingName = NULL ) = 0;
	virtual CSchemaSystemTypeScope* FindTypeScopeForModule( const char* pszModuleName, const char** ppszBindingName = NULL ) = 0;
	virtual CSchemaSystemTypeScope* GetTypeScopeForBinding( SchemaTypeScope_t eTypeScope, const char* pszModuleName, const char** ppszBindingName = NULL ) = 0;
	
	virtual bool DefaultTypeScopeIsLocal() = 0;
	
	virtual SchemaMetaInfoHandle_t<CSchemaClassInfo> FindClassByScopedName( const char* pszScopedName ) = 0;
	virtual void ScopedNameForClass( const CSchemaClassInfo* pClassInfo, CBufferString& scopedName ) = 0;
	
	virtual SchemaMetaInfoHandle_t<CSchemaEnumInfo> FindEnumByScopedName( const char* pszScopedName ) = 0;
	virtual void ScopedNameForEnum( const CSchemaEnumInfo* pEnumInfo, CBufferString& scopedName ) = 0;
	
	virtual void FindDescendentsOfClass( const CSchemaClassInfo* pClassInfo, int, CUtlVector<const CSchemaClassInfo*> *descendents, bool ) = 0;
	virtual void LoadSchemaDataForModules( const char** ppszModules, int nModules ) = 0;
	
	virtual const char* GetClassModuleName( const CSchemaClassInfo* pClassInfo ) = 0;
	virtual const char* GetClassProjectName( const CSchemaClassInfo* pClassInfo ) = 0;
	
	virtual const char* GetEnumModuleName( const CSchemaEnumInfo* pEnumInfo ) = 0;
	virtual const char* GetEnumProjectName( const CSchemaEnumInfo* pEnumInfo ) = 0;
	
	virtual bool SchemaSystemIsReady() = 0;
	virtual void VerifySchemaBindingConsistency( bool ) = 0;
	virtual void CompleteModuleRegistration( const char* pszModuleName ) = 0;
	virtual void RegisterAtomicType( const SchemaAtomicTypeInfo_t* pAtomicType ) = 0;
	
	virtual void PrintSchemaStats( const char* pszOptions ) = 0;
	virtual void PrintSchemaMetaStats( const char* pszOptions ) = 0;
	
	virtual void RegisterAtomics( const char* pszModuleName, const char* pszProjectName, CSchemaType** ppSchemaTypes, int, SchemaAtomicTypeInfo_t** ppAtomicTypes ) = 0;
	virtual void RegisterEnums( const char* pszModuleName, const char* pszProjectName, CSchemaType** ppSchemaTypes, int, CSchemaEnumInfo** ppEnumInfos ) = 0;
	virtual bool RegisterClasses( const char* pszModuleName, const char* pszProjectName, CSchemaType** ppSchemaTypes, int, CSchemaClassInfo** ppClassInfos, CBufferString* pErrorStr ) = 0;
	virtual void ValidateClasses( CSchemaClassInfo** ppClassInfos ) = 0;
	
	virtual bool ConvertOldIntrospectedResourceDataToKV3( void*, void*, void*, CKeyValues3Context*, const char* ) = 0;
	virtual void FindClassesByMeta( const char* pszMetaName, int, CUtlVector<const CSchemaClassInfo*> *classes ) = 0;
	
	virtual void InstallCompleteModuleRegistrationCallback( CompleteModuleRegistrationCallbackFn_t pfnCallback, void* pArgument ) = 0;
	virtual void RemoveCompleteModuleRegistrationCallback( CompleteModuleRegistrationCallbackFn_t pfnCallback, void* pArgument ) = 0;
	
	virtual SchemaMetaInfoHandle_t<CSchemaType_Builtin> GetSchemaBuiltinType( SchemaBuiltinType_t eBuiltinType ) = 0;
	
	virtual ~ISchemaSystem() = 0;
};

class CSchemaSystem : public ISchemaSystem
{
	struct DetectedSchemaMismatch_t
	{
		CUtlString m_sBinding;
		CUtlString m_sMismatch;
		CUtlString m_sModule1;
		CUtlString m_sModule2;
	};
	
	struct BindingsAddressRangeForModule_t
	{
		uintp m_minAddr;
		uintp m_maxAddr;
	};
	
	struct CompleteModuleRegistrationCallback_t
	{
		CompleteModuleRegistrationCallbackFn_t m_pfnCallback;
		void* m_pArgument;
	};
	
public:
	CUtlVector<ResourceManifestDesc_t*> m_ResourceManifestDescs;
	int m_nNumConnections;
	CThreadFastMutex m_Mutex;

#ifdef CONVAR_WORK_FINISHED
	CConCommandMemberAccessor<CSchemaSystem> m_SchemaListBindings;
	CConCommandMemberAccessor<CSchemaSystem> m_SchemaAllListBindings;
	CConCommandMemberAccessor<CSchemaSystem> m_SchemaDumpBinding;
	CConCommandMemberAccessor<CSchemaSystem> m_SchemaDetailedClassLayout;
	CConCommandMemberAccessor<CSchemaSystem> m_SchemaStats;
	CConCommandMemberAccessor<CSchemaSystem> m_SchemaMetaStats;
#else
#ifdef _WIN32
	uint8 pad[288];
#else
	uint8 pad[384];
#endif // _WIN32
#endif // CONVAR_WORK_FINISHED
	
	CUtlVector<void*> m_LoadedModules;
	CUtlVector<DetectedSchemaMismatch_t> m_DetectedSchemaMismatches;
	
	bool m_bDefaultTypeScopeIsLocal;
	bool m_bSchemaSystemIsReady;
	
	CUtlStringMap<CSchemaSystemTypeScope*> m_TypeScopes;
	CUtlStringMap<BindingsAddressRangeForModule_t> m_BindingsAddressRanges;
	
	int m_nRegistrations;
	int m_nIgnored;
	int m_nRedundant;
	size_t m_nIgnoredBytes;
	
	CUtlVector<CompleteModuleRegistrationCallback_t> m_CompleteModuleRegistrationCallbacks;
};

#endif // SCHEMASYSTEM_H
