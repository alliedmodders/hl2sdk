//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// This is where all common code for pixel shaders go.
#include "common_fxc.h"

// Put global skip commands here. . make sure and check that the appropriate vars are defined
// so these aren't used on the wrong shaders!
// --------------------------------------------------------------------------------
// Ditch all fastpath attemps if we are doing LIGHTING_PREVIEW.
//	SKIP: defined $LIGHTING_PREVIEW && defined $FASTPATH && $LIGHTING_PREVIEW && $FASTPATH
// --------------------------------------------------------------------------------

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
	LIGHTTYPE_NONE, LIGHTTYPE_SPOT,  LIGHTTYPE_POINT, LIGHTTYPE_DIRECTIONAL, LIGHTTYPE_SPOT, LIGHTTYPE_SPOT, 
	LIGHTTYPE_SPOT, LIGHTTYPE_POINT, LIGHTTYPE_POINT, LIGHTTYPE_DIRECTIONAL,
	LIGHTTYPE_NONE, LIGHTTYPE_SPOT,  LIGHTTYPE_POINT, LIGHTTYPE_DIRECTIONAL, LIGHTTYPE_SPOT, LIGHTTYPE_SPOT, 
	LIGHTTYPE_SPOT, LIGHTTYPE_POINT, LIGHTTYPE_POINT, LIGHTTYPE_DIRECTIONAL
};

static const int g_LocalLightType1Array[22] = {
	LIGHTTYPE_NONE, LIGHTTYPE_NONE, 
	LIGHTTYPE_NONE, LIGHTTYPE_NONE, LIGHTTYPE_NONE, LIGHTTYPE_NONE, LIGHTTYPE_SPOT, LIGHTTYPE_POINT, LIGHTTYPE_DIRECTIONAL, LIGHTTYPE_POINT, LIGHTTYPE_DIRECTIONAL, LIGHTTYPE_DIRECTIONAL,
	LIGHTTYPE_NONE, LIGHTTYPE_NONE, LIGHTTYPE_NONE, LIGHTTYPE_NONE, LIGHTTYPE_SPOT, LIGHTTYPE_POINT, 
	LIGHTTYPE_DIRECTIONAL, LIGHTTYPE_POINT, LIGHTTYPE_DIRECTIONAL, LIGHTTYPE_DIRECTIONAL
};

#define FOGTYPE_RANGE				0
#define FOGTYPE_HEIGHT				1

#define COMPILE_ERROR ( 1/0; )

// -------------------------
// CONSTANTS
// -------------------------
//const float4 cConstants0				: register(c0);

#define cZero	0.0f
#define cOne	1.0f
#define cTwo	2.0f
#define cHalf	0.5f

#pragma def ( vs, c0, 0.0f, 1.0f, 2.0f, 0.5f )
//const float4 cMathConstants0			: register(c0);

const float4 cConstants1				: register(c1);
#define cOOGamma			cConstants1.x
#define cOverbright			2.0f
#define cOneThird			cConstants1.z
#define cOOOverbright		( 1.0f / 2.0f )

const float4 cEyePosWaterZ				: register(c2);
#define cEyePos			cEyePosWaterZ.xyz

// This is still used by asm stuff.
const float4 cObsoleteLightIndex		: register(c3);

const float4x4 cModelViewProj			: register(c4);
const float4x4 cViewProj				: register(c8);
const float4x4 cModelView				: register(c12);

// Only cFlexScale.x is used
// It is a binary value used to switch on/off the addition of the flex delta stream
const float4 cFlexScale					: register(c13);

const float4 cFogParams					: register(c16);
#define cFogEndOverFogRange cFogParams.x
#define cFogOne cFogParams.y
// NOTE cFogParams.z is unused!
#define cOOFogRange cFogParams.w

const float4x4 cViewModel				: register(c17);

const float3 cAmbientCube[6]			: register(c21);

struct LightInfo
{
	float4 color;
	float4 dir;
	float4 pos;
	float4 spotParams;
	float4 atten;
};

LightInfo cLightInfo[2]					: register(c27);
#define LIGHT_0_POSITION_REG						c29

const float4 cModulationColor			: register(c37);

#define SHADER_SPECIFIC_CONST_0 c38
#define SHADER_SPECIFIC_CONST_1 c39
#define SHADER_SPECIFIC_CONST_2 c40
#define SHADER_SPECIFIC_CONST_3 c41
#define SHADER_SPECIFIC_CONST_4 c42
#define SHADER_SPECIFIC_CONST_5 c43
#define SHADER_SPECIFIC_CONST_6 c44
#define SHADER_SPECIFIC_CONST_7 c45
#define SHADER_SPECIFIC_CONST_8 c46
#define SHADER_SPECIFIC_CONST_9 c47

static const int cModel0Index = 48;
const float4x3 cModel[16]					: register(c48);
// last cmodel is c95 for dx80, c204 for dx90


#define DOT_PRODUCT_FACTORS_CONST c240
const float4 cDotProductFactors[4]			: register( DOT_PRODUCT_FACTORS_CONST );

#define MORPH_FACTORS_CONST c244
const float4 cMorphFactors[8]				: register( MORPH_FACTORS_CONST );

#define VERTEX_TEXTURE_DIM_CONST c252
const float2 cVertexTextureDim[4]			: register( VERTEX_TEXTURE_DIM_CONST );


//-----------------------------------------------------------------------------
// Methods to sample specific fields of the vertex textures.
// Note that x and y are *unnormalized*
//-----------------------------------------------------------------------------
float SampleVertexTexture( sampler2D vt, int stage, float flElement, float flField )
{
	// Compute normalized x + y values. 
	// Note that pixel centers are at integer coords, *not* at 0.5!
	float4 t;
	t.x = flField / ( cVertexTextureDim[stage].y - 1.0f );
	t.y = flElement / ( cVertexTextureDim[stage].x - 1.0f );
	t.z = t.w = 0.f;
	return tex2Dlod( vt, t ).x;
}


//-----------------------------------------------------------------------------
// Returns the factor associated w/ a morph target
//-----------------------------------------------------------------------------
float GetMorphFactor( float flMorphTargetId )
{
	float flMorphIndex = flMorphTargetId / 4;
	float flMorphComponent = fmod( flMorphTargetId, 4 );
	return dot( cMorphFactors[flMorphIndex], cDotProductFactors[flMorphComponent] );
}


float RangeFog( const float3 projPos )
{
	return -projPos.z * cOOFogRange + cFogEndOverFogRange;
}

float WaterFog( const float3 worldPos, const float3 projPos )
{
	float4 tmp;
	
	tmp.xy = cEyePosWaterZ.wz - worldPos.z;

	// tmp.x is the distance from the water surface to the vert
	// tmp.y is the distance from the eye position to the vert

	// if $tmp.x < 0, then set it to 0
	// This is the equivalent of moving the vert to the water surface if it's above the water surface
	
	tmp.x = max( cZero, tmp.x );

	// $tmp.w = $tmp.x / $tmp.y
	tmp.w = tmp.x / tmp.y;

	tmp.w *= projPos.z;

	// $tmp.w is now the distance that we see through water.

	return -tmp.w * cOOFogRange + cFogOne;
}

float CalcFog( const float3 worldPos, const float3 projPos, const int fogType )
{
	if( fogType == FOGTYPE_RANGE )
	{
		return RangeFog( projPos );
	}
	else
	{
#if SHADERMODEL_VS_2_0 == 1
		// We do this work in the pixel shader in dx9, so don't do any fog here.
		return 1.0f;
#else
		return WaterFog( worldPos, projPos );
#endif
	}
}

void SkinPosition( int numBones, const float4 modelPos, 
                   const float4 boneWeights, float4 fBoneIndices,
				   out float3 worldPos )
{
	int3 boneIndices = D3DCOLORtoUBYTE4( fBoneIndices );

	if( numBones == 0 )
	{
		worldPos = mul4x3( modelPos, cModel[0] );
	}
	else if( numBones == 1 )
	{
		worldPos = mul4x3( modelPos, cModel[boneIndices[0]] );
	}
	else if( numBones == 2 )
	{
		float4x3 blendMatrix = cModel[boneIndices[0]] * boneWeights[0] +
							   cModel[boneIndices[1]] * boneWeights[1];
		worldPos = mul4x3( modelPos, blendMatrix );
	}
	else // if( numBones == 3 )
	{
		float4x3 mat1 = cModel[boneIndices[0]];
		float4x3 mat2 = cModel[boneIndices[1]];
		float4x3 mat3 = cModel[boneIndices[2]];

		float weight2 = 1.0f - boneWeights[0] - boneWeights[1];

		float4x3 blendMatrix = mat1 * boneWeights[0] + mat2 * boneWeights[1] + mat3 * weight2;
		worldPos = mul4x3( modelPos, blendMatrix );
	}
}

void SkinPositionAndNormal( int numBones, const float4 modelPos, const float3 modelNormal,
                            const float4 boneWeights, float4 fBoneIndices,
						    out float3 worldPos, out float3 worldNormal )
{
	int3 boneIndices = D3DCOLORtoUBYTE4( fBoneIndices );

	if( numBones == 0 )
	{
		worldPos = mul4x3( modelPos, cModel[0] );
		worldNormal = mul3x3( modelNormal, ( const float3x3 )cModel[0] );
	}
	else if( numBones == 1 )
	{
		worldPos = mul4x3( modelPos, cModel[boneIndices[0]] );
		worldNormal = mul3x3( modelNormal, ( const float3x3 )cModel[boneIndices[0]] );
	}
	else if( numBones == 2 )
	{
		float4x3 blendMatrix = cModel[boneIndices[0]] * boneWeights[0] +
							   cModel[boneIndices[1]] * boneWeights[1];
		worldPos = mul4x3( modelPos, blendMatrix );
		worldNormal = mul3x3( modelNormal, ( float3x3 )blendMatrix );
	}
	else // if( numBones == 3 )
	{
		float4x3 mat1 = cModel[boneIndices[0]];
		float4x3 mat2 = cModel[boneIndices[1]];
		float4x3 mat3 = cModel[boneIndices[2]];

		float weight2 = 1.0f - boneWeights[0] - boneWeights[1];

		float4x3 blendMatrix = mat1 * boneWeights[0] + mat2 * boneWeights[1] + mat3 * weight2;
		worldPos = mul4x3( modelPos, blendMatrix );
		worldNormal = mul3x3( modelNormal, ( float3x3 )blendMatrix );
	}
}

// Is it worth keeping SkinPosition and SkinPositionAndNormal around since the optimizer
// gets rid of anything that isn't used?
void SkinPositionNormalAndTangentSpace( 
#ifdef USE_CONDITIONALS
						   bool bZeroBones, bool bOneBone, bool bTwoBones,
#else
						   int numBones, 
#endif
						    const float4 modelPos, const float3 modelNormal, 
							const float4 modelTangentS,
                            const float4 boneWeights, float4 fBoneIndices,
						    out float3 worldPos, out float3 worldNormal, 
							out float3 worldTangentS, out float3 worldTangentT )
{
	int3 boneIndices = D3DCOLORtoUBYTE4( fBoneIndices );

#ifdef USE_CONDITIONALS
	if( bZeroBones )
#else
	if( numBones == 0 )
#endif
	{
//		worldPos = mul( float4( modelPos, 1.0f ), cModel[0] );
		worldPos = mul4x3( modelPos, cModel[0] );
		worldNormal = mul3x3( modelNormal, ( const float3x3 )cModel[0] );
		worldTangentS = mul3x3( ( float3 )modelTangentS, ( const float3x3 )cModel[0] );
	}
#ifdef USE_CONDITIONALS
	else if( bOneBone )
#else
	else if( numBones == 1 )
#endif
	{
		worldPos = mul4x3( modelPos, cModel[boneIndices[0]] );
		worldNormal = mul3x3( modelNormal, ( const float3x3 )cModel[boneIndices[0]] );
		worldTangentS = mul3x3( ( float3 )modelTangentS, ( const float3x3 )cModel[boneIndices[0]] );
	}
#ifdef USE_CONDITIONALS
	else if( bTwoBones )
#else
	else if( numBones == 2 )
#endif
	{
		float4x3 blendMatrix = cModel[boneIndices[0]] * boneWeights[0] +
							   cModel[boneIndices[1]] * boneWeights[1];
		worldPos = mul4x3( modelPos, blendMatrix );
		worldNormal = mul3x3( modelNormal, ( float3x3 )blendMatrix );
		worldTangentS = mul3x3( ( float3 )modelTangentS, ( const float3x3 )blendMatrix );
	}
	else // if( numBones == 3 )
	{
		float4x3 mat1 = cModel[boneIndices[0]];
		float4x3 mat2 = cModel[boneIndices[1]];
		float4x3 mat3 = cModel[boneIndices[2]];

		float weight2 = 1.0f - boneWeights[0] - boneWeights[1];

		float4x3 blendMatrix = mat1 * boneWeights[0] + mat2 * boneWeights[1] + mat3 * weight2;
		worldPos = mul4x3( modelPos, blendMatrix );
		worldNormal = mul3x3( modelNormal, ( float3x3 )blendMatrix );
		worldTangentS = mul3x3( ( float3 )modelTangentS, ( const float3x3 )blendMatrix );
	}
	worldTangentT = cross( worldNormal, worldTangentS ) * modelTangentS.w;
}


//-----------------------------------------------------------------------------
//
// Lighting helper functions
//
//-----------------------------------------------------------------------------
float3 AmbientLight( const float3 worldNormal )
{
	float3 nSquared = worldNormal * worldNormal;
	int3 isNegative = ( worldNormal < 0.0 );
	float3 linearColor;
	linearColor = nSquared.x * cAmbientCube[isNegative.x] +
	              nSquared.y * cAmbientCube[isNegative.y+2] +
	              nSquared.z * cAmbientCube[isNegative.z+4];
	return linearColor;
}
  
float3 SpotLight( const float3 worldPos, const float3 worldNormal, int lightNum, bool bHalfLambert )
{
	// Direct mapping of current code
	float3 lightDir = cLightInfo[lightNum].pos - worldPos;

	// normalize light direction, maintain temporaries for attenuation
	float lightDistSquared = dot( lightDir, lightDir );
	float ooLightDist = rsqrt( lightDistSquared );
	lightDir *= ooLightDist;
	
	float3 attenuationFactors;
	attenuationFactors = dst( lightDistSquared, ooLightDist );

	float flDistanceAttenuation = dot( attenuationFactors, cLightInfo[lightNum].atten );
	flDistanceAttenuation = 1.0f / flDistanceAttenuation;
	
	// There's an additional falloff we need to make to get the edges looking good
	// and confine the light within a sphere.
	float flLinearFactor = saturate( 1.0f - lightDistSquared * cLightInfo[lightNum].atten.w ); 
	flDistanceAttenuation *= flLinearFactor;

	float nDotL;
	if (!bHalfLambert)
	{
		// compute n dot l
		nDotL = dot( worldNormal, lightDir );
		nDotL = max( cZero, nDotL );
	}
	else
	{
		// half-lambert
		nDotL = dot( worldNormal, lightDir ) * 0.5 + 0.5;
		nDotL = nDotL * nDotL;
	}
	
	// compute angular attenuation
	float flCosTheta = dot( cLightInfo[lightNum].dir, -lightDir );
	float flAngularAtten = (flCosTheta - cLightInfo[lightNum].spotParams.z) * cLightInfo[lightNum].spotParams.w;
	flAngularAtten = max( cZero, flAngularAtten );
	flAngularAtten = pow( flAngularAtten, cLightInfo[lightNum].spotParams.x );
	flAngularAtten = min( cOne, flAngularAtten );

	return cLightInfo[lightNum].color * flDistanceAttenuation * flAngularAtten * nDotL;
}

float3 PointLight( const float3 worldPos, const float3 worldNormal, int lightNum, bool bHalfLambert )
{
	// Get light direction
	float3 lightDir = cLightInfo[lightNum].pos - worldPos;

	// Get light distance squared.
	float lightDistSquared = dot( lightDir, lightDir );

	// Get 1/lightDistance
	float ooLightDist = rsqrt( lightDistSquared );

	// Normalize light direction
	lightDir *= ooLightDist;

	// compute distance attenuation factors.
	float3 attenuationFactors;
	attenuationFactors.x = 1.0f;
	attenuationFactors.y = lightDistSquared * ooLightDist;
	attenuationFactors.z = lightDistSquared;

	float flDistanceAtten = 1.0f / dot( cLightInfo[lightNum].atten.xyz, attenuationFactors );

	// There's an additional falloff we need to make to get the edges looking good
	// and confine the light within a sphere.
	float flLinearFactor = saturate( 1.0f - lightDistSquared * cLightInfo[lightNum].atten.w ); 
	flDistanceAtten *= flLinearFactor;

	float NDotL;
	if ( !bHalfLambert )
	{
		// compute n dot l
		NDotL = dot( worldNormal, lightDir );
		NDotL = max( cZero, NDotL );
	}
	else
	{
		// half-lambert
		NDotL = dot( worldNormal, lightDir ) * 0.5 + 0.5;
		NDotL = NDotL * NDotL;
	}

	return cLightInfo[lightNum].color * NDotL * flDistanceAtten;
}


float3 DirectionalLight( const float3 worldNormal, int lightNum, bool bHalfLambert )
{
	float NDotL;
	if ( !bHalfLambert )
	{
		// compute n dot l
		NDotL = dot( worldNormal, -cLightInfo[lightNum].dir );
		NDotL = max( cZero, NDotL );
	}
	else
	{
		// half-lambert
		NDotL = dot( worldNormal, -cLightInfo[lightNum].dir ) * 0.5 + 0.5;
		NDotL = NDotL * NDotL;
	}
	return cLightInfo[lightNum].color * NDotL;
}

float3 DoLight( const float3 worldPos, const float3 worldNormal, 
				int lightNum, int lightType, bool bHalfLambert )
{
	float3 color = 0.0f;
	if( lightType == LIGHTTYPE_SPOT )
	{
		color = SpotLight( worldPos, worldNormal, lightNum, bHalfLambert );
	}
	else if( lightType == LIGHTTYPE_POINT )
	{
		color = PointLight( worldPos, worldNormal, lightNum, bHalfLambert );
	}
	else if( lightType == LIGHTTYPE_DIRECTIONAL )
	{
		color = DirectionalLight( worldNormal, lightNum, bHalfLambert );
	}
	return color;
}

float3 DoLightingLinear( const float3 worldPos, const float3 worldNormal,
				   const float3 staticLightingColor, const int staticLightType,
				   const int ambientLightType, const int localLightType0,
				   const int localLightType1, bool bHalfLambert )
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
		linearColor += AmbientLight( worldNormal );
	}

	if( localLightType0 != LIGHTTYPE_NONE )
	{
		linearColor += DoLight( worldPos, worldNormal, 0, localLightType0, bHalfLambert );
	}

	if( localLightType1 != LIGHTTYPE_NONE )
	{
		linearColor += DoLight( worldPos, worldNormal, 1, localLightType1, bHalfLambert );
	}

	return linearColor;
}


float3 DoLighting( const float3 worldPos, const float3 worldNormal,
				   const float3 staticLightingColor, const int staticLightType,
				   const int ambientLightType, const int localLightType0,
				   const int localLightType1, const float modulation, bool bHalfLambert )
{
	float3 returnColor;

	// special case for no lighting
	if( staticLightType == LIGHTTYPE_NONE && 
		ambientLightType == LIGHTTYPE_NONE &&
		localLightType0 == LIGHTTYPE_NONE &&
		localLightType1 == LIGHTTYPE_NONE )
	{
		returnColor = float3( 0.0f, 0.0f, 0.0f );
	}
	else if( staticLightType == LIGHTTYPE_NONE && 
			 ambientLightType == LIGHTTYPE_AMBIENT &&
			 localLightType0 == LIGHTTYPE_NONE &&
			 localLightType1 == LIGHTTYPE_NONE )
	{
		returnColor = AmbientLight( worldNormal );
	}
	else if( staticLightType == LIGHTTYPE_STATIC && 
			 ambientLightType == LIGHTTYPE_NONE &&
			 localLightType0 == LIGHTTYPE_NONE &&
			 localLightType1 == LIGHTTYPE_NONE )
	{
		returnColor = GammaToLinear( staticLightingColor * cOverbright );
	}
	else
	{
		float3 linearColor = DoLightingLinear( worldPos, worldNormal, staticLightingColor, 
			staticLightType, ambientLightType, localLightType0, localLightType1, bHalfLambert );

		if (modulation != 1.0f)
		{
			linearColor *= modulation;
		}

		// for dx9, we don't need to scale back down to 0..1 for overbrighting.
		// FIXME: But we're going to because there's some visual difference between dx8 + dx9 if we don't
		// gotta look into that later.
//		returnColor = HuePreservingColorClamp( cOOOverbright * LinearToGamma( linearColor ) );
		returnColor = linearColor;
	}

	return returnColor;
}



// returns a linear HDR light value
float3 DoLightingHDR( const float3 worldPos, const float3 worldNormal,
				   const float3 staticLightingColor, const int staticLightType,
				   const int ambientLightType, const int localLightType0,
				   const int localLightType1, const float modulation, bool bHalfLambert )
{
	float3 returnColor;

	// special case for no lighting
	if( staticLightType == LIGHTTYPE_NONE && 
		ambientLightType == LIGHTTYPE_NONE &&
		localLightType0 == LIGHTTYPE_NONE &&
		localLightType1 == LIGHTTYPE_NONE )
	{
		returnColor = float3( 0.0f, 0.0f, 0.0f );
	}
	else if( staticLightType == LIGHTTYPE_STATIC && 
			 ambientLightType == LIGHTTYPE_NONE &&
			 localLightType0 == LIGHTTYPE_NONE &&
			 localLightType1 == LIGHTTYPE_NONE )
	{
		// special case for static lighting only
		// FIXME!!  Should store HDR values per vertex for static prop lighting.
		returnColor = GammaToLinear( staticLightingColor * cOverbright );
	}
	else
	{
		float3 linearColor = DoLightingLinear( worldPos, worldNormal, staticLightingColor, 
			staticLightType, ambientLightType, localLightType0, localLightType1, bHalfLambert );

		if (modulation != 1.0f)
		{
			linearColor *= modulation;
		}

		returnColor = linearColor;
	}

	return returnColor;
}

int4 FloatToInt( in float4 floats )
{
	return D3DCOLORtoUBYTE4( floats.zyxw / 255.001953125 );
}

float2 ComputeSphereMapTexCoords( in float3 reflectionVector )
{
	// transform reflection vector into view space
	reflectionVector = mul( reflectionVector, ( float3x3 )cViewModel );

	// generate <rx ry rz+1>
	float3 tmp = float3( reflectionVector.x, reflectionVector.y, reflectionVector.z + 1.0f );

	// find 1 / len
	float ooLen = dot( tmp, tmp );
	ooLen = 1.0f / sqrt( ooLen );

	// tmp = tmp/|tmp| + 1
	tmp.xy = ooLen * tmp.xy + 1.0f;

	return tmp.xy * cHalf;
}

//-----------------------------------------------------------------------------
//
// Bumped lighting helper functions
//
//-----------------------------------------------------------------------------

float3 Compute_SpotLightVertexColor( const float3 worldPos, const float3 worldNormal, int lightNum )
{
	// Direct mapping of current code
	float3 lightDir = cLightInfo[lightNum].pos - worldPos;

	// normalize light direction, maintain temporaries for attenuation
	float lightDistSquared = dot( lightDir, lightDir );
	float ooLightDist = rsqrt( lightDistSquared );
	lightDir *= ooLightDist;
	
	float3 attenuationFactors;
	attenuationFactors = dst( lightDistSquared, ooLightDist );

	float flDistanceAttenuation = dot( attenuationFactors, cLightInfo[lightNum].atten );
	flDistanceAttenuation = 1.0f / flDistanceAttenuation;
	
	// There's an additional falloff we need to make to get the edges looking good
	// and confine the light within a sphere.
	float flLinearFactor = saturate( 1.0f - lightDistSquared * cLightInfo[lightNum].atten.w ); 
	flDistanceAttenuation *= flLinearFactor;

	float flCosTheta = dot( cLightInfo[lightNum].dir, -lightDir );
	float flAngularAtten = (flCosTheta - cLightInfo[lightNum].spotParams.z) * cLightInfo[lightNum].spotParams.w;
	flAngularAtten = max( 0.0f, flAngularAtten );
	flAngularAtten = pow( flAngularAtten, cLightInfo[lightNum].spotParams.x );
	flAngularAtten = min( 1.0f, flAngularAtten );

	return flDistanceAttenuation * flAngularAtten * cLightInfo[lightNum].color;
}

float3 Compute_PointLightVertexColor( const float3 worldPos, const float3 worldNormal, int lightNum )
{
	// Get light direction
	float3 lightDir = cLightInfo[lightNum].pos - worldPos;

	// Get light distance squared.
	float lightDistSquared = dot( lightDir, lightDir );

	// Get 1/lightDistance
	float ooLightDist = rsqrt( lightDistSquared );

	// Normalize light direction
	lightDir *= ooLightDist;

	// compute distance attenuation factors.
	float3 attenuationFactors;
	attenuationFactors.x = 1.0f;
	attenuationFactors.y = lightDistSquared * ooLightDist;
	attenuationFactors.z = lightDistSquared;

	float flDistanceAtten = 1.0f / dot( cLightInfo[lightNum].atten.xyz, attenuationFactors );

	// There's an additional falloff we need to make to get the edges looking good
	// and confine the light within a sphere.
	float flLinearFactor = saturate( 1.0f - lightDistSquared * cLightInfo[lightNum].atten.w ); 
	flDistanceAtten *= flLinearFactor;

	return flDistanceAtten * cLightInfo[lightNum].color;
}

float3 ComputeDirectionalLightVertexColor( const float3 worldNormal, int lightNum )
{
	return cLightInfo[lightNum].color;
}

float3 GetVertexColorForLight( const float3 worldPos, const float3 worldNormal, 
				int lightNum, int lightType )
{
	if( lightType == LIGHTTYPE_SPOT )
	{
		return Compute_SpotLightVertexColor( worldPos, worldNormal, lightNum );
	}
	else if( lightType == LIGHTTYPE_POINT )
	{
		return Compute_PointLightVertexColor( worldPos, worldNormal, lightNum );
	}
	else if( lightType == LIGHTTYPE_DIRECTIONAL )
	{
		return ComputeDirectionalLightVertexColor( worldNormal, lightNum );
	}
	return float3( 0.0f, 0.0f, 0.0f );
}
