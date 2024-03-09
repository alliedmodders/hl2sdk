#ifndef ENTITYKEYVALUES_H
#define ENTITYKEYVALUES_H
#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h"
#include "tier1/keyvalues3.h"
#include "tier1/utlleanvector.h"
#include "entity2/entitysystem.h"

#include "tier0/memdbgon.h"

#define FOR_EACH_ENTITYKEY( ekv, iter ) \
	for ( auto iter = ekv->First(); ekv->IsValidIterator( iter ); iter = ekv->Next( iter ) )

typedef CKV3MemberName EntityKeyId_t;

enum EntityKVAllocatorType_t : uint8
{
	EKV_ALLOCATOR_NORMAL = 0,
	EKV_ALLOCATOR_ENTSYSTEM_1 = 1,
	EKV_ALLOCATOR_ENTSYSTEM_2 = 2,
	EKV_ALLOCATOR_EXTERNAL = 3,
};

struct EntityIOConnectionDescFat_t
{ 
	const char* m_pszOutputName;
	EntityIOTargetType_t m_eTargetType;
	const char* m_pszTargetName;
	const char* m_pszInputName;
	const char* m_pszOverrideParam;
	float m_flDelay;
	int32 m_nTimesToFire;
};

abstract_class IEntityKeyComplex
{
public:
	IEntityKeyComplex() : m_nRefCount( 0 ) {}

	void AddRef() { ++m_nRefCount; }
	void Release() { if ( --m_nRefCount == 0 ) DeleteThis(); }

private:
	virtual void DeleteThis() = 0;

private:
	int32 m_nRefCount;
};

template < class T >
class CEntityKeyComplex : public IEntityKeyComplex
{
public:
	CEntityKeyComplex( const T& obj ) : m_Object( obj ) {}

private:
	virtual void DeleteThis() { delete this; }

public:
	T m_Object;
};

class CEntityKeyValues
{
public:
	CEntityKeyValues( CKeyValues3Context* allocator = NULL, EntityKVAllocatorType_t allocator_type = EKV_ALLOCATOR_NORMAL );
	~CEntityKeyValues();

	class Iterator_t
	{
		Iterator_t( const KeyValues3* _keys, int _index ) : keys( _keys ), index( _index ) {}
		const KeyValues3* keys;
		int index;
		friend class CEntityKeyValues;
	public:
		bool operator==( const Iterator_t it ) const { return keys == it.keys && index == it.index; }
		bool operator!=( const Iterator_t it ) const { return keys != it.keys || index != it.index; }
	};
	Iterator_t First() const;
	Iterator_t Next( const Iterator_t &it ) const;
	bool IsValidIterator( const Iterator_t &it ) const;
	const KeyValues3* GetKeyValue( const Iterator_t &it ) const;
	EntityKeyId_t GetEntityKeyId( const Iterator_t &it ) const;
	const char* GetAttributeName( const Iterator_t &it ) const;
	bool IsAttribute( const Iterator_t &it ) const;

	// Indicates that kv's is in the spawn queue and we should not modify it
	bool IsQueuedForSpawn() const { return m_nQueuedForSpawnCount > 0; }

	bool IsLoggingEnabled() const { return m_bAllowLogging; }
	void EnableLogging( bool bEnable ) { m_bAllowLogging = bEnable; }

	void AddRef();
	void Release();

	void AddConnectionDesc( 
		const char* pszOutputName,	
		EntityIOTargetType_t eTargetType,
		const char* pszTargetName,
		const char* pszInputName,
		const char* pszOverrideParam,
		float flDelay,
		int32 nTimesToFire );

	void RemoveConnectionDesc( int nDesc );

	EntityIOConnectionDescFat_t* GetConnectionDesc( int nDesc ) { return &m_connectionDescs[nDesc]; }
	int GetNumConnectionDescs() const { return m_connectionDescs.Count(); }

	void CopyFrom( const CEntityKeyValues* pSrc, bool bRemoveAllKeys = false, bool bSkipEHandles = false );
	void RemoveKeyValue( const EntityKeyId_t &id );
	void RemoveAllKeys();

	bool IsEmpty() const;
	
	bool ValuesHasBadNames() const;
	bool AttributesHasBadNames() const;

	bool HasValue( const EntityKeyId_t &id ) const { bool bIsAttribute; return ( GetKeyValue( id, &bIsAttribute ) && !bIsAttribute ); }
	bool HasAttribute( const EntityKeyId_t &id ) const { bool bIsAttribute; return ( GetKeyValue( id, &bIsAttribute ) && bIsAttribute ); }

	const KeyValues3* GetKeyValue( const EntityKeyId_t &id, bool* pIsAttribute = NULL ) const;

	//
	// GetKeyValue helpers
	// 
	bool			GetBool( const EntityKeyId_t &id, bool defaultValue = false ) const;
	int				GetInt( const EntityKeyId_t &id, int defaultValue = 0 ) const;
	uint			GetUint( const EntityKeyId_t &id, uint defaultValue = 0 ) const;
	int64			GetInt64( const EntityKeyId_t &id, int64 defaultValue = 0 ) const;
	uint64			GetUint64( const EntityKeyId_t &id, uint64 defaultValue = 0 ) const;
	float			GetFloat( const EntityKeyId_t &id, float defaultValue = 0.0f ) const;
	double			GetDouble( const EntityKeyId_t &id, double defaultValue = 0.0 ) const;
	const char*		GetString( const EntityKeyId_t &id, const char *defaultValue = "" ) const;
	void*			GetPtr( const EntityKeyId_t &id, void *defaultValue = ( void* )0 ) const;
	CUtlStringToken	GetStringToken( const EntityKeyId_t &id, CUtlStringToken defaultValue = CUtlStringToken() ) const;
	CEntityHandle	GetEHandle( const EntityKeyId_t &id, WorldGroupId_t worldGroupId = WorldGroupId_t(), CEntityHandle defaultValue = CEntityHandle() ) const;
	Color			GetColor( const EntityKeyId_t &id, const Color &defaultValue = Color( 0, 0, 0, 255 ) ) const;
	Vector			GetVector( const EntityKeyId_t &id, const Vector &defaultValue = Vector( 0.0f, 0.0f, 0.0f ) ) const;
	Vector2D		GetVector2D( const EntityKeyId_t &id, const Vector2D &defaultValue = Vector2D( 0.0f, 0.0f ) ) const;
	Vector4D		GetVector4D( const EntityKeyId_t &id, const Vector4D &defaultValue = Vector4D( 0.0f, 0.0f, 0.0f, 0.0f ) ) const;
	Quaternion		GetQuaternion( const EntityKeyId_t &id, const Quaternion &defaultValue = Quaternion( 0.0f, 0.0f, 0.0f, 0.0f ) ) const;
	QAngle			GetQAngle( const EntityKeyId_t &id, const QAngle &defaultValue = QAngle( 0.0f, 0.0f, 0.0f ) ) const;
	matrix3x4_t		GetMatrix3x4( const EntityKeyId_t &id, const matrix3x4_t &defaultValue = matrix3x4_t( Vector( 0.0f, 0.0f, 0.0f ), Vector( 0.0f, 0.0f, 0.0f ), Vector( 0.0f, 0.0f, 0.0f ), Vector( 0.0f, 0.0f, 0.0f ) ) ) const;

	void SetBool( const EntityKeyId_t &id, bool value, bool bAsAttribute = false );
	void SetInt( const EntityKeyId_t &id, int value, bool bAsAttribute = false );
	void SetUint( const EntityKeyId_t &id, uint value, bool bAsAttribute = false );
	void SetInt64( const EntityKeyId_t &id, int64 value, bool bAsAttribute = false );
	void SetUint64( const EntityKeyId_t &id, uint64 value, bool bAsAttribute = false );
	void SetFloat( const EntityKeyId_t &id, float value, bool bAsAttribute = false );
	void SetDouble( const EntityKeyId_t &id, double value, bool bAsAttribute = false );
	void SetString( const EntityKeyId_t &id, const char* string, bool bAsAttribute = false );
	void SetPtr( const EntityKeyId_t &id, void* ptr, bool bAsAttribute = false );
	void SetStringToken( const EntityKeyId_t &id, CUtlStringToken token, bool bAsAttribute = false );
	void SetEHandle( const EntityKeyId_t &id, CEntityHandle ehandle, bool bAsAttribute = false );
	void SetColor( const EntityKeyId_t &id, const Color &color, bool bAsAttribute = false );
	void SetVector( const EntityKeyId_t &id, const Vector &vec, bool bAsAttribute = false );
	void SetVector2D( const EntityKeyId_t &id, const Vector2D &vec2d, bool bAsAttribute = false );
	void SetVector4D( const EntityKeyId_t &id, const Vector4D &vec4d, bool bAsAttribute = false );
	void SetQuaternion( const EntityKeyId_t &id, const Quaternion &quat, bool bAsAttribute = false );
	void SetQAngle( const EntityKeyId_t &id, const QAngle &ang, bool bAsAttribute = false );
	void SetMatrix3x4( const EntityKeyId_t &id, const matrix3x4_t &matrix, bool bAsAttribute = false );	

	CEntityKeyValues& operator=( const CEntityKeyValues& src ) { CopyFrom( &src ); return *this; }

private:
	CEntityKeyValues( const CEntityKeyValues& other );

	// Use public setters for all available types instead
	KeyValues3* SetKeyValue( const EntityKeyId_t &id, const char* pAttributeName = NULL );
	void SetString( KeyValues3* kv, const char* string );

	void ReleaseAllComplexKeys();
	void ValidateAllocator();

private:
	struct EntityComplexKeyListElem_t 
	{
		EntityComplexKeyListElem_t( IEntityKeyComplex* pKey, EntityComplexKeyListElem_t* pNext ) : m_pKey( pKey ), m_pNext( pNext ) {}

		IEntityKeyComplex* m_pKey;
		EntityComplexKeyListElem_t* m_pNext;
	};

	CKeyValues3Context* m_pAllocator;
	EntityComplexKeyListElem_t* m_pComplexKeys;
	KeyValues3* m_pValues;
	KeyValues3* m_pAttributes;
	int16 m_nRefCount;
	int16 m_nQueuedForSpawnCount;
	bool m_bAllowLogging;
	EntityKVAllocatorType_t m_eAllocatorType;
	CUtlLeanVector<EntityIOConnectionDescFat_t> m_connectionDescs;
};

inline CEntityKeyValues::Iterator_t CEntityKeyValues::First() const
{
	const KeyValues3* keys = NULL;

	if ( m_pAllocator )
	{
		if ( m_pValues->GetMemberCount() > 0 )
			keys = m_pValues;
		else
			keys = m_pAttributes;
	}

	return CEntityKeyValues::Iterator_t( keys, 0 );
}

inline CEntityKeyValues::Iterator_t CEntityKeyValues::Next( const CEntityKeyValues::Iterator_t &it ) const
{
	const KeyValues3* keys = it.keys;
	int index = it.index + 1;

	if ( index >= keys->GetMemberCount() )
	{
		if ( keys == m_pValues )
		{
			keys = m_pAttributes;
			index = 0;
		}
		else
		{
			index = keys->GetMemberCount();
		}
	}

	return CEntityKeyValues::Iterator_t( keys, index );
}

inline bool CEntityKeyValues::IsValidIterator( const CEntityKeyValues::Iterator_t &it ) const
{
	return ( it.keys != NULL ) && ( it.index < it.keys->GetMemberCount() );
}

inline const KeyValues3* CEntityKeyValues::GetKeyValue( const CEntityKeyValues::Iterator_t &it ) const
{
	if ( it.index >= it.keys->GetMemberCount() )
		return NULL;

	return it.keys->GetMember( it.index );
}

inline EntityKeyId_t CEntityKeyValues::GetEntityKeyId( const CEntityKeyValues::Iterator_t &it ) const
{
	if ( it.index >= it.keys->GetMemberCount() )
		return EntityKeyId_t();

	return it.keys->GetMemberNameEx( it.index );
}

inline const char* CEntityKeyValues::GetAttributeName( const CEntityKeyValues::Iterator_t &it ) const
{
	if ( it.keys == m_pValues )
		return NULL;

	if ( it.index >= it.keys->GetMemberCount() )
		return "<none>";
	
	return it.keys->GetMemberName( it.index );
}

inline bool CEntityKeyValues::IsAttribute( const CEntityKeyValues::Iterator_t &it ) const
{
	return ( it.index < it.keys->GetMemberCount() ) && ( it.keys == m_pAttributes );
}

inline bool CEntityKeyValues::GetBool( const EntityKeyId_t &id, bool defaultValue ) const
{
	const KeyValues3* kv = GetKeyValue( id );
	return kv ? kv->GetBool( defaultValue ) : defaultValue;
}

inline int CEntityKeyValues::GetInt( const EntityKeyId_t &id, int defaultValue ) const
{
	const KeyValues3* kv = GetKeyValue( id );
	return kv ? kv->GetInt( defaultValue ) : defaultValue;
}

inline uint CEntityKeyValues::GetUint( const EntityKeyId_t &id, uint defaultValue ) const
{
	const KeyValues3* kv = GetKeyValue( id );
	return kv ? kv->GetUInt() : defaultValue;
}

inline int64 CEntityKeyValues::GetInt64( const EntityKeyId_t &id, int64 defaultValue ) const
{
	const KeyValues3* kv = GetKeyValue( id );
	return kv ? kv->GetInt64( defaultValue ) : defaultValue;
}

inline uint64 CEntityKeyValues::GetUint64( const EntityKeyId_t &id, uint64 defaultValue ) const
{
	const KeyValues3* kv = GetKeyValue( id );
	return kv ? kv->GetUInt64( defaultValue ) : defaultValue;
}

inline float CEntityKeyValues::GetFloat( const EntityKeyId_t &id, float defaultValue ) const
{
	const KeyValues3* kv = GetKeyValue( id );
	return kv ? kv->GetFloat( defaultValue ) : defaultValue;
}

inline double CEntityKeyValues::GetDouble( const EntityKeyId_t &id, double defaultValue ) const
{
	const KeyValues3* kv = GetKeyValue( id );
	return kv ? kv->GetDouble( defaultValue ) : defaultValue;
}

inline const char* CEntityKeyValues::GetString( const EntityKeyId_t &id, const char *defaultValue ) const
{
	const KeyValues3* kv = GetKeyValue( id );
	return kv ? kv->GetString( defaultValue ) : defaultValue;
}

inline void* CEntityKeyValues::GetPtr( const EntityKeyId_t &id, void *defaultValue ) const
{
	const KeyValues3* kv = GetKeyValue( id );
	return kv ? kv->GetPointer( defaultValue ) : defaultValue;
}

inline CUtlStringToken CEntityKeyValues::GetStringToken( const EntityKeyId_t &id, CUtlStringToken defaultValue ) const
{
	const KeyValues3* kv = GetKeyValue( id );
	return kv ? kv->GetStringToken( defaultValue ) : defaultValue;
}

inline Color CEntityKeyValues::GetColor( const EntityKeyId_t &id, const Color &defaultValue ) const
{
	const KeyValues3* kv = GetKeyValue( id );
	return kv ? kv->GetColor( defaultValue ) : defaultValue;
}

inline Vector CEntityKeyValues::GetVector( const EntityKeyId_t &id, const Vector &defaultValue ) const
{
	const KeyValues3* kv = GetKeyValue( id );
	return kv ? kv->GetVector( defaultValue ) : defaultValue;
}

inline Vector2D CEntityKeyValues::GetVector2D( const EntityKeyId_t &id, const Vector2D &defaultValue ) const
{
	const KeyValues3* kv = GetKeyValue( id );
	return kv ? kv->GetVector2D( defaultValue ) : defaultValue;
}

inline Vector4D CEntityKeyValues::GetVector4D( const EntityKeyId_t &id, const Vector4D &defaultValue ) const
{
	const KeyValues3* kv = GetKeyValue( id );
	return kv ? kv->GetVector4D( defaultValue ) : defaultValue;
}

inline Quaternion CEntityKeyValues::GetQuaternion( const EntityKeyId_t &id, const Quaternion &defaultValue ) const
{
	const KeyValues3* kv = GetKeyValue( id );
	return kv ? kv->GetQuaternion( defaultValue ) : defaultValue;
}

inline QAngle CEntityKeyValues::GetQAngle( const EntityKeyId_t &id, const QAngle &defaultValue ) const
{
	const KeyValues3* kv = GetKeyValue( id );
	return kv ? kv->GetQAngle( defaultValue ) : defaultValue;
}

inline matrix3x4_t CEntityKeyValues::GetMatrix3x4( const EntityKeyId_t &id, const matrix3x4_t &defaultValue ) const
{
	const KeyValues3* kv = GetKeyValue( id );
	return kv ? kv->GetMatrix3x4( defaultValue ) : defaultValue;
}

inline void CEntityKeyValues::SetBool( const EntityKeyId_t &id, bool value, bool bAsAttribute )
{
	KeyValues3* kv = SetKeyValue( id, bAsAttribute ? id.GetString() : NULL );
	if ( kv ) kv->SetBool( value );
}

inline void CEntityKeyValues::SetInt( const EntityKeyId_t &id, int value, bool bAsAttribute )
{
	KeyValues3* kv = SetKeyValue( id, bAsAttribute ? id.GetString() : NULL );
	if ( kv ) kv->SetInt( value );
}

inline void CEntityKeyValues::SetUint( const EntityKeyId_t &id, uint value, bool bAsAttribute )
{
	KeyValues3* val = SetKeyValue( id, bAsAttribute ? id.GetString() : NULL );
	if ( val ) val->SetUInt( value );
}

inline void CEntityKeyValues::SetInt64( const EntityKeyId_t &id, int64 value, bool bAsAttribute )
{
	KeyValues3* kv = SetKeyValue( id, bAsAttribute ? id.GetString() : NULL );
	if ( kv ) kv->SetInt64( value );
}

inline void CEntityKeyValues::SetUint64( const EntityKeyId_t &id, uint64 value, bool bAsAttribute )
{
	KeyValues3* kv = SetKeyValue( id, bAsAttribute ? id.GetString() : NULL );
	if ( kv ) kv->SetUInt64( value );
}

inline void CEntityKeyValues::SetFloat( const EntityKeyId_t &id, float value, bool bAsAttribute )
{
	KeyValues3* kv = SetKeyValue( id, bAsAttribute ? id.GetString() : NULL );
	if ( kv ) kv->SetFloat( value );
}

inline void CEntityKeyValues::SetDouble( const EntityKeyId_t &id, double value, bool bAsAttribute )
{
	KeyValues3* kv = SetKeyValue( id, bAsAttribute ? id.GetString() : NULL );
	if ( kv ) kv->SetDouble( value );
}

inline void CEntityKeyValues::SetString( const EntityKeyId_t &id, const char* string, bool bAsAttribute )
{
	KeyValues3* kv = SetKeyValue( id, bAsAttribute ? id.GetString() : NULL );
	if ( kv ) SetString( kv, string );
}

inline void CEntityKeyValues::SetPtr( const EntityKeyId_t &id, void* ptr, bool bAsAttribute )
{
	KeyValues3* kv = SetKeyValue( id, bAsAttribute ? id.GetString() : NULL );
	if ( kv ) kv->SetPointer( ptr );
}

inline void CEntityKeyValues::SetStringToken( const EntityKeyId_t &id, CUtlStringToken token, bool bAsAttribute )
{
	KeyValues3* kv = SetKeyValue( id, bAsAttribute ? id.GetString() : NULL );
	if ( kv ) kv->SetStringToken( token );
}

inline void CEntityKeyValues::SetEHandle( const EntityKeyId_t &id, CEntityHandle ehandle, bool bAsAttribute )
{
	KeyValues3* kv = SetKeyValue( id, bAsAttribute ? id.GetString() : NULL );
	if ( kv ) kv->SetEHandle( ehandle );
}

inline void CEntityKeyValues::SetColor( const EntityKeyId_t &id, const Color &color, bool bAsAttribute )
{
	KeyValues3* kv = SetKeyValue( id, bAsAttribute ? id.GetString() : NULL );
	if ( kv ) kv->SetColor( color );
}

inline void CEntityKeyValues::SetVector( const EntityKeyId_t &id, const Vector &vec, bool bAsAttribute )
{
	KeyValues3* kv = SetKeyValue( id, bAsAttribute ? id.GetString() : NULL );
	if ( kv ) kv->SetVector( vec );
}

inline void CEntityKeyValues::SetVector2D( const EntityKeyId_t &id, const Vector2D &vec2d, bool bAsAttribute )
{
	KeyValues3* kv = SetKeyValue( id, bAsAttribute ? id.GetString() : NULL );
	if ( kv ) kv->SetVector2D( vec2d );
}

inline void CEntityKeyValues::SetVector4D( const EntityKeyId_t &id, const Vector4D &vec4d, bool bAsAttribute )
{
	KeyValues3* kv = SetKeyValue( id, bAsAttribute ? id.GetString() : NULL );
	if ( kv ) kv->SetVector4D( vec4d );
}

inline void CEntityKeyValues::SetQuaternion( const EntityKeyId_t &id, const Quaternion &quat, bool bAsAttribute )
{
	KeyValues3* kv = SetKeyValue( id, bAsAttribute ? id.GetString() : NULL );
	if ( kv ) kv->SetQuaternion( quat );
}

inline void CEntityKeyValues::SetQAngle( const EntityKeyId_t &id, const QAngle &ang, bool bAsAttribute )
{
	KeyValues3* kv = SetKeyValue( id, bAsAttribute ? id.GetString() : NULL );
	if ( kv ) kv->SetQAngle( ang );
}

inline void CEntityKeyValues::SetMatrix3x4( const EntityKeyId_t &id, const matrix3x4_t &matrix, bool bAsAttribute )
{
	KeyValues3* kv = SetKeyValue( id, bAsAttribute ? id.GetString() : NULL );
	if ( kv ) kv->SetMatrix3x4( matrix );
}

#include "tier0/memdbgoff.h"

#endif // ENTITYKEYVALUES_H
