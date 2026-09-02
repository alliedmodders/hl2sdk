//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef ISOUNDOPSYSTEM_H
#define ISOUNDOPSYSTEM_H
#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h"

class CUtlString;

abstract_class CSoundEventManager
{
public:
	virtual uint32 GetSoundEventHash( const char *pSoundEvent ) = 0;
	virtual bool IsValidSoundEventHash( uint32 nSoundEvent ) = 0;
	virtual const char *GetSoundEventName( uint32 nSoundEvent ) = 0;
	virtual bool HasSoundEvent( const char *pSoundEvent ) = 0;

private:
	virtual void *GetSoundEventByHash( uint32 nSoundEvent, bool bWarn ) = 0;
	virtual void *GetSoundEventByName( const char *pSoundEvent ) = 0;
	virtual void unk001() = 0;
	virtual void unk002() = 0;
	virtual void unk003() = 0;
	virtual void unk004() = 0;
	virtual void unk005() = 0;
	virtual void unk006() = 0;
	virtual void unk007() = 0;
	virtual void unk008() = 0;
	virtual void unk009() = 0;
	virtual void unk010() = 0;

public:
	virtual uint32 GetSoundEventStackHash( uint32 nSoundEvent ) = 0;
	virtual uint32 GetSoundEventStackHash( const char *pSoundEvent ) = 0;
	virtual void GetSoundEventStackName( uint32 nSoundEvent, CUtlString &sStackName ) = 0;

private:
	virtual void unk101() = 0;
	virtual void unk102() = 0;

public:
	virtual uint32 GetSoundEventDefinitionBaseHash( uint32 nSoundEvent, int nIndex ) = 0;
	virtual uint32 GetSoundEventDefinitionBaseHash( const char *pSoundEvent, int nIndex ) = 0;
	virtual void GetSoundEventDefinitionBaseName( uint32 nSoundEvent, CUtlString &sBaseName, int nIndex ) = 0;
	virtual void GetSoundEventDefinitionBaseField( uint32 nSoundEvent, CUtlString &sBaseField, int nIndex ) = 0;
	virtual int GetSoundEventDefinitionBaseCount( uint32 nSoundEvent ) = 0;
	virtual int GetSoundEventDefinitionBaseCount( const char *pSoundEvent ) = 0;

private:
	virtual void unk201() = 0;

public:
	virtual void *GetSoundEventUpdateGroups( uint32 nSoundEvent ) = 0;
	virtual void *GetSoundEventGroups( const char *pSoundEvent ) = 0;

private:
	virtual void unk301() = 0;
	virtual void unk302() = 0;
	virtual void unk303() = 0;
	virtual void unk304() = 0;

public:
	virtual void PreloadSoundEvent( uint32 nSoundEvent, int16 nPriority ) = 0;
	virtual void PreloadSoundEvent( const char *pSoundEvent, int16 nPriority ) = 0;
};

class ISoundOpSystem
{
public:
	CSoundEventManager *GetSoundEventManager()
	{
		uint8 *pAddress = reinterpret_cast<uint8 *>( this );
		return reinterpret_cast<CSoundEventManager *>( pAddress + sizeof( void * ) );
	}
};

#endif // ISOUNDOPSYSTEM_H
