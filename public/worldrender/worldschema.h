#ifndef WORLD_SCHEMA_H
#define WORLD_SCHEMA_H

#ifdef COMPILER_MSVC
#pragma once
#endif

#include <tier0/platform.h>
#include <tier1/utlstring.h>
#include <tier1/utlvector.h>
#include <mathlib/vector.h>
#include <mathlib/vector2d.h>

//--------------------------------------------------------------------------------------
// Enum related
//--------------------------------------------------------------------------------------
enum WorldNodeFlags_t
{
	WORLD_NODE_SIMPLIFIED						= 0x0001,					// The geometry is simplified
	WORLD_NODE_UNIQUE_UV						= 0x0002,					// The geometry is uniquely mapped (likely, we're a higher LOD)
	WORLD_NODE_ATLASED							= 0x0004,					// This node was atlased but not uniquely mapped
	WORLD_NODE_KDTREE							= 0x0008,					// Node contains a kd-tree for raycasts
	WORLD_NODE_NODRAW							= 0x0010,					// Node has no actual draw calls... it's just a container for stuff and other nodes
	WORLD_NODE_START_TRAVERSAL					= 0x0020,					// Start a traversal at this node (add a check to ensure that the KDTREE flag also exists with this one)
	WORLD_NODE_CAN_SEE_SKY						= 0x0040,					// Can this node see the sky?
	WORLD_NODE_MOST_DETAILED					= 0x0080,					// Node is the most detailed node containing the original geometry and textures
};

struct WorldBuilderParams_t
{
public:
	float m_flMinDrawVolumeSize;
	bool m_bBuildBakedLighting;
	Vector2D m_vLightmapUvScale;
private:
	uint8 m_padding[3];
public:
	uint64 m_nCompileTimestamp;
	uint64 m_nCompileFingerprint;
};

struct NodeData_t
{
public:
	int32 m_nParent;
	Vector m_vOrigin;
	Vector m_vMinBounds;
	Vector m_vMaxBounds;
	float m_flMinimumDistance;
	CUtlVector< int32 > m_ChildNodeIndices;
private:
	uint8 m_padding[4];
public:
	CUtlString m_worldNodePrefix;
};

struct BakedLightingInfo_t
{
public:
	uint32 m_nLightmapVersionNumber;
	uint32 m_nLightmapGameVersionNumber;
	Vector2D m_vLightmapUvScale;
	bool m_bHasLightmaps;
private:
	uint8 m_padding[7];
public:
	CUtlVector< void * /* CStrongHandle< InfoForResourceTypeCTextureBase > */ > m_lightMaps;
};

#include <entity2/entityidentity.h> //TODO: declare CEntityIdentity/CEntityInstance as schema classes here.

#endif // WORLD_SCHEMA_H
