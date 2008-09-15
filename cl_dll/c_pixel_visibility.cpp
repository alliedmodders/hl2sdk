//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "c_pixel_visibility.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterial.h"
#include "ClientEffectPrecacheSystem.h"
#include "view.h"
#include "utlmultilist.h"

static void PixelvisDrawChanged( ConVar *pPixelvisVar, const char *pOld );

ConVar r_pixelvisibility_partial( "r_pixelvisibility_partial", "1" );
ConVar r_dopixelvisibility( "r_dopixelvisibility", "1" );
ConVar r_drawpixelvisibility( "r_drawpixelvisibility", "0", 0, "Show the occlusion proxies", PixelvisDrawChanged );
ConVar r_pixelvisibility_spew( "r_pixelvisibility_spew", "0" );

extern ConVar building_cubemaps;

const float MIN_PROXY_PIXELS = 5.0f;

float PixelVisibility_DrawProxy( OcclusionQueryObjectHandle_t queryHandle, Vector origin, float scale, float proxyAspect, IMaterial *pMaterial, bool screenspace )
{
	Vector point;

	// don't expand this with distance to fit pixels or the sprite will poke through
	// only expand the parts perpendicular to the view
	float forwardScale = scale;
	// draw a pyramid of points touching a sphere of radius "scale" at origin
	float pixelsPerUnit = materials->ComputePixelWidthOfSphere( origin, 1.0f );
	pixelsPerUnit = max( pixelsPerUnit, 1e-4f );
	if ( screenspace )
	{
		// Force this to be the size of a sphere of diameter "scale" at some reference distance (1.0 unit)
		float pixelsPerUnit2 = materials->ComputePixelWidthOfSphere( CurrentViewOrigin() + CurrentViewForward()*1.0f, scale*0.5f );
		// force drawing of "scale" pixels
		scale = pixelsPerUnit2 / pixelsPerUnit;
	}
	else
	{
		float pixels = scale * pixelsPerUnit;
		
		// make the radius larger to ensure a minimum screen space size of the proxy geometry
		if ( pixels < MIN_PROXY_PIXELS )
		{
			scale = MIN_PROXY_PIXELS / pixelsPerUnit;
		}
	}

	// collapses the pyramid to a plane - so this could be a quad instead
	Vector dir = origin - CurrentViewOrigin();
	VectorNormalize(dir);
	origin -= dir * forwardScale;
	forwardScale = 0.0f;
	// 

	Vector verts[5];
	const float sqrt2 = 0.707106781f; // sqrt(2) - keeps all vectors the same length from origin
	scale *= sqrt2;
	float scale45x = scale;
	float scale45y = scale / proxyAspect;
	verts[0] = origin - CurrentViewForward() * forwardScale;					  // the apex of the pyramid
	verts[1] = origin + CurrentViewUp() * scale45y - CurrentViewRight() * scale45x; // these four form the base
	verts[2] = origin + CurrentViewUp() * scale45y + CurrentViewRight() * scale45x; // the pyramid is a sprite with a point that
	verts[3] = origin - CurrentViewUp() * scale45y + CurrentViewRight() * scale45x; // pokes back toward the camera through any nearby 
	verts[4] = origin - CurrentViewUp() * scale45y - CurrentViewRight() * scale45x; // geometry

	// get screen coords of edges
	Vector screen[4];
	for ( int i = 0; i < 4; i++ )
	{
		extern int ScreenTransform( const Vector& point, Vector& screen );
		if ( ScreenTransform( verts[i+1], screen[i] ) )
			return 0;
	}

	// compute area and screen-clipped area
	float w = screen[1].x - screen[0].x;
	float h = screen[0].y - screen[3].y;
	float ws = min(1.0f, screen[1].x) - max(-1.0f, screen[0].x);
	float hs = min(1.0f, screen[0].y) - max(-1.0f, screen[3].y);
	float area = w*h; // area can be zero when we ALT-TAB
	float areaClipped = ws*hs;
	float ratio = 0.0f;
	if ( area != 0 )
	{
		// compute the ratio of the area not clipped by the frustum to total area
		ratio = areaClipped / area;
		ratio = clamp(ratio, 0.0f, 1.0f);
	}

	IMesh* pMesh = materials->GetDynamicMesh( true, NULL, NULL, pMaterial );

	CMeshBuilder meshBuilder;
	materials->BeginOcclusionQueryDrawing( queryHandle );
	meshBuilder.Begin( pMesh, MATERIAL_TRIANGLES, 4 );
	// draw a pyramid
	for ( int i = 0; i < 4; i++ )
	{
		int a = i+1;
		int b = (a%4)+1;
		meshBuilder.Position3fv( verts[0].Base() );
		meshBuilder.AdvanceVertex();
		meshBuilder.Position3fv( verts[a].Base() );
		meshBuilder.AdvanceVertex();
		meshBuilder.Position3fv( verts[b].Base() );
		meshBuilder.AdvanceVertex();
	}
	meshBuilder.End();
	pMesh->Draw();

	// sprite/quad proxy
#if 0
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	VectorMA (origin, -scale, CurrentViewUp(), point);
	VectorMA (point, -scale, CurrentViewRight(), point);
	meshBuilder.Position3fv (point.Base());
	meshBuilder.AdvanceVertex();

	VectorMA (origin, scale, CurrentViewUp(), point);
	VectorMA (point, -scale, CurrentViewRight(), point);
	meshBuilder.Position3fv (point.Base());
	meshBuilder.AdvanceVertex();

	VectorMA (origin, scale, CurrentViewUp(), point);
	VectorMA (point, scale, CurrentViewRight(), point);
	meshBuilder.Position3fv (point.Base());
	meshBuilder.AdvanceVertex();

	VectorMA (origin, -scale, CurrentViewUp(), point);
	VectorMA (point, scale, CurrentViewRight(), point);
	meshBuilder.Position3fv (point.Base());
	meshBuilder.AdvanceVertex();
	
	meshBuilder.End();
	pMesh->Draw();
#endif
	materials->EndOcclusionQueryDrawing( queryHandle );

	// fraction clipped by frustum
	return ratio;
}

class CPixelVisSet
{
public:
	void Init( const pixelvis_queryparams_t &params );
	void MarkActive();
	bool IsActive();
	CPixelVisSet()
	{
		frameIssued = 0;
		serial = 0;
		queryList = 0xFFFF;
		sizeIsScreenSpace = false;
	}

public:
	float			proxySize;
	float			proxyAspect;
	float			fadeTimeInv;
	unsigned short	queryList;
	unsigned short	serial;
	bool			sizeIsScreenSpace;
private:
	int				frameIssued;
};


void CPixelVisSet::Init( const pixelvis_queryparams_t &params )
{
	Assert( params.bSetup );
	proxySize = params.proxySize;
	proxyAspect = params.proxyAspect;
	if ( params.fadeTime > 0.0f )
	{
		fadeTimeInv = 1.0f / params.fadeTime;
	}
	else
	{
		// fade in over 0.125 seconds
		fadeTimeInv = 1.0f / 0.125f;
	}
	frameIssued = 0;
	sizeIsScreenSpace = params.bSizeInScreenspace;
}

void CPixelVisSet::MarkActive()
{
	frameIssued = gpGlobals->framecount;
}

bool CPixelVisSet::IsActive()
{
	return (gpGlobals->framecount - frameIssued) > 1 ? false : true;
}


class CPixelVisibilityQuery
{
public:
	CPixelVisibilityQuery();
	~CPixelVisibilityQuery();
	bool IsValid();
	bool IsForView( int viewID );
	bool IsActive();
	float GetFractionVisible( float fadeTimeInv );
	void IssueQuery( float proxySize, float proxyAspect, IMaterial *pMaterial, bool sizeIsScreenSpace );
	void IssueCountingQuery( float proxySize, float proxyAspect, IMaterial *pMaterial, bool sizeIsScreenSpace );
	void SetView( int viewID ) 
	{ 
		m_viewID = viewID;	
		m_brightnessTarget = 0.0f;
		m_clipFraction = 1.0f;
		m_frameIssued = -1;
		m_failed = false;
		m_wasQueriedThisFrame = false;
		m_hasValidQueryResults = false;
	}

public:
	Vector							m_origin;
	int								m_frameIssued;
private:
	float							m_brightnessTarget;
	float							m_clipFraction;
	OcclusionQueryObjectHandle_t	m_queryHandle;
	OcclusionQueryObjectHandle_t	m_queryHandleCount;
	unsigned short					m_wasQueriedThisFrame : 1;
	unsigned short					m_failed : 1;
	unsigned short					m_hasValidQueryResults : 1;
	unsigned short					m_pad : 13;
	unsigned short					m_viewID;
};

CPixelVisibilityQuery::CPixelVisibilityQuery()
{
	SetView( 0xFFFF );
	m_queryHandle = materials->CreateOcclusionQueryObject();
	m_queryHandleCount = materials->CreateOcclusionQueryObject();
}

CPixelVisibilityQuery::~CPixelVisibilityQuery()
{
	if ( m_queryHandle != INVALID_OCCLUSION_QUERY_OBJECT_HANDLE )
	{
		materials->DestroyOcclusionQueryObject( m_queryHandle );
	}
	if ( m_queryHandleCount != INVALID_OCCLUSION_QUERY_OBJECT_HANDLE )
	{
		materials->DestroyOcclusionQueryObject( m_queryHandleCount );
	}
}

bool CPixelVisibilityQuery::IsValid()
{
	return (m_queryHandle != INVALID_OCCLUSION_QUERY_OBJECT_HANDLE) ? true : false;
}
bool CPixelVisibilityQuery::IsForView( int viewID ) 
{ 
	return m_viewID == viewID ? true : false; 
}

bool CPixelVisibilityQuery::IsActive()
{
	return (gpGlobals->framecount - m_frameIssued) > 1 ? false : true;
}

float CPixelVisibilityQuery::GetFractionVisible( float fadeTimeInv )
{
	if ( !IsValid() )
		return 0.0f;

	if ( !m_wasQueriedThisFrame )
	{
		m_wasQueriedThisFrame = true;
		int pixels = -1;
		int pixelsPossible = -1;
		if ( r_pixelvisibility_partial.GetBool() )
		{
			if ( m_frameIssued != -1 )
			{
				pixelsPossible = materials->OcclusionQuery_GetNumPixelsRendered( m_queryHandleCount );
				pixels = materials->OcclusionQuery_GetNumPixelsRendered( m_queryHandle );
			}

			if ( r_pixelvisibility_spew.GetBool() && CurrentViewID() == 0 ) 
			{
				DevMsg( 1, "Pixels visible: %d (qh:%d) Pixels possible: %d (qh:%d) (frame:%d)\n", pixels, m_queryHandle, pixelsPossible, m_queryHandleCount, gpGlobals->framecount );
			}

			if ( pixels < 0 || pixelsPossible < 0 )
			{
				m_failed = m_frameIssued != -1 ? true : false;
				return m_brightnessTarget * m_clipFraction;
			}
			m_hasValidQueryResults = true;

			if ( pixelsPossible > 0 )
			{
				float target = (float)pixels / (float)pixelsPossible;
				target = (target >= 0.95f) ? 1.0f : (target < 0.0f) ? 0.0f : target;
				float rate = gpGlobals->frametime * fadeTimeInv;
				m_brightnessTarget = Approach( target, m_brightnessTarget, rate ); // fade in / out
			}
			else
			{
				m_brightnessTarget = 0.0f;
			}
		}
		else
		{
			if ( m_frameIssued != -1 )
			{
				pixels = materials->OcclusionQuery_GetNumPixelsRendered( m_queryHandle );
			}

			if ( r_pixelvisibility_spew.GetBool() && CurrentViewID() == 0 ) 
			{
				DevMsg( 1, "Pixels visible: %d (qh:%d) (frame:%d)\n", pixels, m_queryHandle, gpGlobals->framecount );
			}

			if ( pixels < 0 )
			{
				m_failed = m_frameIssued != -1 ? true : false;
				return m_brightnessTarget * m_clipFraction;
			}
			m_hasValidQueryResults = true;
			if ( m_frameIssued == gpGlobals->framecount-1 )
			{
				float rate = gpGlobals->frametime * fadeTimeInv;
				float target = 0.0f;
				if ( pixels > 0 )
				{
					// fade in slower than you fade out
					rate *= 0.5f;
					target = 1.0f;
				}
				m_brightnessTarget = Approach( target, m_brightnessTarget, rate ); // fade in / out
			}
			else
			{
				m_brightnessTarget = 0.0f;
			}
		}
	}

	return m_brightnessTarget * m_clipFraction;
}

void CPixelVisibilityQuery::IssueQuery( float proxySize, float proxyAspect, IMaterial *pMaterial, bool sizeIsScreenSpace )
{
	if ( !m_failed )
	{
		Assert( IsValid() );

		if ( r_pixelvisibility_spew.GetBool() && CurrentViewID() == 0 ) 
		{
			DevMsg( 1, "Draw Proxy: qh:%d org:<%d,%d,%d> (frame:%d)\n", m_queryHandle, (int)m_origin[0], (int)m_origin[1], (int)m_origin[2], gpGlobals->framecount );
		}

		m_clipFraction = PixelVisibility_DrawProxy( m_queryHandle, m_origin, proxySize, proxyAspect, pMaterial, sizeIsScreenSpace );


	}
	m_frameIssued = gpGlobals->framecount;
	m_wasQueriedThisFrame = false;
	m_failed = false;
}

void CPixelVisibilityQuery::IssueCountingQuery( float proxySize, float proxyAspect, IMaterial *pMaterial, bool sizeIsScreenSpace )
{
	if ( !m_failed )
	{
		Assert( IsValid() );
#if 0
		// this centers it on the screen.
		// This is nice because it makes the glows fade as they get partially clipped by the view frustum
		// But it introduces sub-pixel errors (off by one row/column of pixels) so the glows shimmer
		// UNDONE: Compute an offset center coord that matches sub-pixel coords with the real glow position
		// UNDONE: Or frustum clip the sphere/geometry and fade based on proxy size
		Vector origin = m_origin - CurrentViewOrigin();
		float dot = DotProduct(CurrentViewForward(), origin);
		origin = CurrentViewOrigin() + dot * CurrentViewForward();
#endif
		PixelVisibility_DrawProxy( m_queryHandleCount, m_origin, proxySize, proxyAspect, pMaterial, sizeIsScreenSpace );
	}
}


//Precahce the effects
CLIENTEFFECT_REGISTER_BEGIN( PrecacheOcclusionProxy )
CLIENTEFFECT_MATERIAL( "engine/occlusionproxy" )
CLIENTEFFECT_MATERIAL( "engine/occlusionproxy_countdraw" )
CLIENTEFFECT_REGISTER_END()

class CPixelVisibilitySystem : public CAutoGameSystem
{
public:
	
	// GameSystem: Level init, shutdown
	virtual void LevelInitPreEntity();
	virtual void LevelShutdownPostEntity();

	// locals
	CPixelVisibilitySystem();
	float GetFractionVisible( const pixelvis_queryparams_t &params, pixelvis_handle_t *queryHandle );
	void EndView();
	void EndScene();
	unsigned short FindQueryForView( CPixelVisSet *pSet, int viewID );
	unsigned short FindOrCreateQueryForView( CPixelVisSet *pSet, int viewID );

	void DeleteUnusedQueries( CPixelVisSet *pSet, bool bDeleteAll );
	void DeleteUnusedSets( bool bDeleteAll );
	void ShowQueries( bool show );
	unsigned short AllocQuery();
	unsigned short AllocSet();
	void FreeSet( unsigned short node );
	CPixelVisSet *FindOrCreatePixelVisSet( const pixelvis_queryparams_t &params, pixelvis_handle_t *queryHandle );
	bool SupportsOcclusion() { return m_hwCanTestGlows; }
	void DebugInfo()
	{
		Msg("Pixel vis system using %d sets total (%d in free list), %d queries total (%d in free list)\n", 
			m_setList.TotalCount(), m_setList.Count(m_freeSetsList), m_queryList.TotalCount(), m_queryList.Count( m_freeQueriesList ) );
	}
private:
	CUtlMultiList< CPixelVisSet, unsigned short >	m_setList;
	CUtlMultiList<CPixelVisibilityQuery, unsigned short> m_queryList;
	unsigned short m_freeQueriesList;
	unsigned short m_activeSetsList;
	unsigned short m_freeSetsList;
	unsigned short m_pad0;

	IMaterial	*m_pProxyMaterial;
	IMaterial	*m_pDrawMaterial;
	bool		m_hwCanTestGlows;
	bool		m_drawQueries;
};

static CPixelVisibilitySystem g_PixelVisibilitySystem;

CPixelVisibilitySystem::CPixelVisibilitySystem() : CAutoGameSystem( "CPixelVisibilitySystem" )
{
	m_hwCanTestGlows = true;
	m_drawQueries = false;
}
// Level init, shutdown
void CPixelVisibilitySystem::LevelInitPreEntity()
{
	m_hwCanTestGlows = r_dopixelvisibility.GetBool() && engine->GetDXSupportLevel() >= 80;
	if ( m_hwCanTestGlows )
	{
		unsigned short query = materials->CreateOcclusionQueryObject();
		if ( query != INVALID_OCCLUSION_QUERY_OBJECT_HANDLE )
		{
			materials->DestroyOcclusionQueryObject( query );
		}
		else
		{
			m_hwCanTestGlows = false;
		}
	}

	m_pProxyMaterial = materials->FindMaterial("engine/occlusionproxy", TEXTURE_GROUP_CLIENT_EFFECTS);
	m_pProxyMaterial->IncrementReferenceCount();
	m_pDrawMaterial = materials->FindMaterial("engine/occlusionproxy_countdraw", TEXTURE_GROUP_CLIENT_EFFECTS);
	m_pDrawMaterial->IncrementReferenceCount();
	m_freeQueriesList = m_queryList.CreateList();
	m_activeSetsList = m_setList.CreateList();
	m_freeSetsList = m_setList.CreateList();
}

void CPixelVisibilitySystem::LevelShutdownPostEntity()
{
	m_pProxyMaterial->DecrementReferenceCount();
	m_pProxyMaterial = NULL;
	m_pDrawMaterial->DecrementReferenceCount();
	m_pDrawMaterial = NULL;
	DeleteUnusedSets(true);
	m_setList.Purge();
	m_queryList.Purge();
	m_freeQueriesList = m_queryList.InvalidIndex();
	m_activeSetsList = m_setList.InvalidIndex();
	m_freeSetsList = m_setList.InvalidIndex();
}

float CPixelVisibilitySystem::GetFractionVisible( const pixelvis_queryparams_t &params, pixelvis_handle_t *queryHandle )
{
	if ( !m_hwCanTestGlows || building_cubemaps.GetBool() )
	{
		return GlowSightDistance( params.position, true ) > 0 ? 1.0f : 0.0f;
	}
	if ( CurrentViewID() < 0 )
		return 0.0f;

	CPixelVisSet *pSet = FindOrCreatePixelVisSet( params, queryHandle );
	Assert( pSet );
	unsigned short node = FindOrCreateQueryForView( pSet, CurrentViewID() );
	m_queryList[node].m_origin = params.position;
	float fraction = m_queryList[node].GetFractionVisible( pSet->fadeTimeInv );
	pSet->MarkActive();


	return fraction;
}

void CPixelVisibilitySystem::EndView()
{
	if ( !PixelVisibility_IsAvailable() && CurrentViewID() >= 0 )
		return;
	
	if ( m_setList.Head( m_activeSetsList ) == m_setList.InvalidIndex() )
		return;

	IMaterial *pProxy = m_drawQueries ? m_pDrawMaterial : m_pProxyMaterial;
	materials->Bind( pProxy );

	// BUGBUG: If you draw both queries, the measure query fails for some reason.
	if ( r_pixelvisibility_partial.GetBool() && !m_drawQueries )
	{
		materials->DepthRange( 0.0f, 0.01f );
		unsigned short node = m_setList.Head( m_activeSetsList );
		while( node != m_setList.InvalidIndex() )
		{
			CPixelVisSet *pSet = &m_setList[node];
			unsigned short queryNode = FindQueryForView( pSet, CurrentViewID() );
			if ( queryNode != m_queryList.InvalidIndex() )
			{
				m_queryList[queryNode].IssueCountingQuery( pSet->proxySize, pSet->proxyAspect, pProxy, pSet->sizeIsScreenSpace );
			}
			node = m_setList.Next( node );
		}
		materials->DepthRange( 0.0f, 1.0f );
	}

	{
		unsigned short node = m_setList.Head( m_activeSetsList );
		while( node != m_setList.InvalidIndex() )
		{
			CPixelVisSet *pSet = &m_setList[node];
			unsigned short queryNode = FindQueryForView( pSet, CurrentViewID() );
			if ( queryNode != m_queryList.InvalidIndex() )
			{
				m_queryList[queryNode].IssueQuery( pSet->proxySize, pSet->proxyAspect, pProxy, pSet->sizeIsScreenSpace );
			}
			node = m_setList.Next( node );
		}
	}
}

void CPixelVisibilitySystem::EndScene()
{
	DeleteUnusedSets(false);
}

unsigned short CPixelVisibilitySystem::FindQueryForView( CPixelVisSet *pSet, int viewID )
{
	unsigned short node = m_queryList.Head( pSet->queryList );
	while ( node != m_queryList.InvalidIndex() )
	{
		if ( m_queryList[node].IsForView( viewID ) )
			return node;
		node = m_queryList.Next( node );
	}
	return m_queryList.InvalidIndex();
}
unsigned short CPixelVisibilitySystem::FindOrCreateQueryForView( CPixelVisSet *pSet, int viewID )
{
	unsigned short node = FindQueryForView( pSet, viewID );
	if ( node != m_queryList.InvalidIndex() )
		return node;

	node = AllocQuery();
	m_queryList.LinkToHead( pSet->queryList, node );
	m_queryList[node].SetView( viewID );
	return node;
}


void CPixelVisibilitySystem::DeleteUnusedQueries( CPixelVisSet *pSet, bool bDeleteAll )
{
	unsigned short node = m_queryList.Head( pSet->queryList );
	while ( node != m_queryList.InvalidIndex() )
	{
		unsigned short next = m_queryList.Next( node );
		if ( bDeleteAll || !m_queryList[node].IsActive() )
		{
			m_queryList.Unlink( pSet->queryList, node);
			m_queryList.LinkToHead( m_freeQueriesList, node );
		}
		node = next;
	}
}
void CPixelVisibilitySystem::DeleteUnusedSets( bool bDeleteAll )
{
	unsigned short node = m_setList.Head( m_activeSetsList );
	while ( node != m_setList.InvalidIndex() )
	{
		unsigned short next = m_setList.Next( node );
		CPixelVisSet *pSet = &m_setList[node];
		if ( bDeleteAll || !m_setList[node].IsActive() )
		{
			DeleteUnusedQueries( pSet, true );
		}
		else
		{
			DeleteUnusedQueries( pSet, false );
		}
		if ( m_queryList.Head(pSet->queryList) == m_queryList.InvalidIndex() )
		{
			FreeSet( node );
		}
		node = next;
	}
}

void CPixelVisibilitySystem::ShowQueries( bool show )
{
	m_drawQueries = show;
}

unsigned short CPixelVisibilitySystem::AllocQuery()
{
	unsigned short node = m_queryList.Head(m_freeQueriesList);
	if ( node != m_queryList.InvalidIndex() )
	{
		m_queryList.Unlink( m_freeQueriesList, node );
	}
	else
	{
		node = m_queryList.Alloc();
	}
	return node;
}

unsigned short CPixelVisibilitySystem::AllocSet()
{
	unsigned short node = m_setList.Head(m_freeSetsList);
	if ( node != m_setList.InvalidIndex() )
	{
		m_setList.Unlink( m_freeSetsList, node );
	}
	else
	{
		node = m_setList.Alloc();
		m_setList[node].queryList = m_queryList.CreateList();
	}
	m_setList.LinkToHead( m_activeSetsList, node );
	return node;
}

void CPixelVisibilitySystem::FreeSet( unsigned short node )
{
	m_setList.Unlink( m_activeSetsList, node );
	m_setList.LinkToHead( m_freeSetsList, node );
	m_setList[node].serial++;
}

CPixelVisSet *CPixelVisibilitySystem::FindOrCreatePixelVisSet( const pixelvis_queryparams_t &params, pixelvis_handle_t *queryHandle )
{
	if ( queryHandle[0] )
	{
		unsigned short handle = queryHandle[0] & 0xFFFF;
		handle--;
		unsigned short serial = queryHandle[0] >> 16;
		if ( m_setList.IsValidIndex(handle) && m_setList[handle].serial == serial )
		{
			return &m_setList[handle];
		}
	}

	unsigned short node = AllocSet();
	m_setList[node].Init( params );
	unsigned int out = m_setList[node].serial;
	unsigned short nodeHandle = node + 1;
	out <<= 16;
	out |= nodeHandle;
	queryHandle[0] = out;
	return &m_setList[node];
}


void PixelvisDrawChanged( ConVar *pPixelvisVar, const char *pOld )
{
	g_PixelVisibilitySystem.ShowQueries( pPixelvisVar->GetBool() );
}

class CTraceFilterGlow : public CTraceFilterSimple
{
public:
	DECLARE_CLASS( CTraceFilterGlow, CTraceFilterSimple );
	
	CTraceFilterGlow( const IHandleEntity *passentity, int collisionGroup ) : CTraceFilterSimple(passentity, collisionGroup) {}
	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		IClientUnknown *pUnk = (IClientUnknown*)pHandleEntity;
		ICollideable *pCollide = pUnk->GetCollideable();
		if ( pCollide->GetSolid() != SOLID_VPHYSICS && pCollide->GetSolid() != SOLID_BSP )
			return false;
		return BaseClass::ShouldHitEntity( pHandleEntity, contentsMask );
	}
};
float GlowSightDistance( const Vector &glowOrigin, bool bShouldTrace )
{
	float dist = (glowOrigin - CurrentViewOrigin()).Length();
	C_BasePlayer *local = C_BasePlayer::GetLocalPlayer();
	if ( local )
	{
		dist *= local->GetFOVDistanceAdjustFactor();
	}

	if ( bShouldTrace )
	{
		Vector end = glowOrigin;
		// HACKHACK: trace 4" from destination in case the glow is inside some parent object
		//			allow a little error...
		if ( dist > 4 )
		{
			end -= CurrentViewForward()*4;
		}
		int traceFlags = MASK_OPAQUE|CONTENTS_MONSTER|CONTENTS_DEBRIS;
		
		CTraceFilterGlow filter(NULL, COLLISION_GROUP_NONE);
		trace_t tr;
		UTIL_TraceLine( CurrentViewOrigin(), end, traceFlags, &filter, &tr );
		if ( tr.fraction != 1.0f )
			return -1;
	}

	return dist;
}

void PixelVisibility_EndCurrentView()
{
	g_PixelVisibilitySystem.EndView();
}

void PixelVisibility_EndScene()
{
	g_PixelVisibilitySystem.EndScene();
}

float PixelVisibility_FractionVisible( const pixelvis_queryparams_t &params, pixelvis_handle_t *queryHandle )
{
	if ( !queryHandle )
	{
		return GlowSightDistance( params.position, true ) > 0.0f ? 1.0f : 0.0f;
	}
	else 
	{
		return g_PixelVisibilitySystem.GetFractionVisible( params, queryHandle );
	}
}

bool PixelVisibility_IsAvailable()
{
	return r_dopixelvisibility.GetBool() && g_PixelVisibilitySystem.SupportsOcclusion();
}

CON_COMMAND( pixelvis_debug, "Dump debug info" )
{
	g_PixelVisibilitySystem.DebugInfo();
}
