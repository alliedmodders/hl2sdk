//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"

BEGIN_VS_SHADER( UnlitGeneric,
			  "Help for UnlitGeneric" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( BRIGHTNESS, SHADER_PARAM_TYPE_TEXTURE, "shadertest/detail", "HDR brightness texture" )
		SHADER_PARAM( DETAIL, SHADER_PARAM_TYPE_TEXTURE, "shadertest/detail", "detail texture" )
		SHADER_PARAM( DETAILFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $detail" )
		SHADER_PARAM( DETAILSCALE, SHADER_PARAM_TYPE_FLOAT, "4", "scale of the detail texture" )
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_env", "envmap" )
		SHADER_PARAM( ENVMAPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "envmap frame number" )
		SHADER_PARAM( ENVMAPMASK, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_envmask", "envmap mask" )
		SHADER_PARAM( ENVMAPMASKFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "" )
		SHADER_PARAM( ENVMAPMASKTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$envmapmask texcoord transform" )
		SHADER_PARAM( ENVMAPTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "envmap tint" )
		SHADER_PARAM( ENVMAPCONTRAST, SHADER_PARAM_TYPE_FLOAT, "0.0", "contrast 0 == normal 1 == color*color" )
		SHADER_PARAM( ENVMAPSATURATION, SHADER_PARAM_TYPE_FLOAT, "1.0", "saturation 0 == greyscale 1 == normal" )
	END_SHADER_PARAMS

	SHADER_FALLBACK
	{
		if( g_pHardwareConfig->GetHDRType() == HDR_TYPE_NONE )
		{
			return "UnlitGeneric_DX9";
		}
		return 0;
	}

	SHADER_INIT_PARAMS()
	{
		InitParamsUnlitGeneric_DX9(
			BASETEXTURE,	FRAME,				BASETEXTURETRANSFORM,
			DETAIL,			DETAILFRAME,		DETAILSCALE,
			ENVMAP,			ENVMAPFRAME,
			ENVMAPMASK,		ENVMAPMASKFRAME,	ENVMAPMASKTRANSFORM,
			ENVMAPTINT,		ENVMAPCONTRAST,		ENVMAPSATURATION );
	}

	SHADER_INIT
	{
		InitUnlitGeneric_HDR(
			BASETEXTURE,	FRAME,				BASETEXTURETRANSFORM, 
			BRIGHTNESS,
			DETAIL,			DETAILFRAME,		DETAILSCALE,
			ENVMAP,			ENVMAPFRAME,
			ENVMAPMASK,		ENVMAPMASKFRAME,	ENVMAPMASKTRANSFORM,
			ENVMAPTINT,		ENVMAPCONTRAST,		ENVMAPSATURATION );
	}

	SHADER_DRAW
	{
		DrawUnlitGeneric_HDR( 
			BASETEXTURE,	FRAME,				BASETEXTURETRANSFORM, 
			BRIGHTNESS,
			DETAIL,			DETAILFRAME,		DETAILSCALE,
			ENVMAP,			ENVMAPFRAME,
			ENVMAPMASK,		ENVMAPMASKFRAME,	ENVMAPMASKTRANSFORM,
			ENVMAPTINT,		ENVMAPCONTRAST,		ENVMAPSATURATION );
	}
END_SHADER

