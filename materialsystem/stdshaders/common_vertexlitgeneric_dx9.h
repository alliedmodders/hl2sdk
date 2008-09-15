#include "common_ps_fxc.h"

struct PixelShaderLightInfo
{
	float4 color;
	float4 dir;
	float4 pos;
	// These aren't used in pixel shaders.
//	float4 spotParams;
//	float4 atten;
};

#define cOverbright 2.0f
#define cOOOverbright 0.5f

#define LIGHTTYPE_NONE				0
#define LIGHTTYPE_STATIC			1
#define LIGHTTYPE_SPOT				2
#define LIGHTTYPE_POINT				3
#define LIGHTTYPE_DIRECTIONAL		4
#define LIGHTTYPE_AMBIENT			5

static const int g_StaticLightTypeArray[22] = {
	LIGHTTYPE_NONE, LIGHTTYPE_STATIC, 
	LIGHTTYPE_NONE, LIGHTTYPE_NONE, LIGHTTYPE_NONE, LIGHTTYPE_NONE, LIGHTTYPE_NONE, 
	LIGHTTYPE_NONE, LIGHTTYPE_NONE, LIGHTTYPE_NONE, LIGHTTYPE_NONE, LIGHTTYPE_NONE, 
	LIGHTTYPE_STATIC, LIGHTTYPE_STATIC, LIGHTTYPE_STATIC, LIGHTTYPE_STATIC, LIGHTTYPE_STATIC, 
	LIGHTTYPE_STATIC, LIGHTTYPE_STATIC, LIGHTTYPE_STATIC, LIGHTTYPE_STATIC, LIGHTTYPE_STATIC
};

static const int g_AmbientLightTypeArray[22] = {
	LIGHTTYPE_NONE, LIGHTTYPE_NONE, 
	LIGHTTYPE_AMBIENT, LIGHTTYPE_AMBIENT, LIGHTTYPE_AMBIENT, LIGHTTYPE_AMBIENT, LIGHTTYPE_AMBIENT, LIGHTTYPE_AMBIENT, 
	LIGHTTYPE_AMBIENT, LIGHTTYPE_AMBIENT, LIGHTTYPE_AMBIENT, LIGHTTYPE_AMBIENT, 
	LIGHTTYPE_AMBIENT, LIGHTTYPE_AMBIENT, LIGHTTYPE_AMBIENT, LIGHTTYPE_AMBIENT, LIGHTTYPE_AMBIENT, LIGHTTYPE_AMBIENT, 
	LIGHTTYPE_AMBIENT, LIGHTTYPE_AMBIENT, LIGHTTYPE_AMBIENT, LIGHTTYPE_AMBIENT
};

static const int g_LocalLightType0Array[22] = {
	LIGHTTYPE_NONE, LIGHTTYPE_NONE, 
	LIGHTTYPE_NONE, LIGHTTYPE_SPOT, LIGHTTYPE_POINT, LIGHTTYPE_DIRECTIONAL, LIGHTTYPE_SPOT, LIGHTTYPE_SPOT, 
	LIGHTTYPE_SPOT, LIGHTTYPE_POINT, LIGHTTYPE_POINT, LIGHTTYPE_DIRECTIONAL,
	LIGHTTYPE_NONE, LIGHTTYPE_SPOT, LIGHTTYPE_POINT, LIGHTTYPE_DIRECTIONAL, LIGHTTYPE_SPOT, LIGHTTYPE_SPOT, 
	LIGHTTYPE_SPOT, LIGHTTYPE_POINT, LIGHTTYPE_POINT, LIGHTTYPE_DIRECTIONAL
};

static const int g_LocalLightType1Array[22] = {
	LIGHTTYPE_NONE, LIGHTTYPE_NONE, 
	LIGHTTYPE_NONE, LIGHTTYPE_NONE, LIGHTTYPE_NONE, LIGHTTYPE_NONE, LIGHTTYPE_SPOT, LIGHTTYPE_POINT, LIGHTTYPE_DIRECTIONAL, LIGHTTYPE_POINT, LIGHTTYPE_DIRECTIONAL, LIGHTTYPE_DIRECTIONAL,
	LIGHTTYPE_NONE, LIGHTTYPE_NONE, LIGHTTYPE_NONE, LIGHTTYPE_NONE, LIGHTTYPE_SPOT, LIGHTTYPE_POINT, 
	LIGHTTYPE_DIRECTIONAL, LIGHTTYPE_POINT, LIGHTTYPE_DIRECTIONAL, LIGHTTYPE_DIRECTIONAL
};

// Better suited to Pixel shader models, 11 instructions in pixel shader
float3 PixelShaderAmbientLight( const float3 worldNormal, const float3 cAmbientCube[6] )
{
	float3 linearColor, nSquared = worldNormal * worldNormal;
	float3 isNegative = ( worldNormal < 0.0 );
	float3 isPositive = 1-isNegative;

	isNegative *= nSquared;
	isPositive *= nSquared;

	linearColor = isPositive.x * cAmbientCube[0] + isNegative.x * cAmbientCube[1] +
				  isPositive.y * cAmbientCube[2] + isNegative.y * cAmbientCube[3] +
				  isPositive.z * cAmbientCube[4] + isNegative.z * cAmbientCube[5];

	return linearColor;
}

// Better suited to Vertex shader models
// Six VS instructions due to use of constant indexing (slt, mova, mul, mul, mad, mad)
float3 VertexShaderAmbientLight( const float3 worldNormal, const float3 cAmbientCube[6] )
{
	float3 nSquared = worldNormal * worldNormal;
	int3 isNegative = ( worldNormal < 0.0 );
	float3 linearColor;
	linearColor = nSquared.x * cAmbientCube[isNegative.x] +
	              nSquared.y * cAmbientCube[isNegative.y+2] +
	              nSquared.z * cAmbientCube[isNegative.z+4];
	return linearColor;
}

float3 AmbientLight( const float3 worldNormal, const float3 cAmbientCube[6] )
{
	// Vertex shader cases
#ifdef SHADER_MODEL_VS_1_0
	return VertexShaderAmbientLight( worldNormal, cAmbientCube );
#elif SHADER_MODEL_VS_1_1
	return VertexShaderAmbientLight( worldNormal, cAmbientCube );
#elif SHADER_MODEL_VS_2_0
	return VertexShaderAmbientLight( worldNormal, cAmbientCube );
#elif SHADER_MODEL_VS_3_0
	return VertexShaderAmbientLight( worldNormal, cAmbientCube );
#else
	// Pixel shader case
	return PixelShaderAmbientLight( worldNormal, cAmbientCube );
#endif
}



//-----------------------------------------------------------------------------
// Purpose: Compute scalar diffuse term with various optional tweaks such as
//          Half Lambert, ambient occlusion and Lafortune directional diffuse terms
// Output : Returns a scalar result
//-----------------------------------------------------------------------------
float3 DiffuseTerm( const bool bHalfLambert, const float3 worldNormal, const float3 lightDir,
				   const bool bDoAmbientOcclusion, const float fAmbientOcclusion,
				   const bool bDoDirectionalDiffuse, const float3 vEyeDir,
				   const bool bDoLightingWarp, in sampler lightWarpSampler )
{
	float3 fResult, NDotL = dot( worldNormal, lightDir );	// Unsaturated dot (-1 to 1 range)

	if( bHalfLambert )
	{
		fResult = NDotL * 0.5 + 0.5;						// Scale and bias to 0 to 1 range
		fResult *= fResult;									// Square
	}
	else
	{
		fResult = max( 0.0f, NDotL );						// Saturate pure Lambertian term
	}

	if ( bDoDirectionalDiffuse )
	{
		fResult *= dot( vEyeDir, worldNormal ) * 1.3;		// Lafortune directional diffuse term
	}

	if ( bDoAmbientOcclusion )
	{
		// Raise to higher powers for darker AO values
//		float fAOPower = lerp( 4.0f, 1.0f, fAmbientOcclusion );
//		result *= pow( NDotL * 0.5 + 0.5, fAOPower );
		fResult *= fAmbientOcclusion;
	}

	if ( bDoLightingWarp )
	{
		fResult = 2.0f * tex1D( lightWarpSampler, fResult );
	}

	return fResult;
}

float3 PixelShaderDoDiffuseLight( const float3 worldPos, const float3 worldNormal, 
								  int lightNum, int lightType, const float3 vertexColor, in sampler normalizeSampler,
								  PixelShaderLightInfo cLightInfo[2], const bool bHalfLambert, const bool bDoAmbientOcclusion,
								  const float fAmbientOcclusion, const bool bDoDirectionalDiffuse, const float3 vEyeDir,
								  const bool bDoLightingWarp, in sampler lightWarpSampler )
{
	float3 lightDir, color = float3(0,0,0);

	if( ( lightType == LIGHTTYPE_SPOT ) || ( lightType == LIGHTTYPE_POINT ) )					// These two are equivalent for now...
	{
		lightDir = NormalizeWithCubemap( normalizeSampler, cLightInfo[lightNum].pos - worldPos );
		color = vertexColor;
	}
	else if( lightType == LIGHTTYPE_DIRECTIONAL )
	{
		lightDir = -cLightInfo[lightNum].dir;
		color = cLightInfo[lightNum].color;
	}

	return color * DiffuseTerm( bHalfLambert, worldNormal, lightDir, bDoAmbientOcclusion, fAmbientOcclusion,
								bDoDirectionalDiffuse, vEyeDir, bDoLightingWarp, lightWarpSampler );
}

float3 SpecularTerm( const float3 vWorldNormal, const float3 vLightDir, const float fSpecularExponent,
					 const float3 vEyeDir, const bool bDoAmbientOcclusion, const float fAmbientOcclusion )
{
	float3 result = float3(0.0f, 0.0f, 0.0f);

	float3 vReflect = reflect( -vEyeDir, vWorldNormal );			// Reflect view through normal
	float fSpecular = saturate(dot( vReflect, vLightDir ));			// L.R	(use half-angle instead?)
	fSpecular = pow( fSpecular, fSpecularExponent );				// Raise to specular power

	fSpecular *= saturate(dot( vWorldNormal, vLightDir ));			// Mask with N.L

	if ( bDoAmbientOcclusion )
	{
		fSpecular *= fAmbientOcclusion;
	}

	return fSpecular;
}

// Traditional fresnel term approximation
float Fresnel( const float3 vNormal, const float3 vEyeDir )
{
	float fresnel = 1-saturate( dot( vNormal, vEyeDir ) );	// 1-(N.V) for Fresnel term
	return fresnel * fresnel;								// Square for a more subtle look
}

//
// Custom Fresnel with low, mid and high parameters defining a piecewise continuous function
// with traditional fresnel (0 to 1 range) as input.  The 0 to 0.5 range blends between
// low and mid while the 0.5 to 1 range blends between mid and high
//
//    |
//    |    .  M . . . H
//    | . 
//    L
//    |
//    +----------------
//    0               1
//
float Fresnel( const float3 vNormal, const float3 vEyeDir, float fLow, float fMid, float fHigh )
{
	float result, f = Fresnel( vNormal, vEyeDir);			// Traditional Fresnel

	if ( f > 0.5f )
		result = lerp( fMid, fHigh, (2*f)-1 );				// Blend between mid and high values
	else
        result = lerp( fLow, fMid, 2*f );					// Blend between low and mid values

	return result;
}

float3 PixelShaderDoSpecularLight( const float3 vWorldPos, const float3 vWorldNormal, const float fSpecularExponent, const float3 vEyeDir,
								   int lightNum, int lightType, const float3 vertexColor, PixelShaderLightInfo cLightInfo[2],
								   const bool bDoAmbientOcclusion, const float fAmbientOcclusion )
{
	float3 vLightDir, color = float3(0,0,0);

	if ( ( lightType == LIGHTTYPE_SPOT ) || ( lightType == LIGHTTYPE_POINT ) )	// These two are equivalent for now...
	{
		vLightDir = normalize( cLightInfo[lightNum].pos - vWorldPos );
		color = vertexColor;
	}
	else if( lightType == LIGHTTYPE_DIRECTIONAL )
	{
		vLightDir = -cLightInfo[lightNum].dir;
		color = cLightInfo[lightNum].color;
	}

	return color * SpecularTerm( vWorldNormal, vLightDir, fSpecularExponent, vEyeDir, bDoAmbientOcclusion, fAmbientOcclusion );
}

float3 PixelShaderDoLightingLinear( const float3 worldPos, const float3 worldNormal,
				   const float3 staticLightingColor, const int staticLightType,
				   const int ambientLightType, const int localLightType0,
				   const int localLightType1, const float3 vertexColor0,
				   const float3 vertexColor1, const float3 cAmbientCube[6],
				   in sampler normalizeSampler, PixelShaderLightInfo cLightInfo[2], const bool bHalfLambert,
				   const int localLightType2, const int localLightType3,
				   const bool bDoAmbientOcclusion, const float fAmbientOcclusion,
				   const bool bDoDirectionalDiffuse, const float3 vEyeDir,
				   const bool bDoLightingWarp, in sampler lightWarpSampler )
{
	float3 linearColor = 0.0f;
	if( staticLightType == LIGHTTYPE_STATIC )
	{
		// The static lighting comes in in gamma space and has also been premultiplied by $cOOOverbright
		// need to get it into
		// linear space so that we can do adds.
		linearColor += GammaToLinear( staticLightingColor * cOverbright );
	}

	if( ambientLightType == LIGHTTYPE_AMBIENT )
	{
		float3 ambient = AmbientLight( worldNormal, cAmbientCube );

		if ( bDoAmbientOcclusion )
			ambient *= fAmbientOcclusion * fAmbientOcclusion;

		linearColor += ambient;
	}

	if( localLightType0 != LIGHTTYPE_NONE )
	{
		linearColor += PixelShaderDoDiffuseLight( worldPos, worldNormal, 0, localLightType0, vertexColor0, normalizeSampler,
												  cLightInfo, bHalfLambert, bDoAmbientOcclusion, fAmbientOcclusion,
												  bDoDirectionalDiffuse, vEyeDir, bDoLightingWarp, lightWarpSampler );
	}

	if( localLightType1 != LIGHTTYPE_NONE )
	{
		linearColor += PixelShaderDoDiffuseLight( worldPos, worldNormal, 1, localLightType1, vertexColor1, normalizeSampler,
												  cLightInfo, bHalfLambert, bDoAmbientOcclusion, fAmbientOcclusion,
												  bDoDirectionalDiffuse, vEyeDir, bDoLightingWarp, lightWarpSampler );
	}

	return linearColor;
}

float3 PixelShaderDoSpecularLighting( const float3 worldPos, const float3 worldNormal, const float fSpecularExponent, const float3 vEyeDir,
									  const int localLightType0, const int localLightType1, const float3 vertexColor0, const float3 vertexColor1,
									  PixelShaderLightInfo cLightInfo[2], const bool bDoAmbientOcclusion=false, const float fAmbientOcclusion=0 )
{
	float3 returnColor = float3( 0.0f, 0.0f, 0.0f );

	if( localLightType0 != LIGHTTYPE_NONE )
	{
		returnColor += PixelShaderDoSpecularLight( worldPos, worldNormal, fSpecularExponent, vEyeDir, 0,
												   localLightType0, vertexColor0, cLightInfo,
												   bDoAmbientOcclusion, fAmbientOcclusion );
	}

	if( localLightType1 != LIGHTTYPE_NONE )
	{
		returnColor += PixelShaderDoSpecularLight( worldPos, worldNormal, fSpecularExponent, vEyeDir, 1,
												   localLightType1, vertexColor1, cLightInfo,
												   bDoAmbientOcclusion, fAmbientOcclusion );
	}

	return returnColor;
}


// Called directly by newer shaders or through the following wrapper for older shaders
float3 PixelShaderDoLightingTwoLights( const float3 worldPos, const float3 worldNormal,
				   const float3 staticLightingColor, const int staticLightType,
				   const int ambientLightType, const int localLightType0,
				   const int localLightType1, const float modulation,
				   const float3 vertexColor0, const float3 vertexColor1, const float3 cAmbientCube[6],
				   in sampler normalizeSampler, PixelShaderLightInfo cLightInfo[2], const bool bHalfLambert,
				   
				   // New optional/experimental parameters
				   const bool bDoAmbientOcclusion, const float fAmbientOcclusion,
				   const bool bDoDirectionalDiffuse, const float3 vEyeDir,
				   const bool bDoLightingWarp, in sampler lightWarpSampler )
{
	PixelShaderLightInfo lightInfo[2];
	lightInfo[0] = cLightInfo[0];
	lightInfo[1] = cLightInfo[1];

	float3 returnColor;

	// special case for no lighting
	if( staticLightType  == LIGHTTYPE_NONE && 
		ambientLightType == LIGHTTYPE_NONE &&
		localLightType0  == LIGHTTYPE_NONE &&
		localLightType1  == LIGHTTYPE_NONE )
	{
		returnColor = float3( 0.0f, 0.0f, 0.0f );
	}
	else if( staticLightType  == LIGHTTYPE_STATIC && 
			 ambientLightType == LIGHTTYPE_NONE &&
			 localLightType0  == LIGHTTYPE_NONE &&
			 localLightType1  == LIGHTTYPE_NONE )
	{
		// special case for static lighting only
		returnColor = GammaToLinear( staticLightingColor );
	}
	else
	{
		float3 linearColor;

		linearColor = PixelShaderDoLightingLinear( worldPos, worldNormal, staticLightingColor, 
			staticLightType, ambientLightType, localLightType0, localLightType1,
			vertexColor0, vertexColor1, cAmbientCube, normalizeSampler, lightInfo, bHalfLambert,
			LIGHTTYPE_NONE, LIGHTTYPE_NONE, bDoAmbientOcclusion, fAmbientOcclusion,
			bDoDirectionalDiffuse, vEyeDir, bDoLightingWarp, lightWarpSampler );

		if (modulation != 1.0f)
		{
			linearColor *= modulation;
		}

		// go ahead and clamp to the linear space equivalent of overbright 2 so that we match
		// everything else.
//		returnColor = HuePreservingColorClamp( linearColor, pow( 2.0f, 2.2 ) );
		returnColor = linearColor;
	}

	return returnColor;
}


// Two lights...legacy parameter list...
float3 PixelShaderDoLighting( const float3 worldPos, const float3 worldNormal,
				   const float3 staticLightingColor, const int staticLightType,
				   const int ambientLightType, const int localLightType0,
				   const int localLightType1, const float modulation,
				   const float3 vertexColor0, const float3 vertexColor1, const float3 cAmbientCube[6],
				   in sampler normalizeSampler, PixelShaderLightInfo cLightInfo[2], const bool bHalfLambert )
{
	// Call new entrypoint, hooking up default params
	return PixelShaderDoLightingTwoLights( worldPos, worldNormal, staticLightingColor, staticLightType,
				   ambientLightType, localLightType0, localLightType1, modulation,
				   vertexColor0, vertexColor1, cAmbientCube,
				   normalizeSampler, cLightInfo, bHalfLambert,
				   // Defaults for new params
				   false, 0, false, float3(1,0,0), false, normalizeSampler );
}
