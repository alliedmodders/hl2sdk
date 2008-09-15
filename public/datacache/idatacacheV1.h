//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef IDATACACHEV1_H
#define IDATACACHEV1_H
#ifdef _WIN32
#pragma once
#endif


namespace DataCacheV1
{

//-----------------------------------------------------------------------------
// Purpose: Handle to a cached piece of data.
// This pointer goes to NULL if the cache manager
// needs to bump the object out of the heap/cache.
// DO NOT CHANGE
//-----------------------------------------------------------------------------
struct cache_user_t
{
	void *data;
};


//-----------------------------------------------------------------------------
// IDataCache (NOTE: This used to be IVEngineCache )
//-----------------------------------------------------------------------------

#define DATACACHE_INTERFACE_VERSION_1		"VEngineCache001"

abstract_class IDataCache
{
public:
	virtual void Flush( void ) = 0;
	
	// returns the cached data, and moves to the head of the LRU list
	// if present, otherwise returns NULL
	virtual void *Check( cache_user_t *c ) = 0;

	virtual void Free( cache_user_t *c ) = 0;
	
	// Returns NULL if all purgable data was tossed and there still
	// wasn't enough room.
	virtual void *Alloc( cache_user_t *c, int size, const char *name ) = 0;
	
	virtual void Report( void ) = 0;
	
	// all cache entries that subsequently allocated or successfully checked 
	// are considered "locked" and will not be freed when additional memory is needed
	virtual void BeginLock() = 0;

	// reset all protected blocks to normal
	virtual void EndLock() = 0;
};

} // end namespace

#endif // IDATACACHEV1_H
