#if defined( _X360 )

void GetBaseTextureAndNormal( sampler base, sampler base2, sampler bump, bool bBase2, bool bBump, 
							  float3 coords, float2 bumpcoords,
							  float3 vWeights,
							  out float4 vResultBase, out float4 vResultBase2, out float4 vResultBump )
{
	vResultBase = 0;
	vResultBase2 = 0;
	vResultBump = 0;

	if ( !bBump )
	{
		vResultBump = float4(0, 0, 1, 1);
	}

#if SEAMLESS

	vWeights = max( vWeights - 0.3, 0 );
	vWeights *= 1.0f / dot( vWeights, float3(1,1,1) );

	[branch]
	if (vWeights.x > 0)
	{
		vResultBase  += vWeights.x * tex2D( base,  coords.zy );

		if ( bBase2 )
		{
			vResultBase2 += vWeights.x * tex2D( base2, coords.zy );
		}

		if ( bBump )
		{
			vResultBump  += vWeights.x * tex2D( bump,  coords.zy );
		}
	}

	[branch]
	if (vWeights.y > 0)
	{
		vResultBase  += vWeights.y * tex2D( base,  coords.xz );

		if ( bBase2 )
		{
			vResultBase2 += vWeights.y * tex2D( base2, coords.xz );
		}
		if ( bBump )
		{
			vResultBump  += vWeights.y * tex2D( bump,  coords.xz );
		}
	}

	[branch]
	if (vWeights.z > 0)
	{
		vResultBase  += vWeights.z * tex2D( base,  coords.xy );
		if ( bBase2 )
		{
			vResultBase2 += vWeights.z * tex2D( base2, coords.xy );
		}

		if ( bBump )
		{
			vResultBump  += vWeights.z * tex2D( bump,  coords.xy );
		}
	}

	#if ( SHADER_SRGB_READ == 1 )
		// Do this after the blending to save shader ops
		vResultBase.rgb = X360GammaToLinear( vResultBase.rgb );
		vResultBase2.rgb = X360GammaToLinear( vResultBase2.rgb );
	#endif

#else  // not seamless

	vResultBase = tex2Dsrgb( base, coords.xy );

	if ( bBase2 )
	{
		vResultBase2 = tex2Dsrgb( base2, coords.xy );
	}

	if ( bBump )
	{
		vResultBump  = tex2D( bump, bumpcoords.xy );
	}

#endif
}

#else // PC

void GetBaseTextureAndNormal( sampler base, sampler base2, sampler bump, bool bBase2, bool bBump,
							  float3 coords, float2 bumpcoords, float3 vWeights,
							 out float4 vResultBase, out float4 vResultBase2, out float4 vResultBump )
{
	vResultBase = 0;
	vResultBase2 = 0;
	vResultBump = 0;

	if ( !bBump )
	{
		vResultBump = float4(0, 0, 1, 1);
	}

#if SEAMLESS

	vResultBase  += vWeights.x * tex2D( base, coords.zy );
	if ( bBase2 )
	{
		vResultBase2 += vWeights.x * tex2D( base2, coords.zy );
	}
	if ( bBump )
	{
		vResultBump  += vWeights.x * tex2D( bump, coords.zy );
	}

	vResultBase  += vWeights.y * tex2D( base, coords.xz );
	if ( bBase2 )
	{
		vResultBase2 += vWeights.y * tex2D( base2, coords.xz );
	}
	if ( bBump )
	{
		vResultBump  += vWeights.y * tex2D( bump, coords.xz );
	}

	vResultBase  += vWeights.z * tex2D( base, coords.xy );
	if ( bBase2 )
	{
		vResultBase2 += vWeights.z * tex2D( base2, coords.xy );
	}
	if ( bBump )
	{
		vResultBump  += vWeights.z * tex2D( bump, coords.xy );
	}

#else  // not seamless

	vResultBase  = tex2D( base, coords.xy );
	if ( bBase2 )
	{
		vResultBase2 = tex2D( base2, coords.xy );
	}
	if ( bBump )
	{
		vResultBump  = tex2D( bump, bumpcoords.xy );
	}
#endif

}

#endif


float3 LightMapSample( sampler LightmapSampler, float2 vTexCoord )
{
	#if ( !defined( _X360 ) || !defined( USE_32BIT_LIGHTMAPS_ON_360 ) )
	{
		float3 sample = tex2D( LightmapSampler, vTexCoord );

		return sample;
	}
	#else
	{
		#if 0 //1 for cheap sampling, 0 for accurate scaling from the individual samples
		{
			float4 sample = tex2D( LightmapSampler, vTexCoord );

			return sample.rgb * sample.a;
		}
		#else
		{
			float4 Weights;
			float4 samples_0; //no arrays allowed in inline assembly
			float4 samples_1;
			float4 samples_2;
			float4 samples_3;
			
			asm {
				tfetch2D samples_0, vTexCoord.xy, LightmapSampler, OffsetX = -0.5, OffsetY = -0.5, MinFilter=point, MagFilter=point, MipFilter=keep, UseComputedLOD=false
				tfetch2D samples_1, vTexCoord.xy, LightmapSampler, OffsetX =  0.5, OffsetY = -0.5, MinFilter=point, MagFilter=point, MipFilter=keep, UseComputedLOD=false
				tfetch2D samples_2, vTexCoord.xy, LightmapSampler, OffsetX = -0.5, OffsetY =  0.5, MinFilter=point, MagFilter=point, MipFilter=keep, UseComputedLOD=false
				tfetch2D samples_3, vTexCoord.xy, LightmapSampler, OffsetX =  0.5, OffsetY =  0.5, MinFilter=point, MagFilter=point, MipFilter=keep, UseComputedLOD=false

				getWeights2D Weights, vTexCoord.xy, LightmapSampler
			};

			Weights = float4( (1-Weights.x)*(1-Weights.y), Weights.x*(1-Weights.y), (1-Weights.x)*Weights.y, Weights.x*Weights.y );

			float3 result;
			result.rgb  = samples_0.rgb * (samples_0.a * Weights.x);
			result.rgb += samples_1.rgb * (samples_1.a * Weights.y);
			result.rgb += samples_2.rgb * (samples_2.a * Weights.z);
			result.rgb += samples_3.rgb * (samples_3.a * Weights.w);
		
			return result;
		}
		#endif
	}
	#endif
}

#ifdef PIXELSHADER
#define VS_OUTPUT PS_INPUT
#endif

struct VS_OUTPUT
{
#ifndef PIXELSHADER
	float4 projPos					: POSITION;	
#	if !defined( _X360 ) && !defined( SHADER_MODEL_VS_3_0 )
	float  fog						: FOG;
#	endif
#endif
#if SEAMLESS
	#if HARDWAREFOGBLEND || DOPIXELFOG
		float3 SeamlessTexCoord_fogFactorW         : TEXCOORD0;            // zy xz
	#else
		float4 SeamlessTexCoord_fogFactorW         : TEXCOORD0;            // zy xz
	#endif
#else
	#if HARDWAREFOGBLEND || DOPIXELFOG
		float2 baseTexCoord_fogFactorZ				: TEXCOORD0;
	#else
		float3 baseTexCoord_fogFactorZ				: TEXCOORD0;
	#endif

	// detail textures and bumpmaps are mutually exclusive so that we have enough texcoords.
#endif
	float4 detailOrBumpAndEnvmapMaskTexCoord : TEXCOORD1;   // envmap mask

	// CENTROID: TEXCOORD2
#if defined( _X360 )
	float4 lightmapTexCoord1And2		: TEXCOORD2_centroid;
#else
	float4 lightmapTexCoord1And2		: TEXCOORD2;
#endif

	// CENTROID: TEXCOORD3
#if defined( _X360 )
	float4 lightmapTexCoord3			: TEXCOORD3_centroid;
#else
	float4 lightmapTexCoord3			: TEXCOORD3;
#endif

	float4 worldPos_projPosZ		: TEXCOORD4;
	float3x3 tangentSpaceTranspose	: TEXCOORD5;
	// tangentSpaceTranspose		: TEXCOORD6
	// tangentSpaceTranspose		: TEXCOORD7
	float4 vertexColor				: COLOR0;
	float  vertexBlendX				: COLOR1;

	// Extra iterators on 360, used in flashlight combo
#if defined( _X360 ) && FLASHLIGHT
	float4 flashlightSpacePos		: TEXCOORD8;
	float4 vProjPos					: TEXCOORD9;
#endif
};

#define DETAILCOORDS detailOrBumpAndEnvmapMaskTexCoord.xy
#define DETAILORBUMPCOORDS DETAILCOORDS
#define ENVMAPMASKCOORDS detailOrBumpAndEnvmapMaskTexCoord.wz

#if DETAILTEXTURE && BUMPMAP && !SELFILLUM && !FANCY_BLENDING
	#define BUMPCOORDS lightmapTexCoord3.wz
#elif DETAILTEXTURE
	#define BUMPCOORDS baseTexCoord_fogFactorZ.xy
#else
	// This is the SEAMLESS case too since we skip SEAMLESS && DETAILTEXTURE
	#define BUMPCOORDS detailOrBumpAndEnvmapMaskTexCoord.xy
#endif

#if FANCY_BLENDING
	#define FANCYBLENDMASKCOORDS lightmapTexCoord3.wz
#endif

#if SEAMLESS
	// don't use BASETEXCOORD in the SEAMLESS case
#else
	#define BASETEXCOORD baseTexCoord_fogFactorZ.xy
#endif
