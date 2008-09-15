//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
// The interface plug-in shader DLLs must implement
//=============================================================================

#ifndef ISHADERV2_H
#define ISHADERV2_H

#ifdef _WIN32
#pragma once
#endif


#include "materialsystem/imaterialsystem.h"
#include "materialsystem/ishaderapi.h"


//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
class IMaterialVar;
class IShaderShadow;
class IShaderDynamicAPI;
class IShaderInit;


namespace ShaderV2
{

//-----------------------------------------------------------------------------
// Information about each shader parameter
//-----------------------------------------------------------------------------
struct ShaderParamInfo_t
{
	const char *m_pName;
	const char *m_pHelp;
	ShaderParamType_t m_Type;
	const char *m_pDefaultValue;
};


//-----------------------------------------------------------------------------
// Standard vertex shader constants
//-----------------------------------------------------------------------------
enum
{
	// Standard vertex shader constants
	VERTEX_SHADER_MATH_CONSTANTS0 = 0,
	VERTEX_SHADER_MATH_CONSTANTS1 = 1,
	VERTEX_SHADER_CAMERA_POS = 2,
	VERTEX_SHADER_LIGHT_INDEX = 3,
	VERTEX_SHADER_MODELVIEWPROJ = 4,
	VERTEX_SHADER_VIEWPROJ = 8,
	VERTEX_SHADER_HALFLAMBERT = 12,
	VERTEX_SHADER_FOG_PARAMS = 16,
	VERTEX_SHADER_VIEWMODEL = 17,
	VERTEX_SHADER_AMBIENT_LIGHT = 21,
	VERTEX_SHADER_LIGHTS = 27,
	VERTEX_SHADER_SHADER_SPECIFIC_CONST_0 = 38,
	VERTEX_SHADER_SHADER_SPECIFIC_CONST_1 = 39,
	VERTEX_SHADER_SHADER_SPECIFIC_CONST_2 = 40,
	VERTEX_SHADER_SHADER_SPECIFIC_CONST_3 = 41,
	VERTEX_SHADER_SHADER_SPECIFIC_CONST_4 = 42,
	VERTEX_SHADER_SHADER_SPECIFIC_CONST_5 = 43,
	VERTEX_SHADER_SHADER_SPECIFIC_CONST_6 = 44,
	VERTEX_SHADER_SHADER_SPECIFIC_CONST_7 = 45,
	VERTEX_SHADER_SHADER_SPECIFIC_CONST_8 = 46,
	VERTEX_SHADER_SHADER_SPECIFIC_CONST_9 = 47,
	VERTEX_SHADER_MODEL = 48,
};

//-----------------------------------------------------------------------------
// The public methods exposed by each shader
//-----------------------------------------------------------------------------
abstract_class IShader
{
public:
	// Returns the shader name
	virtual char const* GetName( ) const = 0;

	// returns the shader fallbacks
	virtual char const* GetFallbackShader( IMaterialVar** params ) const = 0;

	// Shader parameters
	virtual int GetNumParams( ) const = 0;

	// These functions must be implemented by the shader
	virtual void InitShaderParams( IMaterialVar** ppParams, const char *pMaterialName ) = 0;
	virtual void InitShaderInstance( IMaterialVar** ppParams, IShaderInit *pShaderInit, const char *pMaterialName, const char *pTextureGroupName ) = 0;
	virtual void DrawElements( IMaterialVar **params, int nModulationFlags,
		IShaderShadow* pShaderShadow, IShaderDynamicAPI* pShaderAPI ) = 0;

//	virtual const ShaderParamInfo_t& GetParamInfo( int paramIndex ) const = 0;
	virtual char const* GetParamName( int paramIndex ) const = 0;
	virtual char const* GetParamHelp( int paramIndex ) const = 0;
	virtual ShaderParamType_t GetParamType( int paramIndex ) const = 0;
	virtual char const* GetParamDefault( int paramIndex ) const = 0;

	// Returns the software vertex shader (if any)
	virtual	const SoftwareVertexShader_t GetSoftwareVertexShader() const = 0;

	// FIXME: Figure out a better way to do this?
	virtual int ComputeModulationFlags( IMaterialVar** params, bool bFlashlightMode ) = 0;
	virtual bool NeedsPowerOfTwoFrameBufferTexture( IMaterialVar **params ) const = 0;
	virtual bool NeedsFullFrameBufferTexture( IMaterialVar **params ) const = 0;
};


//-----------------------------------------------------------------------------
// Shader dictionaries defined in DLLs
//-----------------------------------------------------------------------------
enum PrecompiledShaderType_t
{
	PRECOMPILED_VERTEX_SHADER = 0,
	PRECOMPILED_PIXEL_SHADER,

	PRECOMPILED_SHADER_TYPE_COUNT,
};


//-----------------------------------------------------------------------------
// Flags field of PrecompiledShader_t
//-----------------------------------------------------------------------------
enum
{
	// runtime flags
	SHADER_DYNAMIC_COMPILE_IS_HLSL = 0x1,
	SHADER_FAILED_LOAD = 0x2,
};

} // end namespace

#endif // ISHADERV2_H
