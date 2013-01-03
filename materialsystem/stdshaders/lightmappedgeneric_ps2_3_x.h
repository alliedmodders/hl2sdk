//  SKIP: $BUMPMAP2 && $WARPLIGHTING
//  SKIP: $WARPLIGHTING && ( $DETAIL_BLEND_MODE != 12 )
//	SKIP: $ENVMAPMASK && $BUMPMAP
//	SKIP: $NORMALMAPALPHAENVMAPMASK && $BASEALPHAENVMAPMASK
//	SKIP: $NORMALMAPALPHAENVMAPMASK && $ENVMAPMASK
//	SKIP: $BASEALPHAENVMAPMASK && $ENVMAPMASK
//	SKIP: $BASEALPHAENVMAPMASK && $SELFILLUM
//  SKIP: !$FASTPATH && $FASTPATHENVMAPCONTRAST
//  SKIP: !$FASTPATH && $FASTPATHENVMAPTINT
//  SKIP: !$BUMPMAP && $DIFFUSEBUMPMAP
//	SKIP: !$BUMPMAP && $BUMPMAP2
//	SKIP: $ENVMAPMASK && $BUMPMAP2
//	SKIP: $BASETEXTURENOENVMAP && ( !$BASETEXTURE2 || !$CUBEMAP )
//	SKIP: $BASETEXTURE2NOENVMAP && ( !$BASETEXTURE2 || !$CUBEMAP )
//	SKIP: $BASEALPHAENVMAPMASK && $BUMPMAP
//  SKIP: $PARALLAXMAP && ( $DETAIL_BLEND_MODE != 12 )
//  SKIP: $SEAMLESS && $PARALLAXMAP
//  SKIP: $SEAMLESS && ( $DETAIL_BLEND_MODE != 12 )
//  SKIP: $SEAMLESS && $MASKEDBLENDING
//  SKIP: $BUMPMASK && ( $SEAMLESS ||  ( $DETAILTEXTURE != 12 ) || $SELFILLUM || $BASETEXTURENOENVMAP || $BASETEXTURE2 )

// 360 compiler craps out on some combo in this family.  Content doesn't use blendmode 10 anyway
//  SKIP: $FASTPATH && $PIXELFOGTYPE && $BASETEXTURE2 && $CUBEMAP && ($DETAIL_BLEND_MODE == 10 ) [XBOX]

// Too many instructions to do this all at once:
//  SKIP: $FANCY_BLENDING && $BUMPMAP && $DETAILTEXTURE

#define USE_32BIT_LIGHTMAPS_ON_360 //uncomment to use 32bit lightmaps, be sure to keep this in sync with the same #define in materialsystem/cmatlightmaps.cpp

// NOTE: This has to be before inclusion of common_lightmappedgeneric_fxc.h to get the vertex format right!
#if ( DETAIL_BLEND_MODE == 12 )
#define DETAILTEXTURE 0
#else
#define DETAILTEXTURE 1
#endif

#include "common_ps_fxc.h"
#include "common_flashlight_fxc.h"
#define PIXELSHADER
#include "common_lightmappedgeneric_fxc.h"

#if SEAMLESS
#define USE_FAST_PATH 1
#else
#define USE_FAST_PATH FASTPATH
#endif

const float4 g_EnvmapTint : register( c0 );

#if USE_FAST_PATH == 1

#	if FASTPATHENVMAPCONTRAST == 0
static const float3 g_EnvmapContrast = { 0.0f, 0.0f, 0.0f };
#	else
static const float3 g_EnvmapContrast = { 1.0f, 1.0f, 1.0f };
#	endif
static const float3 g_EnvmapSaturation = { 1.0f, 1.0f, 1.0f };
static const float g_FresnelReflection = 1.0f;
static const float g_OneMinusFresnelReflection = 0.0f;
static const float4 g_SelfIllumTint = { 1.0f, 1.0f, 1.0f, 1.0f };
#   if OUTLINE
const float4 g_OutlineParams : register( c2 );
#define OUTLINE_MIN_VALUE0 g_OutlineParams.x
#define OUTLINE_MIN_VALUE1 g_OutlineParams.y
#define OUTLINE_MAX_VALUE0 g_OutlineParams.z
#define OUTLINE_MAX_VALUE1 g_OutlineParams.w

const float4 g_OutlineColor : register( c3 );
#define OUTLINE_COLOR g_OutlineColor

#   endif
#   if SOFTEDGES
const float4 g_EdgeSoftnessParms : register( c4 );
#define SOFT_MASK_MIN g_EdgeSoftnessParms.x
#define SOFT_MASK_MAX g_EdgeSoftnessParms.y
#   endif
#else

const float3 g_EnvmapContrast				: register( c2 );
const float3 g_EnvmapSaturation				: register( c3 );
const float4 g_FresnelReflectionReg			: register( c4 );
#define g_FresnelReflection g_FresnelReflectionReg.a
#define g_OneMinusFresnelReflection g_FresnelReflectionReg.b
const float4 g_SelfIllumTint					: register( c7 );
#endif

const float4 g_DetailTint_and_BlendFactor	: register( c8 );
#define g_DetailTint (g_DetailTint_and_BlendFactor.rgb)
#define g_DetailBlendFactor (g_DetailTint_and_BlendFactor.w)

const float3 g_EyePos						: register( c10 );
const float4 g_FogParams						: register( c11 );
const float4 g_TintValuesTimesLightmapScale	: register( c12 );

#define g_flAlpha2 g_TintValuesTimesLightmapScale.w

const float4 g_FlashlightAttenuationFactors	: register( c13 );
const float3 g_FlashlightPos				: register( c14 );
const float4x4 g_FlashlightWorldToTexture	: register( c15 ); // through c18
const float4 g_ShadowTweaks					: register( c19 );

#if !defined( SHADER_MODEL_PS_2_0 ) && ( FLASHLIGHT == 0 )
#define g_cAmbientColor cFlashlightScreenScale.rgb
//const float3 g_cAmbientColor				: register( c31 );
#endif

#if PARALLAX_MAPPING || (CUBEMAP == 2)
const float4 g_ParallaxMappingControl : register( c20 );
#endif

#if PARALLAX_MAPPING
#define HEIGHT_SCALE ( g_ParallaxMappingControl.x )
#else
#define HEIGHT_SCALE 0
#endif

#if (CUBEMAP == 2)
#define g_DiffuseCubemapScale g_ParallaxMappingControl.y
#endif

const float3 g_TintValuesWithoutLightmapScale	: register( c21 );

sampler BaseTextureSampler		: register( s0 );
sampler LightmapSampler			: register( s1 );
sampler EnvmapSampler			: register( s2 );
#if FANCY_BLENDING
sampler BlendModulationSampler	: register( s3 );
#endif

#if DETAILTEXTURE
sampler DetailSampler			: register( s12 );
#endif

sampler BumpmapSampler			: register( s4 );

#if BUMPMAP2 == 1
sampler BumpmapSampler2			: register( s5 );
#else
sampler EnvmapMaskSampler		: register( s5 );
#endif

#if ( DETAIL_BLEND_MODE == 9 )
sampler PaintSampler			: register( s9 );
#endif

#if WARPLIGHTING
sampler WarpLightingSampler		: register( s6 );
#endif
sampler BaseTextureSampler2		: register( s7 );

#if BUMPMASK == 1
sampler BumpMaskSampler			: register( s8 );
#if NORMALMASK_DECODE_MODE == NORM_DECODE_ATI2N_ALPHA
sampler AlphaMaskSampler		: register( s11 );	// alpha
#else
#define AlphaMaskSampler		BumpMaskSampler
#endif
#endif

#if defined( _X360 ) && FLASHLIGHT
sampler FlashlightSampler		: register( s13 );
sampler ShadowDepthSampler		: register( s14 );
sampler RandRotSampler			: register( s15 );
#endif

#if defined( _X360 )
	// The compiler runs out of temp registers in certain combos, increase the maximum for now
	#if ( BASETEXTURE2 && (BUMPMAP == 2) && CUBEMAP && NORMALMAPALPHAENVMAPMASK && DIFFUSEBUMPMAP && FLASHLIGHT && SHADER_SRGB_READ )
		[maxtempreg(39)]
	#elif ( SHADER_SRGB_READ == 1 )
		[maxtempreg(36)]
	#endif
#endif

#if LIGHTING_PREVIEW == 2
LPREVIEW_PS_OUT main( PS_INPUT i ) : COLOR
#else
float4 main( PS_INPUT i ) : COLOR
#endif
{
	bool bBaseTexture2 = BASETEXTURE2 ? true : false;
	bool bDetailTexture = DETAILTEXTURE ? true : false;
	bool bBumpmap = BUMPMAP ? true : false;
	bool bDiffuseBumpmap = DIFFUSEBUMPMAP ? true : false;
	bool bCubemap = CUBEMAP ? true : false;
	bool bEnvmapMask = ENVMAPMASK ? true : false;
	bool bBaseAlphaEnvmapMask = BASEALPHAENVMAPMASK ? true : false;
	bool bSelfIllum = SELFILLUM ? true : false;
	bool bNormalMapAlphaEnvmapMask = NORMALMAPALPHAENVMAPMASK ? true : false;
	bool bBaseTextureNoEnvmap = BASETEXTURENOENVMAP ? true : false;
	bool bBaseTexture2NoEnvmap = BASETEXTURE2NOENVMAP ? true : false;

	float4 baseColor = 0.0f;
	float4 baseColor2 = 0.0f;
	float4 vNormal = float4(0, 0, 1, 1);
	float3 baseTexCoords = float3(0,0,0);
	float3 worldPos = i.worldPos_projPosZ.xyz;
	float3x3 tangenttranspose = i.tangentSpaceTranspose;

	float3 worldVertToEyeVector = g_EyePos - worldPos;
#if SEAMLESS
	baseTexCoords = i.SeamlessTexCoord_fogFactorW.xyz;
#else
	baseTexCoords.xy = i.BASETEXCOORD.xy;
#endif

	float3 coords = baseTexCoords;

	if ( PARALLAX_MAPPING )
	{		
		float3 tangentspace_eye_vector = mul( worldVertToEyeVector, transpose( tangenttranspose ) );
		
		float flParallaxLimit = length( tangentspace_eye_vector.xy ) / tangentspace_eye_vector.z;
		float2 vOffset = normalize( -tangentspace_eye_vector.xy );
		float CurHeight = 1.0;
		float NewHeight = tex2D( BumpmapSampler, coords.xy ).a;
		float dist = min( 0.06, HEIGHT_SCALE * flParallaxLimit * ( CurHeight - NewHeight ) );
		coords.x += dist * vOffset.x;
		coords.y += dist * vOffset.y;
	}

	float2 detailTexCoord = 0.0f;
	float2 bumpmapTexCoord = 0.0f;
	float2 bumpmap2TexCoord = 0.0f;

#if ( DETAILTEXTURE == 1 )
	detailTexCoord = i.DETAILCOORDS;
#endif

#if BUMPMAP
	bumpmapTexCoord = i.BUMPCOORDS;
#endif

#if BUMPMASK
	bumpmap2TexCoord = i.ENVMAPMASKCOORDS;
#endif

	if ( PARALLAX_MAPPING )
	{
		detailTexCoord = 0;
		bumpmapTexCoord = coords;
	}

	GetBaseTextureAndNormal( BaseTextureSampler, BaseTextureSampler2, BumpmapSampler,
							 bBaseTexture2, bBumpmap || bNormalMapAlphaEnvmapMask, 
							 coords, bumpmapTexCoord, 
							 i.vertexColor.rgb, baseColor, baseColor2, vNormal );

#if BUMPMAP == 1	// not ssbump
	vNormal.xyz = vNormal.xyz * 2.0f - 1.0f;					// make signed if we're not ssbump

	float3 vThisReallyIsANormal = vNormal;
#endif

	float3 lightmapColor1 = float3( 1.0f, 1.0f, 1.0f );
	float3 lightmapColor2 = float3( 1.0f, 1.0f, 1.0f );
	float3 lightmapColor3 = float3( 1.0f, 1.0f, 1.0f );
#if LIGHTING_PREVIEW == 0
	if( bBumpmap && bDiffuseBumpmap )
	{
		float2 bumpCoord1;
		float2 bumpCoord2;
		float2 bumpCoord3;
		ComputeBumpedLightmapCoordinates( i.lightmapTexCoord1And2, i.lightmapTexCoord3.xy,
			bumpCoord1, bumpCoord2, bumpCoord3 );
		
		lightmapColor1 = LightMapSample( LightmapSampler, bumpCoord1 );
		lightmapColor2 = LightMapSample( LightmapSampler, bumpCoord2 );
		lightmapColor3 = LightMapSample( LightmapSampler, bumpCoord3 );
	}
	else
	{
		float2 bumpCoord1 = ComputeLightmapCoordinates( i.lightmapTexCoord1And2, i.lightmapTexCoord3.xy );
		lightmapColor1 = LightMapSample( LightmapSampler, bumpCoord1 );
	}
#endif

#if ( DETAIL_BLEND_MODE == 9 ) && ( defined(SHADER_MODEL_PS_2_B ) || defined( SHADER_MODEL_PS_3_0 ) )
	float2 paintCoord;
	if( bBumpmap && bDiffuseBumpmap )
	{
		float2 bumpCoord1;
		float2 bumpCoord2;
		float2 bumpCoord3;
		ComputeBumpedLightmapCoordinates( i.lightmapTexCoord1And2, i.lightmapTexCoord3.xy,
			bumpCoord1, bumpCoord2, bumpCoord3 );
		paintCoord.y = bumpCoord1.y;
		paintCoord.x = bumpCoord1.x - ( bumpCoord2.x - bumpCoord1.x );
	}
	else
	{
		paintCoord = ComputeLightmapCoordinates( i.lightmapTexCoord1And2, i.lightmapTexCoord3.xy );
	}
	float4 paintColor = tex2D( PaintSampler, paintCoord );
	baseColor.xyz = lerp( baseColor.xyz, paintColor.xyz, paintColor.w );
#endif

	float2 envmapMaskTexCoord = i.ENVMAPMASKCOORDS;

	float4 detailColor = float4( 1.0f, 1.0f, 1.0f, 1.0f );

#if DETAILTEXTURE
#if SHADER_MODEL_PS_2_0
	detailColor = tex2D( DetailSampler, detailTexCoord );
#else
	detailColor = float4( g_DetailTint, 1.0f ) * tex2D( DetailSampler, detailTexCoord );
#endif
#endif

#if ( OUTLINE || SOFTEDGES )
	float distAlphaMask = baseColor.a;

#   if OUTLINE
	if ( ( distAlphaMask >= OUTLINE_MIN_VALUE0 ) &&
		 ( distAlphaMask <= OUTLINE_MAX_VALUE1 ) )
	{
		float oFactor=1.0;
		if ( distAlphaMask <= OUTLINE_MIN_VALUE1 )
		{
			oFactor=smoothstep( OUTLINE_MIN_VALUE0, OUTLINE_MIN_VALUE1, distAlphaMask );
		}
		else
		{
			oFactor=smoothstep( OUTLINE_MAX_VALUE1, OUTLINE_MAX_VALUE0, distAlphaMask );
		}
		baseColor = lerp( baseColor, OUTLINE_COLOR, oFactor );
	}
#   endif
#   if SOFTEDGES
	baseColor.a *= smoothstep( SOFT_MASK_MAX, SOFT_MASK_MIN, distAlphaMask );
#   else
	baseColor.a *= distAlphaMask >= 0.5;
#   endif
#endif


	float blendedAlpha = baseColor.a;

#if MASKEDBLENDING
	float blendfactor=0.5;
#else
	float blendfactor=i.vertexBlendX.x;
#endif

	if( bBaseTexture2 )
	{
#if (SELFILLUM == 0) && (NORMALMAPALPHAENVMAPMASK==0) && (PIXELFOGTYPE != PIXEL_FOG_TYPE_HEIGHT) && (FANCY_BLENDING)
		float4 modt=tex2D(BlendModulationSampler,i.lightmapTexCoord3.zw);
#if MASKEDBLENDING
		float minb=modt.g-modt.r;
		float maxb=modt.g+modt.r;
#else
		float minb=max(0,modt.g-modt.r);
		float maxb=min(1,modt.g+modt.r);
#endif
		blendfactor=smoothstep(minb,maxb,blendfactor);
#endif
		baseColor.rgb = lerp( baseColor, baseColor2.rgb, blendfactor );
		blendedAlpha = lerp( baseColor.a, baseColor2.a, blendfactor );
	}

	float3 specularFactor = 1.0f;
	float4 vNormalMask = float4(0, 0, 1, 1);
	if( bBumpmap )
	{
		if( bBaseTextureNoEnvmap )
		{
			vNormal.a = 0.0f;
		}

#if ( BUMPMAP2 == 1 )
		{
	#if ( BUMPMASK == 1 )
			float2 b2TexCoord = bumpmap2TexCoord;
	#else
			float2 b2TexCoord = bumpmapTexCoord;
	#endif

			float4 vNormal2;
			#if ( BUMPMAP == 2 )
			{
				vNormal2 = tex2D( BumpmapSampler2, b2TexCoord );
			}
			#else
			{
				float4 normalTexel = tex2D( BumpmapSampler2, b2TexCoord );
				vNormal2 = float4( normalTexel.xyz * 2.0f - 1.0f, normalTexel.a );
			}
			#endif

			if( bBaseTexture2NoEnvmap )
			{
				vNormal2.a = 0.0f;
			}

	#if ( BUMPMASK == 1 )
			float3 vNormal1 = DecompressNormal( BumpmapSampler, i.BUMPCOORDS, NORMALMASK_DECODE_MODE, AlphaMapSampler );

			vNormal.xyz = normalize( vNormal1.xyz + vNormal2.xyz );

			// Third normal map...same coords as base
			normalTexel = tex2D( BumpMaskSampler, i.baseTexCoord.xy );
			vNormalMask = float4( normalTexel.xyz * 2.0f - 1.0f, normalTexel.a );

			vNormal.xyz = lerp( vNormalMask.xyz, vNormal.xyz, vNormalMask.a );		// Mask out normals from vNormal
			specularFactor = vNormalMask.a;
	#else // BUMPMASK == 0
			vNormal.xyz = lerp( vNormal.xyz, vNormal2.xyz, blendfactor);
	#endif

		}

#endif // BUMPMAP2 == 1

		if( bNormalMapAlphaEnvmapMask )
		{
			specularFactor *= vNormal.a;
		}
	}
	else if ( bNormalMapAlphaEnvmapMask )
	{
		specularFactor *= vNormal.a;
	}

#if ( BUMPMAP2 == 0 )
	if( bEnvmapMask )
	{
		specularFactor *= tex2D( EnvmapMaskSampler, envmapMaskTexCoord ).xyz;	
	}
#endif

	if( bBaseAlphaEnvmapMask )
	{
		specularFactor *= 1.0 - blendedAlpha; // Reversing alpha blows!
	}

	float4 albedo = float4( 1.0f, 1.0f, 1.0f, 1.0f );
	float alpha = 1.0f;
	albedo *= baseColor;
	if( !bBaseAlphaEnvmapMask && !bSelfIllum )
	{
		alpha *= baseColor.a;
	}

	#if ( DETAIL_BLEND_MODE != 9 )
	{
		if( bDetailTexture )
		{
			albedo = TextureCombine( albedo, detailColor, DETAIL_BLEND_MODE, g_DetailBlendFactor );
			#if ( ( DETAIL_BLEND_MODE == TCOMBINE_MOD2X_SELECT_TWO_PATTERNS ) && !MASKEDBLENDING && !BASETEXTURE2 && !SELFILLUM )
			{
				// don't do this in the MASKEDBLENDING or SELFILLUM case since we don't have enough instructions in ps20
				specularFactor *= 2.0 * lerp( detailColor.g, detailColor.b, baseColor.a );
			}
			#endif
		}
	}
	#endif

	// The vertex color contains the modulation color + vertex color combined
#if ( SEAMLESS == 0 )
	albedo.xyz *= i.vertexColor;
#endif
	// MAINTOL4DMERGEFIXME
//	alpha *= i.vertexColor.a * g_flAlpha2; // not sure about this one
	alpha *= i.vertexColor.a; // not sure about this one

	// Save this off for single-pass flashlight, since we'll still need the SSBump vector, not a real normal
	float3 vSSBumpVector = vNormal.xyz;

	float3 diffuseLighting;
	if( bBumpmap && bDiffuseBumpmap )
	{

// ssbump
#if ( BUMPMAP == 2 )
#if ( DETAIL_BLEND_MODE == TCOMBINE_SSBUMP_BUMP )
		vNormal *= 2*detailColor;
#endif
		diffuseLighting = vNormal.x * lightmapColor1 +
						  vNormal.y * lightmapColor2 +
						  vNormal.z * lightmapColor3;
		diffuseLighting *= g_TintValuesTimesLightmapScale.rgb;
		// now, calculate vNormal for reflection purposes. if vNormal isn't needed, hopefully
		// the compiler will eliminate these calculations
		vNormal.xyz = normalize( bumpBasis[0]*vNormal.x + bumpBasis[1]*vNormal.y + bumpBasis[2]*vNormal.z);
#else
		float3 dp;
		dp.x = saturate( dot( vNormal, bumpBasis[0] ) );
		dp.y = saturate( dot( vNormal, bumpBasis[1] ) );
		dp.z = saturate( dot( vNormal, bumpBasis[2] ) );
		dp *= dp;
		
#if ( DETAIL_BLEND_MODE == TCOMBINE_SSBUMP_BUMP )
		dp *= 2*detailColor;
#endif
		diffuseLighting = dp.x * lightmapColor1 +
						  dp.y * lightmapColor2 +
						  dp.z * lightmapColor3;
		float sum = dot( dp, float3( 1.0f, 1.0f, 1.0f ) );
		diffuseLighting *= g_TintValuesTimesLightmapScale.rgb / sum;
#endif
	}
	else
	{
		diffuseLighting = lightmapColor1 * g_TintValuesTimesLightmapScale.rgb;
	}

#if WARPLIGHTING && ( SEAMLESS == 0 )
	float len=0.5*length(diffuseLighting);
	// FIXME: 8-bit lookup textures like this need a "nice filtering" VTF option, which converts
	//        them to 16-bit on load or does filtering in the shader (since most hardware - 360
	//        included - interpolates 8-bit textures at 8-bit precision, which causes banding)
	diffuseLighting *= 2.0*tex2D(WarpLightingSampler,float2(len,0));
#endif

	float3 worldSpaceNormal = mul( vNormal, tangenttranspose );
#if !defined( SHADER_MODEL_PS_2_0 ) && ( FLASHLIGHT == 0 )
	diffuseLighting += g_cAmbientColor;
#endif

	float3 diffuseComponent = albedo * diffuseLighting;

#if defined( _X360 ) && FLASHLIGHT

	// ssbump doesn't pass a normal to the flashlight...it computes shadowing a different way
#if ( BUMPMAP == 2 )
	bool bHasNormal = false;

	float3 worldPosToLightVector = g_FlashlightPos - worldPos;

	float3 tangentPosToLightVector;
	tangentPosToLightVector.x = dot( worldPosToLightVector, tangenttranspose[0] );
	tangentPosToLightVector.y = dot( worldPosToLightVector, tangenttranspose[1] );
	tangentPosToLightVector.z = dot( worldPosToLightVector, tangenttranspose[2] );

	tangentPosToLightVector = normalize( tangentPosToLightVector );
	float nDotL = saturate( vSSBumpVector.x*dot( tangentPosToLightVector, bumpBasis[0]) +
							vSSBumpVector.y*dot( tangentPosToLightVector, bumpBasis[1]) +
							vSSBumpVector.z*dot( tangentPosToLightVector, bumpBasis[2]) );
#else
	bool bHasNormal = true;
	float nDotL = 1.0f;
#endif

	bool bShadows = FLASHLIGHTSHADOWS ? true : false;
	float3 flashlightColor = DoFlashlight( g_FlashlightPos, worldPos, i.flashlightSpacePos,
		worldSpaceNormal, g_FlashlightAttenuationFactors.xyz, 
		g_FlashlightAttenuationFactors.w, FlashlightSampler, ShadowDepthSampler,
		RandRotSampler, 0, bShadows, false, i.vProjPos.xy / i.vProjPos.w, false, g_ShadowTweaks, bHasNormal );

	diffuseComponent = albedo.xyz * ( diffuseLighting + ( flashlightColor * nDotL * g_TintValuesWithoutLightmapScale.rgb ) );
#endif

	if( bSelfIllum )
	{
		float3 selfIllumComponent = g_SelfIllumTint * albedo.xyz;
		diffuseComponent = lerp( diffuseComponent, selfIllumComponent, baseColor.a );
	}

	float3 specularLighting = float3( 0.0f, 0.0f, 0.0f );
#if CUBEMAP
	if( bCubemap )
	{
		float3 reflectVect = CalcReflectionVectorUnnormalized( worldSpaceNormal, worldVertToEyeVector );

		// Calc Fresnel factor
		half3 eyeVect = normalize(worldVertToEyeVector);
		float fresnel = 1.0 - dot( worldSpaceNormal, eyeVect );
		fresnel = pow( fresnel, 5.0 );
		fresnel = fresnel * g_OneMinusFresnelReflection + g_FresnelReflection;
		
		specularLighting = ENV_MAP_SCALE * texCUBE( EnvmapSampler, reflectVect );
#if (CUBEMAP == 2) //cubemap darkened by lightmap mode
		specularLighting = lerp( specularLighting, specularLighting * saturate( diffuseLighting ), g_DiffuseCubemapScale ); //reduce the cubemap contribution when the pixel is in shadow
#endif
		specularLighting *= specularFactor;
								   
		specularLighting *= g_EnvmapTint;
#if FANCY_BLENDING == 0
		float3 specularLightingSquared = specularLighting * specularLighting;
		specularLighting = lerp( specularLighting, specularLightingSquared, g_EnvmapContrast );
		float3 greyScale = dot( specularLighting, float3( 0.299f, 0.587f, 0.114f ) );
		specularLighting = lerp( greyScale, specularLighting, g_EnvmapSaturation );
#endif
		specularLighting *= fresnel;
	}
#endif

	#if ( DETAIL_BLEND_MODE != 9 )
	{
		if( bDetailTexture )
		{
			diffuseComponent = TextureCombinePostLighting( diffuseComponent, detailColor, DETAIL_BLEND_MODE, g_DetailBlendFactor );
		}
	}
	#endif

	float3 result = diffuseComponent + specularLighting;
	
#if ( LIGHTING_PREVIEW == 3 )
	return float4( worldSpaceNormal, i.worldPos_projPosZ.w );
#endif

#if ( LIGHTING_PREVIEW == 1 )
	float dotprod = 0.7+0.25 * dot( worldSpaceNormal, normalize( float3( 1, 2, -.5 ) ) );
	return FinalOutput( float4( dotprod*albedo.xyz, alpha ), 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
#endif

#if ( LIGHTING_PREVIEW == 2 )
	LPREVIEW_PS_OUT ret;
	ret.color = float4( albedo.xyz,alpha );
	ret.normal = float4( worldSpaceNormal, i.worldPos_projPosZ.w );
	ret.position = float4( worldPos, alpha );
	ret.flags = float4( 1, 1, 1, alpha );
	
	return FinalOutput( ret, 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );	
#endif

#if ( LIGHTING_PREVIEW == 0 )
	bool bWriteDepthToAlpha = false;

	// ps_2_b and beyond
#if !(defined(SHADER_MODEL_PS_1_1) || defined(SHADER_MODEL_PS_1_4) || defined(SHADER_MODEL_PS_2_0))
	bWriteDepthToAlpha = ( WRITE_DEPTH_TO_DESTALPHA != 0 ) && ( WRITEWATERFOGTODESTALPHA == 0 );
#endif

	float flVertexFogFactor = 0.0f;
	#if !HARDWAREFOGBLEND && !DOPIXELFOG
	{
		#if ( SEAMLESS )
		{
			flVertexFogFactor = i.SeamlessTexCoord_fogFactorW.w;
		}
		#else
		{
			flVertexFogFactor = i.baseTexCoord_fogFactorZ.z;
		}
		#endif
	}
	#endif
	float fogFactor = CalcPixelFogFactorSupportsVertexFog( PIXELFOGTYPE, g_FogParams, g_EyePos.xyz, worldPos, i.worldPos_projPosZ.w, flVertexFogFactor );

#if WRITEWATERFOGTODESTALPHA && (PIXELFOGTYPE == PIXEL_FOG_TYPE_HEIGHT)
	alpha = fogFactor;
#endif
	return FinalOutput( float4( result.rgb, alpha ), fogFactor, PIXELFOGTYPE, TONEMAP_SCALE_LINEAR, bWriteDepthToAlpha, i.worldPos_projPosZ.w );
#endif

}
 
