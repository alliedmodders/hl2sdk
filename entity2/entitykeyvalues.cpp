#include "entity2/entitysystem.h"
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

		if ( EntitySystem() && m_pAllocator == EntitySystem()->GetEntityKeyValuesAllocator() )
			EntitySystem()->AddEntityKeyValuesAllocatorRef();

		m_pKeyValues = m_pAllocator->AllocKV();
		m_pAttributes = m_pAllocator->AllocKV();
	}
	else
	{
		if ( !EntitySystem() && ( m_eAllocatorType == EKV_ALLOCATOR_ENTSYSTEM1 || m_eAllocatorType == EKV_ALLOCATOR_ENTSYSTEM2 ) )
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
			m_pAllocator->FreeKV( m_pKeyValues );
			m_pAllocator->FreeKV( m_pAttributes );

			if ( EntitySystem() && m_pAllocator == EntitySystem()->GetEntityKeyValuesAllocator() )
				EntitySystem()->ReleaseEntityKeyValuesAllocatorRef();
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
		if ( m_eAllocatorType == EKV_ALLOCATOR_ENTSYSTEM1 || m_eAllocatorType == EKV_ALLOCATOR_ENTSYSTEM2 )
		{
			Assert( EntitySystem() );
			m_pAllocator = EntitySystem()->GetEntityKeyValuesAllocator();
			EntitySystem()->AddEntityKeyValuesAllocatorRef();
		}
		else
		{
			Assert( m_eAllocatorType != EKV_ALLOCATOR_EXTERNAL );
			m_pAllocator = new CKeyValues3Context( true );
		}

		m_pKeyValues = m_pAllocator->AllocKV();
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

const KeyValues3* CEntityKeyValues::GetValue( const EntityKeyId_t &id, bool* pIsAttribute ) const
{
	if ( !m_pAllocator )
		return NULL;

	const KeyValues3* value = m_pKeyValues->FindMember( id );

	if ( value )
	{
		if ( pIsAttribute )
			*pIsAttribute = false;
	}
	else
	{
		value = m_pAttributes->FindMember( id );

		if ( value )
		{
			if ( pIsAttribute )
				*pIsAttribute = true;
		}
	}

	return value;
}

KeyValues3* CEntityKeyValues::SetValue( const EntityKeyId_t &id, const char* pAttributeName )
{
	if ( m_nQueuedForSpawnCount > 0 )
		return NULL;

	ValidateAllocator();

	bool bIsAttribute;
	KeyValues3* value = const_cast<KeyValues3*>( GetValue( id, &bIsAttribute ) );

	if ( value )
	{
		if ( !bIsAttribute && pAttributeName )
		{
			value = NULL;
			Warning( "Attempted to set non-attribute value %s as if it was an attribute!\n", pAttributeName );
		}
		else if ( bIsAttribute && !pAttributeName )
		{
			pAttributeName = "<none>";

			for ( int i = 0; i < m_pAttributes->GetMemberCount(); ++i )
			{
				if ( m_pAttributes->GetMember( i ) != value )
					continue;

				pAttributeName = m_pAttributes->GetMemberName( i );
				break;
			}

			value = NULL;
			Warning( "Attempted to set attribute %s as if it was a non-attribute key!\n", pAttributeName );
		}
	}
	else
	{
		if ( pAttributeName )
			value = m_pAttributes->FindOrCreateMember( id );
		else
			value = m_pKeyValues->FindOrCreateMember( id );
	}

	return value;
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
			if ( bSkipEHandles && pSrc->GetValue( iter )->GetSubType() == KV3_SUBTYPE_EHANDLE )
				continue;
		}

		KeyValues3* value = SetValue( pSrc->GetEntityKeyId( iter ), pAttributeName );

		if ( value )
			*value = *pSrc->GetValue( iter );
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

void CEntityKeyValues::RemoveKey( const EntityKeyId_t &id )
{
	if ( m_nQueuedForSpawnCount > 0 || !m_pAllocator )
		return;

	if ( !m_pKeyValues->RemoveMember( id ) )
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
			m_pKeyValues->SetToEmptyTable();
			m_pAttributes->SetToEmptyTable();
		}
		else
		{
			m_pAllocator->Clear();
			m_pKeyValues = m_pAllocator->AllocKV();
			m_pAttributes = m_pAllocator->AllocKV();
		}
	}
}

bool CEntityKeyValues::IsEmpty() const
{
	if ( !m_pAllocator )
		return true;

	if ( !m_pKeyValues->GetMemberCount() && !m_pAttributes->GetMemberCount() )
		return true;

	return false;
}

bool CEntityKeyValues::KeysHasBadNames() const
{
	if ( !m_pAllocator )
		return false;

	return m_pKeyValues->TableHasBadNames();
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
	const KeyValues3* value = GetValue( id );
	
	if ( !value )
		return defaultValue;

	switch ( value->GetType() )
	{
		case KV3_TYPE_UINT:
			return value->GetEHandle( defaultValue );
#if 0
		case KV3_TYPE_STRING:
			Assert( EntitySystem() );
			return EntitySystem()->FindFirstEntityHandleByName( value->GetString(), worldGroupId );
#endif
		default:
			return defaultValue;
	}
}

void CEntityKeyValues::SetString( KeyValues3* value, const char* string )
{
	ValidateAllocator();

	value->SetStringExternal( m_pAllocator->AllocString( string ) );
}
