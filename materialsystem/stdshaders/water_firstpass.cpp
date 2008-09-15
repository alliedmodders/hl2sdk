//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//


#include "BaseVSShader.h"
#include "waterfirstpass_vs11.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_VS_SHADER( Water_FirstPass, "Help for WaterFistPass" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( NORMALMAP, SHADER_PARAM_TYPE_TEXTURE, "", "normal map" )
		SHADER_PARAM( BUMPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $bumpmap" )
		SHADER_PARAM( BUMPTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$bumpmap texcoord transform" )
		SHADER_PARAM( CHEAPWATERSTARTDISTANCE, SHADER_PARAM_TYPE_FLOAT, "", "This is the distance from the eye in inches that the shader should start transitioning to a cheaper water shader." )
		SHADER_PARAM( CHEAPWATERENDDISTANCE, SHADER_PARAM_TYPE_FLOAT, "", "This is the distance from the eye in inches that the shader should finish transitioning to a cheaper water shader." )
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
	}

	SHADER_FALLBACK
	{
		return 0;
	}

	SHADER_INIT
	{
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

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableAlphaWrites( true );
			pShaderShadow->EnableColorWrites( false );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
			int fmt = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_TANGENT_S | VERTEX_TANGENT_T;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0, 0 );

			pShaderShadow->SetVertexShader( "WaterFirstPass_vs11", 0 );
			pShaderShadow->SetPixelShader( "WaterFirstPass_ps11", 0 );
			DisableFog();
		}
		DYNAMIC_STATE
		{
			waterfirstpass_vs11_Dynamic_Index vshIndex;
			pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );

			BindTexture( SHADER_TEXTURE_STAGE0, NORMALMAP, BUMPFRAME );
			pShaderAPI->BindNormalizationCubeMap( SHADER_TEXTURE_STAGE1 );

			SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_1, BUMPTRANSFORM );
			SetCheapWaterFactors( params, pShaderAPI, VERTEX_SHADER_SHADER_SPECIFIC_CONST_3 );
		}
		Draw();
	}
END_SHADER

