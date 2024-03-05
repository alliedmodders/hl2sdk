#ifndef ENTITYINSTANCE_H
#define ENTITYINSTANCE_H
#ifdef _WIN32
#pragma once
#endif

#include "tier1/utlsymbollarge.h"
#include "entity2/entitycomponent.h"
#include "entity2/entityidentity.h"
#include "variant.h"
#include "schemasystem/schematypes.h"

class CEntityKeyValues;
class CFieldPath;
class ISave;
class IRestore;
struct CEntityPrecacheContext;
struct ChangeAccessorFieldPathIndexInfo_t;
struct datamap_t;

class CEntityInstance
{
public:
	virtual void* GetScriptDesc() = 0;
	
	virtual ~CEntityInstance() = 0;
	
	virtual void Connect() = 0;
	virtual void Disconnect() = 0;
	virtual void Precache( const CEntityPrecacheContext* pContext ) = 0;
	virtual void AddedToEntityDatabase() = 0;
	virtual void Spawn( const CEntityKeyValues* pKeyValues ) = 0;
	virtual void PostDataUpdate( /*DataUpdateType_t*/int updateType ) = 0;
	virtual void OnDataUnchangedInPVS() = 0;
	virtual void Activate( /*ActivateType_t*/int activateType ) = 0;
	virtual void UpdateOnRemove() = 0;
	virtual void OnSetDormant( /*EntityDormancyType_t*/int prevDormancyType, /*EntityDormancyType_t*/int newDormancyType ) = 0;

	virtual void* ScriptEntityIO() = 0;
	virtual int ScriptAcceptInput( const CUtlSymbolLarge &sInputName, CEntityInstance* pActivator, CEntityInstance* pCaller, const variant_t &value, int nOutputID ) = 0;
	
	virtual void PreDataUpdate( /*DataUpdateType_t*/int updateType ) = 0;
	
	virtual void DrawEntityDebugOverlays( uint64 debug_bits ) = 0;
	virtual void DrawDebugTextOverlays( void* unk, uint64 debug_bits, int flags ) = 0;
	
	virtual int Save( ISave &save ) = 0;
	virtual int Restore( IRestore &restore ) = 0;
	virtual void OnSave() = 0;
	virtual void OnRestore() = 0;
	
	virtual int ObjectCaps() = 0;
	virtual CEntityIndex RequiredEdictIndex() = 0;
	
	// marks an entire entity for transmission over the network
	virtual void NetworkStateChanged() = 0;
	
	// marks a field for transmission over the network
	// nOffset is the flattened field offset
	//		calculated taking into account embedded structures
	//		if PathIndex is specified, then the offset must start from the last object in the chain
	// nItem is the index of the array element 
	//		if the field is a CNetworkUtlVectorBase, otherwise pass -1
	// PathIndex is the value to specify 
	//		if the path to the field goes through one or more pointers, otherwise pass -1
	// 		this value is usually a member of the CNetworkVarChainer and belongs to the last object in the chain
	virtual void NetworkStateChanged( uint nOffset, int nItem = -1, ChangeAccessorFieldPathIndex_t PathIndex = ChangeAccessorFieldPathIndex_t() ) = 0;
	
	virtual void LogFieldInfo( const char* pszFieldName, const char* pszInfo ) = 0;
	virtual bool FullEdictChanged() = 0;
	virtual void unk001() = 0;
	virtual ChangeAccessorFieldPathIndex_t AddChangeAccessorPath( const CFieldPath& path ) = 0;
	virtual void AssignChangeAccessorPathIds() = 0;
	virtual ChangeAccessorFieldPathIndexInfo_t* GetChangeAccessorPathInfo_1() = 0;
	virtual ChangeAccessorFieldPathIndexInfo_t* GetChangeAccessorPathInfo_2() = 0;
	
	virtual void unk101() = 0;
	virtual void ReloadPrivateScripts() = 0;
	virtual datamap_t* GetDataDescMap() = 0;
	virtual void unk201() = 0;
	virtual SchemaMetaInfoHandle_t<CSchemaClassInfo> Schema_DynamicBinding() = 0;

public:
	inline CEntityHandle GetRefEHandle() const
	{
		return m_pEntity->GetRefEHandle();
	}
	
	inline const char *GetClassname() const
	{
		return m_pEntity->GetClassname();
	}

	inline CEntityIndex GetEntityIndex() const
	{
		return m_pEntity->GetEntityIndex();
	}

public:
	CUtlSymbolLarge m_iszPrivateVScripts; // 0x8
	CEntityIdentity* m_pEntity; // 0x10
private:
	void* m_hPrivateScope; // 0x18 - CEntityPrivateScriptScope
public:
	CEntityKeyValues* m_pKeyValues; // 0x20
	CScriptComponent* m_CScriptComponent; // 0x28	
};

// -------------------------------------------------------------------------------------------------- //
// CEntityInstance dependant functions
// -------------------------------------------------------------------------------------------------- //

inline bool CEntityHandle::operator <( const CEntityInstance *pEntity ) const
{
	uint32 otherIndex = (pEntity) ? pEntity->GetRefEHandle().m_Index : INVALID_EHANDLE_INDEX;
	return m_Index < otherIndex;
}

inline const CEntityHandle &CEntityHandle::Set( const CEntityInstance *pEntity )
{
	if(pEntity)
	{
		*this = pEntity->GetRefEHandle();
	}
	else
	{
		m_Index = INVALID_EHANDLE_INDEX;
	}

	return *this;
}

#endif // ENTITYINSTANCE_H
