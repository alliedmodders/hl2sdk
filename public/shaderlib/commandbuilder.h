//===== Copyright © 1996-2007, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
// Utility class for building command buffers into memory
//===========================================================================//

#ifndef COMMANDBUILDER_H
#define COMMANDBUILDER_H


#ifdef _WIN32
#pragma once
#endif


#include "shaderapi/commandbuffer.h"
#include "shaderlib/BaseShader.h"
#include "tier1/convar.h"


#ifdef DBGFLAG_ASSERT
#define TRACK_STORAGE 1
#else
#define TRACK_STORAGE 0
#endif

//-----------------------------------------------------------------------------
// Buffer for storing commands into
//-----------------------------------------------------------------------------
template<int N> class CFixedCommandStorageBuffer
{
public:
	uint8 m_Data[N];

	uint8 *m_pDataOut;
#if TRACK_STORAGE
	size_t m_nNumBytesRemaining;
#endif
	
	FORCEINLINE CFixedCommandStorageBuffer( void )
	{
		m_pDataOut = m_Data;
#if TRACK_STORAGE
		m_nNumBytesRemaining = N;
#endif

	}

	FORCEINLINE void EnsureCapacity( size_t sz )
	{
#if TRACK_STORAGE
		if ( m_nNumBytesRemaining < sz + 32 )
			Error( "getting scary\n" );
#endif
		Assert( m_nNumBytesRemaining >= sz );
	}

	template<class T> FORCEINLINE void Put( T const &nValue )
	{
		EnsureCapacity( sizeof( T ) );
		*( reinterpret_cast<T *>( m_pDataOut ) ) = nValue;
		m_pDataOut += sizeof( nValue );
#if TRACK_STORAGE
		m_nNumBytesRemaining -= sizeof( nValue );
#endif
	}

	FORCEINLINE void PutInt( int nValue )
	{
		Put( nValue );
	}

	FORCEINLINE void PutFloat( float nValue )
	{
		Put( nValue );
	}

	FORCEINLINE void PutPtr( void * pPtr )
	{
		Put( pPtr );
	}
	
	FORCEINLINE void PutMemory( const void *pMemory, size_t nBytes )
	{
		EnsureCapacity( nBytes );
		memcpy( m_pDataOut, pMemory, nBytes );
		m_pDataOut += nBytes;
#if TRACK_STORAGE
		m_nNumBytesRemaining -= nBytes;
#endif
	}

	FORCEINLINE uint8 *Base( void )
	{
		return m_Data;
	}

	FORCEINLINE void Reset( void )
	{
		m_pDataOut = m_Data;
#if TRACK_STORAGE
		m_nNumBytesRemaining = N;
#endif
	}

	FORCEINLINE size_t Size( void ) const
	{
		return m_pDataOut - m_Data;
	}

};


//-----------------------------------------------------------------------------
// Base class used to build up command buffers
//-----------------------------------------------------------------------------
template<class S> class CBaseCommandBufferBuilder
{
public:
	S m_Storage;

	FORCEINLINE void End( void )
	{
		m_Storage.PutInt( CBCMD_END );
	}

	FORCEINLINE IMaterialVar *Param( int nVar ) const
	{
		return CBaseShader::s_ppParams[nVar];
	}

	FORCEINLINE void Reset( void )
	{
		m_Storage.Reset();
	}

	FORCEINLINE size_t Size( void ) const
	{
		return m_Storage.Size();
	}

	FORCEINLINE uint8 *Base( void )
	{
		return m_Storage.Base();
	}

	FORCEINLINE void OutputConstantData( float const *pSrcData )
	{
		m_Storage.PutFloat( pSrcData[0] );
		m_Storage.PutFloat( pSrcData[1] );
		m_Storage.PutFloat( pSrcData[2] );
		m_Storage.PutFloat( pSrcData[3] );
	}

	FORCEINLINE void OutputConstantData4( float flVal0, float flVal1, float flVal2, float flVal3 )
	{
		m_Storage.PutFloat( flVal0 );
		m_Storage.PutFloat( flVal1 );
		m_Storage.PutFloat( flVal2 );
		m_Storage.PutFloat( flVal3 );
	}
};


//-----------------------------------------------------------------------------
// Used by SetPixelShaderFlashlightState
//-----------------------------------------------------------------------------
struct CBCmdSetPixelShaderFlashlightState_t
{
	Sampler_t m_LightSampler;
	Sampler_t m_DepthSampler;
	Sampler_t m_ShadowNoiseSampler;
	int m_nColorConstant;
	int m_nAttenConstant;
	int m_nOriginConstant;
	int m_nDepthTweakConstant;
	int m_nScreenScaleConstant;
	int m_nWorldToTextureConstant;
	bool m_bFlashlightNoLambert;
	bool m_bSinglePassFlashlight;
};


//-----------------------------------------------------------------------------
// Used to build a per-pass command buffer
//-----------------------------------------------------------------------------
template<class S> class CCommandBufferBuilder : public CBaseCommandBufferBuilder<S>
{
	typedef CBaseCommandBufferBuilder<S> PARENT;

public:
	FORCEINLINE void End( void )
	{
		this->m_Storage.PutInt( CBCMD_END );
	}

	FORCEINLINE void SetPixelShaderConstants( int nFirstConstant, int nConstants )
	{
		this->m_Storage.PutInt( CBCMD_SET_PIXEL_SHADER_FLOAT_CONST );
		this->m_Storage.PutInt( nFirstConstant );
		this->m_Storage.PutInt( nConstants );
	}

	FORCEINLINE void OutputConstantData( float const *pSrcData )
	{
		this->m_Storage.PutFloat( pSrcData[0] );
		this->m_Storage.PutFloat( pSrcData[1] );
		this->m_Storage.PutFloat( pSrcData[2] );
		this->m_Storage.PutFloat( pSrcData[3] );
	}
	
	FORCEINLINE void OutputConstantData4( float flVal0, float flVal1, float flVal2, float flVal3 )
	{
		this->m_Storage.PutFloat( flVal0 );
		this->m_Storage.PutFloat( flVal1 );
		this->m_Storage.PutFloat( flVal2 );
		this->m_Storage.PutFloat( flVal3 );
	}

	FORCEINLINE void SetPixelShaderConstant( int nFirstConstant, float const *pSrcData, int nNumConstantsToSet )
	{
		SetPixelShaderConstants( nFirstConstant, nNumConstantsToSet );
		this->m_Storage.PutMemory( pSrcData, 4 * sizeof( float ) * nNumConstantsToSet );
	}

	FORCEINLINE void SetPixelShaderConstant( int nFirstConstant, int nVar )
	{
		SetPixelShaderConstant( nFirstConstant, this->Param( nVar )->GetVecValue() );
	}

	void SetPixelShaderConstantGammaToLinear( int pixelReg, int constantVar )
	{
		float val[4];
		this->Param(constantVar)->GetVecValue( val, 3 );
		val[0] = val[0] > 1.0f ? val[0] : GammaToLinear( val[0] );
		val[1] = val[1] > 1.0f ? val[1] : GammaToLinear( val[1] );
		val[2] = val[2] > 1.0f ? val[2] : GammaToLinear( val[2] );
		val[3] = 1.0;
		SetPixelShaderConstant( pixelReg, val );
	}

	FORCEINLINE void SetPixelShaderConstant( int nFirstConstant, float const *pSrcData )
	{
		SetPixelShaderConstants( nFirstConstant, 1 );
		OutputConstantData( pSrcData );
	}

	FORCEINLINE void SetPixelShaderConstant4( int nFirstConstant, float flVal0, float flVal1, float flVal2, float flVal3 )
	{
		SetPixelShaderConstants( nFirstConstant, 1 );
		OutputConstantData4( flVal0, flVal1, flVal2, flVal3 );
	}

	FORCEINLINE void SetPixelShaderConstant_W( int pixelReg, int constantVar, float fWValue )
	{
		if ( constantVar != -1 )
		{
			float val[3];
			this->Param(constantVar)->GetVecValue( val, 3);
			SetPixelShaderConstant4( pixelReg, val[0], val[1], val[2], fWValue );
		}
	}

	FORCEINLINE void SetVertexShaderConstant( int nFirstConstant, float const *pSrcData )
	{
		this->m_Storage.PutInt( CBCMD_SET_VERTEX_SHADER_FLOAT_CONST );
		this->m_Storage.PutInt( nFirstConstant );
		this->m_Storage.PutInt( 1 );
		OutputConstantData( pSrcData );
	}

	FORCEINLINE void SetVertexShaderConstant( int nFirstConstant, float const *pSrcData, int nConsts )
	{
		this->m_Storage.PutInt( CBCMD_SET_VERTEX_SHADER_FLOAT_CONST );
		this->m_Storage.PutInt( nFirstConstant );
		this->m_Storage.PutInt( nConsts );
		this->m_Storage.PutMemory( pSrcData, 4 * nConsts * sizeof( float ) );
	}


	FORCEINLINE void SetVertexShaderConstant4( int nFirstConstant, float flVal0, float flVal1, float flVal2, float flVal3 )
	{
		this->m_Storage.PutInt( CBCMD_SET_VERTEX_SHADER_FLOAT_CONST );
		this->m_Storage.PutInt( nFirstConstant );
		this->m_Storage.PutInt( 1 );
		this->m_Storage.PutFloat( flVal0 );
		this->m_Storage.PutFloat( flVal1 );
		this->m_Storage.PutFloat( flVal2 );
		this->m_Storage.PutFloat( flVal3 );
	}

	void SetVertexShaderTextureTransform( int vertexReg, int transformVar )
	{
		Vector4D transformation[2];
		IMaterialVar* pTransformationVar = this->Param( transformVar );
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
		SetVertexShaderConstant( vertexReg, transformation[0].Base(), 2 ); 
	}


	void SetVertexShaderTextureScaledTransform( int vertexReg, int transformVar, int scaleVar )
	{
		Vector4D transformation[2];
		IMaterialVar* pTransformationVar = this->Param( transformVar );
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
		IMaterialVar* pScaleVar = this->Param( scaleVar );
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
		SetVertexShaderConstant( vertexReg, transformation[0].Base(), 2 ); 
	}

	FORCEINLINE void SetEnvMapTintPixelShaderDynamicState( int pixelReg, int tintVar )
	{
#ifdef _WIN32
		extern ConVar mat_fullbright;
		if( g_pConfig->bShowSpecular && mat_fullbright.GetInt() != 2 )
		{
			SetPixelShaderConstant( pixelReg, this->Param( tintVar)->GetVecValue() );
		}
		else
		{
			SetPixelShaderConstant4( pixelReg, 0.0, 0.0, 0.0, 0.0 );
		}
#endif
	}

	FORCEINLINE void SetEnvMapTintPixelShaderDynamicStateGammaToLinear( int pixelReg, int tintVar, float fAlphaVal = 1.0f )
	{
#ifdef _WIN32
		extern ConVar mat_fullbright;
		if( g_pConfig->bShowSpecular && mat_fullbright.GetInt() != 2 )
		{
			float color[4];
			color[3] = fAlphaVal;
			this->Param( tintVar)->GetLinearVecValue( color, 3 );
			SetPixelShaderConstant( pixelReg, color );
		}
		else
		{
			SetPixelShaderConstant4( pixelReg, 0.0, 0.0, 0.0, fAlphaVal );
		}
#endif
	}

	FORCEINLINE void StoreEyePosInPixelShaderConstant( int nConst, float wValue = 1.0f )
	{
		this->m_Storage.PutInt( CBCMD_STORE_EYE_POS_IN_PSCONST );
		this->m_Storage.PutInt( nConst );
		this->m_Storage.PutFloat( wValue );
	}

	FORCEINLINE void SetPixelShaderFogParams( int nReg )
	{
		this->m_Storage.PutInt( CBCMD_SETPIXELSHADERFOGPARAMS );
		this->m_Storage.PutInt( nReg );
	}

	FORCEINLINE void BindStandardTexture( Sampler_t nSampler, StandardTextureId_t nTextureId )
	{
		this->m_Storage.PutInt( CBCMD_BIND_STANDARD_TEXTURE );
		this->m_Storage.PutInt( nSampler );
		this->m_Storage.PutInt( nTextureId );
	}

	FORCEINLINE void BindTexture( CBaseShader *pShader, Sampler_t nSampler, ITexture *pTexture, int nFrame )
	{
		ShaderAPITextureHandle_t hTexture = pShader->GetShaderAPITextureBindHandle( pTexture, nFrame );
		Assert( hTexture != INVALID_SHADERAPI_TEXTURE_HANDLE );
		this->m_Storage.PutInt( CBCMD_BIND_SHADERAPI_TEXTURE_HANDLE );
		this->m_Storage.PutInt( nSampler );
		this->m_Storage.PutInt( hTexture );
	}

	FORCEINLINE void BindTexture( Sampler_t nSampler, ShaderAPITextureHandle_t hTexture )
	{
		Assert( hTexture != INVALID_SHADERAPI_TEXTURE_HANDLE );
		this->m_Storage.PutInt( CBCMD_BIND_SHADERAPI_TEXTURE_HANDLE );
		this->m_Storage.PutInt( nSampler );
		this->m_Storage.PutInt( hTexture );
	}

	FORCEINLINE void BindTexture( CBaseShader *pShader, Sampler_t nSampler, int nTextureVar, int nFrameVar = -1 )
	{
		ShaderAPITextureHandle_t hTexture = pShader->GetShaderAPITextureBindHandle( nTextureVar, nFrameVar );
		BindTexture( nSampler, hTexture );
	}

	FORCEINLINE void BindMultiTexture( CBaseShader *pShader, Sampler_t nSampler1, Sampler_t nSampler2, int nTextureVar, int nFrameVar )
	{
		ShaderAPITextureHandle_t hTexture = pShader->GetShaderAPITextureBindHandle( nTextureVar, nFrameVar, 0 );
		BindTexture( nSampler1, hTexture );
		hTexture = pShader->GetShaderAPITextureBindHandle( nTextureVar, nFrameVar, 1 );
		BindTexture( nSampler2, hTexture );
	}

	FORCEINLINE void SetPixelShaderIndex( int nIndex )
	{
		this->m_Storage.PutInt( CBCMD_SET_PSHINDEX );
		this->m_Storage.PutInt( nIndex );
	}

	FORCEINLINE void SetVertexShaderIndex( int nIndex )
	{
		this->m_Storage.PutInt( CBCMD_SET_VSHINDEX );
		this->m_Storage.PutInt( nIndex );
	}

	FORCEINLINE void SetDepthFeatheringPixelShaderConstant( int iConstant, float fDepthBlendScale )
	{
		this->m_Storage.PutInt( CBCMD_SET_DEPTH_FEATHERING_CONST );
		this->m_Storage.PutInt( iConstant );
		this->m_Storage.PutFloat( fDepthBlendScale );
	}

	FORCEINLINE void SetVertexShaderFlashlightState( int iConstant )
	{
		this->m_Storage.PutInt( CBCMD_SET_VERTEX_SHADER_FLASHLIGHT_STATE );
		this->m_Storage.PutInt( iConstant );
	}

	FORCEINLINE void SetPixelShaderFlashlightState( const CBCmdSetPixelShaderFlashlightState_t &state )
	{
		this->m_Storage.PutInt( CBCMD_SET_PIXEL_SHADER_FLASHLIGHT_STATE );
		this->m_Storage.PutInt( state.m_LightSampler );
		this->m_Storage.PutInt( state.m_DepthSampler );
		this->m_Storage.PutInt( state.m_ShadowNoiseSampler );
		this->m_Storage.PutInt( state.m_nColorConstant );
		this->m_Storage.PutInt( state.m_nAttenConstant );
		this->m_Storage.PutInt( state.m_nOriginConstant );
		this->m_Storage.PutInt( state.m_nDepthTweakConstant );
		this->m_Storage.PutInt( state.m_nScreenScaleConstant );
		this->m_Storage.PutInt( state.m_nWorldToTextureConstant );
		this->m_Storage.PutInt( state.m_bFlashlightNoLambert );
		this->m_Storage.PutInt( state.m_bSinglePassFlashlight );
	}

	FORCEINLINE void SetPixelShaderUberLightState( int iEdge0Const, int iEdge1Const, int iEdgeOOWConst, int iShearRoundConst, int iAABBConst, int iWorldToLightConst )
	{
		this->m_Storage.PutInt( CBCMD_SET_PIXEL_SHADER_UBERLIGHT_STATE );
		this->m_Storage.PutInt( iEdge0Const );
		this->m_Storage.PutInt( iEdge1Const );
		this->m_Storage.PutInt( iEdgeOOWConst );
		this->m_Storage.PutInt( iShearRoundConst );
		this->m_Storage.PutInt( iAABBConst );
		this->m_Storage.PutInt( iWorldToLightConst );
	}

	FORCEINLINE void Goto( uint8 *pCmdBuf )
	{
		this->m_Storage.PutInt( CBCMD_JUMP );
		this->m_Storage.PutPtr( pCmdBuf );
	}

	FORCEINLINE void Call( uint8 *pCmdBuf )
	{
		this->m_Storage.PutInt( CBCMD_JSR );
		this->m_Storage.PutPtr( pCmdBuf );
	}

	FORCEINLINE void Reset( void )
	{
		this->m_Storage.Reset();
	}
	
	FORCEINLINE size_t Size( void ) const
	{
		return this->m_Storage.Size();
	}

	FORCEINLINE uint8 *Base( void )
	{
		return this->m_Storage.Base();
	}

	FORCEINLINE void SetVertexShaderNearAndFarZ( int iRegNum )
	{
		this->m_Storage.PutInt( CBCMD_SET_VERTEX_SHADER_NEARZFARZ_STATE );
		this->m_Storage.PutInt( iRegNum );
	}
};


//-----------------------------------------------------------------------------
// Builds a command buffer specifically for per-instance state setting
//-----------------------------------------------------------------------------
template<class S> class CInstanceCommandBufferBuilder : public CBaseCommandBufferBuilder< S >
{
	typedef CBaseCommandBufferBuilder< S > PARENT;

public:
	FORCEINLINE void End( void )
	{
		this->m_Storage.PutInt( CBICMD_END );
	}

	FORCEINLINE void SetPixelShaderLocalLighting( int nConst )
	{
		this->m_Storage.PutInt( CBICMD_SETPIXELSHADERLOCALLIGHTING );
		this->m_Storage.PutInt( nConst );
	}

	FORCEINLINE void SetPixelShaderAmbientLightCube( int nConst )
	{
		this->m_Storage.PutInt( CBICMD_SETPIXELSHADERAMBIENTLIGHTCUBE );
		this->m_Storage.PutInt( nConst );
	}

	FORCEINLINE void SetVertexShaderLocalLighting( )
	{
		this->m_Storage.PutInt( CBICMD_SETVERTEXSHADERLOCALLIGHTING );
	}

	FORCEINLINE void SetVertexShaderAmbientLightCube( void )
	{
		this->m_Storage.PutInt( CBICMD_SETVERTEXSHADERAMBIENTLIGHTCUBE );
	}

	FORCEINLINE void SetSkinningMatrices( void )
	{
		this->m_Storage.PutInt( CBICMD_SETSKINNINGMATRICES );
	}

	FORCEINLINE void SetPixelShaderAmbientLightCubeLuminance( int nConst )
	{
		this->m_Storage.PutInt( CBICMD_SETPIXELSHADERAMBIENTLIGHTCUBELUMINANCE );
		this->m_Storage.PutInt( nConst );
	}

	FORCEINLINE void SetPixelShaderGlintDamping( int nConst )
	{
		this->m_Storage.PutInt( CBICMD_SETPIXELSHADERGLINTDAMPING );
		this->m_Storage.PutInt( nConst );
	}

	FORCEINLINE void SetModulationPixelShaderDynamicState_LinearColorSpace_LinearScale( int nConst, const Vector &vecGammaSpaceColor2Factor, float scale )
	{
		this->m_Storage.PutInt( CBICMD_SETMODULATIONPIXELSHADERDYNAMICSTATE_LINEARCOLORSPACE_LINEARSCALE );
		this->m_Storage.PutInt( nConst );
		this->m_Storage.Put( vecGammaSpaceColor2Factor );
		this->m_Storage.PutFloat( scale );
	}

	FORCEINLINE void SetModulationPixelShaderDynamicState_LinearScale( int nConst, const Vector &vecGammaSpaceColor2Factor, float scale )
	{
		this->m_Storage.PutInt( CBICMD_SETMODULATIONPIXELSHADERDYNAMICSTATE_LINEARSCALE );
		this->m_Storage.PutInt( nConst );
		this->m_Storage.Put( vecGammaSpaceColor2Factor );
		this->m_Storage.PutFloat( scale );
	}

	FORCEINLINE void SetModulationPixelShaderDynamicState_LinearScale_ScaleInW( int nConst, const Vector &vecGammaSpaceColor2Factor, float scale )
	{
		this->m_Storage.PutInt( CBICMD_SETMODULATIONPIXELSHADERDYNAMICSTATE_LINEARSCALE_SCALEINW );
		this->m_Storage.PutInt( nConst );
		this->m_Storage.Put( vecGammaSpaceColor2Factor );
		this->m_Storage.PutFloat( scale );
	}

	FORCEINLINE void SetModulationPixelShaderDynamicState_LinearColorSpace( int nConst, const Vector &vecGammaSpaceColor2Factor )
	{
		this->m_Storage.PutInt( CBICMD_SETMODULATIONPIXELSHADERDYNAMICSTATE_LINEARCOLORSPACE );
		this->m_Storage.PutInt( nConst );
		this->m_Storage.Put( vecGammaSpaceColor2Factor );
	}

	FORCEINLINE void SetModulationPixelShaderDynamicState( int nConst, const Vector &vecGammaSpaceColor2Factor )
	{
		this->m_Storage.PutInt( CBICMD_SETMODULATIONPIXELSHADERDYNAMICSTATE );
		this->m_Storage.PutInt( nConst );
		this->m_Storage.Put( vecGammaSpaceColor2Factor );
	}

	FORCEINLINE void SetModulationPixelShaderDynamicState_Identity( int nConst )
	{
		this->m_Storage.PutInt( CBICMD_SETMODULATIONPIXELSHADERDYNAMICSTATE_IDENTITY );
		this->m_Storage.PutInt( nConst );
	}

	FORCEINLINE void SetModulationVertexShaderDynamicState( int nConst, const Vector &vecGammaSpaceColor2Factor )
	{
		this->m_Storage.PutInt( CBICMD_SETMODULATIONVERTEXSHADERDYNAMICSTATE );
		this->m_Storage.PutInt( nConst );
		this->m_Storage.Put( vecGammaSpaceColor2Factor );
	}
	
	FORCEINLINE void SetModulationVertexShaderDynamicState_LinearScale( int nConst, const Vector &vecGammaSpaceColor2Factor, float flScale )
	{
		this->m_Storage.PutInt( CBICMD_SETMODULATIONVERTEXSHADERDYNAMICSTATE_LINEARSCALE );
		this->m_Storage.PutInt( nConst );
		this->m_Storage.Put( vecGammaSpaceColor2Factor );
		this->m_Storage.PutFloat( flScale );
	}
};


#endif // commandbuilder_h
