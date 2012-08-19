//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: NOTE: This file is for backward compat!
// We'll get rid of it soon. Most of the contents of this file were moved
// into shaderpi/ishadershadow.h, shaderapi/ishaderdynamic.h, or
// shaderapi/shareddefs.h
//
// $NoKeywords: $
//
//===========================================================================//

#ifndef SHADERAPI_SHAREDDEFS_H
#define SHADERAPI_SHAREDDEFS_H

#ifdef _WIN32
#pragma once
#endif

#include "mathlib/vector.h"
#include "mathlib/vector4d.h"
#include "mathlib/vmatrix.h"

//-----------------------------------------------------------------------------
// Important enumerations
//-----------------------------------------------------------------------------
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
	SHADER_TEXWRAPMODE_REPEAT,
	SHADER_TEXWRAPMODE_BORDER
	// MIRROR? - probably don't need it.
};

enum TextureBindFlags_t
{
	TEXTURE_BINDFLAGS_SRGBREAD = 0x80000000,
	TEXTURE_BINDFLAGS_SHADOWDEPTH = 0x40000000,
	TEXTURE_BINDFLAGS_NONE = 0
};

struct AspectRatioInfo_t
{
	bool m_bIsWidescreen;
	bool m_bIsHidef; 
	float m_flFrameBufferAspectRatio;
	float m_flPhysicalAspectRatio;
	float m_flFrameBuffertoPhysicalScalar;
	float m_flPhysicalToFrameBufferScalar;
	bool m_bInitialized;
};

struct CascadedShadowMappingState_t
{
	uint m_nNumCascades;
	bool m_bIsRenderingViewModels;
	Vector m_vLightColor;
	float m_flPadding0;
	Vector m_vLightDir;
	float m_flPadding1;
	struct {
		float m_flInvShadowTextureWidth;
		float m_flInvShadowTextureHeight;
		float m_flHalfInvShadowTextureWidth;
		float m_flHalfInvShadowTextureHeight;
	} m_TexParams;
	struct {
		float m_flShadowTextureWidth;
		float m_flShadowTextureHeight;
		float m_flSplitLerpFactorBase;
		float m_flSplitLerpFactorInvRange;
	} m_TexParams2;
	struct {
		float m_flDistLerpFactorBase;
		float m_flDistLerpFactorInvRange;
		float m_flUnused0;
		float m_flUnused1;
	} m_TexParams3;
	VMatrix m_matWorldToShadowTexMatrices[4];
	Vector4D m_vCascadeAtlasUVOffsets[4];
	Vector4D m_vCamPosition;
};

//-----------------------------------------------------------------------------
// Sampler identifiers
// Samplers are used to enable and bind textures + by programmable shading algorithms
//-----------------------------------------------------------------------------
enum Sampler_t
{
	SHADER_SAMPLER0 = 0,
	SHADER_SAMPLER1,
	SHADER_SAMPLER2,
	SHADER_SAMPLER3,
	SHADER_SAMPLER4,
	SHADER_SAMPLER5,
	SHADER_SAMPLER6,
	SHADER_SAMPLER7,
	SHADER_SAMPLER8,
	SHADER_SAMPLER9,
	SHADER_SAMPLER10,
	SHADER_SAMPLER11,
	SHADER_SAMPLER12,
	SHADER_SAMPLER13,
	SHADER_SAMPLER14,
	SHADER_SAMPLER15,

	SHADER_SAMPLER_COUNT,
};

//-----------------------------------------------------------------------------
// Vertex texture sampler identifiers
//-----------------------------------------------------------------------------
enum VertexTextureSampler_t
{
	SHADER_VERTEXTEXTURE_SAMPLER0 = 0,
	SHADER_VERTEXTEXTURE_SAMPLER1,
	SHADER_VERTEXTEXTURE_SAMPLER2,
	SHADER_VERTEXTEXTURE_SAMPLER3,
};


#if defined( _X360 )
#define REVERSE_DEPTH_ON_X360 //uncomment to use D3DFMT_D24FS8 with an inverted depth viewport for better performance. Keep this in sync with the same named #define in materialsystem/stdshaders/common_fxc.h
#endif

#if defined( REVERSE_DEPTH_ON_X360 )
#define ReverseDepthOnX360() true
#else
#define ReverseDepthOnX360() false
#endif



#endif // SHADERAPI_SHAREDDEFS_H
