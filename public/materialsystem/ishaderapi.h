//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef ISHADERAPI_H
#define ISHADERAPI_H

#ifdef _WIN32
#pragma once
#endif

#include <materialsystem/imaterial.h>
#include <materialsystem/imaterialsystem.h>

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
class CMeshBuilder;
class IMaterialVar;
struct LightDesc_t; 


//-----------------------------------------------------------------------------
// important enumerations
//-----------------------------------------------------------------------------
enum ShaderDepthFunc_t 
{ 
	SHADER_DEPTHFUNC_NEVER,
	SHADER_DEPTHFUNC_NEARER,
	SHADER_DEPTHFUNC_EQUAL,
	SHADER_DEPTHFUNC_NEAREROREQUAL,
	SHADER_DEPTHFUNC_FARTHER,
	SHADER_DEPTHFUNC_NOTEQUAL,
	SHADER_DEPTHFUNC_FARTHEROREQUAL,
	SHADER_DEPTHFUNC_ALWAYS
};

enum ShaderBlendFactor_t
{
	SHADER_BLEND_ZERO,
	SHADER_BLEND_ONE,
	SHADER_BLEND_DST_COLOR,
	SHADER_BLEND_ONE_MINUS_DST_COLOR,
	SHADER_BLEND_SRC_ALPHA,
	SHADER_BLEND_ONE_MINUS_SRC_ALPHA,
	SHADER_BLEND_DST_ALPHA,
	SHADER_BLEND_ONE_MINUS_DST_ALPHA,
	SHADER_BLEND_SRC_ALPHA_SATURATE,
	SHADER_BLEND_SRC_COLOR,
	SHADER_BLEND_ONE_MINUS_SRC_COLOR
};

enum ShaderAlphaFunc_t
{
	SHADER_ALPHAFUNC_NEVER,
	SHADER_ALPHAFUNC_LESS,
	SHADER_ALPHAFUNC_EQUAL,
	SHADER_ALPHAFUNC_LEQUAL,
	SHADER_ALPHAFUNC_GREATER,
	SHADER_ALPHAFUNC_NOTEQUAL,
	SHADER_ALPHAFUNC_GEQUAL,
	SHADER_ALPHAFUNC_ALWAYS
};

enum ShaderStencilFunc_t 
{ 
	SHADER_STENCILFUNC_NEVER = 0,
	SHADER_STENCILFUNC_LESS,
	SHADER_STENCILFUNC_EQUAL,
	SHADER_STENCILFUNC_LEQUAL,
	SHADER_STENCILFUNC_GREATER,
	SHADER_STENCILFUNC_NOTEQUAL,
	SHADER_STENCILFUNC_GEQUAL,
	SHADER_STENCILFUNC_ALWAYS
};

enum ShaderStencilOp_t 
{ 
	SHADER_STENCILOP_KEEP = 0,
	SHADER_STENCILOP_ZERO,
	SHADER_STENCILOP_SET_TO_REFERENCE,
	SHADER_STENCILOP_INCREMENT_CLAMP,
	SHADER_STENCILOP_DECREMENT_CLAMP,
	SHADER_STENCILOP_INVERT,
	SHADER_STENCILOP_INCREMENT_WRAP,
	SHADER_STENCILOP_DECREMENT_WRAP,
};

enum ShaderTexChannel_t
{
	SHADER_TEXCHANNEL_COLOR = 0,
	SHADER_TEXCHANNEL_ALPHA
};

enum ShaderShadeMode_t
{
	SHADER_FLAT = 0,
	SHADER_SMOOTH
};

enum ShaderTexCoordComponent_t
{
	SHADER_TEXCOORD_S = 0,
	SHADER_TEXCOORD_T,
	SHADER_TEXCOORD_U
};

enum ShaderPolyModeFace_t
{
	SHADER_POLYMODEFACE_FRONT,
	SHADER_POLYMODEFACE_BACK,
	SHADER_POLYMODEFACE_FRONT_AND_BACK,
};

enum ShaderPolyMode_t
{
	SHADER_POLYMODE_POINT,
	SHADER_POLYMODE_LINE,
	SHADER_POLYMODE_FILL
};

enum ShaderTexArg_t
{
	SHADER_TEXARG_TEXTURE = 0,
	SHADER_TEXARG_VERTEXCOLOR,
	SHADER_TEXARG_SPECULARCOLOR,
	SHADER_TEXARG_CONSTANTCOLOR,
	SHADER_TEXARG_PREVIOUSSTAGE,
	SHADER_TEXARG_NONE,
	SHADER_TEXARG_ZERO,
	SHADER_TEXARG_TEXTUREALPHA,
	SHADER_TEXARG_INVTEXTUREALPHA,
	SHADER_TEXARG_ONE,
};

enum ShaderTexOp_t
{
	// DX5 shaders support these
	SHADER_TEXOP_MODULATE = 0,
	SHADER_TEXOP_MODULATE2X,
	SHADER_TEXOP_MODULATE4X,
	SHADER_TEXOP_SELECTARG1,
	SHADER_TEXOP_SELECTARG2,
	SHADER_TEXOP_DISABLE,

	// DX6 shaders support these
	SHADER_TEXOP_ADD,
	SHADER_TEXOP_SUBTRACT,
	SHADER_TEXOP_ADDSIGNED2X,
	SHADER_TEXOP_BLEND_CONSTANTALPHA,
	SHADER_TEXOP_BLEND_TEXTUREALPHA,
	SHADER_TEXOP_BLEND_PREVIOUSSTAGEALPHA,
	SHADER_TEXOP_MODULATECOLOR_ADDALPHA,
	SHADER_TEXOP_MODULATEINVCOLOR_ADDALPHA,

	// DX7
	SHADER_TEXOP_DOTPRODUCT3
};

enum ShaderTexGenParam_t
{
	SHADER_TEXGENPARAM_OBJECT_LINEAR,
	SHADER_TEXGENPARAM_EYE_LINEAR,
	SHADER_TEXGENPARAM_SPHERE_MAP,
	SHADER_TEXGENPARAM_CAMERASPACEREFLECTIONVECTOR,
	SHADER_TEXGENPARAM_CAMERASPACENORMAL
};

enum ShaderDrawBitField_t
{
	SHADER_DRAW_POSITION			= 0x0001,
	SHADER_DRAW_NORMAL				= 0x0002,
	SHADER_DRAW_COLOR				= 0x0004,
	SHADER_DRAW_SPECULAR			= 0x0008,

	SHADER_DRAW_TEXCOORD0			= 0x0010,
	SHADER_DRAW_TEXCOORD1			= 0x0020,
	SHADER_DRAW_TEXCOORD2			= 0x0040,
	SHADER_DRAW_TEXCOORD3			= 0x0080,

	SHADER_DRAW_LIGHTMAP_TEXCOORD0	= 0x0100,
	SHADER_DRAW_LIGHTMAP_TEXCOORD1	= 0x0200,
	SHADER_DRAW_LIGHTMAP_TEXCOORD2	= 0x0400,
	SHADER_DRAW_LIGHTMAP_TEXCOORD3	= 0x0800,

	SHADER_DRAW_SECONDARY_TEXCOORD0	= 0x1000,
	SHADER_DRAW_SECONDARY_TEXCOORD1	= 0x2000,
	SHADER_DRAW_SECONDARY_TEXCOORD2	= 0x4000,
	SHADER_DRAW_SECONDARY_TEXCOORD3	= 0x8000,

	// Texgen draw flags are set by the engine; don't set these in the shader
	SHADER_DRAW_TEXGEN_TEXCOORD0	= 0x10000,
	SHADER_DRAW_TEXGEN_TEXCOORD1	= 0x20000,
	SHADER_DRAW_TEXGEN_TEXCOORD2	= 0x40000,
	SHADER_DRAW_TEXGEN_TEXCOORD3	= 0x80000,

	SHADER_TEXCOORD_MASK = SHADER_DRAW_TEXCOORD0 | SHADER_DRAW_TEXCOORD1 | 
							SHADER_DRAW_TEXCOORD2 | SHADER_DRAW_TEXCOORD3,

	SHADER_LIGHTMAP_TEXCOORD_MASK = SHADER_DRAW_LIGHTMAP_TEXCOORD0 | 
									SHADER_DRAW_LIGHTMAP_TEXCOORD1 | 
									SHADER_DRAW_LIGHTMAP_TEXCOORD2 | 
									SHADER_DRAW_LIGHTMAP_TEXCOORD3,

	SHADER_SECONDARY_TEXCOORD_MASK = SHADER_DRAW_SECONDARY_TEXCOORD0 | 
									SHADER_DRAW_SECONDARY_TEXCOORD1 | 
									SHADER_DRAW_SECONDARY_TEXCOORD2 | 
									SHADER_DRAW_SECONDARY_TEXCOORD3,

	SHADER_TEXGEN_TEXCOORD_MASK = SHADER_DRAW_TEXGEN_TEXCOORD0 | 
									SHADER_DRAW_TEXGEN_TEXCOORD1 | 
									SHADER_DRAW_TEXGEN_TEXCOORD2 | 
									SHADER_DRAW_TEXGEN_TEXCOORD3,
};

enum ShaderTexFilterMode_t
{
	SHADER_TEXFILTERMODE_NEAREST,
	SHADER_TEXFILTERMODE_LINEAR,
	SHADER_TEXFILTERMODE_NEAREST_MIPMAP_NEAREST,
	SHADER_TEXFILTERMODE_LINEAR_MIPMAP_NEAREST,
	SHADER_TEXFILTERMODE_NEAREST_MIPMAP_LINEAR,
	SHADER_TEXFILTERMODE_LINEAR_MIPMAP_LINEAR,
	SHADER_TEXFILTERMODE_ANISOTROPIC
};

enum ShaderTexWrapMode_t
{
	SHADER_TEXWRAPMODE_CLAMP,
	SHADER_TEXWRAPMODE_REPEAT
	// MIRROR? - probably don't need it.
};


//-----------------------------------------------------------------------------
// Texture stage identifiers
//-----------------------------------------------------------------------------
enum TextureStage_t
{
	SHADER_TEXTURE_STAGE0 = 0,
	SHADER_TEXTURE_STAGE1,
	SHADER_TEXTURE_STAGE2,
	SHADER_TEXTURE_STAGE3,
	SHADER_TEXTURE_STAGE4,
	SHADER_TEXTURE_STAGE5,
	SHADER_TEXTURE_STAGE6,
	SHADER_TEXTURE_STAGE7,
	SHADER_TEXTURE_STAGE8,
	SHADER_TEXTURE_STAGE9,
	SHADER_TEXTURE_STAGE10,
	SHADER_TEXTURE_STAGE11,
	SHADER_TEXTURE_STAGE12,
	SHADER_TEXTURE_STAGE13,
	SHADER_TEXTURE_STAGE14,
	SHADER_TEXTURE_STAGE15,
};

enum ShaderFogMode_t
{
	SHADER_FOGMODE_DISABLED = 0,
	SHADER_FOGMODE_OO_OVERBRIGHT,
	SHADER_FOGMODE_BLACK,
	SHADER_FOGMODE_GREY,
	SHADER_FOGMODE_FOGCOLOR,
	SHADER_FOGMODE_WHITE,
	SHADER_FOGMODE_NUMFOGMODES
};

enum ShaderMaterialSource_t
{
	SHADER_MATERIALSOURCE_MATERIAL = 0,
	SHADER_MATERIALSOURCE_COLOR1,
	SHADER_MATERIALSOURCE_COLOR2,
};


//-----------------------------------------------------------------------------
// The Shader interface versions
//-----------------------------------------------------------------------------
#define SHADERAPI_INTERFACE_VERSION		"ShaderApi028"
#define SHADERSHADOW_INTERFACE_VERSION	"ShaderShadow010"

//-----------------------------------------------------------------------------
// Methods that can be called from the SHADER_INIT blocks of shaders
//-----------------------------------------------------------------------------
abstract_class IShaderInit
{
public:
	// Loads up a texture
	virtual void LoadTexture( IMaterialVar *pTextureVar, const char *pTextureGroupName ) = 0; 
	virtual void LoadBumpMap( IMaterialVar *pTextureVar, const char *pTextureGroupName ) = 0;
	virtual void LoadCubeMap( IMaterialVar **ppParams, IMaterialVar *pTextureVar ) = 0;
};


//-----------------------------------------------------------------------------
// the shader API interface (methods called from shaders)
//-----------------------------------------------------------------------------
abstract_class IShaderShadow
{
public:
	// Sets the default *shadow* state
	virtual void SetDefaultState() = 0;

	// Methods related to depth buffering
	virtual void DepthFunc( ShaderDepthFunc_t depthFunc ) = 0;
	virtual void EnableDepthWrites( bool bEnable ) = 0;
	virtual void EnableDepthTest( bool bEnable ) = 0;
	virtual void EnablePolyOffset( bool bEnable ) = 0;

	// These methods for controlling stencil are obsolete and stubbed to do nothing.  Stencil
	// control is via the shaderapi/material system now, not part of the shadow state.
	// Methods related to stencil
	virtual void EnableStencil( bool bEnable ) = 0;
	virtual void StencilFunc( ShaderStencilFunc_t stencilFunc ) = 0;
	virtual void StencilPassOp( ShaderStencilOp_t stencilOp ) = 0;
	virtual void StencilFailOp( ShaderStencilOp_t stencilOp ) = 0;
	virtual void StencilDepthFailOp( ShaderStencilOp_t stencilOp ) = 0;
	virtual void StencilReference( int nReference ) = 0;
	virtual void StencilMask( int nMask ) = 0;
	virtual void StencilWriteMask( int nMask ) = 0;

	// Suppresses/activates color writing 
	virtual void EnableColorWrites( bool bEnable ) = 0;
	virtual void EnableAlphaWrites( bool bEnable ) = 0;

	// Methods related to alpha blending
	virtual void EnableBlending( bool bEnable ) = 0;
	virtual void BlendFunc( ShaderBlendFactor_t srcFactor, ShaderBlendFactor_t dstFactor ) = 0;

	// Alpha testing
	virtual void EnableAlphaTest( bool bEnable ) = 0;
	virtual void AlphaFunc( ShaderAlphaFunc_t alphaFunc, float alphaRef /* [0-1] */ ) = 0;

	// Wireframe/filled polygons
	virtual void PolyMode( ShaderPolyModeFace_t face, ShaderPolyMode_t polyMode ) = 0;

	// Back face culling
	virtual void EnableCulling( bool bEnable ) = 0;
	
	// constant color + transparency
	virtual void EnableConstantColor( bool bEnable ) = 0;
#ifndef _XBOX
	// Indicates we're preprocessing vertex data
	virtual void EnableVertexDataPreprocess( bool bEnable ) = 0;
#endif
	// Indicates the vertex format for use with a vertex shader
	// The flags to pass in here come from the VertexFormatFlags_t enum
	// If pTexCoordDimensions is *not* specified, we assume all coordinates
	// are 2-dimensional
	virtual void VertexShaderVertexFormat( unsigned int flags, 
			int numTexCoords, int* pTexCoordDimensions, int numBoneWeights,
			int userDataSize ) = 0;

	// Pixel and vertex shader methods
	virtual void SetVertexShader( char const* pFileName, int nStaticVshIndex ) = 0;
	virtual	void SetPixelShader( char const* pFileName, int nStaticPshIndex = 0 ) = 0;

	// Indicates we're going to light the model
	virtual void EnableLighting( bool bEnable ) = 0;

	// Enables specular lighting (lighting has also got to be enabled)
	virtual void EnableSpecular( bool bEnable ) = 0;
#ifndef _XBOX
	// Convert from linear to gamma color space on writes to frame buffer.
	virtual void EnableSRGBWrite( bool bEnable ) = 0;
#endif

#ifndef _XBOX
	// Convert from gamma to linear on texture fetch.
	virtual void EnableSRGBRead( TextureStage_t stage, bool bEnable ) = 0;
#endif
	// Activate/deactivate skinning. Indexed blending is automatically
	// enabled if it's available for this hardware. When blending is enabled,
	// we allocate enough room for 3 weights (max allowed)
	virtual void EnableVertexBlend( bool bEnable ) = 0;

	// per texture unit stuff
	virtual void OverbrightValue( TextureStage_t stage, float value ) = 0;
	virtual void EnableTexture( TextureStage_t stage, bool bEnable ) = 0;
	virtual void EnableTexGen( TextureStage_t stage, bool bEnable ) = 0;
	virtual void TexGen( TextureStage_t stage, ShaderTexGenParam_t param ) = 0;

	// alternate method of specifying per-texture unit stuff, more flexible and more complicated
	// Can be used to specify different operation per channel (alpha/color)...
	virtual void EnableCustomPixelPipe( bool bEnable ) = 0;
	virtual void CustomTextureStages( int stageCount ) = 0;
	virtual void CustomTextureOperation( TextureStage_t stage, ShaderTexChannel_t channel, 
		ShaderTexOp_t op, ShaderTexArg_t arg1, ShaderTexArg_t arg2 ) = 0;

	// indicates what per-vertex data we're providing
	virtual void DrawFlags( unsigned int drawFlags ) = 0;

	// A simpler method of dealing with alpha modulation
	virtual void EnableAlphaPipe( bool bEnable ) = 0;
	virtual void EnableConstantAlpha( bool bEnable ) = 0;
	virtual void EnableVertexAlpha( bool bEnable ) = 0;
	virtual void EnableTextureAlpha( TextureStage_t stage, bool bEnable ) = 0;

	// GR - Separate alpha blending
#ifndef _XBOX
	virtual void EnableBlendingSeparateAlpha( bool bEnable ) = 0;
	virtual void BlendFuncSeparateAlpha( ShaderBlendFactor_t srcFactor, ShaderBlendFactor_t dstFactor ) = 0;
#endif
	virtual void FogMode( ShaderFogMode_t fogMode ) = 0;

	virtual void SetDiffuseMaterialSource( ShaderMaterialSource_t materialSource ) = 0;

	// Indicates the morph format for use with a vertex shader
	// The flags to pass in here come from the MorphFormatFlags_t enum
	virtual void SetMorphFormat( MorphFormat_t flags ) = 0;
#ifdef _XBOX
	virtual void SetColorSign( TextureStage_t stage, bool isSigned ) = 0;
#endif
};
// end class IShaderShadow

//-----------------------------------------------------------------------------
// the 3D shader API interface
// This interface is all that shaders see.
//-----------------------------------------------------------------------------
enum StandardTextureId_t
{
	// Lightmaps
	TEXTURE_LIGHTMAP = 0,
	TEXTURE_LIGHTMAP_ALPHA,
	TEXTURE_LIGHTMAP_FULLBRIGHT,
	TEXTURE_LIGHTMAP_BUMPED,
	TEXTURE_LIGHTMAP_BUMPED_FULLBRIGHT,

	// Flat colors
	TEXTURE_WHITE,
	TEXTURE_BLACK,
	TEXTURE_GREY,

	// Normalmaps
	TEXTURE_NORMALMAP_FLAT,

	// Normalization
	TEXTURE_NORMALIZATION_CUBEMAP,
	TEXTURE_NORMALIZATION_CUBEMAP_SIGNED,

	// Frame-buffer textures
	TEXTURE_FRAME_BUFFER_FULL_TEXTURE_0,
	TEXTURE_FRAME_BUFFER_FULL_TEXTURE_1,

	// Color correction
	TEXTURE_COLOR_CORRECTION_VOLUME_0,
	TEXTURE_COLOR_CORRECTION_VOLUME_1,
	TEXTURE_COLOR_CORRECTION_VOLUME_2,
	TEXTURE_COLOR_CORRECTION_VOLUME_3,

#ifdef _XBOX
	// An alias to the Back Frame Buffer
	TEXTURE_FRAME_BUFFER_ALIAS,
#endif
};


abstract_class IShaderDynamicAPI
{
public:
	// returns the current time in seconds....
	virtual double CurrentTime() const = 0;

	// NOTE: All these various bind methods are deprecated. Use the StandardTextureId_t enum in future
	// Lightmap texture binding
	virtual void BindLightmap( TextureStage_t stage ) = 0;
	// GR - bind separate lightmap alpha
	virtual void BindLightmapAlpha( TextureStage_t stage ) = 0;
	virtual void BindBumpLightmap( TextureStage_t stage ) = 0;
	virtual void BindFullbrightLightmap( TextureStage_t stage ) = 0;
	virtual void BindWhite( TextureStage_t stage ) = 0;
	virtual void BindBlack( TextureStage_t stage ) = 0;
	virtual void BindGrey( TextureStage_t stage ) = 0;
//	virtual void BindSyncTexture( TextureStage_t stage, int texture ) = 0;
	virtual void BindFBTexture( TextureStage_t stage, int textureIndex = 0 ) = 0;

	// Gets the lightmap dimensions
	virtual void GetLightmapDimensions( int *w, int *h ) = 0;

	// Special system flat normal map binding.
	virtual void BindFlatNormalMap( TextureStage_t stage ) = 0;
	virtual void BindNormalizationCubeMap( TextureStage_t stage ) = 0;
#ifndef _XBOX
	virtual void BindSignedNormalizationCubeMap( TextureStage_t stage ) = 0;
#endif
	// Scene fog state.
	// This is used by the shaders for picking the proper vertex shader for fogging based on dynamic state.
	virtual MaterialFogMode_t GetSceneFogMode( ) = 0;
	virtual void GetSceneFogColor( unsigned char *rgb ) = 0;

	// stuff related to matrix stacks
	virtual void MatrixMode( MaterialMatrixMode_t matrixMode ) = 0;
	virtual void PushMatrix() = 0;
	virtual void PopMatrix() = 0;
	virtual void LoadMatrix( float *m ) = 0;
	virtual void MultMatrix( float *m ) = 0;
	virtual void MultMatrixLocal( float *m ) = 0;
	virtual void GetMatrix( MaterialMatrixMode_t matrixMode, float *dst ) = 0;
	virtual void LoadIdentity( void ) = 0;
	virtual void LoadCameraToWorld( void ) = 0;
	virtual void Ortho( double left, double right, double bottom, double top, double zNear, double zFar ) = 0;
	virtual void PerspectiveX( double fovx, double aspect, double zNear, double zFar ) = 0;
	virtual	void PickMatrix( int x, int y, int width, int height ) = 0;
	virtual void Rotate( float angle, float x, float y, float z ) = 0;
	virtual void Translate( float x, float y, float z ) = 0;
	virtual void Scale( float x, float y, float z ) = 0;
	virtual void ScaleXY( float x, float y ) = 0;

	// Sets the color to modulate by
	virtual void Color3f( float r, float g, float b ) = 0;
	virtual void Color3fv( float const* pColor ) = 0;
	virtual void Color4f( float r, float g, float b, float a ) = 0;
	virtual void Color4fv( float const* pColor ) = 0;

	virtual void Color3ub( unsigned char r, unsigned char g, unsigned char b ) = 0;
	virtual void Color3ubv( unsigned char const* pColor ) = 0;
	virtual void Color4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a ) = 0;
	virtual void Color4ubv( unsigned char const* pColor ) = 0;

	// Sets the constant register for vertex and pixel shaders
	virtual void SetVertexShaderConstant( int var, float const* pVec, int numConst = 1, bool bForce = false ) = 0;
	virtual void SetPixelShaderConstant( int var, float const* pVec, int numConst = 1, bool bForce = false ) = 0;

	// Sets the default *dynamic* state
	virtual void SetDefaultState() = 0;

	// Get the current camera position in world space.
	virtual void GetWorldSpaceCameraPosition( float* pPos ) const = 0;

	virtual int GetCurrentNumBones( void ) const = 0;
	virtual int GetCurrentLightCombo( void ) const = 0;
	virtual MaterialFogMode_t GetCurrentFogType( void ) const = 0;

	// fixme: move this to shadow state
	virtual void SetTextureTransformDimension( int textureStage, int dimension, bool projected ) = 0;
	virtual void DisableTextureTransform( int textureStage ) = 0;
	virtual void SetBumpEnvMatrix( int textureStage, float m00, float m01, float m10, float m11 ) = 0;

	// Sets the vertex and pixel shaders
	virtual void SetVertexShaderIndex( int vshIndex = -1 ) = 0;
	virtual void SetPixelShaderIndex( int pshIndex = 0 ) = 0;

	// Get the dimensions of the back buffer.
	virtual void GetBackBufferDimensions( int& width, int& height ) const = 0;

	// FIXME: The following 6 methods used to live in IShaderAPI
	// and were moved for stdshader_dx8. Let's try to move them back!

	// Get the lights
	virtual int GetMaxLights( void ) const = 0;
	virtual const LightDesc_t& GetLight( int lightNum ) const = 0;

	virtual void SetPixelShaderFogParams( int reg ) = 0;

	// Render state for the ambient light cube
	virtual void SetVertexShaderStateAmbientLightCube() = 0;
	virtual void SetPixelShaderStateAmbientLightCube( int pshReg ) = 0;
	virtual void CommitPixelShaderLighting( int pshReg ) = 0;

	// Use this to get the mesh builder that allows us to modify vertex data
	virtual CMeshBuilder* GetVertexModifyBuilder() = 0;
	virtual bool InFlashlightMode() const = 0;
	virtual const FlashlightState_t &GetFlashlightState( VMatrix &worldToTexture ) const = 0;
	virtual bool InEditorMode() const = 0;

	//
	// NOTE: Stuff after this is added after shipping HL2.
	//
#ifndef _XBOX
	// Gets the bound morph's vertex format; returns 0 if no morph is bound
	virtual MorphFormat_t GetBoundMorphFormat() = 0;
#endif
	// Binds a standard texture
	virtual void BindStandardTexture( TextureStage_t stage, StandardTextureId_t id ) = 0;

	virtual ITexture *GetRenderTargetEx( int nRenderTargetID ) = 0;

#ifndef _XBOX
	virtual void SetToneMappingScaleLinear( const Vector &scale ) = 0;
	virtual const Vector &GetToneMappingScaleLinear( void ) const = 0;
	virtual const Vector &GetToneMappingScaleGamma( void ) const = 0;
#endif

#ifdef _XBOX
	virtual bool GetBoundTextureDimensions(TextureStage_t stage, int *pWidth, int *pHeight, bool *pIsLinear ) = 0;
#endif

	virtual void LoadBoneMatrix( int boneIndex, const float *m ) = 0;

	virtual void PerspectiveOffCenterX( double fovx, double aspect, double zNear, double zFar, double bottom, double top, double left, double right ) = 0;

	virtual void SetFloatRenderingParameter(int parm_number, float value) = 0;

	virtual void SetIntRenderingParameter(int parm_number, int value) = 0 ;
	virtual void SetVectorRenderingParameter(int parm_number, Vector const &value) = 0 ;

	virtual float GetFloatRenderingParameter(int parm_number) const = 0 ;

	virtual int GetIntRenderingParameter(int parm_number) const = 0 ;

	virtual Vector GetVectorRenderingParameter(int parm_number) const = 0 ;

	// stencil buffer operations.
	virtual void SetStencilEnable(bool onoff) = 0;
	virtual void SetStencilFailOperation(StencilOperation_t op) = 0;
	virtual void SetStencilZFailOperation(StencilOperation_t op) = 0;
	virtual void SetStencilPassOperation(StencilOperation_t op) = 0;
	virtual void SetStencilCompareFunction(StencilComparisonFunction_t cmpfn) = 0;
	virtual void SetStencilReferenceValue(int ref) = 0;
	virtual void SetStencilTestMask(uint32 msk) = 0;
	virtual void SetStencilWriteMask(uint32 msk) = 0;
	virtual void ClearStencilBufferRectangle(
		int xmin, int ymin, int xmax, int ymax,int value)=0;

	virtual void GetDXLevelDefaults(uint &max_dxlevel,uint &recommended_dxlevel) = 0;

	virtual const FlashlightState_t &GetFlashlightStateEx( VMatrix &worldToTexture, ITexture **pFlashlightDepthTexture ) const = 0;

	virtual float GetAmbientLightCubeLuminance() = 0;
};
// end class IShaderDynamicAPI

//-----------------------------------------------------------------------------
// Software vertex shaders
//-----------------------------------------------------------------------------
typedef void (*SoftwareVertexShader_t)( CMeshBuilder& meshBuilder, IMaterialVar **params, IShaderDynamicAPI *pShaderAPI );


#endif // ISHADERAPI_H
