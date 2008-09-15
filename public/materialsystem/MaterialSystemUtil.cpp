//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
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
	if (pMaterialName)
	{
		Assert( pTextureGroupName );
		Init( pMaterialName, pTextureGroupName, bComplain );
	}
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
	if ( pMaterial != m_pMaterial )
	{
		Shutdown();
		m_pMaterial = pMaterial;
		if ( m_pMaterial )
		{
			m_pMaterial->IncrementReferenceCount();
		}
	}
}

void CMaterialReference::Init( const char *pMaterialName, KeyValues *pVMTKeyValues )
{
	// CreateMaterial has a refcount of 1
	Shutdown();
	m_pMaterial = materials->CreateMaterial( pMaterialName, pVMTKeyValues );
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
void CMaterialReference::Shutdown( )
{
	if ( m_pMaterial )
	{
		m_pMaterial->DecrementReferenceCount();
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

CTextureReference::CTextureReference( const CTextureReference &ref )
{
	m_pTexture = ref.m_pTexture;
	if( m_pTexture )
	{
		m_pTexture->IncrementReferenceCount();
	}
}

void CTextureReference::operator=( CTextureReference &ref )
{
	m_pTexture = ref.m_pTexture;
	if( m_pTexture )
	{
		m_pTexture->IncrementReferenceCount();
	}
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
	Shutdown();

	m_pTexture = pTexture;
	if (m_pTexture)
	{
		m_pTexture->IncrementReferenceCount();
	}
}

void CTextureReference::InitProceduralTexture( const char *pTextureName, const char *pTextureGroupName, int w, int h, ImageFormat fmt, int nFlags )
{
	Shutdown();

	m_pTexture = materials->CreateProceduralTexture( pTextureName, pTextureGroupName, w, h, fmt, nFlags );
	if ( m_pTexture )
	{
		m_pTexture->IncrementReferenceCount();
	}
}

void CTextureReference::InitRenderTarget( int w, int h, RenderTargetSizeMode_t sizeMode, ImageFormat fmt, MaterialRenderTargetDepth_t depth, bool bHDR )
{
	Shutdown();

	int textureFlags = TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT;
	if( depth == MATERIAL_RT_DEPTH_ONLY )
		textureFlags |= TEXTUREFLAGS_POINTSAMPLE;

	// NOTE: Refcount returned by CreateRenderTargetTexture is 1
	m_pTexture = materials->CreateNamedRenderTargetTextureEx( NULL, w, h, sizeMode, fmt, 
		depth, textureFlags, 
		bHDR ? CREATERENDERTARGETFLAGS_HDR : 0 );

	Assert( m_pTexture );
}

//-----------------------------------------------------------------------------
// Detach from a texture
//-----------------------------------------------------------------------------
void CTextureReference::Shutdown()
{
	if ( m_pTexture )
	{
		m_pTexture->DecrementReferenceCount();
		m_pTexture = NULL;
	}
}

