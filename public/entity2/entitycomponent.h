#ifndef ENTITYCOMPONENT_H
#define ENTITYCOMPONENT_H

#if _WIN32
#pragma once
#endif

#include "tier0/platform.h"
#include "tier1/utlsymbollarge.h"
#include "tier1/utlstring.h"
#include "datamap.h"

class CEntityIdentity;
class CEntityComponentHelper;
struct ComponentUnserializerClassInfo_t;
struct EntOutput_t;

struct ComponentUnserializerKeyNamesChunk_t
{
	CUtlStringToken m_keyNames[4];
};

struct ComponentUnserializerFieldInfo_t
{
	void* m_pSchemaEnum;
	const char* m_pKeyName;
	
	uint16 m_nOffset;
	uint16 m_nArraySize;
	
	fieldtype_t m_Type;
	
	bool m_bUnserializeAsMatrix : 1;
	bool m_bArrayElement : 1;
	bool m_bRemovedKeyField : 1;
};

struct ComponentUnserializerPtrToClassInfo_t
{
	uint m_nOffset;
	ComponentUnserializerClassInfo_t* m_pClassInfo;
};

struct ComponentUnserializerClassInfo_t
{
	ComponentUnserializerClassInfo_t* m_pBaseClassInfo;

	ComponentUnserializerKeyNamesChunk_t* m_pKeyNamesChunks;
	ComponentUnserializerFieldInfo_t* m_pFieldInfos;
	EntOutput_t* m_pOutputs;
	ComponentUnserializerPtrToClassInfo_t* m_pClassInfoPtrs;

	uint16 m_nFieldInfoCount;
	uint16 m_nKeyNamesChunkCount;
	uint16 m_nOutputCount;
	uint16 m_nClassInfoPtrCount;
};

struct EntComponentInfo_t
{
	const char* m_pName;
	const char* m_pCPPClassname;
	const char* m_pNetworkDataReferencedDescription;
	const char* m_pNetworkDataReferencedPtrPropDescription;
	int m_nRuntimeIndex;
	uint m_nFlags;
	ComponentUnserializerClassInfo_t m_componentUnserializerClassInfo;
	void* m_pScriptDesc;
	CEntityComponentHelper* m_pBaseClassComponentHelper;
};

class CEntityComponentHelper
{
public:
	virtual void Schema_DynamicBinding(void**) = 0;
	virtual void Finalize() = 0;
	virtual void GetSchemaBinding(void**) = 0;
	virtual datamap_t* GetDataDescMap() = 0;
	virtual bool Allocate( CEntityIdentity* pEntity, void* pComponent ) = 0;
	virtual void Free( CEntityIdentity* pEntity, void* pComponent ) = 0;

public:
	uint m_flags;
	EntComponentInfo_t* m_pInfo;
	int m_nPriority;
	CEntityComponentHelper* m_pNext;
};

class CEntityComponent
{
private:
	uint8 unknown[0x8]; // 0x0
};

class CScriptComponent : public CEntityComponent
{
private:
	uint8 unknown[0x28]; // 0x8
public:
	CUtlSymbolLarge m_scriptClassName; // 0x30
};

#endif // ENTITYCOMPONENT_H