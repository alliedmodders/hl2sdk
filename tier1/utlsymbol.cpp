//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Defines a symbol table
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#pragma warning (disable:4514)

#include "utlsymbol.h"
#include "tier0/memdbgon.h"

#define INVALID_STRING_INDEX CStringPoolIndex( 0xFFFF, 0xFFFF )

#define MIN_STRING_POOL_SIZE	2048

//-----------------------------------------------------------------------------
// globals
//-----------------------------------------------------------------------------

CUtlSymbolTable* CUtlSymbol::s_pSymbolTable = 0; 
bool CUtlSymbol::s_bAllowStaticSymbolTable = true;


//-----------------------------------------------------------------------------
// symbol methods
//-----------------------------------------------------------------------------

void CUtlSymbol::Initialize()
{
	// If this assert fails, then the module that this call is in has chosen to disallow
	// use of the static symbol table. Usually, it's to prevent confusion because it's easy
	// to accidentally use the global symbol table when you really want to use a specific one.
	Assert( s_bAllowStaticSymbolTable );

	// necessary to allow us to create global symbols
	static bool symbolsInitialized = false;
	if (!symbolsInitialized)
	{
		s_pSymbolTable = new CUtlSymbolTable;
		symbolsInitialized = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Singleton to delete table on exit from module
//-----------------------------------------------------------------------------
class CCleanupUtlSymbolTable
{
public:
	~CCleanupUtlSymbolTable()
	{
		delete CUtlSymbol::s_pSymbolTable;
		CUtlSymbol::s_pSymbolTable = NULL;
	}
};

static CCleanupUtlSymbolTable g_CleanupSymbolTable;

CUtlSymbolTable* CUtlSymbol::CurrTable()
{
	Initialize();
	return s_pSymbolTable; 
}


//-----------------------------------------------------------------------------
// string->symbol->string
//-----------------------------------------------------------------------------

CUtlSymbol::CUtlSymbol( char const* pStr )
{
	m_Id = CurrTable()->AddString( pStr );
}

char const* CUtlSymbol::String( ) const
{
	return CurrTable()->String(m_Id);
}

void CUtlSymbol::DisableStaticSymbolTable()
{
	s_bAllowStaticSymbolTable = false;
}

//-----------------------------------------------------------------------------
// checks if the symbol matches a string
//-----------------------------------------------------------------------------

bool CUtlSymbol::operator==( char const* pStr ) const
{
	if (m_Id == UTL_INVAL_SYMBOL) 
		return false;
	return strcmp( String(), pStr ) == 0;
}



//-----------------------------------------------------------------------------
// symbol table stuff
//-----------------------------------------------------------------------------

inline const char* CUtlSymbolTable::StringFromIndex( const CStringPoolIndex &index ) const
{
	Assert( index.m_iPool < m_StringPools.Count() );
	Assert( index.m_iOffset < m_StringPools[index.m_iPool]->m_TotalLen );

	return &m_StringPools[index.m_iPool]->m_Data[index.m_iOffset];
}


bool CUtlSymbolTable::SymLess( CStringPoolIndex const& i1, CStringPoolIndex const& i2, void *pUserData )
{
	CUtlSymbolTable *pTable = (CUtlSymbolTable*)pUserData;

	char const* str1 = (i1 == INVALID_STRING_INDEX) ? pTable->m_pLessCtxUserString :
											pTable->StringFromIndex( i1 );
	char const* str2 = (i2 == INVALID_STRING_INDEX) ? pTable->m_pLessCtxUserString :
											pTable->StringFromIndex( i2 );
	
	return strcmp( str1, str2 ) < 0;
}


bool CUtlSymbolTable::SymLessi( CStringPoolIndex const& i1, CStringPoolIndex const& i2, void *pUserData )
{
	CUtlSymbolTable *pTable = (CUtlSymbolTable*)pUserData;

	char const* str1 = (i1 == INVALID_STRING_INDEX) ? pTable->m_pLessCtxUserString :
											pTable->StringFromIndex( i1 );
	char const* str2 = (i2 == INVALID_STRING_INDEX) ? pTable->m_pLessCtxUserString :
											pTable->StringFromIndex( i2 );
	
	return strcmpi( str1, str2 ) < 0;
}

//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------

CUtlSymbolTable::CUtlSymbolTable( int growSize, int initSize, bool caseInsensitive ) : 
	m_Lookup( growSize, initSize ), m_StringPools( 8 )
{
	if ( caseInsensitive )
		m_Lookup.SetLessFunc( SymLessi );
	else
		m_Lookup.SetLessFunc( SymLess );
}

CUtlSymbolTable::~CUtlSymbolTable()
{
	// Release the stringpool string data
	RemoveAll();
}


CUtlSymbol CUtlSymbolTable::Find( char const* pString )
{	
	if (!pString)
		return CUtlSymbol();
	
	// Store a special context used to help with insertion
	m_pLessCtxUserString = pString;
	
	// Passing this special invalid symbol makes the comparison function
	// use the string passed in the context
	UtlSymId_t idx = m_Lookup.Find( INVALID_STRING_INDEX, this );
	return CUtlSymbol( idx );
}


int CUtlSymbolTable::FindPoolWithSpace( int len )	const
{
	for ( int i=0; i < m_StringPools.Count(); i++ )
	{
		StringPool_t *pPool = m_StringPools[i];

		if ( (pPool->m_TotalLen - pPool->m_SpaceUsed) >= len )
		{
			return i;
		}
	}

	return -1;
}


//-----------------------------------------------------------------------------
// Finds and/or creates a symbol based on the string
//-----------------------------------------------------------------------------

CUtlSymbol CUtlSymbolTable::AddString( char const* pString )
{
	if (!pString) 
		return CUtlSymbol( UTL_INVAL_SYMBOL );

	CUtlSymbol id = Find( pString );
	
	if (id.IsValid())
		return id;

	int len = strlen(pString) + 1;

	// Find a pool with space for this string, or allocate a new one.
	int iPool = FindPoolWithSpace( len );
	if ( iPool == -1 )
	{
		// Add a new pool.
		int newPoolSize = max( len, MIN_STRING_POOL_SIZE );
		StringPool_t *pPool = (StringPool_t*)malloc( sizeof( StringPool_t ) + newPoolSize - 1 );
		pPool->m_TotalLen = newPoolSize;
		pPool->m_SpaceUsed = 0;
		iPool = m_StringPools.AddToTail( pPool );
	}

	// Copy the string in.
	StringPool_t *pPool = m_StringPools[iPool];
	Assert( pPool->m_SpaceUsed < 0xFFFF );	// This should never happen, because if we had a string > 64k, it
											// would have been given its entire own pool.
	
	unsigned short iStringOffset = pPool->m_SpaceUsed;

	memcpy( &pPool->m_Data[pPool->m_SpaceUsed], pString, len );
	pPool->m_SpaceUsed += len;

	// didn't find, insert the string into the vector.
	CStringPoolIndex index;
	index.m_iPool = iPool;
	index.m_iOffset = iStringOffset;

	UtlSymId_t idx = m_Lookup.Insert( index, this );
	return CUtlSymbol( idx );
}


//-----------------------------------------------------------------------------
// Look up the string associated with a particular symbol
//-----------------------------------------------------------------------------

char const* CUtlSymbolTable::String( CUtlSymbol id ) const
{
	if (!id.IsValid()) 
		return "";
	
	Assert( m_Lookup.IsValidIndex((UtlSymId_t)id) );
	return StringFromIndex( m_Lookup[id] );
}


//-----------------------------------------------------------------------------
// Remove all symbols in the table.
//-----------------------------------------------------------------------------

void CUtlSymbolTable::RemoveAll()
{
	m_Lookup.RemoveAll();
	
	for ( int i=0; i < m_StringPools.Count(); i++ )
		free( m_StringPools[i] );

	m_StringPools.RemoveAll();
}

