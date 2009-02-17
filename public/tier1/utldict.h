//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: A dictionary mapping from symbol to structure 
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#ifndef UTLDICT_H
#define UTLDICT_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/dbg.h"
#include "utlrbtree.h"
#include "utlsymbol.h"


//-----------------------------------------------------------------------------
// A dictionary mapping from symbol to structure
//-----------------------------------------------------------------------------
template <class T, class I> 
class CUtlDict
{
public:
	// constructor, destructor
	// Left at growSize = 0, the memory will first allocate 1 element and double in size
	// at each increment.
	CUtlDict( bool caseInsensitive = true, int growSize = 0, int initSize = 0 );
	~CUtlDict( );
	
	// gets particular elements
	T&         Element( I i );
	const T&   Element( I i ) const;
	T&         operator[]( I i );
	const T&   operator[]( I i ) const;

	// gets element names
	char	   *GetElementName( I i );
	char const *GetElementName( I i ) const;

	void		SetElementName( I i, char const *pName );

	// Number of elements
	unsigned int Count() const;
	
	// Checks if a node is valid and in the tree
	bool  IsValidIndex( I i ) const;
	
	// Invalid index
	static I InvalidIndex();
	
	// Insert method (inserts in order)
	I  Insert( const char *pName, const T &element );
	I  Insert( const char *pName );
	
	// Find method
	I  Find( const char *pName ) const;
	
	// Remove methods
	void	RemoveAt( I i );
	void	Remove( const char *pName );
	void	RemoveAll( );
	
	// Purge memory
	void	Purge();
	void	PurgeAndDeleteElements();	// Call delete on each element.

	// Iteration methods
	I		First() const;
	I		Next( I i ) const;

protected:
	struct DictElement_t
	{
		CUtlSymbol	m_Name;
		T			m_Data;
	};

	static bool DictLessFunc( const DictElement_t &src1, const DictElement_t &src2 );
	typedef CUtlRBTree< DictElement_t, I > DictElementMap_t;

	DictElementMap_t		m_Elements;
	mutable CUtlSymbolTable	m_SymbolTable;
};


//-----------------------------------------------------------------------------
// less function
//-----------------------------------------------------------------------------
template <class T, class I>
bool CUtlDict<T, I>::DictLessFunc( const DictElement_t &src1, const DictElement_t &src2 )
{
	return src1.m_Name < src2.m_Name;
}


//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------
template <class T, class I>
CUtlDict<T, I>::CUtlDict( bool caseInsensitive, int growSize, int initSize ) : m_Elements( growSize, initSize, DictLessFunc ), 
	m_SymbolTable( growSize, initSize, caseInsensitive )
{
}

template <class T, class I> 
CUtlDict<T, I>::~CUtlDict()
{
}


//-----------------------------------------------------------------------------
// gets particular elements
//-----------------------------------------------------------------------------
template <class T, class I>
inline T& CUtlDict<T, I>::Element( I i )        
{ 
	return m_Elements[i].m_Data; 
}

template <class T, class I>
inline const T& CUtlDict<T, I>::Element( I i ) const  
{ 
	return m_Elements[i].m_Data; 
}

//-----------------------------------------------------------------------------
// gets element names
//-----------------------------------------------------------------------------
template <class T, class I>
inline char *CUtlDict<T, I>::GetElementName( I i )
{
	return (char *)( m_SymbolTable.String( m_Elements[ i ].m_Name ) );
}

template <class T, class I>
inline char const *CUtlDict<T, I>::GetElementName( I i ) const
{
	return m_SymbolTable.String( m_Elements[ i ].m_Name );
}

template <class T, class I>
inline T& CUtlDict<T, I>::operator[]( I i )        
{ 
	return Element(i); 
}

template <class T, class I>
inline const T & CUtlDict<T, I>::operator[]( I i ) const  
{ 
	return Element(i); 
}

template <class T, class I>
inline void CUtlDict<T, I>::SetElementName( I i, char const *pName )
{
	// TODO:  This makes a copy of the old element
	// TODO:  This relies on the rb tree putting the most recently
	//  removed element at the head of the insert list
	m_Elements[ i ].m_Name = m_SymbolTable.AddString( pName );
	m_Elements.Reinsert( i );
}

//-----------------------------------------------------------------------------
// Num elements
//-----------------------------------------------------------------------------
template <class T, class I>
inline	unsigned int CUtlDict<T, I>::Count() const          
{ 
	return m_Elements.Count(); 
}

	
//-----------------------------------------------------------------------------
// Checks if a node is valid and in the tree
//-----------------------------------------------------------------------------
template <class T, class I>
inline	bool CUtlDict<T, I>::IsValidIndex( I i ) const 
{
	return m_Elements.IsValidIndex(i);
}
	
	
//-----------------------------------------------------------------------------
// Invalid index
//-----------------------------------------------------------------------------
template <class T, class I>
inline I CUtlDict<T, I>::InvalidIndex()         
{ 
	return DictElementMap_t::InvalidIndex(); 
}

	
//-----------------------------------------------------------------------------
// Delete a node from the tree
//-----------------------------------------------------------------------------
template <class T, class I>
void CUtlDict<T, I>::RemoveAt(I elem) 
{
	m_Elements.RemoveAt(elem);
}


//-----------------------------------------------------------------------------
// remove a node in the tree
//-----------------------------------------------------------------------------
template <class T, class I> void CUtlDict<T, I>::Remove( const char *search )
{
	I node = Find( search );
	if (node != InvalidIndex())
		RemoveAt(node);
}


//-----------------------------------------------------------------------------
// Removes all nodes from the tree
//-----------------------------------------------------------------------------
template <class T, class I>
void CUtlDict<T, I>::RemoveAll()
{
	m_Elements.RemoveAll();
}

template <class T, class I>
void CUtlDict<T, I>::Purge()
{
	m_Elements.RemoveAll();
}


template <class T, class I>
void CUtlDict<T, I>::PurgeAndDeleteElements()
{
	// Delete all the elements.
	I index = m_Elements.FirstInorder();
	while ( index != m_Elements.InvalidIndex() )
	{
		delete m_Elements[index].m_Data;
		index = m_Elements.NextInorder( index );
	}

	m_Elements.RemoveAll();
}


//-----------------------------------------------------------------------------
// inserts a node into the tree
//-----------------------------------------------------------------------------
template <class T, class I> 
I CUtlDict<T, I>::Insert( const char *pName, const T &element )
{
	DictElement_t elem;
	elem.m_Name = m_SymbolTable.AddString( pName );
	I idx = m_Elements.Insert( elem );
	m_Elements[idx].m_Data = element;
	return idx;
}

template <class T, class I> 
I CUtlDict<T, I>::Insert( const char *pName )
{
	DictElement_t elem;
	elem.m_Name = m_SymbolTable.AddString( pName );
	I idx = m_Elements.Insert( elem );
	return idx;
}


//-----------------------------------------------------------------------------
// finds a node in the tree
//-----------------------------------------------------------------------------
template <class T, class I> 
I CUtlDict<T, I>::Find( const char *pName ) const
{
	DictElement_t elem;
	elem.m_Name = m_SymbolTable.AddString( pName );

	return m_Elements.Find( elem );
}


//-----------------------------------------------------------------------------
// Iteration methods
//-----------------------------------------------------------------------------
template <class T, class I> 
I CUtlDict<T, I>::First() const
{
	return m_Elements.FirstInorder();
}

template <class T, class I> 
I CUtlDict<T, I>::Next( I i ) const
{
	return m_Elements.NextInorder(i);
}

#endif // UTLDICT_H
