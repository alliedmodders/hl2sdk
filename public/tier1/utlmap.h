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

//-----------------------------------------------------------------------------
//
// Purpose:	An associative container. Pretty much identical to std::map.
//			Note this class is not thread safe
//

template <typename K, typename T, typename I = unsigned short> 
class CUtlMap
{
public:
	typedef K KeyType_t;
	typedef T ElemType_t;
	typedef I IndexType_t;

	// Less func typedef
	// Returns true if the first parameter is "less" than the second
	typedef bool (*LessFunc_t)( const KeyType_t &, const KeyType_t & );
	
	// constructor, destructor
	// Left at growSize = 0, the memory will first allocate 1 element and double in size
	// at each increment.
	// LessFunc_t is required, but may be set after the constructor using SetLessFunc() below
	CUtlMap( int growSize = 0, int initSize = 0, LessFunc_t lessfunc = 0 )
	 : m_Tree( growSize, initSize, TreeLessFunc )
	{
		m_pfnLess = lessfunc;
	}
	
	CUtlMap( LessFunc_t lessfunc )
	 : m_Tree( TreeLessFunc )
	{
		m_pfnLess = lessfunc;
	}
	
	// gets particular elements
	ElemType_t &		Element( IndexType_t i )			{ return m_Tree.Element( i ).elem; }
	const ElemType_t &	Element( IndexType_t i ) const		{ return m_Tree.Element( i ).elem; }
	ElemType_t &		operator[]( IndexType_t i )			{ return m_Tree.Element( i ).elem; }
	const ElemType_t &	operator[]( IndexType_t i ) const	{ return m_Tree.Element( i ).elem; }
	KeyType_t &			Key( IndexType_t i )				{ return m_Tree.Element( i ).key; }
	const KeyType_t &	Key( IndexType_t i ) const			{ return m_Tree.Element( i ).key; }

	
	// Num elements
	unsigned int Count() const								{ return m_Tree.Count(); }
	
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
		m_pfnLess = func;
	}
	
	// Insert method (inserts in order)
	IndexType_t  Insert( const KeyType_t &key, const ElemType_t &insert )
	{
		Node_t node;
		node.key = key;
		node.elem = insert;
		Assert( m_pfnLess );
		KeyLessFunc( m_pfnLess );
		return m_Tree.Insert( node );
	}
	
	// Find method
	IndexType_t  Find( const KeyType_t &key ) const
	{
		Node_t dummyNode;
		dummyNode.key = key;
		Assert( m_pfnLess );
		KeyLessFunc( m_pfnLess );
		return m_Tree.Find( dummyNode );
	}
	
	// Remove methods
	void     RemoveAt( IndexType_t i )						{ m_Tree.RemoveAt( i ); }
	bool     Remove( const KeyType_t &key )
	{
		Node_t dummyNode;
		dummyNode.key = key;
		Assert( m_pfnLess );
		KeyLessFunc( m_pfnLess );
		return m_Tree.Remove( dummyNode );
	}
	
	void     RemoveAll( )									{ m_Tree.RemoveAll(); }
			
	// Iteration
	IndexType_t  FirstInorder() const						{ return m_Tree.FirstInorder(); }
	IndexType_t  NextInorder( IndexType_t i ) const			{ return m_Tree.NextInorder( i ); }
	IndexType_t  PrevInorder( IndexType_t i ) const			{ return m_Tree.PrevInorder( i ); }
	IndexType_t  LastInorder() const						{ return m_Tree.LastInorder(); }		
	
	// If you change the search key, this can be used to reinsert the 
	// element into the map.
	void	Reinsert( const KeyType_t &key, IndexType_t i )
	{
		m_Tree[i].key = key;
		Reinsert(i);
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


	struct Node_t
	{
		KeyType_t	key;
		ElemType_t	elem;
	};
	
	typedef CUtlRBTree<Node_t, I> CTree;

	CTree *AccessTree()	{ return &m_Tree; }

protected:
		
	static bool TreeLessFunc( const Node_t &left, const Node_t &right )
	{
		return (*KeyLessFunc())( left.key, right.key );
	}
	
	static LessFunc_t KeyLessFunc( LessFunc_t pfnNew = NULL )
	{
		// @Note (toml 12-10-02): This is why the class is not thread safe. The way RB
		//						  tree is implemented, I could see no other efficient
		//						  and portable way to do this. This function local
		//						  static approach is used to not require 
		//						  instantiation of a static data member or use
		//						  of Microsoft extensions (_declspec(selectany))
		static LessFunc_t pfnLess;
		
		if ( pfnNew != NULL )
			pfnLess = pfnNew;
		return pfnLess;
	}
	
	CTree 	   m_Tree;
	LessFunc_t m_pfnLess;
};

//-----------------------------------------------------------------------------

#endif // UTLMAP_H
