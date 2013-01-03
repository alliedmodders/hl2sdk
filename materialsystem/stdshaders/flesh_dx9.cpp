//========= Copyright © 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: General 'flesh' shader for AlienSwarm
//
//=============================================================================//

#include "BaseVSShader.h"
#include "flesh_helper.h"
#include "cpp_shader_constant_register_map.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( Flesh, Flesh_dx9 )

BEGIN_VS_SHADER( Flesh_dx9, "Flesh" )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( NORMALMAP, SHADER_PARAM_TYPE_TEXTURE, "models/shadertest/shader1_normal", "normal map" )
		SHADER_PARAM( TRANSMATMASKSTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "Masks for material changes: fresnel hue-shift, jellyfish, forward scatter and back scatter" );
		SHADER_PARAM( EFFECTMASKSTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "Masks for material effects: self-illum, color warping, iridescence and clearcoat" )
		SHADER_PARAM( COLORWARPTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "3D rgb-lookup table texture for tinting color map" )
		SHADER_PARAM( FRESNELCOLORWARPTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "3D rgb-lookup table texture for tinting fresnel-based hue shift color" )
		SHADER_PARAM( OPACITYTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "Texture to control surface opacity when INTERIOR=1" )
		SHADER_PARAM( IRIDESCENTWARP, SHADER_PARAM_TYPE_TEXTURE, "shader/BaseTexture", "1D lookup texture for iridescent effect" )
		SHADER_PARAM( DETAIL, SHADER_PARAM_TYPE_TEXTURE, "shader/BaseTexture", "detail map" ) 

		SHADER_PARAM( UVSCALE, SHADER_PARAM_TYPE_FLOAT, "1", "uv scale" )
		SHADER_PARAM( DETAILBLENDMODE, SHADER_PARAM_TYPE_INTEGER, "0", "detail blend mode: 1=mod2x, 2=add, 3=alpha blend (detailalpha), 4=crossfade, 5=additive self-illum, 6=multiply" )
		SHADER_PARAM( DETAILBLENDFACTOR, SHADER_PARAM_TYPE_FLOAT, "1.0", "strength of detail map" )
		SHADER_PARAM( DETAILSCALE, SHADER_PARAM_TYPE_FLOAT, "4.0", "detail map scale based on original UV set" )
		SHADER_PARAM( DETAILFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $detail" )
		SHADER_PARAM( DETAILTEXTURETRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$detail texcoord transform" )
		SHADER_PARAM( BUMPSTRENGTH, SHADER_PARAM_TYPE_FLOAT, "1.0", "bump map strength" )
		SHADER_PARAM( FRESNELBUMPSTRENGTH, SHADER_PARAM_TYPE_FLOAT, "1.0", "bump map strength for fresnel" )
		SHADER_PARAM( TRANSLUCENTFRESNELMINMAXEXP, SHADER_PARAM_TYPE_VEC3, "[0.8 1.0 1.0]", "translucency fresnel params" )

		SHADER_PARAM( INTERIOR, SHADER_PARAM_TYPE_BOOL, "1", "enable surface translucency (refractive/foggy interior)" )
		SHADER_PARAM( INTERIORFOGSTRENGTH, SHADER_PARAM_TYPE_FLOAT, "0.06", "fog strength" )
		SHADER_PARAM( INTERIORBACKGROUNDBOOST, SHADER_PARAM_TYPE_FLOAT, "7", "boosts the brightness of bright background pixels" )
		SHADER_PARAM( INTERIORAMBIENTSCALE, SHADER_PARAM_TYPE_FLOAT, "0.3", "scales ambient light in the interior volume" );
		SHADER_PARAM( INTERIORBACKLIGHTSCALE, SHADER_PARAM_TYPE_FLOAT, "0.3", "scales backlighting in the interior volume" );
		SHADER_PARAM( INTERIORCOLOR, SHADER_PARAM_TYPE_VEC3, "[0.7 0.5 0.45]", "tints light in the interior volume" )
		SHADER_PARAM( INTERIORREFRACTSTRENGTH, SHADER_PARAM_TYPE_FLOAT, "0.015", "strength of bumped refract of the background seen through the interior" )
		SHADER_PARAM( INTERIORREFRACTBLUR, SHADER_PARAM_TYPE_FLOAT, "0.2", "strength of blur applied to the background seen through the interior" )

		SHADER_PARAM( DIFFUSESOFTNORMAL, SHADER_PARAM_TYPE_FLOAT, "0.0f", "diffuse lighting uses softened normal" )
		SHADER_PARAM( DIFFUSEEXPONENT, SHADER_PARAM_TYPE_FLOAT, "1.0f", "diffuse lighting exponent" )
		SHADER_PARAM( PHONGEXPONENT, SHADER_PARAM_TYPE_FLOAT, "1.0", "specular exponent" )
		SHADER_PARAM( PHONGSCALE, SHADER_PARAM_TYPE_FLOAT, "1.0", "specular lighting scale" )
		SHADER_PARAM( PHONGEXPONENT2, SHADER_PARAM_TYPE_FLOAT, "1.0", "specular exponent 2" )
		SHADER_PARAM( PHONGFRESNEL, SHADER_PARAM_TYPE_VEC3, "[0 0.5 1]", "phong fresnel terms view-forward, 45 degrees, edge" )
		SHADER_PARAM( PHONGSCALE2, SHADER_PARAM_TYPE_FLOAT, "1.0", "specular lighting 2 scale" )
		SHADER_PARAM( PHONGFRESNEL2, SHADER_PARAM_TYPE_VEC3, "[1 1 1]", "phong fresnel terms view-forward, 45 degrees, edge" )
		SHADER_PARAM( PHONG2SOFTNESS, SHADER_PARAM_TYPE_FLOAT, "1.0", "clearcoat uses softened normal" )
		SHADER_PARAM( RIMLIGHTEXPONENT, SHADER_PARAM_TYPE_FLOAT, "1.0", "rim lighting exponent" )
		SHADER_PARAM( RIMLIGHTSCALE, SHADER_PARAM_TYPE_FLOAT, "1.0", "rim lighting scale" )
		SHADER_PARAM( PHONGCOLORTINT, SHADER_PARAM_TYPE_VEC3, "[1.0 1.0 1.0]", "specular texture tint" )
		SHADER_PARAM( AMBIENTBOOST, SHADER_PARAM_TYPE_FLOAT, "0.0", "ambient boost amount" )
		SHADER_PARAM( AMBIENTBOOSTMASKMODE, SHADER_PARAM_TYPE_INTEGER, "0", "masking mode for ambient scale: 0 = allover, 1 or 2 = mask by transmatmask r or g, 3 to 5 effectmask g b or a" )

		SHADER_PARAM( BACKSCATTER, SHADER_PARAM_TYPE_FLOAT, "0.0", "subsurface back-scatter intensity" )
		SHADER_PARAM( FORWARDSCATTER, SHADER_PARAM_TYPE_FLOAT, "0.0", "subsurface forward-scatter intensity" )
		SHADER_PARAM( SSDEPTH, SHADER_PARAM_TYPE_FLOAT, "0.1", "subsurface depth over which effect is not visible" )
		SHADER_PARAM( SSBENTNORMALINTENSITY, SHADER_PARAM_TYPE_FLOAT, "0.2", "subsurface bent normal intensity: higher is more view-dependent" )
		SHADER_PARAM( SSCOLORTINT, SHADER_PARAM_TYPE_VEC3, "[0.2, 0.05, 0.0]", "color tint for subsurface effects" )
		SHADER_PARAM( SSTINTBYALBEDO, SHADER_PARAM_TYPE_FLOAT, "0.0", "blend ss color tint to albedo color based on this factor" )
		SHADER_PARAM( NORMAL2SOFTNESS, SHADER_PARAM_TYPE_FLOAT, "0.0", "mip level for effects with soft normal (clearcoat and subsurface)" )
		SHADER_PARAM( HUESHIFTINTENSITY, SHADER_PARAM_TYPE_FLOAT, "0.5", "scales effect of fresnel-based hue-shift" )
		SHADER_PARAM( HUESHIFTFRESNELEXPONENT, SHADER_PARAM_TYPE_FLOAT, "2.0f", "fresnel exponent for hue-shift (edge-based)" )
		SHADER_PARAM( IRIDESCENCEBOOST, SHADER_PARAM_TYPE_FLOAT, "1.0f", "boost iridescence effect")
		SHADER_PARAM( IRIDESCENCEEXPONENT, SHADER_PARAM_TYPE_FLOAT, "2.0f", "fresnel exponent for iridescence effect (center-based)" )
		SHADER_PARAM( SELFILLUMTINT, SHADER_PARAM_TYPE_VEC3, "[0.7 0.5 0.45]", "self-illum color tint" )

		SHADER_PARAM( UVPROJOFFSET, SHADER_PARAM_TYPE_VEC3, "[0 0 0]", "Center for UV projection" )
		SHADER_PARAM( BBMIN, SHADER_PARAM_TYPE_VEC3, "[0 0 0]", "UV projection bounding box min" )
		SHADER_PARAM( BBMAX, SHADER_PARAM_TYPE_VEC3, "[0 0 0]", "UV projection bounding box max" )

		SHADER_PARAM( BUMPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "normal map animated texture frame" )
	END_SHADER_PARAMS

	void SetupVarsFlesh( FleshVars_t &info )
	{
		info.m_nBaseTexture = BASETEXTURE;
		info.m_nBaseTextureTransform = BASETEXTURETRANSFORM;
		info.m_nNormalMap = NORMALMAP;
		info.m_nDetailTexture = DETAIL;
		info.m_nColorWarpTexture = COLORWARPTEXTURE;
		info.m_nFresnelColorWarpTexture = FRESNELCOLORWARPTEXTURE;
		info.m_nTransMatMasksTexture = TRANSMATMASKSTEXTURE;
		info.m_nEffectMasksTexture = EFFECTMASKSTEXTURE;
		info.m_nIridescentWarpTexture = IRIDESCENTWARP;
		info.m_nOpacityTexture = OPACITYTEXTURE;
		info.m_nBumpStrength = BUMPSTRENGTH;
		info.m_nFresnelBumpStrength = FRESNELBUMPSTRENGTH;
		info.m_nUVScale = UVSCALE;
		info.m_nDetailScale = DETAILSCALE;
		info.m_nDetailFrame = DETAILFRAME;
		info.m_nDetailBlendMode = DETAILBLENDMODE;
		info.m_nDetailBlendFactor = DETAILBLENDFACTOR;
		info.m_nDetailTextureTransform = DETAILTEXTURETRANSFORM;

		info.m_nInteriorEnable = INTERIOR;
		info.m_nInteriorFogStrength = INTERIORFOGSTRENGTH;
		info.m_nInteriorBackgroundBoost = INTERIORBACKGROUNDBOOST;
		info.m_nInteriorAmbientScale = INTERIORAMBIENTSCALE;
		info.m_nInteriorBackLightScale = INTERIORBACKLIGHTSCALE;
		info.m_nInteriorColor = INTERIORCOLOR;
		info.m_nInteriorRefractStrength = INTERIORREFRACTSTRENGTH;
		info.m_nInteriorRefractBlur = INTERIORREFRACTBLUR;

		info.m_nFresnelParams = TRANSLUCENTFRESNELMINMAXEXP;
		info.m_nDiffuseSoftNormal = DIFFUSESOFTNORMAL;
		info.m_nDiffuseExponent = DIFFUSEEXPONENT;
		info.m_nSpecExp = PHONGEXPONENT;
		info.m_nSpecScale = PHONGSCALE;
		info.m_nSpecFresnel = PHONGFRESNEL;
		info.m_nSpecExp2 = PHONGEXPONENT2;
		info.m_nSpecScale2 = PHONGSCALE2;
		info.m_nSpecFresnel2 = PHONGFRESNEL2;
		info.m_nPhong2Softness = PHONG2SOFTNESS;
		info.m_nRimLightExp = RIMLIGHTEXPONENT;
		info.m_nRimLightScale = RIMLIGHTSCALE;
		info.m_nPhongColorTint = PHONGCOLORTINT;
		info.m_nSelfIllumTint = SELFILLUMTINT;
		info.m_nUVProjOffset = UVPROJOFFSET;
		info.m_nBBMin = BBMIN;
		info.m_nBBMax = BBMAX;
		info.m_nFlashlightTexture = FLASHLIGHTTEXTURE;
		info.m_nFlashlightTextureFrame = FLASHLIGHTTEXTUREFRAME;
		info.m_nBumpFrame = BUMPFRAME;
		info.m_nBackScatter = BACKSCATTER;
		info.m_nForwardScatter = FORWARDSCATTER;
		info.m_nAmbientBoost = AMBIENTBOOST;
		info.m_nAmbientBoostMaskMode = AMBIENTBOOSTMASKMODE;
		info.m_nIridescenceExponent = IRIDESCENCEEXPONENT;
		info.m_nIridescenceBoost = IRIDESCENCEBOOST;
		info.m_nHueShiftIntensity = HUESHIFTINTENSITY;
		info.m_nHueShiftFresnelExponent = HUESHIFTFRESNELEXPONENT;
		info.m_nSSDepth = SSDEPTH;
		info.m_nSSTintByAlbedo = SSTINTBYALBEDO;
		info.m_nSSBentNormalIntensity = SSBENTNORMALINTENSITY;
		info.m_nSSColorTint = SSCOLORTINT;
		info.m_nNormal2Softness = NORMAL2SOFTNESS;
	}

	bool IsTranslucent( IMaterialVar **params ) const
	{
		// Could be alpha-blended or refractive ('interior')
		return IS_FLAG_SET( MATERIAL_VAR_TRANSLUCENT ) || !!params[INTERIOR]->GetIntValue();
	}

	SHADER_INIT_PARAMS()
	{
		FleshVars_t info;
		SetupVarsFlesh( info );
		InitParamsFlesh( this, params, pMaterialName, info );
	}

	SHADER_FALLBACK
	{
		// TODO: Reasonable fallback
		if ( g_pHardwareConfig->GetDXSupportLevel() < 90 )
		{
			return "Wireframe";
		}

		return 0;
	}

	SHADER_INIT
	{
		FleshVars_t info;
		SetupVarsFlesh( info );
		InitFlesh( this, params, info );
	}

	SHADER_DRAW
	{
		FleshVars_t info;
		SetupVarsFlesh( info );
		DrawFlesh( this, params, pShaderAPI, pShaderShadow, info, vertexCompression, pContextDataPtr );
	}

END_SHADER