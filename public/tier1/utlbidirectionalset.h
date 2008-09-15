//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Bi-directional set. A Bucket knows about the elements that lie
// in it, and the elements know about the buckets they lie in.
//
// $Revision: $
// $NoKeywords: $
//=============================================================================//

#ifndef UTLBIDIRECTIONALSET_H
#define UTLBIDIRECTIONALSET_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/dbg.h"
#include "utllinkedlist.h"

//-----------------------------------------------------------------------------
// Templatized helper class to deal with the kinds of things that spatial
// partition code always seems to have; buckets with lists of lots of elements
// and elements that live in lots of buckets. This makes it really quick to
// add and remove elements, and to iterate over all elements in a bucket.
//
// For this to work, you must initialize the set with two functions one that
// maps from bucket to the index of the first element in that bucket, and one 
// that maps from element to the index of the first bucket that element lies in.
// The set will completely manage the index, it's just expected that those
// indices will be stored outside the set.
//-----------------------------------------------------------------------------
template< class CBucketHandle, class CElementHandle, class I >
class CBidirectionalSet
{
public:
	// Install methods to get at the first bucket given a element
	// and vice versa...
	typedef I& (*FirstElementFunc_t)(CBucketHandle);
	typedef I& (*FirstBucketFunc_t)(CElementHandle);

	// Constructor
	CBidirectionalSet();

	// Call this before using the set
	void Init( FirstElementFunc_t elemFunc, FirstBucketFunc_t bucketFunc );

	// Add an element to a particular bucket
	void AddElementToBucket( CBucketHandle bucket, CElementHandle element );

	// Test if an element is in a particular bucket.
	// NOTE: EXPENSIVE!!!
	bool IsElementInBucket( CBucketHandle bucket, CElementHandle element );
	
	// Remove an element from a particular bucket
	void RemoveElementFromBucket( CBucketHandle bucket, CElementHandle element );

	// Remove an element from all buckets
	void RemoveElement( CElementHandle element );
	void RemoveBucket( CBucketHandle element );

	// Used to iterate elements in a bucket; I is the iterator
	I FirstElement( CBucketHandle bucket ) const;
	I NextElement( I idx ) const;
	CElementHandle Element( I idx ) const;

	// Used to iterate buckets associated with an element; I is the iterator
	I FirstBucket( CElementHandle bucket ) const;
	I NextBucket( I idx ) const;
	CBucketHandle Bucket( I idx ) const;

	static I InvalidIndex();

	// Ensure capacity
	void	EnsureCapacity( int count );

	// Deallocate....
	void	Purge();

private:
	struct BucketListInfo_t
	{
		CElementHandle	m_Element;
		I				m_BucketListIndex;	// what's the m_BucketsUsedByElement index of the entry?
	};

	struct ElementListInfo_t
	{
		CBucketHandle	m_Bucket;
		I				m_ElementListIndex;	// what's the m_ElementsInBucket index of the entry?
	};

	// Maintains a list of all elements in a particular bucket 
	CUtlLinkedList< BucketListInfo_t, I >	m_ElementsInBucket;

	// Maintains a list of all buckets a particular element lives in
	CUtlLinkedList< ElementListInfo_t, I >	m_BucketsUsedByElement;

	FirstBucketFunc_t	m_FirstBucket;
	FirstElementFunc_t	m_FirstElement;
};


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
template< class CBucketHandle, class CElementHandle, class I >
CBidirectionalSet<CBucketHandle,CElementHandle,I>::CBidirectionalSet( )
{
	m_FirstBucket = NULL;
	m_FirstElement = NULL;
}


//-----------------------------------------------------------------------------
// Call this before using the set
//-----------------------------------------------------------------------------
template< class CBucketHandle, class CElementHandle, class I >
void CBidirectionalSet<CBucketHandle,CElementHandle,I>::Init( FirstElementFunc_t elemFunc, FirstBucketFunc_t bucketFunc )
{
	m_FirstBucket = bucketFunc;
	m_FirstElement = elemFunc;
}


//-----------------------------------------------------------------------------
// Adds an element to the bucket
//-----------------------------------------------------------------------------
template< class CBucketHandle, class CElementHandle, class I >
void CBidirectionalSet<CBucketHandle,CElementHandle,I>::AddElementToBucket( CBucketHandle bucket, CElementHandle element )
{
	Assert( m_FirstBucket && m_FirstElement );

#ifdef _DEBUG
	// Make sure that this element doesn't already exist in the list of elements in the bucket
	I elementInBucket = m_FirstElement( bucket );
	while( elementInBucket != m_ElementsInBucket.InvalidIndex() )
	{
		// If you hit an Assert here, fix the calling code.  It's too expensive to ensure
		// that each item only shows up once here.  Hopefully you can do something better
		// outside of here.
		Assert( m_ElementsInBucket[elementInBucket].m_Element != element );
		elementInBucket = m_ElementsInBucket.Next( elementInBucket );
	}
#endif

#ifdef _DEBUG
	// Make sure that this bucket doesn't already exist in the element's list of buckets.
	I bucketInElement = m_FirstBucket( element );
	while( bucketInElement != m_BucketsUsedByElement.InvalidIndex() )
	{
		// If you hit an Assert here, fix the calling code.  It's too expensive to ensure
		// that each item only shows up once here.  Hopefully you can do something better
		// outside of here.
		Assert( m_BucketsUsedByElement[bucketInElement].m_Bucket != bucket );
		bucketInElement = m_BucketsUsedByElement.Next( bucketInElement );
	}
#endif

	// Allocate new element + bucket entries
	I idx = m_ElementsInBucket.Alloc(true);
	I list = m_BucketsUsedByElement.Alloc( true );

	// Store off the element data
	m_ElementsInBucket[idx].m_Element = element;
	m_ElementsInBucket[idx].m_BucketListIndex = list;

	// Here's the bucket data
	m_BucketsUsedByElement[list].m_Bucket = bucket;
	m_BucketsUsedByElement[list].m_ElementListIndex = idx;

	// Insert the element into the list of elements in the bucket
	I& firstElementInBucket = m_FirstElement( bucket );
	if ( firstElementInBucket != m_ElementsInBucket.InvalidIndex() )
		m_ElementsInBucket.LinkBefore( firstElementInBucket, idx );
	firstElementInBucket = idx;

	// Insert the bucket into the element's list of buckets
	I& firstBucketInElement = m_FirstBucket( element );
	if ( firstBucketInElement != m_BucketsUsedByElement.InvalidIndex() )
		m_BucketsUsedByElement.LinkBefore( firstBucketInElement, list );
	firstBucketInElement = list;
}

//-----------------------------------------------------------------------------
// Test if an element is in a particular bucket.
// NOTE: EXPENSIVE!!!
//-----------------------------------------------------------------------------
template< class CBucketHandle, class CElementHandle, class I >
bool CBidirectionalSet<CBucketHandle,CElementHandle,I>::IsElementInBucket( CBucketHandle bucket, CElementHandle element )
{
	// Search through all elements in this bucket to see if element is in there.
	I elementInBucket = m_FirstElement( bucket );
	while( elementInBucket != m_ElementsInBucket.InvalidIndex() )
	{
		if( m_ElementsInBucket[elementInBucket].m_Element == element )
		{
			return true;
		}
		elementInBucket = m_ElementsInBucket.Next( elementInBucket );
	}
	return false;
}


//-----------------------------------------------------------------------------
// Remove an element from a particular bucket
//-----------------------------------------------------------------------------
template< class CBucketHandle, class CElementHandle, class I >
void CBidirectionalSet<CBucketHandle,CElementHandle,I>::RemoveElementFromBucket( CBucketHandle bucket, CElementHandle element )
{
	// FIXME: Implement me!
	Assert(0);
}


//-----------------------------------------------------------------------------
// Removes an element from all buckets
//-----------------------------------------------------------------------------
template< class CBucketHandle, class CElementHandle, class I >
void CBidirectionalSet<CBucketHandle,CElementHandle,I>::RemoveElement( CElementHandle element )
{
	Assert( m_FirstBucket && m_FirstElement );

	// Iterate over the list of all buckets the element is in
	I i = m_FirstBucket( element );
	while (i != m_BucketsUsedByElement.InvalidIndex())
	{
		CBucketHandle bucket = m_BucketsUsedByElement[i].m_Bucket;
		I elementListIndex = m_BucketsUsedByElement[i].m_ElementListIndex; 

		// Unhook the element from the bucket's list of elements
		if (elementListIndex == m_FirstElement(bucket))
			m_FirstElement(bucket) = m_ElementsInBucket.Next(elementListIndex);
		m_ElementsInBucket.Free(elementListIndex);

		unsigned short prevNode = i;
		i = m_BucketsUsedByElement.Next(i);
		m_BucketsUsedByElement.Free(prevNode);
	}

	// Mark the list as empty
	m_FirstBucket( element ) = m_BucketsUsedByElement.InvalidIndex();
}

//-----------------------------------------------------------------------------
// Removes a bucket from all elements
//-----------------------------------------------------------------------------
template< class CBucketHandle, class CElementHandle, class I >
void CBidirectionalSet<CBucketHandle,CElementHandle,I>::RemoveBucket( CBucketHandle bucket )
{
	// Iterate over the list of all elements in the bucket
	I i = m_FirstElement( bucket );
	while (i != m_ElementsInBucket.InvalidIndex())
	{
		CElementHandle element = m_ElementsInBucket[i].m_Element;
		I bucketListIndex = m_ElementsInBucket[i].m_BucketListIndex; 

		// Unhook the bucket from the element's list of buckets
		if (bucketListIndex == m_FirstBucket(element))
			m_FirstBucket(element) = m_BucketsUsedByElement.Next(bucketListIndex);
		m_BucketsUsedByElement.Free(bucketListIndex);

		// Remove the list element
		unsigned short prevNode = i;
		i = m_ElementsInBucket.Next(i);
		m_ElementsInBucket.Free(prevNode);
	}

	// Mark the bucket list as empty
	m_FirstElement( bucket ) = m_ElementsInBucket.InvalidIndex();
}


//-----------------------------------------------------------------------------
// Ensure capacity
//-----------------------------------------------------------------------------
template< class CBucketHandle, class CElementHandle, class I >
void CBidirectionalSet<CBucketHandle,CElementHandle,I>::EnsureCapacity( int count )
{
	m_ElementsInBucket.EnsureCapacity( count );
	m_BucketsUsedByElement.EnsureCapacity( count );
}


//-----------------------------------------------------------------------------
// Deallocate....
//-----------------------------------------------------------------------------
template< class CBucketHandle, class CElementHandle, class I >
void CBidirectionalSet<CBucketHandle,CElementHandle,I>::Purge()
{
	m_ElementsInBucket.Purge( );
	m_BucketsUsedByElement.Purge( );
}


//-----------------------------------------------------------------------------
// Invalid index for iteration..
//-----------------------------------------------------------------------------
template< class CBucketHandle, class CElementHandle, class I >
inline I CBidirectionalSet<CBucketHandle,CElementHandle,I>::InvalidIndex()
{
	return CUtlLinkedList< CElementHandle, I >::InvalidIndex();
}


//-----------------------------------------------------------------------------
// Used to iterate elements in a bucket; I is the iterator
//-----------------------------------------------------------------------------
template< class CBucketHandle, class CElementHandle, class I >
inline I CBidirectionalSet<CBucketHandle,CElementHandle,I>::FirstElement( CBucketHandle bucket ) const
{
	Assert( m_FirstElement );
	return m_FirstElement(bucket);
}

template< class CBucketHandle, class CElementHandle, class I >
inline I CBidirectionalSet<CBucketHandle,CElementHandle,I>::NextElement( I idx ) const
{
	return m_ElementsInBucket.Next(idx);
}

template< class CBucketHandle, class CElementHandle, class I >
inline CElementHandle CBidirectionalSet<CBucketHandle,CElementHandle,I>::Element( I idx ) const
{
	return m_ElementsInBucket[idx].m_Element;
}

//-----------------------------------------------------------------------------
// Used to iterate buckets an element lies in; I is the iterator
//-----------------------------------------------------------------------------
template< class CBucketHandle, class CElementHandle, class I >
inline I CBidirectionalSet<CBucketHandle,CElementHandle,I>::FirstBucket( CElementHandle element ) const
{
	Assert( m_FirstBucket );
	return m_FirstBucket(element);
}

template< class CBucketHandle, class CElementHandle, class I >
inline I CBidirectionalSet<CBucketHandle,CElementHandle,I>::NextBucket( I idx ) const
{
	return m_BucketsUsedByElement.Next(idx);
}

template< class CBucketHandle, class CElementHandle, class I >
inline CBucketHandle CBidirectionalSet<CBucketHandle,CElementHandle,I>::Bucket( I idx ) const
{
	return m_BucketsUsedByElement[idx].m_Bucket;
}
   
#endif // UTLBIDIRECTIONALSET_H
