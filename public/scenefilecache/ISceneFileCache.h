//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef ISCENEFILECACHE_H
#define ISCENEFILECACHE_H
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"
#include "appframework/IAppSystem.h"

class ISceneFileCacheCallback;

class ISceneFileCache : public IAppSystem
{
public:

	virtual void OutputStatus() = 0;

	// Wipe the entire cache
	virtual void Clear() = 0;
	// Throw out the data, but keep the cache entries
	virtual void Flush() = 0;

	virtual int FindOrAddScene( char const *filename ) = 0;
	virtual size_t GetSceneBufferSize( char const *filename ) = 0;
	virtual bool GetSceneData( char const *filename, byte *buf, size_t bufsize ) = 0;
	virtual void PollForAsyncLoading( ISceneFileCacheCallback *entity, char const *pszScene ) = 0;
	virtual bool IsStillAsyncLoading( char const *filename ) = 0;
};

class ISceneFileCacheCallback
{
public:
	virtual void FinishAsyncLoading( char const *pszScene, bool binary, size_t buflen, char const *buffer ) = 0;
};

#define SCENE_FILE_CACHE_INTERFACE_VERSION "SceneFileCache001"

#endif // ISCENEFILECACHE_H
