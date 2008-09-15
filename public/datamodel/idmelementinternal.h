//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef IDMELEMENTINTERNAL_H
#define IDMELEMENTINTERNAL_H
#ifdef _WIN32
#pragma once
#endif

#include "datamodel/idatamodel.h"
#include "datamodel/idmelement.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class IDmElementInternal;
class IDmAttribute;
class IDmElement;
struct DmObjectId_t;


//-----------------------------------------------------------------------------
// Purpose: An IDmElementInternal is a container/node which has
//  a list of attributes of core or extended types
//-----------------------------------------------------------------------------
abstract_class IDmElementInternal
{
public:
	virtual void			AddRef() = 0;
	virtual int				Release() = 0;

	// The IDmElement that this IDmElementInternal is attached to...
	virtual		  DmElementHandle_t GetElement() = 0;
	virtual const DmElementHandle_t	GetElement() const = 0;
	virtual	void			AttachToElement( DmElementHandle_t hElement ) = 0;

	// Is the element dirty?
	virtual bool			IsDirty() const = 0;
	virtual void			MarkDirty( bool dirty = true ) = 0;
	virtual void			MarkAttributesClean() = 0;
	virtual void			MarkBeingUnserialized( bool beingUnserialized = true ) = 0;
	virtual bool			IsBeingUnserialized() const = 0;

	// Attribute manipulation
	virtual IDmAttribute	*FindOrAddAttribute( char const *pAttributeName, DmAttributeType_t type ) = 0;
	virtual IDmAttribute	*AddAttribute( char const *pAttributeName, DmAttributeType_t type ) = 0;
	virtual IDmAttribute	*AddExternalAttribute( char const *pAttributeName, DmAttributeType_t type, void *pMemory ) = 0;
	virtual void			RemoveAttribute( char const *pAttributeName ) = 0;
	virtual bool			HasAttribute( char const *pAttributeName ) const = 0;
	virtual void			RemoveAttributeByPtr( IDmAttribute *ptr ) = 0;

// 	virtual void			SetAttribute( char const *pAttributeName, IDmAttribute *pAttribute ) = 0;
	virtual IDmAttribute	*GetAttribute( char const *pAttributeName ) = 0;
	virtual const IDmAttribute *GetAttribute( char const *pAttributeName ) const = 0;

	// Attribute iteration
	virtual int				NumAttributes() const = 0;
	virtual IDmAttribute	*FirstAttribute() = 0;
	virtual const IDmAttribute *FirstAttribute() const = 0;

	virtual void			RenameAttribute( char const *pAttributeName, char const *pNewName ) = 0;

	// Utility method for getting at the type, name, and id
	virtual const char*		GetType() const = 0;
	virtual const char*		GetName() const = 0;
	virtual const DmObjectId_t &GetId() const = 0;
};


#endif // IDMELEMENTINTERNAL_H
