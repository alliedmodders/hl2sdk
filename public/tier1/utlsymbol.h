//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Defines a symbol table
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#ifndef UTLSYMBOL_H
#define UTLSYMBOL_H

#include "utlrbtree.h"
#include "utlvector.h"

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------

class CUtlSymbolTable;


//-----------------------------------------------------------------------------
// This is a symbol, which is a easier way of dealing with strings.
//-----------------------------------------------------------------------------

typedef unsigned short UtlSymId_t;

#define UTL_INVAL_SYMBOL  ((UtlSymId_t)~0)

class CUtlSymbol
{
public:
	// constructor, destructor
	CUtlSymbol() : m_Id(UTL_INVAL_SYMBOL) {}
	CUtlSymbol( UtlSymId_t id ) : m_Id(id) {}
	CUtlSymbol( char const* pStr );
	CUtlSymbol( CUtlSymbol const& sym ) : m_Id(sym.m_Id) {}
	
	// operator=
	CUtlSymbol& operator=( CUtlSymbol const& src ) { m_Id = src.m_Id; return *this; }
	
	// operator==
	bool operator==( CUtlSymbol const& src ) const { return m_Id == src.m_Id; }
	bool operator==( char const* pStr ) const;
	
	// Is valid?
	bool IsValid() const { return m_Id != UTL_INVAL_SYMBOL; }
	
	// Gets at the symbol
	operator UtlSymId_t const() const { return m_Id; }
	
	// Gets the string associated with the symbol
	char const* String( ) const;

	// Modules can choose to disable the static symbol table so to prevent accidental use of them.
	static void DisableStaticSymbolTable();
		
protected:
	UtlSymId_t   m_Id;
		
	// Initializes the symbol table
	static void Initialize();
	
	// returns the current symbol table
	static CUtlSymbolTable* CurrTable();
		
	// The standard global symbol table
	static CUtlSymbolTable* s_pSymbolTable; 

	static bool s_bAllowStaticSymbolTable;

	friend class CCleanupUtlSymbolTable;
};


//-----------------------------------------------------------------------------
// CUtlSymbolTable:
// description:
//    This class defines a symbol table, which allows us to perform mappings
//    of strings to symbols and back. The symbol class itself contains
//    a static version of this class for creating global strings, but this
//    class can also be instanced to create local symbol tables.
//-----------------------------------------------------------------------------

class CUtlSymbolTable
{
public:
	// constructor, destructor
	CUtlSymbolTable( int growSize = 0, int initSize = 32, bool caseInsensitive = false );
	~CUtlSymbolTable();
	
	// Finds and/or creates a symbol based on the string
	CUtlSymbol AddString( char const* pString );

	// Finds the symbol for pString
	CUtlSymbol Find( char const* pString );
	
	// Look up the string associated with a particular symbol
	char const* String( CUtlSymbol id ) const;
	
	// Remove all symbols in the table.
	void  RemoveAll();

	int GetNumStrings( void ) const
	{
		return m_Lookup.Count();
	}

protected:
	class CStringPoolIndex
	{
	public:
		inline CStringPoolIndex()
		{
		}

		inline CStringPoolIndex( unsigned short iPool, unsigned short iOffset )
		{
			m_iPool = iPool;
			m_iOffset = iOffset;
		}

		inline bool operator==( const CStringPoolIndex &other )	const
		{
			return m_iPool == other.m_iPool && m_iOffset == other.m_iOffset;
		}

		unsigned short m_iPool;		// Index into m_StringPools.
		unsigned short m_iOffset;	// Index into the string pool.
	};

	// Stores the symbol lookup
	CUtlRBTree< CStringPoolIndex, unsigned short, CLessFuncCallerUserData<CStringPoolIndex> >	m_Lookup;

	typedef struct
	{	
		int m_TotalLen;		// How large is 
		int m_SpaceUsed;
		char m_Data[1];
	} StringPool_t;

	// stores the string data
	CUtlVector<StringPool_t*> m_StringPools;
	
	// Stored before using the RBTree comparisons.
	const char *m_pLessCtxUserString;


private:

	int FindPoolWithSpace( int len ) const;

	const char* StringFromIndex( const CStringPoolIndex &index ) const;
		
	// Less function, for sorting strings
	static bool SymLess( CStringPoolIndex const& i1, CStringPoolIndex const& i2, void *pUserData );
	// case insensitive less function
	static bool SymLessi( CStringPoolIndex const& i1, CStringPoolIndex const& i2, void *pUserData );
};


#endif // UTLSYMBOL_H
