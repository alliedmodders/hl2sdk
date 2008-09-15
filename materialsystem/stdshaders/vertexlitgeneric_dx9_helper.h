//========= Copyright © 1996-2005, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef VERTEXLITGENERIC_DX9_HELPER_H
#define VERTEXLITGENERIC_DX9_HELPER_H

#include <string.h>


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CBaseVSShader;
class IMaterialVar;
class IShaderDynamicAPI;
class IShaderShadow;


//-----------------------------------------------------------------------------
// Init params/ init/ draw methods
//-----------------------------------------------------------------------------
struct VertexLitGeneric_DX9_Vars_t
{
	VertexLitGeneric_DX9_Vars_t() { memset( this, 0xFF, sizeof(*this) ); }

	int m_nBaseTexture;
	int m_nBaseTextureFrame;
	int m_nBaseTextureTransform;
	int m_nAlbedo;
	int m_nDetail;
	int m_nDetailFrame;
	int m_nDetailScale;
	int m_nEnvmap;
	int m_nEnvmapFrame;
	int m_nEnvmapMask;
	int m_nEnvmapMaskFrame;
	int m_nEnvmapMaskTransform;
	int m_nEnvmapTint;
	int m_nBumpmap;
	int m_nBumpFrame;
	int m_nBumpmap2;
	int m_nBumpFrame2;
	int m_nBumpmap3;
	int m_nBumpFrame3;
	int m_nBumpTransform;
	int m_nEnvmapContrast;
	int m_nEnvmapSaturation;
	int m_nAlphaTestReference;
	int m_nFlashlightTexture;
	int m_nFlashlightTextureFrame;

	int m_nSelfIllumTint;

	int m_nPhongExponent;
	int m_nPhongExponentTexture;
	int m_nLightWarpTexture;
	int m_nPhongBoost;
	int m_nPhongFresnelRanges;
	int m_nSelfIllumEnvMapMask_Alpha;
	int m_nAmbientOnly;
	int m_nHDRColorScale;
	int m_nPhong;
};

void InitParamsVertexLitGeneric_DX9( CBaseVSShader *pShader, IMaterialVar** params, const char *pMaterialName, bool bVertexLitGeneric, VertexLitGeneric_DX9_Vars_t &info );
void InitVertexLitGeneric_DX9( CBaseVSShader *pShader, IMaterialVar** params, bool bVertexLitGeneric, VertexLitGeneric_DX9_Vars_t &info );
void DrawVertexLitGeneric_DX9( CBaseVSShader *pShader, IMaterialVar** params, 
	IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow, bool bVertexLitGeneric, VertexLitGeneric_DX9_Vars_t &info );


#endif // VERTEXLITGENERIC_DX9_HELPER_H
