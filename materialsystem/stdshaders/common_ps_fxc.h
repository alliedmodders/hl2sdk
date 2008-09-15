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
// HDR should never be enabled if we don't aren't running in float or integer HDR mode.
// SKIP: defined $HDRTYPE && defined $HDRENABLED && !$HDRTYPE && $HDRENABLED
// --------------------------------------------------------------------------------
// We don't ever write water fog to dest alpha if we aren't doing water fog.
// SKIP: defined $FOGTYPE && defined $WRITEWATERFOGTODESTALPHA && ( $FOGTYPE != 2 ) && $WRITEWATERFOGTODESTALPHA
// --------------------------------------------------------------------------------
// We don't need fog in the pixel shader if we aren't in float fog mode
// NOSKIP: defined $HDRTYPE && defined $HDRENABLED && defined $FOGTYPE && $HDRTYPE != HDR_TYPE_FLOAT && $FOGTYPE != 0
// --------------------------------------------------------------------------------
// We don't do HDR and LIGHTING_PREVIEW at the same time since it's runnin LDR in hammer.
//  SKIP: defined $LIGHTING_PREVIEW && defined $HDRTYPE && $LIGHTING_PREVIEW && $HDRTYPE != 0
// --------------------------------------------------------------------------------
// Ditch all fastpath attemps if we are doing LIGHTING_PREVIEW.
//	SKIP: defined $LIGHTING_PREVIEW && defined $FASTPATHENVMAPTINT && $LIGHTING_PREVIEW && $FASTPATHENVMAPTINT
//	SKIP: defined $LIGHTING_PREVIEW && defined $FASTPATHENVMAPCONTRAST && $LIGHTING_PREVIEW && $FASTPATHENVMAPCONTRAST
//	SKIP: defined $LIGHTING_PREVIEW && defined $FASTPATH && $LIGHTING_PREVIEW && $FASTPATH
// --------------------------------------------------------------------------------
// Ditch flashlight depth when flashlight is diabled
//  SKIP: ($FLASHLIGHT || $FLASHLIGHTDEPTH) && $LIGHTING_PREVIEW
// --------------------------------------------------------------------------------

// System defined pixel shader constants

// w/a unused.
const float4 g_GammaFogColor : register( c29 );
// NOTE: w == 1.0f
const float4 cGammaLightScale : register( c30 );
// NOTE: w == 1.0f
const float4 cLinearLightScale : register( c31 );
#define LIGHT_MAP_SCALE (cLinearLightScale.y)
#define ENV_MAP_SCALE (cLinearLightScale.z)

#if defined( SHADER_MODEL_PS_2_0 ) || defined( SHADER_MODEL_PS_2_B )
const float4 cFlashlightColor : register( c28 );
#endif

struct HDR_PS_OUTPUT
{
    float4 color : COLOR0;
};


HDR_PS_OUTPUT LinearColorToHDROutput( float4 linearColor, float fogFactor )
{
	// assume that sRGBWrite is enabled
	linearColor.xyz *= cLinearLightScale.x;
	HDR_PS_OUTPUT output;
	output.color = linearColor;
	return output;
}

HDR_PS_OUTPUT LinearColorToHDROutput_NoScale( float4 linearColor, float fogFactor )
{
	// assume that sRGBWrite is enabled
//	linearColor.xyz *= cLinearLightScale.xyz;
	HDR_PS_OUTPUT output;
	output.color = linearColor;
	return output;
}

HDR_PS_OUTPUT GammaColorToHDROutput( float4 gammaColor )
{
	// assume that sRGBWrite is disabled.
	HDR_PS_OUTPUT output;
	gammaColor.xyz *= cGammaLightScale.x;
	output.color.xyz = gammaColor;
	output.color.a = gammaColor.a;
	return output;
}

/*
// unused
HALF Luminance( HALF3 color )
{
	return dot( color, HALF3( HALF_CONSTANT(0.30f), HALF_CONSTANT(0.59f), HALF_CONSTANT(0.11f) ) );
}
*/

/*
// unused
HALF LuminanceScaled( HALF3 color )
{
	return dot( color, HALF3( HALF_CONSTANT(0.30f) / MAX_HDR_OVERBRIGHT, HALF_CONSTANT(0.59f) / MAX_HDR_OVERBRIGHT, HALF_CONSTANT(0.11f) / MAX_HDR_OVERBRIGHT ) );
}
*/

/*
// unused
HALF AvgColor( HALF3 color )
{
	return dot( color, HALF3( HALF_CONSTANT(0.33333f), HALF_CONSTANT(0.33333f), HALF_CONSTANT(0.33333f) ) );
}
*/

/*
// unused
HALF4 DiffuseBump( sampler lightmapSampler,
                   float2  lightmapTexCoord1,
                   float2  lightmapTexCoord2,
                   float2  lightmapTexCoord3,
                   HALF3   normal )
{
	HALF3 lightmapColor1 = tex2D( lightmapSampler, lightmapTexCoord1 );
	HALF3 lightmapColor2 = tex2D( lightmapSampler, lightmapTexCoord2 );
	HALF3 lightmapColor3 = tex2D( lightmapSampler, lightmapTexCoord3 );

	HALF3 diffuseLighting;
	diffuseLighting = saturate( dot( normal, bumpBasis[0] ) ) * lightmapColor1 +
					  saturate( dot( normal, bumpBasis[1] ) ) * lightmapColor2 +
					  saturate( dot( normal, bumpBasis[2] ) ) * lightmapColor3;

	return HALF4( diffuseLighting, LuminanceScaled( diffuseLighting ) );
}
*/


/*
// unused
HALF Fresnel( HALF3 normal,
              HALF3 eye,
              HALF2 scaleBias )
{
	HALF fresnel = HALF_CONSTANT(1.0f) - dot( normal, eye );
	fresnel = pow( fresnel, HALF_CONSTANT(5.0f) );

	return fresnel * scaleBias.x + scaleBias.y;
}
*/

/*
// unused
HALF4 GetNormal( sampler normalSampler,
                 float2 normalTexCoord )
{
	HALF4 normal = tex2D( normalSampler, normalTexCoord );
	normal.rgb = HALF_CONSTANT(2.0f) * normal.rgb - HALF_CONSTANT(1.0f);

	return normal;
}
*/

HALF3 NormalizeWithCubemap( sampler normalizeSampler, HALF3 input )
{
//	return texCUBE( normalizeSampler, input ) * 2.0f - 1.0f;
	return texCUBE( normalizeSampler, input );
}

/*
HALF4 EnvReflect( sampler envmapSampler,
				 sampler normalizeSampler,
				 HALF3 normal,
				 float3 eye,
				 HALF2 fresnelScaleBias )
{
	HALF3 normEye = NormalizeWithCubemap( normalizeSampler, eye );
	HALF fresnel = Fresnel( normal, normEye, fresnelScaleBias );
	HALF3 reflect = CalcReflectionVectorUnnormalized( normal, eye );
	return texCUBE( envmapSampler, reflect );
}
*/

float CalcWaterFogAlpha( const float flWaterZ, const float flEyePosZ, const float flWorldPosZ, const float flProjPosZ, const float flFogOORange )
{
//	float flDepthFromWater = flWaterZ - flWorldPosZ + 2.0f; // hackity hack . .this is for the DF_FUDGE_UP in view_scene.cpp
	float flDepthFromWater = flWaterZ - flWorldPosZ;

	// if flDepthFromWater < 0, then set it to 0
	// This is the equivalent of moving the vert to the water surface if it's above the water surface
	// We'll do this with the saturate at the end instead.
//	flDepthFromWater = max( 0.0f, flDepthFromWater );

	// Calculate the ratio of water fog to regular fog (ie. how much of the distance from the viewer
	// to the vert is actually underwater.
	float flDepthFromEye = flEyePosZ - flWorldPosZ;
	float f = (flDepthFromWater / flDepthFromEye) * flProjPosZ;

	// $tmp.w is now the distance that we see through water.
	return saturate( f * flFogOORange );
}

float RemapValClamped( float val, float A, float B, float C, float D)
{
	float cVal = (val - A) / (B - A);
	cVal = saturate( cVal );

	return C + (D - C) * cVal;
}


//===================================================================================//
// This is based on Natasha Tatarchuk's Parallax Occlusion Mapping (ATI)
//===================================================================================//
// INPUT:
//		inTexCoord: 
//			the texcoord for the height/displacement map before parallaxing
//
//		vParallax:
//			Compute initial parallax displacement direction:
//			float2 vParallaxDirection = normalize( vViewTS.xy );
//			float fLength = length( vViewTS );
//			float fParallaxLength = sqrt( fLength * fLength - vViewTS.z * vViewTS.z ) / vViewTS.z; 
//			Out.vParallax = vParallaxDirection * fParallaxLength * fProjectedBumpHeight;
//
//		vNormal:
//			tangent space normal
//
//		vViewW: 
//			float3 vViewW = /*normalize*/(mul( matViewInverse, float4( 0, 0, 0, 1)) - inPosition );
//
// OUTPUT:
//		the new texcoord after parallaxing
float2 CalcParallaxedTexCoord( float2 inTexCoord, float2 vParallax, float3 vNormal, 
							   float3 vViewW, sampler HeightMapSampler )
{
	const int nMinSamples = 8;
	const int nMaxSamples = 50;

   //  Normalize the incoming view vector to avoid artifacts:
//   vView = normalize( vView );
   vViewW = normalize( vViewW );
//   vLight = normalize( vLight );
   
   // Change the number of samples per ray depending on the viewing angle
   // for the surface. Oblique angles require smaller step sizes to achieve 
   // more accurate precision         
   int nNumSteps = (int) lerp( nMaxSamples, nMinSamples, dot( vViewW, vNormal ) );
      
   float4 cResultColor = float4( 0, 0, 0, 1 );
         
   //===============================================//
   // Parallax occlusion mapping offset computation //
   //===============================================//      
   float fCurrHeight = 0.0;
   float fStepSize   = 1.0 / (float) nNumSteps;
   float fPrevHeight = 1.0;
   float fNextHeight = 0.0;

   int nStepIndex = 0;
//   bool bCondition = true;
   
   float2 dx = ddx( inTexCoord );
   float2 dy = ddy( inTexCoord );
   
   float2 vTexOffsetPerStep = fStepSize * vParallax;
   
   float2 vTexCurrentOffset = inTexCoord;
   float fCurrentBound = 1.0;
   
   float x = 0;
   float y = 0;
   float xh = 0;
   float yh = 0;   
   
   float2 texOffset2 = 0;
   
   bool bCondition = true;
   while ( bCondition == true && nStepIndex < nNumSteps ) 
   {
      vTexCurrentOffset -= vTexOffsetPerStep;
      
      fCurrHeight = tex2Dgrad( HeightMapSampler, vTexCurrentOffset, dx, dy ).r;
            
      fCurrentBound -= fStepSize;
      
      if ( fCurrHeight > fCurrentBound ) 
      {                
         x  = fCurrentBound; 
         y  = fCurrentBound + fStepSize; 
         xh = fCurrHeight;
         yh = fPrevHeight;
         
         texOffset2 = vTexCurrentOffset - vTexOffsetPerStep;
                  
         bCondition = false;
      }
      else
      {
         nStepIndex++;
         fPrevHeight = fCurrHeight;
      }
     
   }   // End of while ( bCondition == true && nStepIndex > -1 )#else

   fCurrentBound -= fStepSize;
   
   float fParallaxAmount;
   float numerator = (x * (y - yh) - y * (x - xh));
   float denomenator = ((y - yh) - (x - xh));
	// avoid NaN generation
   if( ( numerator == 0.0f ) && ( denomenator == 0.0f ) )
   {
      fParallaxAmount = 0.0f;
   }
   else
   {
      fParallaxAmount = numerator / denomenator;
   }

   float2 vParallaxOffset = vParallax * (1 - fParallaxAmount );

   // Sample the height at the next possible step:
   fNextHeight = tex2Dgrad( HeightMapSampler, texOffset2, dx, dy ).r;
   
   // Original offset:
   float2 texSampleBase = inTexCoord - vParallaxOffset;

   return texSampleBase;

#if 0
   cResultColor.rgb = ComputeDiffuseColor( texSampleBase, vLight );
        
   float fBound = 1.0 - fStepSize * nStepIndex;
   if ( fNextHeight < fCurrentBound )
//    if( 0 )
   {
      //void DoIteration( in float2 vParallaxJittered, in float3 vLight, inout float4 cResultColor )
      //cResultColor.rgb = float3(1,0,0);
      DoIteration( vParallax + vPixelSize, vLight, fStepSize, inTexCoord, nStepIndex, dx, dy, fBound, cResultColor );
      DoIteration( vParallax - vPixelSize, vLight, fStepSize, inTexCoord, nStepIndex, dx, dy, fBound, cResultColor );
      DoIteration( vParallax + float2( -vPixelSize.x, vPixelSize.y ), vLight, fStepSize, inTexCoord, nStepIndex, dx, dy, fBound, cResultColor );
      DoIteration( vParallax + float2( vPixelSize.x, -vPixelSize.y ), vLight, fStepSize, inTexCoord, nStepIndex, dx, dy, fBound, cResultColor );

      cResultColor.rgb /= 5;
//      cResultColor.rgb = float3( 1.0f, 0.0f, 0.0f );
   }   // End of if ( fNextHeight < fCurrentBound )
  
#if DOSHADOWS
   {
      //============================================//
      // Soft shadow and self-occlusion computation //
      //============================================//
      // Compute the blurry shadows (note that this computation takes into 
      // account self-occlusion for shadow computation):
      float sh0 =  tex2D( sNormalMap, texSampleBase).w;
      float shA = (tex2D( sNormalMap, texSampleBase + inXY * 0.88 ).w - sh0 - 0.88 ) *  1 * fShadowSoftening;
      float sh9 = (tex2D( sNormalMap, texSampleBase + inXY * 0.77 ).w - sh0 - 0.77 ) *  2 * fShadowSoftening;
      float sh8 = (tex2D( sNormalMap, texSampleBase + inXY * 0.66 ).w - sh0 - 0.66 ) *  4 * fShadowSoftening;
      float sh7 = (tex2D( sNormalMap, texSampleBase + inXY * 0.55 ).w - sh0 - 0.55 ) *  6 * fShadowSoftening;
      float sh6 = (tex2D( sNormalMap, texSampleBase + inXY * 0.44 ).w - sh0 - 0.44 ) *  8 * fShadowSoftening;
      float sh5 = (tex2D( sNormalMap, texSampleBase + inXY * 0.33 ).w - sh0 - 0.33 ) * 10 * fShadowSoftening;
      float sh4 = (tex2D( sNormalMap, texSampleBase + inXY * 0.22 ).w - sh0 - 0.22 ) * 12 * fShadowSoftening;
      
      // Compute the actual shadow strength:
      float fShadow = 1 - max( max( max( max( max( max( shA, sh9 ), sh8 ), sh7 ), sh6 ), sh5 ), sh4 );

      cResultColor.rgb *= fShadow * 0.6 + 0.4;
   }
#endif
   
   return cResultColor;
#endif
}


//======================================//
// HSL Color space conversion routines  //
//======================================//

#define HUE          0
#define SATURATION   1
#define LIGHTNESS    2

// Convert from RGB to HSL color space
float4 RGBtoHSL( float4 inColor )
{
   float h, s;
   float flMax = max( inColor.r, max( inColor.g, inColor.b ) );
   float flMin = min( inColor.r, min( inColor.g, inColor.b ) );
   
   float l = (flMax + flMin) / 2.0f;
   
   if (flMax == flMin)   // achromatic case
   {
      s = h = 0;
   }
   else                  // chromatic case
   {
      // Next, calculate the hue
      float delta = flMax - flMin;
      
      // First, calculate the saturation
      if (l < 0.5f)      // If we're in the lower hexcone
      {
         s = delta/(flMax + flMin);
      }
      else
      {
         s = delta/(2 - flMax - flMin);
      }
      
      if ( inColor.r == flMax )
      {
         h = (inColor.g - inColor.b)/delta;      // color between yellow and magenta
      }
      else if ( inColor.g == flMax )
      {
         h = 2 + (inColor.b - inColor.r)/delta;  // color between cyan and yellow
      }
      else // blue must be max
      {
         h = 4 + (inColor.r - inColor.g)/delta;  // color between magenta and cyan
      }
      
      h *= 60.0f;
      
      if (h < 0.0f)
      {
         h += 360.0f;
      }
      
      h /= 360.0f;  
   }

   return float4 (h, s, l, 1.0f);
}

float HueToRGB( float v1, float v2, float vH )
{
   float fResult = v1;
   
   vH = fmod (vH + 1.0f, 1.0f);

   if ( ( 6.0f * vH ) < 1.0f )
   {
      fResult = ( v1 + ( v2 - v1 ) * 6.0f * vH );
   }
   else if ( ( 2.0f * vH ) < 1.0f )
   {
      fResult = ( v2 );
   }
   else if ( ( 3.0f * vH ) < 2.0f )
   {
      fResult = ( v1 + ( v2 - v1 ) * ( ( 2.0f / 3.0f ) - vH ) * 6.0f );
   }

   return fResult;
}

// Convert from HSL to RGB color space
float4 HSLtoRGB( float4 hsl )
{
   float r, g, b;
   float h = hsl[HUE];
   float s = hsl[SATURATION];
   float l = hsl[LIGHTNESS];

   if ( s == 0 )
   {
      r = g = b = l;
   }
   else
   {
      float v1, v2;
      
      if ( l < 0.5f )
         v2 = l * ( 1.0f + s );
      else
         v2 = ( l + s ) - ( s * l );

      v1 = 2 * l - v2;

      r = HueToRGB( v1, v2, h + ( 1.0f / 3.0f ) );
      g = HueToRGB( v1, v2, h );
      b = HueToRGB( v1, v2, h - ( 1.0f / 3.0f ) );
   }
  
   return float4( r, g, b, 1.0f );
}

//======================================//
// Shadowing Functions					//
//======================================//

float DoShadow( sampler DepthSampler, float4 texCoord )
{
	float2 uoffset = float2( 0.5f/512.f, 0.0f );
	float2 voffset = float2( 0.0f, 0.5f/512.f );
	float3 projTexCoord = texCoord.xyz / texCoord.w;
	float4 flashlightDepth = float4(	tex2D( DepthSampler, projTexCoord + uoffset + voffset ).x,
										tex2D( DepthSampler, projTexCoord + uoffset - voffset ).x,
										tex2D( DepthSampler, projTexCoord - uoffset + voffset ).x,
										tex2D( DepthSampler, projTexCoord - uoffset - voffset ).x	);

	const float flBias = 0.0005f;

	float shadowed = 0.0f;
	float z = 1.0f - (texCoord.w - texCoord.z);
	float4 dz = float4(z,z,z,z) - (flashlightDepth + float4( flBias, flBias, flBias, flBias ));
	float4 shadow = float4(0.25f,0.25f,0.25f,0.25f);

	if( dz.x < 0.0f )
		shadowed += shadow.x;
	if( dz.y < 0.0f )
		shadowed += shadow.y;
	if( dz.z < 0.0f )
		shadowed += shadow.z;
	if( dz.w < 0.0f )
		shadowed += shadow.w;

	return shadowed;
}

float DoShadow9Sample( sampler DepthSampler, float4 texCoord, float bias )
{
	float2 offset[9] = {	float2( -1.0f/512.0f, -1.0f/512.0f ),
							float2(  0.0f/512.0f, -1.0f/512.0f ),
							float2(  1.0f/512.0f, -1.0f/512.0f ),
							float2( -1.0f/512.0f,  0.0f ),
							float2(  0.0f/512.0f,  0.0f ),
							float2(  1.0f/512.0f,  0.0f ),
							float2( -1.0f/512.0f,  1.0f/512.0f ),
							float2(  0.0f/512.0f,  1.0f/512.0f ),
							float2(  1.0f/512.0f,  1.0f/512.0f )	};

	float3 projTexCoord = texCoord.xyz / texCoord.w;
	float z = 1.0f - (texCoord.w - texCoord.z);

	float numShadowed = 0.0f;

	float flashlightDepth[9];
	for( int i=0;i<9;i++ )
	{
		float flashlightDepth = tex2D( DepthSampler, projTexCoord + offset[i] ).x + bias;
		if( flashlightDepth < z )
		{
			numShadowed += 1.0f;
		}
	}

	return 1.0f - numShadowed / 9.0f;
}

float3 DoFlashlight( float3 flashlightPos, float3 worldPos, float3 worldNormal, 
					float4x4 worldToTexture, float3 attenuationFactors, 
					float farZ, sampler FlashlightSampler, sampler FlashlightDepthSampler, bool bDoShadow )
{
	float3 delta = flashlightPos - worldPos;
	float3 dir = normalize( delta );
	float distSquared = dot( delta, delta );
	float dist = sqrt( distSquared );

	float endFalloffFactor = RemapValClamped( dist, farZ, 0.6f * farZ, 0.0f, 1.0f );

	// fixme: need to figure out which was is more efficient for this vector by matrix multiply.
	float4 flashlightTexCoord = mul( float4( worldPos, 1.0f ), worldToTexture );
	float3 diffuseLighting = float3(1,1,1);
	if( flashlightTexCoord.w < 0.0f )
		diffuseLighting = float3(0,0,0);

#if defined( SHADER_MODEL_PS_2_0 ) || defined( SHADER_MODEL_PS_2_B )
	float3 flashlightColor = tex2D( FlashlightSampler, flashlightTexCoord.xyz / flashlightTexCoord.w ) * cFlashlightColor;
#else
	float3 flashlightColor = tex2D( FlashlightSampler, flashlightTexCoord.xyz / flashlightTexCoord.w );
#endif

	if( bDoShadow )
		flashlightColor *= DoShadow( FlashlightDepthSampler, flashlightTexCoord );

//  Note: only linear attenuation is currently used and we need all the instructions we can get
//	diffuseLighting = dot( attenuationFactors, float3( 1.0f, 1.0f/dist, 1.0f/distSquared ) );
	diffuseLighting *= attenuationFactors.y / dist;
	diffuseLighting *= dot( dir, worldNormal );
	diffuseLighting *= flashlightColor;
	diffuseLighting *= endFalloffFactor;

	return diffuseLighting;
}
