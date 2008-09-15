//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#ifndef IMATERIAL_H
#define IMATERIAL_H

#ifdef _WIN32
#pragma once
#endif

#include "bitmap/imageformat.h"
#include "materialsystem/imaterialsystem.h"

//-----------------------------------------------------------------------------
// forward declaraions
//-----------------------------------------------------------------------------

class IMaterialVar;
class ITexture;
class IMaterialProxy;
class Vector;

//-----------------------------------------------------------------------------
// Flags for GetVertexFormat
// NOTE: If you change these, change them in playback.cpp as well!!!!
//-----------------------------------------------------------------------------
enum VertexFormatFlags_t
{
	VERTEX_POSITION	= 0x0001,
	VERTEX_NORMAL	= 0x0002,
	VERTEX_COLOR	= 0x0004,
	VERTEX_SPECULAR = 0x0008,

	VERTEX_TANGENT_S	= 0x0010,
	VERTEX_TANGENT_T	= 0x0020,
	VERTEX_TANGENT_SXT	= 0x0040,

	VERTEX_TANGENT_SPACE= VERTEX_TANGENT_S | VERTEX_TANGENT_T | VERTEX_TANGENT_SXT,

	// Indicates we're using bone indices
	VERTEX_BONE_INDEX = 0x0080,

	// Indicates this is a vertex shader
	VERTEX_FORMAT_VERTEX_SHADER = 0x0100,

	// Indicates this format shouldn't be bloated to cache align it
	// (only used for VertexUsage)
	VERTEX_FORMAT_USE_EXACT_FORMAT = 0x0200,

	// Update this if you add or remove bits...
	VERTEX_LAST_BIT = 9,

	VERTEX_BONE_WEIGHT_BIT = VERTEX_LAST_BIT + 1,
	USER_DATA_SIZE_BIT = VERTEX_LAST_BIT + 4,
	NUM_TEX_COORD_BIT = VERTEX_LAST_BIT + 7,
	TEX_COORD_SIZE_BIT = VERTEX_LAST_BIT + 10,

	VERTEX_BONE_WEIGHT_MASK = ( 0x7 << VERTEX_BONE_WEIGHT_BIT ),
	USER_DATA_SIZE_MASK = ( 0x7 << USER_DATA_SIZE_BIT ),
	NUM_TEX_COORD_MASK = ( 0x7 << NUM_TEX_COORD_BIT ),
	TEX_COORD_SIZE_MASK = ( 0x7 << TEX_COORD_SIZE_BIT ),
};


//-----------------------------------------------------------------------------
// Macros for construction..
//-----------------------------------------------------------------------------
#define VERTEX_BONEWEIGHT( _n )				((_n) << VERTEX_BONE_WEIGHT_BIT)
#define VERTEX_USERDATA_SIZE( _n )			((_n) << USER_DATA_SIZE_BIT)
#define VERTEX_NUM_TEXCOORDS( _n )			((_n) << NUM_TEX_COORD_BIT)
#define VERTEX_TEXCOORD_SIZE( _stage, _n )	((_n) << (TEX_COORD_SIZE_BIT + (3*_stage)))


//-----------------------------------------------------------------------------
// The vertex format type
//-----------------------------------------------------------------------------
typedef unsigned int VertexFormat_t;


//-----------------------------------------------------------------------------
// Gets at various vertex format info...
//-----------------------------------------------------------------------------
inline int VertexFlags( VertexFormat_t vertexFormat )
{
	return vertexFormat & ( (1 << (VERTEX_LAST_BIT+1)) - 1 );
}

inline int NumBoneWeights( VertexFormat_t vertexFormat )
{
	return (vertexFormat >> VERTEX_BONE_WEIGHT_BIT) & 0x7;
}

inline int NumTextureCoordinates( VertexFormat_t vertexFormat )
{
	return (vertexFormat >> NUM_TEX_COORD_BIT) & 0x7;
}

inline int UserDataSize( VertexFormat_t vertexFormat )
{
	return (vertexFormat >> USER_DATA_SIZE_BIT) & 0x7;
}

inline int TexCoordSize( int stage, VertexFormat_t vertexFormat )
{
	return (vertexFormat >> (TEX_COORD_SIZE_BIT + 3*stage) ) & 0x7;
}

inline bool UsesVertexShader( VertexFormat_t vertexFormat )
{
	return (vertexFormat & VERTEX_FORMAT_VERTEX_SHADER) != 0;
}


//-----------------------------------------------------------------------------
// Enumeration for the various fields capable of being morphed
//-----------------------------------------------------------------------------
enum MorphFormatFlags_t
{
	MORPH_POSITION	= 0x0001,	// 3D
	MORPH_NORMAL	= 0x0002,	// 3D
	MORPH_WRINKLE	= 0x0004,	// 1D
};


//-----------------------------------------------------------------------------
// The morph format type
//-----------------------------------------------------------------------------
typedef unsigned int MorphFormat_t;


//-----------------------------------------------------------------------------
// shader state flags, can be read from the FLAGS materialvar.
// Also can be read or written to with the Set/GetMaterialVarFlags() call
// Also make sure you add/remove a string associated
// with each flag below to CShaderSystem::ShaderStateString in ShaderSystem.cpp
//-----------------------------------------------------------------------------
enum MaterialVarFlags_t
{
	MATERIAL_VAR_DEBUG					  = (1 << 0),
	MATERIAL_VAR_NO_DEBUG_OVERRIDE		  = (1 << 1),
	MATERIAL_VAR_NO_DRAW				  = (1 << 2),
	MATERIAL_VAR_USE_IN_FILLRATE_MODE	  = (1 << 3),

	MATERIAL_VAR_VERTEXCOLOR			  = (1 << 4),
	MATERIAL_VAR_VERTEXALPHA			  = (1 << 5),
	MATERIAL_VAR_SELFILLUM				  = (1 << 6),
	MATERIAL_VAR_ADDITIVE				  = (1 << 7),
	MATERIAL_VAR_ALPHATEST				  = (1 << 8),
	MATERIAL_VAR_MULTIPASS				  = (1 << 9),
	MATERIAL_VAR_ZNEARER				  = (1 << 10),
	MATERIAL_VAR_MODEL					  = (1 << 11),
	MATERIAL_VAR_FLAT					  = (1 << 12),
	MATERIAL_VAR_NOCULL					  = (1 << 13),
	MATERIAL_VAR_NOFOG					  = (1 << 14),
	MATERIAL_VAR_IGNOREZ				  = (1 << 15),
	MATERIAL_VAR_DECAL					  = (1 << 16),
	MATERIAL_VAR_ENVMAPSPHERE			  = (1 << 17),
	MATERIAL_VAR_NOALPHAMOD				  = (1 << 18),
	MATERIAL_VAR_ENVMAPCAMERASPACE	      = (1 << 19),
	MATERIAL_VAR_BASEALPHAENVMAPMASK	  = (1 << 20),
	MATERIAL_VAR_TRANSLUCENT              = (1 << 21),
	MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK = (1 << 22),
	MATERIAL_VAR_NEEDS_SOFTWARE_SKINNING  = (1 << 23),
	MATERIAL_VAR_OPAQUETEXTURE			  = (1 << 24),
	MATERIAL_VAR_ENVMAPMODE				  = (1 << 25),
	MATERIAL_VAR_SUPPRESS_DECALS		  = (1 << 26),
	MATERIAL_VAR_HALFLAMBERT			  = (1 << 27),
	MATERIAL_VAR_WIREFRAME                = (1 << 28),

	// NOTE: Only add flags here that either should be read from
	// .vmts or can be set directly from client code. Other, internal
	// flags should to into the flag enum in IMaterialInternal.h
};


//-----------------------------------------------------------------------------
// Internal flags not accessible from outside the material system. Stored in Flags2
//-----------------------------------------------------------------------------
enum MaterialVarFlags2_t
{
	// NOTE: These are for $flags2!!!!!
//	UNUSED											= (1 << 0),

	MATERIAL_VAR2_LIGHTING_UNLIT					= 0,
	MATERIAL_VAR2_LIGHTING_VERTEX_LIT				= (1 << 1),
	MATERIAL_VAR2_LIGHTING_LIGHTMAP					= (1 << 2),
	MATERIAL_VAR2_LIGHTING_BUMPED_LIGHTMAP			= (1 << 3),
	MATERIAL_VAR2_LIGHTING_MASK						= 
		( MATERIAL_VAR2_LIGHTING_VERTEX_LIT | 
		  MATERIAL_VAR2_LIGHTING_LIGHTMAP | 
		  MATERIAL_VAR2_LIGHTING_BUMPED_LIGHTMAP ),

	// FIXME: Should this be a part of the above lighting enums?
	MATERIAL_VAR2_DIFFUSE_BUMPMAPPED_MODEL			= (1 << 4),
	MATERIAL_VAR2_USES_ENV_CUBEMAP					= (1 << 5),
	MATERIAL_VAR2_NEEDS_TANGENT_SPACES				= (1 << 6),
	MATERIAL_VAR2_NEEDS_SOFTWARE_LIGHTING			= (1 << 7),
	// GR - HDR path puts lightmap alpha in separate texture...
	MATERIAL_VAR2_BLEND_WITH_LIGHTMAP_ALPHA			= (1 << 8),
	MATERIAL_VAR2_NEEDS_BAKED_LIGHTING_SNAPSHOTS	= (1 << 9),
	MATERIAL_VAR2_USE_FLASHLIGHT					= (1 << 10),
	MATERIAL_VAR2_USE_FIXED_FUNCTION_BAKED_LIGHTING = (1 << 11),
	MATERIAL_VAR2_NEEDS_FIXED_FUNCTION_FLASHLIGHT	= (1 << 12),
	MATERIAL_VAR2_USE_EDITOR						= (1 << 13),
};


//-----------------------------------------------------------------------------
// Preview image return values
//-----------------------------------------------------------------------------
enum PreviewImageRetVal_t
{
	MATERIAL_PREVIEW_IMAGE_BAD = 0,
	MATERIAL_PREVIEW_IMAGE_OK,
	MATERIAL_NO_PREVIEW_IMAGE,
};


//-----------------------------------------------------------------------------
// material interface
//-----------------------------------------------------------------------------
abstract_class IMaterial
{
public:
	// Get the name of the material.  This is a full path to 
	// the vmt file starting from "hl2/materials" (or equivalent) without
	// a file extension.
	virtual const char *	GetName() const = 0;
	virtual const char *	GetTextureGroupName() const = 0;

#ifndef _XBOX
	// Get the preferred size/bitDepth of a preview image of a material.
	// This is the sort of image that you would use for a thumbnail view
	// of a material, or in WorldCraft until it uses materials to render.
	// separate this for the tools maybe
	virtual PreviewImageRetVal_t GetPreviewImageProperties( int *width, int *height, 
				 			ImageFormat *imageFormat, bool* isTranslucent ) const = 0;
	
	// Get a preview image at the specified width/height and bitDepth.
	// Will do resampling if necessary.(not yet!!! :) )
	// Will do color format conversion. (works now.)
	virtual PreviewImageRetVal_t GetPreviewImage( unsigned char *data, 
												 int width, int height,
												 ImageFormat imageFormat ) const = 0;
#endif											    
	// 
	virtual int				GetMappingWidth( ) = 0;
	virtual int				GetMappingHeight( ) = 0;

	virtual int				GetNumAnimationFrames( ) = 0;

	// For material subrects (material pages).  Offset(u,v) and scale(u,v) are normalized to texture.
	virtual bool			InMaterialPage( void ) = 0;
	virtual	void			GetMaterialOffset( float *pOffset ) = 0;
	virtual void			GetMaterialScale( float *pScale ) = 0;
	virtual IMaterial		*GetMaterialPage( void ) = 0;

	// find a vmt variable.
	// This is how game code affects how a material is rendered.
	// The game code must know about the params that are used by
	// the shader for the material that it is trying to affect.
	virtual IMaterialVar *	FindVar( const char *varName, bool *found, bool complain = true ) = 0;

	// The user never allocates or deallocates materials.  Reference counting is
	// used instead.  Garbage collection is done upon a call to 
	// IMaterialSystem::UncacheUnusedMaterials.
	virtual void			IncrementReferenceCount( void ) = 0;
	virtual void			DecrementReferenceCount( void ) = 0;

	// Each material is assigned a number that groups it with like materials
	// for sorting in the application.
	virtual int 			GetEnumerationID( void ) const = 0;

	virtual void			GetLowResColorSample( float s, float t, float *color ) const = 0;

	// This computes the state snapshots for this material
	virtual void			RecomputeStateSnapshots() = 0;

	// Are we translucent?
	virtual bool			IsTranslucent() = 0;

	// Are we alphatested?
	virtual bool			IsAlphaTested() = 0;

	// Are we vertex lit?
	virtual bool			IsVertexLit() = 0;

	// Gets the vertex format
	virtual VertexFormat_t	GetVertexFormat() const = 0;

	// If true, this material will pre-process the data and cannot be rendered
	// on static meshes.
	virtual bool			WillPreprocessData( void ) const = 0;

	// returns true if this material uses a material proxy
	virtual bool			HasProxy( void ) const = 0;
#ifndef _XBOX
	// returns true if there is a software vertex shader.  Software vertex shaders are
	// different from "WillPreprocessData" in that all the work is done on the verts
	// before calling the shader instead of between passes.  Software vertex shaders are
	// preferred when possible.
	virtual bool			UsesSoftwareVertexShader( void ) const = 0;
#endif
	virtual bool			UsesEnvCubemap( void ) = 0;

	virtual bool			NeedsTangentSpace( void ) = 0;

	virtual bool			NeedsPowerOfTwoFrameBufferTexture( void ) = 0;
	virtual bool			NeedsFullFrameBufferTexture( void ) = 0;
	
	// returns true if the shader doesn't do skinning itself and requires
	// the data that is sent to it to be preskinned.
	virtual bool			NeedsSoftwareSkinning( void ) = 0;
	
	// Apply constant color or alpha modulation
	virtual void			AlphaModulate( float alpha ) = 0;
	virtual void			ColorModulate( float r, float g, float b ) = 0;

	// Material Var flags...
	virtual void			SetMaterialVarFlag( MaterialVarFlags_t flag, bool on ) = 0;
	virtual bool			GetMaterialVarFlag( MaterialVarFlags_t flag ) const = 0;

	// Gets material reflectivity
	virtual void			GetReflectivity( Vector& reflect ) = 0;

	// Gets material property flags
	virtual bool			GetPropertyFlag( MaterialPropertyTypes_t type ) = 0;

	// Is the material visible from both sides?
	virtual bool			IsTwoSided() = 0;

	// Sets the shader associated with the material
	virtual void			SetShader( const char *pShaderName ) = 0;

	// Can't be const because the material might have to precache itself.
	virtual int				GetNumPasses( void ) = 0; 

	// Can't be const because the material might have to precache itself.
	virtual int				GetTextureMemoryBytes( void ) = 0; 

	// Meant to be used with materials created using CreateMaterial
	// It updates the materials to reflect the current values stored in the material vars
	virtual void			Refresh() = 0;

	// GR - returns true is material uses lightmap alpha for blending
	virtual bool			NeedsLightmapBlendAlpha( void ) = 0;

	// returns true if the shader doesn't do lighting itself and requires
	// the data that is sent to it to be prelighted
	virtual bool			NeedsSoftwareLighting( void ) = 0;

	// Gets at the shader parameters
	virtual int				ShaderParamCount() const = 0;
	virtual IMaterialVar	**GetShaderParams( void ) = 0;

	// Returns true if this is the error material you get back from IMaterialSystem::FindMaterial if
	// the material can't be found.
	virtual bool			IsErrorMaterial() const = 0;

	virtual void			SetUseFixedFunctionBakedLighting( bool bEnable ) = 0;

	// Gets the current alpha modulation
	virtual float			GetAlphaModulation() = 0;
	virtual void			GetColorModulation( float *r, float *g, float *b ) = 0;
#ifndef _XBOX
	// Gets the morph format
	virtual MorphFormat_t	GetMorphFormat() const = 0;
#endif
	
	// fast find that stores the index of the found var in the string table in local cache
	virtual IMaterialVar *	FindVarFast( char const *pVarName, unsigned int *pToken ) = 0;

	// Sets new VMT shader parameters for the material
	virtual void			SetShaderAndParams( KeyValues *pKeyValues ) = 0;
	virtual const char *	GetShaderName() const = 0;

	virtual void			DeleteIfUnreferenced() = 0;
};


inline bool IsErrorMaterial( IMaterial *pMat )
{
	return !pMat || pMat->IsErrorMaterial();
}


#endif // IMATERIAL_H
