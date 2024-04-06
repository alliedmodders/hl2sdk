#include "keyvalues3.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Nasty hack to redefine gcc's offsetof which doesn't like GET_OUTER macro
#ifdef COMPILER_GCC
#undef offsetof
#define offsetof(s,m)	(size_t)&(((s *)0)->m)
#endif

KeyValues3::KeyValues3( KV3TypeEx_t type, KV3SubType_t subtype ) : 
	m_bExternalStorage( true ),
	m_TypeEx( type ),
	m_SubType( subtype ),
	m_nFlags( 0 ),
	m_nReserved( 0 ),
	m_nData( 0 )
{
	ResolveUnspecified();
	Alloc();
}

KeyValues3::KeyValues3( int cluster_elem, KV3TypeEx_t type, KV3SubType_t subtype ) : 
	m_bExternalStorage( false ),
	m_TypeEx( type ),
	m_SubType( subtype ),
	m_nFlags( 0 ),
	m_nClusterElement( cluster_elem ),
	m_nReserved( 0 ),
	m_nData( 0 )
{
	ResolveUnspecified();
	Alloc();
}

KeyValues3::~KeyValues3() 
{ 
	Free(); 
};

void KeyValues3::Alloc()
{
	switch ( GetTypeEx() )
	{
		case KV3_TYPEEX_ARRAY:
		{
			CKeyValues3Context* context = GetContext();
			if ( context )
				m_pArray = context->AllocArray();
			else
				m_pArray = new CKeyValues3Array;
			break;
		}
		case KV3_TYPEEX_TABLE:
		{
			CKeyValues3Context* context = GetContext();
			if ( context )
				m_pTable = context->AllocTable();
			else
				m_pTable = new CKeyValues3Table;
			break;
		}
		case KV3_TYPEEX_ARRAY_FLOAT32:
		case KV3_TYPEEX_ARRAY_FLOAT64:
		case KV3_TYPEEX_ARRAY_INT16:
		case KV3_TYPEEX_ARRAY_INT32:
		case KV3_TYPEEX_ARRAY_UINT8_SHORT:
		case KV3_TYPEEX_ARRAY_INT16_SHORT:
		{
			m_bFreeArrayMemory = false;
			m_nNumArrayElements = 0;
			m_nData = 0;
			break;
		}
		default: 
			break;
	}
}

void KeyValues3::Free( bool bClearingContext )
{
	switch ( GetTypeEx() )
	{
		case KV3_TYPEEX_STRING:
		{
			free( (void*)m_pString );
			m_pString = NULL;
			break;
		}
		case KV3_TYPEEX_BINARY_BLOB:
		{
			if ( m_pBinaryBlob )
				free( m_pBinaryBlob );
			m_pBinaryBlob = NULL;
			break;
		}
		case KV3_TYPEEX_BINARY_BLOB_EXTERN:
		{
			if ( m_pBinaryBlob )
			{
				if ( m_pBinaryBlob->m_bFreeMemory )
					free( (void*)m_pBinaryBlob->m_pubData );
				free( m_pBinaryBlob );
			}
			m_pBinaryBlob = NULL;
			break;
		}
		case KV3_TYPEEX_ARRAY:
		{
			m_pArray->Purge( bClearingContext );

			CKeyValues3Context* context = GetContext();

			if ( context )
			{
				if ( !bClearingContext )
					context->FreeArray( m_pArray );
			}
			else
			{
				delete m_pArray;
			}

			m_pArray = NULL;
			break;
		}
		case KV3_TYPEEX_TABLE:
		{
			m_pTable->Purge( bClearingContext );

			CKeyValues3Context* context = GetContext();

			if ( context )
			{
				if ( !bClearingContext )
					context->FreeTable( m_pTable );
			}
			else
			{
				delete m_pTable;
			}

			m_pTable = NULL;
			break;
		}
		case KV3_TYPEEX_ARRAY_FLOAT32:
		case KV3_TYPEEX_ARRAY_FLOAT64:
		case KV3_TYPEEX_ARRAY_INT16:
		case KV3_TYPEEX_ARRAY_INT32:
		case KV3_TYPEEX_ARRAY_UINT8_SHORT:
		case KV3_TYPEEX_ARRAY_INT16_SHORT:
		{
			if ( m_bFreeArrayMemory )
				free( m_pData );
			m_bFreeArrayMemory = false;
			m_nNumArrayElements = 0;
			m_nData = 0;
			break;
		}
		default: 
			break;
	}
}

void KeyValues3::ResolveUnspecified()
{
	if ( GetSubType() == KV3_SUBTYPE_UNSPECIFIED )
	{
		switch ( GetType() )
		{
			case KV3_TYPE_NULL:
				m_SubType = KV3_SUBTYPE_NULL;
				break;
			case KV3_TYPE_BOOL:
				m_SubType = KV3_SUBTYPE_BOOL8;
				break;
			case KV3_TYPE_INT:
				m_SubType = KV3_SUBTYPE_INT64;
				break;
			case KV3_TYPE_UINT:
				m_SubType = KV3_SUBTYPE_UINT64;
				break;
			case KV3_TYPE_DOUBLE:
				m_SubType = KV3_SUBTYPE_FLOAT64;
				break;
			case KV3_TYPE_STRING:
				m_SubType = KV3_SUBTYPE_STRING;
				break;
			case KV3_TYPE_BINARY_BLOB:
				m_SubType = KV3_SUBTYPE_BINARY_BLOB;
				break;
			case KV3_TYPE_ARRAY:
				m_SubType = KV3_SUBTYPE_ARRAY;
				break;
			case KV3_TYPE_TABLE:
				m_SubType = KV3_SUBTYPE_TABLE;
				break;
			default:
				m_SubType = KV3_SUBTYPE_INVALID;
				break;
		}
	}
}

void KeyValues3::PrepareForType( KV3TypeEx_t type, KV3SubType_t subtype )
{
	if ( GetTypeEx() == type )
	{
		switch ( type )
		{
			case KV3_TYPEEX_STRING:
			case KV3_TYPEEX_BINARY_BLOB:
			case KV3_TYPEEX_BINARY_BLOB_EXTERN:
			case KV3_TYPEEX_ARRAY_FLOAT32:
			case KV3_TYPEEX_ARRAY_FLOAT64:
			case KV3_TYPEEX_ARRAY_INT16:
			case KV3_TYPEEX_ARRAY_INT32:
			case KV3_TYPEEX_ARRAY_UINT8_SHORT:
			case KV3_TYPEEX_ARRAY_INT16_SHORT:
			{
				Free();
				break;
			}
			default: 
				break;
		}
	}
	else
	{
		Free();
		m_TypeEx = type;
		m_nData = 0;
		Alloc();
	}

	m_SubType = subtype;
}

void KeyValues3::OnClearContext()
{ 
	Free( true ); 
	m_TypeEx = KV3_TYPEEX_NULL; 
	m_nData = 0; 
}

CKeyValues3Cluster* KeyValues3::GetCluster() const
{
	if ( m_bExternalStorage )
		return NULL;

	return GET_OUTER( CKeyValues3Cluster, m_KeyValues[ m_nClusterElement ] );
}

CKeyValues3Context* KeyValues3::GetContext() const
{ 
	CKeyValues3Cluster* cluster = GetCluster();

	if ( cluster )
		return cluster->GetContext();
	else
		return NULL;
}

KV3MetaData_t* KeyValues3::GetMetaData( CKeyValues3Context** ppCtx ) const
{
	CKeyValues3Cluster* cluster = GetCluster();

	if ( cluster )
	{
		if ( ppCtx )
			*ppCtx = cluster->GetContext();

		return cluster->GetMetaData( m_nClusterElement );
	}
	else
	{
		if ( ppCtx )
			*ppCtx = NULL;

		return NULL;
	}
}

const char* KeyValues3::GetString( const char* defaultValue ) const
{
	switch ( GetTypeEx() )
	{
		case KV3_TYPEEX_STRING:
		case KV3_TYPEEX_STRING_EXTERN:
			return m_pString;
		case KV3_TYPEEX_STRING_SHORT:
			return m_szStringShort;
		default:
			return defaultValue;
	}
}

void KeyValues3::SetString( const char* pString, KV3SubType_t subtype )
{
	if ( !pString )
		pString = "";

	if ( strlen( pString ) < sizeof( m_szStringShort ) )
	{
		PrepareForType( KV3_TYPEEX_STRING_SHORT, subtype );
		V_strncpy( m_szStringShort, pString, sizeof( m_szStringShort ) );
	}
	else
	{
		PrepareForType( KV3_TYPEEX_STRING, subtype );
		m_pString = strdup( pString );
	}
}

void KeyValues3::SetStringExternal( const char* pString, KV3SubType_t subtype )
{
	if ( strlen( pString ) < sizeof( m_szStringShort ) )
	{
		PrepareForType( KV3_TYPEEX_STRING_SHORT, subtype );
		V_strncpy( m_szStringShort, pString, sizeof( m_szStringShort ) );
	}
	else
	{
		PrepareForType( KV3_TYPEEX_STRING_EXTERN, subtype );
		m_pString = pString;
	}
}

const byte* KeyValues3::GetBinaryBlob() const
{
	switch ( GetTypeEx() )
	{
		case KV3_TYPEEX_BINARY_BLOB:
			return m_pBinaryBlob ? m_pBinaryBlob->m_ubData : NULL;
		case KV3_TYPEEX_BINARY_BLOB_EXTERN:
			return m_pBinaryBlob ? m_pBinaryBlob->m_pubData : NULL;
		default:
			return NULL;
	}
}

int KeyValues3::GetBinaryBlobSize() const
{
	if ( GetType() != KV3_TYPE_BINARY_BLOB || !m_pBinaryBlob )
		return 0;

	return ( int )m_pBinaryBlob->m_nSize;
}

void KeyValues3::SetToBinaryBlob( const byte* blob, int size )
{
	PrepareForType( KV3_TYPEEX_BINARY_BLOB, KV3_SUBTYPE_BINARY_BLOB );

	if ( size > 0 )
	{
		m_pBinaryBlob = (KV3BinaryBlob_t*)malloc( sizeof( size_t ) + size );
		m_pBinaryBlob->m_nSize = size;
		memcpy( m_pBinaryBlob->m_ubData, blob, size );
	}
	else
	{
		m_pBinaryBlob = NULL;
	}
}

void KeyValues3::SetToBinaryBlobExternal( const byte* blob, int size, bool free_mem )
{
	PrepareForType( KV3_TYPEEX_BINARY_BLOB_EXTERN, KV3_SUBTYPE_BINARY_BLOB );

	if ( size > 0 )
	{
		m_pBinaryBlob = (KV3BinaryBlob_t*)malloc( sizeof( KV3BinaryBlob_t ) );
		m_pBinaryBlob->m_nSize = size;
		m_pBinaryBlob->m_pubData = blob;
		m_pBinaryBlob->m_bFreeMemory = free_mem;
	}
	else
	{
		m_pBinaryBlob = NULL;
	}
}

Color KeyValues3::GetColor( const Color &defaultValue ) const
{
	int32 color[4];
	if ( ReadArrayInt32( 4, color ) )
	{
		return Color( color[0], color[1], color[2], color[3] );
	}
	else if ( ReadArrayInt32( 3, color ) )
	{
		return Color( color[0], color[1], color[2], 255 );
	}
	else
	{
		return defaultValue;
	}
}

void KeyValues3::SetColor( const Color &color )
{
	if ( color.a() == 255 )
		AllocArray<uint8>( 3, &color[0], KV3_ARRAY_ALLOC_NORMAL, KV3_TYPEEX_ARRAY_UINT8_SHORT, KV3_TYPEEX_INVALID, KV3_SUBTYPE_COLOR32, KV3_TYPEEX_UINT, KV3_SUBTYPE_UINT8 );
	else
		AllocArray<uint8>( 4, &color[0], KV3_ARRAY_ALLOC_NORMAL, KV3_TYPEEX_ARRAY_UINT8_SHORT, KV3_TYPEEX_INVALID, KV3_SUBTYPE_COLOR32, KV3_TYPEEX_UINT, KV3_SUBTYPE_UINT8 );
}

int KeyValues3::GetArrayElementCount() const
{
	if ( GetType() != KV3_TYPE_ARRAY )
		return 0;

	if ( GetTypeEx() == KV3_TYPEEX_ARRAY )
		return m_pArray->Count();
	else
		return m_nNumArrayElements;
}

KeyValues3** KeyValues3::GetArrayBase()
{
	if ( GetType() != KV3_TYPE_ARRAY )
		return NULL;

	NormalizeArray();

	return m_pArray->Base();
}

KeyValues3* KeyValues3::GetArrayElement( int elem )
{
	if ( GetType() != KV3_TYPE_ARRAY )
		return NULL;

	NormalizeArray();

	if ( elem < 0 || elem >= m_pArray->Count() )
		return NULL;

	return m_pArray->Element( elem );
}

KeyValues3* KeyValues3::InsertArrayElementBefore( int elem )
{
	if ( GetType() != KV3_TYPE_ARRAY )
		return NULL;

	NormalizeArray();

	return *m_pArray->InsertBeforeGetPtr( elem, 1 );
}

KeyValues3* KeyValues3::AddArrayElementToTail()
{
	if ( GetType() != KV3_TYPE_ARRAY )
		PrepareForType( KV3_TYPEEX_ARRAY, KV3_SUBTYPE_ARRAY );
	else
		NormalizeArray();

	return *m_pArray->InsertBeforeGetPtr( m_pArray->Count(), 1 );
}

void KeyValues3::SetArrayElementCount( int count, KV3TypeEx_t type, KV3SubType_t subtype )
{
	if ( GetType() != KV3_TYPE_ARRAY )
		PrepareForType( KV3_TYPEEX_ARRAY, KV3_SUBTYPE_ARRAY );
	else
		NormalizeArray();

	m_pArray->SetCount( count, type, subtype );
}

void KeyValues3::RemoveArrayElements( int elem, int num )
{
	if ( GetType() != KV3_TYPE_ARRAY )
		return;

	NormalizeArray();

	m_pArray->RemoveMultiple( elem, num );
}

void KeyValues3::NormalizeArray()
{
	switch ( GetTypeEx() )
	{
		case KV3_TYPEEX_ARRAY_FLOAT32:
		{
			NormalizeArray<float32>( KV3_TYPEEX_DOUBLE, KV3_SUBTYPE_FLOAT32, m_nNumArrayElements, m_f32Array, m_bFreeArrayMemory );
			break;
		}
		case KV3_TYPEEX_ARRAY_FLOAT64:
		{
			NormalizeArray<float64>( KV3_TYPEEX_DOUBLE, KV3_SUBTYPE_FLOAT64, m_nNumArrayElements, m_f64Array, m_bFreeArrayMemory );
			break;
		}
		case KV3_TYPEEX_ARRAY_INT16:
		{
			NormalizeArray<int16>( KV3_TYPEEX_INT, KV3_SUBTYPE_INT16, m_nNumArrayElements, m_i16Array, m_bFreeArrayMemory );
			break;
		}
		case KV3_TYPEEX_ARRAY_INT32:
		{
			NormalizeArray<int32>( KV3_TYPEEX_INT, KV3_SUBTYPE_INT32, m_nNumArrayElements, m_i32Array, m_bFreeArrayMemory );
			break;
		}
		case KV3_TYPEEX_ARRAY_UINT8_SHORT:
		{
			uint8 u8ArrayShort[8];
			memcpy( u8ArrayShort, m_u8ArrayShort, sizeof( u8ArrayShort ) );
			NormalizeArray<uint8>( KV3_TYPEEX_UINT, KV3_SUBTYPE_UINT8, m_nNumArrayElements, u8ArrayShort, false );
			break;
		}
		case KV3_TYPEEX_ARRAY_INT16_SHORT:
		{
			int16 i16ArrayShort[4];
			memcpy( i16ArrayShort, m_i16ArrayShort, sizeof( i16ArrayShort ) );
			NormalizeArray<int16>( KV3_TYPEEX_INT, KV3_SUBTYPE_INT16, m_nNumArrayElements, i16ArrayShort, false );
			break;
		}
		default: 
			break;
	}
}

bool KeyValues3::ReadArrayInt32( int dest_size, int32* data ) const
{
	int src_size = 0;

	if ( GetType() == KV3_TYPE_STRING )
	{
		CSplitString values( GetString(), " " );
		src_size = values.Count();
		int count = MIN( src_size, dest_size );
		for ( int i = 0; i < count; ++i )
			data[ i ] = V_StringToInt32( values[ i ], 0, NULL, NULL, PARSING_FLAG_SKIP_ASSERT );
	}
	else
	{
		switch ( GetTypeEx() )
		{
			case KV3_TYPEEX_ARRAY:
			{
				src_size = m_pArray->Count();
				int count = MIN( src_size, dest_size );
				KeyValues3** arr = m_pArray->Base();
				for ( int i = 0; i < count; ++i )
					data[ i ] = arr[ i ]->GetInt();
				break;
			}
			case KV3_TYPEEX_ARRAY_INT16:
			{
				src_size = m_nNumArrayElements;
				int count = MIN( src_size, dest_size );
				for ( int i = 0; i < count; ++i )
					data[ i ] = ( int32 )m_i16Array[ i ];
				break;
			}
			case KV3_TYPEEX_ARRAY_INT32:
			{
				src_size = m_nNumArrayElements;
				int count = MIN( src_size, dest_size );
				memcpy( data, m_i32Array, count * sizeof( int32 ) );
				break;
			}
			case KV3_TYPEEX_ARRAY_UINT8_SHORT:
			{
				src_size = m_nNumArrayElements;
				int count = MIN( src_size, dest_size );
				for ( int i = 0; i < count; ++i )
					data[ i ] = ( int32 )m_u8ArrayShort[ i ];
				break;
			}
			case KV3_TYPEEX_ARRAY_INT16_SHORT:
			{
				src_size = m_nNumArrayElements;
				int count = MIN( src_size, dest_size );
				for ( int i = 0; i < count; ++i )
					data[ i ] = ( int32 )m_i16ArrayShort[ i ];
				break;
			}
			default: 
				break;
		}
	}

	if ( src_size < dest_size )
		memset( &data[ src_size ], 0, ( dest_size - src_size ) * sizeof( int32 ) ); 

	return ( src_size == dest_size );
}

bool KeyValues3::ReadArrayFloat32( int dest_size, float32* data ) const
{
	int src_size = 0;

	if ( GetType() == KV3_TYPE_STRING )
	{
		CSplitString values( GetString(), " " );
		src_size = values.Count();
		int count = MIN( src_size, dest_size );
		for ( int i = 0; i < count; ++i )
			data[ i ] = ( float32 )V_StringToFloat64( values[ i ], 0, NULL, NULL, PARSING_FLAG_SKIP_ASSERT );
	}
	else
	{
		switch ( GetTypeEx() )
		{
			case KV3_TYPEEX_ARRAY:
			{
				src_size = m_pArray->Count();
				int count = MIN( src_size, dest_size );
				KeyValues3** arr = m_pArray->Base();
				for ( int i = 0; i < count; ++i )
					data[ i ] = arr[ i ]->GetFloat();
				break;
			}
			case KV3_TYPEEX_ARRAY_FLOAT32:
			{
				src_size = m_nNumArrayElements;
				int count = MIN( src_size, dest_size );
				memcpy( data, m_f32Array, count * sizeof( float32 ) );
				break;
			}
			case KV3_TYPEEX_ARRAY_FLOAT64:
			{
				src_size = m_nNumArrayElements;
				int count = MIN( src_size, dest_size );
				for ( int i = 0; i < count; ++i )
					data[ i ] = ( float32 )m_f64Array[ i ];
				break;
			}
			default: 
				break;
		}
	}

	if ( src_size < dest_size )
		memset( &data[ src_size ], 0, ( dest_size - src_size ) * sizeof( float32 ) ); 

	return ( src_size == dest_size );
}

int KeyValues3::GetMemberCount() const
{
	if ( GetType() != KV3_TYPE_TABLE )
		return 0;
	
	return m_pTable->GetMemberCount();
}

KeyValues3* KeyValues3::GetMember( KV3MemberId_t id )
{
	if ( GetType() != KV3_TYPE_TABLE || id < 0 || id >= m_pTable->GetMemberCount() )
		return NULL;
	
	return m_pTable->GetMember( id );
}

const char* KeyValues3::GetMemberName( KV3MemberId_t id ) const
{
	if ( GetType() != KV3_TYPE_TABLE || id < 0 || id >= m_pTable->GetMemberCount() )
		return NULL;
	
	return m_pTable->GetMemberName( id );
}

CKV3MemberName KeyValues3::GetMemberNameEx( KV3MemberId_t id ) const
{
	if ( GetType() != KV3_TYPE_TABLE || id < 0 || id >= m_pTable->GetMemberCount() )
		return CKV3MemberName();

	return CKV3MemberName( m_pTable->GetMemberHash( id ), m_pTable->GetMemberName( id ) );
}

unsigned int KeyValues3::GetMemberHash( KV3MemberId_t id ) const
{
	if ( GetType() != KV3_TYPE_TABLE || id < 0 || id >= m_pTable->GetMemberCount() )
		return 0;
	
	return m_pTable->GetMemberHash( id );
}

KeyValues3* KeyValues3::FindMember( const CKV3MemberName &name, KeyValues3* defaultValue )
{
	if ( GetType() != KV3_TYPE_TABLE )
		return defaultValue;

	KV3MemberId_t id = m_pTable->FindMember( name );

	if ( id == KV3_INVALID_MEMBER )
		return defaultValue;

	return m_pTable->GetMember( id );
}

KeyValues3* KeyValues3::FindOrCreateMember( const CKV3MemberName &name, bool *pCreated )
{
	if ( GetType() != KV3_TYPE_TABLE )
		PrepareForType( KV3_TYPEEX_TABLE, KV3_SUBTYPE_TABLE );

	KV3MemberId_t id = m_pTable->FindMember( name );

	if ( id == KV3_INVALID_MEMBER )
	{
		if ( pCreated )
			*pCreated = true;

		id = m_pTable->CreateMember( name );
	}
	else
	{
		if ( pCreated )
			*pCreated = false;
	}

	return m_pTable->GetMember( id );
}

bool KeyValues3::TableHasBadNames() const
{
	if ( GetType() != KV3_TYPE_TABLE )
		return false;

	return m_pTable->HasBadNames();
}

void KeyValues3::SetTableHasBadNames( bool bHasBadNames )
{
	if ( GetType() != KV3_TYPE_TABLE )
		return;

	m_pTable->SetHasBadNames( bHasBadNames );
}

void KeyValues3::SetToEmptyTable()
{
	PrepareForType( KV3_TYPEEX_TABLE, KV3_SUBTYPE_TABLE );
	m_pTable->RemoveAll();
}

bool KeyValues3::RemoveMember( KV3MemberId_t id )
{
	if ( GetType() != KV3_TYPE_TABLE || id < 0 || id >= m_pTable->GetMemberCount() )
		return false;

	m_pTable->RemoveMember( id );

	return true;
}

bool KeyValues3::RemoveMember( const KeyValues3* kv )
{
	if ( GetType() != KV3_TYPE_TABLE )
		return false;

	KV3MemberId_t id = m_pTable->FindMember( kv );

	if ( id == KV3_INVALID_MEMBER )
		return false;

	m_pTable->RemoveMember( id );

	return true;
}

bool KeyValues3::RemoveMember( const CKV3MemberName &name )
{
	if ( GetType() != KV3_TYPE_TABLE )
		return false;

	KV3MemberId_t id = m_pTable->FindMember( name );

	if ( id == KV3_INVALID_MEMBER )
		return false;

	m_pTable->RemoveMember( id );

	return true;
}

const char* KeyValues3::GetTypeAsString() const
{
	static const char* s_Types[] =
	{
		"invalid",
		"null",
		"bool",
		"int",
		"uint",
		"double",
		"string",
		"binary_blob",
		"array",
		"table",
		NULL
	};

	KV3Type_t type = GetType();

	if ( type < KV3_TYPE_COUNT )
		return s_Types[type];

	return "<unknown>";
}

const char* KeyValues3::GetSubTypeAsString() const
{
	static const char* s_SubTypes[] =
	{
		"invalid",
		"resource",
		"resource_name",
		"panorama",
		"soundevent",
		"subclass",
		"entity_name",
		"unspecified",
		"null",
		"binary_blob",
		"array",
		"table",
		"bool8",
		"char8",
		"uchar32",
		"int8",
		"uint8",
		"int16",
		"uint16",
		"int32",
		"uint32",
		"int64",
		"uint64",
		"float32",
		"float64",
		"string",
		"pointer",
		"color32",
		"vector",
		"vector2d",
		"vector4d",
		"rotation_vector",
		"quaternion",
		"qangle",
		"matrix3x4",
		"transform",
		"string_token",
		"ehandle",
		NULL
	};

	KV3SubType_t subtype = GetSubType();

	if ( subtype < KV3_SUBTYPE_COUNT )
		return s_SubTypes[subtype];

	return "<unknown>";
}

const char* KeyValues3::ToString( CBufferString& buff, uint flags ) const
{
	if ( ( flags & KV3_TO_STRING_DONT_CLEAR_BUFF ) != 0 )
		flags &= ~KV3_TO_STRING_DONT_APPEND_STRINGS;
	else
		buff.ToGrowable()->Clear();

	KV3Type_t type = GetType();

	switch ( type )
	{
		case KV3_TYPE_NULL:
		{
			return buff.ToGrowable()->Get();
		}
		case KV3_TYPE_BOOL:
		{
			const char* str = m_Bool ? "true" : "false";

			if ( ( flags & KV3_TO_STRING_DONT_APPEND_STRINGS ) != 0 )
				return str;

			buff.Insert( buff.ToGrowable()->GetTotalNumber(), str );
			return buff.ToGrowable()->Get();
		}
		case KV3_TYPE_INT:
		{
			buff.AppendFormat( "%lld", m_Int );
			return buff.ToGrowable()->Get();
		}
		case KV3_TYPE_UINT:
		{
			if ( GetSubType() == KV3_SUBTYPE_POINTER )
			{
				if ( ( flags & KV3_TO_STRING_APPEND_ONLY_NUMERICS ) == 0 )
					buff.Insert( buff.ToGrowable()->GetTotalNumber(), "<pointer>" );

				if ( ( flags & KV3_TO_STRING_RETURN_NON_NUMERICS ) == 0 )
					return NULL;

				return buff.ToGrowable()->Get();
			}
			
			buff.AppendFormat( "%llu", m_UInt );
			return buff.ToGrowable()->Get();
		}
		case KV3_TYPE_DOUBLE:
		{
			buff.AppendFormat( "%g", m_Double );
			return buff.ToGrowable()->Get();
		}
		case KV3_TYPE_STRING:
		{
			const char* str = GetString();

			if ( ( flags & KV3_TO_STRING_DONT_APPEND_STRINGS ) != 0 )
				return str;

			buff.Insert( buff.ToGrowable()->GetTotalNumber(), str );
			return buff.ToGrowable()->Get();
		}
		case KV3_TYPE_BINARY_BLOB:
		{
			if ( ( flags & KV3_TO_STRING_APPEND_ONLY_NUMERICS ) == 0 )
				buff.AppendFormat( "<binary blob: %u bytes>", GetBinaryBlobSize() );

			if ( ( flags & KV3_TO_STRING_RETURN_NON_NUMERICS ) == 0 )
				return NULL;

			return buff.ToGrowable()->Get();
		}
		case KV3_TYPE_ARRAY:
		{
			int elements = GetArrayElementCount();

			if ( elements > 0 && elements <= 4 )
			{
				switch ( GetTypeEx() )
				{
					case KV3_TYPEEX_ARRAY:
					{
						bool unprintable = false;
						CBufferStringGrowable<128> temp;

						KeyValues3** arr = m_pArray->Base();
						for ( int i = 0; i < elements; ++i )
						{
							switch ( arr[i]->GetType() )
							{
								case KV3_TYPE_INT:
									temp.AppendFormat( "%lld", arr[i]->m_Int );
									break;
								case KV3_TYPE_UINT:
									if ( arr[i]->GetSubType() == KV3_SUBTYPE_POINTER )
										unprintable = true;
									else
										temp.AppendFormat( "%llu", arr[i]->m_UInt );
									break;
								case KV3_TYPE_DOUBLE:
									temp.AppendFormat( "%g", arr[i]->m_Double );
									break;
								default:
									unprintable = true;
									break;
							}

							if ( unprintable )
								break;

							if ( i != elements - 1 ) temp.Insert( temp.ToGrowable()->GetTotalNumber(), " " );
						}

						if ( unprintable )
							break;

						buff.Insert( buff.ToGrowable()->GetTotalNumber(), temp.Get() );
						return buff.ToGrowable()->Get();
					}
					case KV3_TYPEEX_ARRAY_FLOAT32:
					{
						for ( int i = 0; i < elements; ++i )
						{
							buff.AppendFormat( "%g", m_f32Array[i] );
							if ( i != elements - 1 ) buff.Insert( buff.ToGrowable()->GetTotalNumber(), " " );
						}
						return buff.ToGrowable()->Get();
					}
					case KV3_TYPEEX_ARRAY_FLOAT64:
					{
						for ( int i = 0; i < elements; ++i )
						{
							buff.AppendFormat( "%g", m_f64Array[i] );
							if ( i != elements - 1 ) buff.Insert( buff.ToGrowable()->GetTotalNumber(), " " );
						}
						return buff.ToGrowable()->Get();
					}
					case KV3_TYPEEX_ARRAY_INT16:
					{
						for ( int i = 0; i < elements; ++i )
						{
							buff.AppendFormat( "%d", m_i16Array[i] );
							if ( i != elements - 1 ) buff.Insert( buff.ToGrowable()->GetTotalNumber(), " " );
						}
						return buff.ToGrowable()->Get();
					}
					case KV3_TYPEEX_ARRAY_INT32:
					{
						for ( int i = 0; i < elements; ++i )
						{
							buff.AppendFormat( "%d", m_i32Array[i] );
							if ( i != elements - 1 ) buff.Insert( buff.ToGrowable()->GetTotalNumber(), " " );
						}
						return buff.ToGrowable()->Get();
					}
					case KV3_TYPEEX_ARRAY_UINT8_SHORT:
					{
						for ( int i = 0; i < elements; ++i )
						{
							buff.AppendFormat( "%u", m_u8ArrayShort[i] );
							if ( i != elements - 1 ) buff.Insert( buff.ToGrowable()->GetTotalNumber(), " " );
						}
						return buff.ToGrowable()->Get();
					}
					case KV3_TYPEEX_ARRAY_INT16_SHORT:
					{
						for ( int i = 0; i < elements; ++i )
						{
							buff.AppendFormat( "%d", m_i16ArrayShort[i] );
							if ( i != elements - 1 ) buff.Insert( buff.ToGrowable()->GetTotalNumber(), " " );
						}
						return buff.ToGrowable()->Get();
					}
					default:
						break;
				}
			}

			if ( ( flags & KV3_TO_STRING_APPEND_ONLY_NUMERICS ) == 0 )
				buff.AppendFormat( "<array: %u elements>", elements );

			if ( ( flags & KV3_TO_STRING_RETURN_NON_NUMERICS ) == 0 )
				return NULL;

			return buff.ToGrowable()->Get();
		}
		case KV3_TYPE_TABLE:
		{
			if ( ( flags & KV3_TO_STRING_APPEND_ONLY_NUMERICS ) == 0 )
				buff.AppendFormat( "<table: %u members>", GetMemberCount() );

			if ( ( flags & KV3_TO_STRING_RETURN_NON_NUMERICS ) == 0 )
				return NULL;

			return buff.ToGrowable()->Get();
		}
		default: 
		{
			if ( ( flags & KV3_TO_STRING_APPEND_ONLY_NUMERICS ) == 0 )
				buff.AppendFormat( "<unknown KV3 basic type '%s' (%d)>", GetTypeAsString(), type );

			if ( ( flags & KV3_TO_STRING_RETURN_NON_NUMERICS ) == 0 )
				return NULL;

			return buff.ToGrowable()->Get();
		}
	}
}

void KeyValues3::CopyFrom( const KeyValues3* pSrc )
{
	SetToNull();

	CKeyValues3Context* context;
	KV3MetaData_t* pDestMetaData = GetMetaData( &context );

	if ( pDestMetaData )
	{
		KV3MetaData_t* pSrcMetaData = pSrc->GetMetaData();

		if ( pSrcMetaData )
			context->CopyMetaData( pDestMetaData, pSrcMetaData );
		else
			pDestMetaData->Clear();
	}

	KV3SubType_t eSrcSubType = pSrc->GetSubType();

	switch ( pSrc->GetType() )
	{
		case KV3_TYPE_BOOL:
			SetBool( pSrc->m_Bool );
			break;
		case KV3_TYPE_INT:
			SetValue<int64>( pSrc->m_Int, KV3_TYPEEX_INT, eSrcSubType );
			break;
		case KV3_TYPE_UINT:
			SetValue<uint64>( pSrc->m_UInt, KV3_TYPEEX_UINT, eSrcSubType );
			break;
		case KV3_TYPE_DOUBLE:
			SetValue<float64>( pSrc->m_Double, KV3_TYPEEX_DOUBLE, eSrcSubType );
			break;
		case KV3_TYPE_STRING:
			SetString( pSrc->GetString(), eSrcSubType );
			break;
		case KV3_TYPE_BINARY_BLOB:
			SetToBinaryBlob( pSrc->GetBinaryBlob(), pSrc->GetBinaryBlobSize() );
			break;
		case KV3_TYPE_ARRAY:
		{
			switch ( pSrc->GetTypeEx() )
			{
				case KV3_TYPEEX_ARRAY:
				{
					PrepareForType( KV3_TYPEEX_ARRAY, KV3_SUBTYPE_ARRAY );
					m_pArray->CopyFrom( pSrc->m_pArray );
					break;
				}
				case KV3_TYPEEX_ARRAY_FLOAT32:
					AllocArray<float32>( pSrc->m_nNumArrayElements, pSrc->m_f32Array, KV3_ARRAY_ALLOC_NORMAL, KV3_TYPEEX_INVALID, KV3_TYPEEX_ARRAY_FLOAT32, eSrcSubType, KV3_TYPEEX_DOUBLE, KV3_SUBTYPE_FLOAT32 );
					break;
				case KV3_TYPEEX_ARRAY_FLOAT64:
					AllocArray<float64>( pSrc->m_nNumArrayElements, pSrc->m_f64Array, KV3_ARRAY_ALLOC_NORMAL, KV3_TYPEEX_INVALID, KV3_TYPEEX_ARRAY_FLOAT64, eSrcSubType, KV3_TYPEEX_DOUBLE, KV3_SUBTYPE_FLOAT64 );
					break;
				case KV3_TYPEEX_ARRAY_INT16:
					AllocArray<int16>( pSrc->m_nNumArrayElements, pSrc->m_i16Array, KV3_ARRAY_ALLOC_NORMAL, KV3_TYPEEX_ARRAY_INT16_SHORT, KV3_TYPEEX_ARRAY_INT16, eSrcSubType, KV3_TYPEEX_INT, KV3_SUBTYPE_INT16 );
					break;
				case KV3_TYPEEX_ARRAY_INT32:
					AllocArray<int32>( pSrc->m_nNumArrayElements, pSrc->m_i32Array, KV3_ARRAY_ALLOC_NORMAL, KV3_TYPEEX_INVALID, KV3_TYPEEX_ARRAY_INT32, eSrcSubType, KV3_TYPEEX_INT, KV3_SUBTYPE_INT32 );
					break;
				case KV3_TYPEEX_ARRAY_UINT8_SHORT:
					AllocArray<uint8>( pSrc->m_nNumArrayElements, pSrc->m_u8ArrayShort, KV3_ARRAY_ALLOC_NORMAL, KV3_TYPEEX_ARRAY_UINT8_SHORT, KV3_TYPEEX_INVALID, eSrcSubType, KV3_TYPEEX_UINT, KV3_SUBTYPE_UINT8 );
					break;
				case KV3_TYPEEX_ARRAY_INT16_SHORT:
					AllocArray<int16>( pSrc->m_nNumArrayElements, pSrc->m_i16ArrayShort, KV3_ARRAY_ALLOC_NORMAL, KV3_TYPEEX_ARRAY_INT16_SHORT, KV3_TYPEEX_ARRAY_INT16, eSrcSubType, KV3_TYPEEX_INT, KV3_SUBTYPE_INT16 );
					break;
				default:
					break;
			}
			break;
		}
		case KV3_TYPE_TABLE:
		{
			PrepareForType( KV3_TYPEEX_TABLE, KV3_SUBTYPE_TABLE );
			m_pTable->CopyFrom( pSrc->m_pTable );
			break;
		}
		default:
			break;
	}

	m_SubType = eSrcSubType;
	m_nFlags = pSrc->m_nFlags;
}

KeyValues3& KeyValues3::operator=( const KeyValues3& src )
{
	if ( this == &src )
		return *this;

	CopyFrom( &src );

	return *this;
}

CKeyValues3Array::CKeyValues3Array( int cluster_elem ) : 
	m_nClusterElement( cluster_elem ) 
{
}

CKeyValues3ArrayCluster* CKeyValues3Array::GetCluster() const
{
	if ( m_nClusterElement == -1 )
		return NULL;

	return GET_OUTER( CKeyValues3ArrayCluster, m_Elements[ m_nClusterElement ] );
}

CKeyValues3Context* CKeyValues3Array::GetContext() const
{ 
	CKeyValues3ArrayCluster* cluster = GetCluster();

	if ( cluster )
		return cluster->GetContext();
	else
		return NULL;
}

void CKeyValues3Array::SetCount( int count, KV3TypeEx_t type, KV3SubType_t subtype )
{
	int nOldSize = m_Elements.Count();

	CKeyValues3Context* context = GetContext();

	for ( int i = count; i < nOldSize; ++i )
	{
		if ( context )
			context->FreeKV( m_Elements[ i ] );
		else
			delete m_Elements[ i ];
	}

	m_Elements.SetCount( count );

	for ( int i = nOldSize; i < count; ++i )
	{
		if ( context )
			m_Elements[ i ] = context->AllocKV( type, subtype );
		else
			m_Elements[ i ] = new KeyValues3( type, subtype );
	}
}

KeyValues3** CKeyValues3Array::InsertBeforeGetPtr( int elem, int num )
{
	KeyValues3** kv = m_Elements.InsertBeforeGetPtr( elem, num );

	CKeyValues3Context* context = GetContext();

	for ( int i = 0; i < num; ++i )
	{
		if ( context )
			m_Elements[ elem + i ] = context->AllocKV();
		else
			m_Elements[ elem + i ] = new KeyValues3;
	}

	return kv;
}

void CKeyValues3Array::CopyFrom( const CKeyValues3Array* pSrc )
{
	int nNewSize = pSrc->m_Elements.Count();

	SetCount( nNewSize );

	for ( int i = 0; i < nNewSize; ++i )
		*m_Elements[i] = *pSrc->m_Elements[i];
}

void CKeyValues3Array::RemoveMultiple( int elem, int num )
{
	CKeyValues3Context* context = GetContext();

	for ( int i = 0; i < num; ++i )
	{
		if ( context )
			context->FreeKV( m_Elements[ elem + i ] );
		else
			delete m_Elements[ elem + i ];
	}

	m_Elements.RemoveMultiple( elem, num );
}

void CKeyValues3Array::Purge( bool bClearingContext )
{ 
	CKeyValues3Context* context = GetContext();

	FOR_EACH_LEANVEC( m_Elements, iter )
	{
		if ( context )
		{
			if ( !bClearingContext )
				context->FreeKV( m_Elements[ iter ] );
		}
		else
		{
			delete m_Elements[ iter ];
		}
	}

	m_Elements.Purge();
}

CKeyValues3Table::CKeyValues3Table( int cluster_elem ) :
	m_nClusterElement( cluster_elem ),
	m_pFastSearch( NULL ),
	m_bHasBadNames( false ) 
{
}

CKeyValues3TableCluster* CKeyValues3Table::GetCluster() const
{
	if ( m_nClusterElement == -1 )
		return NULL;

	return GET_OUTER( CKeyValues3TableCluster, m_Elements[ m_nClusterElement ] );
}

CKeyValues3Context* CKeyValues3Table::GetContext() const
{ 
	CKeyValues3TableCluster* cluster = GetCluster();

	if ( cluster )
		return cluster->GetContext();
	else
		return NULL;
}

void CKeyValues3Table::EnableFastSearch()
{
	if ( m_pFastSearch )
		m_pFastSearch->m_member_ids.RemoveAll();
	else
		m_pFastSearch = new kv3tablefastsearch_t;

	for ( int i = 0; i < m_Hashes.Count(); ++i )
		m_pFastSearch->m_member_ids.Insert( m_Hashes[i], i );

	m_pFastSearch->m_ignore = false;
	m_pFastSearch->m_ignores_counter = 0;
}

KV3MemberId_t CKeyValues3Table::FindMember( const KeyValues3* kv ) const
{
	for ( int i = 0; i < m_Hashes.Count(); ++i )
	{
		if ( m_Members[i] == kv )
			return i;
	}

	return KV3_INVALID_MEMBER;
}

KV3MemberId_t CKeyValues3Table::FindMember( const CKV3MemberName &name )
{
	bool bFastSearch = false;

	if ( m_pFastSearch )
	{
		if ( m_pFastSearch->m_ignore )
		{
			if ( ++m_pFastSearch->m_ignores_counter > 4 )
			{
				EnableFastSearch();
				bFastSearch = true;
			}
		}
		else
		{
			bFastSearch = true;
		}
	}

	if ( bFastSearch )
	{
		UtlHashHandle_t h = m_pFastSearch->m_member_ids.Find( name.GetHashCode() );

		if ( h != m_pFastSearch->m_member_ids.InvalidHandle() )
			return m_pFastSearch->m_member_ids[ h ];
	}
	else
	{
		for ( int i = 0; i < m_Hashes.Count(); ++i )
		{
			if ( m_Hashes[i] == name.GetHashCode() )
				return i;
		}
	}

	return KV3_INVALID_MEMBER;
}

KV3MemberId_t CKeyValues3Table::CreateMember( const CKV3MemberName &name )
{
	if ( m_Hashes.Count() >= 128 && !m_pFastSearch )
		EnableFastSearch();

	*m_Hashes.AddToTailGetPtr() = name.GetHashCode();

	KV3MemberId_t memberId = m_Hashes.Count() - 1;

	CKeyValues3Context* context = GetContext();

	if ( context )
	{
		*m_Members.AddToTailGetPtr() = context->AllocKV();
		*m_Names.AddToTailGetPtr() = context->AllocString( name.GetString() );
	}
	else
	{
		*m_Members.AddToTailGetPtr() = new KeyValues3;
		*m_Names.AddToTailGetPtr() = strdup( name.GetString() );
	}

	m_IsExternalName.AddToTail( false );

	if ( m_pFastSearch && !m_pFastSearch->m_ignore )
		m_pFastSearch->m_member_ids.Insert( name.GetHashCode(), memberId );

	return memberId;
}

void CKeyValues3Table::CopyFrom( const CKeyValues3Table* pSrc )
{
	int nNewSize = pSrc->m_Hashes.Count();

	RemoveAll( nNewSize );

	m_Hashes.SetCount( nNewSize );
	m_Members.SetCount( nNewSize );
	m_Names.SetCount( nNewSize );
	m_IsExternalName.SetCount( nNewSize );

	CKeyValues3Context* context = GetContext();

	for ( int i = 0; i < nNewSize; ++i )
	{
		m_Hashes[i] = pSrc->m_Hashes[i];

		if ( context )
		{
			m_Members[i] = context->AllocKV();
			m_Names[i] = context->AllocString( pSrc->m_Names[i] );
		}
		else
		{
			m_Members[i] = new KeyValues3;
			m_Names[i] = strdup( pSrc->m_Names[i] );
		}

		m_IsExternalName[i] = false;

		*m_Members[i] = *pSrc->m_Members[i];
	}

	if ( nNewSize >= 128 )
		EnableFastSearch();
}

void CKeyValues3Table::RemoveMember( KV3MemberId_t id )
{
	CKeyValues3Context* context = GetContext();

	if ( context ) 
	{
		context->FreeKV( m_Members[ id ] );
	}
	else
	{
		delete m_Members[ id ];
		free( (void*)m_Names[ id ] );
	}

	m_Hashes.Remove( id );
	m_Members.Remove( id );
	m_Names.Remove( id );
	m_IsExternalName.Remove( id );

	if ( m_pFastSearch )
	{
		m_pFastSearch->m_ignore = true;
		m_pFastSearch->m_ignores_counter = 1;
	}
}

void CKeyValues3Table::RemoveAll( int nAllocSize )
{
	CKeyValues3Context* context = GetContext();

	for ( int i = 0; i < m_Hashes.Count(); ++i )
	{
		if ( context )
		{
			context->FreeKV( m_Members[i] );
		}
		else
		{
			delete m_Members[i]; 
			free( (void*)m_Names[i] );
		}
	}

	m_Hashes.RemoveAll();
	m_Members.RemoveAll();
	m_Names.RemoveAll();
	m_IsExternalName.RemoveAll();

	if ( nAllocSize > 0 )
	{
		m_Hashes.EnsureCapacity( nAllocSize );
		m_Members.EnsureCapacity( nAllocSize );
		m_Names.EnsureCapacity( nAllocSize );
		m_IsExternalName.EnsureCapacity( nAllocSize );
	}

	if ( m_pFastSearch )
	{
		if ( nAllocSize >= 128 )
		{
			m_pFastSearch->Clear();
		}
		else
		{
			delete m_pFastSearch;
			m_pFastSearch = NULL;
		}
	}
}

void CKeyValues3Table::Purge( bool bClearingContext )
{
	CKeyValues3Context* context = GetContext();

	for ( int i = 0; i < m_Hashes.Count(); ++i )
	{
		if ( context )
		{
			if ( !bClearingContext )
				context->FreeKV( m_Members[i] );
		}
		else
		{
			delete m_Members[i]; 
			free( (void*)m_Names[i] );
		}
	}

	if ( m_pFastSearch )
		delete m_pFastSearch;
	m_pFastSearch = NULL;

	m_Hashes.Purge();
	m_Members.Purge();
	m_Names.Purge();
	m_IsExternalName.Purge();
}

CKeyValues3Cluster::CKeyValues3Cluster( CKeyValues3Context* context ) : 
	m_pContext( context ), 
	m_nAllocatedElements( 0 ),
	m_pMetaData( NULL ), 
	m_pNextFree( NULL ) 
{
	memset( &m_KeyValues, 0, sizeof( m_KeyValues ) );
}

CKeyValues3Cluster::~CKeyValues3Cluster() 
{
	FreeMetaData();
}

#include "tier0/memdbgoff.h"

KeyValues3* CKeyValues3Cluster::Alloc( KV3TypeEx_t type, KV3SubType_t subtype )
{
	Assert( IsFree() );
	int element = KV3Helpers::BitScanFwd( ~m_nAllocatedElements );
	m_nAllocatedElements |= ( 1ull << element );
	KeyValues3* kv = &m_KeyValues[ element ];
	new( kv ) KeyValues3( element, type, subtype );
	return kv;
}

#include "tier0/memdbgon.h"

void CKeyValues3Cluster::Free( int element )
{
	Assert( element >= 0 && element < KV3_CLUSTER_MAX_ELEMENTS );
	KeyValues3* kv = &m_KeyValues[ element ];
	Destruct( kv );
	memset( (void *)kv, 0, sizeof( KeyValues3 ) );
	m_nAllocatedElements &= ~( 1ull << element );
}

void CKeyValues3Cluster::PurgeElements()
{
	uint64 mask = 1;
	for ( int i = 0; i < KV3_CLUSTER_MAX_ELEMENTS; ++i )
	{
		if ( ( m_nAllocatedElements & mask ) != 0 )
			m_KeyValues[ i ].OnClearContext();
		mask <<= 1;
	}

	m_nAllocatedElements = 0;
}

void CKeyValues3Cluster::Purge()
{
	PurgeElements();

	if ( m_pMetaData )
	{
		for ( int i = 0; i < KV3_CLUSTER_MAX_ELEMENTS; ++i )
			m_pMetaData->m_elements[ i ].Purge();
	}
}

void CKeyValues3Cluster::Clear()
{
	PurgeElements();

	if ( m_pMetaData )
	{
		for ( int i = 0; i < KV3_CLUSTER_MAX_ELEMENTS; ++i )
			m_pMetaData->m_elements[ i ].Clear();
	}
}

void CKeyValues3Cluster::EnableMetaData( bool bEnable )
{
	if ( bEnable ) 
	{
		if ( !m_pMetaData )
			m_pMetaData = new kv3metadata_t;
	}
	else
	{
		FreeMetaData();
	}
}

void CKeyValues3Cluster::FreeMetaData()
{
	if ( m_pMetaData )
		delete m_pMetaData;
	m_pMetaData = NULL;
}

KV3MetaData_t* CKeyValues3Cluster::GetMetaData( int element ) const
{
	Assert( element >= 0 && element < KV3_CLUSTER_MAX_ELEMENTS );

	if ( !m_pMetaData )
		return NULL;

	return &m_pMetaData->m_elements[ element ];
}

CKeyValues3ContextBase::CKeyValues3ContextBase( CKeyValues3Context* context ) : 	
	m_pContext( context ),
	m_KV3BaseCluster( context ),
	m_pKV3FreeCluster( &m_KV3BaseCluster ),
	m_pArrayFreeCluster( NULL ),
	m_pTableFreeCluster( NULL ),
	m_pParsingErrorListener( NULL )
{
}

CKeyValues3ContextBase::~CKeyValues3ContextBase()
{
	Purge();
}

void CKeyValues3ContextBase::Clear()
{
	m_BinaryData.Clear();

	m_KV3BaseCluster.Clear();
	m_KV3BaseCluster.SetNextFree( NULL );
	m_pKV3FreeCluster = &m_KV3BaseCluster;

	FOR_EACH_LEANVEC( m_KV3Clusters, iter )
	{
		m_KV3Clusters[ iter ]->Clear();
		m_KV3Clusters[ iter ]->SetNextFree( m_pKV3FreeCluster );
		m_pKV3FreeCluster = m_KV3Clusters[ iter ];
	}

	m_pArrayFreeCluster = NULL;

	FOR_EACH_LEANVEC( m_ArrayClusters, iter )
	{
		m_ArrayClusters[ iter ]->Clear();
		m_ArrayClusters[ iter ]->SetNextFree( m_pArrayFreeCluster );
		m_pArrayFreeCluster = m_ArrayClusters[ iter ];
	}

	m_pTableFreeCluster = NULL;

	FOR_EACH_LEANVEC( m_TableClusters, iter )
	{
		m_TableClusters[ iter ]->Clear();
		m_TableClusters[ iter ]->SetNextFree( m_pTableFreeCluster );
		m_pTableFreeCluster = m_TableClusters[ iter ];
	}

	m_Symbols.RemoveAll();

	m_bFormatConverted = false;
}

void CKeyValues3ContextBase::Purge()
{
	m_BinaryData.Purge();

	m_KV3BaseCluster.Purge();
	m_KV3BaseCluster.SetNextFree( NULL );
	m_pKV3FreeCluster = &m_KV3BaseCluster;

	FOR_EACH_LEANVEC( m_KV3Clusters, iter )
	{
		m_KV3Clusters[ iter ]->Purge();
		delete m_KV3Clusters[ iter ];
	}

	m_KV3Clusters.Purge();

	FOR_EACH_LEANVEC( m_ArrayClusters, iter )
	{
		m_ArrayClusters[ iter ]->Purge();
		delete m_ArrayClusters[ iter ];
	}

	m_pArrayFreeCluster = NULL;
	m_ArrayClusters.Purge();

	FOR_EACH_LEANVEC( m_TableClusters, iter )
	{
		m_TableClusters[ iter ]->Purge();
		delete m_TableClusters[ iter ];
	}

	m_pTableFreeCluster = NULL;
	m_TableClusters.Purge();

	m_Symbols.Purge();

	m_bFormatConverted = false;
}

CKeyValues3Context::CKeyValues3Context( bool bNoRoot ) : BaseClass( this )
{
	if ( bNoRoot )
	{
		m_bRootAvailabe =  false;
	}
	else
	{
		m_bRootAvailabe = true;
		m_KV3BaseCluster.Alloc();
	}

	m_bMetaDataEnabled = false;
	m_bFormatConverted = false;
}

CKeyValues3Context::~CKeyValues3Context() 
{
}

void CKeyValues3Context::Clear()
{
	BaseClass::Clear();

	if ( m_bRootAvailabe )
		m_KV3BaseCluster.Alloc();
}

void CKeyValues3Context::Purge()
{
	BaseClass::Purge();

	if ( m_bRootAvailabe )
		m_KV3BaseCluster.Alloc();
}

KeyValues3* CKeyValues3Context::Root()
{
	if ( !m_bRootAvailabe )
	{
		Plat_FatalErrorFunc( "FATAL: %s called on a pool context (no root available)\n", __FUNCTION__ );
		DebuggerBreak();
	}

	return m_KV3BaseCluster.Head();
}

const char* CKeyValues3Context::AllocString( const char* pString )
{
	return m_Symbols.AddString( pString ).String();
}

void CKeyValues3Context::EnableMetaData( bool bEnable )
{
	if ( bEnable != m_bMetaDataEnabled )
	{
		m_KV3BaseCluster.EnableMetaData( bEnable );

		for ( int i = 0; i < m_KV3Clusters.Count(); ++i )
			m_KV3Clusters[ i ]->EnableMetaData( bEnable );

		m_bMetaDataEnabled = bEnable;
	}
}

void CKeyValues3Context::CopyMetaData( KV3MetaData_t* pDest, const KV3MetaData_t* pSrc )
{
	pDest->m_nLine = pSrc->m_nLine;
	pDest->m_nColumn = pSrc->m_nColumn;
	pDest->m_nFlags = pSrc->m_nFlags;
	pDest->m_sName = m_Symbols.AddString( pSrc->m_sName.String() );

	pDest->m_Comments.Purge();
	pDest->m_Comments.EnsureCapacity( pSrc->m_Comments.Count() );

	FOR_EACH_MAP_FAST( pSrc->m_Comments, iter )
	{
		pDest->m_Comments.Insert( pSrc->m_Comments.Key( iter ), pSrc->m_Comments.Element( iter ) );
	}
}

KeyValues3* CKeyValues3Context::AllocKV( KV3TypeEx_t type, KV3SubType_t subtype )
{
	KeyValues3* kv;

	if ( m_pKV3FreeCluster )
	{
		kv = m_pKV3FreeCluster->Alloc( type, subtype );

		if ( !m_pKV3FreeCluster->IsFree() )
		{
			CKeyValues3Cluster* cluster = m_pKV3FreeCluster->GetNextFree();
			m_pKV3FreeCluster->SetNextFree( NULL );
			m_pKV3FreeCluster = cluster;
		}
	}
	else
	{
		CKeyValues3Cluster* cluster = new CKeyValues3Cluster( m_pContext );
		cluster->EnableMetaData( m_bMetaDataEnabled );
		*m_KV3Clusters.AddToTailGetPtr() = cluster;
		m_pKV3FreeCluster = cluster;
		kv = cluster->Alloc( type, subtype );
	}

	return kv;
}

void CKeyValues3Context::FreeKV( KeyValues3* kv )
{
	CKeyValues3Context* context;
	KV3MetaData_t* metadata = kv->GetMetaData( &context );

	Assert( context == m_pContext );

	if ( metadata )
		metadata->Clear();

	Free<KeyValues3, CKeyValues3Cluster, KV3ClustersVec_t>( kv, &m_KV3BaseCluster, m_pKV3FreeCluster, m_KV3Clusters );
}
