#include "entity2/entitykeyvalues.h"
#include "tier0/logging.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CEntityKeyValues::CEntityKeyValues( CKeyValues3Context* allocator, EntityKVAllocatorType_t allocator_type ) :
	m_pComplexKeys( NULL ),
	m_nRefCount( 0 ),
	m_nQueuedForSpawnCount( 0 ),
	m_bAllowLogging( false ),
	m_eAllocatorType( allocator_type )
{
	if ( allocator )
	{
		m_pAllocator = allocator;

		if ( GameEntitySystem() && m_pAllocator == GameEntitySystem()->GetEntityKeyValuesAllocator() )
			GameEntitySystem()->AddEntityKeyValuesAllocatorRef();

		m_pValues = m_pAllocator->AllocKV();
		m_pAttributes = m_pAllocator->AllocKV();
	}
	else
	{
		if ( !GameEntitySystem() && ( m_eAllocatorType == EKV_ALLOCATOR_ENTSYSTEM_1 || m_eAllocatorType == EKV_ALLOCATOR_ENTSYSTEM_2 ) )
			m_eAllocatorType = EKV_ALLOCATOR_NORMAL;

		m_pAllocator = NULL;
	}
}

CEntityKeyValues::~CEntityKeyValues()
{
	ReleaseAllComplexKeys();

	if ( m_pAllocator )
	{
		if ( m_eAllocatorType != EKV_ALLOCATOR_NORMAL )
		{
			m_pAllocator->FreeKV( m_pValues );
			m_pAllocator->FreeKV( m_pAttributes );

			if ( GameEntitySystem() && m_pAllocator == GameEntitySystem()->GetEntityKeyValuesAllocator() )
				GameEntitySystem()->ReleaseEntityKeyValuesAllocatorRef();
		}
		else
		{
			delete m_pAllocator;
		}
	}
}

void CEntityKeyValues::ValidateAllocator()
{
	if ( !m_pAllocator )
	{
		if ( m_eAllocatorType == EKV_ALLOCATOR_ENTSYSTEM_1 || m_eAllocatorType == EKV_ALLOCATOR_ENTSYSTEM_2 )
		{
			Assert( GameEntitySystem() );
			m_pAllocator = GameEntitySystem()->GetEntityKeyValuesAllocator();
			GameEntitySystem()->AddEntityKeyValuesAllocatorRef();
		}
		else
		{
			Assert( m_eAllocatorType != EKV_ALLOCATOR_EXTERNAL );
			m_pAllocator = new CKeyValues3Context( true );
		}

		m_pValues = m_pAllocator->AllocKV();
		m_pAttributes = m_pAllocator->AllocKV();
	}
}

void CEntityKeyValues::AddRef()
{
	++m_nRefCount;

	if ( m_bAllowLogging )
		Log_Msg( LOG_GENERAL, "kv 0x%p AddRef refcount == %d\n", this, m_nRefCount );
}

void CEntityKeyValues::Release()
{
	--m_nRefCount;

	if ( m_bAllowLogging )
		Log_Msg( LOG_GENERAL, "kv 0x%p Release refcount == %d\n", this, m_nRefCount );

	if ( m_nRefCount <= 0 )
		delete this;
}

const KeyValues3* CEntityKeyValues::GetKeyValue( const EntityKeyId_t &id, bool* pIsAttribute ) const
{
	if ( !m_pAllocator )
		return NULL;

	const KeyValues3* kv = m_pValues->FindMember( id );

	if ( kv )
	{
		if ( pIsAttribute )
			*pIsAttribute = false;
	}
	else
	{
		kv = m_pAttributes->FindMember( id );

		if ( kv )
		{
			if ( pIsAttribute )
				*pIsAttribute = true;
		}
	}

	return kv;
}

KeyValues3* CEntityKeyValues::SetKeyValue( const EntityKeyId_t &id, const char* pAttributeName )
{
	if ( m_nQueuedForSpawnCount > 0 )
		return NULL;

	ValidateAllocator();

	bool bIsAttribute;
	KeyValues3* kv = const_cast<KeyValues3*>( GetKeyValue( id, &bIsAttribute ) );

	if ( kv )
	{
		if ( !bIsAttribute && pAttributeName )
		{
			kv = NULL;
			Warning( "Attempted to set non-attribute value %s as if it was an attribute!\n", pAttributeName );
		}
		else if ( bIsAttribute && !pAttributeName )
		{
			pAttributeName = "<none>";

			for ( int i = 0; i < m_pAttributes->GetMemberCount(); ++i )
			{
				if ( m_pAttributes->GetMember( i ) != kv )
					continue;

				pAttributeName = m_pAttributes->GetMemberName( i );
				break;
			}

			kv = NULL;
			Warning( "Attempted to set attribute %s as if it was a non-attribute key!\n", pAttributeName );
		}
	}
	else
	{
		if ( pAttributeName )
			kv = m_pAttributes->FindOrCreateMember( id );
		else
			kv = m_pValues->FindOrCreateMember( id );
	}

	return kv;
}

void CEntityKeyValues::AddConnectionDesc( 
	const char* pszOutputName,
	EntityIOTargetType_t eTargetType,
	const char* pszTargetName,
	const char* pszInputName,
	const char* pszOverrideParam,
	float flDelay,
	int32 nTimesToFire )
{
	if ( m_nQueuedForSpawnCount > 0 )
		return;

	ValidateAllocator();

	EntityIOConnectionDescFat_t* desc = m_connectionDescs.AddToTailGetPtr();

	desc->m_pszOutputName		= m_pAllocator->AllocString( pszOutputName ? pszOutputName : "" );
	desc->m_eTargetType			= eTargetType;
	desc->m_pszTargetName		= m_pAllocator->AllocString( pszTargetName ? pszTargetName : "" );
	desc->m_pszInputName		= m_pAllocator->AllocString( pszInputName ? pszInputName : "" );
	desc->m_pszOverrideParam	= m_pAllocator->AllocString( pszOverrideParam ? pszOverrideParam : "" );
	desc->m_flDelay				= flDelay;
	desc->m_nTimesToFire		= nTimesToFire;
}

void CEntityKeyValues::RemoveConnectionDesc( int nDesc )
{
	if (m_nQueuedForSpawnCount > 0)
		return;

	m_connectionDescs.Remove( nDesc );
}

void CEntityKeyValues::CopyFrom( const CEntityKeyValues* pSrc, bool bRemoveAllKeys, bool bSkipEHandles )
{
	if ( bRemoveAllKeys )
		RemoveAllKeys();

	for ( EntityComplexKeyListElem_t* pListElem = pSrc->m_pComplexKeys; pListElem != NULL; pListElem = pListElem->m_pNext )
	{
		m_pComplexKeys = new EntityComplexKeyListElem_t( pListElem->m_pKey, m_pComplexKeys );
		pListElem->m_pKey->AddRef();
	}

	FOR_EACH_ENTITYKEY( pSrc, iter )
	{
		const char* pAttributeName = NULL;

		if ( pSrc->IsAttribute( iter ) )
		{
			pAttributeName = pSrc->GetAttributeName( iter );
		}
		else
		{
			if ( bSkipEHandles && pSrc->GetKeyValue( iter )->GetSubType() == KV3_SUBTYPE_EHANDLE )
				continue;
		}

		KeyValues3* kv = SetKeyValue( pSrc->GetEntityKeyId( iter ), pAttributeName );

		if ( kv )
			*kv = *pSrc->GetKeyValue( iter );
	}

	m_connectionDescs.RemoveAll();
	m_connectionDescs.EnsureCapacity( pSrc->m_connectionDescs.Count() );

	FOR_EACH_LEANVEC( pSrc->m_connectionDescs, iter )
	{
		AddConnectionDesc( 
			pSrc->m_connectionDescs[ iter ].m_pszOutputName,
			pSrc->m_connectionDescs[ iter ].m_eTargetType,
			pSrc->m_connectionDescs[ iter ].m_pszTargetName,
			pSrc->m_connectionDescs[ iter ].m_pszInputName,
			pSrc->m_connectionDescs[ iter ].m_pszOverrideParam,
			pSrc->m_connectionDescs[ iter ].m_flDelay,
			pSrc->m_connectionDescs[ iter ].m_nTimesToFire );
	}
}

void CEntityKeyValues::RemoveKeyValue( const EntityKeyId_t &id )
{
	if ( m_nQueuedForSpawnCount > 0 || !m_pAllocator )
		return;

	if ( !m_pValues->RemoveMember( id ) )
		m_pAttributes->RemoveMember( id );
}

void CEntityKeyValues::RemoveAllKeys()
{
	if ( m_nQueuedForSpawnCount > 0 )
		return;

	ReleaseAllComplexKeys();

	if ( m_pAllocator )
	{
		if ( m_eAllocatorType != EKV_ALLOCATOR_NORMAL )
		{
			m_pValues->SetToEmptyTable();
			m_pAttributes->SetToEmptyTable();
		}
		else
		{
			m_pAllocator->Clear();
			m_pValues = m_pAllocator->AllocKV();
			m_pAttributes = m_pAllocator->AllocKV();
		}
	}
}

bool CEntityKeyValues::IsEmpty() const
{
	if ( !m_pAllocator )
		return true;

	if ( !m_pValues->GetMemberCount() && !m_pAttributes->GetMemberCount() )
		return true;

	return false;
}

bool CEntityKeyValues::ValuesHasBadNames() const
{
	if ( !m_pAllocator )
		return false;

	return m_pValues->TableHasBadNames();
}

bool CEntityKeyValues::AttributesHasBadNames() const
{
	if ( !m_pAllocator )
		return false;

	return m_pAttributes->TableHasBadNames();
}

void CEntityKeyValues::ReleaseAllComplexKeys()
{
	EntityComplexKeyListElem_t* pListElem = m_pComplexKeys;

	while ( pListElem != NULL )
	{
		EntityComplexKeyListElem_t* pListElemForRelease = pListElem;
		pListElem = pListElem->m_pNext;
		pListElemForRelease->m_pKey->Release();
		delete pListElemForRelease;
	}

	m_pComplexKeys = NULL;
}

CEntityHandle CEntityKeyValues::GetEHandle( const EntityKeyId_t &id, WorldGroupId_t worldGroupId, CEntityHandle defaultValue ) const
{
	const KeyValues3* kv = GetKeyValue( id );
	
	if ( !kv )
		return defaultValue;

	switch ( kv->GetType() )
	{
		case KV3_TYPE_UINT:
			return kv->GetEHandle( defaultValue );
		case KV3_TYPE_STRING:
			Assert( GameEntitySystem() );
			return GameEntitySystem()->FindFirstEntityHandleByName( kv->GetString(), worldGroupId );
		default:
			return defaultValue;
	}
}

void CEntityKeyValues::SetString( KeyValues3* kv, const char* string )
{
	ValidateAllocator();

	kv->SetStringExternal( m_pAllocator->AllocString( string ) );
}
