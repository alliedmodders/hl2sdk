//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "vertexlitgeneric_dx9_helper.h"
#include "skin_dx9_helper.h"
#include "basevsshader.h"
#include "SDK_vertexlit_and_unlit_generic_vs20.inc"
#include "SDK_vertexlit_and_unlit_generic_ps20.inc"
#include "SDK_vertexlit_and_unlit_generic_bump_vs20.inc"
#include "SDK_vertexlit_and_unlit_generic_bump_ps20.inc"
#include "convar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//-----------------------------------------------------------------------------
// Initialize shader parameters
//-----------------------------------------------------------------------------
void InitParamsVertexLitGeneric_DX9( CBaseVSShader *pShader, IMaterialVar** params, const char *pMaterialName, bool bVertexLitGeneric, VertexLitGeneric_DX9_Vars_t &info )
{	
	Assert( info.m_nPhong >= 0 );
	Assert( info.m_nPhongExponent >= 0 );
	Assert( info.m_nPhongExponentTexture >= 0 );
	Assert( info.m_nLightWarpTexture >= 0 );
	Assert( info.m_nPhongBoost >= 0 );
	Assert( info.m_nPhongFresnelRanges >= 0 );

	if( !params[info.m_nPhong]->IsDefined() )
	{
		params[info.m_nPhong]->SetIntValue( 0 );
	}

	if( params[info.m_nPhong]->GetIntValue() != 0 )
	{
		if( g_pHardwareConfig->GetDXSupportLevel() < 90 )
		{
			params[info.m_nPhong]->SetIntValue( 0 );
		}
		else
		{
			InitParamsSkin_DX9( pShader, params, pMaterialName, true, info );
			return;
		}
	}
	
	// FLASHLIGHTFIXME: Do ShaderAPI::BindFlashlightTexture
	Assert( info.m_nFlashlightTexture >= 0 );
	params[info.m_nFlashlightTexture]->SetStringValue( "effects/flashlight001" );
	
	// Write over $basetexture with $info.m_nBumpmap if we are going to be using diffuse normal mapping.
	if( info.m_nAlbedo != -1 && g_pConfig->UseBumpmapping() && info.m_nBumpmap != -1 && params[info.m_nBumpmap]->IsDefined() && params[info.m_nAlbedo]->IsDefined() &&
		params[info.m_nBaseTexture]->IsDefined() )
	{
		params[info.m_nBaseTexture]->SetStringValue( params[info.m_nAlbedo]->GetStringValue() );
	}

	if( bVertexLitGeneric )
	{
		SET_FLAGS( MATERIAL_VAR_MODEL );
		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_VERTEX_LIT );
	}
	else
	{
		CLEAR_FLAGS( MATERIAL_VAR_SELFILLUM );
	}

	if( !params[info.m_nEnvmapMaskFrame]->IsDefined() )
	{
		params[info.m_nEnvmapMaskFrame]->SetIntValue( 0 );
	}
	
	if( !params[info.m_nEnvmapTint]->IsDefined() )
	{
		params[info.m_nEnvmapTint]->SetVecValue( 1.0f, 1.0f, 1.0f );
	}

	if( (info.m_nSelfIllumTint != -1) && (!params[info.m_nSelfIllumTint]->IsDefined()) )
	{
		params[info.m_nSelfIllumTint]->SetVecValue( 1.0f, 1.0f, 1.0f );
	}

	if( !params[info.m_nDetailScale]->IsDefined() )
	{
		params[info.m_nDetailScale]->SetFloatValue( 4.0f );
	}
	
	// With HDR, we don't support envmap contrast and saturation since they were
	// trying to emulate HDR effects.
	if( !params[info.m_nEnvmapContrast]->IsDefined())
	{
		params[info.m_nEnvmapContrast]->SetFloatValue( 0.0f );
	}
	
	// With HDR, we don't support envmap contrast and saturation since they were
	// trying to emulate HDR effects.
	if( !params[info.m_nEnvmapSaturation]->IsDefined() )
	{
		params[info.m_nEnvmapSaturation]->SetFloatValue( 1.0f );
	}
	
	if( !params[info.m_nEnvmapFrame]->IsDefined() )
	{
		params[info.m_nEnvmapFrame]->SetIntValue( 0 );
	}

	if( (info.m_nBumpFrame != -1) && !params[info.m_nBumpFrame]->IsDefined() )
	{
		params[info.m_nBumpFrame]->SetIntValue( 0 );
	}

	// No texture means no self-illum or env mask in base alpha
	if ( !params[info.m_nBaseTexture]->IsDefined() )
	{
		CLEAR_FLAGS( MATERIAL_VAR_SELFILLUM );
		CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
	}

	// If in decal mode, no debug override...
	if (IS_FLAG_SET(MATERIAL_VAR_DECAL))
	{
		SET_FLAGS( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
	}

	if( ( (info.m_nBumpmap != -1) && g_pConfig->UseBumpmapping() && params[info.m_nBumpmap]->IsDefined() ) || params[info.m_nEnvmap]->IsDefined() )
	{
		SET_FLAGS2( MATERIAL_VAR2_NEEDS_TANGENT_SPACES );
	}
	else
	{
		CLEAR_FLAGS( MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK );
	}

	bool hasNormalMapAlphaEnvmapMask = IS_FLAG_SET( MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK );
	if( hasNormalMapAlphaEnvmapMask )
	{
		params[info.m_nEnvmapMask]->SetUndefined();
		CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
	}

	if( IS_FLAG_SET( MATERIAL_VAR_BASEALPHAENVMAPMASK ) && info.m_nBumpmap != -1 && 
		params[info.m_nBumpmap]->IsDefined() && !hasNormalMapAlphaEnvmapMask )
	{
		Warning( "material %s has a normal map and $basealphaenvmapmask.  Must use $normalmapalphaenvmapmask to get specular.\n\n", pMaterialName );
		params[info.m_nEnvmap]->SetUndefined();
	}
	
	if( params[info.m_nEnvmapMask]->IsDefined() && info.m_nBumpmap != -1 && params[info.m_nBumpmap]->IsDefined() )
	{
		params[info.m_nEnvmapMask]->SetUndefined();
		if( !hasNormalMapAlphaEnvmapMask )
		{
			Warning( "material %s has a normal map and an envmapmask.  Must use $normalmapalphaenvmapmask.\n\n", pMaterialName );
			params[info.m_nEnvmap]->SetUndefined();
		}
	}

	// If mat_specular 0, then get rid of envmap
	if( !g_pConfig->UseSpecular() && params[info.m_nEnvmap]->IsDefined() && params[info.m_nBaseTexture]->IsDefined() )
	{
		params[info.m_nEnvmap]->SetUndefined();
	}

	if( ( info.m_nHDRColorScale != -1 ) && !params[info.m_nHDRColorScale]->IsDefined() )
	{
		params[info.m_nHDRColorScale]->SetFloatValue( 1.0f );
	}
}


//-----------------------------------------------------------------------------
// Initialize shader
//-----------------------------------------------------------------------------

void InitVertexLitGeneric_DX9( CBaseVSShader *pShader, IMaterialVar** params, bool bVertexLitGeneric, VertexLitGeneric_DX9_Vars_t &info )
{
	Assert( info.m_nPhong >= 0 );

	if( params[info.m_nPhong]->GetIntValue() )
	{
		InitSkin_DX9( pShader, params, true, info );
		return;
	}

	Assert( info.m_nFlashlightTexture >= 0 );
	pShader->LoadTexture( info.m_nFlashlightTexture );
	
	bool bIsBaseTextureTranslucent = false;
	if ( params[info.m_nBaseTexture]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nBaseTexture );
		
		if ( params[info.m_nBaseTexture]->GetTextureValue()->IsTranslucent() )
		{
			bIsBaseTextureTranslucent = true;
		}
	}
	// No alpha channel in any of the textures? No self illum or envmapmask
	if ( !bIsBaseTextureTranslucent )
	{
		CLEAR_FLAGS( MATERIAL_VAR_SELFILLUM );
		CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
	}

	if ( params[info.m_nDetail]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nDetail );
	}
	
	if ( g_pConfig->UseBumpmapping() )
	{
		if ( (info.m_nBumpmap != -1) && params[info.m_nBumpmap]->IsDefined() )
		{
			pShader->LoadBumpMap( info.m_nBumpmap );
			SET_FLAGS2( MATERIAL_VAR2_DIFFUSE_BUMPMAPPED_MODEL );
		}
	}

	// Don't alpha test if the alpha channel is used for other purposes
	if ( IS_FLAG_SET(MATERIAL_VAR_SELFILLUM) || IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK) )
	{
		CLEAR_FLAGS( MATERIAL_VAR_ALPHATEST );
	}
	
	if ( params[info.m_nEnvmap]->IsDefined() )
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
	}
	if ( params[info.m_nEnvmapMask]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nEnvmapMask );
	}
}


//-----------------------------------------------------------------------------
// Draws the shader
//-----------------------------------------------------------------------------
void DrawVertexLitGeneric_DX9( CBaseVSShader *pShader, IMaterialVar** params, 
	IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow, bool bVertexLitGeneric, VertexLitGeneric_DX9_Vars_t &info )
{
	Assert( info.m_nPhong >= 0 );
	if( params[info.m_nPhong]->GetIntValue() )
	{
		DrawSkin_DX9( pShader, params, pShaderAPI, pShaderShadow, true, info );
		return;		
	}

	bool hasBaseTexture = params[info.m_nBaseTexture]->IsTexture();
	bool hasBump = (info.m_nBumpmap != -1) && params[info.m_nBumpmap]->IsTexture();
	bool hasDetailTexture = !hasBump && params[info.m_nDetail]->IsTexture();
	bool hasDiffuseLighting = bVertexLitGeneric;
	bool hasBaseAlphaEnvmapMask = IS_FLAG_SET( MATERIAL_VAR_BASEALPHAENVMAPMASK );
	bool hasNormalMapAlphaEnvmapMask = IS_FLAG_SET( MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK );
	bool hasVertexColor = bVertexLitGeneric ? false : IS_FLAG_SET( MATERIAL_VAR_VERTEXCOLOR );
	bool hasVertexAlpha = bVertexLitGeneric ? false : IS_FLAG_SET( MATERIAL_VAR_VERTEXALPHA );
	bool bIsAlphaTested = IS_FLAG_SET( MATERIAL_VAR_ALPHATEST ) != 0;
	bool hasSelfIllumInEnvMapMask =
		( info.m_nSelfIllumEnvMapMask_Alpha != -1 ) &&
		( params[info.m_nSelfIllumEnvMapMask_Alpha]->GetFloatValue() != 0.0 ) ;
	bool bAmbientOnly = (info.m_nAmbientOnly != -1) && (params[info.m_nAmbientOnly]->GetIntValue() == 1);

	BlendType_t blendType;
	if ( params[info.m_nBaseTexture]->IsTexture() )
	{
		blendType = pShader->EvaluateBlendRequirements( info.m_nBaseTexture, true );
	}
	else
	{
		blendType = pShader->EvaluateBlendRequirements( info.m_nEnvmapMask, false );
	}
	
	if( pShader->IsSnapshotting() )
	{
		// look at color and alphamod stuff.
		// Unlit generic never uses the flashlight
		bool hasFlashlight = hasDiffuseLighting && CShader_IsFlag2Set( params, MATERIAL_VAR2_USE_FLASHLIGHT );
		bool hasEnvmap = !hasFlashlight && params[info.m_nEnvmap]->IsTexture();
		bool hasEnvmapMask = (hasSelfIllumInEnvMapMask || !hasFlashlight) && 
			params[info.m_nEnvmapMask]->IsTexture();
		bool bHasNormal = bVertexLitGeneric || hasEnvmap;
		bool hasSelfIllum = !hasFlashlight && IS_FLAG_SET( MATERIAL_VAR_SELFILLUM );

		if( hasFlashlight ) hasEnvmapMask = false;

		bool bHalfLambert = IS_FLAG_SET( MATERIAL_VAR_HALFLAMBERT );
		// Alpha test: FIXME: shouldn't this be handled in CBaseVSShader::SetInitialShadowState
		pShaderShadow->EnableAlphaTest( bIsAlphaTested );

		if( info.m_nAlphaTestReference != -1 && params[info.m_nAlphaTestReference]->GetFloatValue() > 0.0f )
		{
			pShaderShadow->AlphaFunc( SHADER_ALPHAFUNC_GEQUAL, params[info.m_nAlphaTestReference]->GetFloatValue() );
		}

		if( hasFlashlight )
		{
			if (params[info.m_nBaseTexture]->IsTexture())
			{
				pShader->SetAdditiveBlendingShadowState( info.m_nBaseTexture, true );
			}
			else
			{
				pShader->SetAdditiveBlendingShadowState( info.m_nEnvmapMask, false );
			}
			if( bIsAlphaTested )
			{
				// disable alpha test and use the zfunc zequals since alpha isn't guaranteed to 
				// be the same on both the regular pass and the flashlight pass.
				pShaderShadow->EnableAlphaTest( false );
				pShaderShadow->DepthFunc( SHADER_DEPTHFUNC_EQUAL );
			}
			pShaderShadow->EnableBlending( true );
			pShaderShadow->EnableDepthWrites( false );
		}
		else
		{
			if (params[info.m_nBaseTexture]->IsTexture())
			{
				pShader->SetDefaultBlendingShadowState( info.m_nBaseTexture, true );
			}
			else
			{
				pShader->SetDefaultBlendingShadowState( info.m_nEnvmapMask, false );
			}
		}
		
		unsigned int flags = VERTEX_POSITION;
		if( bHasNormal )
		{
			flags |= VERTEX_NORMAL;
		}

		int userDataSize = 0;
		if( hasBaseTexture )
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->EnableSRGBRead( SHADER_TEXTURE_STAGE0, true );
		}
		if( hasEnvmap )
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
			if( g_pHardwareConfig->GetHDRType() == HDR_TYPE_NONE )
			{
				pShaderShadow->EnableSRGBRead( SHADER_TEXTURE_STAGE1, true );
			}
		}
		if( hasFlashlight )
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE7, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE4, true );
			userDataSize = 4; // tangent S
		}
		if( hasDetailTexture )
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE2, true );
		}
		if( hasBump )
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE3, true );
			userDataSize = 4; // tangent S
			// Normalizing cube map
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE5, true );
		}
		if( hasEnvmapMask )
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE4, true );
		}

		if( hasVertexColor || hasVertexAlpha )
		{
			flags |= VERTEX_COLOR;
		}

		pShaderShadow->EnableSRGBWrite( true );
		
		// texcoord0 : base texcoord
		const int numTexCoords = 1;
		int numBoneWeights = 0;
		if( IS_FLAG_SET( MATERIAL_VAR_MODEL ) )
		{
			numBoneWeights = 3;
			flags |= VERTEX_BONE_INDEX;
		}
		
		pShaderShadow->VertexShaderVertexFormat( 
			flags, numTexCoords, NULL, numBoneWeights, userDataSize );

		if ( hasBump )
		{
			DECLARE_STATIC_VERTEX_SHADER( sdk_vertexlit_and_unlit_generic_bump_vs20 );
			SET_STATIC_VERTEX_SHADER_COMBO( HALFLAMBERT,  bHalfLambert);
			SET_STATIC_VERTEX_SHADER( sdk_vertexlit_and_unlit_generic_bump_vs20 );
			
			DECLARE_STATIC_PIXEL_SHADER( sdk_vertexlit_and_unlit_generic_bump_ps20 );
			SET_STATIC_PIXEL_SHADER_COMBO( BASETEXTURE,  hasBaseTexture );
			SET_STATIC_PIXEL_SHADER_COMBO( CUBEMAP,  hasEnvmap );
			SET_STATIC_PIXEL_SHADER_COMBO( DIFFUSELIGHTING,  hasDiffuseLighting );
			SET_STATIC_PIXEL_SHADER_COMBO( SELFILLUM,  hasSelfIllum );
			SET_STATIC_PIXEL_SHADER_COMBO( NORMALMAPALPHAENVMAPMASK,  hasNormalMapAlphaEnvmapMask );
			SET_STATIC_PIXEL_SHADER_COMBO( HALFLAMBERT,  bHalfLambert);
			SET_STATIC_PIXEL_SHADER_COMBO( FLASHLIGHT,  hasFlashlight );
			SET_STATIC_PIXEL_SHADER( sdk_vertexlit_and_unlit_generic_bump_ps20 );
		}
		else
		{
			DECLARE_STATIC_VERTEX_SHADER( sdk_vertexlit_and_unlit_generic_vs20 );
			SET_STATIC_VERTEX_SHADER_COMBO( VERTEXCOLOR,  hasVertexColor || hasVertexAlpha );
			SET_STATIC_VERTEX_SHADER_COMBO( CUBEMAP,  hasEnvmap );
			SET_STATIC_VERTEX_SHADER_COMBO( HALFLAMBERT,  bHalfLambert );
			SET_STATIC_VERTEX_SHADER_COMBO( FLASHLIGHT,  hasFlashlight );
			SET_STATIC_VERTEX_SHADER( sdk_vertexlit_and_unlit_generic_vs20 );
			
			DECLARE_STATIC_PIXEL_SHADER( sdk_vertexlit_and_unlit_generic_ps20 );
			SET_STATIC_PIXEL_SHADER_COMBO( SELFILLUM_ENVMAPMASK_ALPHA, 
										   ( hasSelfIllumInEnvMapMask && ( hasEnvmapMask ) ) );
			SET_STATIC_PIXEL_SHADER_COMBO( BASETEXTURE,  hasBaseTexture );
			SET_STATIC_PIXEL_SHADER_COMBO( DETAILTEXTURE,  hasDetailTexture );
			SET_STATIC_PIXEL_SHADER_COMBO( CUBEMAP,  hasEnvmap );
			SET_STATIC_PIXEL_SHADER_COMBO( DIFFUSELIGHTING,  hasDiffuseLighting );
			SET_STATIC_PIXEL_SHADER_COMBO( ENVMAPMASK,  hasEnvmapMask );
			SET_STATIC_PIXEL_SHADER_COMBO( BASEALPHAENVMAPMASK,  hasBaseAlphaEnvmapMask );
			SET_STATIC_PIXEL_SHADER_COMBO( SELFILLUM,  hasSelfIllum );
			SET_STATIC_PIXEL_SHADER_COMBO( VERTEXCOLOR,  hasVertexColor );
			SET_STATIC_PIXEL_SHADER_COMBO( VERTEXALPHA,  hasVertexAlpha );
			SET_STATIC_PIXEL_SHADER_COMBO( FLASHLIGHT,  hasFlashlight );
			SET_STATIC_PIXEL_SHADER( sdk_vertexlit_and_unlit_generic_ps20 );
		}

		if( hasFlashlight )
		{
			pShader->FogToBlack();
		}
		else
		{
			pShader->DefaultFog();
		}

		// HACK HACK HACK - enable alpha writes all the time so that we have them for
		// underwater stuff
		if( blendType != BT_BLENDADD && blendType != BT_BLEND && !bIsAlphaTested )
		{
			pShaderShadow->EnableAlphaWrites( true );
		}
	}
	else
	{
		bool hasFlashlight = hasDiffuseLighting && pShaderAPI->InFlashlightMode();
		bool hasEnvmap = !hasFlashlight && params[info.m_nEnvmap]->IsTexture();
		bool hasEnvmapMask = !hasFlashlight && params[info.m_nEnvmapMask]->IsTexture();

		if( hasBaseTexture )
		{
			pShader->BindTexture( SHADER_TEXTURE_STAGE0, info.m_nBaseTexture, info.m_nBaseTextureFrame );
		}
		if( hasEnvmap )
		{
			pShader->BindTexture( SHADER_TEXTURE_STAGE1, info.m_nEnvmap, info.m_nEnvmapFrame );
		}
		if( hasDetailTexture )
		{
			pShader->BindTexture( SHADER_TEXTURE_STAGE2, info.m_nDetail, info.m_nDetailFrame );
		}
		if( !g_pConfig->m_bFastNoBump )
		{
			if( hasBump )
			{
				pShader->BindTexture( SHADER_TEXTURE_STAGE3, info.m_nBumpmap, info.m_nBumpFrame );
			}
		}
		else
		{
			if( hasBump )
			{
				pShaderAPI->BindFlatNormalMap( SHADER_TEXTURE_STAGE3 );
			}
		}
		if( hasEnvmapMask )
		{
			pShader->BindTexture( SHADER_TEXTURE_STAGE4, info.m_nEnvmapMask, info.m_nEnvmapMaskFrame );
		}
		bool bHasFlashlightDepth = false;
		if( hasFlashlight )
		{
			Assert( info.m_nFlashlightTexture >= 0 && info.m_nFlashlightTextureFrame >= 0 );
			pShader->BindTexture( SHADER_TEXTURE_STAGE7, info.m_nFlashlightTexture, info.m_nFlashlightTextureFrame );
			VMatrix worldToTexture;
			ITexture *pFlashlightDepthTexture;
			FlashlightState_t state = pShaderAPI->GetFlashlightStateEx( worldToTexture, &pFlashlightDepthTexture );
			SetFlashLightColorFromState( state, pShaderAPI );
			if( pFlashlightDepthTexture )
			{
				pShader->BindTexture( SHADER_TEXTURE_STAGE4, pFlashlightDepthTexture, 0 );
				bHasFlashlightDepth = true;
			}
		}

		int lightCombo = 0;
		if( bVertexLitGeneric && !hasFlashlight )
		{
			lightCombo = pShaderAPI->GetCurrentLightCombo();
		}
		MaterialFogMode_t fogType = pShaderAPI->GetSceneFogMode();
		int fogIndex = ( fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z ) ? 1 : 0;
		int numBones	= pShaderAPI->GetCurrentNumBones();

		if ( hasBump )
		{
			DECLARE_DYNAMIC_VERTEX_SHADER( sdk_vertexlit_and_unlit_generic_bump_vs20 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( LIGHT_COMBO,  lightCombo );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG,  fogIndex );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( NUM_BONES,  numBones );
			SET_DYNAMIC_VERTEX_SHADER( sdk_vertexlit_and_unlit_generic_bump_vs20 );

			DECLARE_DYNAMIC_PIXEL_SHADER( sdk_vertexlit_and_unlit_generic_bump_ps20 );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( LIGHT_COMBO,  lightCombo );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( WRITEWATERFOGTODESTALPHA,  fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z &&
				blendType != BT_BLENDADD && blendType != BT_BLEND && !bIsAlphaTested );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( FOGTYPE, pShaderAPI->GetSceneFogMode() );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( FLASHLIGHTDEPTH, bHasFlashlightDepth );
			SET_DYNAMIC_PIXEL_SHADER( sdk_vertexlit_and_unlit_generic_bump_ps20 );
		}
		else
		{
			if ( bAmbientOnly )	// Override selected light combo to be ambient only
				lightCombo = 2;

			DECLARE_DYNAMIC_VERTEX_SHADER( sdk_vertexlit_and_unlit_generic_vs20 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( LIGHT_COMBO,  lightCombo );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG,  fogIndex );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( NUM_BONES,  numBones );
			SET_DYNAMIC_VERTEX_SHADER_COMBO(
				LIGHTING_PREVIEW, hasDiffuseLighting && pShaderAPI->GetIntRenderingParameter(INT_RENDERPARM_ENABLE_FIXED_LIGHTING)!=0);
			SET_DYNAMIC_VERTEX_SHADER( sdk_vertexlit_and_unlit_generic_vs20 );

			DECLARE_DYNAMIC_PIXEL_SHADER( sdk_vertexlit_and_unlit_generic_ps20 );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( WRITEWATERFOGTODESTALPHA,  fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z &&
				blendType != BT_BLENDADD && blendType != BT_BLEND && !bIsAlphaTested );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( FOGTYPE, pShaderAPI->GetSceneFogMode() );
			SET_DYNAMIC_PIXEL_SHADER_COMBO(
				LIGHTING_PREVIEW, hasDiffuseLighting && pShaderAPI->GetIntRenderingParameter(INT_RENDERPARM_ENABLE_FIXED_LIGHTING));
			SET_DYNAMIC_PIXEL_SHADER_COMBO( FLASHLIGHTDEPTH, bHasFlashlightDepth );
			SET_DYNAMIC_PIXEL_SHADER( sdk_vertexlit_and_unlit_generic_ps20 );
		}

		pShader->SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, info.m_nBaseTextureTransform );
		if( hasDetailTexture )
		{
			pShader->SetVertexShaderTextureScaledTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_2, info.m_nBaseTextureTransform, info.m_nDetailScale );
			Assert( !hasBump );
		}
		if( hasBump )
		{
			pShader->SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_2, info.m_nBumpTransform );
			Assert( !hasDetailTexture );
		}
		if( hasEnvmapMask )
		{
			pShader->SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_4, info.m_nEnvmapMaskTransform );
		}
		
		if( hasEnvmap )
		{
			pShader->SetEnvMapTintPixelShaderDynamicState( 0, info.m_nEnvmapTint, -1, true );
		}
		if( ( info.m_nHDRColorScale != -1 ) && pShader->IsHDREnabled() )
		{
			pShader->SetModulationPixelShaderDynamicState_LinearColorSpace_LinearScale( 1, params[info.m_nHDRColorScale]->GetFloatValue() );
		}
		else
		{
			pShader->SetModulationPixelShaderDynamicState_LinearColorSpace( 1 );
		}

		pShader->SetPixelShaderConstant( 2, info.m_nEnvmapContrast );
		pShader->SetPixelShaderConstant( 3, info.m_nEnvmapSaturation );

		pShader->SetPixelShaderConstant( 4, info.m_nSelfIllumTint );
		pShader->SetAmbientCubeDynamicStateVertexShader();
		if( hasBump )
		{
			pShaderAPI->BindSignedNormalizationCubeMap( SHADER_TEXTURE_STAGE5 );
			pShaderAPI->SetPixelShaderStateAmbientLightCube( 5 );
            pShaderAPI->CommitPixelShaderLighting( 13 );
	
		}

		if( fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z )
		{
			pShader->SetWaterFogColorPixelShaderConstantLinear( 19 );
		}

		float eyePos[4];
		pShaderAPI->GetWorldSpaceCameraPosition( eyePos );
		pShaderAPI->SetPixelShaderConstant( 20, eyePos, 1 );
		pShaderAPI->SetPixelShaderFogParams( 21 );

		// flashlightfixme: put this in common code.
		if( hasFlashlight )
		{
			VMatrix worldToTexture;
			const FlashlightState_t &flashlightState = pShaderAPI->GetFlashlightState( worldToTexture );

			// Set the flashlight attenuation factors
			float atten[4];
			atten[0] = flashlightState.m_fConstantAtten;
			atten[1] = flashlightState.m_fLinearAtten;
			atten[2] = flashlightState.m_fQuadraticAtten;
			atten[3] = flashlightState.m_FarZ;
			pShaderAPI->SetPixelShaderConstant( 22, atten, 1 );

			// Set the flashlight origin
			float pos[4];
			pos[0] = flashlightState.m_vecLightOrigin[0];
			pos[1] = flashlightState.m_vecLightOrigin[1];
			pos[2] = flashlightState.m_vecLightOrigin[2];
			pos[3] = 1.0f;
			pShaderAPI->SetPixelShaderConstant( 23, pos, 1 );

			pShaderAPI->SetPixelShaderConstant( 24, worldToTexture.Base(), 4 );
		}		
	}
}
