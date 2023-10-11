//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//
#if !defined( ILOOPMODE_H )
#define ILOOPMODE_H
#ifdef _WIN32
#pragma once
#endif

#include <appframework/IAppSystem.h>

class KeyValues;

abstract_class ILoopMode
{
public:
	virtual bool LoopInit(KeyValues *pKeyValues, void *a3) = 0;
	virtual void unk1(void) = 0;
	virtual void OnLoopActivate(void *a2, void* a3) = 0;
	virtual void OnLoopDeactivate(void* a2, void* a3) = 0;
	virtual void OnLoopFrameUpdate(void* a2, int a3, int a4) = 0;
	virtual void* unk3(void) = 0;
	virtual void OnLoopRender(void) = 0;
	virtual bool unk5(void) = 0;
};

abstract_class ILoopModeFactory
{
public:
	virtual void Init(void) = 0;
	virtual void unk1(void) = 0;
	virtual ILoopMode *CreateLoopMode(void) = 0;
	virtual void DestroyLoopMode(ILoopMode *pFactory) = 0;
	virtual bool unk3(void) = 0;
};



#endif // ILOOPMODE_H
