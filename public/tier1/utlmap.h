//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#ifndef UTLMAP_H
#define UTLMAP_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/dbg.h"
#include "utlrbtree.h"
#include "utlleanvector.h"

//-----------------------------------------------------------------------------
//
// Purpose:	An associative container. Pretty much identical to std::map.
//
//-----------------------------------------------------------------------------

// This is a useful macro to iterate from start to end in order in a map
#define FOR_EACH_MAP( mapName, iteratorName ) \
	for ( int iteratorName = (mapName).FirstInorder(); (mapName).IsUtlMap && iteratorName != (mapName).InvalidIndex(); iteratorName = (mapName).NextInorder( iteratorName ) )

// faster iteration, but in an unspecified order
#define FOR_EACH_MAP_FAST( mapName, iteratorName ) \
	for ( int iteratorName = 0; (mapName).IsUtlMap && iteratorName < (mapName).MaxElement(); ++iteratorName ) if ( !(mapName).IsValidIndex( iteratorName ) ) continue; else

struct base_utlmap_t
{
public:
	// This enum exists so that FOR_EACH_MAP and FOR_EACH_MAP_FAST cannot accidentally
	// be used on a type that is not a CUtlOrderedMapBase. If the code compiles then all is well.
	// The check for IsUtlMap being true should be free.
	// Using an enum rather than a static const bool ensures that this trick works even
	// with optimizations disabled on gcc.
	enum CompileTimeCheck
	{
		IsUtlMap = 1
	};
};

template <typename K, typename T, typename LF, typename I = int>
class CUtlOrderedMapBase : public base_utlmap_t
{
public:
	typedef K KeyType_t;
	typedef T ElemType_t;
	typedef I IndexType_t;

	// Less func typedef
	// Returns true if the first parameter is "less" than the second
	typedef LF LessFunc_t;
	
	// constructor, destructor
	// Left at growSize = 0, the memory will first allocate 1 element and double in size
	// at each increment.
	// LessFunc_t is required, but may be set after the constructor using SetLessFunc() below
	CUtlOrderedMapBase( int growSize, int initSize, const LessFunc_t &lessfunc )
	 : m_Tree( growSize, initSize, CKeyLess( lessfunc ) )
	{
	}
	
	void EnsureCapacity( int num )							{ m_Tree.EnsureCapacity( num ); }

	// gets particular elements
	ElemType_t &		Element( IndexType_t i )			{ return m_Tree.Element( i ).elem; }
	const ElemType_t &	Element( IndexType_t i ) const		{ return m_Tree.Element( i ).elem; }
	ElemType_t &		operator[]( IndexType_t i )			{ return m_Tree.Element( i ).elem; }
	const ElemType_t &	operator[]( IndexType_t i ) const	{ return m_Tree.Element( i ).elem; }
	KeyType_t &			Key( IndexType_t i )				{ return m_Tree.Element( i ).key; }
	const KeyType_t &	Key( IndexType_t i ) const			{ return m_Tree.Element( i ).key; }

	
	// Num elements
	IndexType_t  Count() const								{ return m_Tree.Count(); }

	bool IsEmpty() const									{ return Count() == 0; }
	
	// Max "size" of the vector
	IndexType_t  MaxElement() const							{ return m_Tree.MaxElement(); }
	
	// Checks if a node is valid and in the map
	bool  IsValidIndex( IndexType_t i ) const				{ return m_Tree.IsValidIndex( i ); }
	
	// Checks if the map as a whole is valid
	bool  IsValid() const									{ return m_Tree.IsValid(); }
	
	// Invalid index
	static IndexType_t InvalidIndex()						{ return CTree::InvalidIndex(); }
	
	// Sets the less func
	void SetLessFunc( LessFunc_t func )
	{
		m_Tree.SetLessFunc( CKeyLess( func ) );
	}
	
	// Insert method (inserts in order)
	IndexType_t  Insert( const KeyType_t &key, const ElemType_t &insert )
	{
		Node_t node;
		node.key = key;
		node.elem = insert;
		return m_Tree.Insert( node );
	}
	
	IndexType_t  Insert( const KeyType_t &key )
	{
		Node_t node;
		node.key = key;
		return m_Tree.Insert( node );
	}

	// Returns the existing index if the key is already present
	IndexType_t InsertIfNotFound( const KeyType_t &key, const ElemType_t &insert )
	{
		Node_t node;
		node.key = key;
		node.elem = insert;
		return m_Tree.InsertIfNotFound( node );
	}

	// pInserted reports whether an insert happened
	IndexType_t FindOrInsert( const KeyType_t &key, const ElemType_t &insert, bool *pInserted = NULL )
	{
		IndexType_t i = Find( key );
		if ( i != InvalidIndex() )
		{
			if ( pInserted )
				*pInserted = false;
			return i;
		}

		if ( pInserted )
			*pInserted = true;
		return Insert( key, insert );
	}

	IndexType_t FindOrInsertDefault( const KeyType_t &key, bool *pInserted = NULL )
	{
		return FindOrInsert( key, ElemType_t(), pInserted );
	}

	ElemType_t *FindOrInsertGetPtr( const KeyType_t &key, const ElemType_t &insert, bool *pInserted = NULL )
	{
		return &Element( FindOrInsert( key, insert, pInserted ) );
	}

	ElemType_t *FindOrInsertDefaultGetPtr( const KeyType_t &key, bool *pInserted = NULL )
	{
		return &Element( FindOrInsertDefault( key, pInserted ) );
	}

	// Find method
	IndexType_t  Find( const KeyType_t &key ) const
	{
		Node_t dummyNode;
		dummyNode.key = key;
		return m_Tree.Find( dummyNode );
	}

	const ElemType_t &FindElement( const KeyType_t &key ) const
	{
		return Element( Find( key ) );
	}

	// By value, so a temporary defaultValue is safe
	ElemType_t FindElement( const KeyType_t &key, const ElemType_t &defaultValue ) const
	{
		IndexType_t i = Find( key );
		if ( i == InvalidIndex() )
			return defaultValue;
		return Element( i );
	}

	// NULL when the key isn't present
	ElemType_t *FindGetPtr( const KeyType_t &key )
	{
		IndexType_t i = Find( key );
		return i == InvalidIndex() ? NULL : &Element( i );
	}

	const ElemType_t *FindGetPtr( const KeyType_t &key ) const
	{
		IndexType_t i = Find( key );
		return i == InvalidIndex() ? NULL : &Element( i );
	}

	bool HasElement( const KeyType_t &key ) const			{ return Find( key ) != InvalidIndex(); }
	bool HasKey( const KeyType_t &key ) const				{ return Find( key ) != InvalidIndex(); }
	
	// Remove methods
	void     RemoveAt( IndexType_t i )						{ m_Tree.RemoveAt( i ); }
	bool     Remove( const KeyType_t &key )
	{
		Node_t dummyNode;
		dummyNode.key = key;
		return m_Tree.Remove( dummyNode );
	}
	
	void     RemoveAll( )									{ m_Tree.RemoveAll(); }
	void     Purge( )										{ m_Tree.Purge(); }

	// Only valid when ElemType_t is a pointer
	void PurgeAndDeleteElements()
	{
		for ( IndexType_t i = 0; i < MaxElement(); ++i )
		{
			if ( IsValidIndex( i ) )
				delete Element( i );
		}
		Purge();
	}

	void RemoveAllAndDeleteElements()
	{
		for ( IndexType_t i = 0; i < MaxElement(); ++i )
		{
			if ( IsValidIndex( i ) )
				delete Element( i );
		}
		RemoveAll();
	}
			
	// Iteration
	IndexType_t  FirstInorder() const						{ return m_Tree.FirstInorder(); }
	IndexType_t  NextInorder( IndexType_t i ) const			{ return m_Tree.NextInorder( i ); }
	IndexType_t  PrevInorder( IndexType_t i ) const			{ return m_Tree.PrevInorder( i ); }
	IndexType_t  LastInorder() const						{ return m_Tree.LastInorder(); }		

	// InvalidIndex once the neighbouring element has a different key
	IndexType_t NextInorderSameKey( IndexType_t i ) const
	{
		IndexType_t iNext = NextInorder( i );
		if ( !IsValidIndex( iNext ) || Key( iNext ) != Key( i ) )
			return InvalidIndex();
		return iNext;
	}

	IndexType_t PrevInorderSameKey( IndexType_t i ) const
	{
		IndexType_t iPrev = PrevInorder( i );
		if ( !IsValidIndex( iPrev ) || Key( iPrev ) != Key( i ) )
			return InvalidIndex();
		return iPrev;
	}
	
	// If you change the search key, this can be used to reinsert the 
	// element into the map.
	void	Reinsert( const KeyType_t &key, IndexType_t i )
	{
		m_Tree[i].key = key;
		m_Tree.Reinsert(i);
	}

	IndexType_t InsertOrReplace( const KeyType_t &key, const ElemType_t &insert )
	{
		IndexType_t i = Find( key );
		if ( i != InvalidIndex() )
		{
			Element( i ) = insert;
			return i;
		}
		
		return Insert( key, insert );
	}

	void Swap( CUtlOrderedMapBase< K, T, LF, I > &that )
	{
		m_Tree.Swap( that.m_Tree );
	}

	void CopyFrom( const CUtlOrderedMapBase< K, T, LF, I > &other )
	{
		m_Tree.CopyFrom( other.m_Tree );
	}

	CUtlOrderedMapBase< K, T, LF, I > &operator=( const CUtlOrderedMapBase< K, T, LF, I > &other )
	{
		CopyFrom( other );
		return *this;
	}


	struct Node_t
	{
		Node_t()
		{
		}

		Node_t( const Node_t &from )
		  : key( from.key ),
			elem( from.elem )
		{
		}

		KeyType_t	key;
		ElemType_t	elem;
	};
	
	class CKeyLess
	{
	public:
		CKeyLess( LessFunc_t lessFunc ) : m_LessFunc(lessFunc) {}

		bool operator!() const
		{
			return !m_LessFunc;
		}

		bool operator()( const Node_t &left, const Node_t &right ) const
		{
			return m_LessFunc( left.key, right.key );
		}

		LessFunc_t m_LessFunc;
	};

	typedef CUtlRBTree<Node_t, I, CKeyLess> CTree;

	CTree *AccessTree()	{ return &m_Tree; }

protected:
	CTree 	   m_Tree;
};

// Adds the default LessFunc and index type. Storage and logic live in the base.
template <typename K, typename T, typename LF = CDefLess<K>, typename I = int>
class CUtlOrderedMap : public CUtlOrderedMapBase< K, T, LF, I >
{
public:
	typedef CUtlOrderedMapBase< K, T, LF, I > BaseClass;

	CUtlOrderedMap( int growSize, int initSize, const LF &lessfunc = LF() )
	 : BaseClass( growSize, initSize, lessfunc )
	{
	}

	CUtlOrderedMap( const LF &lessfunc = LF() )
	 : BaseClass( 0, 0, lessfunc )
	{
	}
};

#endif // UTLMAP_H
