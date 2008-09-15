//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef ATTRIBUTEFLAGS_H
#define ATTRIBUTEFLAGS_H

#ifdef _WIN32
#pragma once
#endif

enum
{
	FATTRIB_NONE			= 0,
	FATTRIB_READONLY		= (1<<0), // Don't allow editing value in editors
	FATTRIB_DONTSAVE		= (1<<1), // Don't persist to .dme file
	FATTRIB_DIRTY			= (1<<2), // Indicates the attribute has been changed since the resolve phase
	FATTRIB_DEFINED			= (1<<3), // Indicates the attribute has been set to a value
	FATTRIB_HAS_CALLBACK	= (1<<4), // Indicates that this will notify its owner and/or other elements when it changes
	FATTRIB_EXTERNAL		= (1<<5), // Indicates this attribute's data is externally owned (in a CDmElement somewhere)
	FATTRIB_TOPOLOGICAL		= (1<<6), // Indicates this attribute effects the scene's topology (ie it's an attribute name or element)
	FATTRIB_MUSTCOPY		= (1<<7), // parent element must make a new copy during CopyInto, even for shallow copy
	FATTRIB_STANDARD		= (1<<8), // This flag is set if it's a "standard" attribute, namely "name", "type", or "id"
	FATTRIB_USERDEFINED		= (1<<9), // This flag is used to sort attributes in the element properties view. User defined flags come last.
	FATTRIB_NODUPLICATES	= (1<<10),// For element array types, disallows duplicate values from being inserted into the array.
	FATTRIB_HAS_ARRAY_CALLBACK	= (1<<11), // Indicates that this will notify its owner and/or other elements array elements changes. Note that when elements shift (say, inserting at head, or fast remove), callbacks are not executed for these elements.
	FATTRIB_HAS_PRE_CALLBACK	= (1<<12), // Indicates that this will notify its owner and/or other elements right before it changes
	FATTRIB_OPERATOR_DIRTY	= (1<<13),// Used and cleared only by operator phase of datamodel
};

#endif // ATTRIBUTEFLAGS_H
