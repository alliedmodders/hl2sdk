//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $Workfile:     $
// $NoKeywords: $
//===========================================================================//

#include "materialsystem/MaterialSystemUtil.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/itexture.h"
#include "materialsystem/imaterialsystem.h"
#include "tier1/KeyValues.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Little utility class to deal with material references
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------
CMaterialReference::CMaterialReference( char const* pMaterialName, const char *pTextureGroupName, bool bComplain ) : m_pMaterial( 0 )
{
	if ( pMaterialName )
	{
		Assert( pTextureGroupName );
		Init( pMaterialName, pTextureGroupName, bComplain );
	}
}

const CMaterialReference& CMaterialReference::operator=( const CMaterialReference &ref )
{
	Init( ref.m_pMaterial );
	return *this;
}

CMaterialReference::~CMaterialReference()
{
	Shutdown();
}

//-----------------------------------------------------------------------------
// Attach to a material
//-----------------------------------------------------------------------------
void CMaterialReference::Init( char const* pMaterialName, const char *pTextureGroupName, bool bComplain )
{
	IMaterial *pMaterial = materials->FindMaterial( pMaterialName, pTextureGroupName, bComplain);
	Assert( pMaterial );
	Init( pMaterial );
}

void CMaterialReference::Init( const char *pMaterialName, KeyValues *pVMTKeyValues )
{
	// CreateMaterial has a refcount of 1
	Shutdown();
	m_pMaterial = materials->CreateMaterial( pMaterialName, pVMTKeyValues );
}

void CMaterialReference::Init( const char *pMaterialName, const char *pTextureGroupName, KeyValues *pVMTKeyValues )
{
	IMaterial *pMaterial = materials->FindProceduralMaterial( pMaterialName, pTextureGroupName, pVMTKeyValues );
	Assert( pMaterial );
	Init( pMaterial );
}

void CMaterialReference::Init( IMaterial* pMaterial )
{
	if ( m_pMaterial != pMaterial )
	{
		Shutdown();
		m_pMaterial = pMaterial;
		if ( m_pMaterial )
		{
			m_pMaterial->IncrementReferenceCount();
		}
	}
}

void CMaterialReference::Init( CMaterialReference& ref )
{
	if ( m_pMaterial != ref.m_pMaterial )
	{
		Shutdown();
		m_pMaterial = ref.m_pMaterial;
		if (m_pMaterial)
		{
			m_pMaterial->IncrementReferenceCount();
		}
	}
}

//-----------------------------------------------------------------------------
// Detach from a material
//-----------------------------------------------------------------------------
void CMaterialReference::Shutdown( bool bDeleteIfUnreferenced /*=false*/ )
{
	if ( m_pMaterial && materials )
	{
		m_pMaterial->DecrementReferenceCount();
		if ( bDeleteIfUnreferenced )
		{
			m_pMaterial->DeleteIfUnreferenced();
		}
		m_pMaterial = NULL;
	}
}


//-----------------------------------------------------------------------------
// Little utility class to deal with texture references
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------
CTextureReference::CTextureReference( ) : m_pTexture(NULL)
{
}

CTextureReference::CTextureReference( const CTextureReference &ref ) : m_pTexture( NULL )
{
	Init( ref.m_pTexture );
}

const CTextureReference& CTextureReference::operator=( CTextureReference &ref )
{
	Init( ref.m_pTexture );
	return *this;
}

CTextureReference::~CTextureReference( )
{
	Shutdown();
}

//-----------------------------------------------------------------------------
// Attach to a texture
//-----------------------------------------------------------------------------
void CTextureReference::Init( char const* pTextureName, const char *pTextureGroupName, bool bComplain )
{
	Shutdown();
	m_pTexture = materials->FindTexture( pTextureName, pTextureGroupName, bComplain );
	if ( m_pTexture )
	{
		m_pTexture->IncrementReferenceCount();
	}
}

void CTextureReference::Init( ITexture* pTexture )
{
	if ( m_pTexture != pTexture )
	{
		Shutdown();

		m_pTexture = pTexture;
		if (m_pTexture)
		{
			m_pTexture->IncrementReferenceCount();
		}
	}
}

void CTextureReference::InitProceduralTexture( const char *pTextureName, const char *pTextureGroupName, int w, int h, ImageFormat fmt, int nFlags )
{
	Shutdown();

	m_pTexture = materials->CreateProceduralTexture( pTextureName, pTextureGroupName, w, h, fmt, nFlags );
	
	// NOTE: The texture reference is already incremented internally above!
	/*
	if ( m_pTexture )
	{
		m_pTexture->IncrementReferenceCount();
	}
	*/
}

void CTextureReference::InitRenderTarget( int w, int h, RenderTargetSizeMode_t sizeMode, ImageFormat fmt, MaterialRenderTargetDepth_t depth, bool bHDR, char *pStrOptionalName /* = NULL */ )
{
	Shutdown();

	int textureFlags = TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT;
	if ( depth == MATERIAL_RT_DEPTH_ONLY )
		textureFlags |= TEXTUREFLAGS_POINTSAMPLE;

	int renderTargetFlags = bHDR ? CREATERENDERTARGETFLAGS_HDR : 0;

	// NOTE: Refcount returned by CreateRenderTargetTexture is 1
	m_pTexture = materials->CreateNamedRenderTargetTextureEx( pStrOptionalName, w, h, sizeMode, fmt, 
		depth, textureFlags, renderTargetFlags );

	Assert( m_pTexture );
}

//-----------------------------------------------------------------------------
// Detach from a texture
//-----------------------------------------------------------------------------
void CTextureReference::Shutdown( bool bDeleteIfUnReferenced )
{
	if ( m_pTexture && materials )
	{
		m_pTexture->DecrementReferenceCount();
		if ( bDeleteIfUnReferenced )
		{
			m_pTexture->DeleteIfUnreferenced();
		}
		m_pTexture = NULL;
	}
}

//-----------------------------------------------------------------------------
// Builds ONLY the system ram render target. Used when caller is explicitly managing.
// The paired EDRAM surface can be built in an alternate format.
//-----------------------------------------------------------------------------
#if defined( _X360 )
void CTextureReference::InitRenderTargetTexture( int w, int h, RenderTargetSizeMode_t sizeMode, ImageFormat fmt, MaterialRenderTargetDepth_t depth, bool bHDR, char *pStrOptionalName )
{
	// other variants not implemented yet
	Assert( depth == MATERIAL_RT_DEPTH_NONE || depth == MATERIAL_RT_DEPTH_SHARED );
	Assert( !bHDR );

	int renderTargetFlags = CREATERENDERTARGETFLAGS_NOEDRAM;

	m_pTexture = materials->CreateNamedRenderTargetTextureEx( 
		pStrOptionalName, 
		w, 
		h, 
		sizeMode, 
		fmt, 
		depth, 
		TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT, 
		renderTargetFlags );
	Assert( m_pTexture );
}
#endif

//-----------------------------------------------------------------------------
// Builds ONLY the EDRAM render target surface. Used when caller is explicitly managing.
// The paired system memory texture can be built in an alternate format.
//-----------------------------------------------------------------------------
#if defined( _X360 )
void CTextureReference::InitRenderTargetSurface( int width, int height, ImageFormat fmt, bool bSameAsTexture, RTMultiSampleCount360_t multiSampleCount )
{
	// texture has to be created first
	Assert( m_pTexture && m_pTexture->IsRenderTarget() );

	m_pTexture->CreateRenderTargetSurface( width, height, fmt, bSameAsTexture, multiSampleCount );
}
#endif

