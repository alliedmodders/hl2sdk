//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
// This is what all vs/ps (dx8+) shaders inherit from.
//=============================================================================//
#if !defined(_STATIC_LINKED) || defined(STDSHADER_DX8_DLL_EXPORT) || defined(STDSHADER_DX9_DLL_EXPORT)

#include "basevsshader.h"
#include "vmatrix.h"
#include "bumpvects.h"

#include "ConVar.h"

#ifdef HDR
#include "SDK_vertexlit_and_unlit_generic_hdr_ps20.inc"
#endif

#include "SDK_lightmappedgeneric_flashlight_vs11.inc"
#ifndef _XBOX
#ifdef STDSHADER_DX9_DLL_EXPORT
#include "SDK_lightmappedgeneric_flashlight_vs20.inc"
#endif
#endif
#include "SDK_flashlight_ps11.inc"
#ifndef _XBOX
#ifdef STDSHADER_DX9_DLL_EXPORT
#include "SDK_flashlight_ps20.inc"
#endif
#endif
#include "SDK_unlitgeneric_vs11.inc"
#ifndef _XBOX
#include "SDK_VertexLitGeneric_EnvmappedBumpmap_NoLighting_ps14.inc"
#endif
#include "SDK_VertexLitGeneric_EnvmappedBumpmap_NoLighting.inc"
#include "SDK_vertexlitgeneric_flashlight_vs11.inc"
#include "SDK_lightmappedgeneric_basetexture.inc"
#ifndef _XBOX
#include "SDK_lightmappedGeneric_bumpmappedlightmap_base_ps14.inc"
#include "SDK_lightmappedGeneric_bumpmappedlightmap_blend_ps14.inc"
#include "SDK_lightmappedgeneric_bumpmappedenvmap_ps14.inc"
#endif
#include "SDK_lightmappedgeneric_bumpmappedenvmap.inc"
#include "SDK_lightmappedgeneric_basetextureblend.inc"
#include "SDK_lightmappedgeneric_bumpmappedlightmap.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// These functions are to be called from the shaders.

//-----------------------------------------------------------------------------
// Pixel and vertex shader constants....
//-----------------------------------------------------------------------------
void CBaseVSShader::SetPixelShaderConstant( int pixelReg, int constantVar, int constantVar2 )
{
	Assert( !IsSnapshotting() );
	if ((!s_ppParams) || (constantVar == -1) || (constantVar2 == -1))
		return;

	IMaterialVar* pPixelVar = s_ppParams[constantVar];
	Assert( pPixelVar );
	IMaterialVar* pPixelVar2 = s_ppParams[constantVar2];
	Assert( pPixelVar2 );

	float val[4];
	if (pPixelVar->GetType() == MATERIAL_VAR_TYPE_VECTOR)
	{
		pPixelVar->GetVecValue( val, 3 );
	}
	else
	{
		val[0] = val[1] = val[2] = pPixelVar->GetFloatValue();
	}

	val[3] = pPixelVar2->GetFloatValue();
	s_pShaderAPI->SetPixelShaderConstant( pixelReg, val );	
}

void CBaseVSShader::SetPixelShaderConstantGammaToLinear( int pixelReg, int constantVar, int constantVar2 )
{
	Assert( !IsSnapshotting() );
	if ((!s_ppParams) || (constantVar == -1) || (constantVar2 == -1))
		return;

	IMaterialVar* pPixelVar = s_ppParams[constantVar];
	Assert( pPixelVar );
	IMaterialVar* pPixelVar2 = s_ppParams[constantVar2];
	Assert( pPixelVar2 );

	float val[4];
	if (pPixelVar->GetType() == MATERIAL_VAR_TYPE_VECTOR)
	{
		pPixelVar->GetVecValue( val, 3 );
	}
	else
	{
		val[0] = val[1] = val[2] = pPixelVar->GetFloatValue();
	}

	val[3] = pPixelVar2->GetFloatValue();
	val[0] = GammaToLinear( val[0] );
	val[1] = GammaToLinear( val[1] );
	val[2] = GammaToLinear( val[2] );
	s_pShaderAPI->SetPixelShaderConstant( pixelReg, val );	
}

void CBaseVSShader::SetPixelShaderConstant( int pixelReg, int constantVar )
{
	Assert( !IsSnapshotting() );
	if ((!s_ppParams) || (constantVar == -1))
		return;

	IMaterialVar* pPixelVar = s_ppParams[constantVar];
	Assert( pPixelVar );

	float val[4];
	if (pPixelVar->GetType() == MATERIAL_VAR_TYPE_VECTOR)
		pPixelVar->GetVecValue( val, 4 );
	else
		val[0] = val[1] = val[2] = val[3] = pPixelVar->GetFloatValue();
	s_pShaderAPI->SetPixelShaderConstant( pixelReg, val );	
}

void CBaseVSShader::SetPixelShaderConstantGammaToLinear( int pixelReg, int constantVar )
{
	Assert( !IsSnapshotting() );
	if ((!s_ppParams) || (constantVar == -1))
		return;

	IMaterialVar* pPixelVar = s_ppParams[constantVar];
	Assert( pPixelVar );

	float val[4];
	if (pPixelVar->GetType() == MATERIAL_VAR_TYPE_VECTOR)
		pPixelVar->GetVecValue( val, 4 );
	else
		val[0] = val[1] = val[2] = val[3] = pPixelVar->GetFloatValue();
	val[0] = GammaToLinear( val[0] );
	val[1] = GammaToLinear( val[1] );
	val[2] = GammaToLinear( val[2] );
	s_pShaderAPI->SetPixelShaderConstant( pixelReg, val );	
}

void CBaseVSShader::SetVertexShaderConstantGammaToLinear( int var, float const* pVec, int numConst, bool bForce )
{
	int i;
	for( i = 0; i < numConst; i++ )
	{
		float vec[4];
		vec[0] = GammaToLinear( pVec[i*4+0] );
		vec[1] = GammaToLinear( pVec[i*4+1] );
		vec[2] = GammaToLinear( pVec[i*4+2] );
		vec[3] = pVec[i*4+3];
		s_pShaderAPI->SetVertexShaderConstant( var + i, vec, 1, bForce );
	}
}

void CBaseVSShader::SetPixelShaderConstantGammaToLinear( int var, float const* pVec, int numConst, bool bForce )
{
	int i;
	for( i = 0; i < numConst; i++ )
	{
		float vec[4];
		vec[0] = GammaToLinear( pVec[i*4+0] );
		vec[1] = GammaToLinear( pVec[i*4+1] );
		vec[2] = GammaToLinear( pVec[i*4+2] );
		vec[3] = pVec[i*4+3];
		s_pShaderAPI->SetPixelShaderConstant( var + i, vec, 1, bForce );
	}
}

// GR - special version with fix for const/lerp issue
void CBaseVSShader::SetPixelShaderConstantFudge( int pixelReg, int constantVar )
{
	Assert( !IsSnapshotting() );
	if ((!s_ppParams) || (constantVar == -1))
		return;

	IMaterialVar* pPixelVar = s_ppParams[constantVar];
	Assert( pPixelVar );

	float val[4];
	if (pPixelVar->GetType() == MATERIAL_VAR_TYPE_VECTOR)
	{
		pPixelVar->GetVecValue( val, 4 );
		val[0] = val[0] * 0.992f + 0.0078f;
		val[1] = val[1] * 0.992f + 0.0078f;
		val[2] = val[2] * 0.992f + 0.0078f;
		val[3] = val[3] * 0.992f + 0.0078f;
	}
	else
		val[0] = val[1] = val[2] = val[3] = pPixelVar->GetFloatValue() * 0.992f + 0.0078f;
	s_pShaderAPI->SetPixelShaderConstant( pixelReg, val );	
}

void CBaseVSShader::SetVertexShaderConstant( int vertexReg, int constantVar )
{
	Assert( !IsSnapshotting() );
	if ((!s_ppParams) || (constantVar == -1))
		return;

	IMaterialVar* pVertexVar = s_ppParams[constantVar];
	Assert( pVertexVar );

	float val[4];
	if (pVertexVar->GetType() == MATERIAL_VAR_TYPE_VECTOR)
		pVertexVar->GetVecValue( val, 4 );
	else
		val[0] = val[1] = val[2] = val[3] = pVertexVar->GetFloatValue();
	s_pShaderAPI->SetVertexShaderConstant( vertexReg, val );	
}

//-----------------------------------------------------------------------------
// Sets normalized light color for pixel shaders.
//-----------------------------------------------------------------------------
void CBaseVSShader::SetPixelShaderLightColors( int pixelReg )
{
	int i;
	int maxLights = s_pShaderAPI->GetMaxLights();
	for( i = 0; i < maxLights; i++ )
	{
		const LightDesc_t & lightDesc = s_pShaderAPI->GetLight( i );
		if( lightDesc.m_Type != MATERIAL_LIGHT_DISABLE )
		{
			Vector color( lightDesc.m_Color[0], lightDesc.m_Color[1], lightDesc.m_Color[2] );
			VectorNormalize( color );
			float val[4] = { color[0], color[1], color[2], 1.0f };
			s_pShaderAPI->SetPixelShaderConstant( pixelReg + i, val, 1 );
		}
		else
		{
			float zero[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			s_pShaderAPI->SetPixelShaderConstant( pixelReg + i, zero, 1 );
		}
	}
}


//-----------------------------------------------------------------------------
// Sets vertex shader texture transforms
//-----------------------------------------------------------------------------
void CBaseVSShader::SetVertexShaderTextureTranslation( int vertexReg, int translationVar )
{
	float offset[2] = {0, 0};

	IMaterialVar* pTranslationVar = s_ppParams[translationVar];
	if (pTranslationVar)
	{
		if (pTranslationVar->GetType() == MATERIAL_VAR_TYPE_VECTOR)
			pTranslationVar->GetVecValue( offset, 2 );
		else
			offset[0] = offset[1] = pTranslationVar->GetFloatValue();
	}

	Vector4D translation[2];
	translation[0].Init( 1.0f, 0.0f, 0.0f, offset[0] );
	translation[1].Init( 0.0f, 1.0f, 0.0f, offset[1] );
	s_pShaderAPI->SetVertexShaderConstant( vertexReg, translation[0].Base(), 2 ); 
}

void CBaseVSShader::SetVertexShaderTextureScale( int vertexReg, int scaleVar )
{
	float scale[2] = {1, 1};

	IMaterialVar* pScaleVar = s_ppParams[scaleVar];
	if (pScaleVar)
	{
		if (pScaleVar->GetType() == MATERIAL_VAR_TYPE_VECTOR)
			pScaleVar->GetVecValue( scale, 2 );
		else if (pScaleVar->IsDefined())
			scale[0] = scale[1] = pScaleVar->GetFloatValue();
	}

	Vector4D scaleMatrix[2];
	scaleMatrix[0].Init( scale[0], 0.0f, 0.0f, 0.0f );
	scaleMatrix[1].Init( 0.0f, scale[1], 0.0f, 0.0f );
	s_pShaderAPI->SetVertexShaderConstant( vertexReg, scaleMatrix[0].Base(), 2 ); 
}

void CBaseVSShader::SetVertexShaderTextureTransform( int vertexReg, int transformVar )
{
	Vector4D transformation[2];
	IMaterialVar* pTransformationVar = s_ppParams[transformVar];
	if (pTransformationVar && (pTransformationVar->GetType() == MATERIAL_VAR_TYPE_MATRIX))
	{
		const VMatrix &mat = pTransformationVar->GetMatrixValue();
		transformation[0].Init( mat[0][0], mat[0][1], mat[0][2], mat[0][3] );
		transformation[1].Init( mat[1][0], mat[1][1], mat[1][2], mat[1][3] );
	}
	else
	{
		transformation[0].Init( 1.0f, 0.0f, 0.0f, 0.0f );
		transformation[1].Init( 0.0f, 1.0f, 0.0f, 0.0f );
	}
	s_pShaderAPI->SetVertexShaderConstant( vertexReg, transformation[0].Base(), 2 ); 
}

void CBaseVSShader::SetVertexShaderTextureScaledTransform( int vertexReg, int transformVar, int scaleVar )
{
	Vector4D transformation[2];
	IMaterialVar* pTransformationVar = s_ppParams[transformVar];
	if (pTransformationVar && (pTransformationVar->GetType() == MATERIAL_VAR_TYPE_MATRIX))
	{
		const VMatrix &mat = pTransformationVar->GetMatrixValue();
		transformation[0].Init( mat[0][0], mat[0][1], mat[0][2], mat[0][3] );
		transformation[1].Init( mat[1][0], mat[1][1], mat[1][2], mat[1][3] );
	}
	else
	{
		transformation[0].Init( 1.0f, 0.0f, 0.0f, 0.0f );
		transformation[1].Init( 0.0f, 1.0f, 0.0f, 0.0f );
	}

	Vector2D scale( 1, 1 );
	IMaterialVar* pScaleVar = s_ppParams[scaleVar];
	if (pScaleVar)
	{
		if (pScaleVar->GetType() == MATERIAL_VAR_TYPE_VECTOR)
			pScaleVar->GetVecValue( scale.Base(), 2 );
		else if (pScaleVar->IsDefined())
			scale[0] = scale[1] = pScaleVar->GetFloatValue();
	}

	// Apply the scaling
	transformation[0][0] *= scale[0];
	transformation[0][1] *= scale[1];
	transformation[1][0] *= scale[0];
	transformation[1][1] *= scale[1];
	transformation[0][3] *= scale[0];
	transformation[1][3] *= scale[1];
	s_pShaderAPI->SetVertexShaderConstant( vertexReg, transformation[0].Base(), 2 ); 
}


//-----------------------------------------------------------------------------
// Sets pixel shader texture transforms
//-----------------------------------------------------------------------------
void CBaseVSShader::SetPixelShaderTextureTranslation( int pixelReg, int translationVar )
{
	float offset[2] = {0, 0};

	IMaterialVar* pTranslationVar = s_ppParams[translationVar];
	if (pTranslationVar)
	{
		if (pTranslationVar->GetType() == MATERIAL_VAR_TYPE_VECTOR)
			pTranslationVar->GetVecValue( offset, 2 );
		else
			offset[0] = offset[1] = pTranslationVar->GetFloatValue();
	}

	Vector4D translation[2];
	translation[0].Init( 1.0f, 0.0f, 0.0f, offset[0] );
	translation[1].Init( 0.0f, 1.0f, 0.0f, offset[1] );
	s_pShaderAPI->SetPixelShaderConstant( pixelReg, translation[0].Base(), 2 ); 
}

void CBaseVSShader::SetPixelShaderTextureScale( int pixelReg, int scaleVar )
{
	float scale[2] = {1, 1};

	IMaterialVar* pScaleVar = s_ppParams[scaleVar];
	if (pScaleVar)
	{
		if (pScaleVar->GetType() == MATERIAL_VAR_TYPE_VECTOR)
			pScaleVar->GetVecValue( scale, 2 );
		else if (pScaleVar->IsDefined())
			scale[0] = scale[1] = pScaleVar->GetFloatValue();
	}

	Vector4D scaleMatrix[2];
	scaleMatrix[0].Init( scale[0], 0.0f, 0.0f, 0.0f );
	scaleMatrix[1].Init( 0.0f, scale[1], 0.0f, 0.0f );
	s_pShaderAPI->SetPixelShaderConstant( pixelReg, scaleMatrix[0].Base(), 2 ); 
}

void CBaseVSShader::SetPixelShaderTextureTransform( int pixelReg, int transformVar )
{
	Vector4D transformation[2];
	IMaterialVar* pTransformationVar = s_ppParams[transformVar];
	if (pTransformationVar && (pTransformationVar->GetType() == MATERIAL_VAR_TYPE_MATRIX))
	{
		const VMatrix &mat = pTransformationVar->GetMatrixValue();
		transformation[0].Init( mat[0][0], mat[0][1], mat[0][2], mat[0][3] );
		transformation[1].Init( mat[1][0], mat[1][1], mat[1][2], mat[1][3] );
	}
	else
	{
		transformation[0].Init( 1.0f, 0.0f, 0.0f, 0.0f );
		transformation[1].Init( 0.0f, 1.0f, 0.0f, 0.0f );
	}
	s_pShaderAPI->SetPixelShaderConstant( pixelReg, transformation[0].Base(), 2 ); 
}

void CBaseVSShader::SetPixelShaderTextureScaledTransform( int pixelReg, int transformVar, int scaleVar )
{
	Vector4D transformation[2];
	IMaterialVar* pTransformationVar = s_ppParams[transformVar];
	if (pTransformationVar && (pTransformationVar->GetType() == MATERIAL_VAR_TYPE_MATRIX))
	{
		const VMatrix &mat = pTransformationVar->GetMatrixValue();
		transformation[0].Init( mat[0][0], mat[0][1], mat[0][2], mat[0][3] );
		transformation[1].Init( mat[1][0], mat[1][1], mat[1][2], mat[1][3] );
	}
	else
	{
		transformation[0].Init( 1.0f, 0.0f, 0.0f, 0.0f );
		transformation[1].Init( 0.0f, 1.0f, 0.0f, 0.0f );
	}

	Vector2D scale( 1, 1 );
	IMaterialVar* pScaleVar = s_ppParams[scaleVar];
	if (pScaleVar)
	{
		if (pScaleVar->GetType() == MATERIAL_VAR_TYPE_VECTOR)
			pScaleVar->GetVecValue( scale.Base(), 2 );
		else if (pScaleVar->IsDefined())
			scale[0] = scale[1] = pScaleVar->GetFloatValue();
	}

	// Apply the scaling
	transformation[0][0] *= scale[0];
	transformation[0][1] *= scale[1];
	transformation[1][0] *= scale[0];
	transformation[1][1] *= scale[1];
	transformation[0][3] *= scale[0];
	transformation[1][3] *= scale[1];
	s_pShaderAPI->SetPixelShaderConstant( pixelReg, transformation[0].Base(), 2 ); 
}


//-----------------------------------------------------------------------------
// Moves a matrix into vertex shader constants 
//-----------------------------------------------------------------------------
void CBaseVSShader::SetVertexShaderMatrix3x4( int vertexReg, int matrixVar )
{
	IMaterialVar* pTranslationVar = s_ppParams[matrixVar];
	if (pTranslationVar)
	{
		s_pShaderAPI->SetVertexShaderConstant( vertexReg, &pTranslationVar->GetMatrixValue( )[0][0], 3 ); 
	}
	else
	{
		VMatrix matrix;
		MatrixSetIdentity( matrix );
		s_pShaderAPI->SetVertexShaderConstant( vertexReg, &matrix[0][0], 3 ); 
	}
}

void CBaseVSShader::SetVertexShaderMatrix4x4( int vertexReg, int matrixVar )
{
	IMaterialVar* pTranslationVar = s_ppParams[matrixVar];
	if (pTranslationVar)
	{
		s_pShaderAPI->SetVertexShaderConstant( vertexReg, &pTranslationVar->GetMatrixValue( )[0][0], 4 ); 
	}
	else
	{
		VMatrix matrix;
		MatrixSetIdentity( matrix );
		s_pShaderAPI->SetVertexShaderConstant( vertexReg, &matrix[0][0], 4 ); 
	}
}


//-----------------------------------------------------------------------------
// Loads the view matrix into pixel shader constants
//-----------------------------------------------------------------------------
void CBaseVSShader::LoadViewMatrixIntoVertexShaderConstant( int vertexReg )
{
	VMatrix mat, transpose;
	s_pShaderAPI->GetMatrix( MATERIAL_VIEW, mat.m[0] );

	MatrixTranspose( mat, transpose );
	s_pShaderAPI->SetVertexShaderConstant( vertexReg, transpose.m[0], 3 );
}


//-----------------------------------------------------------------------------
// Loads bump lightmap coordinates into the pixel shader
//-----------------------------------------------------------------------------
void CBaseVSShader::LoadBumpLightmapCoordinateAxes_PixelShader( int pixelReg )
{
	Vector4D basis[3];
	for (int i = 0; i < 3; ++i)
	{
		memcpy( &basis[i], &g_localBumpBasis[i], 3 * sizeof(float) );
		basis[i][3] = 0.0f;
	}
	s_pShaderAPI->SetPixelShaderConstant( pixelReg, (float*)basis, 3 );
}


//-----------------------------------------------------------------------------
// Loads bump lightmap coordinates into the pixel shader
//-----------------------------------------------------------------------------
void CBaseVSShader::LoadBumpLightmapCoordinateAxes_VertexShader( int vertexReg )
{
	Vector4D basis[3];

	// transpose
	int i;
	for (i = 0; i < 3; ++i)
	{
		basis[i][0] = g_localBumpBasis[0][i];
		basis[i][1] = g_localBumpBasis[1][i];
		basis[i][2] = g_localBumpBasis[2][i];
		basis[i][3] = 0.0f;
	}
	s_pShaderAPI->SetVertexShaderConstant( vertexReg, (float*)basis, 3 );
	for (i = 0; i < 3; ++i)
	{
		memcpy( &basis[i], &g_localBumpBasis[i], 3 * sizeof(float) );
		basis[i][3] = 0.0f;
	}
	s_pShaderAPI->SetVertexShaderConstant( vertexReg + 3, (float*)basis, 3 );
}


//-----------------------------------------------------------------------------
// Helper methods for pixel shader overbrighting
//-----------------------------------------------------------------------------
void CBaseVSShader::EnablePixelShaderOverbright( int reg, bool bEnable, bool bDivideByTwo )
{
	// can't have other overbright values with pixel shaders as it stands.
	float v[4];
	if( bEnable )
	{
		v[0] = v[1] = v[2] = v[3] = bDivideByTwo ? OVERBRIGHT / 2.0f : OVERBRIGHT;
	}
	else
	{
		v[0] = v[1] = v[2] = v[3] = bDivideByTwo ? 1.0f / 2.0f : 1.0f;
	}
	s_pShaderAPI->SetPixelShaderConstant( reg, v, 1 );
}


//-----------------------------------------------------------------------------
// Helper for dealing with modulation
//-----------------------------------------------------------------------------
void CBaseVSShader::SetModulationVertexShaderDynamicState()
{
 	float color[4] = { 1.0, 1.0, 1.0, 1.0 };
	ComputeModulationColor( color );
	s_pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_MODULATION_COLOR, color );
}

void CBaseVSShader::SetModulationPixelShaderDynamicState( int modulationVar )
{
	float color[4] = { 1.0, 1.0, 1.0, 1.0 };
	ComputeModulationColor( color );
	s_pShaderAPI->SetPixelShaderConstant( modulationVar, color );
}

void CBaseVSShader::SetModulationPixelShaderDynamicState_LinearColorSpace( int modulationVar )
{
	float color[4] = { 1.0, 1.0, 1.0, 1.0 };
	ComputeModulationColor( color );
	color[0] = GammaToLinear( color[0] );
	color[1] = GammaToLinear( color[1] );
	color[2] = GammaToLinear( color[2] );
	s_pShaderAPI->SetPixelShaderConstant( modulationVar, color );
}

void CBaseVSShader::SetModulationPixelShaderDynamicState_LinearColorSpace_LinearScale( int modulationVar, float flScale )
{
	float color[4] = { 1.0, 1.0, 1.0, 1.0 };
	ComputeModulationColor( color );
	color[0] = GammaToLinear( color[0] ) * flScale;
	color[1] = GammaToLinear( color[1] ) * flScale;
	color[2] = GammaToLinear( color[2] ) * flScale;
	s_pShaderAPI->SetPixelShaderConstant( modulationVar, color );
}


//-----------------------------------------------------------------------------
// Converts a color + alpha into a vector4
//-----------------------------------------------------------------------------
void CBaseVSShader::ColorVarsToVector( int colorVar, int alphaVar, Vector4D &color )
{
	color.Init( 1.0, 1.0, 1.0, 1.0 ); 
	if ( colorVar != -1 )
	{
		IMaterialVar* pColorVar = s_ppParams[colorVar];
		if ( pColorVar->GetType() == MATERIAL_VAR_TYPE_VECTOR )
		{
			pColorVar->GetVecValue( color.Base(), 3 );
		}
		else
		{
			color[0] = color[1] = color[2] = pColorVar->GetFloatValue();
		}
	}
	if ( alphaVar != -1 )
	{
		float flAlpha = s_ppParams[alphaVar]->GetFloatValue();
		color[3] = clamp( flAlpha, 0.0f, 1.0f );
	}
}


//-----------------------------------------------------------------------------
// Sets a color + alpha into shader constants
//-----------------------------------------------------------------------------
void CBaseVSShader::SetColorVertexShaderConstant( int nVertexReg, int colorVar, int alphaVar )
{
	Vector4D color;
	ColorVarsToVector( colorVar, alphaVar, color );
	s_pShaderAPI->SetVertexShaderConstant( nVertexReg, color.Base() );
}

void CBaseVSShader::SetColorPixelShaderConstant( int nPixelReg, int colorVar, int alphaVar )
{
	Vector4D color;
	ColorVarsToVector( colorVar, alphaVar, color );
	s_pShaderAPI->SetPixelShaderConstant( nPixelReg, color.Base() );
}



#ifdef _DEBUG
ConVar mat_envmaptintoverride( "mat_envmaptintoverride", "-1" );
ConVar mat_envmaptintscale( "mat_envmaptintscale", "-1" );
#endif

//-----------------------------------------------------------------------------
// Helpers for dealing with envmap tint
//-----------------------------------------------------------------------------
// set alphaVar to -1 to ignore it.
void CBaseVSShader::SetEnvMapTintPixelShaderDynamicState( int pixelReg, int tintVar, int alphaVar, bool bConvertFromGammaToLinear )
{
	float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	if( g_pConfig->bShowSpecular )
	{
		IMaterialVar* pAlphaVar = NULL;
		if( alphaVar >= 0 )
		{
			pAlphaVar = s_ppParams[alphaVar];
		}
		if( pAlphaVar )
		{
			color[3] = pAlphaVar->GetFloatValue();
		}

		IMaterialVar* pTintVar = s_ppParams[tintVar];
#ifdef _DEBUG
		pTintVar->GetVecValue( color, 3 );

		float envmapTintOverride = mat_envmaptintoverride.GetFloat();
		float envmapTintScaleOverride = mat_envmaptintscale.GetFloat();

		if( envmapTintOverride != -1.0f )
		{
			color[0] = color[1] = color[2] = envmapTintOverride;
		}
		if( envmapTintScaleOverride != -1.0f )
		{
			color[0] *= envmapTintScaleOverride;
			color[1] *= envmapTintScaleOverride;
			color[2] *= envmapTintScaleOverride;
		}

		if( bConvertFromGammaToLinear )
		{
			color[0] = GammaToLinear( color[0] );
			color[1] = GammaToLinear( color[1] );
			color[2] = GammaToLinear( color[2] );
		}
#else
		if( bConvertFromGammaToLinear )
		{
			pTintVar->GetLinearVecValue( color, 3 );
		}
		else
		{
			pTintVar->GetVecValue( color, 3 );
		}
#endif
	}
	else
	{
		color[0] = color[1] = color[2] = color[3] = 0.0f;
	}
	s_pShaderAPI->SetPixelShaderConstant( pixelReg, color, 1 );
}

void CBaseVSShader::SetAmbientCubeDynamicStateVertexShader( )
{
	s_pShaderAPI->SetVertexShaderStateAmbientLightCube();
}

float CBaseVSShader::GetAmbientLightCubeLuminance( )
{
	return s_pShaderAPI->GetAmbientLightCubeLuminance();
}

const char *CBaseVSShader::UnlitGeneric_ComputePixelShaderName( bool bMask,
														  bool bEnvmap,
														  bool bBaseTexture,
														  bool bBaseAlphaEnvmapMask,
														  bool bDetail )
{
	static char const* s_pPixelShaders[] = 
	{
		"SDK_UnlitGeneric_NoTexture",
		"SDK_UnlitGeneric",
		"SDK_UnlitGeneric_EnvMapNoTexture",
		"SDK_UnlitGeneric_EnvMap",
		"SDK_UnlitGeneric_NoTexture",
		"SDK_UnlitGeneric",
		"SDK_UnlitGeneric_EnvMapMaskNoTexture",
		"SDK_UnlitGeneric_EnvMapMask",

		// Detail texture
		// The other commented-out versions are used if we want to
		// apply the detail *after* the environment map is added
		"SDK_UnlitGeneric_DetailNoTexture",
		"SDK_UnlitGeneric_Detail",
		"SDK_UnlitGeneric_EnvMapNoTexture", //"UnlitGeneric_DetailEnvMapNoTexture",
		"SDK_UnlitGeneric_DetailEnvMap",
		"SDK_UnlitGeneric_DetailNoTexture",
		"SDK_UnlitGeneric_Detail",
		"SDK_UnlitGeneric_EnvMapMaskNoTexture", //"UnlitGeneric_DetailEnvMapMaskNoTexture",
		"SDK_UnlitGeneric_DetailEnvMapMask",
	};

	if (!bMask && bEnvmap && bBaseTexture && bBaseAlphaEnvmapMask)
	{
		if (!bDetail)
			return "SDK_UnlitGeneric_BaseAlphaMaskedEnvMap";
		else
			return "SDK_UnlitGeneric_DetailBaseAlphaMaskedEnvMap";
	}
	else
	{
		int pshIndex = 0;
		if (bBaseTexture)
			pshIndex |= 0x1;
		if (bEnvmap)
			pshIndex |= 0x2;
		if (bMask)
			pshIndex |= 0x4;
		if (bDetail)
			pshIndex |= 0x8;
		return s_pPixelShaders[pshIndex];
	}
}

//-----------------------------------------------------------------------------
// Vertex shader unlit generic pass
//-----------------------------------------------------------------------------
void CBaseVSShader::VertexShaderUnlitGenericPass( bool doSkin, int baseTextureVar, int frameVar, 
	int baseTextureTransformVar, int detailVar, int detailTransform, bool bDetailTransformIsScale,
	int envmapVar, int envMapFrameVar, int envmapMaskVar, int envmapMaskFrameVar, 
	int envmapMaskScaleVar, int envmapTintVar, int alphaTestReferenceVar )
{
	IMaterialVar** params = s_ppParams;

	bool bBaseAlphaEnvmapMask = IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK);
	bool bEnvmap = (envmapVar >= 0) && params[envmapVar]->IsTexture();
	bool bMask = false;
	if (bEnvmap && (envmapMaskVar >= 0))
	{
		bMask = params[envmapMaskVar]->IsTexture();
	}
	bool bDetail = (detailVar >= 0) && params[detailVar]->IsTexture(); 
	bool bBaseTexture = (baseTextureVar >= 0) && params[baseTextureVar]->IsTexture();
	bool bVertexColor = IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR);
	bool bEnvmapCameraSpace = IS_FLAG_SET(MATERIAL_VAR_ENVMAPCAMERASPACE);
	bool bEnvmapSphere = IS_FLAG_SET(MATERIAL_VAR_ENVMAPSPHERE);

	if (IsSnapshotting())
	{
		// Alpha test
		s_pShaderShadow->EnableAlphaTest( IS_FLAG_SET(MATERIAL_VAR_ALPHATEST) );

		if( alphaTestReferenceVar != -1 && params[alphaTestReferenceVar]->GetFloatValue() > 0.0f )
		{
			s_pShaderShadow->AlphaFunc( SHADER_ALPHAFUNC_GEQUAL, params[alphaTestReferenceVar]->GetFloatValue() );
		}

		// Base texture on stage 0
		if (bBaseTexture)
		{
			s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
		}

		if (bDetail)
		{
			s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE3, true );
		}

		if (bEnvmap)
		{
			// envmap on stage 1
			s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );

			// envmapmask on stage 2
			if (bMask || bBaseAlphaEnvmapMask )
				s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE2, true );
		}

		if (bBaseTexture)
			SetDefaultBlendingShadowState( baseTextureVar, true );
		else if (bMask)
			SetDefaultBlendingShadowState( envmapMaskVar, false );
		else
			SetDefaultBlendingShadowState();

		int fmt = VERTEX_POSITION;
		if( bEnvmap )
			fmt |= VERTEX_NORMAL;
		if (doSkin)
			fmt |= VERTEX_BONE_INDEX;
		if (bVertexColor)
			fmt |= VERTEX_COLOR;

		s_pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, doSkin ? 3 : 0, 0 );
		const char *pshName = UnlitGeneric_ComputePixelShaderName(
			bMask,
			bEnvmap,
			bBaseTexture,
			bBaseAlphaEnvmapMask,
			bDetail );
		s_pShaderShadow->SetPixelShader( pshName );

		// Compute the vertex shader index.
		sdk_unlitgeneric_vs11_Static_Index vshIndex;
		vshIndex.SetDETAIL( bDetail );
		vshIndex.SetENVMAP( bEnvmap );
		vshIndex.SetENVMAPCAMERASPACE( bEnvmap && bEnvmapCameraSpace );
		vshIndex.SetENVMAPSPHERE( bEnvmap && bEnvmapSphere );
		vshIndex.SetVERTEXCOLOR( bVertexColor );
		s_pShaderShadow->SetVertexShader( "SDK_unlitgeneric_vs11", vshIndex.GetIndex() );

		DefaultFog();
	}
	else
	{
		if (bBaseTexture)
		{
			BindTexture( SHADER_TEXTURE_STAGE0, baseTextureVar, frameVar );
			SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, baseTextureTransformVar );
		}

		if (bDetail)
		{
			BindTexture( SHADER_TEXTURE_STAGE3, detailVar, frameVar );

			if (bDetailTransformIsScale)
			{
				SetVertexShaderTextureScaledTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_4, baseTextureTransformVar, detailTransform );
			}
			else
			{
				SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_4, detailTransform );
			}
		}

		if (bEnvmap)
		{
			BindTexture( SHADER_TEXTURE_STAGE1, envmapVar, envMapFrameVar );

			if (bMask || bBaseAlphaEnvmapMask)
			{
				if (bMask)
					BindTexture( SHADER_TEXTURE_STAGE2, envmapMaskVar, envmapMaskFrameVar );
				else
					BindTexture( SHADER_TEXTURE_STAGE2, baseTextureVar, frameVar );

				SetVertexShaderTextureScaledTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_2, baseTextureTransformVar, envmapMaskScaleVar );
			}

			SetEnvMapTintPixelShaderDynamicState( 2, envmapTintVar, -1 );

			if (bEnvmapSphere || IS_FLAG_SET(MATERIAL_VAR_ENVMAPCAMERASPACE))
			{
				LoadViewMatrixIntoVertexShaderConstant( VERTEX_SHADER_VIEWMODEL );
			}
		}

		SetModulationVertexShaderDynamicState();

		// Compute the vertex shader index.
		sdk_unlitgeneric_vs11_Dynamic_Index vshIndex;
		vshIndex.SetDOWATERFOG( s_pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
#if !defined( _XBOX )
		vshIndex.SetNUM_BONES( s_pShaderAPI->GetCurrentNumBones() );
#else
		vshIndex.SetSKINNING( s_pShaderAPI->GetCurrentNumBones() > 0 );
#endif
		s_pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
	}

	Draw();
}


void CBaseVSShader::DrawWorldBaseTexture( int baseTextureVar, int baseTextureTransformVar,
									int frameVar, int colorVar, int alphaVar )
{
	if( IsSnapshotting() )
	{
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
		s_pShaderShadow->VertexShaderVertexFormat( 
			VERTEX_POSITION, 1, 0, 0, 0 );
		s_pShaderShadow->SetPixelShader( "SDK_LightmappedGeneric_BaseTexture" );
		SetNormalBlendingShadowState();
		sdk_lightmappedgeneric_basetexture_Static_Index vshIndex;
		s_pShaderShadow->SetVertexShader( "SDK_LightmappedGeneric_BaseTexture", vshIndex.GetIndex() );

		FogToOOOverbright();
	}
	else
	{
		BindTexture( SHADER_TEXTURE_STAGE0, baseTextureVar, frameVar );
		SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, baseTextureTransformVar );
		SetColorPixelShaderConstant( 0, colorVar, alphaVar );
		sdk_lightmappedgeneric_basetexture_Dynamic_Index vshIndex;
		vshIndex.SetDOWATERFOG( s_pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
		s_pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
	}
	Draw();
}

void CBaseVSShader::DrawWorldBumpedDiffuseLighting( int bumpmapVar, int bumpFrameVar,
											  int bumpTransformVar, bool bMultiply )
{
	if( IsSnapshotting() )
	{
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE2, true );
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE3, true );
		if( bMultiply )
		{
			s_pShaderShadow->EnableBlending( true );
			SingleTextureLightmapBlendMode();
		}
		s_pShaderShadow->VertexShaderVertexFormat( 
			VERTEX_POSITION, 3, 0, 0, 0 );

		sdk_lightmappedgeneric_bumpmappedlightmap_Static_Index vshIndex;
		s_pShaderShadow->SetVertexShader( "SDK_LightmappedGeneric_BumpmappedLightmap", vshIndex.GetIndex() );

		s_pShaderShadow->SetPixelShader( "SDK_LightmappedGeneric_BumpmappedLightmap" );
		FogToFogColor();
	}
	else
	{
		if( !g_pConfig->m_bFastNoBump )
		{
			BindTexture( SHADER_TEXTURE_STAGE0, bumpmapVar, bumpFrameVar );
		}
		else
		{
			s_pShaderAPI->BindFlatNormalMap( SHADER_TEXTURE_STAGE0 );
		}
		LoadBumpLightmapCoordinateAxes_PixelShader( 0 );
		s_pShaderAPI->BindBumpLightmap( SHADER_TEXTURE_STAGE1 );
		SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, bumpTransformVar );
		SetModulationPixelShaderDynamicState( 3 );

		sdk_lightmappedgeneric_bumpmappedlightmap_Dynamic_Index vshIndex;
		vshIndex.SetDOWATERFOG( s_pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
		s_pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
	}
	Draw();
}

#ifndef _XBOX
void CBaseVSShader::DrawWorldBumpedDiffuseLighting_Base_ps14( int bumpmapVar, int bumpFrameVar,
											  int bumpTransformVar, 
											  int baseTextureVar, int baseTextureTransformVar, int frameVar )
{
	if( IsSnapshotting() )
	{
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE2, true );
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE3, true );
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE4, true );
		s_pShaderShadow->VertexShaderVertexFormat( 
			VERTEX_POSITION, 3, 0, 0, 0 );

		sdk_lightmappedgeneric_bumpmappedlightmap_base_ps14_Static_Index vshIndex;
		s_pShaderShadow->SetVertexShader( "SDK_LightmappedGeneric_BumpmappedLightmap_Base_ps14", vshIndex.GetIndex() );

		s_pShaderShadow->SetPixelShader( "SDK_LightmappedGeneric_BumpmappedLightmap_Base_ps14" );
		FogToFogColor();
	}
	else
	{
		if( !g_pConfig->m_bFastNoBump )
		{
			BindTexture( SHADER_TEXTURE_STAGE0, bumpmapVar, bumpFrameVar );
		}
		else
		{
			s_pShaderAPI->BindFlatNormalMap( SHADER_TEXTURE_STAGE0 );
		}
		LoadBumpLightmapCoordinateAxes_PixelShader( 0 );
		s_pShaderAPI->BindBumpLightmap( SHADER_TEXTURE_STAGE1 );
		BindTexture( SHADER_TEXTURE_STAGE4, baseTextureVar, frameVar );
		SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, bumpTransformVar );
		SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_2, baseTextureTransformVar );
		SetModulationPixelShaderDynamicState( 3 );

		sdk_lightmappedgeneric_bumpmappedlightmap_base_ps14_Dynamic_Index vshIndex;
		vshIndex.SetDOWATERFOG( s_pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
		s_pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
	}
	Draw();
}
#endif

#ifndef _XBOX
void CBaseVSShader::DrawWorldBumpedDiffuseLighting_Blend_ps14( int bumpmapVar, int bumpFrameVar,
											int bumpTransformVar,
											int baseTextureVar, int baseTextureTransformVar, 
											int baseTextureFrameVar,
											int baseTexture2Var, int baseTextureTransform2Var, 
											int baseTextureFrame2Var)
{
	if( IsSnapshotting() )
	{
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE2, true );
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE3, true );
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE4, true );
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE5, true );
		s_pShaderShadow->VertexShaderVertexFormat( 
			VERTEX_POSITION, 3, 0, 0, 0 );

		sdk_lightmappedgeneric_bumpmappedlightmap_blend_ps14_Static_Index vshIndex;
		s_pShaderShadow->SetVertexShader( "SDK_LightmappedGeneric_BumpmappedLightmap_Blend_ps14", vshIndex.GetIndex() );

		s_pShaderShadow->SetPixelShader( "SDK_LightmappedGeneric_BumpmappedLightmap_Blend_ps14" );
		FogToFogColor();
	}
	else
	{
		if( !g_pConfig->m_bFastNoBump )
		{
			BindTexture( SHADER_TEXTURE_STAGE0, bumpmapVar, bumpFrameVar );
		}
		else
		{
			s_pShaderAPI->BindFlatNormalMap( SHADER_TEXTURE_STAGE0 );
		}
		LoadBumpLightmapCoordinateAxes_PixelShader( 0 );
		s_pShaderAPI->BindBumpLightmap( SHADER_TEXTURE_STAGE1 );
		BindTexture( SHADER_TEXTURE_STAGE4, baseTextureVar, baseTextureFrameVar );
		BindTexture( SHADER_TEXTURE_STAGE5, baseTexture2Var, baseTextureFrame2Var );
		SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, bumpTransformVar );
		SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_2, baseTextureTransformVar );
		SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_4, baseTextureTransform2Var );
		SetModulationPixelShaderDynamicState( 3 );

		sdk_lightmappedgeneric_bumpmappedlightmap_blend_ps14_Dynamic_Index vshIndex;
		vshIndex.SetDOWATERFOG( s_pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
		s_pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
	}
	Draw();
}
#endif

//#define USE_DEST_ALPHA
#define USE_NORMALMAP_ALPHA

void CBaseVSShader::DrawWorldBumpedSpecularLighting( int bumpmapVar, int envmapVar,
											   int bumpFrameVar, int envmapFrameVar,
											   int envmapTintVar, int alphaVar,
											   int envmapContrastVar, int envmapSaturationVar,
											   int bumpTransformVar, int fresnelReflectionVar,
											   bool bBlend, bool bNoWriteZ )
{
	// + BUMPED CUBEMAP
	if( IsSnapshotting() )
	{
		SetInitialShadowState( );
		if ( bNoWriteZ )
		{
			s_pShaderShadow->EnableDepthWrites( false );
		}
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE3, true );
		if( g_pHardwareConfig->SupportsPixelShaders_1_4() )
		{
			s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE4, true );
		}
		if( bBlend )
		{
			s_pShaderShadow->EnableBlending( true );
			s_pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE );
		}
		// FIXME: Remove the normal (needed for tangent space gen)
		s_pShaderShadow->VertexShaderVertexFormat( 
			VERTEX_POSITION | VERTEX_NORMAL | VERTEX_TANGENT_S |
			VERTEX_TANGENT_T, 1, 0, 0, 0 );

		IMaterialVar** params = s_ppParams;
		bool bHasNormalMapAlphaEnvMapMask = IS_FLAG_SET( MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK );
#ifndef _XBOX
		if( g_pHardwareConfig->SupportsPixelShaders_1_4() )
		{
			sdk_lightmappedgeneric_bumpmappedenvmap_ps14_Static_Index vshIndex;
			s_pShaderShadow->SetVertexShader( "SDK_LightmappedGeneric_BumpmappedEnvmap_ps14", vshIndex.GetIndex() );

			int nPshIndex = bHasNormalMapAlphaEnvMapMask ? 1 : 0;
			s_pShaderShadow->SetPixelShader( "SDK_LightmappedGeneric_BumpmappedEnvmap_ps14", nPshIndex );
		}
		else
#endif
		{
			sdk_lightmappedgeneric_bumpmappedenvmap_Static_Index vshIndex;
			s_pShaderShadow->SetVertexShader( "SDK_LightmappedGeneric_BumpmappedEnvmap", vshIndex.GetIndex() );

			int nPshIndex = bHasNormalMapAlphaEnvMapMask ? 1 : 0;
			s_pShaderShadow->SetPixelShader( "SDK_LightmappedGeneric_BumpmappedEnvmap", nPshIndex );
		}
		FogToBlack();
	}
	else
	{
		IMaterialVar** params = s_ppParams;
		s_pShaderAPI->SetDefaultState();
		BindTexture( SHADER_TEXTURE_STAGE0, bumpmapVar, bumpFrameVar );
		BindTexture( SHADER_TEXTURE_STAGE3, envmapVar, envmapFrameVar );
#ifndef _XBOX
		if( g_pHardwareConfig->SupportsPixelShaders_1_4() )
		{
			s_pShaderAPI->BindNormalizationCubeMap( SHADER_TEXTURE_STAGE4 );

			sdk_lightmappedgeneric_bumpmappedenvmap_ps14_Dynamic_Index vshIndex;
			vshIndex.SetDOWATERFOG( s_pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			s_pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
		}
		else
#endif
		{
			sdk_lightmappedgeneric_bumpmappedenvmap_Dynamic_Index vshIndex;
			vshIndex.SetDOWATERFOG( s_pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			s_pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
		}
				
		SetEnvMapTintPixelShaderDynamicState( 0, envmapTintVar, alphaVar );
		// GR - fudge consts a bit to fix const/lerp issues
		SetPixelShaderConstantFudge( 1, envmapContrastVar );
		SetPixelShaderConstantFudge( 2, envmapSaturationVar );
		float greyWeights[4] = { 0.299f, 0.587f, 0.114f, 0.0f };
		s_pShaderAPI->SetPixelShaderConstant( 3, greyWeights );

		// [ 0, 0 ,0, R(0) ]
		float fresnel[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		fresnel[3] = params[fresnelReflectionVar]->GetFloatValue();
		s_pShaderAPI->SetPixelShaderConstant( 4, fresnel );
		// [ 0, 0 ,0, 1-R(0) ]
		fresnel[3] = 1.0f - fresnel[3];
		s_pShaderAPI->SetPixelShaderConstant( 6, fresnel );

		float one[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		s_pShaderAPI->SetPixelShaderConstant( 5, one );
		SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, bumpTransformVar );
	}
	Draw();
}

void CBaseVSShader::DrawModelBumpedSpecularLighting( int bumpMapVar, int bumpMapFrameVar,
											   int envMapVar, int envMapVarFrame,
											   int envMapTintVar, int alphaVar,
											   int envMapContrastVar, int envMapSaturationVar,
											   int bumpTransformVar,
											   bool bBlendSpecular, bool bNoWriteZ )
{
	IMaterialVar** params = s_ppParams;
	
	if( IsSnapshotting() )
	{
		SetInitialShadowState( );
		if ( bNoWriteZ )
		{
			s_pShaderShadow->EnableDepthWrites( false );
		}
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE3, true );
		if( g_pHardwareConfig->SupportsPixelShaders_1_4() )
		{
			s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE4, true );
		}
		s_pShaderShadow->EnableAlphaTest( false );
		if( bBlendSpecular )
		{
			s_pShaderShadow->EnableBlending( true );
			SetAdditiveBlendingShadowState( -1, false );
		}
		else
		{
			s_pShaderShadow->EnableBlending( false );
			SetNormalBlendingShadowState( -1, false );
		}
		
		int numLights = 0;
		
		s_pShaderShadow->VertexShaderVertexFormat( 
			VERTEX_POSITION | VERTEX_NORMAL | VERTEX_BONE_INDEX, 
			1, 0, numLights, 4 /* userDataSize */ );

		bool bHasNormalMapAlphaEnvMapMask = IS_FLAG_SET( MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK );
#ifndef _XBOX
		if( g_pHardwareConfig->SupportsPixelShaders_1_4() )
		{
			sdk_vertexlitgeneric_envmappedbumpmap_nolighting_ps14_Static_Index vshIndex;
			s_pShaderShadow->SetVertexShader( "SDK_VertexLitGeneric_EnvmappedBumpmap_NoLighting_ps14", vshIndex.GetIndex() );
			if( bHasNormalMapAlphaEnvMapMask )
			{
				s_pShaderShadow->SetPixelShader( "SDK_VertexLitGeneric_EnvmappedBumpmapV2_MultByAlpha_ps14" );
			}
			else
			{
				s_pShaderShadow->SetPixelShader( "SDK_VertexLitGeneric_EnvmappedBumpmapV2_ps14" );
			}
		}
		else
#endif
		{
			sdk_vertexlitgeneric_envmappedbumpmap_nolighting_Static_Index vshIndex;
			s_pShaderShadow->SetVertexShader( "SDK_VertexLitGeneric_EnvmappedBumpmap_NoLighting", vshIndex.GetIndex() );
			// This version does not multiply by lighting.
			// NOTE: We don't support multiplying by lighting for bumped specular stuff.
			if( bHasNormalMapAlphaEnvMapMask )
			{
				s_pShaderShadow->SetPixelShader( "SDK_VertexLitGeneric_EnvmappedBumpmapV2_MultByAlpha" );
			}
			else
			{
				s_pShaderShadow->SetPixelShader( "SDK_VertexLitGeneric_EnvmappedBumpmapV2" );
			}
		}
		FogToBlack();
	}
	else
	{
		s_pShaderAPI->SetDefaultState();
		BindTexture( SHADER_TEXTURE_STAGE0, bumpMapVar, bumpMapFrameVar );
		BindTexture( SHADER_TEXTURE_STAGE3, envMapVar, envMapVarFrame );
		if( g_pHardwareConfig->SupportsPixelShaders_1_4() )
		{
			s_pShaderAPI->BindNormalizationCubeMap( SHADER_TEXTURE_STAGE4 );
		}
				
		if( bBlendSpecular )
		{
			SetEnvMapTintPixelShaderDynamicState( 0, envMapTintVar, -1 );
		}
		else
		{
			SetEnvMapTintPixelShaderDynamicState( 0, envMapTintVar, alphaVar );
		}
		// GR - fudge consts a bit to fix const/lerp issues
		SetPixelShaderConstantFudge( 1, envMapContrastVar );
		SetPixelShaderConstantFudge( 2, envMapSaturationVar );
		float greyWeights[4] = { 0.299f, 0.587f, 0.114f, 0.0f };
		s_pShaderAPI->SetPixelShaderConstant( 3, greyWeights );
		
		// handle scrolling of bump texture
		SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_4, bumpTransformVar );

#ifndef _XBOX
		if( g_pHardwareConfig->SupportsPixelShaders_1_4() )
		{
			sdk_vertexlitgeneric_envmappedbumpmap_nolighting_ps14_Dynamic_Index vshIndex;
			vshIndex.SetDOWATERFOG( s_pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			vshIndex.SetNUM_BONES( s_pShaderAPI->GetCurrentNumBones() );
			s_pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
		}
		else
#endif
		{
			sdk_vertexlitgeneric_envmappedbumpmap_nolighting_Dynamic_Index vshIndex;
			vshIndex.SetDOWATERFOG( s_pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
#if !defined( _XBOX )
			vshIndex.SetNUM_BONES( s_pShaderAPI->GetCurrentNumBones() );
#else
			vshIndex.SetSKINNING( s_pShaderAPI->GetCurrentNumBones() > 0 );
#endif
			s_pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
		}
	}
	Draw();
}

void CBaseVSShader::DrawBaseTextureBlend( int baseTextureVar, int baseTextureTransformVar, 
									 int baseTextureFrameVar,
									 int baseTexture2Var, int baseTextureTransform2Var, 
									 int baseTextureFrame2Var, int colorVar, int alphaVar )
{
	if( IsSnapshotting() )
	{
		SetInitialShadowState();
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE2, true );
		s_pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 |
			SHADER_DRAW_LIGHTMAP_TEXCOORD1 );
		// FIXME: Remove the normal (needed for tangent space gen)
		s_pShaderShadow->VertexShaderVertexFormat( 
			VERTEX_POSITION, 2, 0, 0, 0 );

		sdk_lightmappedgeneric_basetextureblend_Static_Index vshIndex;
		s_pShaderShadow->SetVertexShader( "SDK_lightmappedgeneric_basetextureblend", vshIndex.GetIndex() );

		s_pShaderShadow->SetPixelShader( "SDK_lightmappedgeneric_basetextureblend", 0 );
		FogToOOOverbright();
	}
	else
	{
		s_pShaderAPI->SetDefaultState();
		BindTexture( SHADER_TEXTURE_STAGE0, baseTextureVar, baseTextureFrameVar );
		BindTexture( SHADER_TEXTURE_STAGE1, baseTexture2Var, baseTextureFrame2Var );
		s_pShaderAPI->BindLightmap( SHADER_TEXTURE_STAGE2 );
		SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, baseTextureTransformVar );
		SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_2, baseTextureTransform2Var );
		SetColorPixelShaderConstant( 0, colorVar, alphaVar );
		sdk_lightmappedgeneric_basetextureblend_Dynamic_Index vshIndex;
		vshIndex.SetDOWATERFOG( s_pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
		s_pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
	}
	Draw();
}

void CBaseVSShader::DrawWorldBumpedUsingVertexShader( int baseTextureVar, int baseTextureTransformVar,
												int bumpmapVar, int bumpFrameVar, 
												int bumpTransformVar,
												int envmapMaskVar, int envmapMaskFrame,
												int envmapVar, 
												int envmapFrameVar,
												int envmapTintVar, int colorVar, int alphaVar,
												int envmapContrastVar, int envmapSaturationVar,
												int frameVar, int fresnelReflectionVar,
												bool doBaseTexture2,
												int baseTexture2Var, int baseTextureTransform2Var,
												int baseTextureFrame2Var
												)
{
	IMaterialVar** params = s_ppParams;
	// Draw base texture
	bool bMultiplyDiffuseLighting = false;
	bool bBlendSpecular = false;
	
	// Draw base texture(s)
	if( doBaseTexture2 && params[baseTexture2Var]->IsTexture() && params[baseTextureVar]->IsTexture() )
	{
		DrawBaseTextureBlend( baseTextureVar, baseTextureTransformVar, frameVar,
			baseTexture2Var, baseTextureTransform2Var, baseTextureFrame2Var, colorVar, alphaVar );
		bMultiplyDiffuseLighting = true;
		bBlendSpecular = true;
	}
	else if( params[baseTextureVar]->IsTexture() )
	{
		DrawWorldBaseTexture( baseTextureVar, baseTextureTransformVar, frameVar, colorVar, alphaVar );
		bMultiplyDiffuseLighting = true;
		bBlendSpecular = true;
	}
	else
	{
		// Just use color here
	}

	// Draw diffuse lighting
	if( params[baseTextureVar]->IsTexture() || !params[envmapVar]->IsTexture() )
	{
		DrawWorldBumpedDiffuseLighting( bumpmapVar, bumpFrameVar, bumpTransformVar, 
			bMultiplyDiffuseLighting );
		bBlendSpecular = true;
	}

	// Add specular lighting
	if( params[envmapVar]->IsTexture() )
	{
		DrawWorldBumpedSpecularLighting(
			bumpmapVar, envmapVar,
			bumpFrameVar, envmapFrameVar,
			envmapTintVar, alphaVar,
			envmapContrastVar, envmapSaturationVar,
			bumpTransformVar, fresnelReflectionVar,
			bBlendSpecular );
	}
}

//-----------------------------------------------------------------------------
// GR - translucency query
//-----------------------------------------------------------------------------
BlendType_t CBaseVSShader::EvaluateBlendRequirements( int textureVar, bool isBaseTexture )
{
	// Either we've got a constant modulation
	bool isTranslucent = IsAlphaModulating();

	// Or we've got a vertex alpha
	isTranslucent = isTranslucent || (CurrentMaterialVarFlags() & MATERIAL_VAR_VERTEXALPHA);

	// Or we've got a texture alpha (for blending or alpha test)
	isTranslucent = isTranslucent || ( TextureIsTranslucent( textureVar, isBaseTexture ) &&
		                               !(CurrentMaterialVarFlags() & MATERIAL_VAR_ALPHATEST ) );

	if ( CurrentMaterialVarFlags() & MATERIAL_VAR_ADDITIVE )
	{
		// Additive
		return isTranslucent ? BT_BLENDADD : BT_ADD;
	}
	else
	{
		// Normal blending
		return isTranslucent ? BT_BLEND : BT_NONE;
	}
}

void CBaseVSShader::SetFlashlightVertexShaderConstants( bool bBump, int bumpTransformVar, bool bSetTextureTransforms )
{
	Assert( !IsSnapshotting() );

	VMatrix worldToTexture;
	const FlashlightState_t &flashlightState = s_pShaderAPI->GetFlashlightState( worldToTexture );

	// Set the flashlight origin
	float pos[4];
	pos[0] = flashlightState.m_vecLightOrigin[0];
	pos[1] = flashlightState.m_vecLightOrigin[1];
	pos[2] = flashlightState.m_vecLightOrigin[2];
	pos[3] = 1.0f;
	s_pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_LIGHT0_POSITION, pos, 1 );

	s_pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_1, worldToTexture.Base(), 4 );

	// Set the flashlight attenuation factors
	float atten[4];
	atten[0] = flashlightState.m_fConstantAtten;
	atten[1] = flashlightState.m_fLinearAtten;
	atten[2] = flashlightState.m_fQuadraticAtten;
	atten[3] = flashlightState.m_FarZ;
	s_pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_5, atten, 1 );

	if( bSetTextureTransforms )
	{
		SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_6, BASETEXTURETRANSFORM );
		if( bBump && bumpTransformVar != -1 )
		{
			SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_8, bumpTransformVar );
		}
	}
}

void CBaseVSShader::DrawFlashlight_dx80( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow, bool bBump,
										int bumpmapVar, int bumpmapFrame, int bumpTransform, int flashlightTextureVar, int flashlightTextureFrameVar,
										bool bLightmappedGeneric, bool bWorldVertexTransition, int nWorldVertexTransitionPassID, int baseTexture2Var,
										int baseTexture2FrameVar, bool bTeeth, int nTeethForwardVar, int nTeethIllumFactorVar )
{
	// FLASHLIGHTFIXME: hack . . need to fix the vertex shader so that it can deal with and without bumps for vertexlitgeneric
	if( !bLightmappedGeneric )
	{
		bBump = false;
	}
	if( pShaderShadow )
	{
		SetInitialShadowState();
		pShaderShadow->EnableDepthWrites( false );

		// Alpha test
		pShaderShadow->EnableAlphaTest( IS_FLAG_SET( MATERIAL_VAR_ALPHATEST ) );

		// Alpha blend
		if( bWorldVertexTransition )
		{
			// use separeate alpha blend to make sure that we aren't adding alpha from source
			if( nWorldVertexTransitionPassID == 0 )
			{
				EnableAlphaBlending( SHADER_BLEND_DST_ALPHA, SHADER_BLEND_ONE );
			}
			else
			{
				EnableAlphaBlending( SHADER_BLEND_ONE_MINUS_DST_ALPHA, SHADER_BLEND_ONE );
			}
		}
		else
		{
			SetAdditiveBlendingShadowState( BASETEXTURE, true );
		}
		
		pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
		pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
		pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE2, true );
		pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE3, true );

		if( bLightmappedGeneric )
		{
			bool bUsingVertexColor = IS_FLAG_SET( MATERIAL_VAR_VERTEXCOLOR );
			sdk_lightmappedgeneric_flashlight_vs11_Static_Index	vshIndex;
			vshIndex.SetNORMALMAP( bBump );
			vshIndex.SetWORLDVERTEXTRANSITION( bWorldVertexTransition );
			vshIndex.SetVERTEXCOLOR( bUsingVertexColor );
			pShaderShadow->SetVertexShader( "SDK_lightmappedgeneric_flashlight_vs11", vshIndex.GetIndex() );

			unsigned int flags = VERTEX_POSITION | VERTEX_NORMAL;
			if( bBump )
			{
				flags |= VERTEX_TANGENT_S | VERTEX_TANGENT_T;
			}
			if ( bWorldVertexTransition || bUsingVertexColor )
			{
				flags |= VERTEX_COLOR;
			}
			pShaderShadow->VertexShaderVertexFormat( flags, 1, 0, 0, 0 );
		}
		else
		{
			sdk_vertexlitgeneric_flashlight_vs11_Static_Index vshIndex;
			vshIndex.SetTEETH( bTeeth );
			pShaderShadow->SetVertexShader( "SDK_vertexlitgeneric_flashlight_vs11", vshIndex.GetIndex() );

			unsigned int flags = VERTEX_POSITION | VERTEX_NORMAL;
			pShaderShadow->VertexShaderVertexFormat( flags, 1, 0, 3, bBump ? 4 : 0 );
		}

		sdk_flashlight_ps11_Static_Index	pshIndex;
		pshIndex.SetNORMALMAP( bBump );
		pShaderShadow->SetPixelShader( "SDK_flashlight_ps11", pshIndex.GetIndex() );

		FogToBlack();
	}
	else
	{
		// Specify that we have XYZ texcoords that need to be divided by W before the pixel shader.
		// NOTE Tried to divide XY by Z, but doesn't work.
		// The dx9.0c runtime says that we shouldn't have a non-zero dimension when using vertex and pixel shaders.
		pShaderAPI->SetTextureTransformDimension( 0, 0, true );
		BindTexture( SHADER_TEXTURE_STAGE0, flashlightTextureVar, flashlightTextureFrameVar );

		if( bWorldVertexTransition && ( nWorldVertexTransitionPassID == 1 ) )
		{
			BindTexture( SHADER_TEXTURE_STAGE1, baseTexture2Var, baseTexture2FrameVar );
		}
		else
		{
			if( params[BASETEXTURE]->IsTexture() )
			{
				BindTexture( SHADER_TEXTURE_STAGE1, BASETEXTURE, FRAME );
			}
			else
			{
				pShaderAPI->BindGrey( SHADER_TEXTURE_STAGE1 );
			}
		}
		pShaderAPI->BindNormalizationCubeMap( SHADER_TEXTURE_STAGE2 );
		if( bBump )
		{
			BindTexture( SHADER_TEXTURE_STAGE3, bumpmapVar, bumpmapFrame );
		}
		else
		{
			pShaderAPI->BindNormalizationCubeMap( SHADER_TEXTURE_STAGE3 );
		}

		if( bLightmappedGeneric )
		{
			sdk_lightmappedgeneric_flashlight_vs11_Dynamic_Index vshIndex;
			vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
		}
		else
		{
			sdk_vertexlitgeneric_flashlight_vs11_Dynamic_Index vshIndex;
			vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );

			if( bTeeth )
			{
				Assert( nTeethForwardVar >= 0 );
				Assert( nTeethIllumFactorVar >= 0 );
				Vector4D lighting;
				params[nTeethForwardVar]->GetVecValue( lighting.Base(), 3 );
				lighting[3] = params[nTeethIllumFactorVar]->GetFloatValue();
				pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, lighting.Base() );
			}

#if !defined( _XBOX )
			vshIndex.SetNUM_BONES( pShaderAPI->GetCurrentNumBones() );
#else
			vshIndex.SetSKINNING( pShaderAPI->GetCurrentNumBones() > 0 );
#endif
			pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
		}

		sdk_flashlight_ps11_Dynamic_Index pshIndex;
		pShaderAPI->SetPixelShaderIndex( pshIndex.GetIndex() );

#ifdef _XBOX
		float val[4];
		val[0] = val[1] = val[2] = val[3] = 2.0f;
		s_pShaderAPI->SetPixelShaderConstant( 0, val );
#endif
		SetFlashlightVertexShaderConstants( bBump, bumpTransform, true );
	}
	Draw();
}

#ifndef _XBOX
#ifdef STDSHADER_DX9_DLL_EXPORT
void CBaseVSShader::DrawFlashlight_dx90( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, 
										IShaderShadow* pShaderShadow, DrawFlashlight_dx90_Vars_t &vars )
{
	// FLASHLIGHTFIXME: hack . . need to fix the vertex shader so that it can deal with and without bumps for vertexlitgeneric
	if( !vars.m_bLightmappedGeneric )
	{
		vars.m_bBump = false;
	}
	bool bBump2 = vars.m_bWorldVertexTransition && vars.m_bBump && vars.m_nBumpmap2Var != -1 && params[vars.m_nBumpmap2Var]->IsTexture();

	if( pShaderShadow )
	{
		SetInitialShadowState();
		pShaderShadow->EnableDepthWrites( false );
		pShaderShadow->EnableAlphaWrites( false );

		// Alpha test
		pShaderShadow->EnableAlphaTest( IS_FLAG_SET( MATERIAL_VAR_ALPHATEST ) );

		// Alpha blend
		SetAdditiveBlendingShadowState( BASETEXTURE, true );
		
		pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
		pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
		pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE2, true );
		pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE3, true );
		pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE7, true );
		if( vars.m_bWorldVertexTransition )
		{
			// $basetexture2
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE4, true );
		}
		if( bBump2 )
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE6, true );
		}
		
		if( vars.m_bLightmappedGeneric )
		{
			sdk_lightmappedgeneric_flashlight_vs20_Static_Index	vshIndex;
			vshIndex.SetWORLDVERTEXTRANSITION( vars.m_bWorldVertexTransition );
			vshIndex.SetNORMALMAP( vars.m_bBump );
			pShaderShadow->SetVertexShader( "SDK_lightmappedgeneric_flashlight_vs20", vshIndex.GetIndex() );

			unsigned int flags = VERTEX_POSITION | VERTEX_NORMAL;
			if( vars.m_bBump )
			{
				flags |= VERTEX_TANGENT_S | VERTEX_TANGENT_T;
			}
			int numTexCoords = 1;
			if( vars.m_bWorldVertexTransition )
			{
				flags |= VERTEX_COLOR;
				numTexCoords = 2; // need lightmap texcoords to get alpha.
			}
			pShaderShadow->VertexShaderVertexFormat( flags, numTexCoords, 0, 0, 0 );
		}
		else
		{
			sdk_vertexlitgeneric_flashlight_vs11_Static_Index vshIndex;
			vshIndex.SetTEETH( vars.m_bTeeth );
			pShaderShadow->SetVertexShader( "SDK_vertexlitgeneric_flashlight_vs11", vshIndex.GetIndex() );

			unsigned int flags = VERTEX_POSITION | VERTEX_NORMAL;
			int numTexCoords = 1;
			pShaderShadow->VertexShaderVertexFormat( flags, numTexCoords, 0, 3, vars.m_bBump ? 4 : 0 );
		}

		sdk_flashlight_ps20_Static_Index	pshIndex;
		pshIndex.SetNORMALMAP( vars.m_bBump );
		pshIndex.SetNORMALMAP2( bBump2 );
		pshIndex.SetWORLDVERTEXTRANSITION( vars.m_bWorldVertexTransition );
		pshIndex.SetHDRTYPE( g_pHardwareConfig->GetHDRType() );
		pShaderShadow->SetPixelShader( "SDK_flashlight_ps20", pshIndex.GetIndex() );

		FogToBlack();
	}
	else
	{
		BindTexture( SHADER_TEXTURE_STAGE0, vars.m_nFlashlightTextureVar, vars.m_nFlashlightTextureFrameVar );

		bool bHasFlashlightDepth = false;
		VMatrix worldToTexture;
		ITexture *pFlashlightDepthTexture;
		FlashlightState_t state = pShaderAPI->GetFlashlightStateEx( worldToTexture, &pFlashlightDepthTexture );
		SetFlashLightColorFromState( state, pShaderAPI );
		if( pFlashlightDepthTexture )
		{
			bHasFlashlightDepth = true;
			BindTexture( SHADER_TEXTURE_STAGE7, pFlashlightDepthTexture, 0 );
		}

		if( params[BASETEXTURE]->IsTexture() )
		{
			BindTexture( SHADER_TEXTURE_STAGE1, BASETEXTURE, FRAME );
		}
		else
		{
			pShaderAPI->BindGrey( SHADER_TEXTURE_STAGE1 );
		}
		if( vars.m_bWorldVertexTransition )
		{
			Assert( vars.m_nBaseTexture2Var >= 0 && vars.m_nBaseTexture2FrameVar >= 0 );
			BindTexture( SHADER_TEXTURE_STAGE4, vars.m_nBaseTexture2Var, vars.m_nBaseTexture2FrameVar );
		}
		pShaderAPI->BindNormalizationCubeMap( SHADER_TEXTURE_STAGE2 );
		if( vars.m_bBump )
		{
			BindTexture( SHADER_TEXTURE_STAGE3, vars.m_nBumpmapVar, vars.m_nBumpmapFrame );
		}
		else
		{
			pShaderAPI->BindNormalizationCubeMap( SHADER_TEXTURE_STAGE3 );
		}

		if( vars.m_bWorldVertexTransition )
		{
			if( bBump2 )
			{
				BindTexture( SHADER_TEXTURE_STAGE6, vars.m_nBumpmap2Var, vars.m_nBumpmap2Frame );
			}
		}

		if( vars.m_bLightmappedGeneric )
		{
			DECLARE_DYNAMIC_VERTEX_SHADER( sdk_lightmappedgeneric_flashlight_vs20 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG, pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			SET_DYNAMIC_VERTEX_SHADER( sdk_lightmappedgeneric_flashlight_vs20 );
		}
		else
		{
			sdk_vertexlitgeneric_flashlight_vs11_Dynamic_Index vshIndex;
			vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			vshIndex.SetNUM_BONES( pShaderAPI->GetCurrentNumBones() );
			pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );

			if( vars.m_bTeeth )
			{
				Assert( vars.m_nTeethForwardVar >= 0 );
				Assert( vars.m_nTeethIllumFactorVar >= 0 );
				Vector4D lighting;
				params[vars.m_nTeethForwardVar]->GetVecValue( lighting.Base(), 3 );
				lighting[3] = params[vars.m_nTeethIllumFactorVar]->GetFloatValue();
				pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, lighting.Base() );
			}
		}

		DECLARE_DYNAMIC_PIXEL_SHADER( sdk_flashlight_ps20 );
		SET_DYNAMIC_PIXEL_SHADER_COMBO( HDRENABLED,  IsHDREnabled() );
		SET_DYNAMIC_PIXEL_SHADER_COMBO( FOGTYPE,  pShaderAPI->GetSceneFogMode() );
		SET_DYNAMIC_PIXEL_SHADER_COMBO( FLASHLIGHTDEPTH, bHasFlashlightDepth );
		SET_DYNAMIC_PIXEL_SHADER( sdk_flashlight_ps20 );

		SetFlashlightVertexShaderConstants( vars.m_bBump, vars.m_nBumpTransform, true );
	}
	Draw();
}
#endif
#endif

void CBaseVSShader::SetWaterFogColorPixelShaderConstantLinear( int reg )
{
	unsigned char fogRGB[3];
	s_pShaderAPI->GetSceneFogColor( fogRGB );
	float waterFogColor[4];
	waterFogColor[0] = GammaToLinear( fogRGB[0] * ( 1.0f / 255.0f ) );
	waterFogColor[1] = GammaToLinear( fogRGB[1] * ( 1.0f / 255.0f ) );
	waterFogColor[2] = GammaToLinear( fogRGB[2] * ( 1.0f / 255.0f ) );
	waterFogColor[3] = 0.0f;
	s_pShaderAPI->SetPixelShaderConstant( reg, waterFogColor );
}

void CBaseVSShader::SetWaterFogColorPixelShaderConstantGamma( int reg )
{
	unsigned char fogRGB[3];
	s_pShaderAPI->GetSceneFogColor( fogRGB );
	float waterFogColor[4];
	waterFogColor[0] = fogRGB[0] * ( 1.0f / 255.0f );
	waterFogColor[1] = fogRGB[1] * ( 1.0f / 255.0f );
	waterFogColor[2] = fogRGB[2] * ( 1.0f / 255.0f );
	waterFogColor[3] = 0.0f;
	s_pShaderAPI->SetPixelShaderConstant( reg, waterFogColor );
}

void CBaseVSShader::InitParamsUnlitGeneric_DX8(
		int baseTextureVar,
		int detailScaleVar,
		int envmapOptionalVar,
		int envmapVar,
		int envmapTintVar, 
		int envmapMaskScaleVar )
{
	IMaterialVar** params = s_ppParams;

	if( envmapTintVar >= 0 && !params[envmapTintVar]->IsDefined() )
	{
		params[envmapTintVar]->SetVecValue( 1.0f, 1.0f, 1.0f );
	}

	if( envmapMaskScaleVar >= 0 && !params[envmapMaskScaleVar]->IsDefined() )
	{
		params[envmapMaskScaleVar]->SetFloatValue( 1.0f );
	}

	if( detailScaleVar >= 0 && !params[detailScaleVar]->IsDefined() )
	{
		params[detailScaleVar]->SetFloatValue( 4.0f );
	}

	// No texture means no self-illum or env mask in base alpha
	if ( baseTextureVar >= 0 && !params[baseTextureVar]->IsDefined() )
	{
		CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
	}

	// If in decal mode, no debug override...
	if (IS_FLAG_SET(MATERIAL_VAR_DECAL))
	{
		SET_FLAGS( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
	}

	// Get rid of the envmap if it's optional for this dx level.
	if( envmapOptionalVar >= 0 && params[envmapOptionalVar]->IsDefined() && params[envmapOptionalVar]->GetIntValue() )
	{
		if (envmapVar >= 0)
		{
			params[envmapVar]->SetUndefined();
		}
	}

	// If mat_specular 0, then get rid of envmap
	if( envmapVar >= 0 && baseTextureVar >= 0 && !g_pConfig->UseSpecular() && params[envmapVar]->IsDefined() && params[baseTextureVar]->IsDefined() )
	{
		params[envmapVar]->SetUndefined();
	}
}

void CBaseVSShader::InitUnlitGeneric_DX8(
		int baseTextureVar,
		int detailVar,
		int envmapVar,
		int envmapMaskVar )
{
	IMaterialVar** params = s_ppParams;

	if (baseTextureVar >= 0 && params[baseTextureVar]->IsDefined())
	{
		LoadTexture( baseTextureVar );

		if (!params[baseTextureVar]->GetTextureValue()->IsTranslucent())
		{
			if (IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK))
				CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
		}
	}

	// Don't alpha test if the alpha channel is used for other purposes
	if ( IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK) )
		CLEAR_FLAGS( MATERIAL_VAR_ALPHATEST );

	// the second texture (if there is one)
	if (detailVar >= 0 && params[detailVar]->IsDefined())
	{
		LoadTexture( detailVar );
	}

	if (envmapVar >= 0 && params[envmapVar]->IsDefined())
	{
		if( !IS_FLAG_SET(MATERIAL_VAR_ENVMAPSPHERE) )
			LoadCubeMap( envmapVar );
		else
			LoadTexture( envmapVar );

		if( !g_pHardwareConfig->SupportsCubeMaps() )
			SET_FLAGS(MATERIAL_VAR_ENVMAPSPHERE);

		if (envmapMaskVar >= 0 && params[envmapMaskVar]->IsDefined())
			LoadTexture( envmapMaskVar );
	}
}

#endif // !_STATIC_LINKED || STDSHADER_DX8_DLL_EXPORT
