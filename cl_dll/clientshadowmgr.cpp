//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//
// Interface to the client system responsible for dealing with shadows
//
// Boy is this complicated. OK, lets talk about how this works at the moment
//
// The ClientShadowMgr contains all of the highest-level state for rendering
// shadows, and it controls the ShadowMgr in the engine which is the central
// clearing house for rendering shadows.
//
// There are two important types of objects with respect to shadows:
// the shadow receiver, and the shadow caster. How is the association made
// between casters + the receivers? Turns out it's done slightly differently 
// depending on whether the receiver is the world, or if it's an entity.
//
// In the case of the world, every time the engine's ProjectShadow() is called, 
// any previous receiver state stored (namely, which world surfaces are
// receiving shadows) are cleared. Then, when ProjectShadow is called, 
// the engine iterates over all nodes + leaves within the shadow volume and 
// marks front-facing surfaces in them as potentially being affected by the 
// shadow. Later on, if those surfaces are actually rendered, the surfaces
// are clipped by the shadow volume + rendered.
// 
// In the case of entities, there are slightly different methods depending
// on whether the receiver is a brush model or a studio model. However, there
// are a couple central things that occur with both.
//
// Every time a shadow caster is moved, the ClientLeafSystem's ProjectShadow
// method is called to tell it to remove the shadow from all leaves + all 
// renderables it's currently associated with. Then it marks each leaf in the
// shadow volume as being affected by that shadow, and it marks every renderable
// in that volume as being potentially affected by the shadow (the function
// AddShadowToRenderable is called for each renderable in leaves affected
// by the shadow volume).
//
// Every time a shadow receiver is moved, the ClientLeafSystem first calls 
// RemoveAllShadowsFromRenderable to have it clear out its state, and then
// the ClientLeafSystem calls AddShadowToRenderable() for all shadows in all
// leaves the renderable has moved into.
//
// Now comes the difference between brush models + studio models. In the case
// of brush models, when a shadow is added to the studio model, it's done in
// the exact same way as for the world. Surfaces on the brush model are marked
// as potentially being affected by the shadow, and if those surfaces are
// rendered, the surfaces are clipped to the shadow volume. When ProjectShadow()
// is called, turns out the same operation that removes the shadow that moved
// from the world surfaces also works to remove the shadow from brush surfaces.
//
// In the case of studio models, we need a separate operation to remove
// the shadow from all studio models
//===========================================================================//


#include "cbase.h"
#include "engine/IShadowMgr.h"
#include "model_types.h"
#include "bitmap/imageformat.h"
#include "materialsystem/IMaterialProxy.h"
#include "materialsystem/IMaterialVar.h"
#include "materialsystem/IMaterial.h"
#include "materialsystem/IMesh.h"
#include "materialsystem/ITexture.h"
#include "utlmultilist.h"
#include "CollisionUtils.h"
#include "IVRenderView.h"
#include "tier0/vprof.h"
#include "engine/ivmodelinfo.h"
#include "view_shared.h"
#include "engine/IVDebugOverlay.h"
#include "engine/IStaticPropMgr.h"
#include "datacache/imdlcache.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar r_flashlightdrawfrustum( "r_flashlightdrawfrustum", "0" );
static ConVar r_flashlightmodels( "r_flashlightmodels", "1" );
static ConVar r_shadowrendertotexture( "r_shadowrendertotexture", "1" );

#ifdef DOSHADOWEDFLASHLIGHT
static ConVar r_flashlightdepthtexture( "r_flashlightdepthtexture", "0" );
static ConVar r_flashlightdepthres( "r_flashlightdepthres", "512" );
#endif


#ifdef _WIN32
#pragma warning( disable: 4701 )
#endif

//-----------------------------------------------------------------------------
// A texture allocator used to batch textures together
// At the moment, the implementation simply allocates blocks of max 256x256
// and each block stores an array of uniformly-sized textures
//-----------------------------------------------------------------------------
typedef unsigned short TextureHandle_t;
enum
{
	INVALID_TEXTURE_HANDLE = (TextureHandle_t)~0
};

class CTextureAllocator
{
public:
	// Initialize the allocator with something that knows how to refresh the bits
	void			Init();
	void			Shutdown();

	// Resets the allocator
	void			Reset();

	// Deallocates everything
	void			DeallocateAllTextures();

	// Allocate, deallocate texture
	TextureHandle_t	AllocateTexture( int w, int h );
	void			DeallocateTexture( TextureHandle_t h );

	// Mark texture as being used... (return true if re-render is needed)
	bool			UseTexture( TextureHandle_t h, bool bWillRedraw, float flArea );

	// Advance frame...
	void			AdvanceFrame();

	// Get at the location of the texture
	void			GetTextureRect(TextureHandle_t handle, int& x, int& y, int& w, int& h );

	// Get at the texture it's a part of
	ITexture*		GetTexture();
	
	// Get at the total texture size.
	void			GetTotalTextureSize( int& w, int& h );

	void			DebugPrintCache( void );

private:
	typedef unsigned short FragmentHandle_t;

	enum
	{
		INVALID_FRAGMENT_HANDLE = (FragmentHandle_t)~0,
#ifndef _XBOX
		TEXTURE_PAGE_SIZE	    = 1024,
		MAX_TEXTURE_POWER    	= 8,
		MIN_TEXTURE_POWER	    = 4,
#else
		TEXTURE_PAGE_SIZE	    = 512,
		MAX_TEXTURE_POWER    	= 7,
		MIN_TEXTURE_POWER	    = 3,
#endif
		MAX_TEXTURE_SIZE	    = (1 << MAX_TEXTURE_POWER),
		MIN_TEXTURE_SIZE	    = (1 << MIN_TEXTURE_POWER),
		BLOCK_SIZE			    = MAX_TEXTURE_SIZE,
		BLOCKS_PER_ROW		    = (TEXTURE_PAGE_SIZE / MAX_TEXTURE_SIZE),
		BLOCK_COUNT			    = (BLOCKS_PER_ROW * BLOCKS_PER_ROW),
	};

	struct TextureInfo_t
	{
		FragmentHandle_t	m_Fragment;
		unsigned short		m_Size;
		unsigned short		m_Power;
	};

	struct FragmentInfo_t
	{
		unsigned short	m_Block;
		unsigned short	m_Index;
		TextureHandle_t	m_Texture;

		// Makes sure we don't overflow
		unsigned int	m_FrameUsed;
	};

	struct BlockInfo_t
	{
		unsigned short	m_FragmentPower;
	};

	struct Cache_t
	{
		unsigned short	m_List;
	};

	// Adds a block worth of fragments to the LRU
	void AddBlockToLRU( int block );

	// Unlink fragment from cache
	void UnlinkFragmentFromCache( Cache_t& cache, FragmentHandle_t fragment );

	// Mark something as being used (MRU)..
	void MarkUsed( FragmentHandle_t fragment );

	// Mark something as being unused (LRU)..
	void MarkUnused( FragmentHandle_t fragment );

	// Disconnect texture from fragment
	void DisconnectTextureFromFragment( FragmentHandle_t f );

	// Returns the size of a particular fragment
	int	GetFragmentPower( FragmentHandle_t f ) const;

	// Stores the actual texture we're writing into
	CTextureReference	m_TexturePage;

	CUtlLinkedList< TextureInfo_t, TextureHandle_t >	m_Textures;
	CUtlMultiList< FragmentInfo_t, FragmentHandle_t >	m_Fragments;

	Cache_t		m_Cache[MAX_TEXTURE_POWER+1]; 
	BlockInfo_t	m_Blocks[BLOCK_COUNT];
	unsigned int m_CurrentFrame;
};


//-----------------------------------------------------------------------------
// Allocate/deallocate the texture page
//-----------------------------------------------------------------------------
void CTextureAllocator::Init()
{
	for ( int i = 0; i <= MAX_TEXTURE_POWER; ++i )
	{
		m_Cache[i].m_List = m_Fragments.InvalidIndex();
	}

#ifndef _XBOX
	// GR: don't need depth buffer for shadows
	m_TexturePage.InitRenderTarget( TEXTURE_PAGE_SIZE, TEXTURE_PAGE_SIZE, RT_SIZE_NO_CHANGE, IMAGE_FORMAT_ARGB8888, MATERIAL_RT_DEPTH_NONE, false );
#else
	// xboxissue - has to be linear format because swizzled render targets cannot be subrect cleared.
	// can use a 16 bit format, and use rgb channel instead of alpha for shadow info
	m_TexturePage.InitRenderTarget( TEXTURE_PAGE_SIZE, TEXTURE_PAGE_SIZE, RT_SIZE_NO_CHANGE, IMAGE_FORMAT_LINEAR_BGRX5551, MATERIAL_RT_DEPTH_NONE, false );
#endif
}

void CTextureAllocator::Shutdown()
{
	m_TexturePage.Shutdown( );
}


//-----------------------------------------------------------------------------
// Initialize the allocator with something that knows how to refresh the bits
//-----------------------------------------------------------------------------
void CTextureAllocator::Reset()
{
	DeallocateAllTextures();

	m_Textures.EnsureCapacity(256);
	m_Fragments.EnsureCapacity(256);

	// Set up the block sizes....
	// FIXME: Improve heuristic?!?
#ifndef _XBOX
	m_Blocks[0].m_FragmentPower = 4;	// 128 x 16
	m_Blocks[1].m_FragmentPower = 5;	// 64 x 32
	m_Blocks[2].m_FragmentPower = 6;	// 32 x 64
	m_Blocks[3].m_FragmentPower = 6;		 
	m_Blocks[4].m_FragmentPower = 7;	// 24 x 128
	m_Blocks[5].m_FragmentPower = 7;
	m_Blocks[6].m_FragmentPower = 7;
	m_Blocks[7].m_FragmentPower = 7;
	m_Blocks[8].m_FragmentPower = 7;
	m_Blocks[9].m_FragmentPower = 7;
	m_Blocks[10].m_FragmentPower = 8;	// 6 x 256
	m_Blocks[11].m_FragmentPower = 8;	 
	m_Blocks[12].m_FragmentPower = 8;
	m_Blocks[13].m_FragmentPower = 8;
	m_Blocks[14].m_FragmentPower = 8;
	m_Blocks[15].m_FragmentPower = 8;
#else
	m_Blocks[0].m_FragmentPower  = MAX_TEXTURE_POWER-4;	// 128 cells at ExE
	m_Blocks[1].m_FragmentPower  = MAX_TEXTURE_POWER-3;	// 64 cells at DxD
	m_Blocks[2].m_FragmentPower  = MAX_TEXTURE_POWER-2;	// 32 cells at CxC
	m_Blocks[3].m_FragmentPower  = MAX_TEXTURE_POWER-2;		 
	m_Blocks[4].m_FragmentPower  = MAX_TEXTURE_POWER-1;	// 24 cells at BxB
	m_Blocks[5].m_FragmentPower  = MAX_TEXTURE_POWER-1;
	m_Blocks[6].m_FragmentPower  = MAX_TEXTURE_POWER-1;
	m_Blocks[7].m_FragmentPower  = MAX_TEXTURE_POWER-1;
	m_Blocks[8].m_FragmentPower  = MAX_TEXTURE_POWER-1;
	m_Blocks[9].m_FragmentPower  = MAX_TEXTURE_POWER-1;
	m_Blocks[10].m_FragmentPower = MAX_TEXTURE_POWER;	// 6 cells at AxA
	m_Blocks[11].m_FragmentPower = MAX_TEXTURE_POWER;	 
	m_Blocks[12].m_FragmentPower = MAX_TEXTURE_POWER;
	m_Blocks[13].m_FragmentPower = MAX_TEXTURE_POWER;
	m_Blocks[14].m_FragmentPower = MAX_TEXTURE_POWER;
	m_Blocks[15].m_FragmentPower = MAX_TEXTURE_POWER;
#endif

	// Initialize the LRU
	int i;
	for ( i = 0; i <= MAX_TEXTURE_POWER; ++i )
	{
		m_Cache[i].m_List = m_Fragments.CreateList();
	}

	// Now that the block sizes are allocated, create LRUs for the various block sizes
	for ( i = 0; i < BLOCK_COUNT; ++i)
	{
		// Initialize LRU
		AddBlockToLRU( i );
	}

	m_CurrentFrame = 0;
}

void CTextureAllocator::DeallocateAllTextures()
{
	m_Textures.Purge();
	m_Fragments.Purge();
	for ( int i = 0; i <= MAX_TEXTURE_POWER; ++i )
	{
		m_Cache[i].m_List = m_Fragments.InvalidIndex();
	}
}


//-----------------------------------------------------------------------------
// Dump the state of the cache to debug out
//-----------------------------------------------------------------------------
void CTextureAllocator::DebugPrintCache( void )
{
	// For each fragment
	int nNumFragments = m_Fragments.TotalCount();
	int nNumInvalidFragments = 0;

	Warning("Fragments (%d):\n===============\n", nNumFragments);

	for ( int f = 0; f < nNumFragments; f++ )
	{
		if ( ( m_Fragments[f].m_FrameUsed != 0 ) && ( m_Fragments[f].m_Texture != INVALID_TEXTURE_HANDLE ) )
			Warning("Fragment %d, Block: %d, Index: %d, Texture: %d Frame Used: %d\n", f, m_Fragments[f].m_Block, m_Fragments[f].m_Index, m_Fragments[f].m_Texture, m_Fragments[f].m_FrameUsed );
		else
			nNumInvalidFragments++;
	}

	Warning("Invalid Fragments: %d\n", nNumInvalidFragments);

//	for ( int c = 0; c <= MAX_TEXTURE_POWER; ++c )
//	{
//		Warning("Cache Index (%d)\n", m_Cache[c].m_List);
//	}

}


//-----------------------------------------------------------------------------
// Adds a block worth of fragments to the LRU
//-----------------------------------------------------------------------------
void CTextureAllocator::AddBlockToLRU( int block )
{
	int power = m_Blocks[block].m_FragmentPower;
 	int size = (1 << power);

	// Compute the number of fragments in this block
	int fragmentCount = MAX_TEXTURE_SIZE / size;
	fragmentCount *= fragmentCount;

	// For each fragment, indicate which block it's a part of (and the index)
	// and then stick in at the top of the LRU
	while (--fragmentCount >= 0 )
	{
		FragmentHandle_t f = m_Fragments.Alloc( );
		m_Fragments[f].m_Block = block;
		m_Fragments[f].m_Index = fragmentCount;
		m_Fragments[f].m_Texture = INVALID_TEXTURE_HANDLE;
		m_Fragments[f].m_FrameUsed = 0xFFFFFFFF;
		m_Fragments.LinkToHead( m_Cache[power].m_List, f );
	}
}


//-----------------------------------------------------------------------------
// Unlink fragment from cache
//-----------------------------------------------------------------------------
void CTextureAllocator::UnlinkFragmentFromCache( Cache_t& cache, FragmentHandle_t fragment )
{
	m_Fragments.Unlink( cache.m_List, fragment);
}


//-----------------------------------------------------------------------------
// Mark something as being used (MRU)..
//-----------------------------------------------------------------------------
void CTextureAllocator::MarkUsed( FragmentHandle_t fragment )
{
	int block = m_Fragments[fragment].m_Block;
	int power = m_Blocks[block].m_FragmentPower;

	// Hook it at the end of the LRU
	Cache_t& cache = m_Cache[power];
	m_Fragments.LinkToTail( cache.m_List, fragment );
	m_Fragments[fragment].m_FrameUsed = m_CurrentFrame;
}


//-----------------------------------------------------------------------------
// Mark something as being unused (LRU)..
//-----------------------------------------------------------------------------
void CTextureAllocator::MarkUnused( FragmentHandle_t fragment )
{
	int block = m_Fragments[fragment].m_Block;
	int power = m_Blocks[block].m_FragmentPower;

	// Hook it at the end of the LRU
	Cache_t& cache = m_Cache[power];
	m_Fragments.LinkToHead( cache.m_List, fragment );
}


//-----------------------------------------------------------------------------
// Allocate, deallocate texture
//-----------------------------------------------------------------------------
TextureHandle_t	CTextureAllocator::AllocateTexture( int w, int h )
{
	// Implementational detail for now
	Assert( w == h );

	// Clamp texture size
	if (w < MIN_TEXTURE_SIZE)
		w = MIN_TEXTURE_SIZE;
	else if (w > MAX_TEXTURE_SIZE)
		w = MAX_TEXTURE_SIZE;

	TextureHandle_t handle = m_Textures.AddToTail();
	m_Textures[handle].m_Fragment = INVALID_FRAGMENT_HANDLE;
	m_Textures[handle].m_Size = w;

	// Find the power of two
	int power = 0;
	int size = 1;
	while(size < w)
	{
		size <<= 1;
		++power;
	}
	Assert( size == w );

	m_Textures[handle].m_Power = power;

	return handle;
}

void CTextureAllocator::DeallocateTexture( TextureHandle_t h )
{
//	Warning("Beginning of DeallocateTexture\n");
//	DebugPrintCache();

	if (m_Textures[h].m_Fragment != INVALID_FRAGMENT_HANDLE)
	{
		MarkUnused(m_Textures[h].m_Fragment);
		m_Fragments[m_Textures[h].m_Fragment].m_FrameUsed = 0xFFFFFFFF;	// non-zero frame
		DisconnectTextureFromFragment( m_Textures[h].m_Fragment );
	}
	m_Textures.Remove(h);

//	Warning("End of DeallocateTexture\n");
//	DebugPrintCache();
}


//-----------------------------------------------------------------------------
// Disconnect texture from fragment
//-----------------------------------------------------------------------------
void CTextureAllocator::DisconnectTextureFromFragment( FragmentHandle_t f )
{
//	Warning( "Beginning of DisconnectTextureFromFragment\n" );
//	DebugPrintCache();

	FragmentInfo_t& info = m_Fragments[f];
	if (info.m_Texture != INVALID_TEXTURE_HANDLE)
	{
		m_Textures[info.m_Texture].m_Fragment = INVALID_FRAGMENT_HANDLE;
		info.m_Texture = INVALID_TEXTURE_HANDLE;
	}


//	Warning( "End of DisconnectTextureFromFragment\n" );
//	DebugPrintCache();
}


//-----------------------------------------------------------------------------
// Mark texture as being used...
//-----------------------------------------------------------------------------
bool CTextureAllocator::UseTexture( TextureHandle_t h, bool bWillRedraw, float flArea )
{
//	Warning( "Top of UseTexture\n" );
//	DebugPrintCache();

	TextureInfo_t& info = m_Textures[h];

	// 4 is the minimum power we have allocated
	int nDesiredPower = 4;
	int nDesiredWidth = 16;
	while ( (nDesiredWidth * nDesiredWidth) < flArea )
	{
		if ( nDesiredPower >= info.m_Power )
		{
			nDesiredPower = info.m_Power;
			break;
		}

		++nDesiredPower;
		nDesiredWidth *= 2;
	}

	// If we've got a valid fragment for this texture, no worries!
	int nCurrentPower = -1;
	FragmentHandle_t currentFragment = info.m_Fragment;
	if (currentFragment != INVALID_FRAGMENT_HANDLE)
	{
		// If the current fragment is at or near the desired power, we're done
		nCurrentPower = GetFragmentPower(info.m_Fragment);
		Assert( nCurrentPower <= info.m_Power );
		bool bShouldKeepTexture = (!bWillRedraw) && (nDesiredPower < 8) && (nDesiredPower - nCurrentPower <= 1);
		if ((nCurrentPower == nDesiredPower) || bShouldKeepTexture)
		{
			// Move to the back of the LRU
			MarkUsed( currentFragment );
			return false;
		}
	}

//	Warning( "\n\nUseTexture B\n" );
//	DebugPrintCache();

	// Grab the LRU fragment from the appropriate cache
	// If that fragment is connected to a texture, disconnect it.
	int power = nDesiredPower;

	FragmentHandle_t f = INVALID_FRAGMENT_HANDLE;
	bool done = false;
	while (!done && power >= 0)
	{
		f = m_Fragments.Head( m_Cache[power].m_List );
	
		// This represents an overflow condition (used too many textures of
		// the same size in a single frame). It that happens, just use a texture
		// of lower res.
		if ( (f != m_Fragments.InvalidIndex()) && (m_Fragments[f].m_FrameUsed != m_CurrentFrame) )
		{
			done = true;
		}
		else
		{
			--power;
		}
	}


//	Warning( "\n\nUseTexture C\n" );
//	DebugPrintCache();

	// Ok, lets see if we're better off than we were...
	if (currentFragment != INVALID_FRAGMENT_HANDLE)
	{
		if (power <= nCurrentPower)
		{
			// Oops... we're not. Let's leave well enough alone
			// Move to the back of the LRU
			MarkUsed( currentFragment );
			return false;
		}
		else
		{
			// Clear out the old fragment
			DisconnectTextureFromFragment(currentFragment);
		}
	}

	if ( f == INVALID_FRAGMENT_HANDLE )
	{
		return false;
	}

	// Disconnect existing texture from this fragment (if necessary)
	DisconnectTextureFromFragment(f);

	// Connnect new texture to this fragment
	info.m_Fragment = f;
	m_Fragments[f].m_Texture = h;

	// Move to the back of the LRU
	MarkUsed( f );

	// Indicate we need a redraw
	return true;
}


//-----------------------------------------------------------------------------
// Returns the size of a particular fragment
//-----------------------------------------------------------------------------
int	CTextureAllocator::GetFragmentPower( FragmentHandle_t f ) const
{
	return m_Blocks[m_Fragments[f].m_Block].m_FragmentPower;
}


//-----------------------------------------------------------------------------
// Advance frame...
//-----------------------------------------------------------------------------
void CTextureAllocator::AdvanceFrame()
{
	// Be sure that this is called as infrequently as possible (i.e. once per frame,
	// NOT once per view) to prevent cache thrash when rendering multiple views in a single frame
	m_CurrentFrame++;
}


//-----------------------------------------------------------------------------
// Prepare to render into texture...
//-----------------------------------------------------------------------------
ITexture* CTextureAllocator::GetTexture()
{
	return m_TexturePage;
}


//-----------------------------------------------------------------------------
// Get at the total texture size.
//-----------------------------------------------------------------------------
void CTextureAllocator::GetTotalTextureSize( int& w, int& h )
{
	w = h = TEXTURE_PAGE_SIZE;
}


//-----------------------------------------------------------------------------
// Returns the rectangle the texture lives in..
//-----------------------------------------------------------------------------
void CTextureAllocator::GetTextureRect(TextureHandle_t handle, int& x, int& y, int& w, int& h )
{
	TextureInfo_t& info = m_Textures[handle];
	Assert( info.m_Fragment != INVALID_FRAGMENT_HANDLE );

	// Compute the position of the fragment in the page
	FragmentInfo_t& fragment = m_Fragments[info.m_Fragment];
	int blockY = fragment.m_Block / BLOCKS_PER_ROW;
	int blockX = fragment.m_Block - blockY * BLOCKS_PER_ROW;

	int fragmentSize = (1 << m_Blocks[fragment.m_Block].m_FragmentPower);
	int fragmentsPerRow = BLOCK_SIZE / fragmentSize;
	int fragmentY = fragment.m_Index / fragmentsPerRow;
	int fragmentX = fragment.m_Index - fragmentY * fragmentsPerRow;

	x = blockX * BLOCK_SIZE + fragmentX * fragmentSize;
	y = blockY * BLOCK_SIZE + fragmentY * fragmentSize;
	w = fragmentSize;
	h = fragmentSize;
}


//-----------------------------------------------------------------------------
// Defines how big of a shadow texture we should be making per caster...
//-----------------------------------------------------------------------------
#define TEXEL_SIZE_PER_CASTER_SIZE	2.0f 
#define MAX_FALLOFF_AMOUNT 240
#define MAX_CLIP_PLANE_COUNT 4
#define SHADOW_CULL_TOLERANCE 0.5f

static ConVar r_shadows( "r_shadows", "1" ); // hook into engine's cvars..
static ConVar r_shadowmaxrendered("r_shadowmaxrendered", "32");
static ConVar r_shadows_gamecontrol( "r_shadows_gamecontrol", "-1" ); // hook into engine's cvars..

//-----------------------------------------------------------------------------
// The class responsible for dealing with shadows on the client side
// Oh, and let's take a moment and notice how happy Robin and John must be 
// owing to the lack of space between this lovely comment and the class name =)
//-----------------------------------------------------------------------------
class CClientShadowMgr : public IClientShadowMgr
{
public:
	CClientShadowMgr();

	virtual char const *Name() { return "CCLientShadowMgr"; }

	// Inherited from IClientShadowMgr
	virtual bool Init();
	virtual void Shutdown();
	virtual void LevelInitPreEntity();
	virtual void LevelInitPostEntity() {}
	virtual void LevelShutdownPreEntity() {}
	virtual void LevelShutdownPostEntity();

	virtual bool IsPerFrame() { return true; }

	virtual void PreRender();
	virtual void Update( float frametime ) { }
	virtual void PostRender() {}

	virtual void OnSave() {}
	virtual void OnRestore() {}
	virtual void SafeRemoveIfDesired() {}

	virtual ClientShadowHandle_t CreateShadow( ClientEntityHandle_t entity, int flags );
	virtual void DestroyShadow( ClientShadowHandle_t handle );

	// Create flashlight (projected texture light source)
	virtual ClientShadowHandle_t CreateFlashlight( const FlashlightState_t &lightState );
	virtual void UpdateFlashlightState( ClientShadowHandle_t shadowHandle, const FlashlightState_t &lightState );
	virtual void DestroyFlashlight( ClientShadowHandle_t shadowHandle );

	// Update a shadow
	virtual void UpdateProjectedTexture( ClientShadowHandle_t handle, bool force );

	void ComputeBoundingSphere( IClientRenderable* pRenderable, Vector& origin, float& radius );

	virtual void AddToDirtyShadowList( ClientShadowHandle_t handle, bool bForce );
	virtual void AddToDirtyShadowList( IClientRenderable *pRenderable, bool force );

	// Marks the render-to-texture shadow as needing to be re-rendered
	virtual void MarkRenderToTextureShadowDirty( ClientShadowHandle_t handle );

	// deals with shadows being added to shadow receivers
	void AddShadowToReceiver( ClientShadowHandle_t handle,
		IClientRenderable* pRenderable, ShadowReceiver_t type );

	// deals with shadows being added to shadow receivers
	void RemoveAllShadowsFromReceiver( IClientRenderable* pRenderable, ShadowReceiver_t type );

	// Re-renders all shadow textures for shadow casters that lie in the leaf list
	void ComputeShadowTextures( const CViewSetup *pView, int leafCount, LeafIndex_t* pLeafList );

	// Returns the shadow texture
	ITexture* GetShadowTexture( unsigned short h );

	// Returns shadow information
	const ShadowInfo_t& GetShadowInfo( ClientShadowHandle_t h );

	// Renders the shadow texture to screen...
	void RenderShadowTexture( int w, int h );

	// Sets the shadow direction
	virtual void SetShadowDirection( const Vector& dir );
	const Vector &GetShadowDirection() const;

	// Sets the shadow color
	virtual void SetShadowColor( unsigned char r, unsigned char g, unsigned char b );
	void GetShadowColor( unsigned char *r, unsigned char *g, unsigned char *b ) const;

	// Sets the shadow distance
	virtual void SetShadowDistance( float flMaxDistance );
	float GetShadowDistance( ) const;

	// Sets the screen area at which blobby shadows are always used
	virtual void SetShadowBlobbyCutoffArea( float flMinArea );
	float GetBlobbyCutoffArea( ) const;

	// Set the darkness falloff bias
	virtual void SetFalloffBias( ClientShadowHandle_t handle, unsigned char ucBias );

	void RestoreRenderState();

	// Computes a rough bounding box encompassing the volume of the shadow
	void ComputeShadowBBox( IClientRenderable *pRenderable, const Vector &vecAbsCenter, float flRadius, Vector *pAbsMins, Vector *pAbsMaxs );

	// Returns true if the shadow is far enough to want to use blobby shadows
	bool ShouldUseBlobbyShadows( float flRadius, float flScreenArea );

	bool WillParentRenderBlobbyShadow( IClientRenderable *pRenderable );

	// Are we the child of a shadow with render-to-texture?
	bool ShouldUseParentShadow( IClientRenderable *pRenderable );

	void SetShadowsDisabled( bool bDisabled ) 
	{ 
		r_shadows_gamecontrol.SetValue( bDisabled != 1 );
	}

private:
	enum
	{
		SHADOW_FLAGS_TEXTURE_DIRTY =	(CLIENT_SHADOW_FLAGS_LAST_FLAG << 1),
		SHADOW_FLAGS_BRUSH_MODEL =		(CLIENT_SHADOW_FLAGS_LAST_FLAG << 2), 
		SHADOW_FLAGS_USING_LOD_SHADOW = (CLIENT_SHADOW_FLAGS_LAST_FLAG << 3),
	};

	struct ClientShadow_t
	{
		ClientEntityHandle_t	m_Entity;
		ShadowHandle_t			m_ShadowHandle;
		ClientLeafShadowHandle_t m_ClientLeafShadowHandle;
		unsigned short			m_Flags;
		VMatrix					m_WorldToShadow;
		Vector2D				m_WorldSize;
		Vector					m_LastOrigin;
		QAngle					m_LastAngles;
		TextureHandle_t			m_ShadowTexture;
		CTextureReference		m_ShadowDepthTexture;
		int						m_nRenderFrame;
		EHANDLE					m_hTargetEntity;
		bool					m_bLightWorld;
	};

private:
	// Shadow update functions
	void UpdateStudioShadow( IClientRenderable *pRenderable, ClientShadowHandle_t handle );
	void UpdateBrushShadow( IClientRenderable *pRenderable, ClientShadowHandle_t handle );
	void UpdateShadow( ClientShadowHandle_t handle, bool force );

	// Gets the entity whose shadow this shadow will render into
	IClientRenderable *GetParentShadowEntity( ClientShadowHandle_t handle );

	// Adds the child bounds to the bounding box
	void AddChildBounds( matrix3x4_t &worldToBBox, IClientRenderable* pParent, Vector &vecMins, Vector &vecMaxs );

	// Compute a bounds for the entity + children
	void ComputeHierarchicalBounds( IClientRenderable *pRenderable, Vector &vecMins, Vector &vecMaxs );

	// Builds matrices transforming from world space to shadow space
	void BuildGeneralWorldToShadowMatrix( VMatrix& worldToShadow,
		const Vector& origin, const Vector& dir, const Vector& xvec, const Vector& yvec );
	void BuildOrthoWorldToShadowMatrix( VMatrix& worldToShadow,
		const Vector& origin, const Vector& dir, const Vector& xvec, const Vector& yvec );
	void BuildPerspectiveWorldToFlashlightMatrix( VMatrix& worldToShadow,
		const Vector& origin, const Vector& dir, const Vector& xvec, const Vector& yvec, float fovDegrees,
		float zNear, float zFar );

	// Update a shadow
	void UpdateProjectedTextureInternal( ClientShadowHandle_t handle, bool force );

	// Compute the shadow origin and attenuation start distance
	float ComputeLocalShadowOrigin( IClientRenderable* pRenderable, 
		const Vector& mins, const Vector& maxs, const Vector& localShadowDir, float backupFactor, Vector& origin );

	// Remove a shadow from the dirty list
	void RemoveShadowFromDirtyList( ClientShadowHandle_t handle );

	// NOTE: this will ONLY return SHADOWS_NONE, SHADOWS_SIMPLE, or SHADOW_RENDER_TO_TEXTURE.
	ShadowType_t GetActualShadowCastType( ClientShadowHandle_t handle ) const;
	ShadowType_t GetActualShadowCastType( IClientRenderable *pRenderable ) const;

	// Builds a simple blobby shadow
	void BuildOrthoShadow( IClientRenderable* pRenderable, ClientShadowHandle_t handle, const Vector& mins, const Vector& maxs);

	// Builds a more complex shadow...
	void BuildRenderToTextureShadow( IClientRenderable* pRenderable, 
			ClientShadowHandle_t handle, const Vector& mins, const Vector& maxs );

	// Build a projected-texture flashlight
	void BuildFlashlight( ClientShadowHandle_t handle );

	// Does all the lovely stuff we need to do to have render-to-texture shadows
	void SetupRenderToTextureShadow( ClientShadowHandle_t h );
	void CleanUpRenderToTextureShadow( ClientShadowHandle_t h );

	void CleanUpDepthTextureShadow( ClientShadowHandle_t h );
	
	// Compute the extra shadow planes
	void ComputeExtraClipPlanes( IClientRenderable* pRenderable, 
		ClientShadowHandle_t handle, const Vector* vec, 
		const Vector& mins, const Vector& maxs, const Vector& localShadowDir );

	// Set extra clip planes related to shadows...
	void ClearExtraClipPlanes( ClientShadowHandle_t h );
	void AddExtraClipPlane( ClientShadowHandle_t h, const Vector& normal, float dist );

	// Cull if the origin is on the wrong side of a shadow clip plane....
	bool CullReceiver( ClientShadowHandle_t handle, IClientRenderable* pRenderable, IClientRenderable* pSourceRenderable );

	bool ComputeSeparatingPlane( IClientRenderable* pRend1, IClientRenderable* pRend2, cplane_t* pPlane );

	// Causes all shadows to be re-updated
	void UpdateAllShadows();

	// One of these gets called with every shadow that potentially will need to re-render
	bool DrawRenderToTextureShadow( unsigned short clientShadowHandle, float flArea );
	void DrawRenderToTextureShadowLOD( unsigned short clientShadowHandle );

	// Draws all children shadows into our own
	bool DrawShadowHierarchy( IClientRenderable *pRenderable, const ClientShadow_t &shadow, bool bChild = false );

	// Computes + sets the render-to-texture texcoords
	void SetRenderToTextureShadowTexCoords( ShadowHandle_t handle, int x, int y, int w, int h );

	// Visualization....
	void DrawRenderToTextureDebugInfo( IClientRenderable* pRenderable, const Vector& mins, const Vector& maxs );

	// Advance frame
	void AdvanceFrame();

	// Returns renderable-specific shadow info
	float GetShadowDistance( IClientRenderable *pRenderable ) const;
	const Vector &GetShadowDirection( IClientRenderable *pRenderable ) const;

	// Initialize, shutdown render-to-texture shadows
	void InitDepthTextureShadows();
	void ShutdownDepthTextureShadows();

	// Initialize, shutdown render-to-texture shadows
	void InitRenderToTextureShadows();
	void ShutdownRenderToTextureShadows();

	static bool ShadowHandleCompareFunc( const ClientShadowHandle_t& lhs, const ClientShadowHandle_t& rhs )
	{
		return lhs < rhs;
	}

	ClientShadowHandle_t CreateProjectedTexture( ClientEntityHandle_t entity, int flags );

	// Allocate/deallocate a depth buffer for use by a shadow map
	bool	AllocateDepthBuffer( CTextureReference &depthBuffer );
	void    DeallocateDepthBuffer( CTextureReference &depthBuffer );

	// Set and clear flashlight target renderable
	void	SetFlashlightTarget( ClientShadowHandle_t shadowHandle, EHANDLE targetEntity );

	// Set flashlight light world flag
	void	SetFlashlightLightWorld( ClientShadowHandle_t shadowHandle, bool bLightWorld );

	bool	IsFlashlightTarget( ClientShadowHandle_t shadowHandle, IClientRenderable *pRenderable );

private:
	Vector	m_SimpleShadowDir;
	color32	m_AmbientLightColor;
	CMaterialReference m_SimpleShadow;
	CMaterialReference m_RenderShadow;
	CMaterialReference m_RenderModelShadow;
	CUtlLinkedList< ClientShadow_t, ClientShadowHandle_t >	m_Shadows;
	CTextureAllocator m_ShadowAllocator;
	bool m_RenderToTextureActive;
	bool m_bRenderTargetNeedsClear;
	bool m_bUpdatingDirtyShadows;
	float m_flShadowCastDist;
	float m_flMinShadowArea;
	CUtlRBTree< ClientShadowHandle_t, unsigned short >	m_DirtyShadows;

	bool m_DepthTextureActive;
	CUtlVector< CTextureReference > m_DepthTextureCache;
	int	m_nMaxDepthTextureShadows;

	friend class CVisibleShadowList;
};


//-----------------------------------------------------------------------------
// Singleton
//-----------------------------------------------------------------------------
static CClientShadowMgr s_ClientShadowMgr;
IClientShadowMgr* g_pClientShadowMgr = &s_ClientShadowMgr;


CClientShadowMgr::CClientShadowMgr() :
	m_DirtyShadows( 0, 0, ShadowHandleCompareFunc ),
	m_DepthTextureActive( false )
{
}

//-----------------------------------------------------------------------------
// Changes the shadow direction...
//-----------------------------------------------------------------------------
static void ShadowDir_f()
{
	Vector dir;
	if (engine->Cmd_Argc() == 1)
	{
		Vector dir = s_ClientShadowMgr.GetShadowDirection();
		Msg( "%.2f %.2f %.2f\n", dir.x, dir.y, dir.z );
		return;
	}

	if (engine->Cmd_Argc() == 4)
	{
		dir.x = atof( engine->Cmd_Argv(1) );
		dir.y = atof( engine->Cmd_Argv(2) );
		dir.z = atof( engine->Cmd_Argv(3) );
		s_ClientShadowMgr.SetShadowDirection(dir);
	}
}

static void ShadowAngles_f()
{
	Vector dir;
	QAngle angles;
	if (engine->Cmd_Argc() == 1)
	{
		Vector dir = s_ClientShadowMgr.GetShadowDirection();
		QAngle angles;
		VectorAngles( dir, angles );
		Msg( "%.2f %.2f %.2f\n", angles.x, angles.y, angles.z );
		return;
	}

	if (engine->Cmd_Argc() == 4)
	{
		angles.x = atof( engine->Cmd_Argv(1) );
		angles.y = atof( engine->Cmd_Argv(2) );
		angles.z = atof( engine->Cmd_Argv(3) );
		AngleVectors( angles, &dir );
		s_ClientShadowMgr.SetShadowDirection(dir);
	}
}

static void ShadowColor_f()
{
	if (engine->Cmd_Argc() == 1)
	{
		unsigned char r, g, b;
		s_ClientShadowMgr.GetShadowColor( &r, &g, &b );
		Msg( "Shadow color %d %d %d\n", r, g, b );
		return;
	}

	if (engine->Cmd_Argc() == 4)
	{
		int r = atoi( engine->Cmd_Argv(1) );
		int g = atoi( engine->Cmd_Argv(2) );
		int b = atoi( engine->Cmd_Argv(3) );
		s_ClientShadowMgr.SetShadowColor(r, g, b);
	}
}

static void ShadowDistance_f()
{
	if (engine->Cmd_Argc() == 1)
	{
		float flDist = s_ClientShadowMgr.GetShadowDistance( );
		Msg( "Shadow distance %.2f\n", flDist );
		return;
	}

	if (engine->Cmd_Argc() == 2)
	{
		float flDistance = atof( engine->Cmd_Argv(1) );
		s_ClientShadowMgr.SetShadowDistance( flDistance );
	}
}

static void ShadowBlobbyCutoff_f()
{
	if (engine->Cmd_Argc() == 1)
	{
		float flArea = s_ClientShadowMgr.GetBlobbyCutoffArea( );
		Msg( "Cutoff area %.2f\n", flArea );
		return;
	}

	if (engine->Cmd_Argc() == 2)
	{
		float flArea = atof( engine->Cmd_Argv(1) );
		s_ClientShadowMgr.SetShadowBlobbyCutoffArea( flArea );
	}
}

static ConCommand r_shadowdir("r_shadowdir", ShadowDir_f, "Set shadow direction", FCVAR_CHEAT );
static ConCommand r_shadowangles("r_shadowangles", ShadowAngles_f, "Set shadow angles", FCVAR_CHEAT );
static ConCommand r_shadowcolor("r_shadowcolor", ShadowColor_f, "Set shadow color", FCVAR_CHEAT );
static ConCommand r_shadowdist("r_shadowdist", ShadowDistance_f, "Set shadow distance", FCVAR_CHEAT );
static ConCommand r_shadowblobbycutoff("r_shadowblobbycutoff", ShadowBlobbyCutoff_f, "some shadow stuff", FCVAR_CHEAT );

static void ShadowRestoreFunc( int nChangeFlags )
{
	s_ClientShadowMgr.RestoreRenderState();
}

//-----------------------------------------------------------------------------
// Initialization, shutdown
//-----------------------------------------------------------------------------
bool CClientShadowMgr::Init()
{
	m_bRenderTargetNeedsClear = false;
	m_SimpleShadow.Init( "decals/simpleshadow", TEXTURE_GROUP_DECAL );

	Vector dir( 0.1, 0.1, -1 );
	SetShadowDirection(dir);
	SetShadowDistance( 50 );
#ifndef _XBOX
	SetShadowBlobbyCutoffArea( 0.005 );
#else
	SetShadowBlobbyCutoffArea( 5000 );
#endif

	m_nMaxDepthTextureShadows = 4;

	if ( r_shadowrendertotexture.GetBool() )
	{
		InitRenderToTextureShadows();
	}

#ifdef DOSHADOWEDFLASHLIGHT
	if ( r_flashlightdepthtexture.GetBool() )
	{
		InitDepthTextureShadows();
	}
#endif

	materials->AddRestoreFunc( ShadowRestoreFunc );

	return true;
}

void CClientShadowMgr::Shutdown()
{
	m_SimpleShadow.Shutdown();
	m_Shadows.RemoveAll();
	ShutdownRenderToTextureShadows();

	ShutdownDepthTextureShadows();

	materials->RemoveRestoreFunc( ShadowRestoreFunc );
}


//-----------------------------------------------------------------------------
// Initialize, shutdown depth-texture shadows
//-----------------------------------------------------------------------------
void CClientShadowMgr::InitDepthTextureShadows()
{
#ifdef DOSHADOWEDFLASHLIGHT
	if( !m_DepthTextureActive )
	{
		m_DepthTextureActive = true;

		materials->BeginRenderTargetAllocation();

		m_DepthTextureCache.Purge();
		for( int i=0;i<m_nMaxDepthTextureShadows;i++ )
		{
			CTextureReference depthTex;
			depthTex.InitRenderTarget( r_flashlightdepthres.GetInt(), r_flashlightdepthres.GetInt(), RT_SIZE_DEFAULT, IMAGE_FORMAT_RGBA16161616F, MATERIAL_RT_DEPTH_SEPARATE, false );

			m_DepthTextureCache.AddToTail( depthTex );
		}

		materials->EndRenderTargetAllocation();
	}
#endif
}


void CClientShadowMgr::ShutdownDepthTextureShadows() 
{
	if( m_DepthTextureActive )
	{
		while( m_DepthTextureCache.Count() )
		{
			m_DepthTextureCache[ m_DepthTextureCache.Count()-1 ].Shutdown();
			m_DepthTextureCache.Remove( m_DepthTextureCache.Count()-1 );
		}

		m_DepthTextureActive = false;
	}
}

//-----------------------------------------------------------------------------
// Initialize, shutdown render-to-texture shadows
//-----------------------------------------------------------------------------
void CClientShadowMgr::InitRenderToTextureShadows()
{
	if (!m_RenderToTextureActive)
	{
		m_RenderToTextureActive = true;
		m_RenderShadow.Init( "decals/rendershadow", TEXTURE_GROUP_DECAL );
		m_RenderModelShadow.Init( "decals/rendermodelshadow", TEXTURE_GROUP_DECAL );
		m_ShadowAllocator.Init();

		m_ShadowAllocator.Reset();
		m_bRenderTargetNeedsClear = true;

		float fr = (float)m_AmbientLightColor.r / 255.0f;
		float fg = (float)m_AmbientLightColor.g / 255.0f;
		float fb = (float)m_AmbientLightColor.b / 255.0f;
		m_RenderShadow->ColorModulate( fr, fg, fb );
		m_RenderModelShadow->ColorModulate( fr, fg, fb );

		// Iterate over all existing textures and allocate shadow textures
		for (ClientShadowHandle_t i = m_Shadows.Head(); i != m_Shadows.InvalidIndex(); i = m_Shadows.Next(i) )
		{
			ClientShadow_t& shadow = m_Shadows[i];
			if ( shadow.m_Flags & SHADOW_FLAGS_USE_RENDER_TO_TEXTURE )
			{
				SetupRenderToTextureShadow( i );
				MarkRenderToTextureShadowDirty( i );

				// Switch the material to use render-to-texture shadows
				shadowmgr->SetShadowMaterial( shadow.m_ShadowHandle, m_RenderShadow, m_RenderModelShadow, (void*)i );
			}
		}
	}
}

void CClientShadowMgr::ShutdownRenderToTextureShadows()
{
	if (m_RenderToTextureActive)
	{
		// Iterate over all existing textures and deallocate shadow textures
		for (ClientShadowHandle_t i = m_Shadows.Head(); i != m_Shadows.InvalidIndex(); i = m_Shadows.Next(i) )
		{
			CleanUpRenderToTextureShadow( i );

			// Switch the material to use blobby shadows
			ClientShadow_t& shadow = m_Shadows[i];
			shadowmgr->SetShadowMaterial( shadow.m_ShadowHandle, m_SimpleShadow, m_SimpleShadow, (void*)CLIENTSHADOW_INVALID_HANDLE );
			shadowmgr->SetShadowTexCoord( shadow.m_ShadowHandle, 0, 0, 1, 1 );
			ClearExtraClipPlanes( i );
		}

		m_RenderShadow.Shutdown();
		m_RenderModelShadow.Shutdown();

		m_ShadowAllocator.DeallocateAllTextures();
		m_ShadowAllocator.Shutdown();

		// Cause the render target to go away
		materials->UncacheUnusedMaterials();

		m_RenderToTextureActive = false;
	}
}


//-----------------------------------------------------------------------------
// Sets the shadow color
//-----------------------------------------------------------------------------
void CClientShadowMgr::SetShadowColor( unsigned char r, unsigned char g, unsigned char b )
{
	float fr = (float)r / 255.0f;
	float fg = (float)g / 255.0f;
	float fb = (float)b / 255.0f;

	// Hook the shadow color into the shadow materials
	m_SimpleShadow->ColorModulate( fr, fg, fb );

	if (m_RenderToTextureActive)
	{
		m_RenderShadow->ColorModulate( fr, fg, fb );
		m_RenderModelShadow->ColorModulate( fr, fg, fb );
	}

	m_AmbientLightColor.r = r;
	m_AmbientLightColor.g = g;
	m_AmbientLightColor.b = b;
}

void CClientShadowMgr::GetShadowColor( unsigned char *r, unsigned char *g, unsigned char *b ) const
{
	*r = m_AmbientLightColor.r;
	*g = m_AmbientLightColor.g;
	*b = m_AmbientLightColor.b;
}


//-----------------------------------------------------------------------------
// Level init... get the shadow color
//-----------------------------------------------------------------------------
void CClientShadowMgr::LevelInitPreEntity()
{
	m_bUpdatingDirtyShadows = false;

	Vector ambientColor;
	engine->GetAmbientLightColor( ambientColor );
	ambientColor *= 3;
	ambientColor += Vector( 0.3f, 0.3f, 0.3f );

	unsigned char r = ambientColor[0] > 1.0 ? 255 : 255 * ambientColor[0];
	unsigned char g = ambientColor[1] > 1.0 ? 255 : 255 * ambientColor[1];
	unsigned char b = ambientColor[2] > 1.0 ? 255 : 255 * ambientColor[2];

	SetShadowColor(r, g, b);

	// Set up the texture allocator
	if (m_RenderToTextureActive)
	{
		m_ShadowAllocator.Reset();
		m_bRenderTargetNeedsClear = true;
	}
}


//-----------------------------------------------------------------------------
// Clean up all shadows
//-----------------------------------------------------------------------------
void CClientShadowMgr::LevelShutdownPostEntity()
{
	// All shadows *should* have been cleaned up when the entities went away
	// but, just in case....
	Assert( m_Shadows.Count() == 0 );

	ClientShadowHandle_t h = m_Shadows.Head();
	while (h != CLIENTSHADOW_INVALID_HANDLE)
	{
		ClientShadowHandle_t next = m_Shadows.Next(h);
		DestroyShadow( h );
		h = next;
	}

	// Deallocate all textures
	if (m_RenderToTextureActive)
	{
		m_ShadowAllocator.DeallocateAllTextures();
	}

	r_shadows_gamecontrol.SetValue( -1 );
}


//-----------------------------------------------------------------------------
// Deals with alt-tab
//-----------------------------------------------------------------------------
void CClientShadowMgr::RestoreRenderState()
{
	// Mark all shadows dirty; they need to regenerate their state
	ClientShadowHandle_t h;
	for ( h = m_Shadows.Head(); h != m_Shadows.InvalidIndex(); h = m_Shadows.Next(h) )
	{
		m_Shadows[h].m_Flags |= SHADOW_FLAGS_TEXTURE_DIRTY;
	}

	SetShadowColor(m_AmbientLightColor.r, m_AmbientLightColor.g, m_AmbientLightColor.b);
	m_bRenderTargetNeedsClear = true;
}


//-----------------------------------------------------------------------------
// Does all the lovely stuff we need to do to have render-to-texture shadows
//-----------------------------------------------------------------------------
void CClientShadowMgr::SetupRenderToTextureShadow( ClientShadowHandle_t h )
{
	// First, compute how much texture memory we want to use.
	ClientShadow_t& shadow = m_Shadows[h];
	
	IClientRenderable *pRenderable = ClientEntityList().GetClientRenderableFromHandle( shadow.m_Entity );
	if ( !pRenderable )
		return;

	Vector mins, maxs;
	pRenderable->GetShadowRenderBounds( mins, maxs, GetActualShadowCastType( h ) );

	// Compute the maximum dimension
	Vector size;
	VectorSubtract( maxs, mins, size );
	float maxSize = max( size.x, size.y );
	maxSize = max( maxSize, size.z );

	// Figure out the texture size
	// For now, we're going to assume a fixed number of shadow texels
	// per shadow-caster size; add in some extra space at the boundary.
	int texelCount = TEXEL_SIZE_PER_CASTER_SIZE * maxSize;
	
	// Pick the first power of 2 larger...
	int textureSize = 1;
	while (textureSize < texelCount)
	{
		textureSize <<= 1;
	}

	shadow.m_ShadowTexture = m_ShadowAllocator.AllocateTexture( textureSize, textureSize );
}


void CClientShadowMgr::CleanUpRenderToTextureShadow( ClientShadowHandle_t h )
{
	ClientShadow_t& shadow = m_Shadows[h];
	if (m_RenderToTextureActive && (shadow.m_Flags & SHADOW_FLAGS_USE_RENDER_TO_TEXTURE))
	{
		m_ShadowAllocator.DeallocateTexture( shadow.m_ShadowTexture );
		shadow.m_ShadowTexture = INVALID_TEXTURE_HANDLE;
	}
}


void CClientShadowMgr::CleanUpDepthTextureShadow( ClientShadowHandle_t h )
{
	ClientShadow_t& shadow = m_Shadows[h];
	if( m_DepthTextureActive && (shadow.m_Flags & SHADOW_FLAGS_USE_DEPTH_TEXTURE ) )
	{
		DeallocateDepthBuffer( shadow.m_ShadowDepthTexture );
	}
}


//-----------------------------------------------------------------------------
// Causes all shadows to be re-updated
//-----------------------------------------------------------------------------
void CClientShadowMgr::UpdateAllShadows()
{
	m_bUpdatingDirtyShadows = true;

	for (ClientShadowHandle_t i = m_Shadows.Head(); i != m_Shadows.InvalidIndex(); i = m_Shadows.Next(i) )
	{
		UpdateProjectedTextureInternal( i, true );
	}
	m_DirtyShadows.RemoveAll();

	m_bUpdatingDirtyShadows = false;
}


//-----------------------------------------------------------------------------
// Sets the shadow direction
//-----------------------------------------------------------------------------
void CClientShadowMgr::SetShadowDirection( const Vector& dir )
{
	VectorCopy( dir, m_SimpleShadowDir );
	VectorNormalize( m_SimpleShadowDir );

	if ( m_RenderToTextureActive )
	{
		UpdateAllShadows();
	}
}

const Vector &CClientShadowMgr::GetShadowDirection() const
{
	// This will cause blobby shadows to always project straight down
	static Vector s_vecDown( 0, 0, -1 );
	if ( !m_RenderToTextureActive )
		return s_vecDown;

	return m_SimpleShadowDir;
}


//-----------------------------------------------------------------------------
// Gets shadow information for a particular renderable
//-----------------------------------------------------------------------------
float CClientShadowMgr::GetShadowDistance( IClientRenderable *pRenderable ) const
{
	float flDist = m_flShadowCastDist;

	// Allow the renderable to override the default
	pRenderable->GetShadowCastDistance( &flDist, GetActualShadowCastType( pRenderable ) );

	return flDist;
}

const Vector &CClientShadowMgr::GetShadowDirection( IClientRenderable *pRenderable ) const
{
	Vector &vecResult = AllocTempVector();
	vecResult = GetShadowDirection();

	// Allow the renderable to override the default
	pRenderable->GetShadowCastDirection( &vecResult, GetActualShadowCastType( pRenderable ) );

	return vecResult;
}


//-----------------------------------------------------------------------------
// Sets the shadow distance
//-----------------------------------------------------------------------------
void CClientShadowMgr::SetShadowDistance( float flMaxDistance )
{
	m_flShadowCastDist = flMaxDistance;
	UpdateAllShadows();
}

float CClientShadowMgr::GetShadowDistance( ) const
{
	return m_flShadowCastDist;
}


//-----------------------------------------------------------------------------
// Sets the screen area at which blobby shadows are always used
//-----------------------------------------------------------------------------
void CClientShadowMgr::SetShadowBlobbyCutoffArea( float flMinArea )
{
	m_flMinShadowArea = flMinArea;
}

float CClientShadowMgr::GetBlobbyCutoffArea( ) const
{
	return m_flMinShadowArea;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClientShadowMgr::SetFalloffBias( ClientShadowHandle_t handle, unsigned char ucBias )
{
	shadowmgr->SetFalloffBias( m_Shadows[handle].m_ShadowHandle, ucBias );
}

//-----------------------------------------------------------------------------
// Returns the shadow texture
//-----------------------------------------------------------------------------
ITexture* CClientShadowMgr::GetShadowTexture( unsigned short h )
{
	return m_ShadowAllocator.GetTexture();
}


//-----------------------------------------------------------------------------
// Returns information needed by the model proxy
//-----------------------------------------------------------------------------
const ShadowInfo_t& CClientShadowMgr::GetShadowInfo( ClientShadowHandle_t h )
{
	return shadowmgr->GetInfo( m_Shadows[h].m_ShadowHandle );
}


//-----------------------------------------------------------------------------
// Renders the shadow texture to screen...
//-----------------------------------------------------------------------------
void CClientShadowMgr::RenderShadowTexture( int w, int h )
{
	if (m_RenderToTextureActive)
	{
		float flTexWidth, flTexHeight;
#ifdef _XBOX
		// xboxissue - need non-normalized texture coords
		int textureW, textureH;
		m_ShadowAllocator.GetTotalTextureSize( textureW, textureH );
		flTexWidth = textureW;
		flTexHeight = textureH;
#else
		flTexWidth = 1.0f;
		flTexHeight = 1.0f;
#endif
		materials->Bind( m_RenderShadow );
		IMesh* pMesh = materials->GetDynamicMesh( true );

		CMeshBuilder meshBuilder;
		meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

		meshBuilder.Position3f( 0.0f, 0.0f, 0.0f );
		meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
		meshBuilder.Color4ub( 0, 0, 0, 0 );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( w, 0.0f, 0.0f );
		meshBuilder.TexCoord2f( 0, flTexWidth, 0.0f );
		meshBuilder.Color4ub( 0, 0, 0, 0 );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( w, h, 0.0f );
		meshBuilder.TexCoord2f( 0, flTexWidth, flTexHeight );
		meshBuilder.Color4ub( 0, 0, 0, 0 );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( 0.0f, h, 0.0f );
		meshBuilder.TexCoord2f( 0, 0.0f, flTexHeight );
		meshBuilder.Color4ub( 0, 0, 0, 0 );
		meshBuilder.AdvanceVertex();

		meshBuilder.End();
		pMesh->Draw();
	}
}


//-----------------------------------------------------------------------------
// Create/destroy a shadow
//-----------------------------------------------------------------------------
ClientShadowHandle_t CClientShadowMgr::CreateProjectedTexture( ClientEntityHandle_t entity, int flags )
{
	// We need to know if it's a brush model for shadows
	if( !( flags & SHADOW_FLAGS_FLASHLIGHT ) )
	{
		IClientRenderable *pRenderable = ClientEntityList().GetClientRenderableFromHandle( entity );
		int modelType = modelinfo->GetModelType( pRenderable->GetModel() );
		if (modelType == mod_brush)
		{
			flags |= SHADOW_FLAGS_BRUSH_MODEL;
		}
	}

	ClientShadowHandle_t h = m_Shadows.AddToTail();
	ClientShadow_t& shadow = m_Shadows[h];
	shadow.m_Entity = entity;
	shadow.m_ClientLeafShadowHandle = ClientLeafSystem()->AddShadow( h, flags );
	shadow.m_Flags = flags;
	shadow.m_nRenderFrame = -1;
	shadow.m_LastOrigin.Init( FLT_MAX, FLT_MAX, FLT_MAX );
	shadow.m_LastAngles.Init( FLT_MAX, FLT_MAX, FLT_MAX );
	Assert( ( ( shadow.m_Flags & SHADOW_FLAGS_FLASHLIGHT ) == 0 ) != 
			( ( shadow.m_Flags & SHADOW_FLAGS_SHADOW ) == 0 ) );

	// Set up the flags....
	IMaterial* pShadowMaterial = m_SimpleShadow;
	IMaterial* pShadowModelMaterial = m_SimpleShadow;
	void* pShadowProxyData = (void*)CLIENTSHADOW_INVALID_HANDLE;

	if ( m_RenderToTextureActive && (flags & SHADOW_FLAGS_USE_RENDER_TO_TEXTURE) )
	{
		SetupRenderToTextureShadow(h);

		pShadowMaterial = m_RenderShadow;
		pShadowModelMaterial = m_RenderModelShadow;
		pShadowProxyData = (void*)h;
	}

	if( flags & SHADOW_FLAGS_USE_DEPTH_TEXTURE )
	{
		// This should be changed to handle the error more gracefully
		AllocateDepthBuffer( shadow.m_ShadowDepthTexture );

		pShadowMaterial = m_RenderShadow;
		pShadowModelMaterial = m_RenderModelShadow;
		pShadowProxyData = (void*)h;
	}

	int createShadowFlags;
	if( flags & SHADOW_FLAGS_FLASHLIGHT )
	{
		// don't use SHADOW_CACHE_VERTS with projective lightsources since we expect that they will change every frame.
		// FIXME: might want to make it cache optionally if it's an entity light that is static.
		createShadowFlags = SHADOW_FLASHLIGHT;
	}
	else
	{
		createShadowFlags = SHADOW_CACHE_VERTS;
	}
	shadow.m_ShadowHandle = shadowmgr->CreateShadowEx( pShadowMaterial, pShadowModelMaterial, pShadowProxyData, createShadowFlags, shadow.m_ShadowDepthTexture );
	return h;
}

ClientShadowHandle_t CClientShadowMgr::CreateFlashlight( const FlashlightState_t &lightState )
{
	// We don't really need a model entity handle for a projective light source, so use an invalid one.
	static ClientEntityHandle_t invalidHandle = INVALID_CLIENTENTITY_HANDLE;

	int shadowFlags = SHADOW_FLAGS_FLASHLIGHT;
#ifdef DOSHADOWEDFLASHLIGHT
	if( lightState.m_bEnableShadows && r_flashlightdepthtexture.GetBool() )
		shadowFlags |= SHADOW_FLAGS_USE_DEPTH_TEXTURE;
#endif
	ClientShadowHandle_t shadowHandle = CreateProjectedTexture( invalidHandle, shadowFlags );

	m_Shadows[ shadowHandle ].m_bLightWorld = true;

	UpdateFlashlightState( shadowHandle, lightState );
	UpdateProjectedTexture( shadowHandle, true );
	return shadowHandle;
}

ClientShadowHandle_t CClientShadowMgr::CreateShadow( ClientEntityHandle_t entity, int flags )
{
	// We don't really need a model entity handle for a projective light source, so use an invalid one.
	flags &= ~SHADOW_FLAGS_PROJECTED_TEXTURE_TYPE_MASK;
	flags |= SHADOW_FLAGS_SHADOW | SHADOW_FLAGS_TEXTURE_DIRTY;
	ClientShadowHandle_t shadowHandle = CreateProjectedTexture( entity, flags );

	IClientRenderable *pRenderable = ClientEntityList().GetClientRenderableFromHandle( entity );
	if ( pRenderable )
	{
		Assert( !pRenderable->IsShadowDirty( ) );
		pRenderable->MarkShadowDirty( true );
	}

	// NOTE: We *have * to call the version that takes a shadow handle
	// even if we have an entity because this entity hasn't set its shadow handle yet
	AddToDirtyShadowList( shadowHandle, true );
	return shadowHandle;
}


//-----------------------------------------------------------------------------
// Updates the flashlight direction and re-computes surfaces it should lie on
//-----------------------------------------------------------------------------
void CClientShadowMgr::UpdateFlashlightState( ClientShadowHandle_t shadowHandle, const FlashlightState_t &flashlightState )
{
	Vector lightXVec;
	Vector lightYVec( 0.0f, 0.0f, 1.0f );
	if( fabs( DotProduct( lightYVec, flashlightState.m_vecLightDirection ) ) > 0.9f )
	{
		// Don't want lightYVec and m_vecLightDirection to be parallel
		lightYVec.Init( 0.0f, 1.0f, 0.0f );
	}
	CrossProduct( lightYVec, flashlightState.m_vecLightDirection, lightXVec );
	VectorNormalize( lightXVec );
	CrossProduct( flashlightState.m_vecLightDirection, lightXVec, lightYVec );
	VectorNormalize( lightYVec );

	BuildPerspectiveWorldToFlashlightMatrix( m_Shadows[shadowHandle].m_WorldToShadow, flashlightState.m_vecLightOrigin, flashlightState.m_vecLightDirection, 
		lightXVec, lightYVec, flashlightState.m_fVerticalFOVDegrees, flashlightState.m_NearZ, flashlightState.m_FarZ );
											
	shadowmgr->UpdateFlashlightStateEx( m_Shadows[shadowHandle].m_ShadowHandle, flashlightState, m_Shadows[shadowHandle].m_ShadowDepthTexture );
}

void CClientShadowMgr::DestroyFlashlight( ClientShadowHandle_t shadowHandle )
{
	DestroyShadow( shadowHandle );
}

//-----------------------------------------------------------------------------
// Remove a shadow from the dirty list
//-----------------------------------------------------------------------------
void CClientShadowMgr::RemoveShadowFromDirtyList( ClientShadowHandle_t handle )
{
	int idx = m_DirtyShadows.Find( handle );
	if ( idx != m_DirtyShadows.InvalidIndex() )
	{
		// Clean up the shadow update bit.
		IClientRenderable *pRenderable = ClientEntityList().GetClientRenderableFromHandle( m_Shadows[handle].m_Entity );
		if ( pRenderable )
		{
			pRenderable->MarkShadowDirty( false );
		}
		m_DirtyShadows.RemoveAt( idx );
	}
}


//-----------------------------------------------------------------------------
// Remove a shadow
//-----------------------------------------------------------------------------
void CClientShadowMgr::DestroyShadow( ClientShadowHandle_t handle )
{
	Assert( m_Shadows.IsValidIndex(handle) );
	RemoveShadowFromDirtyList( handle );
	shadowmgr->DestroyShadow( m_Shadows[handle].m_ShadowHandle );
	ClientLeafSystem()->RemoveShadow( m_Shadows[handle].m_ClientLeafShadowHandle );
	CleanUpRenderToTextureShadow( handle );
	CleanUpDepthTextureShadow( handle );
	m_Shadows.Remove(handle);
}


//-----------------------------------------------------------------------------
// Build the worldtotexture matrix
//-----------------------------------------------------------------------------
void CClientShadowMgr::BuildGeneralWorldToShadowMatrix( VMatrix& worldToShadow,
	const Vector& origin, const Vector& dir, const Vector& xvec, const Vector& yvec )
{
	// We're assuming here that xvec + yvec aren't necessary perpendicular

	// The shadow->world matrix is pretty simple:
	// Just stick the origin in the translation component
	// and the vectors in the columns...
	worldToShadow.SetBasisVectors( xvec, yvec, dir );
	worldToShadow.SetTranslation( origin );
	worldToShadow[3][0] = worldToShadow[3][1] = worldToShadow[3][2] = 0.0f;
	worldToShadow[3][3] = 1.0f;

	// Now do a general inverse to get worldToShadow
	MatrixInverseGeneral( worldToShadow, worldToShadow );
}

void CClientShadowMgr::BuildOrthoWorldToShadowMatrix( VMatrix& worldToShadow,
	const Vector& origin, const Vector& dir, const Vector& xvec, const Vector& yvec )
{
	// This version is faster and assumes dir, xvec, yvec are perpendicular
	AssertFloatEquals( DotProduct( dir, xvec ), 0.0f, 1e-3 );
	AssertFloatEquals( DotProduct( dir, yvec ), 0.0f, 1e-3 );
	AssertFloatEquals( DotProduct( xvec, yvec ), 0.0f, 1e-3 );

	// The shadow->world matrix is pretty simple:
	// Just stick the origin in the translation component
	// and the vectors in the columns...
	// The inverse of this transposes the rotational component
	// and the translational component =  - (rotation transpose) * origin
	worldToShadow.SetBasisVectors( xvec, yvec, dir );
	MatrixTranspose( worldToShadow, worldToShadow );

	Vector translation;
	Vector3DMultiply( worldToShadow, origin, translation );

	translation *= -1.0f;
	worldToShadow.SetTranslation( translation );

	// The the bottom row.
	worldToShadow[3][0] = worldToShadow[3][1] = worldToShadow[3][2] = 0.0f;
	worldToShadow[3][3] = 1.0f;
}

void CClientShadowMgr::BuildPerspectiveWorldToFlashlightMatrix( VMatrix& worldToShadow,
	const Vector& origin, const Vector& dir, const Vector& xvec, const Vector& yvec, float fovDegrees,
	float zNear, float zFar )
{
	// build the ortho shadow matrix to get us from world to shadow space.  We'll build the perspective
	// part separately and concatenate.
	VMatrix worldToShadowView;
	BuildOrthoWorldToShadowMatrix( worldToShadowView, origin, dir, xvec, yvec );

	VMatrix perspective;
	MatrixBuildPerspective( perspective, fovDegrees, fovDegrees, zNear, zFar );

	MatrixMultiply( perspective, worldToShadowView, worldToShadow );
}

//-----------------------------------------------------------------------------
// Compute the shadow origin and attenuation start distance
//-----------------------------------------------------------------------------
float CClientShadowMgr::ComputeLocalShadowOrigin( IClientRenderable* pRenderable, 
	const Vector& mins, const Vector& maxs, const Vector& localShadowDir, float backupFactor, Vector& origin )
{
	// Compute the centroid of the object...
	Vector vecCentroid;
	VectorAdd( mins, maxs, vecCentroid );
	vecCentroid *= 0.5f;

	Vector vecSize;
	VectorSubtract( maxs, mins, vecSize );
	float flRadius = vecSize.Length() * 0.5f;

	// NOTE: The *origin* of the shadow cast is a point on a line passing through
	// the centroid of the caster. The direction of this line is the shadow cast direction,
	// and the point on that line corresponds to the endpoint of the box that is
	// furthest *back* along the shadow direction

	// For the first point at which the shadow could possibly start falling off,
	// we need to use the point at which the ray described above leaves the
	// bounding sphere surrounding the entity. This is necessary because otherwise,
	// tall, thin objects would have their shadows appear + disappear as then spun about their origin

	// Figure out the corner corresponding to the min + max projection
	// along the shadow direction

	// We're basically finding the point on the cube that has the largest and smallest
	// dot product with the local shadow dir. Then we're taking the dot product
	// of that with the localShadowDir. lastly, we're subtracting out the
	// centroid projection to give us a distance along the localShadowDir to
	// the front and back of the cube along the direction of the ray.
	float centroidProjection = DotProduct( vecCentroid, localShadowDir );
	float minDist = -centroidProjection;
	for (int i = 0; i < 3; ++i)
	{
		if ( localShadowDir[i] > 0.0f )
		{
			minDist += localShadowDir[i] * mins[i];
		}
		else
		{
			minDist += localShadowDir[i] * maxs[i];
		}
	}

	minDist *= backupFactor;

	VectorMA( vecCentroid, minDist, localShadowDir, origin );

	return flRadius - minDist;
}


//-----------------------------------------------------------------------------
// Sorts the components of a vector
//-----------------------------------------------------------------------------
static inline void SortAbsVectorComponents( const Vector& src, int* pVecIdx )
{
	Vector absVec( fabs(src[0]), fabs(src[1]), fabs(src[2]) );

	int maxIdx = (absVec[0] > absVec[1]) ? 0 : 1;
	if (absVec[2] > absVec[maxIdx])
	{
		maxIdx = 2;
	}

	// always choose something right-handed....
	switch(	maxIdx )
	{
	case 0:
		pVecIdx[0] = 1;
		pVecIdx[1] = 2;
		pVecIdx[2] = 0;
		break;
	case 1:
		pVecIdx[0] = 2;
		pVecIdx[1] = 0;
		pVecIdx[2] = 1;
		break;
	case 2:
		pVecIdx[0] = 0;
		pVecIdx[1] = 1;
		pVecIdx[2] = 2;
		break;
	}
}


//-----------------------------------------------------------------------------
// Build the worldtotexture matrix
//-----------------------------------------------------------------------------
static void BuildWorldToTextureMatrix( const VMatrix& worldToShadow, 
							const Vector2D& size, VMatrix& worldToTexture )
{
	// Build a matrix that maps from shadow space to (u,v) coordinates
	VMatrix shadowToUnit;
	MatrixBuildScale( shadowToUnit, 1.0f / size.x, 1.0f / size.y, 1.0f );
	shadowToUnit[0][3] = shadowToUnit[1][3] = 0.5f;

	// Store off the world to (u,v) transformation
	MatrixMultiply( shadowToUnit, worldToShadow, worldToTexture );
}


//-----------------------------------------------------------------------------
// Set extra clip planes related to shadows...
//-----------------------------------------------------------------------------
void CClientShadowMgr::ClearExtraClipPlanes( ClientShadowHandle_t h )
{
	shadowmgr->ClearExtraClipPlanes( m_Shadows[h].m_ShadowHandle );
}

void CClientShadowMgr::AddExtraClipPlane( ClientShadowHandle_t h, const Vector& normal, float dist )
{
	shadowmgr->AddExtraClipPlane( m_Shadows[h].m_ShadowHandle, normal, dist );
}


//-----------------------------------------------------------------------------
// Compute the extra shadow planes
//-----------------------------------------------------------------------------
void CClientShadowMgr::ComputeExtraClipPlanes( IClientRenderable* pRenderable, 
	ClientShadowHandle_t handle, const Vector* vec, 
	const Vector& mins, const Vector& maxs, const Vector& localShadowDir )
{
	// Compute the world-space position of the corner of the bounding box
	// that's got the highest dotproduct with the local shadow dir...
	Vector origin = pRenderable->GetRenderOrigin( );
	float dir[3];

	int i;
	for ( i = 0; i < 3; ++i )
	{
		if (localShadowDir[i] < 0.0f)
		{
			VectorMA( origin, maxs[i], vec[i], origin );
			dir[i] = 1;
		}
		else
		{
			VectorMA( origin, mins[i], vec[i], origin );
			dir[i] = -1;
		}
	}

	// Now that we have it, create 3 planes...
	Vector normal;
	ClearExtraClipPlanes(handle);
	for ( i = 0; i < 3; ++i )
	{
		VectorMultiply( vec[i], dir[i], normal );
		float dist = DotProduct( normal, origin );
		AddExtraClipPlane( handle, normal, dist );
	}
}


inline ShadowType_t CClientShadowMgr::GetActualShadowCastType( ClientShadowHandle_t handle ) const
{
	if ( handle == CLIENTSHADOW_INVALID_HANDLE )
	{
		return SHADOWS_NONE;
	}
	
	if ( m_Shadows[handle].m_Flags & SHADOW_FLAGS_USE_RENDER_TO_TEXTURE )
	{
		return ( m_RenderToTextureActive ? SHADOWS_RENDER_TO_TEXTURE : SHADOWS_SIMPLE );
	}
	else if( m_Shadows[handle].m_Flags & SHADOW_FLAGS_USE_DEPTH_TEXTURE )
	{
		return SHADOWS_RENDER_TO_DEPTH_TEXTURE;
	}
	else
	{
		return SHADOWS_SIMPLE;
	}
}

inline ShadowType_t CClientShadowMgr::GetActualShadowCastType( IClientRenderable *pEnt ) const
{
	return GetActualShadowCastType( pEnt->GetShadowHandle() );
}


//-----------------------------------------------------------------------------
// Builds a simple blobby shadow
//-----------------------------------------------------------------------------
void CClientShadowMgr::BuildOrthoShadow( IClientRenderable* pRenderable, 
		ClientShadowHandle_t handle, const Vector& mins, const Vector& maxs)
{
	// Get the object's basis
	Vector vec[3];
	AngleVectors( pRenderable->GetRenderAngles(), &vec[0], &vec[1], &vec[2] );
	vec[1] *= -1.0f;

	Vector vecShadowDir = GetShadowDirection( pRenderable );

	// Project the shadow casting direction into the space of the object
	Vector localShadowDir;
	localShadowDir[0] = DotProduct( vec[0], vecShadowDir );
	localShadowDir[1] = DotProduct( vec[1], vecShadowDir );
	localShadowDir[2] = DotProduct( vec[2], vecShadowDir );

	// Figure out which vector has the largest component perpendicular
	// to the shadow handle...
	// Sort by how perpendicular it is
	int vecIdx[3];
	SortAbsVectorComponents( localShadowDir, vecIdx );

	// Here's our shadow basis vectors; namely the ones that are
	// most perpendicular to the shadow casting direction
	Vector xvec = vec[vecIdx[0]];
	Vector yvec = vec[vecIdx[1]];

	// Project them into a plane perpendicular to the shadow direction
	xvec -= vecShadowDir * DotProduct( vecShadowDir, xvec );
	yvec -= vecShadowDir * DotProduct( vecShadowDir, yvec );
	VectorNormalize( xvec );
	VectorNormalize( yvec );

	// Compute the box size
	Vector boxSize;
	VectorSubtract( maxs, mins, boxSize );

	// We project the two longest sides into the vectors perpendicular
	// to the projection direction, then add in the projection of the perp direction
	Vector2D size( boxSize[vecIdx[0]], boxSize[vecIdx[1]] );
	size.x *= fabs( DotProduct( vec[vecIdx[0]], xvec ) );
	size.y *= fabs( DotProduct( vec[vecIdx[1]], yvec ) );

	// Add the third component into x and y
	size.x += boxSize[vecIdx[2]] * fabs( DotProduct( vec[vecIdx[2]], xvec ) );
	size.y += boxSize[vecIdx[2]] * fabs( DotProduct( vec[vecIdx[2]], yvec ) );

	// Bloat a bit, since the shadow wants to extend outside the model a bit
	size.x += 10.0f;
	size.y += 10.0f;

	// Clamp the minimum size
	Vector2DMax( size, Vector2D(10.0f, 10.0f), size );

	// Place the origin at the point with min dot product with shadow dir
	Vector org;
	float falloffStart = ComputeLocalShadowOrigin( pRenderable, mins, maxs, localShadowDir, 2.0f, org );

	// Transform the local origin into world coordinates
	Vector worldOrigin = pRenderable->GetRenderOrigin( );
	VectorMA( worldOrigin, org.x, vec[0], worldOrigin );
	VectorMA( worldOrigin, org.y, vec[1], worldOrigin );
	VectorMA( worldOrigin, org.z, vec[2], worldOrigin );

	// FUNKY: A trick to reduce annoying texelization artifacts!?
	float dx = 1.0f / TEXEL_SIZE_PER_CASTER_SIZE;
	worldOrigin.x = (int)(worldOrigin.x / dx) * dx;
	worldOrigin.y = (int)(worldOrigin.y / dx) * dx;
	worldOrigin.z = (int)(worldOrigin.z / dx) * dx;

	// NOTE: We gotta use the general matrix because xvec and yvec aren't perp
	VMatrix worldToShadow, worldToTexture;
	BuildGeneralWorldToShadowMatrix( m_Shadows[handle].m_WorldToShadow, worldOrigin, vecShadowDir, xvec, yvec );
	BuildWorldToTextureMatrix( m_Shadows[handle].m_WorldToShadow, size, worldToTexture );
	Vector2DCopy( size, m_Shadows[handle].m_WorldSize );
	
	// Compute the falloff attenuation
	// Area computation isn't exact since xvec is not perp to yvec, but close enough
//	float shadowArea = size.x * size.y;	

	// The entity may be overriding our shadow cast distance
	float flShadowCastDistance = GetShadowDistance( pRenderable );
	float maxHeight = flShadowCastDistance + falloffStart; //3.0f * sqrt( shadowArea );

	shadowmgr->ProjectShadow( m_Shadows[handle].m_ShadowHandle, worldOrigin, 
		vecShadowDir, worldToTexture, size, maxHeight, falloffStart, MAX_FALLOFF_AMOUNT, pRenderable->GetRenderOrigin() );

	// Compute extra clip planes to prevent poke-thru
// FIXME!!!!!!!!!!!!!!  Removing this for now since it seems to mess up the blobby shadows.
//	ComputeExtraClipPlanes( pEnt, handle, vec, mins, maxs, localShadowDir );

	// Add the shadow to the client leaf system so it correctly marks 
	// leafs as being affected by a particular shadow
	ClientLeafSystem()->ProjectShadow( m_Shadows[handle].m_ClientLeafShadowHandle, 
		worldOrigin, vecShadowDir, size, maxHeight );
}


//-----------------------------------------------------------------------------
// Visualization....
//-----------------------------------------------------------------------------
void CClientShadowMgr::DrawRenderToTextureDebugInfo( IClientRenderable* pRenderable, const Vector& mins, const Vector& maxs )
{   
	// Get the object's basis
	Vector vec[3];
	AngleVectors( pRenderable->GetRenderAngles(), &vec[0], &vec[1], &vec[2] );
	vec[1] *= -1.0f;

	Vector vecSize;
	VectorSubtract( maxs, mins, vecSize );

	Vector vecOrigin = pRenderable->GetRenderOrigin();
	Vector start, end, end2;

	VectorMA( vecOrigin, mins.x, vec[0], start );
	VectorMA( start, mins.y, vec[1], start );
	VectorMA( start, mins.z, vec[2], start );

	VectorMA( start, vecSize.x, vec[0], end );
	VectorMA( end, vecSize.z, vec[2], end2 );
	debugoverlay->AddLineOverlay( start, end, 255, 0, 0, true, 0.01 ); 
	debugoverlay->AddLineOverlay( end2, end, 255, 0, 0, true, 0.01 ); 

	VectorMA( start, vecSize.y, vec[1], end );
	VectorMA( end, vecSize.z, vec[2], end2 );
	debugoverlay->AddLineOverlay( start, end, 255, 0, 0, true, 0.01 ); 
	debugoverlay->AddLineOverlay( end2, end, 255, 0, 0, true, 0.01 ); 

	VectorMA( start, vecSize.z, vec[2], end );
	debugoverlay->AddLineOverlay( start, end, 255, 0, 0, true, 0.01 );
	
	start = end;
	VectorMA( start, vecSize.x, vec[0], end );
	debugoverlay->AddLineOverlay( start, end, 255, 0, 0, true, 0.01 ); 

	VectorMA( start, vecSize.y, vec[1], end );
	debugoverlay->AddLineOverlay( start, end, 255, 0, 0, true, 0.01 ); 

	VectorMA( end, vecSize.x, vec[0], start );
	VectorMA( start, -vecSize.x, vec[0], end );
	debugoverlay->AddLineOverlay( start, end, 255, 0, 0, true, 0.01 ); 

	VectorMA( start, -vecSize.y, vec[1], end );
	debugoverlay->AddLineOverlay( start, end, 255, 0, 0, true, 0.01 ); 

	VectorMA( start, -vecSize.z, vec[2], end );
	debugoverlay->AddLineOverlay( start, end, 255, 0, 0, true, 0.01 );

	start = end;
	VectorMA( start, -vecSize.x, vec[0], end );
	debugoverlay->AddLineOverlay( start, end, 255, 0, 0, true, 0.01 ); 

	VectorMA( start, -vecSize.y, vec[1], end );
	debugoverlay->AddLineOverlay( start, end, 255, 0, 0, true, 0.01 ); 

	C_BaseEntity *pEnt = pRenderable->GetIClientUnknown()->GetBaseEntity();
	if ( pEnt )
	{
		debugoverlay->AddTextOverlay( vecOrigin, 0, "%d", pEnt->entindex() );
	}
	else
	{
		debugoverlay->AddTextOverlay( vecOrigin, 0, "%X", (size_t)pRenderable );
	}
}


//-----------------------------------------------------------------------------
// Builds a more complex shadow...
//-----------------------------------------------------------------------------
void CClientShadowMgr::BuildRenderToTextureShadow( IClientRenderable* pRenderable, 
		ClientShadowHandle_t handle, const Vector& mins, const Vector& maxs)
{
//	DrawRenderToTextureDebugInfo( pRenderable, mins, maxs );

	// Get the object's basis
	Vector vec[3];
	AngleVectors( pRenderable->GetRenderAngles(), &vec[0], &vec[1], &vec[2] );
	vec[1] *= -1.0f;

	Vector vecShadowDir = GetShadowDirection( pRenderable );

	// Project the shadow casting direction into the space of the object
	Vector localShadowDir;
	localShadowDir[0] = DotProduct( vec[0], vecShadowDir );
	localShadowDir[1] = DotProduct( vec[1], vecShadowDir );
	localShadowDir[2] = DotProduct( vec[2], vecShadowDir );

	// Figure out which vector has the largest component perpendicular
	// to the shadow handle...
	// Sort by how perpendicular it is
	int vecIdx[3];
	SortAbsVectorComponents( localShadowDir, vecIdx );

	// Here's our shadow basis vectors; namely the ones that are
	// most perpendicular to the shadow casting direction
	Vector yvec = vec[vecIdx[0]];

	// Project it into a plane perpendicular to the shadow direction
	yvec -= vecShadowDir * DotProduct( vecShadowDir, yvec );
	VectorNormalize( yvec );

	// Compute the x vector
	Vector xvec;
	CrossProduct( yvec, vecShadowDir, xvec );

	// Compute the box size
	Vector boxSize;
	VectorSubtract( maxs, mins, boxSize );

	// We project the two longest sides into the vectors perpendicular
	// to the projection direction, then add in the projection of the perp direction
	Vector2D size;
	size.x = boxSize.x * fabs( DotProduct( vec[0], xvec ) ) + 
		boxSize.y * fabs( DotProduct( vec[1], xvec ) ) + 
		boxSize.z * fabs( DotProduct( vec[2], xvec ) );
	size.y = boxSize.x * fabs( DotProduct( vec[0], yvec ) ) + 
		boxSize.y * fabs( DotProduct( vec[1], yvec ) ) + 
		boxSize.z * fabs( DotProduct( vec[2], yvec ) );

	size.x += 2.0f * TEXEL_SIZE_PER_CASTER_SIZE;
	size.y += 2.0f * TEXEL_SIZE_PER_CASTER_SIZE;

	// Place the origin at the point with min dot product with shadow dir
	Vector org;
	float falloffStart = ComputeLocalShadowOrigin( pRenderable, mins, maxs, localShadowDir, 1.0f, org );

	// Transform the local origin into world coordinates
	Vector worldOrigin = pRenderable->GetRenderOrigin( );
	VectorMA( worldOrigin, org.x, vec[0], worldOrigin );
	VectorMA( worldOrigin, org.y, vec[1], worldOrigin );
	VectorMA( worldOrigin, org.z, vec[2], worldOrigin );

	VMatrix worldToTexture;
	BuildOrthoWorldToShadowMatrix( m_Shadows[handle].m_WorldToShadow, worldOrigin, vecShadowDir, xvec, yvec );
	BuildWorldToTextureMatrix( m_Shadows[handle].m_WorldToShadow, size, worldToTexture );
	Vector2DCopy( size, m_Shadows[handle].m_WorldSize );

	// Compute the falloff attenuation
	// Area computation isn't exact since xvec is not perp to yvec, but close enough
	// Extra factor of 4 in the maxHeight due to the size being half as big
//	float shadowArea = size.x * size.y;	

	// The entity may be overriding our shadow cast distance
	float flShadowCastDistance = GetShadowDistance( pRenderable );
	float maxHeight = flShadowCastDistance + falloffStart; //3.0f * sqrt( shadowArea );

	shadowmgr->ProjectShadow( m_Shadows[handle].m_ShadowHandle, worldOrigin, 
		vecShadowDir, worldToTexture, size, maxHeight, falloffStart, MAX_FALLOFF_AMOUNT, pRenderable->GetRenderOrigin() );

	// Compute extra clip planes to prevent poke-thru
	ComputeExtraClipPlanes( pRenderable, handle, vec, mins, maxs, localShadowDir );

	// Add the shadow to the client leaf system so it correctly marks 
	// leafs as being affected by a particular shadow
	ClientLeafSystem()->ProjectShadow( m_Shadows[handle].m_ClientLeafShadowHandle, 
		worldOrigin, vecShadowDir, size, maxHeight );
}

static void LineDrawHelper( const Vector &startShadowSpace, const Vector &endShadowSpace, 
						   const VMatrix &shadowToWorld, unsigned char r = 255, unsigned char g = 255, 
						   unsigned char b = 255 )
{
	Vector startWorldSpace, endWorldSpace;
	Vector3DMultiplyPositionProjective( shadowToWorld, startShadowSpace, startWorldSpace );
	Vector3DMultiplyPositionProjective( shadowToWorld, endShadowSpace, endWorldSpace );

	debugoverlay->AddLineOverlay( startWorldSpace + Vector( 0.0f, 0.0f, 1.0f ), 
		endWorldSpace + Vector( 0.0f, 0.0f, 1.0f ), r, g, b, false
		, 0.0 );
}

static void DebugDrawFrustum( const VMatrix &worldToFlashlight )
{
	VMatrix flashlightToWorld;
	MatrixInverseGeneral( worldToFlashlight, flashlightToWorld );
	
	LineDrawHelper( Vector( 0.0f, 0.0f, 0.0f ), Vector( 0.0f, 0.0f, 1.0f ), flashlightToWorld, 255, 0, 0 );
	LineDrawHelper( Vector( 0.0f, 0.0f, 1.0f ), Vector( 0.0f, 1.0f, 1.0f ), flashlightToWorld, 255, 0, 0 );
	LineDrawHelper( Vector( 0.0f, 1.0f, 1.0f ), Vector( 0.0f, 1.0f, 0.0f ), flashlightToWorld, 255, 0, 0 );
	LineDrawHelper( Vector( 0.0f, 1.0f, 0.0f ), Vector( 0.0f, 0.0f, 0.0f ), flashlightToWorld, 255, 0, 0 );
	LineDrawHelper( Vector( 1.0f, 0.0f, 0.0f ), Vector( 1.0f, 0.0f, 1.0f ), flashlightToWorld, 255, 0, 0 );
	LineDrawHelper( Vector( 1.0f, 0.0f, 1.0f ), Vector( 1.0f, 1.0f, 1.0f ), flashlightToWorld, 255, 0, 0 );
	LineDrawHelper( Vector( 1.0f, 1.0f, 1.0f ), Vector( 1.0f, 1.0f, 0.0f ), flashlightToWorld, 255, 0, 0 );
	LineDrawHelper( Vector( 1.0f, 1.0f, 0.0f ), Vector( 1.0f, 0.0f, 0.0f ), flashlightToWorld, 255, 0, 0 );
	LineDrawHelper( Vector( 0.0f, 0.0f, 0.0f ), Vector( 1.0f, 0.0f, 0.0f ), flashlightToWorld, 255, 0, 0 );
	LineDrawHelper( Vector( 0.0f, 0.0f, 1.0f ), Vector( 1.0f, 0.0f, 1.0f ), flashlightToWorld, 255, 0, 0 );
	LineDrawHelper( Vector( 0.0f, 1.0f, 1.0f ), Vector( 1.0f, 1.0f, 1.0f ), flashlightToWorld, 255, 0, 0 );
	LineDrawHelper( Vector( 0.0f, 1.0f, 0.0f ), Vector( 1.0f, 1.0f, 0.0f ), flashlightToWorld, 255, 0, 0 );
}

void CClientShadowMgr::BuildFlashlight( ClientShadowHandle_t handle )
{
	ClientShadow_t &shadow = m_Shadows[handle];

	if( r_flashlightdrawfrustum.GetBool() )
	{
		DebugDrawFrustum( shadow.m_WorldToShadow );
	}

	if( shadow.m_bLightWorld )
	{
		shadowmgr->ProjectFlashlight( shadow.m_ShadowHandle, shadow.m_WorldToShadow );
	}
	else
	{
		// This should clear all models and surfaces from this shadow
		shadowmgr->EnableShadow( shadow.m_ShadowHandle, false );
		shadowmgr->EnableShadow( shadow.m_ShadowHandle, true );
	}

	if( r_flashlightmodels.GetBool() )
	{
		if( shadow.m_hTargetEntity==NULL )
		{
			// Add the shadow to the client leaf system so it correctly marks 
			// leafs as being affected by a particular shadow
			ClientLeafSystem()->ProjectFlashlight( shadow.m_ClientLeafShadowHandle, shadow.m_WorldToShadow );
		}
		else
		{
			// We know what we are focused on, so just add the shadow directly to that receiver
			Assert( shadow.m_hTargetEntity->GetModel() );

			C_BaseEntity *pChild = shadow.m_hTargetEntity->FirstMoveChild();
			while( pChild )
			{
				int modelType = modelinfo->GetModelType( pChild->GetModel() );
				if (modelType == mod_brush)
				{
					AddShadowToReceiver( handle, pChild, SHADOW_RECEIVER_BRUSH_MODEL );
				}
				else if ( modelType == mod_studio )
				{
					AddShadowToReceiver( handle, pChild, SHADOW_RECEIVER_STUDIO_MODEL );
				}
		
				pChild = pChild->NextMovePeer();
			}
 
			int modelType = modelinfo->GetModelType( shadow.m_hTargetEntity->GetModel() );
			if (modelType == mod_brush)
			{
				AddShadowToReceiver( handle, shadow.m_hTargetEntity, SHADOW_RECEIVER_BRUSH_MODEL );
			}
			else if ( modelType == mod_studio )
			{
				AddShadowToReceiver( handle, shadow.m_hTargetEntity, SHADOW_RECEIVER_STUDIO_MODEL );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Adds the child bounds to the bounding box
//-----------------------------------------------------------------------------
void CClientShadowMgr::AddChildBounds( matrix3x4_t &worldToBBox, IClientRenderable* pParent, Vector &vecMins, Vector &vecMaxs )
{
	Vector vecChildMins, vecChildMaxs;
	Vector vecNewChildMins, vecNewChildMaxs;
	matrix3x4_t childToBBox;

	IClientRenderable *pChild = pParent->FirstShadowChild();
	while( pChild )
	{
		// Transform the child bbox into the space of the main bbox
		// FIXME: Optimize this?
		if ( GetActualShadowCastType( pChild ) != SHADOWS_NONE)
		{
			pChild->GetShadowRenderBounds( vecChildMins, vecChildMaxs, SHADOWS_RENDER_TO_TEXTURE );
			ConcatTransforms( worldToBBox, pChild->RenderableToWorldTransform(), childToBBox );
			TransformAABB( childToBBox, vecChildMins, vecChildMaxs, vecNewChildMins, vecNewChildMaxs );
			VectorMin( vecMins, vecNewChildMins, vecMins );
			VectorMax( vecMaxs, vecNewChildMaxs, vecMaxs );
		}

		AddChildBounds( worldToBBox, pChild, vecMins, vecMaxs );
		pChild = pChild->NextShadowPeer();
	}
}


//-----------------------------------------------------------------------------
// Compute a bounds for the entity + children
//-----------------------------------------------------------------------------
void CClientShadowMgr::ComputeHierarchicalBounds( IClientRenderable *pRenderable, Vector &vecMins, Vector &vecMaxs )
{
	ShadowType_t shadowType = GetActualShadowCastType( pRenderable );

	pRenderable->GetShadowRenderBounds( vecMins, vecMaxs, shadowType );

	// We could use a good solution for this in the regular PC build, since
	// it causes lots of extra bone setups for entities you can't see.
	if ( IsPC() )
	{
		IClientRenderable *pChild = pRenderable->FirstShadowChild();

		// Don't recurse down the tree when we hit a blobby shadow
		if ( pChild && shadowType != SHADOWS_SIMPLE )
		{
			matrix3x4_t worldToBBox;
			MatrixInvert( pRenderable->RenderableToWorldTransform(), worldToBBox );
			AddChildBounds( worldToBBox, pRenderable, vecMins, vecMaxs );
		}
	}
}


//-----------------------------------------------------------------------------
// Shadow update functions
//-----------------------------------------------------------------------------
void CClientShadowMgr::UpdateStudioShadow( IClientRenderable *pRenderable, ClientShadowHandle_t handle )
{
	if( !( m_Shadows[handle].m_Flags & SHADOW_FLAGS_FLASHLIGHT ) )
	{
		Vector mins, maxs;
		ComputeHierarchicalBounds( pRenderable, mins, maxs );

		ShadowType_t shadowType = GetActualShadowCastType( handle );
		if ( shadowType != SHADOWS_RENDER_TO_TEXTURE )
		{
			BuildOrthoShadow( pRenderable, handle, mins, maxs );
		}
		else
		{
			BuildRenderToTextureShadow( pRenderable, handle, mins, maxs );
		}
	}
	else
	{
		BuildFlashlight( handle );
	}
}

void CClientShadowMgr::UpdateBrushShadow( IClientRenderable *pRenderable, ClientShadowHandle_t handle )
{
	if( !( m_Shadows[handle].m_Flags & SHADOW_FLAGS_FLASHLIGHT ) )
	{
		// Compute the bounding box in the space of the shadow...
		Vector mins, maxs;
		ComputeHierarchicalBounds( pRenderable, mins, maxs );

		ShadowType_t shadowType = GetActualShadowCastType( handle );
		if ( shadowType != SHADOWS_RENDER_TO_TEXTURE )
		{
			BuildOrthoShadow( pRenderable, handle, mins, maxs );
		}
		else
		{
			BuildRenderToTextureShadow( pRenderable, handle, mins, maxs );
		}
	}
	else
	{
		BuildFlashlight( handle );
	}
}


#ifdef _DEBUG

static bool s_bBreak = false;

void ShadowBreak_f()
{
	s_bBreak = true;
}

static ConCommand r_shadowbreak("r_shadowbreak", ShadowBreak_f);

#endif // _DEBUG


bool CClientShadowMgr::WillParentRenderBlobbyShadow( IClientRenderable *pRenderable )
{
	if ( !pRenderable )
		return false;

	IClientRenderable *pShadowParent = pRenderable->GetShadowParent();
	if ( !pShadowParent )
		return false;

	// If there's *no* shadow casting type, then we want to see if we can render into its parent
 	ShadowType_t shadowType = GetActualShadowCastType( pShadowParent );
	if ( shadowType == SHADOWS_NONE )
		return WillParentRenderBlobbyShadow( pShadowParent );

	return shadowType == SHADOWS_SIMPLE;
}


//-----------------------------------------------------------------------------
// Are we the child of a shadow with render-to-texture?
//-----------------------------------------------------------------------------
bool CClientShadowMgr::ShouldUseParentShadow( IClientRenderable *pRenderable )
{
	if ( !pRenderable )
		return false;

	IClientRenderable *pShadowParent = pRenderable->GetShadowParent();
	if ( !pShadowParent )
		return false;

	// Can't render into the parent if the parent is blobby
	ShadowType_t shadowType = GetActualShadowCastType( pShadowParent );
	if ( shadowType == SHADOWS_SIMPLE )
		return false;

	// If there's *no* shadow casting type, then we want to see if we can render into its parent
 	if ( shadowType == SHADOWS_NONE )
		return ShouldUseParentShadow( pShadowParent );

	// Here, the parent uses a render-to-texture shadow
	return true;
}


//-----------------------------------------------------------------------------
// Before we render any view, make sure all shadows are re-projected vs world
//-----------------------------------------------------------------------------
void CClientShadowMgr::PreRender()
{
	VPROF_BUDGET( "CClientShadowMgr::PreRender", VPROF_BUDGETGROUP_SHADOW_RENDERING );
	MDLCACHE_CRITICAL_SECTION();

	bool bRenderToTextureActive = r_shadowrendertotexture.GetBool();
	if ( bRenderToTextureActive != m_RenderToTextureActive )
	{
		if ( m_RenderToTextureActive )
		{
			ShutdownRenderToTextureShadows();
		}
		else
		{
			InitRenderToTextureShadows();
		}

		UpdateAllShadows();
		return;
	}

#ifdef DOSHADOWEDFLASHLIGHT
	bool bDepthTextureActive = r_flashlightdepthtexture.GetBool();
	if ( bDepthTextureActive != m_DepthTextureActive )
	{	
		if( m_DepthTextureActive )
		{
			ShutdownDepthTextureShadows();
		}								  
		else
		{
			InitDepthTextureShadows();
		}

		UpdateAllShadows();

		return;
	}
#endif

	m_bUpdatingDirtyShadows = true;

	unsigned short i = m_DirtyShadows.FirstInorder();
	while ( i != m_DirtyShadows.InvalidIndex() )
	{
		ClientShadowHandle_t& handle = m_DirtyShadows[ i ];
		Assert( m_Shadows.IsValidIndex( handle ) );
		UpdateProjectedTextureInternal( handle, false );
		i = m_DirtyShadows.NextInorder(i);
	}

	m_DirtyShadows.RemoveAll();

	m_bUpdatingDirtyShadows = false;
}


//-----------------------------------------------------------------------------
// Gets the entity whose shadow this shadow will render into
//-----------------------------------------------------------------------------
IClientRenderable *CClientShadowMgr::GetParentShadowEntity( ClientShadowHandle_t handle )
{
	ClientShadow_t& shadow = m_Shadows[handle];
	IClientRenderable *pRenderable = ClientEntityList().GetClientRenderableFromHandle( shadow.m_Entity );
	if ( pRenderable )
	{
		if ( ShouldUseParentShadow( pRenderable ) )
		{
			IClientRenderable *pParent = pRenderable->GetShadowParent();
			while ( GetActualShadowCastType( pParent ) == SHADOWS_NONE )
			{
				pParent = pParent->GetShadowParent();
				Assert( pParent );
			}
			return pParent;
		}
	}
	return NULL;
}


//-----------------------------------------------------------------------------
// Marks a shadow as needing re-projection
//-----------------------------------------------------------------------------
void CClientShadowMgr::AddToDirtyShadowList( ClientShadowHandle_t handle, bool bForce )
{
	// Don't add to the dirty shadow list while we're iterating over it
	// The only way this can happen is if a child is being rendered into a parent
	// shadow, and we don't need it to be added to the dirty list in that case.
	if ( m_bUpdatingDirtyShadows )
		return;

	if ( handle == CLIENTSHADOW_INVALID_HANDLE )
		return;

	Assert( m_DirtyShadows.Find( handle ) == m_DirtyShadows.InvalidIndex() );
	m_DirtyShadows.Insert( handle );

	// This pretty much guarantees we'll recompute the shadow
	if ( bForce )
	{
		m_Shadows[handle].m_LastAngles.Init( FLT_MAX, FLT_MAX, FLT_MAX );
	}

	// If we use our parent shadow, then it's dirty too...
	IClientRenderable *pParent = GetParentShadowEntity( handle );
	if ( pParent )
	{
		AddToDirtyShadowList( pParent, bForce );
	}
}


//-----------------------------------------------------------------------------
// Marks a shadow as needing re-projection
//-----------------------------------------------------------------------------
void CClientShadowMgr::AddToDirtyShadowList( IClientRenderable *pRenderable, bool bForce )
{
	// Don't add to the dirty shadow list while we're iterating over it
	// The only way this can happen is if a child is being rendered into a parent
	// shadow, and we don't need it to be added to the dirty list in that case.
	if ( m_bUpdatingDirtyShadows )
		return;

	// Are we already in the dirty list?
	if ( pRenderable->IsShadowDirty( ) )
		return;

	ClientShadowHandle_t handle = pRenderable->GetShadowHandle();
	if ( handle == CLIENTSHADOW_INVALID_HANDLE )
		return;

#ifdef _DEBUG
	// Make sure everything's consistent
	if ( pRenderable->GetShadowHandle() != CLIENTSHADOW_INVALID_HANDLE )
	{
		IClientRenderable *pShadowRenderable = ClientEntityList().GetClientRenderableFromHandle( m_Shadows[handle].m_Entity );
		Assert( pRenderable == pShadowRenderable );
	}
#endif

	pRenderable->MarkShadowDirty( true );
	AddToDirtyShadowList( handle, bForce );
}


//-----------------------------------------------------------------------------
// Marks the render-to-texture shadow as needing to be re-rendered
//-----------------------------------------------------------------------------
void CClientShadowMgr::MarkRenderToTextureShadowDirty( ClientShadowHandle_t handle )
{
	// Don't add bogus handles!
	if (handle != CLIENTSHADOW_INVALID_HANDLE)
	{
		// Mark the shadow has having a dirty renter-to-texture
		ClientShadow_t& shadow = m_Shadows[handle];
		shadow.m_Flags |= SHADOW_FLAGS_TEXTURE_DIRTY;

		// If we use our parent shadow, then it's dirty too...
		IClientRenderable *pParent = GetParentShadowEntity( handle );
		if ( pParent )
		{
			ClientShadowHandle_t parentHandle = pParent->GetShadowHandle();
			if ( parentHandle != CLIENTSHADOW_INVALID_HANDLE )
			{
				m_Shadows[parentHandle].m_Flags |= SHADOW_FLAGS_TEXTURE_DIRTY;
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Update a shadow
//-----------------------------------------------------------------------------
void CClientShadowMgr::UpdateShadow( ClientShadowHandle_t handle, bool force )
{
	ClientShadow_t& shadow = m_Shadows[handle];

	// Get the client entity....
	IClientRenderable *pRenderable = ClientEntityList().GetClientRenderableFromHandle( shadow.m_Entity );
	if ( !pRenderable )
	{
		// Retire the shadow if the entity is gone
		DestroyShadow( handle );
		return;
	}

	if ( !pRenderable->GetModel() )
	{
		pRenderable->MarkShadowDirty( false );
		return;
	}

#ifdef _DEBUG
	if (s_bBreak)
	{
		s_bBreak = false;
	}
#endif
	// Hierarchical children shouldn't be projecting shadows...
	// Check to see if it's a child of an entity with a render-to-texture shadow...
	if ( ShouldUseParentShadow( pRenderable ) || WillParentRenderBlobbyShadow( pRenderable ) )
	{
		shadowmgr->EnableShadow( shadow.m_ShadowHandle, false );
		pRenderable->MarkShadowDirty( false );
		return;
	}

	shadowmgr->EnableShadow( shadow.m_ShadowHandle, true );

	// Figure out if the shadow moved...
	// Even though we have dirty bits, some entities
	// never clear those dirty bits
	const Vector& origin = pRenderable->GetRenderOrigin();
	const QAngle& angles = pRenderable->GetRenderAngles();

	if (force || (origin != shadow.m_LastOrigin) || (angles != shadow.m_LastAngles))
	{
		// Store off the new pos/orientation
		VectorCopy( origin, shadow.m_LastOrigin );
		VectorCopy( angles, shadow.m_LastAngles );

		const model_t *pModel = pRenderable->GetModel();
		MaterialFogMode_t fogMode = materials->GetFogMode();
		materials->FogMode( MATERIAL_FOG_NONE );
		switch( modelinfo->GetModelType( pModel ) )
		{
		case mod_brush:
			UpdateBrushShadow( pRenderable, handle );
			break;

		case mod_studio:
			UpdateStudioShadow( pRenderable, handle );
			break;

		default:
			// Shouldn't get here if not a brush or studio
			Assert(0);
			break;
		}
		materials->FogMode( fogMode );
	}

	// NOTE: We can't do this earlier because pEnt->GetRenderOrigin() can
	// provoke a recomputation of render origin, which, for aiments, can cause everything
	// to be marked as dirty. So don't clear the flag until this point.
	pRenderable->MarkShadowDirty( false );
}


//-----------------------------------------------------------------------------
// Update a shadow
//-----------------------------------------------------------------------------
void CClientShadowMgr::UpdateProjectedTextureInternal( ClientShadowHandle_t handle, bool force )
{
	ClientShadow_t& shadow = m_Shadows[handle];

	if( shadow.m_Flags & SHADOW_FLAGS_FLASHLIGHT )
	{
		Assert( ( shadow.m_Flags & SHADOW_FLAGS_SHADOW ) == 0 );
		ClientShadow_t& shadow = m_Shadows[handle];

		shadowmgr->EnableShadow( shadow.m_ShadowHandle, true );

		// Make sure to allocate/deallocated shadow depth texture here
		// if they are out of sync with flags
#ifdef DOSHADOWEDFLASHLIGHT
		if( shadow.m_Flags & SHADOW_FLAGS_USE_DEPTH_TEXTURE )
		{
			if( !shadow.m_ShadowDepthTexture && r_flashlightdepthtexture.GetBool() )
			{
				AllocateDepthBuffer( shadow.m_ShadowDepthTexture );
			}
		}
		else
#endif
		{
			if( shadow.m_ShadowDepthTexture )
			{
				DeallocateDepthBuffer( shadow.m_ShadowDepthTexture );
			}
		}
		// FIXME: What's the difference between brush and model shadows for light projectors? Answer: nothing.
		UpdateBrushShadow( NULL, handle );
	}
	else
	{
		Assert( shadow.m_Flags & SHADOW_FLAGS_SHADOW );
		Assert( ( shadow.m_Flags & SHADOW_FLAGS_FLASHLIGHT ) == 0 );
		UpdateShadow( handle, force );
	}
}


//-----------------------------------------------------------------------------
// Update a shadow
//-----------------------------------------------------------------------------
void CClientShadowMgr::UpdateProjectedTexture( ClientShadowHandle_t handle, bool force )
{
	if (handle == CLIENTSHADOW_INVALID_HANDLE)
		return;

	UpdateProjectedTextureInternal( handle, force );
	RemoveShadowFromDirtyList( handle );
}

	
//-----------------------------------------------------------------------------
// Computes bounding sphere
//-----------------------------------------------------------------------------
void CClientShadowMgr::ComputeBoundingSphere( IClientRenderable* pRenderable, Vector& origin, float& radius )
{
	Assert( pRenderable );
	Vector mins, maxs;
	pRenderable->GetShadowRenderBounds( mins, maxs, GetActualShadowCastType( pRenderable ) );
	Vector size;
	VectorSubtract( maxs, mins, size );
	radius = size.Length() * 0.5f;

	// Compute centroid (local space)
	Vector centroid;
	VectorAdd( mins, maxs, centroid );
	centroid *= 0.5f;

	// Transform centroid into world space
	Vector vec[3];
	AngleVectors( pRenderable->GetRenderAngles(), &vec[0], &vec[1], &vec[2] );
	vec[1] *= -1.0f;

	VectorCopy( pRenderable->GetRenderOrigin(), origin );
	VectorMA( origin, centroid.x, vec[0], origin );
	VectorMA( origin, centroid.y, vec[1], origin );
	VectorMA( origin, centroid.z, vec[2], origin );
}


//-----------------------------------------------------------------------------
// Computes a rough AABB encompassing the volume of the shadow
//-----------------------------------------------------------------------------
void CClientShadowMgr::ComputeShadowBBox( IClientRenderable *pRenderable, const Vector &vecAbsCenter, float flRadius, Vector *pAbsMins, Vector *pAbsMaxs )
{
	// This is *really* rough. Basically we simply determine the
	// maximum shadow casting length and extrude the box by that distance

	Vector vecShadowDir = GetShadowDirection( pRenderable );
	for (int i = 0; i < 3; ++i)
	{
		float flShadowCastDistance = GetShadowDistance( pRenderable );
		float flDist = flShadowCastDistance * vecShadowDir[i];

		if (vecShadowDir[i] < 0)
		{
			(*pAbsMins)[i] = vecAbsCenter[i] - flRadius + flDist;
			(*pAbsMaxs)[i] = vecAbsCenter[i] + flRadius;
		}
		else
		{
			(*pAbsMins)[i] = vecAbsCenter[i] - flRadius;
			(*pAbsMaxs)[i] = vecAbsCenter[i] + flRadius + flDist;
		}
	}
}


//-----------------------------------------------------------------------------
// Compute a separating axis...
//-----------------------------------------------------------------------------
bool CClientShadowMgr::ComputeSeparatingPlane( IClientRenderable* pRend1, IClientRenderable* pRend2, cplane_t* pPlane )
{
	Vector min1, max1, min2, max2;
	pRend1->GetShadowRenderBounds( min1, max1, GetActualShadowCastType( pRend1 ) );
	pRend2->GetShadowRenderBounds( min2, max2, GetActualShadowCastType( pRend2 ) );
	return ::ComputeSeparatingPlane( 
		pRend1->GetRenderOrigin(), pRend1->GetRenderAngles(), min1, max1,
		pRend2->GetRenderOrigin(), pRend2->GetRenderAngles(), min2, max2,
		3.0f, pPlane );
}


//-----------------------------------------------------------------------------
// Cull shadows based on rough bounding volumes
//-----------------------------------------------------------------------------
bool CClientShadowMgr::CullReceiver( ClientShadowHandle_t handle, IClientRenderable* pRenderable,
									IClientRenderable* pSourceRenderable )
{
	// check flags here instead and assert !pSourceRenderable
	if( m_Shadows[handle].m_Flags & SHADOW_FLAGS_FLASHLIGHT )
	{
		Assert( !pSourceRenderable );	
		const Frustum_t &frustum = shadowmgr->GetFlashlightFrustum( m_Shadows[handle].m_ShadowHandle );

		Vector mins, maxs;
		pRenderable->GetRenderBoundsWorldspace( mins, maxs );

		return R_CullBox( mins, maxs, frustum );
	}

	Assert( pSourceRenderable );	
	// Compute a bounding sphere for the renderable
	Vector origin;
	float radius;
	ComputeBoundingSphere( pRenderable, origin, radius );

	// Transform the sphere center into the space of the shadow
	Vector localOrigin;
	const ClientShadow_t& shadow = m_Shadows[handle];
	const ShadowInfo_t& info = shadowmgr->GetInfo( shadow.m_ShadowHandle );
	Vector3DMultiplyPosition( shadow.m_WorldToShadow, origin, localOrigin );

	// Compute a rough bounding box for the shadow (in shadow space)
	Vector shadowMin, shadowMax;
	shadowMin.Init( -shadow.m_WorldSize.x * 0.5f, -shadow.m_WorldSize.y * 0.5f, 0 );
	shadowMax.Init( shadow.m_WorldSize.x * 0.5f, shadow.m_WorldSize.y * 0.5f, info.m_FalloffOffset + info.m_MaxDist );

	// If the bounding sphere doesn't intersect with the shadow volume, cull
	if (!IsBoxIntersectingSphere( shadowMin, shadowMax, localOrigin, radius ))
		return true;

	Vector originSource;
	float radiusSource;
	ComputeBoundingSphere( pSourceRenderable, originSource, radiusSource );

	// Fast check for separating plane...
	bool foundSeparatingPlane = false;
	cplane_t plane;
	if (!IsSphereIntersectingSphere( originSource, radiusSource, origin, radius ))
	{
		foundSeparatingPlane = true;

		// the plane normal doesn't need to be normalized...
		VectorSubtract( origin, originSource, plane.normal );
	}
	else
	{
		foundSeparatingPlane = ComputeSeparatingPlane( pRenderable, pSourceRenderable, &plane );
	}

	if (foundSeparatingPlane)
	{
		// Compute which side of the plane the renderable is on..
		Vector vecShadowDir = GetShadowDirection( pSourceRenderable );
		float shadowDot = DotProduct( vecShadowDir, plane.normal );
		float receiverDot = DotProduct( plane.normal, origin );
		float sourceDot = DotProduct( plane.normal, originSource );

		if (shadowDot > 0.0f)
		{
			if (receiverDot <= sourceDot)
			{
//				Vector dest;
//				VectorMA( pSourceRenderable->GetRenderOrigin(), 50, plane.normal, dest ); 
//				debugoverlay->AddLineOverlay( pSourceRenderable->GetRenderOrigin(), dest, 255, 255, 0, true, 1.0f );
				return true;
			}
			else
			{
//				Vector dest;
//				VectorMA( pSourceRenderable->GetRenderOrigin(), 50, plane.normal, dest ); 
//				debugoverlay->AddLineOverlay( pSourceRenderable->GetRenderOrigin(), dest, 255, 0, 0, true, 1.0f );
			}
		}
		else
		{
			if (receiverDot >= sourceDot)
			{
//				Vector dest;
//				VectorMA( pSourceRenderable->GetRenderOrigin(), -50, plane.normal, dest ); 
//				debugoverlay->AddLineOverlay( pSourceRenderable->GetRenderOrigin(), dest, 255, 255, 0, true, 1.0f );
				return true;
			}
			else
			{
//				Vector dest;
//				VectorMA( pSourceRenderable->GetRenderOrigin(), 50, plane.normal, dest ); 
//				debugoverlay->AddLineOverlay( pSourceRenderable->GetRenderOrigin(), dest, 255, 0, 0, true, 1.0f );
			}
		}
	}

	// No additional clip planes? ok then it's a valid receiver
	/*
	if (shadow.m_ClipPlaneCount == 0)
		return false;

	// Check the additional cull planes
	int i;
	for ( i = 0; i < shadow.m_ClipPlaneCount; ++i)
	{
		// Fast sphere cull
		if (DotProduct( origin, shadow.m_ClipPlane[i] ) - radius > shadow.m_ClipDist[i])
			return true;
	}

	// More expensive box on plane side cull...
	Vector vec[3];
	Vector mins, maxs;
	cplane_t plane;
	AngleVectors( pRenderable->GetRenderAngles(), &vec[0], &vec[1], &vec[2] );
	pRenderable->GetBounds( mins, maxs );

	for ( i = 0; i < shadow.m_ClipPlaneCount; ++i)
	{
		// Transform the plane into the space of the receiver
		plane.normal.x = DotProduct( vec[0], shadow.m_ClipPlane[i] );
		plane.normal.y = DotProduct( vec[1], shadow.m_ClipPlane[i] );
		plane.normal.z = DotProduct( vec[2], shadow.m_ClipPlane[i] );

		plane.dist = shadow.m_ClipDist[i] - DotProduct( shadow.m_ClipPlane[i], pRenderable->GetRenderOrigin() );

		// If the box is on the front side of the plane, we're done.
		if (BoxOnPlaneSide2( mins, maxs, &plane, 3.0f ) == 1)
			return true;
	}
	*/

	return false;
}


//-----------------------------------------------------------------------------
// deals with shadows being added to shadow receivers
//-----------------------------------------------------------------------------
void CClientShadowMgr::AddShadowToReceiver( ClientShadowHandle_t handle,
	IClientRenderable* pRenderable, ShadowReceiver_t type )
{
	ClientShadow_t &shadow = m_Shadows[handle];

	// Don't add a shadow cast by an object to itself...
	IClientRenderable* pSourceRenderable = ClientEntityList().GetClientRenderableFromHandle( shadow.m_Entity );

	// NOTE: if pSourceRenderable == NULL, the source is probably a flashlight since there is no entity.
	if (pSourceRenderable == pRenderable)
		return;

	// Don't bother if this renderable doesn't receive shadows or light from flashlights
	if( !pRenderable->ShouldReceiveProjectedTextures( SHADOW_FLAGS_PROJECTED_TEXTURE_TYPE_MASK ) )
		return;

	// Cull if the origin is on the wrong side of a shadow clip plane....
	if ( CullReceiver( handle, pRenderable, pSourceRenderable ) )
		return;

	// Do different things depending on the receiver type
	switch( type )
	{
	case SHADOW_RECEIVER_BRUSH_MODEL:

		if( shadow.m_Flags & SHADOW_FLAGS_FLASHLIGHT )
		{
			if( (!shadow.m_hTargetEntity) || IsFlashlightTarget( handle, pRenderable ) )
			{
				shadowmgr->AddShadowToBrushModel( shadow.m_ShadowHandle, 
					const_cast<model_t*>(pRenderable->GetModel()),
					pRenderable->GetRenderOrigin(), pRenderable->GetRenderAngles() );

				shadowmgr->AddFlashlightRenderable( shadow.m_ShadowHandle, pRenderable );
			}
		}
		else
		{
			shadowmgr->AddShadowToBrushModel( shadow.m_ShadowHandle, 
				const_cast<model_t*>(pRenderable->GetModel()),
				pRenderable->GetRenderOrigin(), pRenderable->GetRenderAngles() );
		}
		break;

	case SHADOW_RECEIVER_STATIC_PROP:
		// Don't add shadows to props if we're not using render-to-texture
		if ( GetActualShadowCastType( handle ) == SHADOWS_RENDER_TO_TEXTURE )
		{
			// Also don't add them unless an NPC or player casts them..
			// They are wickedly expensive!!!
			C_BaseEntity *pEnt = pSourceRenderable->GetIClientUnknown()->GetBaseEntity();
			if ( pEnt && ( pEnt->GetFlags() & (FL_NPC | FL_CLIENT)) )
			{
				staticpropmgr->AddShadowToStaticProp( shadow.m_ShadowHandle, pRenderable );
			}
		}
		else if( shadow.m_Flags & SHADOW_FLAGS_FLASHLIGHT )
		{
			if( (!shadow.m_hTargetEntity) || IsFlashlightTarget( handle, pRenderable ) )
			{
				staticpropmgr->AddShadowToStaticProp( shadow.m_ShadowHandle, pRenderable );

				shadowmgr->AddFlashlightRenderable( shadow.m_ShadowHandle, pRenderable );
			}
		}
		break;

	case SHADOW_RECEIVER_STUDIO_MODEL:
		if( shadow.m_Flags & SHADOW_FLAGS_FLASHLIGHT )
		{
			if( (!shadow.m_hTargetEntity) || IsFlashlightTarget( handle, pRenderable ) )
			{
				pRenderable->CreateModelInstance();
				shadowmgr->AddShadowToModel( shadow.m_ShadowHandle, pRenderable->GetModelInstance() );
				shadowmgr->AddFlashlightRenderable( shadow.m_ShadowHandle, pRenderable );
			}
		}
		break;
//	default:
	}
}


//-----------------------------------------------------------------------------
// deals with shadows being added to shadow receivers
//-----------------------------------------------------------------------------
void CClientShadowMgr::RemoveAllShadowsFromReceiver( 
					IClientRenderable* pRenderable, ShadowReceiver_t type )
{
	// Don't bother if this renderable doesn't receive shadows
	if ( !pRenderable->ShouldReceiveProjectedTextures( SHADOW_FLAGS_PROJECTED_TEXTURE_TYPE_MASK ) )
		return;

	// Do different things depending on the receiver type
	switch( type )
	{
	case SHADOW_RECEIVER_BRUSH_MODEL:
		{
			model_t* pModel = const_cast<model_t*>(pRenderable->GetModel());
			shadowmgr->RemoveAllShadowsFromBrushModel( pModel );
		}
		break;

	case SHADOW_RECEIVER_STATIC_PROP:
		staticpropmgr->RemoveAllShadowsFromStaticProp(pRenderable);
		break;

	case SHADOW_RECEIVER_STUDIO_MODEL:
		if( pRenderable && pRenderable->GetModelInstance() != MODEL_INSTANCE_INVALID )
		{
			shadowmgr->RemoveAllShadowsFromModel( pRenderable->GetModelInstance() );
		}
		break;

//	default:
//		// FIXME: How do deal with this stuff? Add a method to IClientRenderable?
//		C_BaseEntity* pEnt = static_cast<C_BaseEntity*>(pRenderable);
//		pEnt->RemoveAllShadows();
	}
}


//-----------------------------------------------------------------------------
// Computes + sets the render-to-texture texcoords
//-----------------------------------------------------------------------------
void CClientShadowMgr::SetRenderToTextureShadowTexCoords( ShadowHandle_t handle, int x, int y, int w, int h )
{
	// Let the shadow mgr know about the texture coordinates...
	// That way it'll be able to batch rendering better.
	int textureW, textureH;
	m_ShadowAllocator.GetTotalTextureSize( textureW, textureH );

	// Go in a half-pixel to avoid blending with neighboring textures..
	float u, v, du, dv;
#ifndef _XBOX
	u  = ((float)x + 0.5f) / (float)textureW;
	v  = ((float)y + 0.5f) / (float)textureH;
	du = ((float)w - 1) / (float)textureW;
	dv = ((float)h - 1) / (float)textureH;
#else
	// xboxissue - need non-normalized tecture coords
	u  = ((float)x + 0.5f);
	v  = ((float)y + 0.5f);
	du = ((float)w - 1);
	dv = ((float)h - 1);
#endif
	shadowmgr->SetShadowTexCoord( handle, u, v, du, dv );
}


//-----------------------------------------------------------------------------
// Draws all children shadows into our own
//-----------------------------------------------------------------------------
bool CClientShadowMgr::DrawShadowHierarchy( IClientRenderable *pRenderable, const ClientShadow_t &shadow, bool bChild )
{
	bool bDrewTexture = false;

	// Stop traversing when we hit a blobby shadow
	ShadowType_t shadowType = GetActualShadowCastType( pRenderable );
	if ( pRenderable && shadowType == SHADOWS_SIMPLE )
		return false;

	if ( !pRenderable || shadowType != SHADOWS_NONE )
	{
		bool bDrawModelShadow;
		bool bDrawBrushShadow;
		if ( !bChild )
		{
			bDrawModelShadow = ((shadow.m_Flags & SHADOW_FLAGS_BRUSH_MODEL) == 0);
			bDrawBrushShadow = !bDrawModelShadow;
		}
		else
		{
			int nModelType = modelinfo->GetModelType( pRenderable->GetModel() );
			bDrawModelShadow = nModelType == mod_studio;
			bDrawBrushShadow = nModelType == mod_brush;
		}
    
		if ( bDrawModelShadow )
		{
			modelrender->DrawModelShadowEx( pRenderable, pRenderable->GetBody(), pRenderable->GetSkin() );
			bDrewTexture = true;
		}
		else if ( bDrawBrushShadow )
		{
			render->DrawBrushModelShadow( pRenderable );
		}
	}

	if ( !pRenderable )
		return bDrewTexture;

	IClientRenderable *pChild;
	for ( pChild = pRenderable->FirstShadowChild(); pChild; pChild = pChild->NextShadowPeer() )
	{
		if ( DrawShadowHierarchy( pChild, shadow, true ) )
		{
			bDrewTexture = true;
		}
	}
	return bDrewTexture;
}


//-----------------------------------------------------------------------------
// This gets called with every shadow that potentially will need to re-render
//-----------------------------------------------------------------------------
bool CClientShadowMgr::DrawRenderToTextureShadow( unsigned short clientShadowHandle, float flArea )
{
	ClientShadow_t& shadow = m_Shadows[clientShadowHandle];

	// If we were previously using the LOD shadow, set the material
	if ( shadow.m_Flags & SHADOW_FLAGS_USING_LOD_SHADOW )
	{
		shadowmgr->SetShadowMaterial( shadow.m_ShadowHandle, m_RenderShadow, m_RenderModelShadow, (void*)clientShadowHandle );
	}

	// Mark texture as being used...
	bool bDirtyTexture = (shadow.m_Flags & SHADOW_FLAGS_TEXTURE_DIRTY) != 0;
	bool bDrewTexture = false;
	bool needsRedraw = m_ShadowAllocator.UseTexture( shadow.m_ShadowTexture, bDirtyTexture, flArea );
	if (needsRedraw || bDirtyTexture)
	{
		// shadow to be redrawn; for now, we'll always do it.
		IClientRenderable *pRenderable = ClientEntityList().GetClientRenderableFromHandle( shadow.m_Entity );

		// Sets the viewport state
		int x, y, w, h;
		m_ShadowAllocator.GetTextureRect( shadow.m_ShadowTexture, x, y, w, h );
		materials->Viewport( x, y, w, h ); 

		// Clear the selected viewport only
		// GR: don't need to clear depth
		materials->ClearBuffers( true, false );

		materials->MatrixMode( MATERIAL_VIEW );
		materials->LoadMatrix( shadowmgr->GetInfo( shadow.m_ShadowHandle ).m_WorldToShadow );
   
		if ( DrawShadowHierarchy( pRenderable, shadow ) )
		{
			bDrewTexture = true;
		}

		// Only clear the dirty flag if the caster isn't animating
		if ( (shadow.m_Flags & SHADOW_FLAGS_ANIMATING_SOURCE) == 0 )
		{
			shadow.m_Flags &= ~SHADOW_FLAGS_TEXTURE_DIRTY;
		}

		SetRenderToTextureShadowTexCoords( shadow.m_ShadowHandle, x, y, w, h );
	}
	else if ( shadow.m_Flags & SHADOW_FLAGS_USING_LOD_SHADOW )
	{
		// In this case, we were previously using the LOD shadow, but we didn't
		// have to reconstitute the texture. In this case, we need to reset the texcoord
		int x, y, w, h;
		m_ShadowAllocator.GetTextureRect( shadow.m_ShadowTexture, x, y, w, h );
		SetRenderToTextureShadowTexCoords( shadow.m_ShadowHandle, x, y, w, h );
	}

	shadow.m_Flags &= ~SHADOW_FLAGS_USING_LOD_SHADOW;
	return bDrewTexture;
}


//-----------------------------------------------------------------------------
// "Draws" the shadow LOD, which really means just set up the blobby shadow
//-----------------------------------------------------------------------------
void CClientShadowMgr::DrawRenderToTextureShadowLOD( unsigned short clientShadowHandle )
{
	ClientShadow_t &shadow = m_Shadows[clientShadowHandle];
	if ( (shadow.m_Flags & SHADOW_FLAGS_USING_LOD_SHADOW) == 0 )
	{
		shadowmgr->SetShadowMaterial( shadow.m_ShadowHandle, m_SimpleShadow, m_SimpleShadow, (void*)CLIENTSHADOW_INVALID_HANDLE );
		shadowmgr->SetShadowTexCoord( shadow.m_ShadowHandle, 0, 0, 1, 1 );
		ClearExtraClipPlanes( shadow.m_ShadowHandle );
		shadow.m_Flags |= SHADOW_FLAGS_USING_LOD_SHADOW;
	}
}


#define SMALL_OBJECT_FIXUP_FACTOR 10

//-----------------------------------------------------------------------------
// Returns true if the shadow is far enough to want to use blobby shadows
//-----------------------------------------------------------------------------
bool CClientShadowMgr::ShouldUseBlobbyShadows( float flRadius, float flScreenArea )
{
	// Adjust the shadow area up for small objects; we don't want blobby shadows for 
	// really small things if we're real close to them...
	float flMaxFixupRadius = 20;
	float flMinFixupRadius = 6;
	if (flRadius < flMaxFixupRadius)
	{
		if (flRadius >= flMinFixupRadius)
			flScreenArea *= SMALL_OBJECT_FIXUP_FACTOR * (1.0f - (flRadius - flMinFixupRadius) / (flMaxFixupRadius - flMinFixupRadius) ) + 1.0f;
		else
			flScreenArea *= SMALL_OBJECT_FIXUP_FACTOR + 1.0f;
	}

	return (flScreenArea <= m_flMinShadowArea);
}


//-----------------------------------------------------------------------------
// Builds a list of potential shadows that lie within our PVS + view frustum
//-----------------------------------------------------------------------------
struct VisibleShadowInfo_t
{
	 ClientShadowHandle_t m_hShadow;
	 float m_flArea;
	 Vector m_vecAbsCenter;
	 float m_flRadius;
};

class CVisibleShadowList : public IClientLeafShadowEnum
{
public:

	CVisibleShadowList();
	int FindShadows( const CViewSetup *pView, int nLeafCount, LeafIndex_t *pLeafList );
	int GetVisibleShadowCount() const;

	const VisibleShadowInfo_t &GetVisibleShadow( int i ) const;

private:
	void EnumShadow( unsigned short clientShadowHandle );
	float ComputeScreenArea( const Vector &vecCenter, float r ) const;
	void PrioritySort();

	CUtlVector<VisibleShadowInfo_t> m_ShadowsInView;
	CUtlVector<int>	m_PriorityIndex;
};


//-----------------------------------------------------------------------------
// Singleton instance
//-----------------------------------------------------------------------------
static CVisibleShadowList s_VisibleShadowList;



CVisibleShadowList::CVisibleShadowList() : m_ShadowsInView( 0, 64 ), m_PriorityIndex( 0, 64 ) 
{
}


//-----------------------------------------------------------------------------
// Accessors
//-----------------------------------------------------------------------------
int CVisibleShadowList::GetVisibleShadowCount() const
{
	return m_ShadowsInView.Count();
}

const VisibleShadowInfo_t &CVisibleShadowList::GetVisibleShadow( int i ) const
{
	return m_ShadowsInView[m_PriorityIndex[i]];
}


//-----------------------------------------------------------------------------
// Computes approximate screen area of the shadow
//-----------------------------------------------------------------------------
float CVisibleShadowList::ComputeScreenArea( const Vector &vecCenter, float r ) const
{
	float flScreenDiameter = materials->ComputePixelDiameterOfSphere( vecCenter, r );
	return flScreenDiameter * flScreenDiameter;
}


//-----------------------------------------------------------------------------
// Visits every shadow in the list of leaves
//-----------------------------------------------------------------------------
void CVisibleShadowList::EnumShadow( unsigned short clientShadowHandle )
{
	CClientShadowMgr::ClientShadow_t& shadow = s_ClientShadowMgr.m_Shadows[clientShadowHandle];

	// Don't bother if we rendered it this frame, no matter which view it was rendered for
	if (shadow.m_nRenderFrame == gpGlobals->framecount)
		return;

	// We don't need to bother with it
	if ( s_ClientShadowMgr.GetActualShadowCastType( clientShadowHandle ) != SHADOWS_RENDER_TO_TEXTURE )
		return;

	IClientRenderable *pRenderable = ClientEntityList().GetClientRenderableFromHandle( shadow.m_Entity );
	Assert( pRenderable );

	// Don't bother with children of hierarchy; they will be drawn with their parents
	if ( s_ClientShadowMgr.ShouldUseParentShadow( pRenderable ) || s_ClientShadowMgr.WillParentRenderBlobbyShadow( pRenderable ) )
		return;
	
	// Compute a sphere surrounding the shadow
	// FIXME: This doesn't account for children of hierarchy... too bad!
	Vector vecAbsCenter;
	float flRadius;
	s_ClientShadowMgr.ComputeBoundingSphere( pRenderable, vecAbsCenter, flRadius );

	// Compute a box surrounding the shadow
	Vector vecAbsMins, vecAbsMaxs;
	s_ClientShadowMgr.ComputeShadowBBox( pRenderable, vecAbsCenter, flRadius, &vecAbsMins, &vecAbsMaxs );

	// FIXME: Add distance check here?
	
	// Make sure it's in the frustum. If it isn't it's not interesting
	if (engine->CullBox( vecAbsMins, vecAbsMaxs ))
		return;

	int i = m_ShadowsInView.AddToTail( );
	VisibleShadowInfo_t &info = m_ShadowsInView[i];
	info.m_hShadow = clientShadowHandle;
	info.m_vecAbsCenter = vecAbsCenter;
	info.m_flRadius = flRadius;
	m_ShadowsInView[i].m_flArea = ComputeScreenArea( vecAbsCenter, flRadius );

	// Har, har. When water is rendering (or any multipass technique), 
	// we may well initially render from a viewpoint which doesn't include this shadow. 
	// That doesn't mean we shouldn't check it again though. Sucks that we need to compute
	// the sphere + bbox multiply times though.
	shadow.m_nRenderFrame = gpGlobals->framecount;
}


//-----------------------------------------------------------------------------
// Sort based on screen area/priority
//-----------------------------------------------------------------------------
void CVisibleShadowList::PrioritySort()
{
	int nCount = m_ShadowsInView.Count();
	m_PriorityIndex.EnsureCapacity( nCount );

	m_PriorityIndex.RemoveAll();

	int i, j;
	for ( i = 0; i < nCount; ++i )
	{
		m_PriorityIndex.AddToTail(i);
	}

	for ( i = 0; i < nCount - 1; ++i )
	{
		int nLargestInd = i;
		float flLargestArea = m_ShadowsInView[m_PriorityIndex[i]].m_flArea;
		for ( j = i + 1; j < nCount; ++j )
		{
			int nIndex = m_PriorityIndex[j];
			if ( flLargestArea < m_ShadowsInView[nIndex].m_flArea )
			{
				nLargestInd = j;
				flLargestArea = m_ShadowsInView[nIndex].m_flArea;
			}
		}
		swap( m_PriorityIndex[i], m_PriorityIndex[nLargestInd] );
	}
}


//-----------------------------------------------------------------------------
// Main entry point for finding shadows in the leaf list
//-----------------------------------------------------------------------------
int CVisibleShadowList::FindShadows( const CViewSetup *pView, int nLeafCount, LeafIndex_t *pLeafList )
{
	VPROF_BUDGET( "CVisibleShadowList::FindShadows", VPROF_BUDGETGROUP_SHADOW_RENDERING );

	m_ShadowsInView.RemoveAll();
	ClientLeafSystem()->EnumerateShadowsInLeaves( nLeafCount, pLeafList, this );
	int nCount = m_ShadowsInView.Count();
	if (nCount != 0)
	{
		// Sort based on screen area/priority
		PrioritySort();
	}
	return nCount;
}	


//-----------------------------------------------------------------------------
// Advances to the next frame, 
//-----------------------------------------------------------------------------
void CClientShadowMgr::AdvanceFrame()
{
	// We're starting the next frame
	m_ShadowAllocator.AdvanceFrame();
}



//-----------------------------------------------------------------------------
// Re-renders all shadow textures for shadow casters that lie in the leaf list
//-----------------------------------------------------------------------------
void CClientShadowMgr::ComputeShadowTextures( const CViewSetup *pView, int leafCount, LeafIndex_t* pLeafList )
{
	VPROF_BUDGET( "CClientShadowMgr::ComputeShadowTextures", VPROF_BUDGETGROUP_SHADOW_RENDERING );
	
	if ( !m_RenderToTextureActive || (r_shadows.GetInt() == 0) || r_shadows_gamecontrol.GetInt() == 0 )
		return;

	MDLCACHE_CRITICAL_SECTION();
	// First grab all shadow textures we may want to render
	int nCount = s_VisibleShadowList.FindShadows( pView, leafCount, pLeafList );
	if ( nCount == 0 )
		return;

	// FIXME: Add heuristics based on distance, etc. to futz with
	// the shadow allocator + to select blobby shadows

	// Clear to white, black alpha
#ifndef _XBOX
	materials->ClearColor4ub( 255, 255, 255, 0 );
#else
	// xboxissue - using rgb instead
	materials->ClearColor4ub( 0, 0, 0, 255 );
#endif

	// No height clip mode please.
	MaterialHeightClipMode_t oldHeightClipMode = materials->GetHeightClipMode();
	materials->SetHeightClipMode( MATERIAL_HEIGHTCLIPMODE_DISABLE );

	// No projection matrix (orthographic mode)
	// FIXME: Make it work for projective shadows?
	materials->MatrixMode( MATERIAL_PROJECTION );
	materials->PushMatrix();
	materials->LoadIdentity();
	materials->Scale( 1, -1, 1 );
	materials->Ortho( 0, 0, 1, 1, -9999, 0 );

	materials->MatrixMode( MATERIAL_VIEW );
	materials->PushMatrix();

	materials->PushRenderTargetAndViewport( m_ShadowAllocator.GetTexture() );

	if ( m_bRenderTargetNeedsClear )
	{
		// GR - draw with disabled depth buffer
		// GR: don't need to clear depth
		materials->ClearBuffers( true, false );
		m_bRenderTargetNeedsClear = false;
	}

	int nMaxShadows = r_shadowmaxrendered.GetInt();
	int nModelsRendered = 0;
	for (int i = 0; i < nCount; ++i)
	{
		const VisibleShadowInfo_t &info = s_VisibleShadowList.GetVisibleShadow(i);
		if ( (nModelsRendered < nMaxShadows) && ( !IsXbox() || !ShouldUseBlobbyShadows( info.m_flRadius, info.m_flArea ) ) )
		{
			if ( DrawRenderToTextureShadow( info.m_hShadow, info.m_flArea ) )
			{
				++nModelsRendered;
			}
		}
		else
		{
			DrawRenderToTextureShadowLOD( info.m_hShadow );
		}
	}

	// Render to the backbuffer again
	materials->PopRenderTargetAndViewport();

	// Restore the matrices
	materials->MatrixMode( MATERIAL_PROJECTION );
	materials->PopMatrix();

	materials->MatrixMode( MATERIAL_VIEW );
	materials->PopMatrix();

	materials->SetHeightClipMode( oldHeightClipMode );

	materials->SetHeightClipMode( oldHeightClipMode );

	// Restore the clear color
	materials->ClearColor3ub( 0, 0, 0 );
}


bool CClientShadowMgr::AllocateDepthBuffer( CTextureReference &depthBuffer )
{
	if( !m_DepthTextureCache.Count() )
		return false;

	depthBuffer = m_DepthTextureCache[ m_DepthTextureCache.Count()-1 ];
	m_DepthTextureCache.Remove( m_DepthTextureCache.Count()-1 );

	return true;
}	


void CClientShadowMgr::DeallocateDepthBuffer( CTextureReference &depthBuffer )
{
	m_DepthTextureCache.AddToTail( depthBuffer );
	depthBuffer.Shutdown();
}


void CClientShadowMgr::SetFlashlightTarget( ClientShadowHandle_t shadowHandle, EHANDLE targetEntity )
{
	Assert( m_Shadows.IsValidIndex( shadowHandle ) );

	CClientShadowMgr::ClientShadow_t &shadow = m_Shadows[ shadowHandle ];
	if( (shadow.m_Flags&SHADOW_FLAGS_FLASHLIGHT) == 0 )
		return;

//	shadow.m_pTargetRenderable = pRenderable;
	shadow.m_hTargetEntity = targetEntity;
}


void CClientShadowMgr::SetFlashlightLightWorld( ClientShadowHandle_t shadowHandle, bool bLightWorld )
{
	Assert( m_Shadows.IsValidIndex( shadowHandle ) );

	CClientShadowMgr::ClientShadow_t &shadow = m_Shadows[ shadowHandle ];
	if( (shadow.m_Flags&SHADOW_FLAGS_FLASHLIGHT) == 0 )
		return;

	shadow.m_bLightWorld = bLightWorld;
}


bool CClientShadowMgr::IsFlashlightTarget( ClientShadowHandle_t shadowHandle, IClientRenderable *pRenderable )
{
	ClientShadow_t &shadow = m_Shadows[ shadowHandle ];

	if( shadow.m_hTargetEntity->GetClientRenderable() == pRenderable )
		return true;

	C_BaseEntity *pChild = shadow.m_hTargetEntity->FirstMoveChild();
	while( pChild )
	{
		if( pChild->GetClientRenderable()==pRenderable )
			return true;

		pChild = pChild->NextMovePeer();
	}
							
	return false;
}

//-----------------------------------------------------------------------------
// A material proxy that resets the base texture to use the rendered shadow
//-----------------------------------------------------------------------------
class CShadowProxy : public IMaterialProxy
{
public:
	CShadowProxy();
	virtual ~CShadowProxy();
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( void *pProxyData );
	virtual void Release( void ) { delete this; }

private:
	IMaterialVar* m_BaseTextureVar;
};

CShadowProxy::CShadowProxy()
{
	m_BaseTextureVar = NULL;
}

CShadowProxy::~CShadowProxy()
{
}


bool CShadowProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	bool foundVar;
	m_BaseTextureVar = pMaterial->FindVar( "$basetexture", &foundVar, false );
	return foundVar;
}

void CShadowProxy::OnBind( void *pProxyData )
{
	unsigned short clientShadowHandle = ( unsigned short )pProxyData;
	ITexture* pTex = s_ClientShadowMgr.GetShadowTexture( clientShadowHandle );
	if ((m_BaseTextureVar->GetType() != MATERIAL_VAR_TYPE_TEXTURE) || 
		(pTex != m_BaseTextureVar->GetTextureValue()))
	{
		m_BaseTextureVar->SetTextureValue( pTex );
	}
}

EXPOSE_INTERFACE( CShadowProxy, IMaterialProxy, "Shadow" IMATERIAL_PROXY_INTERFACE_VERSION );



//-----------------------------------------------------------------------------
// A material proxy that resets the base texture to use the rendered shadow
//-----------------------------------------------------------------------------
class CShadowModelProxy : public IMaterialProxy
{
public:
	CShadowModelProxy();
	virtual ~CShadowModelProxy();
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( void *pProxyData );
	virtual void Release( void ) { delete this; }

private:
	IMaterialVar* m_BaseTextureVar;
	IMaterialVar* m_BaseTextureOffsetVar;
	IMaterialVar* m_BaseTextureScaleVar;
	IMaterialVar* m_BaseTextureMatrixVar;
	IMaterialVar* m_FalloffOffsetVar;
	IMaterialVar* m_FalloffDistanceVar;
	IMaterialVar* m_FalloffAmountVar;
};

CShadowModelProxy::CShadowModelProxy()
{
	m_BaseTextureVar = NULL;
	m_BaseTextureOffsetVar = NULL;
	m_BaseTextureScaleVar = NULL;
	m_BaseTextureMatrixVar = NULL;
	m_FalloffOffsetVar = NULL;
	m_FalloffDistanceVar = NULL;
	m_FalloffAmountVar = NULL;
}

CShadowModelProxy::~CShadowModelProxy()
{
}


bool CShadowModelProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	bool foundVar;
	m_BaseTextureVar = pMaterial->FindVar( "$basetexture", &foundVar, false );
	if (!foundVar)
		return false;
	m_BaseTextureOffsetVar = pMaterial->FindVar( "$basetextureoffset", &foundVar, false );
	if (!foundVar)
		return false;
	m_BaseTextureScaleVar = pMaterial->FindVar( "$basetexturescale", &foundVar, false );
	if (!foundVar)
		return false;
	m_BaseTextureMatrixVar = pMaterial->FindVar( "$basetexturetransform", &foundVar, false );
	if (!foundVar)
		return false;
	m_FalloffOffsetVar = pMaterial->FindVar( "$falloffoffset", &foundVar, false );
	if (!foundVar)
		return false;
	m_FalloffDistanceVar = pMaterial->FindVar( "$falloffdistance", &foundVar, false );
	if (!foundVar)
		return false;
	m_FalloffAmountVar = pMaterial->FindVar( "$falloffamount", &foundVar, false );
	return foundVar;
}

void CShadowModelProxy::OnBind( void *pProxyData )
{
	unsigned short clientShadowHandle = ( unsigned short )pProxyData;
	ITexture* pTex = s_ClientShadowMgr.GetShadowTexture( clientShadowHandle );
	m_BaseTextureVar->SetTextureValue( pTex );

	const ShadowInfo_t& info = s_ClientShadowMgr.GetShadowInfo( clientShadowHandle );
	m_BaseTextureMatrixVar->SetMatrixValue( info.m_WorldToShadow );
	m_BaseTextureOffsetVar->SetVecValue( info.m_TexOrigin.Base(), 2 );
	m_BaseTextureScaleVar->SetVecValue( info.m_TexSize.Base(), 2 );
	m_FalloffOffsetVar->SetFloatValue( info.m_FalloffOffset );
	m_FalloffDistanceVar->SetFloatValue( info.m_MaxDist );
	m_FalloffAmountVar->SetFloatValue( info.m_FalloffAmount );
}

EXPOSE_INTERFACE( CShadowModelProxy, IMaterialProxy, "ShadowModel" IMATERIAL_PROXY_INTERFACE_VERSION );




   


   
