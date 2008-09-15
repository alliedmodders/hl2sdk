//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Defines a group of app systems that all have the same lifetime
// that need to be connected/initialized, etc. in a well-defined order
//
// $Revision: $
// $NoKeywords: $
//=============================================================================

#ifndef IAPPSYSTEMGROUP_H
#define IAPPSYSTEMGROUP_H

#ifdef _WIN32
#pragma once
#endif


#include "tier1/interface.h"
#include "tier1/utlvector.h"
#include "tier1/utldict.h"
#include "IAppSystem.h"

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
class IAppSystem;
class CSysModule;
class IBaseInterface;
class IFileSystem;

//-----------------------------------------------------------------------------
// Handle to a DLL
//-----------------------------------------------------------------------------
typedef int AppModule_t;

enum
{
	APP_MODULE_INVALID = (AppModule_t)~0
};


//-----------------------------------------------------------------------------
// NOTE: The following methods must be implemented in your application
// although they can be empty implementations if you like...
//-----------------------------------------------------------------------------
abstract_class IAppSystemGroup
{
public:
	// An installed application creation function, you should tell the group
	// the DLLs and the singleton interfaces you want to instantiate.
	// Return false if there's any problems and the app will abort
	virtual bool Create( ) = 0;

	// Allow the application to do some work after AppSystems are connected but 
	// they are all Initialized.
	// Return false if there's any problems and the app will abort
	virtual bool PreInit() = 0;

	// Main loop implemented by the application
	virtual int Main( ) = 0;

	// Allow the application to do some work after all AppSystems are shut down
	virtual void PostShutdown() = 0;

	// Call an installed application destroy function, occurring after all modules
	// are unloaded
	virtual void Destroy() = 0;
};


//-----------------------------------------------------------------------------
// Specifies a module + interface name for initialization
//-----------------------------------------------------------------------------
struct AppSystemInfo_t
{
	const char *m_pModuleName;
	const char *m_pInterfaceName;
};


//-----------------------------------------------------------------------------
// This class represents a group of app systems that all have the same lifetime
// that need to be connected/initialized, etc. in a well-defined order
//-----------------------------------------------------------------------------
class CAppSystemGroup : public IAppSystemGroup
{
public:
	// Used to determine where we exited out from the system
	enum AppSystemGroupStage_t
	{
		CREATION = 0,
		CONNECTION,
		PREINITIALIZATION,
		INITIALIZATION,
		SHUTDOWN,
		POSTSHUTDOWN,
		DISCONNECTION,
		DESTRUCTION,

		NONE,	// This means no error
	};

public:
	// constructor
	CAppSystemGroup( CAppSystemGroup *pParentAppSystem = NULL );

	// Runs the app system group.
	// First, modules are loaded, next they are connected, followed by initialization
	// Then Main() is run
	// Then modules are shut down, disconnected, and unloaded
	int Run( );

	// Returns the stage at which the app system group ran into an error
	AppSystemGroupStage_t GetErrorStage() const;

protected:
	// These methods are meant to be called by derived classes of CAppSystemGroup

	// Methods to load + unload DLLs
	AppModule_t LoadModule( const char *pDLLName );
	AppModule_t LoadModule( CreateInterfaceFn factory );

	// Method to add various global singleton systems 
	IAppSystem *AddSystem( AppModule_t module, const char *pInterfaceName );

	// Simpler method of doing the LoadModule/AddSystem thing.
	// Make sure the last AppSystemInfo has a NULL module name
	bool AddSystems( AppSystemInfo_t *pSystems );

	// Method to look up a particular named system...
	void *FindSystem( const char *pInterfaceName );

	// Gets at a class factory for the topmost appsystem group in an appsystem stack
	static CreateInterfaceFn GetFactory();

private:
	void UnloadAllModules( );
	void RemoveAllSystems();

	// Method to connect/disconnect all systems
	bool ConnectSystems( );
	void DisconnectSystems();

	// Method to initialize/shutdown all systems
	InitReturnVal_t InitSystems();
	void ShutdownSystems();
 
	// Gets at the parent appsystem group
	CAppSystemGroup *GetParent();

	// Loads a module the standard way
	virtual CSysModule *LoadModuleDLL( const char *pDLLName );

	struct Module_t
	{
		CSysModule *m_pModule;
		CreateInterfaceFn m_Factory;
		char *m_pModuleName;
	};

	CUtlVector<Module_t> m_Modules;
	CUtlVector<IAppSystem*> m_Systems;
	CUtlDict<int, unsigned short> m_SystemDict;
	CAppSystemGroup *m_pParentAppSystem;
	AppSystemGroupStage_t m_nErrorStage;

	friend void *AppSystemCreateInterfaceFn(const char *pName, int *pReturnCode);
	friend class CSteamAppSystemGroup;
};


//-----------------------------------------------------------------------------
// This class represents a group of app systems that are loaded through steam
//-----------------------------------------------------------------------------
class CSteamAppSystemGroup : public CAppSystemGroup
{
public:
	CSteamAppSystemGroup( IFileSystem *pFileSystem = NULL, CAppSystemGroup *pParentAppSystem = NULL );

	// Used by CSteamApplication to set up necessary pointers if we can't do it in the constructor
	void Setup( IFileSystem *pFileSystem, CAppSystemGroup *pParentAppSystem );

private:
	virtual CSysModule *LoadModuleDLL( const char *pDLLName );

	IFileSystem *m_pFileSystem;
};


//-----------------------------------------------------------------------------
// Helper empty decorator implementation of an IAppSystemGroup
//-----------------------------------------------------------------------------
template< class CBaseClass > 
class CDefaultAppSystemGroup : public CBaseClass
{
public:
	virtual bool Create( ) { return true; }
	virtual bool PreInit() { return true; }
	virtual void PostShutdown() {}
	virtual void Destroy() {}
};


#endif // APPSYSTEMGROUP_H


