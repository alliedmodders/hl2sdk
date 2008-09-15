//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef IDEBUGTEXTUREINFO_H
#define IDEBUGTEXTUREINFO_H
#ifdef _WIN32
#pragma once
#endif


class KeyValues;


// This interface is actually exported by the shader API DLL.
#define DEBUG_TEXTURE_INFO_VERSION "DebugTextureInfo001"


abstract_class IDebugTextureInfo
{
public:
	
	// Use this to turn on the mode where it builds the debug texture list.
	// At the end of the next frame, GetDebugTextureList() will return a valid list of the textures.
	virtual void EnableDebugTextureList( bool bEnable ) = 0;
	
	// If this is on, then it will return all textures that exist, not just the ones that were bound in the last frame.
	virtual void EnableGetAllTextures( bool bEnable ) = 0;

	// Use this to get the results of the texture list.
	// Do NOT release the KeyValues after using them.
	// There will be a bunch of subkeys, each with these values:
	//    Name   - the texture's filename
	//    Binds  - how many times the texture was bound
	//    Format - ImageFormat of the texture
	//    Width  - Width of the texture
	//    Height - Height of the texture
	virtual KeyValues* GetDebugTextureList() = 0;

	// This returns how much memory was used the last frame. It is not necessary to
	// enable any mode to get this - it is always available.
	virtual int GetTextureMemoryUsedLastFrame() = 0;
};


#endif // IDEBUGTEXTUREINFO_H
