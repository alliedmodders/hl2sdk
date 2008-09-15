//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef DMELEMENTREF_H
#define DMELEMENTREF_H
#ifdef _WIN32
#pragma once
#endif

#include "datamodel/idmelementinternal.h" 
#include "datamodel/idatamodel.h"


//-----------------------------------------------------------------------------
// Defined in tier1.lib
//-----------------------------------------------------------------------------
extern IDataModel *g_pDataModel;


//-----------------------------------------------------------------------------
// Refcounted wrapper around a DmElementHandle_t
//-----------------------------------------------------------------------------
class CDmElementRef
{
public:
	// Constructor, destructors
	CDmElementRef();
	CDmElementRef( const CDmElementRef& that );
	CDmElementRef( DmElementHandle_t hElement );
	~CDmElementRef();

	// Get/set operations
	DmElementHandle_t Get() const;
	operator DmElementHandle_t() const;
	void Set( DmElementHandle_t hElement );
	CDmElementRef &operator=( const CDmElementRef &src );

protected:
	// Refcounting
	void AddRef();
	void Release();

protected:
	DmElementHandle_t m_hElement;
};


//-----------------------------------------------------------------------------
// Constructor, destructors
//-----------------------------------------------------------------------------
inline CDmElementRef::CDmElementRef() : m_hElement( DMELEMENT_HANDLE_INVALID )
{
}

inline CDmElementRef::CDmElementRef( const CDmElementRef& that ) : m_hElement( DMELEMENT_HANDLE_INVALID )
{
	Set( that.m_hElement );
}

inline CDmElementRef::CDmElementRef( DmElementHandle_t hElement ) : m_hElement( DMELEMENT_HANDLE_INVALID )
{
	Set( hElement );
}

inline CDmElementRef::~CDmElementRef()
{
	Release();
}


//-----------------------------------------------------------------------------
// Get/set operations
//-----------------------------------------------------------------------------
inline DmElementHandle_t CDmElementRef::Get() const
{
	return m_hElement;
}

inline CDmElementRef::operator DmElementHandle_t() const
{
	return m_hElement;
}

inline void CDmElementRef::Set( DmElementHandle_t hElement )
{
	if ( m_hElement != hElement )
	{
		Release();
		m_hElement = hElement;
		AddRef();
	}
}

inline CDmElementRef &CDmElementRef::operator=( const CDmElementRef &src )
{
	Set( src.Get() );
	return *this;
}
	

//-----------------------------------------------------------------------------
// CDmElementRef ref counting implementation
//-----------------------------------------------------------------------------
inline void CDmElementRef::AddRef()
{
	if ( m_hElement != DMELEMENT_HANDLE_INVALID )
	{
		IDmElementInternal *pInternal = g_pDataModel->GetElementInternal( m_hElement );
		if ( pInternal )
		{
			pInternal->AddRef();
		}
	}
}

inline void CDmElementRef::Release()
{
	if ( m_hElement != DMELEMENT_HANDLE_INVALID )
	{
		IDmElementInternal *pInternal = g_pDataModel->GetElementInternal( m_hElement );
		if ( pInternal )
		{
			pInternal->Release();
		}
		m_hElement = DMELEMENT_HANDLE_INVALID;
	}
}


#endif // DMELEMENTREF_H
