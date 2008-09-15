//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//===========================================================================//

#ifndef IMATERIALSYSTEMHARDWARECONFIG_H
#define IMATERIALSYSTEMHARDWARECONFIG_H

#ifdef _WIN32
#pragma once
#endif


#include "tier1/interface.h"


//-----------------------------------------------------------------------------
// Material system interface version
//-----------------------------------------------------------------------------
#define MATERIALSYSTEM_HARDWARECONFIG_INTERFACE_VERSION		"MaterialSystemHardwareConfig012"

// HDRFIXME NOTE: must match common_ps_fxc.h
enum HDRType_t
{
	HDR_TYPE_NONE,
	HDR_TYPE_INTEGER,
	HDR_TYPE_FLOAT,
};

//-----------------------------------------------------------------------------
// Material system configuration
//-----------------------------------------------------------------------------
class IMaterialSystemHardwareConfig
{
public:
	virtual bool HasDestAlphaBuffer() const = 0;
	virtual bool HasStencilBuffer() const = 0;
	virtual int	 GetFrameBufferColorDepth() const = 0;
	virtual int  GetNumTextureUnits() const = 0;
	virtual bool HasSetDeviceGammaRamp() const = 0;
	virtual bool SupportsCompressedTextures() const = 0;
	virtual bool SupportsVertexAndPixelShaders() const = 0;
	virtual bool SupportsPixelShaders_1_4() const = 0;
	virtual bool SupportsPixelShaders_2_0() const = 0;
	virtual bool SupportsVertexShaders_2_0() const = 0;
	virtual int  MaximumAnisotropicLevel() const = 0;	// 0 means no anisotropic filtering
	virtual int  MaxTextureWidth() const = 0;
	virtual int  MaxTextureHeight() const = 0;
	virtual int	 TextureMemorySize() const = 0;
	virtual bool SupportsOverbright() const = 0;
	virtual bool SupportsCubeMaps() const = 0;
	virtual bool SupportsMipmappedCubemaps() const = 0;
	virtual bool SupportsNonPow2Textures() const = 0;

	// The number of texture stages represents the number of computations
	// we can do in the pixel pipeline, it is *not* related to the
	// simultaneous number of textures we can use
	virtual int  GetNumTextureStages() const = 0;
	virtual int	 NumVertexShaderConstants() const = 0;
	virtual int	 NumPixelShaderConstants() const = 0;
	virtual int	 MaxNumLights() const = 0;
	virtual bool SupportsHardwareLighting() const = 0;
	virtual int	 MaxBlendMatrices() const = 0;
	virtual int	 MaxBlendMatrixIndices() const = 0;
	virtual int	 MaxTextureAspectRatio() const = 0;
	virtual int	 MaxVertexShaderBlendMatrices() const = 0;
	virtual int	 MaxUserClipPlanes() const = 0;
	virtual bool UseFastClipping() const = 0;

	// This here should be the major item looked at when checking for compat
	// from anywhere other than the material system	shaders
	virtual int	 GetDXSupportLevel() const = 0;
	virtual const char *GetShaderDLLName() const = 0;

	virtual bool ReadPixelsFromFrontBuffer() const = 0;

	// Are dx dynamic textures preferred?
	virtual bool PreferDynamicTextures() const = 0;

	virtual bool SupportsHDR() const = 0;

	virtual bool HasProjectedBumpEnv() const = 0;
	virtual bool SupportsSpheremapping() const = 0;
	virtual bool NeedsAAClamp() const = 0;
	virtual bool NeedsATICentroidHack() const = 0;

	virtual bool SupportsColorOnSecondStream() const = 0;
	virtual bool SupportsStaticPlusDynamicLighting() const = 0;

	// Does our card have a hard time with fillrate 
	// relative to other cards w/ the same dx level?
	virtual bool PreferReducedFillrate() const = 0;

	// This is the max dx support level supported by the card
	virtual int	 GetMaxDXSupportLevel() const = 0;

	// Does the card specify fog color in linear space when sRGBWrites are enabled?
	virtual bool SpecifiesFogColorInLinearSpace() const = 0;

	// Does the card support sRGB reads/writes?
	virtual bool SupportsSRGB() const = 0;

	virtual bool IsAAEnabled() const = 0;	// Is antialiasing being used?

	// NOTE: Anything after this was added after shipping HL2.
	virtual int GetVertexTextureCount() const = 0;
	virtual int GetMaxVertexTextureDimension() const = 0;

	virtual int  MaxTextureDepth() const = 0;

	virtual HDRType_t GetHDRType() const = 0;
	virtual HDRType_t GetHardwareHDRType() const = 0;

	virtual bool SupportsPixelShaders_2_b() const = 0;
	virtual bool SupportsStreamOffset() const = 0;
};

#endif // IMATERIALSYSTEMHARDWARECONFIG_H
