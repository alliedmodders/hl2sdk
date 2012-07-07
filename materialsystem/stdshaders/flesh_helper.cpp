//========= Copyright © 1996-2009, Valve Corporation, All rights reserved. ============//

#include "BaseVSShader.h"
#include "flesh_helper.h"
#include "cpp_shader_constant_register_map.h"
/*
#include "mathlib/VMatrix.h"
#include "convar.h"
*/

// Auto generated inc files
#include "flesh_vs30.inc"
#include "flesh_ps30.inc"

#include "shaderlib/commandbuilder.h"


void InitParamsFlesh( CBaseVSShader *pShader, IMaterialVar** params, const char *pMaterialName, FleshVars_t &info )
{
	// Set material parameter default values
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nUVScale, kDefaultUVScale );
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nDetailScale, kDefaultDetailScale );
	SET_PARAM_INT_IF_NOT_DEFINED(   info.m_nDetailFrame, kDefaultDetailFrame );
	SET_PARAM_INT_IF_NOT_DEFINED(   info.m_nDetailBlendMode, kDefaultDetailBlendMode );
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nDetailBlendFactor, kDefaultDetailBlendFactor );
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nBumpStrength, kDefaultBumpStrength );
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nFresnelBumpStrength, kDefaultFresnelBumpStrength );

	SET_PARAM_INT_IF_NOT_DEFINED(   info.m_nInteriorEnable, kDefaultInteriorEnable );
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nInteriorFogStrength, kDefaultInteriorFogStrength );
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nInteriorBackgroundBoost, kDefaultInteriorBackgroundBoost );
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nInteriorAmbientScale, kDefaultInteriorAmbientScale );
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nInteriorBackLightScale, kDefaultInteriorBackLightScale );	
	SET_PARAM_VEC_IF_NOT_DEFINED(   info.m_nInteriorColor, kDefaultInteriorColor, 3 );
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nInteriorRefractStrength, kDefaultInteriorRefractStrength );
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nInteriorRefractBlur, kDefaultInteriorRefractBlur );

	SET_PARAM_VEC_IF_NOT_DEFINED( info.m_nFresnelParams, kDefaultFresnelParams, 3 );
	SET_PARAM_VEC_IF_NOT_DEFINED( info.m_nPhongColorTint, kDefaultPhongColorTint, 3 );
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nDiffuseSoftNormal, kDefaultDiffuseSoftNormal );
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nDiffuseExponent, kDefaultDiffuseExponent );
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nSpecExp, kDefaultSpecExp );
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nSpecScale, kDefaultSpecScale );
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nSpecExp2, kDefaultSpecExp );
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nSpecScale2, kDefaultSpecScale );
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nPhong2Softness, kDefaultPhong2Softness );
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nRimLightExp, kDefaultRimLightExp );
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nRimLightScale, kDefaultRimLightScale );
	SET_PARAM_VEC_IF_NOT_DEFINED( info.m_nSelfIllumTint, kDefaultSelfIllumTint, 3 );
	SET_PARAM_VEC_IF_NOT_DEFINED( info.m_nUVProjOffset, kDefaultUVProjOffset, 3 );
	SET_PARAM_VEC_IF_NOT_DEFINED( info.m_nBBMin, kDefaultBB, 3 );
	SET_PARAM_VEC_IF_NOT_DEFINED( info.m_nBBMax, kDefaultBB, 3 );
	
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nBackScatter, kDefaultBackScatter );
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nForwardScatter, kDefaultForwardScatter );
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nAmbientBoost, kDefaultAmbientBoost );
	SET_PARAM_INT_IF_NOT_DEFINED( info.m_nAmbientBoostMaskMode, kDefaultAmbientBoostMaskMode );
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nIridescenceExponent, kDefaultIridescenceExponent );
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nIridescenceBoost, kDefaultIridescenceBoost );
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nHueShiftIntensity, kDefaultHueShiftIntensity );
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nHueShiftFresnelExponent, kDefaultHueShiftFresnelExponent );
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nSSDepth, kDefaultSSDepth );
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nSSTintByAlbedo, kDefaultSSTintByAlbedo );
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nSSBentNormalIntensity, kDefaultSSBentNormalIntensity );
	SET_PARAM_VEC_IF_NOT_DEFINED( info.m_nSSColorTint, kDefaultSSColorTint, 3 );
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nNormal2Softness, kDefaultNormal2Softness );

	// FLASHLIGHTFIXME: Do ShaderAPI::BindFlashlightTexture
	Assert( info.m_nFlashlightTexture >= 0 );
	params[FLASHLIGHTTEXTURE]->SetStringValue( GetFlashlightTextureFilename() );

	SET_PARAM_INT_IF_NOT_DEFINED( info.m_nFlashlightTextureFrame, 0 );

	SET_PARAM_INT_IF_NOT_DEFINED( info.m_nBumpFrame, kDefaultBumpFrame )

	// Set material flags
	SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_HW_SKINNING );
	SET_FLAGS2( MATERIAL_VAR2_LIGHTING_VERTEX_LIT );

	//SET_FLAGS2( MATERIAL_VAR2_NEEDS_TANGENT_SPACES );

	// If 'interior' is enabled, we use the refract buffer:
	if ( params[info.m_nInteriorEnable]->IsDefined() && params[info.m_nInteriorEnable]->GetIntValue() != 0 )
	{
		//SET_FLAGS2( MATERIAL_VAR2_NEEDS_FULL_FRAME_BUFFER_TEXTURE );
		SET_FLAGS2( MATERIAL_VAR2_NEEDS_POWER_OF_TWO_FRAME_BUFFER_TEXTURE );
	}
}

void InitFlesh( CBaseVSShader *pShader, IMaterialVar** params, FleshVars_t &info )
{
	// Load textures
	if ( (info.m_nBaseTexture != -1) && params[info.m_nBaseTexture]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nBaseTexture );
	}

	if ( (info.m_nNormalMap != -1) && params[info.m_nNormalMap]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nNormalMap );
	}

	if ( (info.m_nTransMatMasksTexture != -1) && params[info.m_nTransMatMasksTexture]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nTransMatMasksTexture );
	}

	if ( (info.m_nEffectMasksTexture != -1) && params[info.m_nEffectMasksTexture]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nEffectMasksTexture );
	}

	if ( (info.m_nIridescentWarpTexture != -1) && params[info.m_nIridescentWarpTexture]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nIridescentWarpTexture );
	}

	if ( (info.m_nFresnelColorWarpTexture != -1) && params[info.m_nFresnelColorWarpTexture]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nFresnelColorWarpTexture );
	}

	if ( (info.m_nColorWarpTexture != -1) && params[info.m_nColorWarpTexture]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nColorWarpTexture );
	}

	if ( (info.m_nOpacityTexture != -1) && params[info.m_nOpacityTexture]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nOpacityTexture );
	}

	if ( (info.m_nFlashlightTexture != -1) && params[info.m_nFlashlightTexture]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nFlashlightTexture );
	}

	if ( ( info.m_nDetailTexture != -1) && params[info.m_nDetailTexture]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nDetailTexture );
	}
}

class CFlesh_DX9_Context : public CBasePerMaterialContextData
{
public:
	CCommandBufferBuilder< CFixedCommandStorageBuffer< 1200 > > m_SemiStaticCmdsOut;
};

void DrawFlesh(  CBaseVSShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI,
				IShaderShadow* pShaderShadow, FleshVars_t &info, VertexCompressionType_t vertexCompression,
				CBasePerMaterialContextData **pContextDataPtr )
{
	CFlesh_DX9_Context *pContextData = reinterpret_cast< CFlesh_DX9_Context *> ( *pContextDataPtr );

	bool bHasFlashlight = pShader->UsingFlashlight( params );
	bool bAlphaBlend = IS_FLAG_SET( MATERIAL_VAR_TRANSLUCENT );
	bool bDetail = ( info.m_nDetailTexture != -1 ) && ( params[info.m_nDetailTexture]->IsTexture() );

	if ( pShader->IsSnapshotting() || (! pContextData ) || ( pContextData->m_bMaterialVarsChanged ) )
	{
		bool bTransMatMasks = (info.m_nTransMatMasksTexture != -1) && params[info.m_nTransMatMasksTexture]->IsTexture();
		bool bEffectMasks = (info.m_nEffectMasksTexture != -1) && params[info.m_nEffectMasksTexture]->IsTexture();
		bool bIridescentWarp = (info.m_nIridescentWarpTexture != -1) && params[info.m_nIridescentWarpTexture]->IsTexture();
		bool bFresnelColorWarp = (info.m_nFresnelColorWarpTexture != -1) && params[info.m_nFresnelColorWarpTexture]->IsTexture();
		bool bColorWarp = (info.m_nColorWarpTexture != -1) && params[info.m_nColorWarpTexture]->IsTexture();
		bool bOpacityTexture = (info.m_nOpacityTexture != -1) && params[info.m_nOpacityTexture]->IsTexture();
		bool bInteriorLayer = (info.m_nInteriorEnable != -1) && ( params[info.m_nInteriorEnable]->GetIntValue() > 0 );
		bool bBackScatter = ( info.m_nBackScatter != -1 ) && ( params[info.m_nBackScatter]->GetFloatValue() > 0 );
		bool bForwardScatter = ( info.m_nForwardScatter != -1) && ( params[info.m_nForwardScatter]->GetFloatValue() > 0 );
		bool bNormal2 = ( info.m_nNormal2Softness != -1 ) && ( params[info.m_nNormal2Softness]->GetFloatValue() > 0 );

		if ( pShader->IsSnapshotting() )
		{
			// Set stream format (note that this shader supports compression)
			unsigned int flags = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_FORMAT_COMPRESSED;
			int nTexCoordCount = 1;
			int userDataSize = 4;
			int texCoordDims[4] = { 2, 2, 2, 2 };
			pShaderShadow->VertexShaderVertexFormat( flags, nTexCoordCount, texCoordDims, userDataSize );

			int nShadowFilterMode = 0;
			if ( bHasFlashlight )
			{
				nShadowFilterMode = g_pHardwareConfig->GetShadowFilterMode();	// Based upon vendor and device dependent formats
			}

			DECLARE_STATIC_VERTEX_SHADER( flesh_vs30 );
			SET_STATIC_VERTEX_SHADER( flesh_vs30 );

			// Pixel Shader
			if( /* g_pHardwareConfig->SupportsPixelShaders_3_0() */ true )
			{
				DECLARE_STATIC_PIXEL_SHADER( flesh_ps30 );
				SET_STATIC_PIXEL_SHADER_COMBO( ALPHABLEND, bAlphaBlend );
				SET_STATIC_PIXEL_SHADER_COMBO( TRANSMAT, bTransMatMasks );
				SET_STATIC_PIXEL_SHADER_COMBO( FRESNEL_WARP, bFresnelColorWarp );
				SET_STATIC_PIXEL_SHADER_COMBO( EFFECTS, bEffectMasks );
				SET_STATIC_PIXEL_SHADER_COMBO( TINTING, bColorWarp );
				SET_STATIC_PIXEL_SHADER_COMBO( IRIDESCENCE, bIridescentWarp );
				SET_STATIC_PIXEL_SHADER_COMBO( OPACITY_TEXTURE, bOpacityTexture );
				SET_STATIC_PIXEL_SHADER_COMBO( DETAIL, bDetail );
				SET_STATIC_PIXEL_SHADER_COMBO( NORMAL2SOFT, bNormal2 );
				SET_STATIC_PIXEL_SHADER_COMBO( INTERIOR_LAYER, bInteriorLayer );
				SET_STATIC_PIXEL_SHADER_COMBO( BACK_SCATTER, bBackScatter );
				SET_STATIC_PIXEL_SHADER_COMBO( FORWARD_SCATTER, bForwardScatter );
				SET_STATIC_PIXEL_SHADER_COMBO( HIGH_PRECISION_DEPTH, (g_pHardwareConfig->GetHDRType() == HDR_TYPE_FLOAT) ? true : false );
				SET_STATIC_PIXEL_SHADER_COMBO( FLASHLIGHTDEPTHFILTERMODE, nShadowFilterMode );
				SET_STATIC_PIXEL_SHADER( flesh_ps30 );
			}
			else
			{
				Assert( !"No ps_3_0" );
			}

			// Textures
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );		// [sRGB] Base
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );		//		 Bump
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER1, false );
			if( bInteriorLayer )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );		// [sRGB] Backbuffer
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER2, true );
			}
			if( bTransMatMasks )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );		//       Trans mat masks
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER3, false );
			}
			if( bColorWarp )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER4, true );		// [sRGB] Color Warp
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER4, true );
			}
			if( bFresnelColorWarp )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER5, true );		// [sRGB] Fresnel color warp (should be sRGB?)
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER5, true );
			}
			if( bOpacityTexture )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER6, true );		//		 Opacity
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER6, false );
			}
			if( bEffectMasks )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER10, true );		//		 Effect masks
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER10, false );
			}
			if( bIridescentWarp )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER11, true );		// [sRGB] Iridescent warp
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER11, true );
			}
			if( bDetail )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER12, true );		// [sRGB] Detail
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER12, true );
			}
			if ( bHasFlashlight )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER7, true );	//		 Shadow depth map
				pShaderShadow->SetShadowDepthFiltering( SHADER_SAMPLER7 );
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER7, false );
				pShaderShadow->EnableTexture( SHADER_SAMPLER8, true );	//		 Noise map
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER8, false );
				pShaderShadow->EnableTexture( SHADER_SAMPLER9, true );	//[sRGB] Flashlight cookie
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER9, true );

				// Flashlight passes - additive blending
				pShader->EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE );
				pShaderShadow->EnableAlphaWrites( false );
				pShaderShadow->EnableDepthWrites( false );
			}
			else if ( bAlphaBlend )
			{
				// Base pass - alpha blending (regular translucency)
				pShader->EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
				pShaderShadow->EnableAlphaWrites( false );	// TODO: write alpha for fog or not?
				pShaderShadow->EnableDepthWrites( true );	// Rely on depth-sorting
			}
			else
			{
				// Base pass - opaque blending (solid flesh or refractive translucency)
				pShader->DisableAlphaBlending();
				pShaderShadow->EnableAlphaWrites( true );
				pShaderShadow->EnableDepthWrites( true );
			}

			pShaderShadow->EnableSRGBWrite( true );

			// Per-instance state
			pShader->PI_BeginCommandBuffer();
			pShader->PI_SetVertexShaderAmbientLightCube();
			pShader->PI_SetPixelShaderAmbientLightCube( PSREG_AMBIENT_CUBE );
			pShader->PI_SetPixelShaderLocalLighting( PSREG_LIGHT_INFO_ARRAY );
			pShader->PI_EndCommandBuffer();
		}
		if ( pShaderAPI && ( (! pContextData ) || ( pContextData->m_bMaterialVarsChanged ) ) )
		{
			if ( !pContextData )								// make sure allocated
			{
				pContextData = new CFlesh_DX9_Context;
				*pContextDataPtr = pContextData;
			}
			pContextData->m_bMaterialVarsChanged = false;
			pContextData->m_SemiStaticCmdsOut.Reset();
			///////////////////////////
			// Semi-static block
			///////////////////////////
			float flConsts[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

			flConsts[0] = IS_PARAM_DEFINED( info.m_nBumpStrength ) ? params[info.m_nBumpStrength]->GetFloatValue() : kDefaultBumpStrength;
			flConsts[1] = (g_pHardwareConfig->GetHDRType() == HDR_TYPE_FLOAT) ? 8192.0f : 192.0f;	// destalpha dest scale factor. TODO: put this in its own const and call shaderAPI method to set
			flConsts[2] = IS_PARAM_DEFINED( info.m_nInteriorFogStrength ) ? params[info.m_nInteriorFogStrength]->GetFloatValue() : kDefaultInteriorFogStrength;
			flConsts[3] = IS_PARAM_DEFINED( info.m_nInteriorRefractStrength ) ? params[info.m_nInteriorRefractStrength]->GetFloatValue() : kDefaultInteriorRefractStrength;
			pContextData->m_SemiStaticCmdsOut.SetPixelShaderConstant( 0, flConsts, 1 );

			Assert( IS_PARAM_DEFINED( info.m_nFresnelParams ) );
			if ( IS_PARAM_DEFINED( info.m_nFresnelParams ) )
				params[info.m_nFresnelParams]->GetVecValue( flConsts, 3 );
			else
				memcpy( flConsts, kDefaultFresnelParams, sizeof( kDefaultFresnelParams ) );
			flConsts[3] = params[info.m_nInteriorBackgroundBoost]->GetFloatValue();
			pContextData->m_SemiStaticCmdsOut.SetPixelShaderConstant( 1, flConsts, 1 );

			flConsts[0] = IS_PARAM_DEFINED( info.m_nRimLightExp ) ? params[info.m_nRimLightExp]->GetFloatValue() : kDefaultRimLightExp;
			flConsts[1] = IS_PARAM_DEFINED( info.m_nRimLightScale ) ? params[info.m_nRimLightScale]->GetFloatValue() : kDefaultRimLightScale;
			flConsts[2] = IS_PARAM_DEFINED( info.m_nSpecScale ) ? params[info.m_nSpecScale]->GetFloatValue() : kDefaultSpecScale;
			flConsts[3] = IS_PARAM_DEFINED( info.m_nSpecExp2 ) ? params[info.m_nSpecExp2]->GetFloatValue() : kDefaultSpecExp;
			pContextData->m_SemiStaticCmdsOut.SetPixelShaderConstant( 3, flConsts, 1 );

			flConsts[0] = IS_PARAM_DEFINED( info.m_nSpecScale2 ) ? params[info.m_nSpecScale2]->GetFloatValue() : kDefaultSpecScale;
			flConsts[1] = IS_PARAM_DEFINED( info.m_nFresnelBumpStrength ) ? params[info.m_nFresnelBumpStrength]->GetFloatValue() : kDefaultFresnelBumpStrength;
			flConsts[2] = IS_PARAM_DEFINED( info.m_nDiffuseSoftNormal ) ? params[info.m_nDiffuseSoftNormal]->GetFloatValue() : kDefaultDiffuseSoftNormal;
			flConsts[3] = IS_PARAM_DEFINED( info.m_nInteriorAmbientScale ) ? params[info.m_nInteriorAmbientScale]->GetFloatValue() : kDefaultInteriorAmbientScale;
			pContextData->m_SemiStaticCmdsOut.SetPixelShaderConstant( 10, flConsts, 1 );

			// Depth alpha [ TODO: support fog ]
			bool bWriteDepthToAlpha = pShaderAPI->ShouldWriteDepthToDestAlpha() && !bAlphaBlend;
			if ( IS_PARAM_DEFINED( info.m_nSpecFresnel ) )
				params[info.m_nSpecFresnel]->GetVecValue( flConsts, 3 );
			else
				memcpy( flConsts, kDefaultSpecFresnel, sizeof( kDefaultSpecFresnel ) );
			flConsts[3] = bWriteDepthToAlpha ? 1.0f : 0.0f;
			pContextData->m_SemiStaticCmdsOut.SetPixelShaderConstant( 12, flConsts, 1 );

			if ( IS_PARAM_DEFINED( info.m_nPhongColorTint ) )
				params[info.m_nPhongColorTint]->GetVecValue( flConsts, 3 );
			else
				memcpy( flConsts, kDefaultPhongColorTint, sizeof( kDefaultPhongColorTint ) );
			flConsts[3] = IS_PARAM_DEFINED( info.m_nInteriorBackLightScale ) ? params[info.m_nInteriorBackLightScale]->GetFloatValue() : kDefaultInteriorBackLightScale;
			pContextData->m_SemiStaticCmdsOut.SetPixelShaderConstant( 19, flConsts, 1 );

			if( bIridescentWarp || bFresnelColorWarp )
			{
				flConsts[0] = ( IS_PARAM_DEFINED( info.m_nIridescenceBoost ) ) ? params[info.m_nIridescenceBoost]->GetFloatValue(): kDefaultIridescenceBoost;
				flConsts[1] = ( IS_PARAM_DEFINED( info.m_nIridescenceExponent ) ) ? params[info.m_nIridescenceExponent]->GetFloatValue(): kDefaultIridescenceExponent;
				flConsts[2] = ( IS_PARAM_DEFINED( info.m_nHueShiftIntensity ) ) ? params[info.m_nHueShiftIntensity]->GetFloatValue(): kDefaultHueShiftIntensity;
				flConsts[3] = ( IS_PARAM_DEFINED( info.m_nHueShiftFresnelExponent ) ) ? params[info.m_nHueShiftFresnelExponent]->GetFloatValue(): kDefaultHueShiftFresnelExponent;
				pContextData->m_SemiStaticCmdsOut.SetPixelShaderConstant( 26, flConsts, 1 );
			}

			if ( IS_PARAM_DEFINED( info.m_nSelfIllumTint ) )
				params[info.m_nSelfIllumTint]->GetVecValue( flConsts, 3 );
			else
				memcpy( flConsts, kDefaultSelfIllumTint, sizeof( kDefaultSelfIllumTint ) );
			float flDiffuseExp = IS_PARAM_DEFINED( info.m_nDiffuseExponent ) ? params[info.m_nDiffuseExponent]->GetFloatValue() : kDefaultDiffuseExponent;
			flConsts[3] = pow( 0.5f, flDiffuseExp );
			pContextData->m_SemiStaticCmdsOut.SetPixelShaderConstant( 27, flConsts, 1 );

			if ( IS_PARAM_DEFINED( info.m_nInteriorColor ) )
				params[info.m_nInteriorColor]->GetVecValue( flConsts, 3 );
			else
				memcpy( flConsts, kDefaultInteriorColor, sizeof( kDefaultInteriorColor ) );
			flConsts[3] = params[info.m_nInteriorRefractBlur]->GetFloatValue();
			pContextData->m_SemiStaticCmdsOut.SetPixelShaderConstant( 32, flConsts, 1 );

			if ( IS_PARAM_DEFINED( info.m_nSpecFresnel2 ) )
				params[info.m_nSpecFresnel2]->GetVecValue( flConsts, 3 );
			else
				memcpy( flConsts, kDefaultSpecFresnel2, sizeof( kDefaultSpecFresnel2 ) );
			flConsts[3] = IS_PARAM_DEFINED( info.m_nPhong2Softness ) ? params[info.m_nPhong2Softness]->GetFloatValue() : kDefaultPhong2Softness;
			pContextData->m_SemiStaticCmdsOut.SetPixelShaderConstant( 42, flConsts, 1 );

			flConsts[0] = flDiffuseExp;
			flConsts[1] = IS_PARAM_DEFINED( info.m_nNormal2Softness ) ? params[info.m_nNormal2Softness]->GetFloatValue() : kDefaultNormal2Softness;
			flConsts[2] = IS_PARAM_DEFINED( info.m_nAmbientBoost ) ? params[info.m_nAmbientBoost]->GetFloatValue() : kDefaultAmbientBoost;
			flConsts[3] = IS_PARAM_DEFINED( info.m_nAmbientBoostMaskMode ) ? params[info.m_nAmbientBoostMaskMode]->GetIntValue() : kDefaultAmbientBoostMaskMode;
			pContextData->m_SemiStaticCmdsOut.SetPixelShaderConstant( 43, flConsts, 1 );

			if ( bForwardScatter || bBackScatter )
			{
				flConsts[0] = IS_PARAM_DEFINED( info.m_nForwardScatter ) ? params[info.m_nForwardScatter]->GetFloatValue() : kDefaultForwardScatter;
				flConsts[1] = IS_PARAM_DEFINED( info.m_nBackScatter ) ? params[info.m_nBackScatter]->GetFloatValue() : kDefaultBackScatter;
				flConsts[2] = IS_PARAM_DEFINED( info.m_nSSDepth ) ? params[info.m_nSSDepth]->GetFloatValue() : kDefaultSSDepth;
				flConsts[3] = IS_PARAM_DEFINED( info.m_nSSBentNormalIntensity ) ? params[info.m_nSSBentNormalIntensity]->GetFloatValue() : kDefaultSSBentNormalIntensity;
				pContextData->m_SemiStaticCmdsOut.SetPixelShaderConstant( 44, flConsts, 1 );

				if( IS_PARAM_DEFINED( info.m_nSSColorTint ) )
					params[info.m_nSSColorTint]->GetVecValue( flConsts, 3 );
				else
					memcpy( flConsts, kDefaultSSColorTint, sizeof( kDefaultSSColorTint ) );
				flConsts[3] = IS_PARAM_DEFINED( info.m_nSSTintByAlbedo ) ? params[info.m_nSSTintByAlbedo]->GetFloatValue() : kDefaultSSTintByAlbedo;
				pContextData->m_SemiStaticCmdsOut.SetPixelShaderConstant( 45, flConsts, 1 );
			}

			flConsts[0] = 0.0f;
			flConsts[1] = 0.0f;
			if ( bDetail )
			{
				flConsts[0] = IS_PARAM_DEFINED( info.m_nDetailBlendMode ) ? params[info.m_nDetailBlendMode]->GetIntValue() : kDefaultDetailBlendMode;
				flConsts[1] = IS_PARAM_DEFINED( info.m_nDetailBlendFactor) ? params[info.m_nDetailBlendFactor]->GetFloatValue() : kDefaultDetailBlendFactor;
				flConsts[2] = 0.0f;
			}
			flConsts[3] = IS_PARAM_DEFINED( info.m_nSpecExp ) ? params[info.m_nSpecExp]->GetFloatValue() : kDefaultSpecExp;
			pContextData->m_SemiStaticCmdsOut.SetPixelShaderConstant( 46, flConsts, 1 );

			pContextData->m_SemiStaticCmdsOut.BindTexture( pShader, SHADER_SAMPLER0, BASETEXTURE, -1 );
			pContextData->m_SemiStaticCmdsOut.BindTexture( pShader, SHADER_SAMPLER1, info.m_nNormalMap, -1 );
			pContextData->m_SemiStaticCmdsOut.BindStandardTexture( SHADER_SAMPLER2, TEXTURE_FRAME_BUFFER_FULL_TEXTURE_0 ); // Refraction Map

			if ( bTransMatMasks )
			{
				pContextData->m_SemiStaticCmdsOut.BindTexture( pShader, SHADER_SAMPLER3, info.m_nTransMatMasksTexture, -1 );
			}

			if ( bColorWarp )
			{
				pContextData->m_SemiStaticCmdsOut.BindTexture( pShader, SHADER_SAMPLER4, info.m_nColorWarpTexture, -1 );
			}

			if ( bFresnelColorWarp )
			{
				pContextData->m_SemiStaticCmdsOut.BindTexture( pShader, SHADER_SAMPLER5, info.m_nFresnelColorWarpTexture, -1 );
			}

			if ( bOpacityTexture )
			{
				pContextData->m_SemiStaticCmdsOut.BindTexture( pShader, SHADER_SAMPLER6, info.m_nOpacityTexture, -1 );
			}

			if ( bEffectMasks )
			{
				pContextData->m_SemiStaticCmdsOut.BindTexture( pShader, SHADER_SAMPLER10, info.m_nEffectMasksTexture, -1 );
			}

			if ( bIridescentWarp )
			{
				pContextData->m_SemiStaticCmdsOut.BindTexture( pShader, SHADER_SAMPLER11, info.m_nIridescentWarpTexture, -1 );
			}

			// VS consts
			flConsts[0] = IS_PARAM_DEFINED( info.m_nUVScale ) ? params[info.m_nUVScale]->GetFloatValue() : kDefaultUVScale;
			pContextData->m_SemiStaticCmdsOut.SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, flConsts, 1 );

			flConsts[0] = IS_PARAM_DEFINED( info.m_nDetailScale ) ? params[info.m_nDetailScale]->GetFloatValue() : kDefaultDetailScale;
			pContextData->m_SemiStaticCmdsOut.SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_1, flConsts, 1 );

			if ( IS_PARAM_DEFINED( info.m_nDetailTextureTransform ) )
				pContextData->m_SemiStaticCmdsOut.SetVertexShaderTextureScaledTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_2, info.m_nDetailTextureTransform, info.m_nDetailScale );
			else
				pContextData->m_SemiStaticCmdsOut.SetVertexShaderTextureScaledTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_2, info.m_nBaseTextureTransform, info.m_nDetailScale );

			pContextData->m_SemiStaticCmdsOut.End();
			// end semi-static block
		}
	}

	if ( pShaderAPI ) //DYNAMIC_STATE
	{
		CCommandBufferBuilder< CFixedCommandStorageBuffer< 400 > > DynamicCmdsOut;
		DynamicCmdsOut.Call( pContextData->m_SemiStaticCmdsOut.Base() );
		///////////////////////////
		// dynamic block
		///////////////////////////

		float camPos[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		pShaderAPI->GetWorldSpaceCameraPosition( camPos );
		DynamicCmdsOut.SetPixelShaderConstant( PSREG_EYEPOS_SPEC_EXPONENT, camPos );

		if ( bDetail )
		{
			DynamicCmdsOut.BindTexture( pShader, SHADER_SAMPLER12, info.m_nDetailTexture, info.m_nDetailFrame );
		}

		float mView[16];
		pShaderAPI->GetMatrix( MATERIAL_VIEW, mView );
		DynamicCmdsOut.SetPixelShaderConstant( 33, mView, 3 );

		DynamicCmdsOut.SetPixelShaderFogParams( 36 );

		LightState_t lightState = { 0, false, false };
		pShaderAPI->GetDX9LightState( &lightState );

		// flashlightfixme: put this in common code.
		bool bFlashlightShadows = false;
		if ( bHasFlashlight )
		{
			Assert( info.m_nFlashlightTexture >= 0 && info.m_nFlashlightTextureFrame >= 0 );
			VMatrix worldToTexture;
			ITexture *pFlashlightDepthTexture;
			FlashlightState_t state = pShaderAPI->GetFlashlightStateEx( worldToTexture, &pFlashlightDepthTexture );
			DynamicCmdsOut.BindTexture( pShader, SHADER_SAMPLER9, state.m_pSpotlightTexture, state.m_nSpotlightTextureFrame );
			bFlashlightShadows = state.m_bEnableShadows;

			SetFlashLightColorFromState( state, pShaderAPI, PSREG_FLASHLIGHT_COLOR );

			if( pFlashlightDepthTexture && g_pConfig->ShadowDepthTexture() && state.m_bEnableShadows )
			{
				DynamicCmdsOut.BindTexture( pShader, SHADER_SAMPLER7, pFlashlightDepthTexture, -1 );
				DynamicCmdsOut.BindStandardTexture( SHADER_SAMPLER8, TEXTURE_SHADOW_NOISE_2D );
			}

			float atten[4], pos[4], tweaks[4];

			atten[0] = state.m_fConstantAtten;		// Set the flashlight attenuation factors
			atten[1] = state.m_fLinearAtten;
			atten[2] = state.m_fQuadraticAtten;
			atten[3] = state.m_FarZAtten;
			DynamicCmdsOut.SetPixelShaderConstant( PSREG_FLASHLIGHT_ATTENUATION, atten, 1 );

			pos[0] = state.m_vecLightOrigin[0];		// Set the flashlight origin
			pos[1] = state.m_vecLightOrigin[1];
			pos[2] = state.m_vecLightOrigin[2];
			pos[3] = state.m_FarZ;

			DynamicCmdsOut.SetPixelShaderConstant( PSREG_FLASHLIGHT_POSITION_RIM_BOOST, pos, 1 );	// steps on rim boost
			DynamicCmdsOut.SetPixelShaderConstant( PSREG_FLASHLIGHT_TO_WORLD_TEXTURE, worldToTexture.Base(), 4 );

			// Tweaks associated with a given flashlight
			tweaks[0] = ShadowFilterFromState( state );
			tweaks[1] = ShadowAttenFromState( state );
			pShader->HashShadow2DJitter( state.m_flShadowJitterSeed, &tweaks[2], &tweaks[3] );
			DynamicCmdsOut.SetPixelShaderConstant( PSREG_ENVMAP_TINT__SHADOW_TWEAKS, tweaks, 1 );

			// Dimensions of screen, used for screen-space noise map sampling
			float vScreenScale[4] = {1280.0f / 32.0f, 720.0f / 32.0f, 0, 0};
			int nWidth, nHeight;
			pShaderAPI->GetBackBufferDimensions( nWidth, nHeight );

			int nTexWidth, nTexHeight;
			pShaderAPI->GetStandardTextureDimensions( &nTexWidth, &nTexHeight, TEXTURE_SHADOW_NOISE_2D );

			vScreenScale[0] = (float) nWidth  / nTexWidth;
			vScreenScale[1] = (float) nHeight / nTexHeight;

			DynamicCmdsOut.SetPixelShaderConstant( PSREG_FLASHLIGHT_SCREEN_SCALE, vScreenScale, 1 );
		}
		DynamicCmdsOut.End();

		// end dynamic block
		pShaderAPI->ExecuteCommandBuffer( DynamicCmdsOut.Base() );

		// Set Vertex Shader Combos
		DECLARE_DYNAMIC_VERTEX_SHADER( flesh_vs30 );
		SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, pShaderAPI->GetCurrentNumBones() > 0 );
		SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
		SET_DYNAMIC_VERTEX_SHADER( flesh_vs30 );
		
		// Set Pixel Shader Combos
		if( /*g_pHardwareConfig->SupportsPixelShaders_2_b()*/ true )
		{
			DECLARE_DYNAMIC_PIXEL_SHADER( flesh_ps30 );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( NUM_LIGHTS, lightState.m_nNumLights );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( FLASHLIGHT, bHasFlashlight );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( FLASHLIGHTSHADOWS, bFlashlightShadows );
			SET_DYNAMIC_PIXEL_SHADER( flesh_ps30 );
		}
		else
		{
			Assert( !"No ps_3_0" );
		}
	}
	pShader->Draw();
}
