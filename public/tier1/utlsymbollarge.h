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

typedef unsigned int UtlSymLargeId_t;

#define UTL_INVAL_SYMBOL_LARGE  ((UtlSymLargeId_t)~0)

class CUtlSymbolLarge
{
public:
	// constructor, destructor
	CUtlSymbolLarge() 
	{
		m_pString = NULL;
	}

	CUtlSymbolLarge( const char* pString )
	{
		m_pString = pString;
	}

	// operator==
	bool operator==( CUtlSymbolLarge const& src ) const 
	{ 
		return m_pString == src.m_pString; 
	}

	// operator!=
	bool operator!=( CUtlSymbolLarge const& src ) const 
	{ 
		return m_pString != src.m_pString; 
	}

	inline const char* String() const 
	{ 
		if ( !m_pString )
			return "";
		return m_pString; 
	}

	inline bool IsValid() const
	{
		return m_pString != NULL;
	}

private:
	// Disallowed
	bool operator==( const char* pString ) const; // disallow since we don't know if the table this is from was case sensitive or not... maybe we don't care

	const char* m_pString;
};

inline uint32 CUtlSymbolLarge_Hash( bool CASEINSENSITIVE, const char *pString, int len )
{
	return ( CASEINSENSITIVE ? MurmurHash2LowerCase( pString, len, 0x31415926 ) : MurmurHash2( pString, len, 0x31415926 ) ); 
}

typedef uint32 LargeSymbolTableHashDecoration_t; 

// The structure consists of the hash immediately followed by the string data
struct CUtlSymbolTableLargeBaseTreeEntry_t
{
	LargeSymbolTableHashDecoration_t	m_Hash;
	// Variable length string data
	char								m_String[1];

	bool IsEmpty() const
	{
		return ( ( m_Hash == 0 ) && ( 0 == m_String[0] ) );
	}

	char const *String() const
	{
		return (const char *)&m_String[ 0 ];
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
	CUtlSymbolLarge AddString( const char* pString );
	CUtlSymbolLarge AddString( const char* pString, int nLength );

	// Finds the symbol for pString
	CUtlSymbolLarge Find( const char* pString ) const;
	CUtlSymbolLarge Find( const char* pString, int nLength ) const;

	// Remove all symbols in the table.
	void RemoveAll();
	void Purge();
	
private:
	CUtlSymbolLarge AddString( unsigned int hash, const char* pString, int nLength );
	CUtlSymbolLarge Find( unsigned int hash, const char* pString, int nLength ) const;

	const char*		String( UtlSymLargeId_t id ) const;
	unsigned int	HashValue( UtlSymLargeId_t id ) const;

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

		unsigned int operator()( UtlSymTableLargeAltKey k ) const
		{
			return CUtlSymbolLarge_Hash( CASEINSENSITIVE, k.m_pString, k.m_nLength );
		}

		unsigned int operator()( UtlSymLargeId_t k ) const
		{
			const CUtlSymbolTableLargeBase* pTable = (const CUtlSymbolTableLargeBase*)((uintp)this + m_ownerOffset);

			return pTable->HashValue( k );
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
		
		bool operator()( UtlSymLargeId_t a, UtlSymLargeId_t b ) const 
		{ 
			const CUtlSymbolTableLargeBase* pTable = (const CUtlSymbolTableLargeBase*)((uintp)this + m_ownerOffset);

			if ( !CASEINSENSITIVE ) 
				return strcmp( pTable->String( a ), pTable->String( b ) ) == 0; 
			else
				return V_stricmp_fast( pTable->String( a ), pTable->String( b ) ) == 0; 
		}

		bool operator()( UtlSymTableLargeAltKey a, UtlSymLargeId_t b ) const 
		{ 
			const char* pString = a.m_pTable->String( b );
			int nLength = strlen( pString );

			if ( a.m_nLength != nLength )
				return false;

			if ( !CASEINSENSITIVE ) 
				return strncmp( a.m_pString, pString, a.m_nLength ) == 0; 
			else
				return _V_strnicmp_fast( a.m_pString, pString, a.m_nLength ) == 0; 
		}

		bool operator()( UtlSymLargeId_t a, UtlSymTableLargeAltKey b ) const 
		{ 
			return operator()( b, a );
		}
	};

	typedef CUtlHashtable<UtlSymLargeId_t, empty_t, UtlSymTableLargeHashFunctor, UtlSymTableLargeEqualFunctor, UtlSymTableLargeAltKey, CUtlMemory_RawAllocator<CUtlHashtableEntry<UtlSymLargeId_t, empty_t>>> Hashtable_t;
	typedef CUtlVector< MemBlockHandle_t, CUtlMemory_RawAllocator<MemBlockHandle_t> > MemBlocksVec_t;

	Hashtable_t					m_HashTable;
	MemBlocksVec_t				m_MemBlocks;
	MUTEX_TYPE					m_Mutex;
	CUtlMemoryBlockAllocator	m_MemBlockAllocator;
	int							m_nElementLimit;
	bool						m_bThrowError;
};

template < bool CASEINSENSITIVE, size_t PAGE_SIZE, class MUTEX_TYPE >
inline CUtlSymbolLarge CUtlSymbolTableLargeBase< CASEINSENSITIVE, PAGE_SIZE, MUTEX_TYPE >::Find( unsigned int hash, const char* pString, int nLength ) const
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
inline CUtlSymbolLarge CUtlSymbolTableLargeBase< CASEINSENSITIVE, PAGE_SIZE, MUTEX_TYPE >::AddString( unsigned int hash, const char* pString, int nLength )
{	
	if ( m_MemBlocks.Count() >= m_nElementLimit )
	{
		if ( m_bThrowError )
		{
			Plat_FatalErrorFunc( "FATAL ERROR: CUtlSymbolTableLarge element limit of %u exceeded\n", m_nElementLimit );
			DebuggerBreak();
		}

		Warning( "ERROR: CUtlSymbolTableLarge element limit of %u exceeded\n", m_nElementLimit );
	}

	MemBlockHandle_t block = m_MemBlockAllocator.Alloc( nLength + sizeof( LargeSymbolTableHashDecoration_t ) + 1 );

	CUtlSymbolTableLargeBaseTreeEntry_t *entry = (CUtlSymbolTableLargeBaseTreeEntry_t *)m_MemBlockAllocator.GetBlock( block );

	entry->m_Hash = hash;
	char *pText = (char *)&entry->m_String[ 0 ];
	memcpy( pText, pString, nLength );
	pText[ nLength ] = '\0';

	UtlSymLargeId_t id = m_MemBlocks.AddToTail( block + sizeof( LargeSymbolTableHashDecoration_t ) );

	empty_t empty;
	m_HashTable.Insert( id, empty, hash );

	return entry->ToSymbol();
}

template < bool CASEINSENSITIVE, size_t PAGE_SIZE, class MUTEX_TYPE >
inline const char* CUtlSymbolTableLargeBase< CASEINSENSITIVE, PAGE_SIZE, MUTEX_TYPE >::String( UtlSymLargeId_t id ) const
{
	return ( const char* )m_MemBlockAllocator.GetBlock( m_MemBlocks[ id ] );
}

template < bool CASEINSENSITIVE, size_t PAGE_SIZE, class MUTEX_TYPE >
inline unsigned int CUtlSymbolTableLargeBase< CASEINSENSITIVE, PAGE_SIZE, MUTEX_TYPE >::HashValue( UtlSymLargeId_t id ) const
{
	CUtlSymbolTableLargeBaseTreeEntry_t *entry = (CUtlSymbolTableLargeBaseTreeEntry_t *)m_MemBlockAllocator.GetBlock( m_MemBlocks[ id ] - sizeof( LargeSymbolTableHashDecoration_t ) );

	return entry->HashValue();
}

template < bool CASEINSENSITIVE, size_t PAGE_SIZE, class MUTEX_TYPE >
inline CUtlSymbolLarge CUtlSymbolTableLargeBase< CASEINSENSITIVE, PAGE_SIZE, MUTEX_TYPE >::Find( const char* pString, int nLength ) const
{	
	CUtlSymbolLarge sym;

	if ( pString && nLength > 0 && *pString )
	{
		unsigned int hash = CUtlSymbolLarge_Hash( CASEINSENSITIVE, pString, nLength );

		m_Mutex.Lock( __FILE__, __LINE__ );

		sym = Find( hash, pString, nLength );

		m_Mutex.Unlock( __FILE__, __LINE__ );
	}

	return sym;
}

template < bool CASEINSENSITIVE, size_t PAGE_SIZE, class MUTEX_TYPE >
inline CUtlSymbolLarge CUtlSymbolTableLargeBase< CASEINSENSITIVE, PAGE_SIZE, MUTEX_TYPE >::Find( const char* pString ) const
{	
	return Find( pString, pString ? strlen( pString ) : 0 );
}

template < bool CASEINSENSITIVE, size_t PAGE_SIZE, class MUTEX_TYPE >
inline CUtlSymbolLarge CUtlSymbolTableLargeBase< CASEINSENSITIVE, PAGE_SIZE, MUTEX_TYPE >::AddString( const char* pString, int nLength )
{	
	CUtlSymbolLarge sym;

	if ( pString && nLength > 0 && *pString )
	{
		unsigned int hash = CUtlSymbolLarge_Hash( CASEINSENSITIVE, pString, nLength );

		m_Mutex.Lock( __FILE__, __LINE__ );

		sym = Find( hash, pString, nLength );

		if ( !sym.IsValid() )
			sym = AddString( hash, pString, nLength );

		m_Mutex.Unlock( __FILE__, __LINE__ );
	}

	return sym;
}

template < bool CASEINSENSITIVE, size_t PAGE_SIZE, class MUTEX_TYPE >
inline CUtlSymbolLarge CUtlSymbolTableLargeBase< CASEINSENSITIVE, PAGE_SIZE, MUTEX_TYPE >::AddString( const char* pString )
{	
	return AddString( pString, pString ? strlen( pString ) : 0 );
}

template < bool CASEINSENSITIVE, size_t PAGE_SIZE, class MUTEX_TYPE >
inline void CUtlSymbolTableLargeBase< CASEINSENSITIVE, PAGE_SIZE, MUTEX_TYPE >::RemoveAll()
{	
	m_Mutex.Lock( __FILE__, __LINE__ );

	m_HashTable.RemoveAll();
	m_MemBlocks.RemoveAll();
	m_MemBlockAllocator.RemoveAll();

	m_Mutex.Unlock( __FILE__, __LINE__ );
}

template < bool CASEINSENSITIVE, size_t PAGE_SIZE, class MUTEX_TYPE >
inline void CUtlSymbolTableLargeBase< CASEINSENSITIVE, PAGE_SIZE, MUTEX_TYPE >::Purge()
{	
	m_Mutex.Lock( __FILE__, __LINE__ );

	m_HashTable.Purge();
	m_MemBlocks.Purge();
	m_MemBlockAllocator.Purge();

	m_Mutex.Unlock( __FILE__, __LINE__ );
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
