//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( SDK_UnlitGeneric, SDK_UnlitGeneric_DX8 )

BEGIN_VS_SHADER( SDK_UnlitGeneric_DX8,
			  "Help for SDK_UnlitGeneric_DX8" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( DETAIL, SHADER_PARAM_TYPE_TEXTURE, "shadertest/detail", "detail texture" )
		SHADER_PARAM( DETAILSCALE, SHADER_PARAM_TYPE_FLOAT, "4", "scale of the detail texture" )
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_env", "envmap" )
		SHADER_PARAM( ENVMAPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "" )
		SHADER_PARAM( ENVMAPMASK, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_envmask", "envmap mask" )
		SHADER_PARAM( ENVMAPMASKFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "" )
		SHADER_PARAM( ENVMAPMASKSCALE, SHADER_PARAM_TYPE_FLOAT, "1", "envmap mask scale" )
		SHADER_PARAM( ENVMAPTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "envmap tint" )
		SHADER_PARAM( ENVMAPOPTIONAL, SHADER_PARAM_TYPE_BOOL, "0", "Make the envmap only apply to dx9 and higher hardware" )
		SHADER_PARAM( ALPHATESTREFERENCE, SHADER_PARAM_TYPE_FLOAT, "0.7", "" )	
	END_SHADER_PARAMS

	SHADER_FALLBACK
	{
		if ( IsPC() && !g_pHardwareConfig->SupportsVertexAndPixelShaders())
		{
			return "SDK_UnlitGeneric_DX6";
		}
		return 0;
	}

	SHADER_INIT_PARAMS()
	{
		InitParamsUnlitGeneric_DX8(
			BASETEXTURE, DETAILSCALE, ENVMAPOPTIONAL,
			ENVMAP, ENVMAPTINT, ENVMAPMASKSCALE );
	}

	SHADER_INIT
	{
		InitUnlitGeneric_DX8( BASETEXTURE, DETAIL, ENVMAP, ENVMAPMASK );
	}

	SHADER_DRAW
	{
		bool doSkin = IS_FLAG_SET( MATERIAL_VAR_MODEL );
		VertexShaderUnlitGenericPass( 
			doSkin, BASETEXTURE, FRAME, BASETEXTURETRANSFORM,
			DETAIL, DETAILSCALE, true, ENVMAP, ENVMAPFRAME, ENVMAPMASK,
			ENVMAPMASKFRAME, ENVMAPMASKSCALE, ENVMAPTINT, ALPHATESTREFERENCE );
	}
END_SHADER

