//===== Copyright ? 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Defines a large symbol table (intp sized handles, can store more than 64k strings)
//
// $Header: $
// $NoKeywords: $
//===========================================================================//

#ifndef UTLSYMBOLLARGE_H
#define UTLSYMBOLLARGE_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h"
#include "tier0/threadtools.h"
#include "tier1/generichash.h"
#include "tier1/utlvector.h"
#include "tier1/utlhashtable.h"
#include "tier1/memblockallocator.h"

//-----------------------------------------------------------------------------
// CUtlSymbolTableLarge:
// description:
//    This class defines a symbol table, which allows us to perform mappings
//    of strings to symbols and back. 
// 
//    This class stores the strings in a series of string pools. The returned CUtlSymbolLarge is just a pointer
//     to the string data, the hash precedes it in memory and is used to speed up searching, etc.
//-----------------------------------------------------------------------------

typedef intp UtlSymLargeId_t;
typedef uint UtlSymLargeElm_t;

#define UTL_INVAL_SYMBOL_LARGE ((UtlSymLargeId_t)0)

#define FOR_EACH_SYMBOL_LARGE( table, iter ) \
	for ( UtlSymLargeElm_t iter = 0; iter < (table).GetNumStrings(); iter++ )
#define FOR_EACH_SYMBOL_LARGE_BACK( table, iter ) \
	for ( UtlSymLargeElm_t iter = (table).GetNumStrings()-1; iter >= 0; iter-- )

class CUtlSymbolLarge
{
public:
	// constructor, destructor
	CUtlSymbolLarge( UtlSymLargeId_t id = UTL_INVAL_SYMBOL_LARGE ) : u( id ) {}
	CUtlSymbolLarge( const char* pString ) : u( pString ) {};

	// operator==
	bool operator==( CUtlSymbolLarge const& src ) const { return u.m_Id == src.u.m_Id; }
	bool operator==( const char* pString ) const = delete; // disallow since we don't know if the table this is from was case sensitive or not... maybe we don't care

	// operator!=
	bool operator!=( CUtlSymbolLarge const& src ) const { return u.m_Id != src.u.m_Id; }
	
	// operator<
	bool operator<( CUtlSymbolLarge const& src ) const { return u.m_Id < src.u.m_Id; }

	template< bool CASEINSENSITIVE = true >
	static uint32 Hash( const char *pString, int nLength = -1 ) { return MakeStringToken2< CASEINSENSITIVE >( pString, nLength ); }

	bool IsValid() const { return u.m_Id != UTL_INVAL_SYMBOL_LARGE; }
	UtlSymLargeId_t GetId() { return u.m_Id; };
	const char* String() const { return IsValid() ? u.m_pAsString : ""; }

private:
	union Data_t
	{
		Data_t( UtlSymLargeId_t id ) : m_Id( id ) {}
		Data_t( const char *pString ) : m_pAsString( pString ) {}

		UtlSymLargeId_t m_Id;
		const char *m_pAsString;
	} u;
};

typedef uint32 LargeSymbolTableHashDecoration_t;

// The structure consists of the hash immediately followed by the string data
struct ALIGN8_POST CUtlSymbolTableLargeBaseTreeEntry_t
{
	LargeSymbolTableHashDecoration_t	m_Hash;
	// Variable length string data
	char m_szString[1];

	bool IsEmpty() const
	{
		return ( !m_Hash && !m_szString[0] );
	}

	const char *String() const
	{
		return (const char *)m_szString;
	}

	void Replace( LargeSymbolTableHashDecoration_t nNewHash, const char *pNewString, int nLength )
	{
		m_Hash = nNewHash;
		Q_memcpy( (char *)m_szString, pNewString, nLength );
		m_szString[nLength] = '\0';
	}

	CUtlSymbolLarge ToSymbol() const
	{
		return CUtlSymbolLarge( String() );
	}

	LargeSymbolTableHashDecoration_t HashValue() const
	{
		return m_Hash;
	}
};

// Base Class for threaded and non-threaded types
template < bool CASEINSENSITIVE, size_t PAGE_SIZE, class MUTEX_TYPE >
class CUtlSymbolTableLargeBase
{
public:
	// constructor, destructor
	CUtlSymbolTableLargeBase( int nGrowSize = 0, int nInitSize = 16, RawAllocatorType_t eAllocatorType = RawAllocator_Standard )
		:	m_HashTable( 0, eAllocatorType ), 
			m_MemBlocks( nGrowSize, nInitSize, eAllocatorType ), 
			m_Mutex( "CUtlSymbolTableLargeBase" ), 
			m_MemBlockAllocator( ( nInitSize > 0 ) ? 8 : 0, PAGE_SIZE, eAllocatorType ), 
			m_nElementLimit( INT_MAX - 1 ), 
			m_bThrowError( true )  { }

	~CUtlSymbolTableLargeBase() { }

	// Finds and/or creates a symbol based on the string
	CUtlSymbolLarge AddString( const char* pString, bool* created = NULL );
	CUtlSymbolLarge AddString( const char* pString, int nLength, bool* created = NULL );

	// Finds the symbol for pString
	CUtlSymbolLarge Find( const char* pString ) const;
	CUtlSymbolLarge Find( const char* pString, int nLength ) const;

	const char*		String( UtlSymLargeElm_t id ) const;
	uint32			Hash( UtlSymLargeElm_t id ) const;

	int				GetNumStrings() const { return m_MemBlocks.Count(); };

	// Remove all symbols in the table.
	void RemoveAll();
	void Purge();
	
private:
	CUtlSymbolLarge AddString( uint32 hash, const char* pString, int nLength, bool* created );
	CUtlSymbolLarge Find( uint32 hash, const char* pString, int nLength ) const;

	struct UtlSymTableLargeAltKey
	{ 
		const CUtlSymbolTableLargeBase*	m_pTable;
		const char*						m_pString;
		int								m_nLength;
	};

	struct UtlSymTableLargeHashFunctor
	{
		ptrdiff_t m_ownerOffset;

		UtlSymTableLargeHashFunctor()
		{
			const ptrdiff_t tableoffset = (uintp)(&((Hashtable_t*)1024)->GetHashRef()) - 1024;
			const ptrdiff_t owneroffset = offsetof(CUtlSymbolTableLargeBase, m_HashTable) + tableoffset;
			m_ownerOffset = -owneroffset;
		}

		uint32 operator()( UtlSymTableLargeAltKey k ) const
		{
			return CUtlSymbolLarge::Hash< CASEINSENSITIVE >( k.m_pString, k.m_nLength );
		}

		uint32 operator()( UtlSymLargeElm_t k ) const
		{
			const CUtlSymbolTableLargeBase* pTable = (const CUtlSymbolTableLargeBase*)((uintp)this + m_ownerOffset);

			return pTable->Hash( k );
		}
	};

	struct UtlSymTableLargeEqualFunctor
	{
		ptrdiff_t m_ownerOffset;

		UtlSymTableLargeEqualFunctor()
		{
			const ptrdiff_t tableoffset = (uintp)(&((Hashtable_t*)1024)->GetEqualRef()) - 1024;
			const ptrdiff_t owneroffset = offsetof(CUtlSymbolTableLargeBase, m_HashTable) + tableoffset;
			m_ownerOffset = -owneroffset;
		}
		
		bool operator()( UtlSymLargeElm_t a, UtlSymLargeElm_t b ) const 
		{ 
			const CUtlSymbolTableLargeBase* pTable = (const CUtlSymbolTableLargeBase*)((uintp)this + m_ownerOffset);

			if ( CASEINSENSITIVE ) 
				return V_stricmp( pTable->String( a ), pTable->String( b ) ) == 0; 
			else
				return V_strcmp( pTable->String( a ), pTable->String( b ) ) == 0; 
		}

		bool operator()( UtlSymTableLargeAltKey a, UtlSymLargeElm_t b ) const 
		{ 
			const char* pString = a.m_pTable->String( b );
			int nLength = strlen( pString );

			if ( a.m_nLength != nLength )
				return false;

			if ( CASEINSENSITIVE ) 
				return V_strnicmp( a.m_pString, pString, a.m_nLength ) == 0; 
			else
				return V_strncmp( a.m_pString, pString, a.m_nLength ) == 0; 
		}

		bool operator()( UtlSymLargeElm_t a, UtlSymTableLargeAltKey b ) const 
		{ 
			return operator()( b, a );
		}
	};

	typedef CUtlHashtable< UtlSymLargeElm_t, empty_t, UtlSymTableLargeHashFunctor, UtlSymTableLargeEqualFunctor, UtlSymTableLargeAltKey, CUtlMemory_RawAllocator< CUtlHashtableEntry< UtlSymLargeElm_t, empty_t > > > Hashtable_t;
	typedef CUtlVector< MemBlockHandle_t, CUtlMemory_RawAllocator< MemBlockHandle_t > > MemBlocksVec_t;

	Hashtable_t					m_HashTable;
	MemBlocksVec_t				m_MemBlocks;
	MUTEX_TYPE					m_Mutex;
	CUtlMemoryBlockAllocator	m_MemBlockAllocator;
	int							m_nElementLimit;
	bool						m_bThrowError;
};

template < bool CASEINSENSITIVE, size_t PAGE_SIZE, class MUTEX_TYPE >
inline CUtlSymbolLarge CUtlSymbolTableLargeBase< CASEINSENSITIVE, PAGE_SIZE, MUTEX_TYPE >::Find( uint32 hash, const char* pString, int nLength ) const
{	
	UtlSymTableLargeAltKey key;
	
	key.m_pTable = this;
	key.m_pString = pString;
	key.m_nLength = nLength;

	UtlHashHandle_t h = m_HashTable.Find( key, hash );

	if ( h == m_HashTable.InvalidHandle() )
		return CUtlSymbolLarge();

	return CUtlSymbolLarge( String( m_HashTable[ h ] ) );
}

template < bool CASEINSENSITIVE, size_t PAGE_SIZE, class MUTEX_TYPE >
inline CUtlSymbolLarge CUtlSymbolTableLargeBase< CASEINSENSITIVE, PAGE_SIZE, MUTEX_TYPE >::AddString( uint32 hash, const char* pString, int nLength, bool* created )
{	
	if ( m_MemBlocks.Count() >= m_nElementLimit )
	{
		if ( m_bThrowError )
		{
			Plat_FatalErrorFunc( "FATAL ERROR: CUtlSymbolTableLarge element limit of %u exceeded\n", m_nElementLimit );
			DebuggerBreak();
		}

		Warning( "ERROR: CUtlSymbolTableLarge element limit of %u exceeded\n", m_nElementLimit );

		return CUtlSymbolLarge();
	}

	if ( created )
		*created = true;

	MemBlockHandle_t block = m_MemBlockAllocator.Alloc( nLength + sizeof( LargeSymbolTableHashDecoration_t ) + 1 );

	CUtlSymbolTableLargeBaseTreeEntry_t *entry = (CUtlSymbolTableLargeBaseTreeEntry_t *)m_MemBlockAllocator.GetBlock( block );

	entry->Replace( hash, pString, nLength );

	UtlSymLargeElm_t elem = m_MemBlocks.AddToTail( block + sizeof( LargeSymbolTableHashDecoration_t ) );

	m_HashTable.Insert( elem, empty_t(), hash );

	return entry->ToSymbol();
}

template < bool CASEINSENSITIVE, size_t PAGE_SIZE, class MUTEX_TYPE >
inline const char* CUtlSymbolTableLargeBase< CASEINSENSITIVE, PAGE_SIZE, MUTEX_TYPE >::String( UtlSymLargeElm_t elem ) const
{
	return ( const char* )m_MemBlockAllocator.GetBlock( m_MemBlocks[ elem ] );
}

template < bool CASEINSENSITIVE, size_t PAGE_SIZE, class MUTEX_TYPE >
inline uint32 CUtlSymbolTableLargeBase< CASEINSENSITIVE, PAGE_SIZE, MUTEX_TYPE >::Hash( UtlSymLargeElm_t elem ) const
{
	CUtlSymbolTableLargeBaseTreeEntry_t *entry = (CUtlSymbolTableLargeBaseTreeEntry_t *)m_MemBlockAllocator.GetBlock( m_MemBlocks[ elem ] - sizeof( LargeSymbolTableHashDecoration_t ) );

	return entry->HashValue();
}

template < bool CASEINSENSITIVE, size_t PAGE_SIZE, class MUTEX_TYPE >
inline CUtlSymbolLarge CUtlSymbolTableLargeBase< CASEINSENSITIVE, PAGE_SIZE, MUTEX_TYPE >::Find( const char* pString, int nLength ) const
{	
	CUtlSymbolLarge sym;

	if ( pString && nLength > 0 && *pString )
	{
		uint32 hash = CUtlSymbolLarge::Hash< CASEINSENSITIVE >( pString, nLength );

		AUTO_LOCK( m_Mutex );

		sym = Find( hash, pString, nLength );
	}

	return sym;
}

template < bool CASEINSENSITIVE, size_t PAGE_SIZE, class MUTEX_TYPE >
inline CUtlSymbolLarge CUtlSymbolTableLargeBase< CASEINSENSITIVE, PAGE_SIZE, MUTEX_TYPE >::Find( const char* pString ) const
{	
	return Find( pString, pString ? strlen( pString ) : 0 );
}

template < bool CASEINSENSITIVE, size_t PAGE_SIZE, class MUTEX_TYPE >
inline CUtlSymbolLarge CUtlSymbolTableLargeBase< CASEINSENSITIVE, PAGE_SIZE, MUTEX_TYPE >::AddString( const char* pString, int nLength, bool* created )
{	
	if ( created )
		*created = false;

	CUtlSymbolLarge sym;

	if ( pString && nLength > 0 && *pString )
	{
		uint32 hash = CUtlSymbolLarge::Hash< CASEINSENSITIVE >( pString, nLength );

		AUTO_LOCK( m_Mutex );

		sym = Find( hash, pString, nLength );

		if ( !sym.IsValid() )
			sym = AddString( hash, pString, nLength, created );
	}

	return sym;
}

template < bool CASEINSENSITIVE, size_t PAGE_SIZE, class MUTEX_TYPE >
inline CUtlSymbolLarge CUtlSymbolTableLargeBase< CASEINSENSITIVE, PAGE_SIZE, MUTEX_TYPE >::AddString( const char* pString, bool* created )
{	
	return AddString( pString, pString ? strlen( pString ) : 0, created );
}

template < bool CASEINSENSITIVE, size_t PAGE_SIZE, class MUTEX_TYPE >
inline void CUtlSymbolTableLargeBase< CASEINSENSITIVE, PAGE_SIZE, MUTEX_TYPE >::RemoveAll()
{	
	AUTO_LOCK( m_Mutex );

	m_HashTable.RemoveAll();
	m_MemBlocks.RemoveAll();
	m_MemBlockAllocator.RemoveAll();
}

template < bool CASEINSENSITIVE, size_t PAGE_SIZE, class MUTEX_TYPE >
inline void CUtlSymbolTableLargeBase< CASEINSENSITIVE, PAGE_SIZE, MUTEX_TYPE >::Purge()
{	
	AUTO_LOCK( m_Mutex );

	m_HashTable.Purge();
	m_MemBlocks.Purge();
	m_MemBlockAllocator.Purge();
}

// Case-sensitive
typedef CUtlSymbolTableLargeBase< false, 2048, CThreadNullMutex > CUtlSymbolTableLarge;
// Case-insensitive
typedef CUtlSymbolTableLargeBase< true, 2048, CThreadNullMutex > CUtlSymbolTableLarge_CI;
// Multi-threaded case-sensitive
typedef CUtlSymbolTableLargeBase< false, 2048, CThreadMutex > CUtlSymbolTableLargeMT;
// Multi-threaded case-insensitive
typedef CUtlSymbolTableLargeBase< true, 2048, CThreadMutex > CUtlSymbolTableLargeMT_CI;

#endif // UTLSYMBOLLARGE_H
