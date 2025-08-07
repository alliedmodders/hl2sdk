#ifndef TIER1_THREADTOOLS_H
#define TIER1_THREADTOOLS_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/threadtools.h"

enum CopiedLockState_t : int32
{
	CLS_NOCOPY = 0,
	CLS_UNLOCKED = 1,
	CLS_LOCKED_BY_COPYING_THREAD = 2,
};

template < class MUTEX, CopiedLockState_t L = CLS_UNLOCKED >
class CCopyableLock : public MUTEX
{
	typedef MUTEX BaseClass;

public:
	// ...
};

#endif // TIER1_THREADTOOLS_H
