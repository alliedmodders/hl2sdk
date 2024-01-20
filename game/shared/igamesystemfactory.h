//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef IGAMESYSTEMFACTORY_H
#define IGAMESYSTEMFACTORY_H
#ifdef _WIN32
#pragma once
#endif

#include "string.h"
#include "igamesystem.h"

/*
* AMNOTE: To register your own game system, CGameSystemStaticFactory could be used to register it,
* but since the actual factory lives inside the game originally and we don't have direct access to it
* you need provide the CBaseGameSystemFactory::sm_pFirst by whatever means are possible to you.
* As a starting point you can look for "Game System %s is defined twice" string in the server binary
* to find it, if that string is missing then this comment is out of date, please, report or update it!
* 
* Once CBaseGameSystemFactory::sm_pFirst is initialized and correct address provided,
* you can create your own gamesystem (refer to igamesystem.h on what it should be like) and register it
* via:
* 
* // Factory object would be returned, and you are responsible for freeing it inside the ``Shutdown`` method of ``YourGameSystemClass``
* // common way to do that would be to store the factory as a static member in ``YourGameSystemClass`` and free it when ``Shutdown`` is called.
* IGameSystemFactory factory = new CGameSystemStaticFactory<YourGameSystemClass>( "YourGameSystemName", &PointerToYourGameSystemInstance );
* 
* after the new factory object has been created and if all is done correctly your game system should be viewable to the game
* and its callbacks should trigger.
*/

abstract_class IGameSystemFactory
{
public:
	virtual bool Init() = 0;
	virtual void PostInit() = 0;
	virtual void Shutdown() = 0;
	virtual IGameSystem* CreateGameSystem() = 0;
	virtual void DestroyGameSystem(IGameSystem* pGameSystem) = 0;
	virtual bool ShouldAutoAdd() = 0;
	virtual int GetPriority() = 0;
	virtual void SetGlobalPtr(void* pValue) = 0;
	virtual bool IsReallocating() = 0;
	virtual IGameSystem* GetStaticGameSystem() = 0;
};

class CBaseGameSystemFactory : public IGameSystemFactory
{
protected:
	CBaseGameSystemFactory(const char* pName)
	{
		m_pName = pName;
		m_pNext = *CBaseGameSystemFactory::sm_pFirst;
		*CBaseGameSystemFactory::sm_pFirst = this;
	}

private:
	CBaseGameSystemFactory* m_pNext;
	const char* m_pName;

public:
	static IGameSystem* GetGlobalPtrByName(const char* pName)
	{
		CBaseGameSystemFactory* pFactoryList = *CBaseGameSystemFactory::sm_pFirst;
		while (pFactoryList)
		{
			if (strcmp(pFactoryList->m_pName, pName) == 0)
			{
				return pFactoryList->GetStaticGameSystem();
			}
			pFactoryList = pFactoryList->m_pNext;
		}
		return nullptr;
	}

	// AMNOTE: This needs to be provided by a thirdparty application,
	// and is required if you want to register your own gamesystem
	static CBaseGameSystemFactory** sm_pFirst;
};

template <class T, class U = T>
class CGameSystemStaticFactory : public CBaseGameSystemFactory
{
public:
	CGameSystemStaticFactory(const char* pName, T* pActualGlobal, U** ppGlobalPointer = nullptr) : CBaseGameSystemFactory(pName)
	{
		m_pActualGlobal = pActualGlobal;
		m_ppGlobalPointer = ppGlobalPointer;
		pActualGlobal->SetName(pName);
	}

	bool Init() override { return m_pActualGlobal->Init(); }
	void PostInit() override { m_pActualGlobal->PostInit(); }
	void Shutdown() override { m_pActualGlobal->Shutdown(); }

	IGameSystem* CreateGameSystem() override
	{
		m_pActualGlobal->SetGameSystemGlobalPtrs(m_pActualGlobal);
		return m_pActualGlobal;
	}

	void DestroyGameSystem(IGameSystem* pGameSystem) override
	{
		m_pActualGlobal->SetGameSystemGlobalPtrs(nullptr);
	}

	bool ShouldAutoAdd() override { return true; }
	int GetPriority() override { return 0; }

	void SetGlobalPtr(void* pValue) override
	{
		if (m_ppGlobalPointer)
		{
			*m_ppGlobalPointer = reinterpret_cast<T*>(pValue);
		}
	}

	bool IsReallocating() override { return false; }
	IGameSystem* GetStaticGameSystem() override { return m_pActualGlobal; }

private:
	T* m_pActualGlobal;
	U** m_ppGlobalPointer;
};

template <class T, class U = T>
class CGameSystemReallocatingFactory : public CBaseGameSystemFactory
{
public:
	CGameSystemReallocatingFactory(const char* pName, U** ppGlobalPointer = nullptr) : CBaseGameSystemFactory(pName)
	{
		m_ppGlobalPointer = ppGlobalPointer;
	}

	bool Init() override { return true; }
	void PostInit() override { }
	void Shutdown() override { }

	IGameSystem* CreateGameSystem() override
	{
		T* pObject = new T();
		pObject->SetGameSystemGlobalPtrs(pObject);
		return pObject;
	}

	void DestroyGameSystem(IGameSystem* pGameSystem) override
	{
		pGameSystem->SetGameSystemGlobalPtrs(nullptr);
		delete pGameSystem;
	}

	bool ShouldAutoAdd() override { return true; }
	int GetPriority() override { return 0; }

	void SetGlobalPtr(void* pValue) override
	{
		if (m_ppGlobalPointer)
		{
			*m_ppGlobalPointer = reinterpret_cast<T*>(pValue);
		}
	}

	bool IsReallocating() override { return true; }
	IGameSystem* GetStaticGameSystem() override { return nullptr; }

private:
	U** m_ppGlobalPointer;
};

#endif // IGAMESYSTEMFACTORY_H
