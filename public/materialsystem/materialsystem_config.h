//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef MATERIALSYSTEM_CONFIG_H
#define MATERIALSYSTEM_CONFIG_H
#ifdef _WIN32
#pragma once
#endif

#include "materialsystem/imaterialsystem.h"

#define MATERIALSYSTEM_CONFIG_VERSION "VMaterialSystemConfig002"

enum MaterialSystem_Config_Flags_t
{
	MATSYS_VIDCFG_FLAGS_WINDOWED					= ( 1 << 0 ),
	MATSYS_VIDCFG_FLAGS_RESIZING					= ( 1 << 1 ),
	MATSYS_VIDCFG_FLAGS_NO_WAIT_FOR_VSYNC			= ( 1 << 3 ),
	MATSYS_VIDCFG_FLAGS_STENCIL						= ( 1 << 4 ),
	MATSYS_VIDCFG_FLAGS_FORCE_TRILINEAR				= ( 1 << 5 ),
	MATSYS_VIDCFG_FLAGS_FORCE_HWSYNC				= ( 1 << 6 ),
	MATSYS_VIDCFG_FLAGS_DISABLE_SPECULAR			= ( 1 << 7 ),
	MATSYS_VIDCFG_FLAGS_DISABLE_BUMPMAP				= ( 1 << 8 ),
	MATSYS_VIDCFG_FLAGS_ENABLE_PARALLAX_MAPPING		= ( 1 << 9 ),
	MATSYS_VIDCFG_FLAGS_USE_Z_PREFILL				= ( 1 << 10 ),
	MATSYS_VIDCFG_FLAGS_REDUCE_FILLRATE				= ( 1 << 11 ),
	MATSYS_VIDCFG_FLAGS_ENABLE_HDR					= ( 1 << 12 ),
};

struct MaterialSystemHardwareIdentifier_t
{
	char *m_pCardName;
	unsigned int m_nVendorID;
	unsigned int m_nDeviceID;
};

struct MaterialSystem_Config_t
{
	bool Windowed() const { return ( m_Flags & MATSYS_VIDCFG_FLAGS_WINDOWED ) != 0; }
	bool Resizing() const { return ( m_Flags & MATSYS_VIDCFG_FLAGS_RESIZING ) != 0; }
	bool WaitForVSync() const { return ( m_Flags & MATSYS_VIDCFG_FLAGS_NO_WAIT_FOR_VSYNC ) == 0; }
	bool Stencil() const { return (m_Flags & MATSYS_VIDCFG_FLAGS_STENCIL ) != 0; }
	bool ForceTrilinear() const { return ( m_Flags & MATSYS_VIDCFG_FLAGS_FORCE_TRILINEAR ) != 0; }
	bool ForceHWSync() const { return ( m_Flags & MATSYS_VIDCFG_FLAGS_FORCE_HWSYNC ) != 0; }
	bool UseSpecular() const { return ( m_Flags & MATSYS_VIDCFG_FLAGS_DISABLE_SPECULAR ) == 0; }
	bool UseBumpmapping() const { return ( m_Flags & MATSYS_VIDCFG_FLAGS_DISABLE_BUMPMAP ) == 0; }
	bool UseParallaxMapping() const { return ( m_Flags & MATSYS_VIDCFG_FLAGS_ENABLE_PARALLAX_MAPPING ) != 0; }
	bool UseZPrefill() const { return ( m_Flags & MATSYS_VIDCFG_FLAGS_USE_Z_PREFILL ) != 0; }
	bool ReduceFillrate() const { return ( m_Flags & MATSYS_VIDCFG_FLAGS_REDUCE_FILLRATE ) != 0; }
	bool HDREnabled() const { return ( m_Flags & MATSYS_VIDCFG_FLAGS_ENABLE_HDR ) != 0; }
	
	void SetFlag( unsigned int flag, bool val )
	{
		if( val )
		{
			m_Flags |= flag;	
		}
		else
		{
			m_Flags &= ~flag;	
		}
	}
	
	// control panel stuff
	MaterialVideoMode_t m_VideoMode;
	float m_fMonitorGamma;
	int m_nAASamples;
	int m_nForceAnisotropicLevel;
	int skipMipLevels;
	int dxSupportLevel;
	unsigned int m_Flags;
	bool bEditMode;				// true if in Hammer.
	unsigned char proxiesTestMode;	// 0 = normal, 1 = no proxies, 2 = alpha test all, 3 = color mod all
	bool bCompressedTextures;
	bool bFilterLightmaps;
	bool bFilterTextures;
	bool bReverseDepth;
	bool bBufferPrimitives;
	bool bDrawFlat;
	bool bMeasureFillRate;
	bool bVisualizeFillRate;
	bool bNoTransparency;
	bool bSoftwareLighting;
	bool bAllowCheats;
	bool bShowMipLevels;
	bool bShowLowResImage;
	bool bShowNormalMap; 
	bool bMipMapTextures;
	unsigned char nFullbright;
	bool m_bFastNoBump;
	bool m_bSuppressRendering;

	// debug modes
	bool bShowSpecular; // This is the fast version that doesn't require reloading materials
	bool bShowDiffuse;  // This is the fast version that doesn't require reloading materials

	// misc
	// can we get rid of numTextureUnits?
	int numTextureUnits; // set to zero if there is no limit on the 
						 // number of texture units to be used.
						 // otherwise, the effective number of texture units
						 // will be max( config->numTexturesUnits, hardwareNumTextureUnits )

	float m_SlopeScaleDepthBias_Decal;
	float m_SlopeScaleDepthBias_Normal;
	float m_DepthBias_Decal;
	float m_DepthBias_Normal;

	MaterialSystem_Config_t()
	{
		memset( this, 0, sizeof( *this ) );

		// video config defaults
		SetFlag( MATSYS_VIDCFG_FLAGS_WINDOWED, false );
		SetFlag( MATSYS_VIDCFG_FLAGS_RESIZING, false );
		SetFlag( MATSYS_VIDCFG_FLAGS_NO_WAIT_FOR_VSYNC, true );
		SetFlag( MATSYS_VIDCFG_FLAGS_STENCIL, false );
		SetFlag( MATSYS_VIDCFG_FLAGS_FORCE_TRILINEAR, true );
		SetFlag( MATSYS_VIDCFG_FLAGS_FORCE_HWSYNC, true );
		SetFlag( MATSYS_VIDCFG_FLAGS_DISABLE_SPECULAR, false );
		SetFlag( MATSYS_VIDCFG_FLAGS_DISABLE_BUMPMAP, false );
		SetFlag( MATSYS_VIDCFG_FLAGS_ENABLE_PARALLAX_MAPPING, true );
		SetFlag( MATSYS_VIDCFG_FLAGS_USE_Z_PREFILL, false );
		SetFlag( MATSYS_VIDCFG_FLAGS_REDUCE_FILLRATE, false );
		m_VideoMode.m_Width = 640;
		m_VideoMode.m_Height = 480;
		m_VideoMode.m_RefreshRate = 60;
		dxSupportLevel = 0;
		bCompressedTextures = true;
		numTextureUnits = 0;
		bFilterTextures = true;
		bFilterLightmaps = true;
		bMipMapTextures = true;
		bBufferPrimitives = true;
		m_fMonitorGamma = 2.2f;

		// misc defaults
		bAllowCheats = false;
		bCompressedTextures = true;
		bEditMode = false;

		// debug modes
		bShowSpecular = true;
		bShowDiffuse = true;
		nFullbright = 0;
		bShowNormalMap = false;
		bFilterLightmaps = true;
		bFilterTextures = true;
		bMipMapTextures = true;
		bShowMipLevels = false;
		bShowLowResImage = false;
		bReverseDepth = false;
		bBufferPrimitives = true;
		bDrawFlat = false;
		bMeasureFillRate = false;
		bVisualizeFillRate = false;
		bSoftwareLighting = false;
		bNoTransparency = false;
		proxiesTestMode = 0;
		m_bFastNoBump = false;
		m_bSuppressRendering = false;
		m_SlopeScaleDepthBias_Decal = -0.5f;
		m_SlopeScaleDepthBias_Normal = 0.0f;
		m_DepthBias_Decal = -262144;
		m_DepthBias_Normal = 0.0f;
	}
};


#endif // MATERIALSYSTEM_CONFIG_H
