#ifndef WORLD_SCHEMA_H
#define WORLD_SCHEMA_H

#ifdef COMPILER_MSVC
#pragma once
#endif

#include <tier0/platform.h>
#include <mathlib/vector2d.h>

// See tier0/basetypes.h :
//   AlliedModders: We don't need this, and it conflicts with generated proto headers
#ifndef schema
#define schema
#endif

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

schema struct WorldBuilderParams_t
{
public:
	float m_flMinDrawVolumeSize;
	bool m_bBuildBakedLighting;
	Vector2D m_vLightmapUvScale;
	uint64 m_nCompileTimestamp;
	uint64 m_nCompileFingerprint;
};

#include <entity2/entityidentity.h> //TODO: declare CEntityIdentity/CEntityInstance as schema classes here.

#endif // WORLD_SCHEMA_H
