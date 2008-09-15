//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef DMELEMENTUTILS_H
#define DMELEMENTUTILS_H
#ifdef _WIN32
#pragma once
#endif

#include "tier1/utlstring.h"
#include "datamodel/idmattribute.h"
#include "datamodel/idmelementinternal.h"


//-----------------------------------------------------------------------------
// Utility methods
//-----------------------------------------------------------------------------
template< class T >
inline bool DmElementFindOrAddAttribute( IDmElementInternal *pElement, const char *pAttributeName, const T &value )
{
	IDmAttribute *pAttribute = pElement->FindOrAddAttribute( pAttributeName, CDmAttributeInfo<T>::AttributeType() );
	if ( pAttribute )
	{
		DmAttributeSetValue<T>( pAttribute, value );
		return true;
	}
	return false;
}

// TEST - to find where we're accidentally calling <DmElementHandle_t>
template<>
inline bool DmElementFindOrAddAttribute( IDmElementInternal *pElement, const char *pAttributeName, const DmElementHandle_t &value )
{
	Assert( 0 );
	return false;
}

inline bool DmElementFindOrAddAttribute( IDmElementInternal *pElement, const char *pAttributeName, const char *value )
{
	int nLen = value ? Q_strlen( value ) + 1 : 0;
	CUtlString str( value, nLen );
	return DmElementFindOrAddAttribute( pElement, pAttributeName, str );
}

inline bool DmElementFindOrAddAttribute( IDmElementInternal *pElement, const char *pAttributeName, IDmElement *pValue )
{
	CDmElementRef elementRef( pValue ? pValue->GetHandle() : DMELEMENT_HANDLE_INVALID );
	return DmElementFindOrAddAttribute( pElement, pAttributeName, elementRef );
}

inline bool DmElementFindOrAddAttribute( IDmElementInternal *pElement, const char *pAttributeName, IDmElementInternal *pValue )
{
	CDmElementRef elementRef( pValue ? pValue->GetElement() : DMELEMENT_HANDLE_INVALID );
	return DmElementFindOrAddAttribute( pElement, pAttributeName, elementRef );
}


//-----------------------------------------------------------------------------
// Utility using handles
//-----------------------------------------------------------------------------
inline IDmAttribute* DmElementGetAttribute( DmElementHandle_t hElement, const char *pAttributeName )
{
	IDmElementInternal *pElement = g_pDataModel->GetElementInternal( hElement );
	return pElement ? pElement->GetAttribute( pAttributeName ) : NULL;
}

template< class T >
inline bool DmElementFindOrAddAttribute( DmElementHandle_t hElement, const char *pAttributeName, const T &value )
{
	IDmElementInternal *pElement = g_pDataModel->GetElementInternal( hElement );
	if ( !pElement )
		return false;

	return DmElementFindOrAddAttribute( pElement, pAttributeName, value );
}

inline bool DmElementFindOrAddAttribute( DmElementHandle_t hElement, const char *pAttributeName, const char *value )
{
	IDmElementInternal *pElement = g_pDataModel->GetElementInternal( hElement );
	if ( !pElement )
		return false;

	return DmElementFindOrAddAttribute( pElement, pAttributeName, value );
}

inline bool DmElementFindOrAddAttribute( DmElementHandle_t hElement, const char *pAttributeName, IDmElement *pValue )
{
	IDmElementInternal *pElement = g_pDataModel->GetElementInternal( hElement );
	if ( !pElement )
		return false;

	return DmElementFindOrAddAttribute( pElement, pAttributeName, pValue );
}

inline bool DmElementFindOrAddAttribute( DmElementHandle_t hElement, const char *pAttributeName, IDmElementInternal *pValue )
{
	IDmElementInternal *pElement = g_pDataModel->GetElementInternal( hElement );
	if ( !pElement )
		return false;

	return DmElementFindOrAddAttribute( pElement, pAttributeName, pValue );
}



#endif // DMELEMENTUTILS_H
