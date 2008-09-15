//========== Copyright © 2005, Valve Corporation, All rights reserved. ========
//
// Purpose:
//
// LIFO from disassembly of Windows API and http://perso.wanadoo.fr/gmem/evenements/jim2002/articles/L17_Fober.pdf
// FIFO from http://perso.wanadoo.fr/gmem/evenements/jim2002/articles/L17_Fober.pdf
//
//=============================================================================

#ifndef TSLIST_H
#define TSLIST_H

#if defined( _WIN32 )
#pragma once
#endif

#if ( defined(_WIN64) || defined(_X360) )
#define USE_NATIVE_SLIST
#endif

#if defined( USE_NATIVE_SLIST )
#ifdef _XBOX
#include "xbox/xbox_platform.h"
#include "xbox/xbox_win32stubs.h"
#include "xbox/xbox_core.h"
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#endif // _WIN32

#include "tier0/threadtools.h"

//-----------------------------------------------------------------------------

#if defined(_WIN64)
#define TSLIST_HEAD_ALIGNMENT MEMORY_ALLOCATION_ALIGNMENT
#define TSLIST_NODE_ALIGNMENT MEMORY_ALLOCATION_ALIGNMENT
#else
#define TSLIST_HEAD_ALIGNMENT 8
#define TSLIST_NODE_ALIGNMENT 4
#endif

#define TSLIST_HEAD_ALIGN __declspec(align(TSLIST_HEAD_ALIGNMENT))
#define TSLIST_NODE_ALIGN __declspec(align(TSLIST_NODE_ALIGNMENT))

//-----------------------------------------------------------------------------
// Lock free list.
//-----------------------------------------------------------------------------

#ifdef USE_NATIVE_SLIST
typedef SLIST_ENTRY TSLNodeBase_t;
typedef SLIST_HEADER TSLHead_t;
#else
struct TSLIST_NODE_ALIGN TSLNodeBase_t
{
	TSLNodeBase_t *Next; // name to match Windows
};

union TSLHead_t
{
	struct Value_t
	{
		TSLNodeBase_t *Next;
		int16   Depth;
		int16	Sequence;
	} value;

	int64 value64;
};
#endif

//-------------------------------------

template <typename T>
class TSLIST_HEAD_ALIGN CTSList
{
public:
	struct TSLIST_NODE_ALIGN Node_t : public TSLNodeBase_t
	{
		Node_t() {}
		Node_t( const T &init ) : elem( init ) {}

		T elem;

		void *operator new( size_t size )													{ return ( TSLIST_NODE_ALIGNMENT > 4 ) ? _aligned_malloc( size, TSLIST_NODE_ALIGNMENT ) : malloc( size ); }
		void *operator new( size_t size, int nBlockUse, const char *pFileName, int nLine )	{ return operator new( size ); }
		void operator delete( void *p )														{ if ( TSLIST_NODE_ALIGNMENT > 4 ) _aligned_free( p ); else free( p ); }
		void operator delete( void *p, int nBlockUse, const char *pFileName, int nLine )	{ operator delete(p); }
	};

	CTSList()
	{
		if ( ((size_t)&m_Head) % TSLIST_HEAD_ALIGNMENT != 0 )
			DebuggerBreak();

#ifdef USE_NATIVE_SLIST
		InitializeSListHead( &m_Head );
#else
		m_Head.value64 = (int64)0;
#endif
	}

	void *operator new( size_t size )														{ return _aligned_malloc( size, TSLIST_HEAD_ALIGNMENT ); }
	void *operator new( size_t size, int nBlockUse, const char *pFileName, int nLine )		{ return operator new( size ); }
	void operator delete( void *pMem )														{ _aligned_free( pMem ); }
	void operator delete( void *pMem, int nBlockUse, const char *pFileName, int nLine )		{ operator delete(pMem); }

	Node_t *Push( Node_t *pNode )
	{
		Assert( (size_t)pNode % TSLIST_NODE_ALIGNMENT == 0 );

#ifdef USE_NATIVE_SLIST
		return (Node_t *)InterlockedPushEntrySList( &m_Head, pNode );
#else
		TSLHead_t oldHead;
		TSLHead_t newHead;

		do 
		{
			oldHead.value64 = m_Head.value64;
			pNode->Next = oldHead.value.Next;
			newHead.value.Next = pNode;
			*((uint32 *)&newHead.value.Depth) = *((uint32 *)&oldHead.value.Depth) + 0x10001;

		} while( !ThreadInterlockedAssignIf64( &m_Head.value64, newHead.value64, oldHead.value64 ) );

		return (Node_t *)oldHead.value.Next;
#endif
	}

	Node_t *Pop()
	{
#ifdef USE_NATIVE_SLIST
		return (Node_t *)InterlockedPopEntrySList( &m_Head );
#else
		TSLHead_t oldHead;
		TSLHead_t newHead;

		do
		{
			oldHead.value64 = m_Head.value64;
			if ( !oldHead.value.Next )
				return NULL;

			newHead.value.Next = oldHead.value.Next->Next;
			*((uint32 *)&newHead.value.Depth) = *((uint32 *)&oldHead.value.Depth) - 1;

		} while( !ThreadInterlockedAssignIf64( &m_Head.value64, newHead.value64, oldHead.value64 ) );

		return (Node_t *)oldHead.value.Next;
#endif
	}

	void PushItem( const T &init )
	{
		Push( new Node_t( init ) );
	}

	bool PopItem( T *pResult)
	{
		Node_t *pNode = Pop();
		if ( !pNode )
			return false;
		*pResult = pNode->elem;
		delete pNode;
		return true;
	}

	Node_t *Detach()
	{
#ifdef USE_NATIVE_SLIST
		return (Node_t *)InterlockedFlushSList( &m_Head );
#else
		TSLHead_t oldHead;
		TSLHead_t newHead;

		do
		{
			oldHead.value64 = m_Head.value64;
			if ( !oldHead.value.Next )
				return NULL;

			newHead.value.Next = NULL;
			*((uint32 *)&newHead.value.Depth) = *((uint32 *)&oldHead.value.Depth) & 0xffff0000;

		} while( !ThreadInterlockedAssignIf64( &m_Head.value64, newHead.value64, oldHead.value64 ) );

		return (Node_t *)oldHead.value.Next;
#endif
	}

	int Count()
	{
#ifdef USE_NATIVE_SLIST
		return QueryDepthSList( &m_Head );
#else
		return m_Head.value.Depth;
#endif
	}

private:
	TSLHead_t m_Head;

};

//-----------------------------------------------------------------------------
// Lock free queue
//-----------------------------------------------------------------------------

template <typename T>
class TSLIST_HEAD_ALIGN CTSQueue
{
public:
	struct TSLIST_NODE_ALIGN Node_t
	{
		Node_t() {}
		Node_t( const T &init ) : elem( init ) {}

		Node_t *pNext;
		T elem;
	};

	union TSLIST_HEAD_ALIGN NodeLink_t
	{
		struct Value_t
		{
			Node_t *pNode;
			int32	sequence;
		} value;

		int64 value64;
	};

	CTSQueue()
	{
		if ( ((size_t)&m_Head) % TSLIST_HEAD_ALIGNMENT != 0 )
			DebuggerBreak();
		m_Count = 0;
		m_Head.value.sequence = m_Tail.value.sequence = 0;
		m_Head.value.pNode = m_Tail.value.pNode = &m_Dummy;
		m_Dummy.pNext = End();
	}

	Node_t *Push( Node_t *pNode )
	{
		Assert( (size_t)pNode % TSLIST_NODE_ALIGNMENT == 0 );

		NodeLink_t oldTail;
		NodeLink_t newTail;

		pNode->pNext = End();

		for (;;)
		{
			oldTail = m_Tail;
			if ( InterlockedCompareExchangeNode( &(oldTail.value.pNode->pNext), pNode, End() ) == End() )
			{
				break;
			}
			else
			{
				newTail.value.pNode = oldTail.value.pNode->pNext;
				newTail.value.sequence = oldTail.value.sequence + 1;

				InterlockedCompareExchangeNodeLink( &m_Tail, newTail, oldTail );
			}
		}

		newTail.value.pNode = pNode;
		newTail.value.sequence = oldTail.value.sequence + 1;

		InterlockedCompareExchangeNodeLink( &m_Tail, newTail, oldTail );

		m_Count++;

		return oldTail.value.pNode;
	}

	Node_t *Pop()
	{
		NodeLink_t head;
		NodeLink_t newHead;
		Node_t *pNext;
		int tailSequence;
		T elem;

		for (;;)
		{
			head = m_Head;
			tailSequence = m_Tail.value.sequence;
			pNext = head.value.pNode->pNext;

			if ( head.value.sequence == m_Head.value.sequence )
			{
				if ( head.value.pNode == m_Tail.value.pNode )
				{
					if ( pNext == End() )
					{
						return NULL;
					}
					NodeLink_t &newTail = newHead;  // just reuse locals memory
					NodeLink_t &oldTail = head;
					oldTail.value.sequence = tailSequence; // reuse head pNode
					newTail.value.pNode = pNext;
					newTail.value.sequence = tailSequence + 1;
					InterlockedCompareExchangeNodeLink( &m_Tail, newTail, oldTail );
				}
				else if ( pNext != End() )
				{
					elem = pNext->elem;
					newHead.value.pNode = pNext;
					newHead.value.sequence = head.value.sequence + 1;
					if ( InterlockedAssignNodeLinkIf( &m_Head, newHead, head ) )
					{
						break;
					}
				}
			}
		}

		m_Count--;

		head.value.pNode->elem = elem;

		return newHead.value.pNode;
	}

	void PushItem( const T &init )
	{
		Push( new Node_t( init ) );
	}

	bool PopItem( T *pResult)
	{
		Node_t *pNode = Pop();
		if ( !pNode )
			return false;
		*pResult = pNode->elem;
		delete pNode;
		return true;
	}

	int Count()
	{
		return m_Count;
	}

private:
	Node_t *End() { return (Node_t *)this; } // just need a unique signifier

#ifndef _WIN64
	Node_t *InterlockedCompareExchangeNode( Node_t * volatile *ppNode, Node_t *value, Node_t *comperand )
	{
		return (Node_t *)::ThreadInterlockedCompareExchangePointer( (void **)ppNode, value, comperand );
	}

	void InterlockedCompareExchangeNodeLink( NodeLink_t volatile *pLink, const NodeLink_t &value, const NodeLink_t &comperand )
	{
		ThreadInterlockedAssignIf64( (int64 *)pLink, value.value64, comperand.value64 );
	}

	bool InterlockedAssignNodeLinkIf( NodeLink_t volatile *pLink, const NodeLink_t &value, const NodeLink_t &comperand )
	{
		return ThreadInterlockedAssignIf64( (int64 *)pLink, value.value64, comperand.value64 );
	}

#else
	Node_t *InterlockedCompareExchangeNode( Node_t * volatile *ppNode, Node_t *value, Node_t *comperand )
	{
		AUTO_LOCK( m_ExchangeMutex );
		Node_t *retVal = *ppNode;
		if ( *ppNode == comperand )
			*ppNode = value;
		return retVal;
	}

	NodeLink_t InterlockedCompareExchangeNodeLink( NodeLink_t volatile *pLink, const NodeLink_t &value, const NodeLink_t &comperand )
	{
		AUTO_LOCK( m_ExchangeMutex );
		NodeLink_t retVal;
		retVal.value64 = pLink->value64;
		if ( pLink->value64 == comperand.value64 )
			pLink->value64  = value.value64;
		return retVal;
	}

	bool InterlockedAssignNodeLinkIf( NodeLink_t volatile *pLink, const NodeLink_t &value, const NodeLink_t &comperand )
	{
		return ( InterlockedCompareExchangeNodeLink( (int64 *)pLink, value.value64, comperand.value64 ).value64 == comperand.value64 );
	}

	CThreadFastMutex m_ExchangeMutex;
#endif

	NodeLink_t m_Head;
	NodeLink_t m_Tail;
	Node_t m_Dummy;

	CInterlockedInt m_Count;

};


#endif // TSLIST_H
