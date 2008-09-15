//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"

#include "unlittwotexture.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_VS_SHADER( UnlitTwoTexture, 
			  "Help for UnlitTwoTexture" )
			  
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( TEXTURE2, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "second texture" )
		SHADER_PARAM( FRAME2, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $texture2" )
		SHADER_PARAM( TEXTURE2TRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$texture2 texcoord transform" )
	END_SHADER_PARAMS

	SHADER_FALLBACK
	{
		if ( IsPC() && !g_pHardwareConfig->SupportsVertexAndPixelShaders())
		{
			return "UnlitTwoTexture_DX6";
		}
		return 0;
	}

	SHADER_INIT
	{
		if (params[BASETEXTURE]->IsDefined())
			LoadTexture( BASETEXTURE );
		if (params[TEXTURE2]->IsDefined())
			LoadTexture( TEXTURE2 );
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );

			// Either we've got a constant modulation
			bool isTranslucent = IsAlphaModulating();

			// Or we've got a texture alpha on either texture
			isTranslucent = isTranslucent || TextureIsTranslucent( BASETEXTURE, true ) ||
				TextureIsTranslucent( TEXTURE2, true );

			if ( isTranslucent )
			{
				if ( IS_FLAG_SET(MATERIAL_VAR_ADDITIVE) )
				{
					EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE );
				}
				else
				{
					EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
				}
			}
			else
			{
				if ( IS_FLAG_SET(MATERIAL_VAR_ADDITIVE) )
				{
					EnableAlphaBlending( SHADER_BLEND_ONE, SHADER_BLEND_ONE );
				}
				else
				{
					DisableAlphaBlending( );
				}
			}

			int fmt = VERTEX_POSITION | VERTEX_NORMAL;
			bool bDoSkin = IS_FLAG_SET(MATERIAL_VAR_MODEL);
			if (bDoSkin)
			{
				fmt |= VERTEX_BONE_INDEX;
			}
			if (IS_FLAG_SET( MATERIAL_VAR_VERTEXCOLOR ))
			{
				fmt |= VERTEX_COLOR;
			}

			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, bDoSkin ? 3 : 0, 0 );

			unlittwotexture_Static_Index vshIndex;
			pShaderShadow->SetVertexShader( "UnlitTwoTexture", vshIndex.GetIndex() );

			pShaderShadow->SetPixelShader( "UnlitTwoTexture" );
			DefaultFog();
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
			BindTexture( SHADER_TEXTURE_STAGE1, TEXTURE2, FRAME2 );
			SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, BASETEXTURETRANSFORM );
			SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_2, TEXTURE2TRANSFORM );
			SetModulationVertexShaderDynamicState();

			unlittwotexture_Dynamic_Index vshIndex;
			vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
#if !defined( _XBOX )
			vshIndex.SetNUM_BONES( pShaderAPI->GetCurrentNumBones() );
#else
			vshIndex.SetSKINNING( pShaderAPI->GetCurrentNumBones() > 0 );
#endif
			pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
		}
		Draw();
	}
END_SHADER
