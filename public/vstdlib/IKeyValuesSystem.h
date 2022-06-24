//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#ifndef VSTDLIB_IKEYVALUESSYSTEM_H
#define VSTDLIB_IKEYVALUESSYSTEM_H
#ifdef _WIN32
#pragma once
#endif

#include "vstdlib/vstdlib.h"

// handle to a KeyValues key name symbol
typedef int HKeySymbol;
#define INVALID_KEY_SYMBOL (-1)

//-----------------------------------------------------------------------------
// Purpose: Interface to shared data repository for KeyValues (included in vgui_controls.lib)
//			allows for central data storage point of KeyValues symbol table
//-----------------------------------------------------------------------------
class IKeyValuesSystem
{
public:
	virtual ~IKeyValuesSystem() = 0;

	// registers the size of the KeyValues in the specified instance
	// so it can build a properly sized memory pool for the KeyValues objects
	// the sizes will usually never differ but this is for versioning safety
	virtual void RegisterSizeofKeyValues(int size) = 0;

	// allocates/frees a KeyValues object from the shared mempool
	virtual void *AllocKeyValuesMemory(int size) = 0;
	virtual void FreeKeyValuesMemory(void *pMem) = 0;

	// symbol table access (used for key names)
	virtual HKeySymbol GetSymbolForString( const char *name, bool bCreate = true ) = 0;
	virtual const char *GetStringForSymbol(HKeySymbol symbol) = 0;

	// for debugging, adds KeyValues record into global list so we can track memory leaks
	virtual void AddKeyValuesToMemoryLeakList(void *pMem, HKeySymbol name) = 0;
	virtual void RemoveKeyValuesFromMemoryLeakList(void *pMem) = 0;

	// set/get a value for keyvalues resolution symbol
	// e.g.: SetKeyValuesExpressionSymbol( "LOWVIOLENCE", true ) - enables [$LOWVIOLENCE]
	virtual void SetKeyValuesExpressionSymbol( const char *name, bool bValue ) = 0;
	virtual bool GetKeyValuesExpressionSymbol( const char *name ) = 0;

	// symbol table access from code with case-preserving requirements (used for key names)
	virtual HKeySymbol GetSymbolForStringCaseSensitive( HKeySymbol &hCaseInsensitiveSymbol, const char *name, bool bCreate = true ) = 0;
};

VSTDLIB_INTERFACE IKeyValuesSystem *KeyValuesSystem();

// #define KEYVALUESSYSTEM_INTERFACE_VERSION "KeyValuesSystem002"

#endif // VSTDLIB_IKEYVALUESSYSTEM_H
