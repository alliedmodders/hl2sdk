#ifndef IGAMERESOURCEMANIFESTLOADCOMPLETION_H
#define IGAMERESOURCEMANIFESTLOADCOMPLETION_H

#ifdef COMPILER_MSVC
#pragma once
#endif

#include "resourcetype.h"

class IGameResourceManifestLoadCompletionCallback
{
public:
	virtual void OnGameResourceManifestLoaded(HGameResourceManifest hManifest, int nResourceCount, ResourceHandle_t *pResourceHandles) = 0;
};

#endif // IGAMERESOURCEMANIFESTLOADCOMPLETION_H
