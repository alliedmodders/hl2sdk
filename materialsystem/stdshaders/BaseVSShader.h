//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
// This is what all vs/ps (dx8+) shaders inherit from.
//=============================================================================//

#ifndef BASEVSSHADER_H
#define BASEVSSHADER_H

#ifdef _WIN32		   
#pragma once
#endif

#include "shaderlib/cshader.h"
#include "shaderlib/baseshader.h"
#include <renderparm.h>


//-----------------------------------------------------------------------------
// Standard vertex shader constants used by shaders in the std shader DLLs
//-----------------------------------------------------------------------------
enum
{
	VERTEX_SHADER_MODULATION_COLOR = 37,
};

//-----------------------------------------------------------------------------
// Helper macro for vertex shaders
//-----------------------------------------------------------------------------
#define BEGIN_VS_SHADER_FLAGS(_name, _help, _flags)	__BEGIN_SHADER_INTERNAL( CBaseVSShader, _name, _help, _flags )
#define BEGIN_VS_SHADER(_name,_help)	__BEGIN_SHADER_INTERNAL( CBaseVSShader, _name, _help, 0 )


// GR - indication of alpha usage
enum BlendType_t
{
	// no alpha blending
	BT_NONE = 0,



	// src * srcAlpha + dst * (1-srcAlpha)
	// two passes for HDR:
	//		pass 1:
	//			color: src * srcAlpha + dst * (1-srcAlpha)
	//			alpha: srcAlpha * zero + dstAlpha * (1-srcAlpha)
	//		pass 2:
	//			color: none
	//			alpha: srcAlpha * one + dstAlpha * one
	//
	BT_BLEND,


	
	// src * one + dst * one
	// one pass for HDR
	BT_ADD,


	
	// Why do we ever use this instead of using premultiplied alpha?
	// src * srcAlpha + dst * one
	// two passes for HDR
	//		pass 1:
	//			color: src * srcAlpha + dst * one
	//			alpha: srcAlpha * one + dstAlpha * one
	//		pass 2:
	//			color: none
	//			alpha: srcAlpha * one + dstAlpha * one
	BT_BLENDADD
};

//-----------------------------------------------------------------------------
// Base class for shaders, contains helper methods.
//-----------------------------------------------------------------------------
class CBaseVSShader : public CBaseShader
{
public:

	// Loads bump lightmap coordinates into the pixel shader
	void LoadBumpLightmapCoordinateAxes_PixelShader( int pixelReg );

	// Loads bump lightmap coordinates into the vertex shader
	void LoadBumpLightmapCoordinateAxes_VertexShader( int vertexReg );

	// Pixel and vertex shader constants....
	void SetPixelShaderConstant( int pixelReg, int constantVar );

	// Pixel and vertex shader constants....
	void SetPixelShaderConstantGammaToLinear( int pixelReg, int constantVar );

	// This version will put constantVar into x,y,z, and constantVar2 into the w
	void SetPixelShaderConstant( int pixelReg, int constantVar, int constantVar2 );
	void SetPixelShaderConstantGammaToLinear( int pixelReg, int constantVar, int constantVar2 );

	// Helpers for setting constants that need to be converted to linear space (from gamma space).
	void SetVertexShaderConstantGammaToLinear( int var, float const* pVec, int numConst = 1, bool bForce = false );
	void SetPixelShaderConstantGammaToLinear( int var, float const* pVec, int numConst = 1, bool bForce = false );

	void SetVertexShaderConstant( int vertexReg, int constantVar );

	// GR - fix for const/lerp issues
	void SetPixelShaderConstantFudge( int pixelReg, int constantVar );

	// Sets light direction for pixel shaders.
	void SetPixelShaderLightColors( int pixelReg );

	// Sets vertex shader texture transforms
	void SetVertexShaderTextureTranslation( int vertexReg, int translationVar );
	void SetVertexShaderTextureScale( int vertexReg, int scaleVar );
 	void SetVertexShaderTextureTransform( int vertexReg, int transformVar );
	void SetVertexShaderTextureScaledTransform( int vertexReg, 
											int transformVar, int scaleVar );

	// Set pixel shader texture transforms
	void SetPixelShaderTextureTranslation( int pixelReg, int translationVar );
	void SetPixelShaderTextureScale( int pixelReg, int scaleVar );
 	void SetPixelShaderTextureTransform( int pixelReg, int transformVar );
	void SetPixelShaderTextureScaledTransform( int pixelReg, 
											int transformVar, int scaleVar );

	// Moves a matrix into vertex shader constants 
	void SetVertexShaderMatrix3x4( int vertexReg, int matrixVar );
	void SetVertexShaderMatrix4x4( int vertexReg, int matrixVar );

	// Loads the view matrix into pixel shader constants
	void LoadViewMatrixIntoVertexShaderConstant( int vertexReg );

	// Sets up ambient light cube...
	void SetAmbientCubeDynamicStateVertexShader( );
	float GetAmbientLightCubeLuminance( );

	// Helpers for dealing with envmaptint
	void SetEnvMapTintPixelShaderDynamicState( int pixelReg, int tintVar, int alphaVar, bool bConvertFromGammaToLinear = false );
	
	// Helper methods for pixel shader overbrighting
	void EnablePixelShaderOverbright( int reg, bool bEnable, bool bDivideByTwo );

	// Helper for dealing with modulation
	void SetModulationVertexShaderDynamicState();
	void SetModulationPixelShaderDynamicState( int modulationVar );
	void SetModulationPixelShaderDynamicState_LinearColorSpace( int modulationVar );
	void SetModulationPixelShaderDynamicState_LinearColorSpace_LinearScale( int modulationVar, float flScale );

	// Sets a color + alpha into shader constants
	void SetColorVertexShaderConstant( int nVertexReg, int colorVar, int alphaVar );
	void SetColorPixelShaderConstant( int nPixelReg, int colorVar, int alphaVar );

	//
	// Standard shader passes!
	//

	void InitParamsUnlitGeneric_DX8( 
		int baseTextureVar,
		int detailScaleVar,
		int envmapOptionalVar,
		int envmapVar,
		int envmapTintVar, 
		int envmapMaskScaleVar );

	void InitUnlitGeneric_DX8( 
		int baseTextureVar,
		int detailVar,
		int envmapVar,
		int envmapMaskVar );

	// Dx8 Unlit Generic pass
	void VertexShaderUnlitGenericPass( bool doSkin, int baseTextureVar, int frameVar, 
		int baseTextureTransformVar, int detailVar, int detailTransform, bool bDetailTransformIsScale, 
		int envmapVar, int envMapFrameVar, int envmapMaskVar, int envmapMaskFrameVar, 
		int envmapMaskScaleVar, int envmapTintVar, int alphaTestReferenceVar ) ;

	// Helpers for drawing world bump mapped stuff.
	void DrawModelBumpedSpecularLighting( int bumpMapVar, int bumpMapFrameVar,
											   int envMapVar, int envMapVarFrame,
											   int envMapTintVar, int alphaVar,
											   int envMapContrastVar, int envMapSaturationVar,
											   int bumpTransformVar,
											   bool bBlendSpecular, bool bNoWriteZ = false );
	void DrawWorldBumpedSpecularLighting( int bumpmapVar, int envmapVar,
											   int bumpFrameVar, int envmapFrameVar,
											   int envmapTintVar, int alphaVar,
											   int envmapContrastVar, int envmapSaturationVar,
											   int bumpTransformVar, int fresnelReflectionVar,
											   bool bBlend, bool bNoWriteZ = false );
	const char *UnlitGeneric_ComputeVertexShaderName( bool bMask,
														  bool bEnvmap,
														  bool bBaseTexture,
														  bool bBaseAlphaEnvmapMask,
														  bool bDetail,
														  bool bVertexColor,
														  bool bEnvmapCameraSpace,
														  bool bEnvmapSphere );
	const char *UnlitGeneric_ComputePixelShaderName( bool bMask,
														  bool bEnvmap,
														  bool bBaseTexture,
														  bool bBaseAlphaEnvmapMask,
														  bool bDetail );
	void DrawWorldBaseTexture( int baseTextureVar, int baseTextureTransformVar, int frameVar, int colorVar, int alphaVar );
	void DrawWorldBumpedDiffuseLighting( int bumpmapVar, int bumpFrameVar,
		int bumpTransformVar, bool bMultiply );
	void DrawWorldBumpedSpecularLighting( int envmapMaskVar, int envmapMaskFrame,
		int bumpmapVar, int envmapVar,
		int bumpFrameVar, int envmapFrameVar,
		int envmapTintVar, int alphaVar,
		int envmapContrastVar, int envmapSaturationVar,
		int bumpTransformVar,  int fresnelReflectionVar,
		bool bBlend );
	void DrawBaseTextureBlend( int baseTextureVar, int baseTextureTransformVar, 
		int baseTextureFrameVar,
		int baseTexture2Var, int baseTextureTransform2Var, 
		int baseTextureFrame2Var, int colorVar, int alphaVar );
	void DrawWorldBumpedDiffuseLighting_Base_ps14( int bumpmapVar, int bumpFrameVar,
		int bumpTransformVar, int baseTextureVar, int baseTextureTransformVar, int frameVar );
	void DrawWorldBumpedDiffuseLighting_Blend_ps14( int bumpmapVar, int bumpFrameVar, int bumpTransformVar, 
		int baseTextureVar, int baseTextureTransformVar, int baseTextureFrameVar, 
		int baseTexture2Var, int baseTextureTransform2Var, int baseTextureFrame2Var);
	void DrawWorldBumpedUsingVertexShader( int baseTextureVar, int baseTextureTransformVar,
		int bumpmapVar, int bumpFrameVar, 
		int bumpTransformVar,
		int envmapMaskVar, int envmapMaskFrame,
		int envmapVar, 
		int envmapFrameVar,
		int envmapTintVar, int colorVar, int alphaVar,
		int envmapContrastVar, int envmapSaturationVar, int frameVar, int fresnelReflectionVar,
		bool doBaseTexture2 = false,
		int baseTexture2Var = -1, int baseTextureTransform2Var = -1,
		int baseTextureFrame2Var = -1 
		);
	
	// Computes the shader index for vertex lit materials
	int ComputeVertexLitShaderIndex( bool bVertexLitGeneric, bool hasBump, bool hasEnvmap, bool hasVertexColor, bool bHasNormal ) const;

	BlendType_t EvaluateBlendRequirements( int textureVar, bool isBaseTexture );

	// Helper for setting up flashlight constants
	void SetFlashlightVertexShaderConstants( bool bBump, int bumpTransformVar, bool bSetTextureTransforms );

	void DrawFlashlight_dx80( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow, 
		bool bBump, int bumpmapVar, int bumpmapFrame, int bumpTransform, int flashlightTextureVar, 
		int flashlightTextureFrameVar, bool bLightmappedGeneric, bool bWorldVertexTransition, 
		int nWorldVertexTransitionPassID, int baseTexture2Var, int baseTexture2FrameVar,
		bool bTeeth=false, int nTeethForwardVar=0, int nTeethIllumFactorVar=0 );

	struct DrawFlashlight_dx90_Vars_t
	{
		DrawFlashlight_dx90_Vars_t() 
		{ 
			// set all ints to -1
			memset( this, 0xFF, sizeof(DrawFlashlight_dx90_Vars_t) ); 
			// set all bools to a default value.
			m_bBump = false;
			m_bLightmappedGeneric = false;
			m_bWorldVertexTransition = false;
			m_bTeeth = false;
		}
		bool m_bBump;
		bool m_bLightmappedGeneric;
		bool m_bWorldVertexTransition;
		bool m_bTeeth;
		int m_nBumpmapVar;
		int m_nBumpmapFrame;
		int m_nBumpTransform;
		int m_nFlashlightTextureVar;
		int m_nFlashlightTextureFrameVar;
		int m_nBaseTexture2Var;
		int m_nBaseTexture2FrameVar;
		int m_nBumpmap2Var;
		int m_nBumpmap2Frame;
		int m_nTeethForwardVar;
		int m_nTeethIllumFactorVar;
	};
	void DrawFlashlight_dx90( IMaterialVar** params, 
		IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow, DrawFlashlight_dx90_Vars_t &vars );
	void SetWaterFogColorPixelShaderConstantLinear( int reg );
	void SetWaterFogColorPixelShaderConstantGamma( int reg );
private:
	// Helper methods for VertexLitGenericPass
	void UnlitGenericShadowState( int baseTextureVar, int detailVar, int envmapVar, int envmapMaskVar, bool doSkin );
	void UnlitGenericDynamicState( int baseTextureVar, int frameVar, int baseTextureTransformVar,
		int detailVar, int detailTransform, bool bDetailTransformIsScale, int envmapVar, 
		int envMapFrameVar, int envmapMaskVar, int envmapMaskFrameVar,
		int envmapMaskScaleVar, int envmapTintVar );

	// Converts a color + alpha into a vector4
	void ColorVarsToVector( int colorVar, int alphaVar, Vector4D &color );

};

FORCEINLINE void SetFlashLightColorFromState( FlashlightState_t const &state, IShaderDynamicAPI *pShaderAPI, int nPSRegister=28 )
{
	float tmscale= ( pShaderAPI->GetToneMappingScaleLinear() ).x;
	tmscale=1.0/tmscale;
	float const *color=state.m_Color.Base();
	float psconsts[4]={ tmscale*color[0], tmscale*color[1], tmscale*color[2], color[3] };
	pShaderAPI->SetPixelShaderConstant( nPSRegister, (float *) psconsts );
}


class ConVar;

#ifdef _DEBUG
extern ConVar mat_envmaptintoverride;
extern ConVar mat_envmaptintscale;
#endif


#endif // BASEVSSHADER_H
