//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//


#include "BaseVSShader.h"
#include "VMatrix.h"

#include "SDK_water_vs11.inc"
#include "SDK_watercheappervertexfresnel_vs11.inc"
#include "SDK_watercheap_vs11.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef _XBOX
DEFINE_FALLBACK_SHADER( SDK_Water, SDK_Water_DX80 )
#endif


BEGIN_VS_SHADER( SDK_Water_DX80, 
			  "Help for Water_DX80" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( REFRACTTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "" )
		SHADER_PARAM( REFLECTTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_WaterReflection", "" )
		SHADER_PARAM( REFRACTAMOUNT, SHADER_PARAM_TYPE_FLOAT, "0", "" )
		SHADER_PARAM( REFRACTTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "refraction tint" )
		SHADER_PARAM( REFLECTAMOUNT, SHADER_PARAM_TYPE_FLOAT, "0", "" )
		SHADER_PARAM( REFLECTTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "reflection tint" )
		SHADER_PARAM( BUMPMAP, SHADER_PARAM_TYPE_TEXTURE, "", "dudv bump map" )
		SHADER_PARAM( NORMALMAP, SHADER_PARAM_TYPE_TEXTURE, "", "normal map" )
		SHADER_PARAM( BUMPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $bumpmap" )
		SHADER_PARAM( BUMPTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$bumpmap texcoord transform" )
		SHADER_PARAM( SCALE, SHADER_PARAM_TYPE_VEC2, "[1 1]", "" )
		SHADER_PARAM( TIME, SHADER_PARAM_TYPE_FLOAT, "", "" )
		SHADER_PARAM( WATERDEPTH, SHADER_PARAM_TYPE_FLOAT, "", "" )
		SHADER_PARAM( CHEAPWATERSTARTDISTANCE, SHADER_PARAM_TYPE_FLOAT, "", "This is the distance from the eye in inches that the shader should start transitioning to a cheaper water shader." )
		SHADER_PARAM( CHEAPWATERENDDISTANCE, SHADER_PARAM_TYPE_FLOAT, "", "This is the distance from the eye in inches that the shader should finish transitioning to a cheaper water shader." )
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "env_cubemap", "envmap" )
		SHADER_PARAM( ENVMAPFRAME, SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( FOGCOLOR, SHADER_PARAM_TYPE_COLOR, "", "" )
		SHADER_PARAM( FORCECHEAP, SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( FORCEEXPENSIVE, SHADER_PARAM_TYPE_BOOL, "", "" )
		SHADER_PARAM( REFLECTENTITIES, SHADER_PARAM_TYPE_BOOL, "", "" )
		SHADER_PARAM( REFLECTBLENDFACTOR, SHADER_PARAM_TYPE_FLOAT, "1.0", "" )
		SHADER_PARAM( NOFRESNEL, SHADER_PARAM_TYPE_BOOL, "0", "" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
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
		if( !params[FOGCOLOR]->IsDefined() )
		{
			params[FOGCOLOR]->SetVecValue( 1.0f, 0.0f, 0.0f );
			Warning( "material %s needs to have a $fogcolor.\n", pMaterialName );
		}
		if( !params[REFLECTENTITIES]->IsDefined() )
		{
			params[REFLECTENTITIES]->SetIntValue( 0 );
		}
		if( !params[FORCEEXPENSIVE]->IsDefined() )
		{
			params[FORCEEXPENSIVE]->SetIntValue( 0 );
		}
		if( !params[REFLECTBLENDFACTOR]->IsDefined() )
		{
			params[REFLECTBLENDFACTOR]->SetFloatValue( 1.0f );
		}
		if( params[FORCEEXPENSIVE]->GetIntValue() && params[FORCECHEAP]->GetIntValue() )
		{
			params[FORCEEXPENSIVE]->SetIntValue( 0 );
		}
	}

	SHADER_FALLBACK
	{
		if ( IsPC() && ( g_pHardwareConfig->GetDXSupportLevel() < 80 ||	!g_pHardwareConfig->HasProjectedBumpEnv() ) )
		{
			return "SDK_Water_DX60";
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
		if (params[BUMPMAP]->IsDefined() )
		{
			LoadTexture( BUMPMAP );
		}
		if (params[ENVMAP]->IsDefined() )
		{
			LoadCubeMap( ENVMAP );
		}
		if (params[NORMALMAP]->IsDefined() )
		{
			LoadBumpMap( NORMALMAP );
		}
	}

	inline void SetCheapWaterFactors( IMaterialVar **params, IShaderDynamicAPI* pShaderAPI, int nConstantReg )
	{
		float flCheapWaterStartDistance = params[CHEAPWATERSTARTDISTANCE]->GetFloatValue();
		float flCheapWaterEndDistance = params[CHEAPWATERENDDISTANCE]->GetFloatValue();
		float flCheapWaterConstants[4] = 
		{ 
			flCheapWaterStartDistance, 
			1.0f / ( flCheapWaterEndDistance - flCheapWaterStartDistance ), 
			0.0f, 
			0.0f 
		};
		pShaderAPI->SetVertexShaderConstant( nConstantReg, flCheapWaterConstants );
	}

	inline void DrawReflection( IMaterialVar **params, IShaderShadow* pShaderShadow, 
		                        IShaderDynamicAPI* pShaderAPI, bool bBlendReflection )
	{
		SHADOW_STATE
		{
			SetInitialShadowState( );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE2, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE3, true );
#ifdef _XBOX
			pShaderShadow->SetColorSign( SHADER_TEXTURE_STAGE0, true );
#endif
			int fmt = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_TANGENT_S | VERTEX_TANGENT_T;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0, 0 );
			if( bBlendReflection )
			{
				EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			}

			sdk_water_vs11_Static_Index vshIndex;
			pShaderShadow->SetVertexShader( "SDK_Water_vs11", vshIndex.GetIndex() );

			pShaderShadow->SetPixelShader( "SDK_WaterReflect_ps11", 0 );

			FogToFogColor();
		}
		DYNAMIC_STATE
		{
			pShaderAPI->SetDefaultState();
#ifndef _XBOX
			// The dx9.0c runtime says that we shouldn't have a non-zero dimension when using vertex and pixel shaders.
			pShaderAPI->SetTextureTransformDimension( 1, 0, true );
#else
			// xboxissue - projected texture coord not available with texbem
			// divide must be in vsh
#endif
			float fReflectionAmount = params[REFLECTAMOUNT]->GetFloatValue();
			pShaderAPI->SetBumpEnvMatrix( 1, fReflectionAmount, 0.0f, 0.0f, fReflectionAmount );

			BindTexture( SHADER_TEXTURE_STAGE0, BUMPMAP, BUMPFRAME );
			BindTexture( SHADER_TEXTURE_STAGE1, REFLECTTEXTURE, -1 );
			BindTexture( SHADER_TEXTURE_STAGE2, NORMALMAP, BUMPFRAME );
			pShaderAPI->BindNormalizationCubeMap( SHADER_TEXTURE_STAGE3 );
			pShaderAPI->SetVertexShaderIndex( 0 );

			SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_1, BUMPTRANSFORM );

			// used to invert y
			float c[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_4, c, 1 );

			SetPixelShaderConstant( 0, REFLECTTINT );

			sdk_water_vs11_Dynamic_Index vshIndex;
			vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
		}
		Draw();
	}
	
	inline void DrawRefraction( IMaterialVar **params, IShaderShadow* pShaderShadow, 
		                        IShaderDynamicAPI* pShaderAPI )
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
			int fmt = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_TANGENT_S | VERTEX_TANGENT_T;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0, 0 );

			sdk_water_vs11_Static_Index vshIndex;
			pShaderShadow->SetVertexShader( "SDK_Water_vs11", vshIndex.GetIndex() );

			pShaderShadow->SetPixelShader( "SDK_WaterRefract_ps11", 0 );
			FogToFogColor();
		}
		DYNAMIC_STATE
		{
			// The dx9.0c runtime says that we shouldn't have a non-zero dimension when using vertex and pixel shaders.
			pShaderAPI->SetTextureTransformDimension( 1, 0, true );
			float fRefractionAmount = params[REFRACTAMOUNT]->GetFloatValue();
			pShaderAPI->SetBumpEnvMatrix( 1, fRefractionAmount, 0.0f, 0.0f, fRefractionAmount );
			BindTexture( SHADER_TEXTURE_STAGE0, BUMPMAP, BUMPFRAME );
			BindTexture( SHADER_TEXTURE_STAGE1, REFRACTTEXTURE );

			SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_1, BUMPTRANSFORM );

			// used to invert y
			float c[4] = { 0.0f, 0.0f, 0.0f, -1.0f };
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_4, c, 1 );

			SetPixelShaderConstant( 0, REFRACTTINT );

			sdk_water_vs11_Dynamic_Index vshIndex;
			vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
		}
		Draw();
	}

	inline void DrawRefractionForFresnel( IMaterialVar **params, IShaderShadow* pShaderShadow, 
		                        IShaderDynamicAPI* pShaderAPI )
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableAlphaWrites( true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE2, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE3, true );
#ifdef _XBOX
			pShaderShadow->SetColorSign( SHADER_TEXTURE_STAGE0, true );
#endif
			int fmt = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_TANGENT_S | VERTEX_TANGENT_T;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0, 0 );

			sdk_water_vs11_Static_Index vshIndex;
			pShaderShadow->SetVertexShader( "SDK_Water_vs11", vshIndex.GetIndex() );

			pShaderShadow->SetPixelShader( "SDK_WaterRefractFresnel_ps11", 0 );
			FogToFogColor();
		}
		DYNAMIC_STATE
		{
			// The dx9.0c runtime says that we shouldn't have a non-zero dimension when using vertex and pixel shaders.
			pShaderAPI->SetTextureTransformDimension( 1, 0, true );
			float fRefractionAmount = params[REFRACTAMOUNT]->GetFloatValue();
			pShaderAPI->SetBumpEnvMatrix( 1, fRefractionAmount, 0.0f, 0.0f, fRefractionAmount );
			BindTexture( SHADER_TEXTURE_STAGE0, BUMPMAP, BUMPFRAME );
			BindTexture( SHADER_TEXTURE_STAGE1, REFRACTTEXTURE );
			BindTexture( SHADER_TEXTURE_STAGE2, NORMALMAP, BUMPFRAME );
			s_pShaderAPI->BindNormalizationCubeMap( SHADER_TEXTURE_STAGE3 );

			SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_1, BUMPTRANSFORM );
			SetCheapWaterFactors( params, pShaderAPI, VERTEX_SHADER_SHADER_SPECIFIC_CONST_3 );

			// used to invert y
			float c[4] = { 0.0f, 0.0f, 0.0f, -1.0f };
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_4, c, 1 );

			SetPixelShaderConstant( 0, REFRACTTINT );

			sdk_water_vs11_Dynamic_Index vshIndex;
			vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
		}
		Draw();
	}

	inline void DrawCheapWater( IMaterialVar **params, IShaderShadow* pShaderShadow, 
		                        IShaderDynamicAPI* pShaderAPI, bool bBlend, bool bBlendFresnel, bool bNoPerVertexFresnel )
	{
		SHADOW_STATE
		{
			SetInitialShadowState( );

			// In edit mode, use nocull
			if ( UsingEditor( params ) )
			{
				s_pShaderShadow->EnableCulling( false );
			}

			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE3, true );
			if( bBlend )
			{
				if ( bBlendFresnel )
				{
					EnableAlphaBlending( SHADER_BLEND_DST_ALPHA, SHADER_BLEND_ONE_MINUS_DST_ALPHA );
				}
				else
				{
					EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
				}
			}

			pShaderShadow->VertexShaderVertexFormat( 
				VERTEX_POSITION | VERTEX_NORMAL | VERTEX_TANGENT_S |
				VERTEX_TANGENT_T, 1, 0, 0, 0 );

			if( bNoPerVertexFresnel )
			{
				sdk_watercheap_vs11_Static_Index vshIndex;
				pShaderShadow->SetVertexShader( "SDK_WaterCheap_vs11", vshIndex.GetIndex() );
			}
			else
			{
				sdk_watercheappervertexfresnel_vs11_Static_Index vshIndex;
				pShaderShadow->SetVertexShader( "SDK_WaterCheapPerVertexFresnel_vs11", vshIndex.GetIndex() );
			}

			static const char *s_pPixelShaderName[] = 
			{
				"SDK_WaterCheapOpaque_ps11",
				"SDK_WaterCheap_ps11",
				"SDK_WaterCheapNoFresnelOpaque_ps11",
				"SDK_WaterCheapNoFresnel_ps11",
			};

			int nPshIndex = 0;
			if ( bBlend ) nPshIndex |= 0x1;
			if ( bNoPerVertexFresnel ) nPshIndex |= 0x2;
			pShaderShadow->SetPixelShader( s_pPixelShaderName[nPshIndex] );

			FogToFogColor();
		}
		DYNAMIC_STATE
		{
			pShaderAPI->SetDefaultState();
			BindTexture( SHADER_TEXTURE_STAGE0, NORMALMAP, BUMPFRAME );
			BindTexture( SHADER_TEXTURE_STAGE3, ENVMAP, ENVMAPFRAME );

			if( bBlend && !bBlendFresnel )
			{
				SetCheapWaterFactors( params, pShaderAPI, VERTEX_SHADER_SHADER_SPECIFIC_CONST_2 );
			}
			else
			{
				float flCheapWaterConstants[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
				pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_2, flCheapWaterConstants );
			}

			SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, BUMPTRANSFORM );

			SetPixelShaderConstant( 0, FOGCOLOR );
			SetPixelShaderConstant( 1, REFLECTTINT, REFLECTBLENDFACTOR );

			if( bNoPerVertexFresnel )
			{
				sdk_watercheap_vs11_Dynamic_Index vshIndex;
				vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
				pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
			}
			else
			{
				sdk_watercheappervertexfresnel_vs11_Dynamic_Index vshIndex;
				vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
				pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
			}
		}
		Draw();
	}

	SHADER_DRAW
	{
		// NOTE: Here's what all this means.
		// 1) ForceCheap means use env_cubemap only
		// 2) ForceExpensive means do real reflection instead of env_cubemap.
		// By default, it will do refraction and use env_cubemap for the reflection.
		// If dest alpha is available, it will also use dest alpha for a fresnel term.
		// otherwise there will be no fresnel term as it looks bizzare.

		bool bBlendReflection = false;
		bool bForceCheap = params[FORCECHEAP]->GetIntValue() != 0 || UsingEditor( params );
		bool bForceExpensive = !bForceCheap && (params[FORCEEXPENSIVE]->GetIntValue() != 0);
		bool bRefraction = params[REFRACTTEXTURE]->IsTexture();
#ifndef _XBOX
		bool bReflection = bForceExpensive && params[REFLECTTEXTURE]->IsTexture();
#else
		bool bReflection = false;
#endif
		bool bReflectionUseFresnel = false;

		// Can't do fresnel when forcing cheap or if there's no refraction
		if( !bForceCheap )
		{
			if( bRefraction )
			{
				// NOTE: Expensive reflection does the fresnel correctly per-pixel
				if ( g_pHardwareConfig->HasDestAlphaBuffer() && !bReflection && !params[NOFRESNEL]->GetIntValue() )
				{
#ifndef _XBOX
					DrawRefractionForFresnel( params, pShaderShadow, pShaderAPI );
#endif
					bReflectionUseFresnel = true;
				}
				else
				{
#ifndef _XBOX
					DrawRefraction( params, pShaderShadow, pShaderAPI );
#endif
				}
				bBlendReflection = true;
			}
#ifndef _XBOX
			if( bReflection )
			{
				DrawReflection( params, pShaderShadow, pShaderAPI, bBlendReflection );
			}
#endif
		}

		// Use $decal to see if we are a decal or not. . if we are, then don't bother
		// drawing the cheap version for now since we don't have access to env_cubemap
		if( !bReflection && params[ENVMAP]->IsTexture() && !IS_FLAG_SET( MATERIAL_VAR_DECAL ) )
		{
			bool bNoPerVertexFresnel = ( params[NOFRESNEL]->GetIntValue() || bReflectionUseFresnel || bForceCheap || !bRefraction );
			DrawCheapWater( params, pShaderShadow, pShaderAPI, bBlendReflection, bReflectionUseFresnel, bNoPerVertexFresnel );
		}
		else
		{
			if ( !bForceCheap )
			{
#ifdef _XBOX
				SHADOW_STATE
				{
					pShaderShadow->EnableAlphaWrites( false );
					pShaderShadow->EnableColorWrites( false );
					pShaderShadow->DepthFunc( SHADER_DEPTHFUNC_ALWAYS );
					pShaderShadow->EnableDepthWrites( false );
					pShaderShadow->VertexShaderVertexFormat( VERTEX_POSITION, 0, 0, 0, 0 );
				}
				DYNAMIC_STATE
				{
				}
				Draw();
#endif
			}
		}
	}
END_SHADER

