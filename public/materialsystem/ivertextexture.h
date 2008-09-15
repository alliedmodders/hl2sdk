//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
// Base texture interface for vertex textures
//=============================================================================

#ifndef IVERTEXTEXTURE_H
#define IVERTEXTEXTURE_H

#ifdef _WIN32
#pragma once
#endif


#include "tier0/platform.h"

//-----------------------------------------------------------------------------
// A vertex texture.
//-----------------------------------------------------------------------------
abstract_class IVertexTexture
{
public:
	// Returns the element + field count
	virtual int GetElementCount() const = 0;
	virtual int GetFieldCount() const = 0;

	// Locks the vertex texture for writing
	virtual void Lock() = 0;
	virtual void Unlock() = 0;

	// Writes elements into the vertex texture; can only be used when locked.
	// WriteElements write all fields of entire elements
	// WriteElementField writes a single field of an element
	virtual void WriteElements( int nFirstElement, const float* pValues, int nElementCount = 1 ) = 0;
	virtual void WriteElementField( int nElement, int nFieldIndex, float flValue ) = 0;

	// Uses for stats. . .get the approximate size of the texture in it's current format.
	virtual int GetApproximateVidMemBytes( void ) const = 0;
};


#endif // IVERTEXTEXTURE_H
