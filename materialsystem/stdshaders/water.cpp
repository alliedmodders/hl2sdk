//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"
#include "VMatrix.h"
#include "common_hlsl_cpp_consts.h" // hack hack hack!
#include "convar.h"

#include "SDK_watercheap_vs20.inc"
#include "SDK_watercheap_ps20.inc"
#include "SDK_water_vs20.inc"
#include "SDK_water_ps20.inc"

static ConVar r_waterforceexpensive( "r_waterforceexpensive", "0" );

DEFINE_FALLBACK_SHADER( SDK_Water, SDK_Water_DX9_HDR )

BEGIN_VS_SHADER( SDK_Water_DX90, 
			  "Help for SDK_Water" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( REFRACTTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_WaterRefraction", "" )
		SHADER_PARAM( REFLECTTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_WaterReflection", "" )
		SHADER_PARAM( REFRACTAMOUNT, SHADER_PARAM_TYPE_FLOAT, "0", "" )
		SHADER_PARAM( REFRACTTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "refraction tint" )
		SHADER_PARAM( REFLECTAMOUNT, SHADER_PARAM_TYPE_FLOAT, "0.8", "" )
		SHADER_PARAM( REFLECTTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "reflection tint" )
		SHADER_PARAM( NORMALMAP, SHADER_PARAM_TYPE_TEXTURE, "dev/water_normal", "normal map" )
		SHADER_PARAM( BUMPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $bumpmap" )
		SHADER_PARAM( BUMPTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$bumpmap texcoord transform" )
		SHADER_PARAM( SCALE, SHADER_PARAM_TYPE_VEC2, "[1 1]", "" )
		SHADER_PARAM( TIME, SHADER_PARAM_TYPE_FLOAT, "", "" )
		SHADER_PARAM( WATERDEPTH, SHADER_PARAM_TYPE_FLOAT, "", "" )
		SHADER_PARAM( CHEAPWATERSTARTDISTANCE, SHADER_PARAM_TYPE_FLOAT, "", "This is the distance from the eye in inches that the shader should start transitioning to a cheaper water shader." )
		SHADER_PARAM( CHEAPWATERENDDISTANCE, SHADER_PARAM_TYPE_FLOAT, "", "This is the distance from the eye in inches that the shader should finish transitioning to a cheaper water shader." )
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "env_cubemap", "envmap" )
		SHADER_PARAM( ENVMAPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "" )
		SHADER_PARAM( FOGCOLOR, SHADER_PARAM_TYPE_COLOR, "", "" )
		SHADER_PARAM( FORCECHEAP, SHADER_PARAM_TYPE_BOOL, "", "" )
		SHADER_PARAM( FORCEEXPENSIVE, SHADER_PARAM_TYPE_BOOL, "", "" )
		SHADER_PARAM( REFLECTENTITIES, SHADER_PARAM_TYPE_BOOL, "", "" )
		SHADER_PARAM( FOGSTART, SHADER_PARAM_TYPE_FLOAT, "", "" )
		SHADER_PARAM( FOGEND, SHADER_PARAM_TYPE_FLOAT, "", "" )
		SHADER_PARAM( ABOVEWATER, SHADER_PARAM_TYPE_BOOL, "", "" )
		SHADER_PARAM( REFLECTBLENDFACTOR, SHADER_PARAM_TYPE_FLOAT, "1.0", "" )
		SHADER_PARAM( NOFRESNEL, SHADER_PARAM_TYPE_BOOL, "0", "" )
		SHADER_PARAM( NOLOWENDLIGHTMAP, SHADER_PARAM_TYPE_BOOL, "0", "" )
		SHADER_PARAM( SCROLL1, SHADER_PARAM_TYPE_COLOR, "", "" )
		SHADER_PARAM( SCROLL2, SHADER_PARAM_TYPE_COLOR, "", "" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		if( !params[ABOVEWATER]->IsDefined() )
		{
			params[ABOVEWATER]->SetIntValue( 1 );
		}
		SET_FLAGS2( MATERIAL_VAR2_NEEDS_TANGENT_SPACES );
		if( !params[CHEAPWATERSTARTDISTANCE]->IsDefined() )
		{
			params[CHEAPWATERSTARTDISTANCE]->SetFloatValue( 500.0f );
		}
		if( !params[CHEAPWATERENDDISTANCE]->IsDefined() )
		{
			params[CHEAPWATERENDDISTANCE]->SetFloatValue( 1000.0f );
		}
		if( !params[SCALE]->IsDefined() )
		{
			params[SCALE]->SetVecValue( 1.0f, 1.0f );
		}
		if( !params[SCROLL1]->IsDefined() )
		{
			params[SCROLL1]->SetVecValue( 0.0f, 0.0f, 0.0f );
		}
		if( !params[SCROLL2]->IsDefined() )
		{
			params[SCROLL2]->SetVecValue( 0.0f, 0.0f, 0.0f );
		}
		if( !params[FOGCOLOR]->IsDefined() )
		{
			params[FOGCOLOR]->SetVecValue( 1.0f, 0.0f, 0.0f );
			Warning( "material %s needs to have a $fogcolor.\n", pMaterialName );
		}
		if( !params[REFLECTENTITIES]->IsDefined() )
		{
			params[REFLECTENTITIES]->SetIntValue( 0 );
		}
		if( !params[REFLECTBLENDFACTOR]->IsDefined() )
		{
			params[REFLECTBLENDFACTOR]->SetFloatValue( 1.0f );
		}

		// By default, we're force expensive on dx9
		if( !params[FORCEEXPENSIVE]->IsDefined() )
		{
			params[FORCEEXPENSIVE]->SetIntValue( 1 );
		}
		if( params[FORCEEXPENSIVE]->GetIntValue() && params[FORCECHEAP]->GetIntValue() )
		{
			params[FORCEEXPENSIVE]->SetIntValue( 0 );
		}

		// Fallbacks for water need lightmaps usually
		if ( !params[NOLOWENDLIGHTMAP]->GetIntValue() )
		{
			SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
		}

	}

	SHADER_FALLBACK
	{
		if( g_pHardwareConfig->GetDXSupportLevel() < 90 )
		{
			return "SDK_Water_DX81";
		}
		return 0;
	}

	SHADER_INIT
	{
		Assert( params[WATERDEPTH]->IsDefined() );

		if( params[REFRACTTEXTURE]->IsDefined() )
		{
			LoadTexture( REFRACTTEXTURE );
		}
		if( params[REFLECTTEXTURE]->IsDefined() )
		{
			LoadTexture( REFLECTTEXTURE );
		}
		if ( params[ENVMAP]->IsDefined() )
		{
			LoadCubeMap( ENVMAP );
		}
		if ( params[NORMALMAP]->IsDefined() )
		{
			LoadBumpMap( NORMALMAP );
		}
	}

	inline void GetVecParam( int constantVar, float *val )
	{
		if( constantVar == -1 )
			return;

		IMaterialVar* pVar = s_ppParams[constantVar];
		Assert( pVar );

		if (pVar->GetType() == MATERIAL_VAR_TYPE_VECTOR)
			pVar->GetVecValue( val, 4 );
		else
			val[0] = val[1] = val[2] = val[3] = pVar->GetFloatValue();
	}

	inline void DrawReflectionRefraction( IMaterialVar **params, IShaderShadow* pShaderShadow,
		IShaderDynamicAPI* pShaderAPI, bool bReflection, bool bRefraction ) 
	{
		SHADOW_STATE
		{
			SetInitialShadowState( );
			if( bRefraction )
			{
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
				if( g_pHardwareConfig->GetHDRType() == HDR_TYPE_INTEGER )
				{
					pShaderShadow->EnableSRGBRead( SHADER_TEXTURE_STAGE0, true );
				}
			}
			if( bReflection )
			{
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE2, true );
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE3, true );
				if( g_pHardwareConfig->GetHDRType() == HDR_TYPE_INTEGER )
				{
					pShaderShadow->EnableSRGBRead( SHADER_TEXTURE_STAGE2, true );
				}
			}
			// normal map
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE4, true );
			// Normalizing cube map
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE5, true );

			int fmt = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_TANGENT_S | VERTEX_TANGENT_T;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0, 0 );
			
			DECLARE_STATIC_VERTEX_SHADER( sdk_water_vs20 );
			SET_STATIC_VERTEX_SHADER( sdk_water_vs20 );

			// "REFLECT" "0..1"
			// "REFRACT" "0..1"
			
			DECLARE_STATIC_PIXEL_SHADER( sdk_water_ps20 );
			SET_STATIC_PIXEL_SHADER_COMBO( REFLECT,  bReflection );
			SET_STATIC_PIXEL_SHADER_COMBO( REFRACT,  bRefraction );
			SET_STATIC_PIXEL_SHADER_COMBO( ABOVEWATER,  params[ABOVEWATER]->GetIntValue() );
			SET_STATIC_PIXEL_SHADER_COMBO( HDRTYPE,  g_pHardwareConfig->GetHDRType() );
			Vector4D Scroll1;
			params[SCROLL1]->GetVecValue( Scroll1.Base(), 4 );
			SET_STATIC_PIXEL_SHADER_COMBO( MULTITEXTURE,fabs(Scroll1.x) > 0.0);
			SET_STATIC_PIXEL_SHADER( sdk_water_ps20 );

			FogToFogColor();

			if( g_pHardwareConfig->GetHDRType() != HDR_TYPE_NONE )
			{
				// we are writing linear values from this shader.
				pShaderShadow->EnableSRGBWrite( true );
			}
		}
		DYNAMIC_STATE
		{
			bool bHDREnabled = IsHDREnabled();
			pShaderAPI->SetDefaultState();
			if( bRefraction )
			{
				// HDRFIXME: add comment about binding.. Specify the number of MRTs in the enable
				BindTexture( SHADER_TEXTURE_STAGE0, REFRACTTEXTURE, -1 );
			}
			if( bReflection )
			{
				BindTexture( SHADER_TEXTURE_STAGE2, REFLECTTEXTURE, -1 );
			}
			BindTexture( SHADER_TEXTURE_STAGE4, NORMALMAP, BUMPFRAME );
			pShaderAPI->BindSignedNormalizationCubeMap( SHADER_TEXTURE_STAGE5 );

			DECLARE_DYNAMIC_VERTEX_SHADER( sdk_water_vs20 );
			SET_DYNAMIC_VERTEX_SHADER( sdk_water_vs20 );
			
			DECLARE_DYNAMIC_PIXEL_SHADER( sdk_water_ps20 );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( HDRENABLED, bHDREnabled );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( FOGTYPE, pShaderAPI->GetSceneFogMode() );
			SET_DYNAMIC_PIXEL_SHADER( sdk_water_ps20 );
			
			// Refraction tint
			if( bRefraction )
			{
				SetPixelShaderConstantGammaToLinear( 1, REFRACTTINT );
			}
			// Reflection tint
			if( bReflection )
			{
				SetPixelShaderConstantGammaToLinear( 4, REFLECTTINT );
			}

			SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_1, BUMPTRANSFORM );
			
			float curtime=pShaderAPI->CurrentTime();
			float vc0[4];
			float v0[4];
			params[SCROLL1]->GetVecValue(v0,4);
			vc0[0]=curtime*v0[0];
			vc0[1]=curtime*v0[1];
			params[SCROLL2]->GetVecValue(v0,4);
			vc0[2]=curtime*v0[0];
			vc0[3]=curtime*v0[1];
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_3, vc0, 1 );

			float c0[4] = { 1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f, 0.0f };
			pShaderAPI->SetPixelShaderConstant( 0, c0, 1 );
			
			float c2[4] = { 0.5f, 0.5f, 0.5f, 0.5f };
			pShaderAPI->SetPixelShaderConstant( 2, c2, 1 );
			
			// fresnel constants
			float c3[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
			pShaderAPI->SetPixelShaderConstant( 3, c3, 1 );

			float c5[4] = { params[REFLECTAMOUNT]->GetFloatValue(), params[REFLECTAMOUNT]->GetFloatValue(), 
				params[REFRACTAMOUNT]->GetFloatValue(), params[REFRACTAMOUNT]->GetFloatValue() };
			pShaderAPI->SetPixelShaderConstant( 5, c5, 1 );

			SetPixelShaderConstant( 6, FOGCOLOR );

			float c7[4] = { 
				params[FOGSTART]->GetFloatValue(), 
				params[FOGEND]->GetFloatValue() - params[FOGSTART]->GetFloatValue(), 
				1.0f, 0.0f };

			if (g_pHardwareConfig->GetHDRType() == HDR_TYPE_INTEGER )
				c7[2]=4.0;									// water overbright factor

			pShaderAPI->SetPixelShaderConstant( 7, c7, 1 );

		}
		Draw();
	}

	inline void DrawCheapWater( IMaterialVar **params, IShaderShadow* pShaderShadow, 
		                        IShaderDynamicAPI* pShaderAPI, bool bBlend, bool bRefraction )
	{
		SHADOW_STATE
		{
			SetInitialShadowState( );

			// In edit mode, use nocull
			if ( UsingEditor( params ) )
			{
				s_pShaderShadow->EnableCulling( false );
			}

			if( bBlend )
			{
				EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			}
			// envmap
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			// normal map
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
			if( bRefraction && bBlend )
			{
				// refraction map (used for alpha)
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE2, true );
			}
			// Normalizing cube map
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE6, true );
			int fmt = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_TANGENT_S | VERTEX_TANGENT_T;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0, 0 );

			DECLARE_STATIC_VERTEX_SHADER( sdk_watercheap_vs20 );
			SET_STATIC_VERTEX_SHADER_COMBO( BLEND,  bBlend && bRefraction );
			SET_STATIC_VERTEX_SHADER( sdk_watercheap_vs20 );

			DECLARE_STATIC_PIXEL_SHADER( sdk_watercheap_ps20 );
			SET_STATIC_PIXEL_SHADER_COMBO( FRESNEL,  params[NOFRESNEL]->GetIntValue() == 0 );
			SET_STATIC_PIXEL_SHADER_COMBO( BLEND,  bBlend );
			SET_STATIC_PIXEL_SHADER_COMBO( REFRACTALPHA,  bRefraction );
			SET_STATIC_PIXEL_SHADER_COMBO( HDRTYPE,  g_pHardwareConfig->GetHDRType() );
			Vector4D Scroll1;
			params[SCROLL1]->GetVecValue( Scroll1.Base(), 4 );
			SET_STATIC_PIXEL_SHADER_COMBO( MULTITEXTURE,fabs(Scroll1.x) > 0.0);
			SET_STATIC_PIXEL_SHADER( sdk_watercheap_ps20 );
			// HDRFIXME: test cheap water!
			if( g_pHardwareConfig->GetHDRType() != HDR_TYPE_NONE )
			{
				// we are writing linear values from this shader.
				pShaderShadow->EnableSRGBWrite( true );
			}

			FogToFogColor();
		}
		DYNAMIC_STATE
		{
			pShaderAPI->SetDefaultState();
			BindTexture( SHADER_TEXTURE_STAGE0, ENVMAP, ENVMAPFRAME );
			BindTexture( SHADER_TEXTURE_STAGE1, NORMALMAP, BUMPFRAME );
			if( bRefraction && bBlend )
			{
				BindTexture( SHADER_TEXTURE_STAGE2, REFRACTTEXTURE, -1 );
			}
			pShaderAPI->BindSignedNormalizationCubeMap( SHADER_TEXTURE_STAGE6 );
			SetPixelShaderConstant( 0, FOGCOLOR );
			float cheapWaterStartDistance = params[CHEAPWATERSTARTDISTANCE]->GetFloatValue();
			float cheapWaterEndDistance = params[CHEAPWATERENDDISTANCE]->GetFloatValue();
			float cheapWaterParams[4] = 
			{
				cheapWaterStartDistance * VSHADER_VECT_SCALE,
				cheapWaterEndDistance * VSHADER_VECT_SCALE,
				PSHADER_VECT_SCALE / ( cheapWaterEndDistance - cheapWaterStartDistance ),
				cheapWaterStartDistance / ( cheapWaterEndDistance - cheapWaterStartDistance ),
			};
			pShaderAPI->SetPixelShaderConstant( 1, cheapWaterParams );
			if( g_pConfig->bShowSpecular )
			{
				SetPixelShaderConstant( 2, REFLECTTINT, REFLECTBLENDFACTOR );
			}
			else
			{
				float zero[4] = { 0.0f, 0.0f, 0.0f, params[REFLECTBLENDFACTOR]->GetFloatValue() };
				pShaderAPI->SetPixelShaderConstant( 2, zero );
			}

			if( params[SCROLL1]->IsDefined())
			{
				float curtime=pShaderAPI->CurrentTime();
				float vc0[4];
				float v0[4];
				params[SCROLL1]->GetVecValue(v0,4);
				vc0[0]=curtime*v0[0];
				vc0[1]=curtime*v0[1];
				params[SCROLL2]->GetVecValue(v0,4);
				vc0[2]=curtime*v0[0];
				vc0[3]=curtime*v0[1];
				pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_3, vc0, 1 );
			}

			DECLARE_DYNAMIC_VERTEX_SHADER( sdk_watercheap_vs20 );
			SET_DYNAMIC_VERTEX_SHADER( sdk_watercheap_vs20 );

			DECLARE_DYNAMIC_PIXEL_SHADER( sdk_watercheap_ps20 );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( HDRENABLED,  IsHDREnabled() );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( FOGTYPE, pShaderAPI->GetSceneFogMode() );
			SET_DYNAMIC_PIXEL_SHADER( sdk_watercheap_ps20 );
		}
		Draw();
	}

	SHADER_DRAW
	{
		// TODO: fit the cheap water stuff into the water shader so that we don't have to do
		// 2 passes.
		bool bForceExpensive = r_waterforceexpensive.GetBool();
		bool bForceCheap = (params[FORCECHEAP]->GetIntValue() != 0) || UsingEditor( params );
		if ( bForceCheap )
		{
			bForceExpensive = false;
		}
		else
		{
			bForceExpensive = bForceExpensive || (params[FORCEEXPENSIVE]->GetIntValue() != 0);
		}
		Assert( !( bForceCheap && bForceExpensive ) );

		bool bRefraction = params[REFRACTTEXTURE]->IsTexture();
		bool bReflection = bForceExpensive && params[REFLECTTEXTURE]->IsTexture();
		bool bDrewSomething = false;
		if ( !bForceCheap && ( bReflection || bRefraction ) )
		{
			bDrewSomething = true;
			DrawReflectionRefraction( params, pShaderShadow, pShaderAPI, bReflection, bRefraction );
		}

		// Use $decal to see if we are a decal or not. . if we are, then don't bother
		// drawing the cheap version for now since we don't have access to env_cubemap
		if( !bReflection && params[ENVMAP]->IsTexture() && !IS_FLAG_SET( MATERIAL_VAR_DECAL ) )
		{
			bDrewSomething = true;
			DrawCheapWater( params, pShaderShadow, pShaderAPI, !bForceCheap, bRefraction );
		}

		if( !bDrewSomething )
		{
			// We are likely here because of the tools. . . draw something so that 
			// we won't go into wireframe-land.
			Draw();
		}
	}
END_SHADER

//-----------------------------------------------------------------------------
// This allows us to use a block labelled 'Water_DX9_HDR' in the water materials
//-----------------------------------------------------------------------------
BEGIN_INHERITED_SHADER( SDK_Water_DX9_HDR, SDK_Water_DX90,
			  "Help for Water_DX9_HDR" )

	SHADER_FALLBACK
	{
		if( g_pHardwareConfig->GetHDRType() == HDR_TYPE_NONE )
		{
			return "SDK_WATER_DX90";
		}
		return 0;
	}
END_INHERITED_SHADER

