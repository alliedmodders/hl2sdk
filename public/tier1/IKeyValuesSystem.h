//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
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

#include "tier0/platform.h"

class CUtlCharConversion;
class CUtlScratchMemoryPool;
class CTemporaryKeyValues;
class KeyValues;

class HKeySymbol
{
public:
	HKeySymbol() : nIndex(~0) { }
	HKeySymbol(uint32 idx) : nIndex(idx) { }

	inline uint32 Get() { return nIndex; }

private:
	uint32 nIndex;
};

//-----------------------------------------------------------------------------
// Purpose: Interface to shared data repository for KeyValues (included in vgui_controls.lib)
//			allows for central data storage point of KeyValues symbol table
//-----------------------------------------------------------------------------
class IKeyValuesSystem
{
public:
	virtual ~IKeyValuesSystem() = 0;

	// allocates/frees a KeyValues object from the shared mempool
	virtual KeyValues *AllocKeyValuesMemory() = 0;
	virtual void FreeKeyValuesMemory(KeyValues *pKV) = 0;

	// symbol table access (used for key names)
	virtual HKeySymbol GetSymbolForString( const char *name, bool bCreate = true ) = 0;
	virtual const char *GetStringForSymbol(HKeySymbol symbol) = 0;

	// for debugging, adds KeyValues record into global list so we can track memory leaks
	virtual void AddKeyValuesToMemoryLeakList(void *pMem, HKeySymbol name) = 0;
	virtual void RemoveKeyValuesFromMemoryLeakList(void *pMem) = 0;

	virtual void unk001() = 0;

	// set/get a value for keyvalues resolution symbol
	// e.g.: SetKeyValuesExpressionSymbol( "LOWVIOLENCE", true ) - enables [$LOWVIOLENCE]
	virtual void SetKeyValuesExpressionSymbol( const char *name, bool bValue ) = 0;
	virtual bool GetKeyValuesExpressionSymbol( const char *name ) = 0;

	// symbol table access from code with case-preserving requirements (used for key names)
	virtual HKeySymbol GetSymbolForStringCaseSensitive( HKeySymbol &hCaseInsensitiveSymbol, const char *name, bool bCreate = true ) = 0;
	virtual HKeySymbol GetCaseInsensitiveSymbolFromCaseSensitiveSymbol( HKeySymbol symbol ) = 0;
	
	virtual const char *CopyString( const char * ) = 0;
	virtual void ReleaseStringCopy( const char * ) = 0;
	virtual const wchar_t *CopyWString( const wchar_t * ) = 0;
	virtual void ReleaseWStringCopy( const wchar_t * ) = 0;

	virtual CUtlCharConversion *GetCharacterConversion( bool is_cstring ) = 0;

	virtual CTemporaryKeyValues *AllocateTemporaryKeyValues() = 0;
	virtual void ReleaseTemporaryKeyValues( CTemporaryKeyValues *temp_kv ) = 0;

	// Returns previously used memory pool if any
	virtual CUtlScratchMemoryPool *SetNewScratchMemoryPool( CUtlScratchMemoryPool *pool ) = 0;
};

PLATFORM_INTERFACE IKeyValuesSystem *KeyValuesSystem();

#endif // VSTDLIB_IKEYVALUESSYSTEM_H
