#ifndef FLATTENEDSERIALIZERS_H
#define FLATTENEDSERIALIZERS_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/basetypes.h"
#include "tier1/utlsymbollarge.h"

DECLARE_POINTER_HANDLE( FlattenedSerializerHandle_t );

struct FlattenedSerializerDesc_t
{
	CUtlSymbolLarge m_name;
	FlattenedSerializerHandle_t m_handle;
};

#endif /* FLATTENEDSERIALIZERS_H */
