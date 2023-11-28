#ifndef IWORLDLOADUNLOAD_H
#define IWORLDLOADUNLOAD_H

#ifdef COMPILER_MSVC
#pragma once
#endif

class IWorldReference;

class IWorldLoadUnloadCallback
{
public:
	virtual void OnWorldCreate(const char *pWorldName, IWorldReference *pWorldRef) = 0;
	virtual void OnWorldDestroy(const char *pWorldName, IWorldReference *pWorldRef) = 0;
};

#endif // IWORLDLOADUNLOAD_H
