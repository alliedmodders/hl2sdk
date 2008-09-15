//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Linked list container class that doesn't move around
//
// $Revision: $
// $NoKeywords: $
//=============================================================================//

#ifndef UTLFIXEDLINKEDLIST_H
#define UTLFIXEDLINKEDLIST_H

#ifdef _WIN32
#pragma once
#endif

#include "basetypes.h"
#include "utlfixedmemory.h"
#include "tier0/dbg.h"


// This is a useful macro to iterate from head to tail in a linked list.
#undef FOR_EACH_LL
#define FOR_EACH_LL( listName, iteratorName ) \
	for( int iteratorName=listName.Head(); iteratorName != listName.InvalidIndex(); iteratorName = listName.Next( iteratorName ) )


//-----------------------------------------------------------------------------
// class CUtlFixedLinkedList:
//-----------------------------------------------------------------------------
template <class T> 
class CUtlFixedLinkedList
{
public:
	typedef T ElemType_t;
	typedef int I;
	typedef int IndexType_t;

	// constructor, destructor
	CUtlFixedLinkedList( int growSize = 0, int initSize = 0 );
	~CUtlFixedLinkedList( );

	// gets particular elements
	T&         Element( I i );
	T const&   Element( I i ) const;
	T&         operator[]( I i );
	T const&   operator[]( I i ) const;

	// Make sure we have a particular amount of memory
	void EnsureCapacity( int num );

	void SetGrowSize( int growSize );

	// Memory deallocation
	void Purge();

	// Delete all the elements then call Purge.
	void PurgeAndDeleteElements();
	
	// Insertion methods....
	I	InsertBefore( I before );
	I	InsertAfter( I after );
	I	AddToHead( ); 
	I	AddToTail( );

	I	InsertBefore( I before, T const& src );
	I	InsertAfter( I after, T const& src );
	I	AddToHead( T const& src ); 
	I	AddToTail( T const& src );

	// Find an element and return its index or InvalidIndex() if it couldn't be found.
	I		Find( const T &src ) const;
	
	// Look for the element. If it exists, remove it and return true. Otherwise, return false.
	bool	FindAndRemove( const T &src );

	// Removal methods
	void	Remove( I elem );
	void	RemoveAll();

	// Allocation/deallocation methods
	// If multilist == true, then list list may contain many
	// non-connected lists, and IsInList and Head + Tail are meaningless...
	I		Alloc( bool multilist = false );
	void	Free( I elem );

	// list modification
	void	LinkBefore( I before, I elem );
	void	LinkAfter( I after, I elem );
	void	Unlink( I elem );
	void	LinkToHead( I elem );
	void	LinkToTail( I elem );

	// invalid index
	inline static I  InvalidIndex()  { return 0; }
	inline static size_t ElementSize() { return sizeof(ListElem_t); }

	// list statistics
	int	Count() const;

	// Traversing the list
	I  Head() const;
	I  Tail() const;
	I  Previous( I i ) const;
	I  Next( I i ) const;

	// Are nodes in the list or valid?
	bool  IsValidIndex( I i ) const;
	bool  IsInList( I i ) const;
   
protected:
	// What the linked list element looks like
	struct ListElem_t
	{
		T  m_Element;
		I  m_Previous;
		I  m_Next;

	private:
		// No copy constructor for these...
		ListElem_t( const ListElem_t& );
	};
	
	// constructs the class
	I AllocInternal( bool multilist = false );
	void ConstructList();
	
	// Gets at the list element....
	ListElem_t& InternalElement( I i ) { return m_Memory[i]; }
	ListElem_t const& InternalElement( I i ) const { return m_Memory[i]; }

	// copy constructors not allowed
	CUtlFixedLinkedList( const CUtlFixedLinkedList<T> &list ) { Assert(0); }
	   
	CUtlFixedMemory<ListElem_t> m_Memory;
	I	m_Head;
	I	m_Tail;
	I	m_FirstFree;
	I	m_ElementCount;		// The number actually in the list
	I	m_TotalElements;	// The total number of elements	allocated
};
   
   
//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------
template <class T>
CUtlFixedLinkedList<T>::CUtlFixedLinkedList( int growSize, int initSize ) :
	m_Memory(growSize, initSize) 
{
	ConstructList();
}

template <class T>
CUtlFixedLinkedList<T>::~CUtlFixedLinkedList( ) 
{
	RemoveAll();
}

template <class T>
void CUtlFixedLinkedList<T>::ConstructList()
{
	m_Head = InvalidIndex(); 
	m_Tail = InvalidIndex();
	m_FirstFree = InvalidIndex();
	m_ElementCount = m_TotalElements = 0;
}


//-----------------------------------------------------------------------------
// gets particular elements
//-----------------------------------------------------------------------------
template <class T>
inline T& CUtlFixedLinkedList<T>::Element( I i )
{
	return m_Memory[i].m_Element; 
}

template <class T>
inline T const& CUtlFixedLinkedList<T>::Element( I i ) const
{
	return m_Memory[i].m_Element; 
}

template <class T>
inline T& CUtlFixedLinkedList<T>::operator[]( I i )        
{ 
	return m_Memory[i].m_Element; 
}

template <class T>
inline T const& CUtlFixedLinkedList<T>::operator[]( I i ) const 
{
	return m_Memory[i].m_Element; 
}


//-----------------------------------------------------------------------------
// list statistics
//-----------------------------------------------------------------------------
template <class T>
inline int CUtlFixedLinkedList<T>::Count() const      
{ 
	return m_ElementCount; 
}


//-----------------------------------------------------------------------------
// Traversing the list
//-----------------------------------------------------------------------------
template <class T>
inline typename CUtlFixedLinkedList<T>::I  CUtlFixedLinkedList<T>::Head() const  
{ 
	return m_Head; 
}

template <class T>
inline typename CUtlFixedLinkedList<T>::I  CUtlFixedLinkedList<T>::Tail() const  
{ 
	return m_Tail; 
}

template <class T>
inline typename CUtlFixedLinkedList<T>::I  CUtlFixedLinkedList<T>::Previous( I i ) const  
{ 
	Assert( IsValidIndex(i) ); 
	return InternalElement(i).m_Previous; 
}

template <class T>
inline typename CUtlFixedLinkedList<T>::I  CUtlFixedLinkedList<T>::Next( I i ) const  
{ 
	Assert( IsValidIndex(i) ); 
	return InternalElement(i).m_Next; 
}


//-----------------------------------------------------------------------------
// Are nodes in the list or valid?
//-----------------------------------------------------------------------------
template <class T>
inline bool CUtlFixedLinkedList<T>::IsValidIndex( I i ) const  
{ 
	return (i != 0) && ((m_Memory[i].m_Previous != i) || (m_Memory[i].m_Next == i));
}

template <class T>
inline bool CUtlFixedLinkedList<T>::IsInList( I i ) const
{
	return (i != 0) && (Previous(i) != i);
}


//-----------------------------------------------------------------------------
// Makes sure we have enough memory allocated to store a requested # of elements
//-----------------------------------------------------------------------------
template< class T >
void CUtlFixedLinkedList<T>::EnsureCapacity( int num )
{
	m_Memory.EnsureCapacity(num);
}

template<class T>
void CUtlFixedLinkedList<T>::SetGrowSize( int growSize )
{
	m_Memory.SetGrowSize( growSize );
}

//-----------------------------------------------------------------------------
// Deallocate memory
//-----------------------------------------------------------------------------
template <class T>
void  CUtlFixedLinkedList<T>::Purge()
{
	RemoveAll();
	m_Memory.Purge( );
	m_FirstFree = InvalidIndex();
	m_TotalElements = 0;
}


template<class T>
void CUtlFixedLinkedList<T>::PurgeAndDeleteElements()
{
	int iNext;
	for( int i=Head(); i != InvalidIndex(); i=iNext )
	{
		iNext = Next(i);
		delete Element(i);
	}

	Purge();
}


//-----------------------------------------------------------------------------
// Node allocation/deallocation
//-----------------------------------------------------------------------------
template <class T>
typename CUtlFixedLinkedList<T>::I CUtlFixedLinkedList<T>::AllocInternal( bool multilist )
{
	I elem;
	if (m_FirstFree == InvalidIndex())
	{
		// Nothing in the free list; add.
		// Since nothing is in the free list, m_TotalElements == total # of elements
		// the list knows about.
		elem = m_Memory.Alloc();
		++m_TotalElements;
		if ( elem == InvalidIndex() )
		{
			Error("CUtlFixedLinkedList overflow!\n");
		}
	} 
	else
	{
		elem = m_FirstFree;
		m_FirstFree = InternalElement(m_FirstFree).m_Next;
	}
	
	if (!multilist)
	{
		InternalElement(elem).m_Next = InternalElement(elem).m_Previous = elem;
	}
	else
	{
		InternalElement(elem).m_Next = InternalElement(elem).m_Previous = InvalidIndex();
	}

	return elem;
}

template <class T>
typename CUtlFixedLinkedList<T>::I CUtlFixedLinkedList<T>::Alloc( bool multilist )
{
	I elem = AllocInternal( multilist );
	Construct( &Element(elem) );

	return elem;
}

template <class T>
void  CUtlFixedLinkedList<T>::Free( I elem )
{
	Assert( IsValidIndex(elem) );
	Unlink(elem);

	ListElem_t &internalElem = InternalElement(elem);
	Destruct( &internalElem.m_Element );
	internalElem.m_Next = m_FirstFree;
	m_FirstFree = elem;
}


//-----------------------------------------------------------------------------
// Insertion methods; allocates and links (uses default constructor)
//-----------------------------------------------------------------------------
template <class T>
typename CUtlFixedLinkedList<T>::I CUtlFixedLinkedList<T>::InsertBefore( I before )
{
	// Make a new node
	I   newNode = AllocInternal();
	
	// Link it in
	LinkBefore( before, newNode );
	
	// Construct the data
	Construct( &Element(newNode) );
	
	return newNode;
}

template <class T>
typename CUtlFixedLinkedList<T>::I CUtlFixedLinkedList<T>::InsertAfter( I after )
{
	// Make a new node
	I   newNode = AllocInternal();
	
	// Link it in
	LinkAfter( after, newNode );
	
	// Construct the data
	Construct( &Element(newNode) );
	
	return newNode;
}

template <class T>
inline typename CUtlFixedLinkedList<T>::I CUtlFixedLinkedList<T>::AddToHead( ) 
{ 
	return InsertAfter( InvalidIndex() ); 
}

template <class T>
inline typename CUtlFixedLinkedList<T>::I CUtlFixedLinkedList<T>::AddToTail( ) 
{ 
	return InsertBefore( InvalidIndex() ); 
}


//-----------------------------------------------------------------------------
// Insertion methods; allocates and links (uses copy constructor)
//-----------------------------------------------------------------------------
template <class T>
typename CUtlFixedLinkedList<T>::I CUtlFixedLinkedList<T>::InsertBefore( I before, T const& src )
{
	// Make a new node
	I   newNode = AllocInternal();
	
	// Link it in
	LinkBefore( before, newNode );
	
	// Construct the data
	CopyConstruct( &Element(newNode), src );
	
	return newNode;
}

template <class T>
typename CUtlFixedLinkedList<T>::I CUtlFixedLinkedList<T>::InsertAfter( I after, T const& src )
{
	// Make a new node
	I   newNode = AllocInternal();
	
	// Link it in
	LinkAfter( after, newNode );
	
	// Construct the data
	CopyConstruct( &Element(newNode), src );
	
	return newNode;
}

template <class T>
inline typename CUtlFixedLinkedList<T>::I CUtlFixedLinkedList<T>::AddToHead( T const& src ) 
{ 
	return InsertAfter( InvalidIndex(), src ); 
}

template <class T>
inline typename CUtlFixedLinkedList<T>::I CUtlFixedLinkedList<T>::AddToTail( T const& src ) 
{ 
	return InsertBefore( InvalidIndex(), src ); 
}


//-----------------------------------------------------------------------------
// Removal methods
//-----------------------------------------------------------------------------

template<class T>
typename CUtlFixedLinkedList<T>::I CUtlFixedLinkedList<T>::Find( const T &src ) const
{
	for ( I i=Head(); i != InvalidIndex(); i = Next( i ) )
	{
		if ( Element( i ) == src )
			return i;
	}
	return InvalidIndex();
}


template<class T>
bool CUtlFixedLinkedList<T>::FindAndRemove( const T &src )
{
	I i = Find( src );
	if ( i == InvalidIndex() )
	{
		return false;
	}
	else
	{
		Remove( i );
		return true;
	}
}


template <class T>
void  CUtlFixedLinkedList<T>::Remove( I elem )
{
	Free( elem );
}

template <class T>
void  CUtlFixedLinkedList<T>::RemoveAll()
{
	if (m_TotalElements == 0)
		return;

	// Have to do some convoluted stuff to invoke the destructor on all
	// valid elements for the multilist case (since we don't have all elements
	// connected to each other in a list).

	// Put everything into the free list
	I elem = m_Memory.FirstElement();
	for (int i = 0; i < m_TotalElements; ++i )
	{
		// Invoke the destructor
		if (IsValidIndex((I)elem))
		{
			Destruct( &Element((I)elem) );
		}
		elem = m_Memory.NextElement( elem );
	}
	
	// This doesn't actually deallocate the memory... purge does that
	m_Memory.RemoveAll();
	m_TotalElements = 0;
	m_FirstFree = NULL;
	
	// Clear everything else out
	m_Head = InvalidIndex(); 
	m_Tail = InvalidIndex();
	m_ElementCount = 0;
}


//-----------------------------------------------------------------------------
// list modification
//-----------------------------------------------------------------------------
template <class T>
void  CUtlFixedLinkedList<T>::LinkBefore( I before, I elem )
{
	Assert( IsValidIndex(elem) );
	
	// Unlink it if it's in the list at the moment
	Unlink(elem);
	
	ListElem_t& newElem = InternalElement(elem);
	
	// The element *after* our newly linked one is the one we linked before.
	newElem.m_Next = before;
	
	if (before == InvalidIndex())
	{
		// In this case, we're linking to the end of the list, so reset the tail
		newElem.m_Previous = m_Tail;
		m_Tail = elem;
	}
	else
	{
		// Here, we're not linking to the end. Set the prev pointer to point to
		// the element we're linking.
		Assert( IsInList(before) );
		ListElem_t& beforeElem = InternalElement(before);
		newElem.m_Previous = beforeElem.m_Previous;
		beforeElem.m_Previous = elem;
	}
	
	// Reset the head if we linked to the head of the list
	if (newElem.m_Previous == InvalidIndex())
		m_Head = elem;
	else
		InternalElement(newElem.m_Previous).m_Next = elem;
	
	// one more element baby
	++m_ElementCount;
}

template <class T>
void  CUtlFixedLinkedList<T>::LinkAfter( I after, I elem )
{
	Assert( IsValidIndex(elem) );
	
	// Unlink it if it's in the list at the moment
	if ( IsInList(elem) )
		Unlink(elem);
	
	ListElem_t& newElem = InternalElement(elem);
	
	// The element *before* our newly linked one is the one we linked after
	newElem.m_Previous = after;
	if (after == InvalidIndex())
	{
		// In this case, we're linking to the head of the list, reset the head
		newElem.m_Next = m_Head;
		m_Head = elem;
	}
	else
	{
		// Here, we're not linking to the end. Set the next pointer to point to
		// the element we're linking.
		Assert( IsInList(after) );
		ListElem_t& afterElem = InternalElement(after);
		newElem.m_Next = afterElem.m_Next;
		afterElem.m_Next = elem;
	}
	
	// Reset the tail if we linked to the tail of the list
	if (newElem.m_Next == InvalidIndex())
		m_Tail = elem;
	else
		InternalElement(newElem.m_Next).m_Previous = elem;
	
	// one more element baby
	++m_ElementCount;
}

template <class T>
void  CUtlFixedLinkedList<T>::Unlink( I elem )
{
	Assert( IsValidIndex(elem) );
	if (IsInList(elem))
	{
		ListElem_t *pOldElem = &m_Memory[elem];
		
		// If we're the first guy, reset the head
		// otherwise, make our previous node's next pointer = our next
		if ( pOldElem->m_Previous != 0 )
		{
			m_Memory[pOldElem->m_Previous].m_Next = pOldElem->m_Next;
		}
		else
		{
			m_Head = pOldElem->m_Next;
		}
		
		// If we're the last guy, reset the tail
		// otherwise, make our next node's prev pointer = our prev
		if ( pOldElem->m_Next != 0 )
		{
			m_Memory[pOldElem->m_Next].m_Previous = pOldElem->m_Previous;
		}
		else
		{
			m_Tail = pOldElem->m_Previous;
		}
		
		// This marks this node as not in the list, 
		// but not in the free list either
		pOldElem->m_Previous = pOldElem->m_Next = elem;
		
		// One less puppy
		--m_ElementCount;
	}
}

template <class T>
inline void CUtlFixedLinkedList<T>::LinkToHead( I elem ) 
{
	LinkAfter( InvalidIndex(), elem ); 
}

template <class T>
inline void CUtlFixedLinkedList<T>::LinkToTail( I elem ) 
{
	LinkBefore( InvalidIndex(), elem ); 
}

   
#endif // UTLFIXEDLINKEDLIST_H
