//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#ifndef IMATERIALSYSTEM_H
#define IMATERIALSYSTEM_H

#ifdef _WIN32
#pragma once
#endif

#define OVERBRIGHT 2.0f
#define OO_OVERBRIGHT ( 1.0f / 2.0f )
#define GAMMA 2.2f
#define TEXGAMMA 2.2f

#include "tier1/interface.h"
#include "vector.h"
#include "vector4d.h"
#include "vmatrix.h"
#include "appframework/IAppSystem.h"
#include "bitmap/imageformat.h"
#include "texture_group_names.h"
#include "vtf/vtf.h"


//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
class IMaterial;
class IMesh;
struct MaterialSystem_Config_t;
class VMatrix;
struct matrix3x4_t;
class ITexture;
struct MaterialSystemHardwareIdentifier_t;
class KeyValues;
class IShader;
class IVertexTexture;
class IMorph;


//-----------------------------------------------------------------------------
// important enumeration
//-----------------------------------------------------------------------------

// NOTE NOTE NOTE!!!!  If you up this, grep for "NEW_INTERFACE" to see if there is anything
// waiting to be enabled during an interface revision.
#define MATERIAL_SYSTEM_INTERFACE_VERSION "VMaterialSystem076"

enum ShaderParamType_t 
{ 
	SHADER_PARAM_TYPE_TEXTURE, 
	SHADER_PARAM_TYPE_INTEGER,
	SHADER_PARAM_TYPE_COLOR,
	SHADER_PARAM_TYPE_VEC2,
	SHADER_PARAM_TYPE_VEC3,
	SHADER_PARAM_TYPE_VEC4,
	SHADER_PARAM_TYPE_ENVMAP,	// obsolete
	SHADER_PARAM_TYPE_FLOAT,
	SHADER_PARAM_TYPE_BOOL,
	SHADER_PARAM_TYPE_FOURCC,
	SHADER_PARAM_TYPE_MATRIX,
	SHADER_PARAM_TYPE_MATERIAL,
	SHADER_PARAM_TYPE_STRING,
};

enum MaterialMatrixMode_t
{
	MATERIAL_VIEW = 0,
	MATERIAL_PROJECTION,

	// Texture matrices
	MATERIAL_TEXTURE0,
	MATERIAL_TEXTURE1,
	MATERIAL_TEXTURE2,
	MATERIAL_TEXTURE3,
#ifndef _XBOX
	MATERIAL_TEXTURE4,
	MATERIAL_TEXTURE5,
	MATERIAL_TEXTURE6,
	MATERIAL_TEXTURE7,
#endif

	MATERIAL_MODEL,

	// Total number of matrices
	NUM_MATRIX_MODES = MATERIAL_MODEL+1,

	// Number of texture transforms
#ifndef _XBOX
	NUM_TEXTURE_TRANSFORMS = MATERIAL_TEXTURE7 - MATERIAL_TEXTURE0 + 1
#else
	NUM_TEXTURE_TRANSFORMS = MATERIAL_TEXTURE3 - MATERIAL_TEXTURE0 + 1
#endif
};

// FIXME: How do I specify the actual number of matrix modes?
#ifndef _XBOX
const int NUM_MODEL_TRANSFORMS = 53;
#else
// xboxissue - minimum number based on bones
const int NUM_MODEL_TRANSFORMS = 47;
#endif
const int MATERIAL_MODEL_MAX = MATERIAL_MODEL + NUM_MODEL_TRANSFORMS;

enum MaterialPrimitiveType_t 
{ 
	MATERIAL_POINTS			= 0x0,
	MATERIAL_LINES,
	MATERIAL_TRIANGLES,
	MATERIAL_TRIANGLE_STRIP,
	MATERIAL_LINE_STRIP,
	MATERIAL_LINE_LOOP,	// a single line loop
	MATERIAL_POLYGON,	// this is a *single* polygon
	MATERIAL_QUADS,

	// This is used for static meshes that contain multiple types of
	// primitive types.	When calling draw, you'll need to specify
	// a primitive type.
	MATERIAL_HETEROGENOUS
};

enum MaterialPropertyTypes_t
{
	MATERIAL_PROPERTY_NEEDS_LIGHTMAP = 0,					// bool
	MATERIAL_PROPERTY_OPACITY,								// int (enum MaterialPropertyOpacityTypes_t)
	MATERIAL_PROPERTY_REFLECTIVITY,							// vec3_t
	MATERIAL_PROPERTY_NEEDS_BUMPED_LIGHTMAPS				// bool
};

// acceptable property values for MATERIAL_PROPERTY_OPACITY
enum MaterialPropertyOpacityTypes_t
{
	MATERIAL_ALPHATEST = 0,
	MATERIAL_OPAQUE,
	MATERIAL_TRANSLUCENT
};

enum MaterialBufferTypes_t
{
	MATERIAL_FRONT = 0,
	MATERIAL_BACK
};

enum MaterialCullMode_t
{
	MATERIAL_CULLMODE_CCW,	// this culls polygons with counterclockwise winding
	MATERIAL_CULLMODE_CW	// this culls polygons with clockwise winding
};

enum MaterialVertexFormat_t
{
	MATERIAL_VERTEX_FORMAT_MODEL,
	MATERIAL_VERTEX_FORMAT_COLOR,
};

enum MaterialFogMode_t
{
	MATERIAL_FOG_NONE,
	MATERIAL_FOG_LINEAR,
	MATERIAL_FOG_LINEAR_BELOW_FOG_Z,
};

enum MaterialHeightClipMode_t
{
	MATERIAL_HEIGHTCLIPMODE_DISABLE,
	MATERIAL_HEIGHTCLIPMODE_RENDER_ABOVE_HEIGHT,
	MATERIAL_HEIGHTCLIPMODE_RENDER_BELOW_HEIGHT
};


//-----------------------------------------------------------------------------
// Vertex texture stage identifiers
//-----------------------------------------------------------------------------
enum VertexTextureStage_t
{
	MATERIAL_VERTEXTEXTURE_STAGE0 = 0,
	MATERIAL_VERTEXTEXTURE_STAGE1,
	MATERIAL_VERTEXTEXTURE_STAGE2,
	MATERIAL_VERTEXTEXTURE_STAGE3,
};


//-----------------------------------------------------------------------------
// Light structure
//-----------------------------------------------------------------------------
#include "mathlib/lightdesc.h"

#if 0
enum LightType_t
{
	MATERIAL_LIGHT_DISABLE = 0,
	MATERIAL_LIGHT_POINT,
	MATERIAL_LIGHT_DIRECTIONAL,
	MATERIAL_LIGHT_SPOT,
};

enum LightType_OptimizationFlags_t
{
	LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION0 = 1,
	LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION1 = 2,
	LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION2 = 4,
};


struct LightDesc_t 
{
    LightType_t		m_Type;
	Vector			m_Color;
    Vector	m_Position;
    Vector  m_Direction;
    float   m_Range;
    float   m_Falloff;
    float   m_Attenuation0;
    float   m_Attenuation1;
    float   m_Attenuation2;
    float   m_Theta;
    float   m_Phi;
	// These aren't used by DX8. . used for software lighting.
	float	m_ThetaDot;
	float	m_PhiDot;
	unsigned int	m_Flags;


	LightDesc_t() {}

private:
	// No copy constructors allowed
	LightDesc_t(const LightDesc_t& vOther);
};
#endif

#define CREATERENDERTARGETFLAGS_HDR				1
#define CREATERENDERTARGETFLAGS_AUTOMIPMAP		2
#define CREATERENDERTARGETFLAGS_UNFILTERABLE_OK 4


//-----------------------------------------------------------------------------
// allowed stencil operations. These match the d3d operations
//-----------------------------------------------------------------------------
enum StencilOperation_t 
{
#ifndef _XBOX
    STENCILOPERATION_KEEP = 1,
    STENCILOPERATION_ZERO = 2,
    STENCILOPERATION_REPLACE = 3,
    STENCILOPERATION_INCRSAT = 4,
    STENCILOPERATION_DECRSAT = 5,
    STENCILOPERATION_INVERT = 6,
    STENCILOPERATION_INCR = 7,
    STENCILOPERATION_DECR = 8,
#else
    STENCILOPERATION_KEEP = 0x1e00,
    STENCILOPERATION_ZERO = 0,
    STENCILOPERATION_REPLACE = 0x1e01,
    STENCILOPERATION_INCRSAT = 0x1e02,
    STENCILOPERATION_DECRSAT = 0x1e03,
    STENCILOPERATION_INVERT = 0x150a,
    STENCILOPERATION_INCR = 0x8507,
    STENCILOPERATION_DECR = 0x8508,
#endif
    STENCILOPERATION_FORCE_DWORD = 0x7fffffff
};

enum StencilComparisonFunction_t 
{
#ifndef _XBOX
    STENCILCOMPARISONFUNCTION_NEVER = 1,
    STENCILCOMPARISONFUNCTION_LESS = 2,
    STENCILCOMPARISONFUNCTION_EQUAL = 3,
    STENCILCOMPARISONFUNCTION_LESSEQUAL = 4,
    STENCILCOMPARISONFUNCTION_GREATER = 5,
    STENCILCOMPARISONFUNCTION_NOTEQUAL = 6,
    STENCILCOMPARISONFUNCTION_GREATEREQUAL = 7,
    STENCILCOMPARISONFUNCTION_ALWAYS = 8,
#else
    STENCILCOMPARISONFUNCTION_NEVER = 0x200,
    STENCILCOMPARISONFUNCTION_LESS = 0x201,
    STENCILCOMPARISONFUNCTION_EQUAL = 0x202,
    STENCILCOMPARISONFUNCTION_LESSEQUAL = 0x203,
    STENCILCOMPARISONFUNCTION_GREATER = 0x204,
    STENCILCOMPARISONFUNCTION_NOTEQUAL = 0x205,
    STENCILCOMPARISONFUNCTION_GREATEREQUAL = 0x206,
    STENCILCOMPARISONFUNCTION_ALWAYS = 0x207,
#endif

    STENCILCOMPARISONFUNCTION_FORCE_DWORD = 0x7fffffff
};


//-----------------------------------------------------------------------------
// Standard lightmaps
//-----------------------------------------------------------------------------
enum StandardLightmap_t
{
	MATERIAL_SYSTEM_LIGHTMAP_PAGE_WHITE = -1,
	MATERIAL_SYSTEM_LIGHTMAP_PAGE_WHITE_BUMP = -2,
	MATERIAL_SYSTEM_LIGHTMAP_PAGE_USER_DEFINED = -3
};


struct MaterialSystem_SortInfo_t
{
	IMaterial	*material;
#ifndef _XBOX
	int			lightmapPageID;
#else
	short		lightmapPageID;
#endif
};


#define MAX_FB_TEXTURES 4

//-----------------------------------------------------------------------------
// Information about each adapter
//-----------------------------------------------------------------------------
enum
{
	MATERIAL_ADAPTER_NAME_LENGTH = 512
};

struct MaterialAdapterInfo_t
{
	char m_pDriverName[MATERIAL_ADAPTER_NAME_LENGTH];
	char m_pDriverDescription[MATERIAL_ADAPTER_NAME_LENGTH];
};

struct Material3DDriverInfo_t
{
	char m_pDriverName[MATERIAL_ADAPTER_NAME_LENGTH];
	char m_pDriverDescription[MATERIAL_ADAPTER_NAME_LENGTH];
	char m_pDriverVersion[MATERIAL_ADAPTER_NAME_LENGTH];
	unsigned int m_VendorID;
	unsigned int m_DeviceID;
	unsigned int m_SubSysID;
	unsigned int m_Revision;
	unsigned int m_WHQLLevel;

};

//-----------------------------------------------------------------------------
// Video mode info..
//-----------------------------------------------------------------------------
struct MaterialVideoMode_t
{
	int m_Width;			// if width and height are 0 and you select 
	int m_Height;			// windowed mode, it'll use the window size
	ImageFormat m_Format;	// use ImageFormats (ignored for windowed mode)
	int m_RefreshRate;		// 0 == default (ignored for windowed mode)
};

// fixme: should move this into something else.
struct FlashlightState_t
{
	Vector m_vecLightDirection; // FLASHLIGHTFIXME: can get this from the matrix
	Vector m_vecLightOrigin;  // FLASHLIGHTFIXME: can get this from the matrix
	float m_NearZ;
	float m_FarZ;
	float m_fHorizontalFOVDegrees;
	float m_fVerticalFOVDegrees;
	float m_fQuadraticAtten;
	float m_fLinearAtten;
	float m_fConstantAtten;
	Vector m_Color;
	ITexture *m_pSpotlightTexture;
	int m_nSpotlightTextureFrame;
	bool  m_bEnableShadows;
};

//-----------------------------------------------------------------------------
// Flags to be used with the Init call
//-----------------------------------------------------------------------------
enum MaterialInitFlags_t
{
	MATERIAL_INIT_ALLOCATE_FULLSCREEN_TEXTURE = 0x2,
	MATERIAL_INIT_REFERENCE_RASTERIZER	= 0x4,
};


//-----------------------------------------------------------------------------
// Flags to specify type of depth buffer used with RT
//-----------------------------------------------------------------------------

// GR - this is to add RT with no depth buffer bound

enum MaterialRenderTargetDepth_t
{
	MATERIAL_RT_DEPTH_SHARED   = 0x0,
	MATERIAL_RT_DEPTH_SEPARATE = 0x1,
	MATERIAL_RT_DEPTH_NONE     = 0x2,
	MATERIAL_RT_DEPTH_ONLY	   = 0x3,
};

//-----------------------------------------------------------------------------
// A function to be called when we need to release all vertex buffers
// NOTE: The restore function will tell the caller if all the vertex formats
// changed so that it can flush caches, etc. if it needs to (for dxlevel support)
//-----------------------------------------------------------------------------
enum RestoreChangeFlags_t
{
	MATERIAL_RESTORE_VERTEX_FORMAT_CHANGED = 0x1,
};


// NOTE: All size modes will force the render target to be smaller than or equal to
// the size of the framebuffer.
enum RenderTargetSizeMode_t
{
	RT_SIZE_NO_CHANGE=0,			// Only allowed for render targets that don't want a depth buffer
									// (because if they have a depth buffer, the render target must be less than or equal to the size of the framebuffer).
	RT_SIZE_DEFAULT=1,				// Don't play with the specified width and height other than making sure it fits in the framebuffer.
	RT_SIZE_PICMIP=2,				// Apply picmip to the render target's width and height.
	RT_SIZE_HDR=3,					// frame_buffer_width / 4
	RT_SIZE_FULL_FRAME_BUFFER=4,	// Same size as frame buffer, or next lower power of 2 if we can't do that.
	RT_SIZE_OFFSCREEN=5,			// Target of specified size, don't mess with dimensions
	RT_SIZE_FULL_FRAME_BUFFER_ROUNDED_UP=6 // Same size as the frame buffer, rounded up if necessary for systems that can't do non-power of two textures.

	
};


typedef void (*MaterialBufferReleaseFunc_t)( );
typedef void (*MaterialBufferRestoreFunc_t)( int nChangeFlags );	// see RestoreChangeFlags_t
typedef void (*ModeChangeCallbackFunc_t)( void );

typedef int VertexBufferHandle_t;
typedef unsigned short MaterialHandle_t;

typedef unsigned short OcclusionQueryObjectHandle_t;
#define INVALID_OCCLUSION_QUERY_OBJECT_HANDLE ( ( OcclusionQueryObjectHandle_t )~( ( OcclusionQueryObjectHandle_t )0 ) )

class IMaterialProxyFactory;
class ITexture;
class IMaterialSystemHardwareConfig;

abstract_class IMaterialSystem : public IAppSystem
{
public:
	// Call this to set an explicit shader version to use 
	// Must be called before Init().
	virtual void				SetShaderAPI( char const *pShaderAPIDLL ) = 0;

	// Must be called before Init(), if you're going to call it at all...
	virtual void				SetAdapter( int nAdapter, int nFlags ) = 0;

	// Call this to initialize the material system
	// returns a method to create interfaces in the shader dll
	virtual CreateInterfaceFn	Init( char const* pShaderAPIDLL, 
									  IMaterialProxyFactory *pMaterialProxyFactory,
									  CreateInterfaceFn fileSystemFactory,
									  CreateInterfaceFn cvarFactory=NULL ) = 0;

//	virtual void				Shutdown( ) = 0;

	virtual IMaterialSystemHardwareConfig *GetHardwareConfig( const char *pVersion, int *returnCode ) = 0;

	// Gets the number of adapters...
	virtual int					GetDisplayAdapterCount() const = 0;

	// Returns info about each adapter
	virtual void				GetDisplayAdapterInfo( int adapter, MaterialAdapterInfo_t& info ) const = 0;

	// Returns the number of modes
	virtual int					GetModeCount( int adapter ) const = 0;

	// Returns mode information..
	virtual void				GetModeInfo( int adapter, int mode, MaterialVideoMode_t& info ) const = 0;

	// Returns the mode info for the current display device
	virtual void				GetDisplayMode( MaterialVideoMode_t& mode ) const = 0;

	// Sets the mode...
	virtual bool				SetMode( void* hwnd, const MaterialSystem_Config_t &config ) = 0;
		
	// Get video card identitier
	virtual const MaterialSystemHardwareIdentifier_t &GetVideoCardIdentifier( void ) const = 0;
	
	// Creates/ destroys a child window
	virtual bool				AddView( void* hwnd ) = 0;
	virtual void				RemoveView( void* hwnd ) = 0;

	// Sets the view
	virtual void				SetView( void* hwnd ) = 0;

	virtual void				Get3DDriverInfo( Material3DDriverInfo_t& info ) const = 0;
	
	// return true if lightmaps need to be redownloaded
	// Call this before rendering each frame with the current config
	// for the material system.
	// Will do whatever is necessary to get the material system into the correct state
	// upon configuration change. .doesn't much else otherwise.
	virtual bool				UpdateConfig( bool bForceUpdate ) = 0;

	// Get the current config for this video card (as last set by UpdateConfig)
	virtual const MaterialSystem_Config_t &GetCurrentConfigForVideoCard() const = 0;

	// Force this to be the config; update all material system convars to match the state
	// return true if lightmaps need to be redownloaded
	virtual bool				OverrideConfig( const MaterialSystem_Config_t &config, bool bForceUpdate ) = 0;

	// This is the interface for knowing what materials are available
	// is to use the following functions to get a list of materials.  The
	// material names will have the full path to the material, and that is the 
	// only way that the directory structure of the materials will be seen through this
	// interface.
	// NOTE:  This is mostly for worldcraft to get a list of materials to put
	// in the "texture" browser.in Worldcraft
	virtual MaterialHandle_t	FirstMaterial() const = 0;

	// returns InvalidMaterial if there isn't another material.
	// WARNING: you must call GetNextMaterial until it returns NULL, 
	// otherwise there will be a memory leak.
	virtual MaterialHandle_t	NextMaterial( MaterialHandle_t h ) const = 0;

	// This is the invalid material
	virtual MaterialHandle_t	InvalidMaterial() const = 0;

	// Returns a particular material
	virtual IMaterial*			GetMaterial( MaterialHandle_t h ) const = 0;

	// Find a material by name.
	// The name of a material is a full path to 
	// the vmt file starting from "hl2/materials" (or equivalent) without
	// a file extension.
	// eg. "dev/dev_bumptest" refers to somethign similar to:
	// "d:/hl2/hl2/materials/dev/dev_bumptest.vmt"
	//
	// Most of the texture groups for pTextureGroupName are listed in texture_group_names.h.
	// 
	// Note: if the material can't be found, this returns a checkerboard material. You can 
	// find out if you have that material by calling IMaterial::IsErrorMaterial().
	// (Or use the global IsErrorMaterial function, which checks if it's null too).
	virtual IMaterial *			FindMaterial( char const* pMaterialName, const char *pTextureGroupName, bool complain = true, const char *pComplainPrefix = NULL ) = 0;

	virtual ITexture *			FindTexture( char const* pTextureName, const char *pTextureGroupName, bool complain = true ) = 0;
	virtual void				BindLocalCubemap( ITexture *pTexture ) = 0;
	
	// pass in an ITexture (that is build with "rendertarget" "1") or
	// pass in NULL for the regular backbuffer.
	virtual void				SetRenderTarget( ITexture *pTexture ) = 0;
	virtual ITexture *			GetRenderTarget( void ) = 0;
	
	virtual void				GetRenderTargetDimensions( int &width, int &height) const = 0;
	virtual void				GetBackBufferDimensions( int &width, int &height) const = 0;
	
	// Get the total number of materials in the system.  These aren't just the used
	// materials, but the complete collection.
	virtual int					GetNumMaterials( ) const = 0;

	// Remove any materials from memory that aren't in use as determined
	// by the IMaterial's reference count.
	virtual void				UncacheUnusedMaterials( bool bRecomputeStateSnapshots = false ) = 0;

	// uncache all materials. .  good for forcing reload of materials.
	virtual void				UncacheAllMaterials( ) = 0;

	// Load any materials into memory that are to be used as determined
	// by the IMaterial's reference count.
	virtual void				CacheUsedMaterials( ) = 0;
	
	// Force all textures to be reloaded from disk.
	virtual void				ReloadTextures( ) = 0;
	
#ifdef _XBOX
	// Is the texture cache performing I/O?
	virtual bool				IsTextureCacheLoading( void ) = 0;
	// Free tagged resources
	virtual void				PurgeTaggedResources( int tag ) = 0;
#endif

	//
	// lightmap allocation stuff
	//

	// To allocate lightmaps, sort the whole world by material twice.
	// The first time through, call AllocateLightmap for every surface.
	// that has a lightmap.
	// The second time through, call AllocateWhiteLightmap for every 
	// surface that expects to use shaders that expect lightmaps.
	virtual void				BeginLightmapAllocation( ) = 0;
	// returns the sorting id for this surface
	virtual int 				AllocateLightmap( int width, int height, 
		                                          int offsetIntoLightmapPage[2],
												  IMaterial *pMaterial ) = 0;
	// returns the sorting id for this surface
	virtual int					AllocateWhiteLightmap( IMaterial *pMaterial ) = 0;
	virtual void				EndLightmapAllocation( ) = 0;

	// lightmaps are in linear color space
	// lightmapPageID is returned by GetLightmapPageIDForSortID
	// lightmapSize and offsetIntoLightmapPage are returned by AllocateLightmap.
	// You should never call UpdateLightmap for a lightmap allocated through
	// AllocateWhiteLightmap.
	virtual void				UpdateLightmap( int lightmapPageID, int lightmapSize[2],
												int offsetIntoLightmapPage[2], 
												float *pFloatImage, float *pFloatImageBump1,
												float *pFloatImageBump2, float *pFloatImageBump3 ) = 0;
	// Force the lightmaps updated with UpdateLightmap to be sent to the hardware.
	virtual void				FlushLightmaps( ) = 0;

#ifdef _XBOX
	virtual void				RegisterPalettedLightmaps( int numPages, const void *pLightmaps ) = 0;
	virtual int					FixupPalettedLightmap( int lightmapPage, IMaterial *iMaterial ) = 0;
#endif

	// fixme: could just be an array of ints for lightmapPageIDs since the material
	// for a surface is already known.
	virtual int					GetNumSortIDs( ) = 0;
//	virtual int					GetLightmapPageIDForSortID( int sortID ) = 0;
	virtual void				GetSortInfo( MaterialSystem_SortInfo_t *sortInfoArray ) = 0;

	virtual void				BeginFrame( ) = 0;
	virtual void				EndFrame( ) = 0;
	
	// Bind a material is current for rendering.
	virtual void				Bind( IMaterial *material, void *proxyData = 0 ) = 0;
	// Bind a lightmap page current for rendering.  You only have to 
	// do this for materials that require lightmaps.
	virtual void				BindLightmapPage( int lightmapPageID ) = 0;

	// inputs are between 0 and 1
	virtual void				DepthRange( float zNear, float zFar ) = 0;

	virtual void				ClearBuffers( bool bClearColor, bool bClearDepth, bool bClearStencil = false ) = 0;

	// read to a unsigned char rgb image.
	virtual void				ReadPixels( int x, int y, int width, int height, unsigned char *data, ImageFormat dstFormat ) = 0;

	// Sets lighting
	virtual void				SetAmbientLight( float r, float g, float b ) = 0;
	virtual void				SetLight( int lightNum, LightDesc_t& desc ) = 0;

	// The faces of the cube are specified in the same order as cubemap textures
	virtual void				SetAmbientLightCube( Vector4D cube[6] ) = 0;
	
	// Blit the backbuffer to the framebuffer texture
	virtual void				CopyRenderTargetToTexture( ITexture *pTexture ) = 0;
	
	// Set the current texture that is a copy of the framebuffer.
	virtual void				SetFrameBufferCopyTexture( ITexture *pTexture, int textureIndex = 0 ) = 0;
	virtual ITexture		   *GetFrameBufferCopyTexture( int textureIndex ) = 0;
	
	// Get the image format of the back buffer. . useful when creating render targets, etc.
	virtual ImageFormat			GetBackBufferFormat() const = 0;
	
	// do we need this?
	virtual void				Flush( bool flushHardware = false ) = 0;

	//
	// end vertex array api
	//

	//
	// Debugging tools
	//
	virtual void				DebugPrintUsedMaterials( const char *pSearchSubString, bool bVerbose ) = 0;
	virtual void				DebugPrintUsedTextures( void ) = 0;
#ifdef _XBOX
	virtual void				ListUsedMaterials( void ) = 0;
#endif
	virtual void				ToggleSuppressMaterial( char const* pMaterialName ) = 0;
	virtual void				ToggleDebugMaterial( char const* pMaterialName ) = 0;

	// matrix api
	virtual void				MatrixMode( MaterialMatrixMode_t matrixMode ) = 0;
	virtual void				PushMatrix( void ) = 0;
	virtual void				PopMatrix( void ) = 0;
	virtual void				LoadMatrix( VMatrix const& matrix ) = 0;
	virtual void				LoadMatrix( matrix3x4_t const& matrix ) = 0;
	virtual void				MultMatrix( VMatrix const& matrix ) = 0;
	virtual void				MultMatrix( matrix3x4_t const& matrix ) = 0;
	virtual void				MultMatrixLocal( VMatrix const& matrix ) = 0;
	virtual void				MultMatrixLocal( matrix3x4_t const& matrix ) = 0;
	virtual void				GetMatrix( MaterialMatrixMode_t matrixMode, VMatrix *matrix ) = 0;
	virtual void				GetMatrix( MaterialMatrixMode_t matrixMode, matrix3x4_t *matrix ) = 0;
	virtual void				LoadIdentity( void ) = 0;
	virtual void				Ortho( double left, double top, double right, double bottom, double zNear, double zFar ) = 0;
	virtual void				PerspectiveX( double fovx, double aspect, double zNear, double zFar ) = 0;
	virtual void				PickMatrix( int x, int y, int width, int height ) = 0;
	virtual void				Rotate( float angle, float x, float y, float z ) = 0;
	virtual void				Translate( float x, float y, float z ) = 0;
	virtual void				Scale( float x, float y, float z ) = 0;
	// end matrix api

	// Sets/gets the viewport
	virtual void				Viewport( int x, int y, int width, int height ) = 0;
	virtual void				GetViewport( int& x, int& y, int& width, int& height ) const = 0;

	// The cull mode
	virtual void				CullMode( MaterialCullMode_t cullMode ) = 0;

	// end matrix api

	// This could easily be extended to a general user clip plane
	virtual void				SetHeightClipMode( MaterialHeightClipMode_t nHeightClipMode ) = 0;
	// garymcthack : fog z is always used for heightclipz for now.
	virtual void				SetHeightClipZ( float z ) = 0;
	
	// Fog methods...
	virtual void				FogMode( MaterialFogMode_t fogMode ) = 0;
	virtual void				FogStart( float fStart ) = 0;
	virtual void				FogEnd( float fEnd ) = 0;
	virtual void				SetFogZ( float fogZ ) = 0;
	virtual MaterialFogMode_t	GetFogMode( void ) = 0;

	virtual void				FogColor3f( float r, float g, float b ) = 0;
	virtual void				FogColor3fv( float const* rgb ) = 0;
	virtual void				FogColor3ub( unsigned char r, unsigned char g, unsigned char b ) = 0;
	virtual void				FogColor3ubv( unsigned char const* rgb ) = 0;

	virtual void				GetFogColor( unsigned char *rgb ) = 0;

	// Sets the number of bones for skinning
	virtual void				SetNumBoneWeights( int numBones ) = 0;

	virtual IMaterialProxyFactory *GetMaterialProxyFactory() = 0;
	
	// Read the page size of an existing lightmap by sort id (returned from AllocateLightmap())
	virtual void				GetLightmapPageSize( int lightmap, int *width, int *height ) const = 0;
	
	/// FIXME: This stuff needs to be cleaned up and abstracted.
	// Stuff that gets exported to the launcher through the engine
	virtual void				SwapBuffers( ) = 0;

	// Use this to spew information about the 3D layer 
	virtual void				SpewDriverInfo() const = 0;

	// Creates/destroys Mesh
	virtual IMesh* CreateStaticMesh( IMaterial* pMaterial, const char *pTextureBudgetGroup, bool bForceTempMesh = false ) = 0;
	virtual IMesh* CreateStaticMesh( MaterialVertexFormat_t fmt, const char *pTextureBudgetGroup, bool bSoftwareVertexShader ) = 0;
#ifdef _XBOX
	virtual IMesh* CreateStaticMesh( unsigned int fmt, const char *pTextureBudgetGroup ) = 0;
#endif
	virtual void DestroyStaticMesh( IMesh* mesh ) = 0;

	// Gets the dynamic mesh associated with the currently bound material
	// note that you've got to render the mesh before calling this function 
	// a second time. Clients should *not* call DestroyStaticMesh on the mesh 
	// returned by this call.
	// Use buffered = false if you want to not have the mesh be buffered,
	// but use it instead in the following pattern:
	//		meshBuilder.Begin
	//		meshBuilder.End
	//		Draw partial
	//		Draw partial
	//		Draw partial
	//		meshBuilder.Begin
	//		meshBuilder.End
	//		etc
	// Use Vertex or Index Override to supply a static vertex or index buffer
	// to use in place of the dynamic buffers.
	//
	// If you pass in a material in pAutoBind, it will automatically bind the
	// material. This can be helpful since you must bind the material you're
	// going to use BEFORE calling GetDynamicMesh.
	virtual IMesh* GetDynamicMesh( 
		bool buffered = true, 
		IMesh* pVertexOverride = 0,	
		IMesh* pIndexOverride = 0, 
		IMaterial *pAutoBind = 0 ) = 0;
#ifndef _XBOX		
	// Selection mode methods
	virtual int  SelectionMode( bool selectionMode ) = 0;
	virtual void SelectionBuffer( unsigned int* pBuffer, int size ) = 0;
	virtual void ClearSelectionNames( ) = 0;
	virtual void LoadSelectionName( int name ) = 0;
	virtual void PushSelectionName( int name ) = 0;
	virtual void PopSelectionName() = 0;
#endif	
	// Installs a function to be called when we need to release vertex buffers + textures
	virtual void AddReleaseFunc( MaterialBufferReleaseFunc_t func ) = 0;
	virtual void RemoveReleaseFunc( MaterialBufferReleaseFunc_t func ) = 0;

	// Installs a function to be called when we need to restore vertex buffers
	virtual void AddRestoreFunc( MaterialBufferRestoreFunc_t func ) = 0;
	virtual void RemoveRestoreFunc( MaterialBufferRestoreFunc_t func ) = 0;

	// Reloads materials
	virtual void	ReloadMaterials( const char *pSubString = NULL ) = 0;

	virtual void	ResetMaterialLightmapPageInfo() = 0;

	// Sets the Clear Color for ClearBuffer....
	virtual void		ClearColor3ub( unsigned char r, unsigned char g, unsigned char b ) = 0;
	virtual void		ClearColor4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a ) = 0;

	// Force it to ignore Draw calls.
	virtual void		SetInStubMode( bool bInStubMode ) = 0;

	// Create a procedural material. The keyvalues looks like a VMT file
	virtual IMaterial	*CreateMaterial( const char *pMaterialName, KeyValues *pVMTKeyValues ) = 0;

	// Creates a render target
	// If depth == true, a depth buffer is also allocated. If not, then
	// the screen's depth buffer is used.
	virtual ITexture*	CreateRenderTargetTexture( 
		int w, 
		int h, 
		RenderTargetSizeMode_t sizeMode,	// Controls how size is generated (and regenerated on video mode change).
		ImageFormat format, 
		MaterialRenderTargetDepth_t depth = MATERIAL_RT_DEPTH_SHARED ) = 0;

	// Creates a procedural texture
	virtual ITexture *CreateProceduralTexture( 
		const char	*pTextureName, 
		const char	*pTextureGroupName,
		int			w, 
		int			h, 
		ImageFormat fmt, 
		int			nFlags ) = 0;

	// Allows us to override the depth buffer setting of a material
	virtual void	OverrideDepthEnable( bool bEnable, bool bDepthEnable ) = 0;

	// FIXME: This is a hack required for NVidia/XBox, can they fix in drivers?
	virtual void	DrawScreenSpaceQuad( IMaterial* pMaterial ) = 0;

	// Release temporary HW memory...
	virtual void	ReleaseTempTextureMemory() = 0;

	virtual ITexture*	CreateNamedRenderTargetTexture( 
		const char *pRTName, 
		int w, 
		int h, 
		RenderTargetSizeMode_t sizeMode,	// Controls how size is generated (and regenerated on video mode change).
		ImageFormat format, 
		MaterialRenderTargetDepth_t depth = MATERIAL_RT_DEPTH_SHARED, 
		bool bClampTexCoords = true, 
		bool bAutoMipMap = false 
		) = 0;

	// For debugging and building recording files. This will stuff a token into the recording file,
	// then someone doing a playback can watch for the token.
	virtual void	SyncToken( const char *pToken ) = 0;

	// FIXME: REMOVE THIS FUNCTION!
	// The only reason why it's not gone is because we're a week from ship when I found the bug in it
	// and everything's tuned to use it.
	// It's returning values which are 2x too big (it's returning sphere diameter x2)
	// Use ComputePixelDiameterOfSphere below in all new code instead.
	virtual float	ComputePixelWidthOfSphere( const Vector& origin, float flRadius ) = 0;

	//
	// Occlusion query support
	//
	
	// Allocate and delete query objects.
	virtual OcclusionQueryObjectHandle_t CreateOcclusionQueryObject( void ) = 0;
	virtual void DestroyOcclusionQueryObject( OcclusionQueryObjectHandle_t ) = 0;

	// Bracket drawing with begin and end so that we can get counts next frame.
	virtual void BeginOcclusionQueryDrawing( OcclusionQueryObjectHandle_t ) = 0;
	virtual void EndOcclusionQueryDrawing( OcclusionQueryObjectHandle_t ) = 0;

	// Get the number of pixels rendered between begin and end on an earlier frame.
	// Calling this in the same frame is a huge perf hit!
	virtual int OcclusionQuery_GetNumPixelsRendered( OcclusionQueryObjectHandle_t ) = 0;

	virtual void SetFlashlightMode( bool bEnable ) = 0;

	virtual void SetFlashlightState( const FlashlightState_t &state, const VMatrix &worldToTexture ) = 0;

	virtual void SetModeChangeCallBack( ModeChangeCallbackFunc_t func ) = 0;

	// Gets *recommended* configuration information associated with the display card, 
	// given a particular dx level to run under. 
	// Use dxlevel 0 to use the recommended dx level.
	// The function returns false if an invalid dxlevel was specified

	// UNDONE: To find out all convars affected by configuration, we'll need to change
	// the dxsupport.pl program to output all column headers into a single keyvalue block
	// and then we would read that in, and send it back to the client
	virtual bool GetRecommendedConfigurationInfo( int nDXLevel, KeyValues * pKeyValues ) = 0;

	// Gets the current height clip mode
	virtual MaterialHeightClipMode_t GetHeightClipMode( ) = 0;

	// This returns the diameter of the sphere in pixels based on 
	// the current model, view, + projection matrices and viewport.
	virtual float	ComputePixelDiameterOfSphere( const Vector& vecAbsOrigin, float flRadius ) = 0;

	// By default, the material system applies the VIEW and PROJECTION matrices	to the user clip
	// planes (which are specified in world space) to generate projection-space user clip planes
	// Occasionally (for the particle system in hl2, for example), we want to override that
	// behavior and explictly specify a ViewProj transform for user clip planes
	virtual void	EnableUserClipTransformOverride( bool bEnable ) = 0;
	virtual void	UserClipTransform( const VMatrix &worldToView ) = 0;

	// -----------------------------------------------------------------------------------
	// This is the end of interface version VMaterialSystem076, which is what we shipped
	// with HL2.  Add anything new past here.
	// -----------------------------------------------------------------------------------

	// Used to iterate over all shaders for editing purposes
	// GetShaders returns the number of shaders it actually found
	virtual int		ShaderCount() const = 0;
	virtual int		GetShaders( int nFirstShader, int nMaxCount, IShader **ppShaderList ) const = 0;

	// Used to enable editor materials. Must be called before Init.
	virtual void	EnableEditorMaterials() = 0;

	// Sets the material proxy factory. Calling this causes all materials to be uncached.
	virtual void	SetMaterialProxyFactory( IMaterialProxyFactory* pFactory ) = 0;

	// Returns the current adapter in use
	virtual int		GetCurrentAdapter() const = 0;

	// Allocates/Frees a vertex texture.
	// Imagine a vertex texture as an array of structures, where each structure has N fields
	// Each field, for now, is a float32.
	// NOTE: It is the responsibility of the client to deal w/ alt-tab and re-fill in the bits
#ifndef _XBOX
	virtual IVertexTexture *CreateVertexTexture( int nElementCount, int nFieldCount ) = 0;
	virtual void DestroyVertexTexture( IVertexTexture *pVertexTexture ) = 0;

	// Binds a vertex texture to a particular texture stage in the vertex pipe
	virtual void BindVertexTexture( IVertexTexture *pVertexTexture, VertexTextureStage_t nStage ) = 0;
#endif

#ifndef _XBOX
	// Creates/destroys morph data associated w/ a particular material
	virtual IMorph *CreateMorph( IMaterial *pMaterial ) = 0;
	virtual void DestroyMorph( IMorph *pMorph ) = 0;

	// Binds the morph data for use in rendering
	virtual void BindMorph( IMorph *pMorph ) = 0;

	// Sets morph target factors
	virtual void SetMorphTargetFactors( int nTargetId, float *pValue, int nCount ) = 0;
#endif

#ifndef _XBOX
	// Converts a representation specified in the src bit count to 8 bits.
	virtual color24 ConvertToColor24( RGBX5551_t inColor ) = 0;

	virtual void LockColorCorrection() = 0;
	virtual void SetColorCorrection( RGBX5551_t inColor, color24 outColor ) = 0;
	virtual void UnlockColorCorrection() = 0;

	virtual color24 GetColorCorrection( RGBX5551_t inColor ) = 0;
#endif

	// Read w/ stretch to a host-memory buffer
	virtual void ReadPixelsAndStretch( Rect_t *pSrcRect, Rect_t *pDstRect, unsigned char *pBuffer, ImageFormat dstFormat, int nDstStride ) = 0;

	// Flushes managed textures from the texture cacher
	virtual void EvictManagedResources() = 0;

	// Gets the window size
	virtual void GetWindowSize( int &width, int &height ) const = 0;

	// Set a linear vector color scale for all 3D rendering.
	// A value of [1.0f, 1.0f, 1.0f] should match non-tone-mapped rendering.
	virtual void	SetToneMappingScaleLinear( const Vector &scale ) = 0;

	virtual ITexture*	CreateNamedRenderTargetTextureEx( 
		const char *pRTName,				// Pass in NULL here for an unnamed render target.
		int w, 
		int h, 
		RenderTargetSizeMode_t sizeMode,	// Controls how size is generated (and regenerated on video mode change).
		ImageFormat format, 
		MaterialRenderTargetDepth_t depth = MATERIAL_RT_DEPTH_SHARED, 
		unsigned int textureFlags = TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT,
		unsigned int renderTargetFlags = 0
		) = 0;

	// For dealing with device lost in cases where SwapBuffers isn't called all the time (Hammer)
	virtual void HandleDeviceLost() = 0;

	// FIXME: Remove this method next time we rev interface versions; it does nothing.
	virtual void AppUsesRenderTargets() = 0;

	// This function performs a texture map from one texture map to the render destination, doing
    // all the necessary pixel/texel coordinate fix ups. fractional values can be used for the
    // src_texture coordinates to get linear sampling - integer values should produce 1:1 mappings
    // for non-scaled operations.
	virtual void DrawScreenSpaceRectangle( 
		IMaterial *pMaterial,
		int destx, int desty,
		int width, int height,
		float src_texture_x0, float src_texture_y0,			// which texel you want to appear at
		                                                    // destx/y
		float src_texture_x1, float src_texture_y1,			// which texel you want to appear at
		                                                    // destx+width-1, desty+height-1
		int src_texture_width, int src_texture_height		// needed for fixup
		)=0;

	virtual void LoadBoneMatrix( int boneIndex, const matrix3x4_t& matrix ) = 0;

	virtual void BeginRenderTargetAllocation() = 0;
	virtual void EndRenderTargetAllocation() = 0; // Simulate an Alt-Tab in here, which causes a release/restore of all resources

	// Must be called between the above Begin-End calls!
	virtual ITexture *CreateNamedRenderTargetTextureEx2( 
		const char *pRTName,				// Pass in NULL here for an unnamed render target.
		int w, 
		int h, 
		RenderTargetSizeMode_t sizeMode,	// Controls how size is generated (and regenerated on video mode change).
		ImageFormat format, 
		MaterialRenderTargetDepth_t depth = MATERIAL_RT_DEPTH_SHARED, 
		unsigned int textureFlags = TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT,
		unsigned int renderTargetFlags = 0
		) = 0;

	// This version will push the current rendertarget + current viewport onto the stack
	virtual void PushRenderTargetAndViewport( ) = 0;

	// This version will push a new rendertarget + a maximal viewport for that rendertarget onto the stack
	virtual void PushRenderTargetAndViewport( ITexture *pTexture ) = 0;

	// This version will push a new rendertarget + a specified viewport onto the stack
	virtual void PushRenderTargetAndViewport( ITexture *pTexture, int nViewX, int nViewY, int nViewW, int nViewH ) = 0;

	// This will pop a rendertarget + viewport
	virtual void PopRenderTargetAndViewport( void ) = 0;

	// FIXME: Is there a better way of doing this?
	// Returns shader flag names for editors to be able to edit them
	virtual int ShaderFlagCount() const = 0;
	virtual const char *ShaderFlagName( int nIndex ) const = 0;

	// Binds a particular texture as the current lightmap
	virtual void BindLightmapTexture( ITexture *pLightmapTexture ) = 0;

	// Gets the actual shader fallback for a particular shader
	virtual void GetShaderFallback( const char *pShaderName, char *pFallbackShader, int nFallbackLength ) = 0;

	// Blit a subrect of the current render target to another texture
	virtual void CopyRenderTargetToTextureEx( ITexture *pTexture, int nRenderTargetID, Rect_t *pSrcRect, Rect_t *pDstRect = NULL ) = 0;

	// Checks to see if a particular texture is loaded
	virtual bool IsTextureLoaded( char const* pTextureName ) const = 0;

	// Special off-center perspective matrix for DoF, MSAA jitter and poster rendering
	virtual void PerspectiveOffCenterX( double fovx, double aspect, double zNear, double zFar, double bottom, double top, double left, double right ) = 0;

	// Rendering parameters control special drawing modes withing the material system, shader
	// system, shaders, and engine. renderparm.h has their definitions.
	virtual void SetFloatRenderingParameter(int parm_number, float value) = 0;
	virtual void SetIntRenderingParameter(int parm_number, int value) = 0;
	virtual void SetVectorRenderingParameter(int parm_number, Vector const &value) = 0;

	virtual float GetFloatRenderingParameter(int parm_number) const = 0;
	virtual int GetIntRenderingParameter(int parm_number) const = 0;
	virtual Vector GetVectorRenderingParameter(int parm_number) const = 0;

	virtual void ReleaseResources(void) = 0;
	virtual void ReacquireResources(void ) = 0;

	// stencil buffer operations.
	virtual void SetStencilEnable(bool onoff) = 0;
	virtual void SetStencilFailOperation(StencilOperation_t op) = 0;
	virtual void SetStencilZFailOperation(StencilOperation_t op) = 0;
	virtual void SetStencilPassOperation(StencilOperation_t op) = 0;
	virtual void SetStencilCompareFunction(StencilComparisonFunction_t cmpfn) = 0;
	virtual void SetStencilReferenceValue(int ref) = 0;
	virtual void SetStencilTestMask(uint32 msk) = 0;
	virtual void SetStencilWriteMask(uint32 msk) = 0;
	virtual void ClearStencilBufferRectangle(int xmin, int ymin, int xmax, int ymax,int value) =0;	
	virtual Vector GetToneMappingScaleLinear( void ) = 0;

	virtual void ResetToneMappingScale( float monoscale) = 0; 			// set scale to monoscale instantly with no chasing

	// call TurnOnToneMapping before drawing the 3d scene to get the proper interpolated brightness
	// value set.
	virtual void TurnOnToneMapping(void) = 0;
	
 	// Call this when the mod has been set up, which may occur after init
 	// At this point, the game + gamebin paths have been set up
 	virtual void ModInit() = 0;
 	virtual void ModShutdown() = 0;
	virtual void GetDXLevelDefaults(uint &max_dxlevel,uint &recommended_dxlevel) = 0;

#ifndef _XBOX
	virtual void LoadColorCorrection( const char *pLookupName ) = 0;
	virtual void CopyColorCorrection( const color24 *pSrcColorCorrection ) = 0;
	virtual void ResetColorCorrection( ) = 0;
#endif

	virtual void SetRenderTargetEx( int nRenderTargetID, ITexture *pTexture ) = 0;


	// rendering clip planes, beware that only the most recently pushed plane will actually be used in a sizeable chunk of hardware configurations
	// and that changes to the clip planes mid-frame while UsingFastClipping() is true will result unresolvable depth inconsistencies
	virtual void PushCustomClipPlane( const float *pPlane ) = 0;
	virtual void PopCustomClipPlane( void ) = 0;

	//returns whether fast clipping is being used or not - needed to be exposed for better per-object clip behavior
	virtual bool UsingFastClipping( void ) = 0;

	// Returns the number of vertices + indices we can render using the dynamic mesh
	// Passing true in the second parameter will return the max # of vertices + indices
	// we can use before a flush is provoked and may return different values 
	// if called multiple times in succession. 
	// Passing false into the second parameter will return
	// the maximum possible vertices + indices that can be rendered in a single batch
	virtual void GetMaxToRender( IMesh *pMesh, bool bMaxUntilFlush, int *pMaxVerts, int *pMaxIndices ) = 0;

	// Returns the max possible vertices + indices to render in a single draw call
	virtual int GetMaxVerticesToRender( IMaterial *pMaterial ) = 0;
	virtual int GetMaxIndicesToRender( ) = 0;
	virtual void DisableAllLocalLights() = 0;
	virtual int CompareMaterialCombos( IMaterial *pMaterial1, IMaterial *pMaterial2, int lightMapID1, int lightMapID2 ) = 0;

#ifdef _XBOX
	virtual bool ForceIntoCache( IMaterial *pMaterial, bool bSyncWait ) = 0;
	virtual void CopyFrontBufferToBackBuffer() = 0;
#endif
	virtual IMesh *GetFlexMesh() = 0;

	virtual int StencilBufferBits( void ) = 0; //number of bits per pixel in the stencil buffer

	virtual void SetFlashlightStateEx( const FlashlightState_t &state, const VMatrix &worldToTexture, ITexture *pFlashlightDepthTexture ) = 0;

	// Returns the currently bound local cubemap
	virtual ITexture *GetLocalCubemap( ) = 0;

	// This is a version of clear buffers which will only clear the buffer at pixels which pass the stencil test
	virtual void ClearBuffersObeyStencil( bool bClearColor, bool bClearDepth ) = 0;

	virtual bool SupportsMSAAMode( int nMSAAMode ) = 0;

	// Hooks for firing PIX events from outside the Material System...
	virtual void BeginPIXEvent( unsigned long color, const char *szName ) = 0;
	virtual void EndPIXEvent() = 0;
	virtual void SetPIXMarker( unsigned long color, const char *szName ) = 0;
};
  
extern IMaterialSystem *materials;
extern IMaterialSystem *g_pMaterialSystem;

#endif // IMATERIALSYSTEM_H
