//========= Copyright © 1996-2009, Valve Corporation, All rights reserved. ============//

#ifndef FLESH_HELPER_H
#define FLESH_HELPER_H
#ifdef _WIN32
#pragma once
#endif

#include <string.h>

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CBaseVSShader;
class IMaterialVar;
class IShaderDynamicAPI;
class IShaderShadow;

//-----------------------------------------------------------------------------
// Struct to hold shader param indices
//-----------------------------------------------------------------------------
struct FleshVars_t
{
	FleshVars_t()
	{
		memset( this, 0xFF, sizeof( FleshVars_t ) );
	}

	int m_nBumpStrength;
	int m_nFresnelBumpStrength;
	int m_nNormalMap;
	int m_nBaseTexture;
	int m_nBaseTextureTransform;
	int m_nDetailTexture;
	int m_nColorWarpTexture;
	int m_nFresnelColorWarpTexture;
	int m_nEffectMasksTexture;
	int m_nIridescentWarpTexture;
	int m_nOpacityTexture;
	int m_nTransMatMasksTexture;
	int m_nUVScale;
	int m_nDetailScale;
	int m_nDetailFrame;
	int m_nDetailBlendMode;
	int m_nDetailBlendFactor;
	int m_nDetailTextureTransform;

	int m_nInteriorEnable;
	int m_nInteriorFogStrength;
	int m_nInteriorBackgroundBoost;
	int m_nInteriorAmbientScale;
	int m_nInteriorBackLightScale;
	int m_nInteriorColor;
	int m_nInteriorRefractStrength;
	int m_nInteriorRefractBlur;

	int m_nFresnelParams;
	int m_nDiffuseSoftNormal;
	int m_nDiffuseExponent;
	int m_nSpecExp;
	int m_nSpecScale;
	int m_nSpecFresnel;
	int m_nSpecExp2;
	int m_nSpecScale2;
	int m_nSpecFresnel2;
	int m_nPhong2Softness;
	int m_nRimLightExp;
	int m_nRimLightScale;
	int m_nPhongColorTint;
	int m_nSelfIllumTint;
	int m_nUVProjOffset;
	int m_nBBMin;
	int m_nBBMax;
	int m_nFlashlightTexture;
	int m_nFlashlightTextureFrame;
	int m_nBumpFrame;
	int m_nGlowScale;
	int m_nBackScatter;
	int m_nForwardScatter;
	int m_nAmbientBoost;
	int m_nAmbientBoostMaskMode;
	int m_nIridescenceExponent;
	int m_nIridescenceBoost;
	int m_nHueShiftIntensity;
	int m_nHueShiftFresnelExponent;
	int m_nSSDepth;
	int m_nSSTintByAlbedo;
	int m_nSSBentNormalIntensity;
	int m_nSSColorTint;
	int m_nNormal2Softness;
};

// default shader param values
static const float kDefaultBumpStrength = 1.0f;
static const float kDefaultFresnelBumpStrength = 1.0f;
static const float kDefaultUVScale = 1.0f;
static const float kDefaultDetailScale = 4.0f;
static const int   kDefaultDetailFrame = 0;
static const int   kDefaultDetailBlendMode = 0;
static const float kDefaultDetailBlendFactor = 1.0f;

static const int   kDefaultInteriorEnable = 0;
static const float kDefaultInteriorFogStrength = 0.06f;
static const float kDefaultInteriorBackgroundBoost = 0.0f;
static const float kDefaultInteriorAmbientScale = 0.3f;
static const float kDefaultInteriorBackLightScale = 0.3f;
static const float kDefaultInteriorColor[3] = { 0.5f, 0.5f, 0.5f };
static const float kDefaultInteriorRefractStrength = 0.015f;
static const float kDefaultInteriorRefractBlur = 0.2f;

static const float kDefaultFresnelParams[3] = { 0.0f, 0.5f, 2.0f };
static const float kDefaultPhongColorTint[3] = { 1.0f, 1.0f, 1.0f };
static const float kDefaultDiffuseSoftNormal = 0.0f;
static const float kDefaultDiffuseExponent = 1.0f;
static const float kDefaultSpecExp = 1.0f;
static const float kDefaultSpecScale = 1.0f;
static const float kDefaultSpecFresnel[3] = { 0.0f, 0.5f, 1.0f };
static const float kDefaultSpecFresnel2[3] = { 1.0f, 1.0f, 1.0f };
static const float kDefaultPhong2Softness = 1.0f;
static const float kDefaultRimLightExp = 10.0f;
static const float kDefaultRimLightScale = 1.0f;
static const float kDefaultSelfIllumTint[3] = { 0.0f, 0.0f, 0.0f };
static const float kDefaultUVProjOffset[3] = { 0.0f, 0.0f, 0.0f };
static const float kDefaultBB[3] = { 0.0f, 0.0f, 0.0f };

static const int   kDefaultBumpFrame = 0;
static const float kDefaultGlowScale = 1.0f;
static const float kDefaultBackScatter = 0.0f;
static const float kDefaultForwardScatter = 0.0f;
static const float kDefaultAmbientBoost = 0.0f;
static const int   kDefaultAmbientBoostMaskMode = 0;
static const float kDefaultIridescenceExponent = 2.0f;
static const float kDefaultIridescenceBoost = 1.0f;
static const float kDefaultHueShiftIntensity = 0.5f;
static const float kDefaultHueShiftFresnelExponent = 2.0f;
static const float kDefaultSSDepth = 0.1f;
static const float kDefaultSSTintByAlbedo = 0.0f;
static const float kDefaultSSBentNormalIntensity = 0.1f;
static const float kDefaultSSColorTint[3] = { 0.2f, 0.05f, 0.0f };
static const float kDefaultNormal2Softness = 0.0f;

void InitParamsFlesh( CBaseVSShader *pShader, IMaterialVar** params, const char *pMaterialName, FleshVars_t &info );
void InitFlesh(	CBaseVSShader *pShader, IMaterialVar** params, FleshVars_t &info );
void DrawFlesh(	CBaseVSShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI,
				IShaderShadow* pShaderShadow, FleshVars_t &info, VertexCompressionType_t vertexCompression,
				CBasePerMaterialContextData **pContextDataPtr );

#endif // FLESH_HELPER_H
