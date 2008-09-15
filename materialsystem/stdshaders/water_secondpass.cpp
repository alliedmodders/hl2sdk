//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//


#include "BaseVSShader.h"
#include "watersecondpass_vs11.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_VS_SHADER( Water_SecondPass, "Help for WaterSecondPass" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( BUMPMAP, SHADER_PARAM_TYPE_TEXTURE, "", "dudv bump map" )
		SHADER_PARAM( REFRACTTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "refraction texture" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
	}

	SHADER_FALLBACK
	{
		return 0;
	}

	SHADER_INIT
	{
		if( params[BUMPMAP]->IsDefined() )
		{
			LoadTexture( BUMPMAP );
		}
		if( params[REFRACTTEXTURE]->IsDefined() )
		{
			LoadTexture( REFRACTTEXTURE );
		}
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableAlphaWrites( false );
			pShaderShadow->EnableColorWrites( true );
			pShaderShadow->DepthFunc( SHADER_DEPTHFUNC_ALWAYS );
			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
			   
			pShaderShadow->VertexShaderVertexFormat( VERTEX_POSITION, 1, 0, 0, 0 );
			 
			pShaderShadow->SetVertexShader( "WaterSecondPass_vs11", 0 );
			pShaderShadow->SetPixelShader( "WaterSecondPass_ps11", 0 );
			DisableFog();
		}
		DYNAMIC_STATE
		{
			ITexture *pTexture = params[BUMPMAP]->GetTextureValue();
			int w = pTexture->GetActualWidth();
			int h = pTexture->GetActualHeight();

			pShaderAPI->SetBumpEnvMatrix( 1, 16.0f / (float)w, 0.0f, 0.0f, 16.0f /(float)h );
			BindTexture( SHADER_TEXTURE_STAGE0, BUMPMAP, 0 );
			BindTexture( SHADER_TEXTURE_STAGE1, REFRACTTEXTURE, 0 );
			SetWaterFogColorPixelShaderConstantGamma( 0 );

			Vector4D vec( 1.0f, 0.0f, 0.0f, 0.0f );
			pShaderAPI->SetPixelShaderConstant( 1, vec.Base() );	

			// Used deal with the fact that the dudv is being treated 
			// as an unsigned value
			vec.Init( -8.0f / (float)w, -8.0f / (float)h, 0.0f, 0.0f );
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_1, vec.Base() );	

			watersecondpass_vs11_Dynamic_Index vshIndex;
			pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
		}
		Draw();
	}
END_SHADER

