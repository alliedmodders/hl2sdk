//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef IDMELEMENT_H
#define IDMELEMENT_H
#ifdef _WIN32
#pragma once
#endif

#include "tier1/utlsymbol.h"


//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
class IDmElementInternal;
class IDmAttribute;
class IDmAttributeArray;


//-----------------------------------------------------------------------------
// handle to an IDmElement
//-----------------------------------------------------------------------------
//#define PERFORM_HANDLE_TYPECHECKING 1
#if PERFORM_HANDLE_TYPECHECKING

// this is here to make sure we're being type-safe about element handles
// otherwise, the compiler lets us cast to bool incorrectly
// the other solution would be to redefine DmElementHandle_t s.t. DMELEMENT_HANDLE_INVALID==0
struct DmElementHandle_t
{
	DmElementHandle_t() : handle( 0xffffffff ) {}
	explicit DmElementHandle_t( int h ) : handle( h ) {}
	inline bool operator==( const DmElementHandle_t &h ) const { return handle == h.handle; }
	inline bool operator!=( const DmElementHandle_t &h ) const { return handle != h.handle; }
	int handle;
};
const DmElementHandle_t DMELEMENT_HANDLE_INVALID;

#else // PERFORM_HANDLE_TYPECHECKING

enum DmElementHandle_t
{
	DMELEMENT_HANDLE_INVALID = 0xffffffff
};

#endif // PERFORM_HANDLE_TYPECHECKING


//-----------------------------------------------------------------------------
// Method of calling back into a class which contains a DmeElement.
//-----------------------------------------------------------------------------
abstract_class IDmElement
{
public:
	// Returns the element attached to the container
	virtual IDmElementInternal *GetDmElementInternal() = 0;

	// Signals when an attribute has changed value
	virtual void OnAttributeChanged( IDmAttribute *pAttribute ) = 0;

	// updates internal data from changed attributes
	virtual void Resolve() = 0;

	// RTTI-type thing. NOTE: These CUtlsymbols must be obtained through the IDataModel interface
	virtual bool IsA( UtlSymId_t typeSymbol ) const = 0;

	// Returns the handle for the element
	virtual DmElementHandle_t GetHandle() const = 0;

	// Called by the framework after the element has been unserialized
	virtual void OnElementUnserialized() = 0;

	// Signals when array elements are added/removed
	virtual void OnAttributeArrayElementAdded( IDmAttributeArray *pAttribute, int nFirstElem, int nLastElem ) = 0;
	virtual void OnAttributeArrayElementRemoved( IDmAttributeArray *pAttribute, int nFirstElem, int nLastElem ) = 0;

	// Called right before an attribute changes
	virtual void PreAttributeChanged( IDmAttribute *pAttribute ) = 0;
	virtual void PerformConstruction() = 0;
	virtual void PerformDestruction() = 0;
};


#endif // IDMELEMENT_H
