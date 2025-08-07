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
#include <initializer_list>

class CEntityKeyValues;
class CFieldPath;
class ISave;
class IRestore;
struct CEntityPrecacheContext;
struct ChangeAccessorFieldPathIndexInfo_t;
struct datamap_t;

struct NetworkStateChangedData
{
	inline NetworkStateChangedData() : m_unk001(1), m_nLine(-1), m_nArrayIndex(-1), m_nPathIndex(ChangeAccessorFieldPathIndex_t()), m_unk101(0) { }
	inline explicit NetworkStateChangedData( bool bFullChanged ) :
		m_unk001(static_cast<uint32>(!bFullChanged)), m_nLine(-1),
		m_nArrayIndex(-1), m_nPathIndex(ChangeAccessorFieldPathIndex_t()),
		m_unk101(0)
	{ }

	// nLocalOffset is the flattened field offset
	//		calculated taking into account embedded structures
	//		if PathIndex is specified, then the offset must start from the last object in the chain
	// nArrayIndex is the index of the array element 
	//		if the field is a CNetworkUtlVectorBase, otherwise pass -1
	// nPathIndex is the value to specify 
	//		if the path to the field goes through one or more pointers, otherwise pass -1
	// 		this value is usually a member of the CNetworkVarChainer and belongs to the last object in the chain
	inline NetworkStateChangedData( uint32 nLocalOffset, int32 nArrayIndex = -1, ChangeAccessorFieldPathIndex_t nPathIndex = ChangeAccessorFieldPathIndex_t() ) :
		m_unk001(1), m_LocalOffsets(0, 1), m_nLine(-1), m_nArrayIndex(nArrayIndex), m_nPathIndex(nPathIndex), m_unk101(0)
	{
		m_LocalOffsets.AddToHead(nLocalOffset);
	}

	inline NetworkStateChangedData(const std::initializer_list< uint32 > nLocalOffsets, int32 nArrayIndex = -1, ChangeAccessorFieldPathIndex_t nPathIndex = ChangeAccessorFieldPathIndex_t()) :
		m_unk001(1), m_LocalOffsets(0, nLocalOffsets.size()), m_nLine(-1), m_nArrayIndex(nArrayIndex), m_nPathIndex(nPathIndex), m_unk101(1)
	{
		for ( const uint32& nLocalOffset : nLocalOffsets )
		{
			m_LocalOffsets.AddToTail( nLocalOffset );
		}
	}

	uint32 m_unk001; // Perhaps it is an enum, default 1, when 0 adds FL_FULL_EDICT_CHANGED
	CUtlVector<uint32> m_LocalOffsets;
	
	// AMNOTE: Mostly unused/debug
	CUtlString m_FieldName;
	CUtlString m_FileName;
	int32 m_nLine;
	int32 m_nArrayIndex;
	ChangeAccessorFieldPathIndex_t m_nPathIndex;

	int16 m_unk101; // default 0, if m_LocalOffsets has multiple values, it is set to 1
};

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

	virtual void unk001() = 0;

	virtual void PostDataUpdate( /*DataUpdateType_t*/int updateType ) = 0;
	virtual void OnDataUnchangedInPVS() = 0;
	virtual void Activate( /*ActivateType_t*/int activateType ) = 0;
	virtual void UpdateOnRemove() = 0;
	virtual void OnSetDormant( /*EntityDormancyType_t*/int prevDormancyType, /*EntityDormancyType_t*/int newDormancyType ) = 0;

	virtual void* ScriptEntityIO() = 0;
	virtual int ScriptAcceptInput( const CUtlSymbolLarge &sInputName, CEntityInstance* pActivator, CEntityInstance* pCaller, const variant_t &value, int nOutputID, void* pUnk1, void* pUnk2 ) = 0;
	
	virtual void PreDataUpdate( /*DataUpdateType_t*/int updateType ) = 0;
	
	virtual void DrawEntityDebugOverlays( uint64 debug_bits ) = 0;
	virtual void DrawDebugTextOverlays( void* unk, uint64 debug_bits, int flags ) = 0;
	
	virtual int Save( ISave &save ) = 0;
	virtual int Restore( IRestore &restore ) = 0;
	virtual void OnSave() = 0;
	virtual void OnRestore() = 0;
	
	virtual void unk101() = 0;

	virtual int ObjectCaps() = 0;
	virtual CEntityIndex RequiredEdictIndex() = 0;

	// marks a field for transmission over the network
	virtual void NetworkStateChanged( const NetworkStateChangedData& data ) = 0;

	// AMNOTE: NetworkState related methods
	virtual void unk201( const void* data ) = 0;
	virtual void unk202( const void* data ) = 0;

	// Toggles network update state, if set to false would skip network updates
	virtual void NetworkUpdateState( bool state ) = 0;
	virtual void NetworkStateChangedLog( const char* pszFieldName, const char* pszInfo ) = 0;

	virtual bool FullEdictChanged() = 0;

	virtual void unk301() = 0;
	virtual void unk302() = 0;

	virtual ChangeAccessorFieldPathIndex_t AddChangeAccessorPath( const CFieldPath& path ) = 0;
	virtual void AssignChangeAccessorPathIds() = 0;
	virtual ChangeAccessorFieldPathIndexInfo_t* GetChangeAccessorPathInfo_1() = 0;
	virtual ChangeAccessorFieldPathIndexInfo_t* GetChangeAccessorPathInfo_2() = 0;
	
	virtual void unk401() = 0;
	virtual void unk402() = 0;

	virtual void ReloadPrivateScripts() = 0;
	virtual datamap_t* GetDataDescMap() = 0;

	virtual void unk501() = 0;

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
private:
	uint8 pad[8];
public:
	CScriptComponent* m_CScriptComponent; // 0x30
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
