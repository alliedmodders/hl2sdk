//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Lightmap only shader
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "lightmappedgeneric_dx9_helper.h"
#include "BaseVSShader.h"
#include "convar.h"
#include "SDK_lightmappedgeneric_ps20.inc"
#include "SDK_lightmappedgeneric_vs20.inc"


// NOTE: The only reason this is not enabled is because I discovered this bug 10 days before RC1
// #define SRGB_ENABLED 1

ConVar mat_disable_lightwarp( "mat_disable_lightwarp", "0" );
ConVar mat_disable_fancy_blending( "mat_disable_fancy_blending", "0" );

void InitParamsLightmappedGeneric_DX9( CBaseVSShader *pShader, IMaterialVar** params, const char *pMaterialName, LightmappedGeneric_DX9_Vars_t &info )
{
	// FLASHLIGHTFIXME
	params[info.m_nFlashlightTexture]->SetStringValue( "effects/flashlight001" );

	// Write over $basetexture with $albedo if we are going to be using diffuse normal mapping.
	if( g_pConfig->UseBumpmapping() && params[info.m_nBumpmap]->IsDefined() && params[info.m_nAlbedo]->IsDefined() &&
		params[info.m_nBaseTexture]->IsDefined() && 
		!( params[info.m_nNoDiffuseBumpLighting]->IsDefined() && params[info.m_nNoDiffuseBumpLighting]->GetIntValue() ) )
	{
		params[info.m_nBaseTexture]->SetStringValue( params[info.m_nAlbedo]->GetStringValue() );
	}

	if( pShader->IsUsingGraphics() && params[info.m_nEnvmap]->IsDefined() && !pShader->CanUseEditorMaterials() )
	{
		if( stricmp( params[info.m_nEnvmap]->GetStringValue(), "env_cubemap" ) == 0 )
		{
			Warning( "env_cubemap used on world geometry without rebuilding map. . ignoring: %s\n", pMaterialName );
			params[info.m_nEnvmap]->SetUndefined();
		}
	}
	
	if ( (mat_disable_lightwarp.GetBool() ) &&
		 (info.m_nLightWarpTexture != -1) )
	{
		params[info.m_nLightWarpTexture]->SetUndefined();
	}
	if ( (mat_disable_fancy_blending.GetBool() ) &&
		 (info.m_nBlendModulateTexture != -1) )
	{
		params[info.m_nBlendModulateTexture]->SetUndefined();
	}

	if( !params[info.m_nEnvmapTint]->IsDefined() )
		params[info.m_nEnvmapTint]->SetVecValue( 1.0f, 1.0f, 1.0f );

	if( !params[info.m_nNoDiffuseBumpLighting]->IsDefined() )
		params[info.m_nNoDiffuseBumpLighting]->SetIntValue( 0 );

	if( !params[info.m_nSelfIllumTint]->IsDefined() )
		params[info.m_nSelfIllumTint]->SetVecValue( 1.0f, 1.0f, 1.0f );

	if( !params[info.m_nDetailScale]->IsDefined() )
		params[info.m_nDetailScale]->SetFloatValue( 4.0f );

	if( !params[info.m_nFresnelReflection]->IsDefined() )
		params[info.m_nFresnelReflection]->SetFloatValue( 1.0f );

	if( !params[info.m_nEnvmapMaskFrame]->IsDefined() )
		params[info.m_nEnvmapMaskFrame]->SetIntValue( 0 );
	
	if( !params[info.m_nEnvmapFrame]->IsDefined() )
		params[info.m_nEnvmapFrame]->SetIntValue( 0 );

	if( !params[info.m_nBumpFrame]->IsDefined() )
		params[info.m_nBumpFrame]->SetIntValue( 0 );

	if( !params[info.m_nDetailFrame]->IsDefined() )
		params[info.m_nDetailFrame]->SetIntValue( 0 );

	if( !params[info.m_nEnvmapContrast]->IsDefined() )
		params[info.m_nEnvmapContrast]->SetFloatValue( 0.0f );
	
	if( !params[info.m_nEnvmapSaturation]->IsDefined() )
		params[info.m_nEnvmapSaturation]->SetFloatValue( 1.0f );
	
	// No texture means no self-illum or env mask in base alpha
	if ( !params[info.m_nBaseTexture]->IsDefined() )
	{
		CLEAR_FLAGS( MATERIAL_VAR_SELFILLUM );
		CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
	}

	if( params[info.m_nBumpmap]->IsDefined() )
	{
		params[info.m_nEnvmapMask]->SetUndefined();
	}
	
	// If in decal mode, no debug override...
	if (IS_FLAG_SET(MATERIAL_VAR_DECAL))
	{
		SET_FLAGS( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
	}

	SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
	if( g_pConfig->UseBumpmapping() && params[info.m_nBumpmap]->IsDefined() && (params[info.m_nNoDiffuseBumpLighting]->GetIntValue() == 0) )
	{
		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_BUMPED_LIGHTMAP );
	}

	// If mat_specular 0, then get rid of envmap
	if( !g_pConfig->UseSpecular() && params[info.m_nEnvmap]->IsDefined() && params[info.m_nBaseTexture]->IsDefined() )
	{
		params[info.m_nEnvmap]->SetUndefined();
	}

	if( !params[info.m_nBaseTextureNoEnvmap]->IsDefined() )
	{
		params[info.m_nBaseTextureNoEnvmap]->SetIntValue( 0 );
	}
	if( !params[info.m_nBaseTexture2NoEnvmap]->IsDefined() )
	{
		params[info.m_nBaseTexture2NoEnvmap]->SetIntValue( 0 );
	}
}

void InitLightmappedGeneric_DX9( CBaseVSShader *pShader, IMaterialVar** params, LightmappedGeneric_DX9_Vars_t &info )
{
	if( g_pConfig->UseBumpmapping() && params[info.m_nBumpmap]->IsDefined() )
	{
		pShader->LoadBumpMap( info.m_nBumpmap );
	}
	
	if( g_pConfig->UseBumpmapping() && params[info.m_nBumpmap2]->IsDefined() )
	{
		pShader->LoadBumpMap( info.m_nBumpmap2 );
	}

	if (params[info.m_nBaseTexture]->IsDefined())
	{
		pShader->LoadTexture( info.m_nBaseTexture );

		if (!params[info.m_nBaseTexture]->GetTextureValue()->IsTranslucent())
		{
			CLEAR_FLAGS( MATERIAL_VAR_SELFILLUM );
			CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
		}
	}

	if (params[info.m_nBaseTexture2]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nBaseTexture2 );
	}

	if (params[info.m_nLightWarpTexture]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nLightWarpTexture );
	}
	if ((info.m_nBlendModulateTexture != -1) &&
		(params[info.m_nBlendModulateTexture]->IsDefined()) )
	{
		pShader->LoadTexture( info.m_nBlendModulateTexture );
	}

	if (params[info.m_nDetail]->IsDefined())
	{
		pShader->LoadTexture( info.m_nDetail );
	}

	pShader->LoadTexture( info.m_nFlashlightTexture );
	
	// Don't alpha test if the alpha channel is used for other purposes
	if (IS_FLAG_SET(MATERIAL_VAR_SELFILLUM) || IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK) )
	{
		CLEAR_FLAGS( MATERIAL_VAR_ALPHATEST );
	}
		
	if (params[info.m_nEnvmap]->IsDefined())
	{
		if( !IS_FLAG_SET(MATERIAL_VAR_ENVMAPSPHERE) )
		{
			pShader->LoadCubeMap( info.m_nEnvmap );
		}
		else
		{
			pShader->LoadTexture( info.m_nEnvmap );
		}

		if( !g_pHardwareConfig->SupportsCubeMaps() )
		{
			SET_FLAGS( MATERIAL_VAR_ENVMAPSPHERE );
		}

		if (params[info.m_nEnvmapMask]->IsDefined())
		{
			pShader->LoadTexture( info.m_nEnvmapMask );
		}
	}
	else
	{
		params[info.m_nEnvmapMask]->SetUndefined();
	}

	// We always need this because of the flashlight.
	SET_FLAGS2( MATERIAL_VAR2_NEEDS_TANGENT_SPACES );
}

void DrawLightmappedGeneric_DX9( CBaseVSShader *pShader, IMaterialVar** params, 
	IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow, LightmappedGeneric_DX9_Vars_t &info )
{
	bool hasBump = params[info.m_nBumpmap]->IsTexture();
	bool hasBump2 = hasBump && params[info.m_nBumpmap2]->IsTexture();
	bool hasDiffuseBumpmap = hasBump && (params[info.m_nNoDiffuseBumpLighting]->GetIntValue() == 0);
	bool hasBaseTexture = params[info.m_nBaseTexture]->IsTexture();
	bool hasBaseTexture2 = hasBaseTexture && params[info.m_nBaseTexture2]->IsTexture();
	bool hasLightWarpTexture = params[info.m_nLightWarpTexture]->IsTexture();
	bool hasDetailTexture = params[info.m_nDetail]->IsTexture();
	bool hasVertexColor = IS_FLAG_SET( MATERIAL_VAR_VERTEXCOLOR ) != 0;
	bool bIsAlphaTested = IS_FLAG_SET( MATERIAL_VAR_ALPHATEST ) != 0;
	bool bHasBlendModulateTexture = 
		(info.m_nBlendModulateTexture != -1) &&
		(params[info.m_nBlendModulateTexture]->IsTexture() );

	int nAlphaChannelTextureVar = hasBaseTexture ? (int)info.m_nBaseTexture : (int)info.m_nEnvmapMask;
	BlendType_t nBlendType = pShader->EvaluateBlendRequirements( nAlphaChannelTextureVar, hasBaseTexture );

	bool hasFlashlight = pShader->UsingFlashlight( params );
	if( hasFlashlight )
	{
		CBaseVSShader::DrawFlashlight_dx90_Vars_t vars;
		vars.m_bBump = hasBump;
		vars.m_nBumpmapVar = info.m_nBumpmap;
		vars.m_nBumpmapFrame = info.m_nBumpFrame;
		vars.m_nBumpTransform = info.m_nBumpTransform;
		vars.m_nFlashlightTextureVar = info.m_nFlashlightTexture;
		vars.m_nFlashlightTextureFrameVar = info.m_nFlashlightTextureFrame;
		vars.m_bLightmappedGeneric = true;
		vars.m_bWorldVertexTransition = hasBaseTexture2;
		vars.m_nBaseTexture2Var = info.m_nBaseTexture2;
		vars.m_nBaseTexture2FrameVar = info.m_nBaseTexture2Frame;
		vars.m_nBumpmap2Var = info.m_nBumpmap2;
		vars.m_nBumpmap2Frame = info.m_nBumpFrame2;
		pShader->DrawFlashlight_dx90( params, pShaderAPI, pShaderShadow, vars );
		return;
	}

	SHADOW_STATE
	{
		bool hasEnvmap = params[info.m_nEnvmap]->IsTexture();
		bool hasEnvmapMask = params[info.m_nEnvmapMask]->IsTexture();
		// Alpha test: FIXME: shouldn't this be handled in Shader_t::SetInitialShadowState
		pShaderShadow->EnableAlphaTest( bIsAlphaTested );
		pShader->SetDefaultBlendingShadowState( nAlphaChannelTextureVar, hasBaseTexture );

		unsigned int flags = VERTEX_POSITION;
		if( hasBaseTexture )
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
#ifdef SRGB_ENABLED
			pShaderShadow->EnableSRGBRead( SHADER_TEXTURE_STAGE0, true );
#else
			if( g_pHardwareConfig->GetHDRType() != HDR_TYPE_NONE )
			{
				pShaderShadow->EnableSRGBRead( SHADER_TEXTURE_STAGE0, true );
			}
#endif
		}
		if (hasLightWarpTexture)
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE6, true );
			pShaderShadow->EnableSRGBRead( SHADER_TEXTURE_STAGE6, false );
		}
		if ( bHasBlendModulateTexture )
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE3, true );
			pShaderShadow->EnableSRGBRead( SHADER_TEXTURE_STAGE3, false );
		}

		if( hasBaseTexture2 )
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE7, true );
#ifdef SRGB_ENABLED
			pShaderShadow->EnableSRGBRead( SHADER_TEXTURE_STAGE7, true );
#else
			if( g_pHardwareConfig->GetHDRType() != HDR_TYPE_NONE )
			{
				pShaderShadow->EnableSRGBRead( SHADER_TEXTURE_STAGE7, true );
			}
#endif
		}
//			if( hasLightmap )
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
#ifdef SRGB_ENABLED
			if( g_pHardwareConfig->GetHDRType() == HDR_TYPE_NONE )
			{
				pShaderShadow->EnableSRGBRead( SHADER_TEXTURE_STAGE1, true );
			}
#endif
		}
		if( hasEnvmap )
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE2, true );
#ifdef SRGB_ENABLED
			if( g_pHardwareConfig->GetHDRType() == HDR_TYPE_NONE )
			{
				pShaderShadow->EnableSRGBRead( SHADER_TEXTURE_STAGE2, true );
			}
#endif
			flags |= VERTEX_TANGENT_S | VERTEX_TANGENT_T | VERTEX_NORMAL;
		}
		if( hasDetailTexture )
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE3, true );
		}
		if( hasBump )
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE4, true );
		}
		if( hasBump2 )
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE5, true );
		}
		if( hasEnvmapMask )
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE5, true );
		}
		if( hasVertexColor || hasBaseTexture2 )
		{
			flags |= VERTEX_COLOR;
		}

		// Normalizing cube map
		pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE6, true );

		// texcoord0 : base texcoord
		// texcoord1 : lightmap texcoord
		// texcoord2 : lightmap texcoord offset
		int numTexCoords = 0;
		if( hasBump )
		{
			numTexCoords = 3;
		}
		else 
		{
			numTexCoords = 2;
		}
		
		pShaderShadow->VertexShaderVertexFormat( 
			flags, numTexCoords, 0, 0, 0 );

		// Pre-cache pixel shaders
		bool hasBaseAlphaEnvmapMask = IS_FLAG_SET( MATERIAL_VAR_BASEALPHAENVMAPMASK );
		bool hasSelfIllum = IS_FLAG_SET( MATERIAL_VAR_SELFILLUM );
		bool hasNormalMapAlphaEnvmapMask = IS_FLAG_SET( MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK );

		DECLARE_STATIC_VERTEX_SHADER( sdk_lightmappedgeneric_vs20 );
		SET_STATIC_VERTEX_SHADER_COMBO( ENVMAP_MASK,  hasEnvmapMask );
		SET_STATIC_VERTEX_SHADER_COMBO( TANGENTSPACE,  params[info.m_nEnvmap]->IsTexture() );
		SET_STATIC_VERTEX_SHADER_COMBO( BUMPMAP,  hasBump );
		SET_STATIC_VERTEX_SHADER_COMBO( DIFFUSEBUMPMAP, hasDiffuseBumpmap );
		SET_STATIC_VERTEX_SHADER_COMBO( VERTEXCOLOR, IS_FLAG_SET( MATERIAL_VAR_VERTEXCOLOR ) );
		SET_STATIC_VERTEX_SHADER_COMBO( VERTEXALPHATEXBLENDFACTOR, hasBaseTexture2 || hasBump2 );

		DECLARE_STATIC_PIXEL_SHADER( sdk_lightmappedgeneric_ps20 );
		SET_STATIC_PIXEL_SHADER_COMBO( BASETEXTURE2, hasBaseTexture2 );
		SET_STATIC_PIXEL_SHADER_COMBO( DETAILTEXTURE, hasDetailTexture );
		SET_STATIC_PIXEL_SHADER_COMBO( BUMPMAP,  hasBump );
		SET_STATIC_PIXEL_SHADER_COMBO( BUMPMAP2, hasBump2 );
		SET_STATIC_PIXEL_SHADER_COMBO( DIFFUSEBUMPMAP,  hasDiffuseBumpmap );
		SET_STATIC_PIXEL_SHADER_COMBO( CUBEMAP,  hasEnvmap );
		SET_STATIC_PIXEL_SHADER_COMBO( ENVMAPMASK,  hasEnvmapMask );
		SET_STATIC_PIXEL_SHADER_COMBO( BASEALPHAENVMAPMASK,  hasBaseAlphaEnvmapMask );
		SET_STATIC_PIXEL_SHADER_COMBO( SELFILLUM,  hasSelfIllum );
		SET_STATIC_PIXEL_SHADER_COMBO( NORMALMAPALPHAENVMAPMASK,  hasNormalMapAlphaEnvmapMask );
		SET_STATIC_PIXEL_SHADER_COMBO( BASETEXTURENOENVMAP,  params[info.m_nBaseTextureNoEnvmap]->GetIntValue() );
		SET_STATIC_PIXEL_SHADER_COMBO( BASETEXTURE2NOENVMAP, params[info.m_nBaseTexture2NoEnvmap]->GetIntValue() );
		SET_STATIC_PIXEL_SHADER_COMBO( WARPLIGHTING, hasLightWarpTexture );
		SET_STATIC_PIXEL_SHADER_COMBO( FANCY_BLENDING, bHasBlendModulateTexture );

		bool bMaskedBlending=( (info.m_nMaskedBlending != -1) &&
							  (params[info.m_nMaskedBlending]->GetIntValue() != 0) );

		SET_STATIC_PIXEL_SHADER_COMBO( MASKEDBLENDING, bMaskedBlending);
		SET_STATIC_PIXEL_SHADER( sdk_lightmappedgeneric_ps20 );

		// HACK HACK HACK - enable alpha writes all the time so that we have them for
		// underwater stuff. 
		// But only do it if we're not using the alpha already for translucency
		if( nBlendType != BT_BLENDADD && nBlendType != BT_BLEND && !bIsAlphaTested )
		{
			pShaderShadow->EnableAlphaWrites( true );
		}

		SET_STATIC_VERTEX_SHADER( sdk_lightmappedgeneric_vs20 );

#ifdef SRGB_ENABLED
		pShaderShadow->EnableSRGBWrite( true );
#else
		if( g_pHardwareConfig->GetHDRType() != HDR_TYPE_NONE )
		{
			pShaderShadow->EnableSRGBWrite( true );
		}
#endif		
		pShader->DefaultFog();
	}
	DYNAMIC_STATE
	{
		bool hasEnvmap = params[info.m_nEnvmap]->IsTexture();
		bool hasEnvmapMask = params[info.m_nEnvmapMask]->IsTexture();
		if( hasBaseTexture )
		{
			pShader->BindTexture( SHADER_TEXTURE_STAGE0, info.m_nBaseTexture, info.m_nBaseTextureFrame );
		}
		else
		{
			pShaderAPI->BindWhite( SHADER_TEXTURE_STAGE0 );
		}

		if( hasBaseTexture2 )
		{
			pShader->BindTexture( SHADER_TEXTURE_STAGE7, info.m_nBaseTexture2, info.m_nBaseTexture2Frame );
		}
//			if( hasLightmap )
		{
			pShaderAPI->BindLightmap( SHADER_TEXTURE_STAGE1 );
		}
		if( hasEnvmap )
		{
			pShader->BindTexture( SHADER_TEXTURE_STAGE2, info.m_nEnvmap, info.m_nEnvmapFrame );
		}
		if( hasDetailTexture )
		{
			pShader->BindTexture( SHADER_TEXTURE_STAGE3, info.m_nDetail, info.m_nDetailFrame );
		}
		if( hasBump )
		{
			if( !g_pConfig->m_bFastNoBump )
			{
				pShader->BindTexture( SHADER_TEXTURE_STAGE4, info.m_nBumpmap, info.m_nBumpFrame );
			}
			else
			{
				pShaderAPI->BindFlatNormalMap( SHADER_TEXTURE_STAGE4 );
			}
		}
		if( hasBump2 )
		{
			if( !g_pConfig->m_bFastNoBump )
			{
				pShader->BindTexture( SHADER_TEXTURE_STAGE5, info.m_nBumpmap2, info.m_nBumpFrame2 );
			}
			else
			{
				pShaderAPI->BindFlatNormalMap( SHADER_TEXTURE_STAGE4 );
			}
		}
		if( hasEnvmapMask )
		{
			pShader->BindTexture( SHADER_TEXTURE_STAGE5, info.m_nEnvmapMask, info.m_nEnvmapMaskFrame );
		}

		if ( hasLightWarpTexture )
		{
			pShader->BindTexture( SHADER_TEXTURE_STAGE6, info.m_nLightWarpTexture, 0 );
		}
		else
			pShaderAPI->BindSignedNormalizationCubeMap( SHADER_TEXTURE_STAGE6 );

		if ( bHasBlendModulateTexture )
		{
			pShader->BindTexture( SHADER_TEXTURE_STAGE3, info.m_nBlendModulateTexture, 0 );
		}
		// If we don't have a texture transform, we don't have
		// to set vertex shader constants or run vertex shader instructions
		// for the texture transform.
		bool bHasTextureTransform = 
			!( params[info.m_nBaseTextureTransform]->MatrixIsIdentity() &&
			   params[info.m_nBumpTransform]->MatrixIsIdentity() &&
			   params[info.m_nEnvmapMaskTransform]->MatrixIsIdentity() );

		bool bHasBlendMaskTransform= (
			(info.m_nBlendMaskTransform != -1) &&
			(info.m_nMaskedBlending != -1) &&
			(params[info.m_nMaskedBlending]->GetIntValue() ) &&
			( ! (params[info.m_nBumpTransform]->MatrixIsIdentity() ) ) );

		bHasTextureTransform |= bHasBlendMaskTransform;
		
		bool bVertexShaderFastPath = !bHasTextureTransform;
		if( pShaderAPI->GetIntRenderingParameter(INT_RENDERPARM_ENABLE_FIXED_LIGHTING)!=0 )
		{
			bVertexShaderFastPath = false;
		}
		if( params[info.m_nDetail]->IsTexture() )
		{
			bVertexShaderFastPath = false;
		}

		if (bHasBlendMaskTransform)
		{
			pShader->SetVertexShaderTextureTransform( 
				VERTEX_SHADER_SHADER_SPECIFIC_CONST_6, info.m_nBlendMaskTransform );
		}

		float color[4] = { 1.0, 1.0, 1.0, 1.0 };
		pShader->ComputeModulationColor( color );
		if( !( bVertexShaderFastPath && color[0] == 1.0f && color[1] == 1.0f && color[2] == 1.0f && color[3] == 1.0f ) )
		{
			bVertexShaderFastPath = false;
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_MODULATION_COLOR, color );
			pShader->SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, info.m_nBaseTextureTransform );
			// If we have a detail texture, then the bump texcoords are the same as the base texcoords.
			if( hasBump && !hasDetailTexture )
			{
				pShader->SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_2, info.m_nBumpTransform );
			}
			if( hasEnvmapMask )
			{
				pShader->SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_4, info.m_nEnvmapMaskTransform );
			}
		}

		MaterialFogMode_t fogType = pShaderAPI->GetSceneFogMode();
		DECLARE_DYNAMIC_VERTEX_SHADER( sdk_lightmappedgeneric_vs20 );
		SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG,  fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
		SET_DYNAMIC_VERTEX_SHADER_COMBO( FASTPATH,  bVertexShaderFastPath );
		SET_DYNAMIC_VERTEX_SHADER_COMBO( 
			LIGHTING_PREVIEW, 
			(pShaderAPI->GetIntRenderingParameter(INT_RENDERPARM_ENABLE_FIXED_LIGHTING))?1:0
			);
		SET_DYNAMIC_VERTEX_SHADER( sdk_lightmappedgeneric_vs20 );

		bool bPixelShaderFastPath = true;
		if( pShaderAPI->GetIntRenderingParameter(INT_RENDERPARM_ENABLE_FIXED_LIGHTING)!=0 )
		{
			bPixelShaderFastPath = false;
		}
		float envmapTintVal[4];
		float selfIllumTintVal[4];
		params[info.m_nEnvmapTint]->GetVecValue( envmapTintVal, 3 );
		params[info.m_nSelfIllumTint]->GetVecValue( selfIllumTintVal, 3 );
		float envmapContrast = params[info.m_nEnvmapContrast]->GetFloatValue();
		float envmapSaturation = params[info.m_nEnvmapSaturation]->GetFloatValue();
		float fresnelReflection = params[info.m_nFresnelReflection]->GetFloatValue();

#ifdef _DEBUG
		float envmapTintOverride = mat_envmaptintoverride.GetFloat();
		float envmapTintScaleOverride = mat_envmaptintscale.GetFloat();

		if( envmapTintOverride != -1.0f )
		{
			envmapTintVal[0] = envmapTintVal[1] = envmapTintVal[2] = envmapTintOverride;
		}
		if( envmapTintScaleOverride != -1.0f )
		{
			envmapTintVal[0] *= envmapTintScaleOverride;
			envmapTintVal[1] *= envmapTintScaleOverride;
			envmapTintVal[2] *= envmapTintScaleOverride;
		}
#endif

		bool bUsingContrast = hasEnvmap && ( (envmapContrast != 0.0f) && (envmapContrast != 1.0f) ) && (envmapSaturation != 1.0f);
		bool bUsingFresnel = hasEnvmap && (fresnelReflection != 1.0f);
		bool bUsingSelfIllumTint = IS_FLAG_SET(MATERIAL_VAR_SELFILLUM) && (selfIllumTintVal[0] != 1.0f || selfIllumTintVal[1] != 1.0f || selfIllumTintVal[2] != 1.0f); 
		bool bUsingEnvMapTint = hasEnvmap && (envmapTintVal[0] != 1.0f || envmapTintVal[1] != 1.0f || envmapTintVal[2] != 1.0f); 
		if ( bUsingContrast || bUsingFresnel || bUsingSelfIllumTint || !g_pConfig->bShowSpecular )
		{
			bPixelShaderFastPath = false;
		}
		
		DECLARE_DYNAMIC_PIXEL_SHADER( sdk_lightmappedgeneric_ps20 );
		SET_DYNAMIC_PIXEL_SHADER_COMBO( FASTPATH,  bPixelShaderFastPath );
		SET_DYNAMIC_PIXEL_SHADER_COMBO( FASTPATHENVMAPTINT,  bPixelShaderFastPath && bUsingEnvMapTint );
		SET_DYNAMIC_PIXEL_SHADER_COMBO( FASTPATHENVMAPCONTRAST,  bPixelShaderFastPath && envmapContrast == 1.0f );
		SET_DYNAMIC_PIXEL_SHADER_COMBO( FOGTYPE, pShaderAPI->GetSceneFogMode() );
		// Don't write fog to alpha if we're using translucency
		SET_DYNAMIC_PIXEL_SHADER_COMBO( WRITEWATERFOGTODESTALPHA,  (fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z) && 
			(nBlendType != BT_BLENDADD) && (nBlendType != BT_BLEND) && !bIsAlphaTested );
		SET_DYNAMIC_PIXEL_SHADER_COMBO(
			LIGHTING_PREVIEW, pShaderAPI->GetIntRenderingParameter(INT_RENDERPARM_ENABLE_FIXED_LIGHTING));

		SET_DYNAMIC_PIXEL_SHADER( sdk_lightmappedgeneric_ps20 );
		
		// always set the transform for detail textures since I'm assuming that you'll
		// always have a detailscale.
		if( hasDetailTexture )
		{
			pShader->SetVertexShaderTextureScaledTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_2, info.m_nBaseTextureTransform, info.m_nDetailScale );
		}
		
		if ( !bPixelShaderFastPath || bUsingEnvMapTint )
		{
			pShader->SetEnvMapTintPixelShaderDynamicState( 0, info.m_nEnvmapTint, -1 );
		}

		if( !bPixelShaderFastPath )
		{
			pShader->SetPixelShaderConstant( 2, info.m_nEnvmapContrast );
			pShader->SetPixelShaderConstant( 3, info.m_nEnvmapSaturation );
			pShader->SetPixelShaderConstant( 7, info.m_nSelfIllumTint );

			// [ 0, 0 ,0, R(0) ]
			float fresnel[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			fresnel[3] = params[info.m_nFresnelReflection]->GetFloatValue();
			pShaderAPI->SetPixelShaderConstant( 4, fresnel );

			// [ 0, 0 ,0, 1-R(0) ]
			fresnel[3] = 1.0f - fresnel[3];
			pShaderAPI->SetPixelShaderConstant( 5, fresnel );
		}

		float eyePos[4];
		pShaderAPI->GetWorldSpaceCameraPosition( eyePos );
		pShaderAPI->SetPixelShaderConstant( 10, eyePos, 1 );
		pShaderAPI->SetPixelShaderFogParams( 11 );

		if( fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z )
		{
			if( g_pHardwareConfig->GetHDRType() != HDR_TYPE_NONE )
			{
				pShader->SetWaterFogColorPixelShaderConstantLinear( 14 );
			}
			else
			{
				pShader->SetWaterFogColorPixelShaderConstantGamma( 14 );
			}
		}
	}
	pShader->Draw();
}
