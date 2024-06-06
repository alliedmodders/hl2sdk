//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#ifndef BSPFLAGS_H
#define BSPFLAGS_H

#ifdef _WIN32
#pragma once
#endif

#include "const.h"

// contents flags are seperate bits
// a given brush can contribute multiple content bits
// multiple brushes can be in a single leaf

// these definitions also need to be in q_shared.h!

// lower bits are stronger, and will eat weaker brushes completely
#define	CONTENTS_EMPTY						0ull // No contents

#define CONTENTS_SOLID						( 1ull << LAYER_INDEX_CONTENTS_SOLID )
#define CONTENTS_HITBOX						( 1ull << LAYER_INDEX_CONTENTS_HITBOX )
#define CONTENTS_TRIGGER					( 1ull << LAYER_INDEX_CONTENTS_TRIGGER )
#define CONTENTS_SKY						( 1ull << LAYER_INDEX_CONTENTS_SKY )

#define CONTENTS_PLAYER_CLIP				( 1ull << LAYER_INDEX_CONTENTS_PLAYER_CLIP )
#define CONTENTS_NPC_CLIP					( 1ull << LAYER_INDEX_CONTENTS_NPC_CLIP )
#define CONTENTS_BLOCK_LOS					( 1ull << LAYER_INDEX_CONTENTS_BLOCK_LOS )
#define CONTENTS_BLOCK_LIGHT				( 1ull << LAYER_INDEX_CONTENTS_BLOCK_LIGHT )
#define CONTENTS_LADDER						( 1ull << LAYER_INDEX_CONTENTS_LADDER )
#define CONTENTS_PICKUP						( 1ull << LAYER_INDEX_CONTENTS_PICKUP )
#define CONTENTS_BLOCK_SOUND				( 1ull << LAYER_INDEX_CONTENTS_BLOCK_SOUND )
#define CONTENTS_NODRAW						( 1ull << LAYER_INDEX_CONTENTS_NODRAW )
#define CONTENTS_WINDOW						( 1ull << LAYER_INDEX_CONTENTS_WINDOW )
#define CONTENTS_PASS_BULLETS				( 1ull << LAYER_INDEX_CONTENTS_PASS_BULLETS )
#define CONTENTS_WORLD_GEOMETRY				( 1ull << LAYER_INDEX_CONTENTS_WORLD_GEOMETRY )
#define CONTENTS_WATER						( 1ull << LAYER_INDEX_CONTENTS_WATER )
#define CONTENTS_SLIME						( 1ull << LAYER_INDEX_CONTENTS_SLIME )
#define CONTENTS_TOUCH_ALL					( 1ull << LAYER_INDEX_CONTENTS_TOUCH_ALL )
#define CONTENTS_PLAYER						( 1ull << LAYER_INDEX_CONTENTS_PLAYER )
#define CONTENTS_NPC						( 1ull << LAYER_INDEX_CONTENTS_NPC )
#define CONTENTS_DEBRIS						( 1ull << LAYER_INDEX_CONTENTS_DEBRIS )
#define CONTENTS_PHYSICS_PROP				( 1ull << LAYER_INDEX_CONTENTS_PHYSICS_PROP )
#define CONTENTS_NAV_IGNORE					( 1ull << LAYER_INDEX_CONTENTS_NAV_IGNORE )
#define CONTENTS_NAV_LOCAL_IGNORE			( 1ull << LAYER_INDEX_CONTENTS_NAV_LOCAL_IGNORE )
#define CONTENTS_POST_PROCESSING_VOLUME		( 1ull << LAYER_INDEX_CONTENTS_POST_PROCESSING_VOLUME )
#define CONTENTS_UNUSED_LAYER3				( 1ull << LAYER_INDEX_CONTENTS_UNUSED_LAYER3 )
#define CONTENTS_CARRIED_OBJECT				( 1ull << LAYER_INDEX_CONTENTS_CARRIED_OBJECT )
#define CONTENTS_PUSHAWAY					( 1ull << LAYER_INDEX_CONTENTS_PUSHAWAY )
#define CONTENTS_SERVER_ENTITY_ON_CLIENT	( 1ull << LAYER_INDEX_CONTENTS_SERVER_ENTITY_ON_CLIENT )
#define CONTENTS_CARRIED_WEAPON				( 1ull << LAYER_INDEX_CONTENTS_CARRIED_WEAPON )
#define CONTENTS_STATIC_LEVEL				( 1ull << LAYER_INDEX_CONTENTS_STATIC_LEVEL )

#define CONTENTS_CSGO_TEAM1					( 1ull << LAYER_INDEX_CONTENTS_CSGO_TEAM1 )
#define CONTENTS_CSGO_TEAM2					( 1ull << LAYER_INDEX_CONTENTS_CSGO_TEAM2 )
#define CONTENTS_CSGO_GRENADE_CLIP			( 1ull << LAYER_INDEX_CONTENTS_CSGO_GRENADE_CLIP )
#define CONTENTS_CSGO_DRONE_CLIP			( 1ull << LAYER_INDEX_CONTENTS_CSGO_DRONE_CLIP )
#define CONTENTS_CSGO_MOVEABLE				( 1ull << LAYER_INDEX_CONTENTS_CSGO_MOVEABLE )
#define CONTENTS_CSGO_OPAQUE				( 1ull << LAYER_INDEX_CONTENTS_CSGO_OPAQUE )
#define CONTENTS_CSGO_MONSTER				( 1ull << LAYER_INDEX_CONTENTS_CSGO_MONSTER )
#define CONTENTS_CSGO_UNUSED_LAYER			( 1ull << LAYER_INDEX_CONTENTS_CSGO_UNUSED_LAYER )
#define CONTENTS_CSGO_THROWN_GRENADE		( 1ull << LAYER_INDEX_CONTENTS_CSGO_THROWN_GRENADE )



// -----------------------------------------------------
// spatial content masks - used for spatial queries (traceline,etc.)
// -----------------------------------------------------
#define	MASK_ALL					(~0ull)
// everything that is normally solid
#define	MASK_SOLID					(CONTENTS_SOLID|CONTENTS_WINDOW|CONTENTS_PLAYER|CONTENTS_NPC|CONTENTS_PASS_BULLETS)
// everything that blocks player movement
#define	MASK_PLAYERSOLID			(CONTENTS_SOLID|CONTENTS_PLAYER_CLIP|CONTENTS_WINDOW|CONTENTS_PLAYER|CONTENTS_NPC|CONTENTS_PASS_BULLETS)
// blocks npc movement
#define	MASK_NPCSOLID				(CONTENTS_SOLID|CONTENTS_NPC_CLIP|CONTENTS_WINDOW|CONTENTS_PLAYER|CONTENTS_NPC|CONTENTS_PASS_BULLETS)
// blocks fluid movement
#define	MASK_NPCFLUID				(CONTENTS_SOLID|CONTENTS_NPC_CLIP|CONTENTS_WINDOW|CONTENTS_PLAYER|CONTENTS_NPC)
// water physics in these contents
#define	MASK_WATER					(CONTENTS_WATER|CONTENTS_SLIME)
// bullets see these as solid
#define	MASK_SHOT					(CONTENTS_SOLID|CONTENTS_PLAYER|CONTENTS_NPC|CONTENTS_WINDOW|CONTENTS_DEBRIS|CONTENTS_HITBOX)
// bullets see these as solid, except monsters (world+brush only)
#define MASK_SHOT_BRUSHONLY			(CONTENTS_SOLID|CONTENTS_WINDOW|CONTENTS_DEBRIS)
// non-raycasted weapons see this as solid (includes grates)
#define MASK_SHOT_HULL				(CONTENTS_SOLID|CONTENTS_PLAYER|CONTENTS_NPC|CONTENTS_WINDOW|CONTENTS_DEBRIS|CONTENTS_PASS_BULLETS)
// hits solids (not grates) and passes through everything else
#define MASK_SHOT_PORTAL			(CONTENTS_SOLID|CONTENTS_WINDOW|CONTENTS_PLAYER|CONTENTS_NPC)
// everything normally solid, except monsters (world+brush only)
#define MASK_SOLID_BRUSHONLY		(CONTENTS_SOLID|CONTENTS_WINDOW|CONTENTS_PASS_BULLETS)
// everything normally solid for player movement, except monsters (world+brush only)
#define MASK_PLAYERSOLID_BRUSHONLY	(CONTENTS_SOLID|CONTENTS_WINDOW|CONTENTS_PLAYER_CLIP|CONTENTS_PASS_BULLETS)
// everything normally solid for npc movement, except monsters (world+brush only)
#define MASK_NPCSOLID_BRUSHONLY		(CONTENTS_SOLID|CONTENTS_WINDOW|CONTENTS_NPC_CLIP|CONTENTS_PASS_BULLETS)

#endif // BSPFLAGS_H
