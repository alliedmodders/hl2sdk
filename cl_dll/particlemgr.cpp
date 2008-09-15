//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//


#include "cbase.h"
#include "particlemgr.h"
#include "particledraw.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterialvar.h"
#include "mempool.h"
#include "IClientMode.h"
#include "view_scene.h"
#include "tier0/vprof.h"
#include "engine/ivdebugoverlay.h"
#include "view.h"
#include "keyvalues.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

int			g_nParticlesDrawn;
// CCycleCount	g_ParticleTimer;

static ConVar r_DrawParticles("r_drawparticles", "1", FCVAR_CHEAT, "Enable/disable particle rendering");
static ConVar particle_simulateoverflow( "particle_simulateoverflow", "0", FCVAR_CHEAT, "Used for stress-testing particle systems. Randomly denies creation of particles." );
static ConVar cl_particleeffect_aabb_buffer( "cl_particleeffect_aabb_buffer", "2", FCVAR_CHEAT, "Add this amount to a particle effect's bbox in the leaf system so if it's growing slowly, it won't have to be reinserted as often." );
static ConVar cl_particles_show_bbox( "cl_particles_show_bbox", "0", FCVAR_CHEAT );

#define BUCKET_SORT_EVERY_N		8			// It does a bucket sort for each material approximately every N times.
#define BBOX_UPDATE_EVERY_N		8			// It does a full bbox update (checks all particles instead of every eighth one).


//-----------------------------------------------------------------------------
//
// Particle manager implementation
//
//-----------------------------------------------------------------------------

#define PARTICLE_SIZE	96

CParticleMgr *ParticleMgr()
{
	static CParticleMgr s_ParticleMgr;
	return &s_ParticleMgr;
}

//-----------------------------------------------------------------------------
// Particle implementation
//-----------------------------------------------------------------------------

void Particle::ToolRecordParticle( KeyValues *msg )
{
	msg->SetPtr( "material", m_pSubTexture );
	msg->SetFloat( "posx", m_Pos.x );
	msg->SetFloat( "posy", m_Pos.y );
	msg->SetFloat( "posz", m_Pos.z );
}


//-----------------------------------------------------------------------------
// CParticleSubTextureGroup implementation.
//-----------------------------------------------------------------------------

CParticleSubTextureGroup::CParticleSubTextureGroup()
{
	m_pPageMaterial = NULL;
}


CParticleSubTextureGroup::~CParticleSubTextureGroup()
{
}


//-----------------------------------------------------------------------------
// CParticleSubTexture implementation.
//-----------------------------------------------------------------------------

CParticleSubTexture::CParticleSubTexture()
{
	m_tCoordMins[0] = m_tCoordMins[0] = 0;
	m_tCoordMaxs[0] = m_tCoordMaxs[0] = 1;
	m_pGroup = &m_DefaultGroup;
	m_pMaterial = NULL;
}


//-----------------------------------------------------------------------------
// CEffectMaterial.
//-----------------------------------------------------------------------------

CEffectMaterial::CEffectMaterial()
{
	m_Particles.m_pNext = m_Particles.m_pPrev = &m_Particles;
	m_pGroup = NULL;
}

					
//-----------------------------------------------------------------------------
// CParticleEffectBinding.
//-----------------------------------------------------------------------------
CParticleEffectBinding::CParticleEffectBinding()
{
	m_pParticleMgr = NULL;
	m_pSim = NULL;

	m_LocalSpaceTransform.Identity();
	m_bLocalSpaceTransformIdentity = true;
	
	m_Flags = 0;
	SetAutoUpdateBBox( true );
	SetFirstFrameFlag( true );
	SetNeedsBBoxUpdate( true );
	SetAlwaysSimulate( true );
	SetEffectCameraSpace( true );
	SetDrawThruLeafSystem( true );
	SetAutoApplyLocalTransform( true );

	// default bbox
	m_Min.Init( -50, -50, -50 );
	m_Max.Init( 50, 50, 50 );

	m_LastMin = m_Min;
	m_LastMax = m_Max;

	SetParticleCullRadius( 0.0f );
	m_nActiveParticles = 0;

	m_FrameCode = 0;
	m_ListIndex = 0xFFFF; 

	m_UpdateBBoxCounter = 0;

	memset( m_EffectMaterialHash, 0, sizeof( m_EffectMaterialHash ) );
}


CParticleEffectBinding::~CParticleEffectBinding()
{
	if( m_pParticleMgr )
		m_pParticleMgr->RemoveEffect( this );

	Term();
}

// The is the max size of the particles for use in bounding	computation
void CParticleEffectBinding::SetParticleCullRadius( float flMaxParticleRadius )
{
	if ( m_flParticleCullRadius != flMaxParticleRadius )
	{
		m_flParticleCullRadius = flMaxParticleRadius;

		if ( m_hRenderHandle != INVALID_CLIENT_RENDER_HANDLE )
		{
			ClientLeafSystem()->RenderableChanged( m_hRenderHandle );
		}
	}
}

const Vector& CParticleEffectBinding::GetRenderOrigin( void )
{
	return m_pSim->GetSortOrigin();
}


const QAngle& CParticleEffectBinding::GetRenderAngles( void )
{
	return vec3_angle;
}

const matrix3x4_t &	CParticleEffectBinding::RenderableToWorldTransform()
{
	static matrix3x4_t mat;
	SetIdentityMatrix( mat );
	PositionMatrix( GetRenderOrigin(), mat );
	return mat;
}

void CParticleEffectBinding::GetRenderBounds( Vector& mins, Vector& maxs )
{
	const Vector &vSortOrigin = m_pSim->GetSortOrigin();

	// Convert to local space (around the sort origin).
	mins = m_Min - vSortOrigin;
	mins.x -= m_flParticleCullRadius; mins.y -= m_flParticleCullRadius; mins.z -= m_flParticleCullRadius;
	maxs = m_Max - vSortOrigin;
	maxs.x += m_flParticleCullRadius; maxs.y += m_flParticleCullRadius; maxs.z += m_flParticleCullRadius;
}

bool CParticleEffectBinding::ShouldDraw( void )
{
	return GetFlag( FLAGS_DRAW_THRU_LEAF_SYSTEM ) != 0;
}


bool CParticleEffectBinding::IsTransparent( void )
{
	return true;
}


inline void CParticleEffectBinding::StartDrawMaterialParticles(
	CEffectMaterial *pMaterial,
	float flTimeDelta,
	IMesh* &pMesh,
	CMeshBuilder &builder,
	ParticleDraw &particleDraw,
	bool bWireframe )
{
	// Setup the ParticleDraw and bind the material.
	if( bWireframe )
	{
		IMaterial *pMaterial = m_pParticleMgr->m_pMaterialSystem->FindMaterial( "debug/debugparticlewireframe", TEXTURE_GROUP_OTHER );
		m_pParticleMgr->m_pMaterialSystem->Bind( pMaterial, NULL );
	}
	else
	{
		m_pParticleMgr->m_pMaterialSystem->Bind( pMaterial->m_pGroup->m_pPageMaterial, m_pParticleMgr );
	}

	pMesh = m_pParticleMgr->m_pMaterialSystem->GetDynamicMesh( true );

	builder.Begin( pMesh, MATERIAL_QUADS, NUM_PARTICLES_PER_BATCH * 4 );
	particleDraw.Init( &builder, pMaterial->m_pGroup->m_pPageMaterial, flTimeDelta );
}


void CParticleEffectBinding::BBoxCalcStart( bool bFullBBoxUpdate, Vector &bbMin, Vector &bbMax )
{
	if ( !GetAutoUpdateBBox() )
		return;

	if ( bFullBBoxUpdate )
	{
		// We're going to fully recompute the bbox.
		bbMin.Init( FLT_MAX, FLT_MAX, FLT_MAX );
		bbMax.Init( -FLT_MAX, -FLT_MAX, -FLT_MAX );
	}
	else
	{
		// We're going to push out the bbox using just some of the particles.
		if ( m_bLocalSpaceTransformIdentity )
		{
			bbMin = m_Min;
			bbMax = m_Max;
		}
		else
		{
			ITransformAABB( m_LocalSpaceTransform.As3x4(), m_Min, m_Max, bbMin, bbMax );
		}
	}
}


void CParticleEffectBinding::BBoxCalcEnd( bool bFullBBoxUpdate, bool bboxSet, Vector &bbMin, Vector &bbMax )
{
	if ( !GetAutoUpdateBBox() )
		return;

	// Get the bbox into world space.
	Vector bbMinWorld, bbMaxWorld;
	if ( m_bLocalSpaceTransformIdentity )
	{
		bbMinWorld = bbMin;
		bbMaxWorld = bbMax;
	}
	else
	{
		TransformAABB( m_LocalSpaceTransform.As3x4(), bbMin, bbMax, bbMinWorld, bbMaxWorld );
	}
	
	if( bFullBBoxUpdate )
	{
		// If there were ANY particles in the system, then we've got a valid bbox here. Otherwise,
		// we don't have anything, so leave m_Min and m_Max at the sort origin.
		if ( bboxSet )
		{
			m_Min = bbMinWorld;
			m_Max = bbMaxWorld;
		}
		else
		{
			m_Min = m_Max = m_pSim->GetSortOrigin();
		}
	}
	else
	{
		// Take whatever our bbox was + pushing out from other particles.
		m_Min = bbMinWorld;
		m_Max = bbMaxWorld;
	}
}


int CParticleEffectBinding::DrawModel( int flags )
{
	VPROF_BUDGET( "CParticleEffectBinding::DrawModel", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
#ifndef PARTICLEPROTOTYPE_APP
	if ( !r_DrawParticles.GetInt() )
		return 0;
#endif

	Assert( flags != 0 );

	// If we're in commander mode and it's trying to draw the effect,
	// exit out. If the effect has FLAGS_ALWAYSSIMULATE set, then it'll come back
	// in here and simulate at the end of the frame.
	if( !g_pClientMode->ShouldDrawParticles() )
		return 0;

	SetDrawn( true );
	
	// Don't do anything if there are no particles.
	if( !m_nActiveParticles )
		return 1;

	// Reset the transformation matrix to identity.
	VMatrix mTempModel, mTempView;
	RenderStart( mTempModel, mTempView );

	// Setup to redo our bbox?
	bool bBucketSort = random->RandomInt( 0, BUCKET_SORT_EVERY_N ) == 0;

	// Set frametime to zero if we've already rendered this frame.
	float flFrameTime = 0;
	if ( m_FrameCode != m_pParticleMgr->m_FrameCode )
	{
		m_FrameCode = m_pParticleMgr->m_FrameCode;
		flFrameTime = Helper_GetFrameTime();
	}

	// For each material, render...
	// This does an incremental bubble sort. It only does one pass every frame, and it will shuffle 
	// unsorted particles one step towards where they should be.
	bool bWireframe = false;
	FOR_EACH_LL( m_Materials, iMaterial )
	{
		CEffectMaterial *pMaterial = m_Materials[iMaterial];
		
		if ( pMaterial->m_pGroup->m_pPageMaterial && pMaterial->m_pGroup->m_pPageMaterial->NeedsPowerOfTwoFrameBufferTexture() )
		{
			UpdateRefractTexture();
		}
		
		DrawMaterialParticles( 
			bBucketSort,
			pMaterial, 
			flFrameTime,
			bWireframe );
	}

	if ( ShouldDrawInWireFrameMode() )
	{
		bWireframe = true;
		FOR_EACH_LL( m_Materials, iDrawMaterial )
		{
			CEffectMaterial *pMaterial = m_Materials[iDrawMaterial];
			
			DrawMaterialParticles( 
				bBucketSort,
				pMaterial, 
				flFrameTime,
				bWireframe );
		}
	}

	if ( !IsRetail() && cl_particles_show_bbox.GetBool() )
	{
		Vector center = (m_Min + m_Max)/2;
		Vector mins   = m_Min - center;
		Vector maxs   = m_Max - center;
	
		int r, g;
		if ( m_Flags & FLAGS_AUTOUPDATEBBOX )
		{
			// red is bad, the bbox update is costly
			r = 255;
			g = 0;
		}
		else
		{
			// green, this effect presents less cpu load 
			r = 0;
			g = 255;
		}
		
		debugoverlay->AddBoxOverlay( center, mins, maxs, QAngle( 0, 0, 0 ), r, g, 0, 16, 0 );
		debugoverlay->AddTextOverlayRGB( center, 0, 0, r, g, 0, 64, "%s:(%d)", m_pSim->GetEffectName(), m_nActiveParticles );
	}

	RenderEnd( mTempModel, mTempView );
	return 1;
}


PMaterialHandle CParticleEffectBinding::FindOrAddMaterial( const char *pMaterialName )
{
	if ( !m_pParticleMgr )
	{
		return NULL;
	}

	return m_pParticleMgr->GetPMaterial( pMaterialName );
}


Particle* CParticleEffectBinding::AddParticle( int sizeInBytes, PMaterialHandle hMaterial )
{
	// We've currently clamped the particle size to PARTICLE_SIZE,
	// we may need to change this algorithm if we get particles with
	// widely varying size
	if ( sizeInBytes > PARTICLE_SIZE )
	{
		Assert( sizeInBytes <= PARTICLE_SIZE );
		return NULL;
	}

	// This is for testing - simulate it running out of memory.
	if ( particle_simulateoverflow.GetInt() )
	{
		if ( rand() % 10 <= 6 )
			return NULL;
	}
	
	// Allocate the puppy. We are actually allocating space for the
	// internals + the actual data
	Particle* pParticle = m_pParticleMgr->AllocParticle( PARTICLE_SIZE );
	if( !pParticle )
		return NULL;

	// Link it in
	CEffectMaterial *pEffectMat = GetEffectMaterial( hMaterial );
	InsertParticleAfter( pParticle, &pEffectMat->m_Particles );
	
	if ( hMaterial )
		pParticle->m_pSubTexture = hMaterial;
	else
		pParticle->m_pSubTexture = &m_pParticleMgr->m_DefaultInvalidSubTexture;

	++m_nActiveParticles;
	return pParticle;
}

void CParticleEffectBinding::SetBBox( const Vector &bbMin, const Vector &bbMax, bool bDisableAutoUpdate )
{
	m_Min = bbMin;
	m_Max = bbMax;
	
	if ( bDisableAutoUpdate )
		SetAutoUpdateBBox( false );
}

void CParticleEffectBinding::SetLocalSpaceTransform( const matrix3x4_t &transform )
{
	m_LocalSpaceTransform.CopyFrom3x4( transform );
	if ( m_LocalSpaceTransform.IsIdentity() )
	{
		m_bLocalSpaceTransformIdentity = true;
	}
	else
	{
		m_bLocalSpaceTransformIdentity = false;
	}
}

bool CParticleEffectBinding::EnlargeBBoxToContain( const Vector &pt )
{
	if ( m_nActiveParticles == 0 )
	{
		m_Min = m_Max = pt;
		return true;
	}

	bool bHasChanged = false;

	// check min bounds
	if ( pt.x < m_Min.x ) 
		{ m_Min.x = pt.x; bHasChanged = true; }

	if ( pt.y < m_Min.y ) 
		{ m_Min.y = pt.y; bHasChanged = true; }

	if ( pt.z < m_Min.z ) 
		{ m_Min.z = pt.z; bHasChanged = true; }

	// check max bounds
	if ( pt.x > m_Max.x ) 
		{ m_Max.x = pt.x; bHasChanged = true; }

	if ( pt.y > m_Max.y ) 
		{ m_Max.y = pt.y; bHasChanged = true; }

	if ( pt.z > m_Max.z ) 
		{ m_Max.z = pt.z; bHasChanged = true; }

	return bHasChanged;
}


void CParticleEffectBinding::DetectChanges()
{
	// if we have no render handle, return
	if ( m_hRenderHandle == INVALID_CLIENT_RENDER_HANDLE )
		return;

	float flBuffer = cl_particleeffect_aabb_buffer.GetFloat();
	float flExtraBuffer = flBuffer * 1.3f;

	// if nothing changed, return
	if ( m_Min.x < m_LastMin.x || 
		 m_Min.y < m_LastMin.y || 
		 m_Min.z < m_LastMin.z || 

		 m_Min.x > (m_LastMin.x + flExtraBuffer) ||
		 m_Min.y > (m_LastMin.y + flExtraBuffer) ||
		 m_Min.z > (m_LastMin.z + flExtraBuffer) ||

		 m_Max.x > m_LastMax.x || 
		 m_Max.y > m_LastMax.y || 
		 m_Max.z > m_LastMax.z || 

		 m_Max.x < (m_LastMax.x - flExtraBuffer) ||
		 m_Max.y < (m_LastMax.y - flExtraBuffer) ||
		 m_Max.z < (m_LastMax.z - flExtraBuffer)
		 )
	{
		// call leafsystem to updated this guy
		ClientLeafSystem()->RenderableChanged( m_hRenderHandle );

		// remember last parameters
		// Add some padding in here so we don't reinsert it into the leaf system if it just changes a tiny amount.
		m_LastMin = m_Min - Vector( flBuffer, flBuffer, flBuffer );
		m_LastMax = m_Max + Vector( flBuffer, flBuffer, flBuffer );
	}
}


void CParticleEffectBinding::GrowBBoxFromParticlePositions( CEffectMaterial *pMaterial, bool bFullBBoxUpdate, bool &bboxSet, Vector &bbMin, Vector &bbMax )
{
	// If its bbox is manually set, don't bother updating it here.
	if ( !GetAutoUpdateBBox() )
		return;

	if ( bFullBBoxUpdate )
	{
		for( Particle *pCur=pMaterial->m_Particles.m_pNext; pCur != &pMaterial->m_Particles; pCur=pCur->m_pNext )
		{
			// Update bounding box 
			VectorMin( bbMin, pCur->m_Pos, bbMin );
			VectorMax( bbMax, pCur->m_Pos, bbMax );
			bboxSet = true;
		}
	}
}


//-----------------------------------------------------------------------------
// Simulate particles
//-----------------------------------------------------------------------------
void CParticleEffectBinding::SimulateParticles( float flTimeDelta )
{
	if ( !m_pSim->ShouldSimulate() )
		return;

	Vector bbMin(0,0,0), bbMax(0,0,0);
	bool bboxSet = false;

	// slow the expensive update operation for particle systems that use auto-update-bbox
	// auto update the bbox after N frames then randomly 1/N or after 2*N frames 
	bool bFullBBoxUpdate = false;
	++m_UpdateBBoxCounter;
	if ( ( m_UpdateBBoxCounter >= BBOX_UPDATE_EVERY_N && random->RandomInt( 0, BBOX_UPDATE_EVERY_N ) == 0 ) ||
		( m_UpdateBBoxCounter >= 2*BBOX_UPDATE_EVERY_N ) )
	{
		bFullBBoxUpdate = true;

		// reset watchdog
		m_UpdateBBoxCounter = 0;
	}

	BBoxCalcStart( bFullBBoxUpdate, bbMin, bbMax );

	FOR_EACH_LL( m_Materials, i )
	{
		CEffectMaterial *pMaterial = m_Materials[i];
	
		CParticleSimulateIterator simulateIterator;

		simulateIterator.m_pEffectBinding = this;
		simulateIterator.m_pMaterial = pMaterial;
		simulateIterator.m_flTimeDelta = flTimeDelta;

		m_pSim->SimulateParticles( &simulateIterator );

		// Update the bbox.
		GrowBBoxFromParticlePositions( pMaterial, bFullBBoxUpdate, bboxSet, bbMin, bbMax );
	}

	BBoxCalcEnd( bFullBBoxUpdate, bboxSet, bbMin, bbMax );
}


void CParticleEffectBinding::SetDrawThruLeafSystem( int bDraw )
{
	if ( bDraw )
	{
		// If SetDrawBeforeViewModel was called, then they shouldn't be telling it to draw through
		// the leaf system too.
		Assert( !( m_Flags & FLAGS_DRAW_BEFORE_VIEW_MODEL) );
	}

	SetFlag( FLAGS_DRAW_THRU_LEAF_SYSTEM, bDraw ); 
}


void CParticleEffectBinding::SetDrawBeforeViewModel( int bDraw )
{
	// Don't draw through the leaf system if they want it to specifically draw before the view model.
	if ( bDraw )
		m_Flags &= ~FLAGS_DRAW_THRU_LEAF_SYSTEM;
	
	SetFlag( FLAGS_DRAW_BEFORE_VIEW_MODEL, bDraw ); 
}


int CParticleEffectBinding::GetNumActiveParticles()
{
	return m_nActiveParticles;
}

// Build a list of all active particles
int CParticleEffectBinding::GetActiveParticleList( int nCount, Particle **ppParticleList )
{
	int nCurrCount = 0;

	FOR_EACH_LL( m_Materials, i )
	{
		CEffectMaterial *pMaterial = m_Materials[i];
		Particle *pParticle = pMaterial->m_Particles.m_pNext;
		for ( ; pParticle != &pMaterial->m_Particles; pParticle = pParticle->m_pNext )
		{
			ppParticleList[nCurrCount] = pParticle;
			if ( ++nCurrCount == nCount )
				return nCurrCount;
		}
	}

	return nCurrCount;
}


int CParticleEffectBinding::DrawMaterialParticles( 
	bool bBucketSort,
	CEffectMaterial *pMaterial, 
	float flTimeDelta,
	bool bWireframe
	 )
{
	// Setup everything.
	CMeshBuilder builder;
	ParticleDraw particleDraw;
	IMesh *pMesh = NULL;
	StartDrawMaterialParticles( pMaterial, flTimeDelta, pMesh, builder, particleDraw, bWireframe );

	if ( m_nActiveParticles > MAX_TOTAL_PARTICLES )
		Error( "CParticleEffectBinding::DrawMaterialParticles: too many particles (%d should be less than %d)", m_nActiveParticles, MAX_TOTAL_PARTICLES );

	// Simluate and render all the particles.
	CParticleRenderIterator renderIterator;

	renderIterator.m_pEffectBinding = this;
	renderIterator.m_pMaterial = pMaterial;
	renderIterator.m_pParticleDraw = &particleDraw;
	renderIterator.m_pMeshBuilder = &builder;
	renderIterator.m_pMesh = pMesh;
	renderIterator.m_bBucketSort = bBucketSort;

	m_pSim->RenderParticles( &renderIterator );
	g_nParticlesDrawn += m_nActiveParticles;

	if( bBucketSort )
	{
		DoBucketSort( pMaterial, renderIterator.m_zCoords, renderIterator.m_nZCoords, renderIterator.m_MinZ, renderIterator.m_MaxZ );
	}

	// Flush out any remaining particles.
	builder.End( false, true );
	
	return m_nActiveParticles;
}


void CParticleEffectBinding::RenderStart( VMatrix &tempModel, VMatrix &tempView )
{
	if( IsEffectCameraSpace() )
	{
		// Store matrices off so we can restore them in RenderEnd().
		m_pParticleMgr->m_pMaterialSystem->GetMatrix(MATERIAL_VIEW, &tempView);
		m_pParticleMgr->m_pMaterialSystem->GetMatrix(MATERIAL_MODEL, &tempModel);

		// We're gonna assume the model matrix was identity and blow it off
		// This means that the particle positions are all specified in world space
		// which makes bounding box computations faster. 
		m_pParticleMgr->m_mModelView = tempView;

		// Force the user clip planes to use the old view matrix
		m_pParticleMgr->m_pMaterialSystem->EnableUserClipTransformOverride( true );
		m_pParticleMgr->m_pMaterialSystem->UserClipTransform( tempView );

		// The particle renderers want to do things in camera space
		m_pParticleMgr->m_pMaterialSystem->MatrixMode( MATERIAL_MODEL );
		m_pParticleMgr->m_pMaterialSystem->LoadIdentity();

		m_pParticleMgr->m_pMaterialSystem->MatrixMode( MATERIAL_VIEW );
		m_pParticleMgr->m_pMaterialSystem->LoadIdentity();
	}
	else
	{
		m_pParticleMgr->m_mModelView.Identity();
	}

	// Add their local space transform if they have one and they want it applied.
	if ( GetAutoApplyLocalTransform() && !m_bLocalSpaceTransformIdentity )
	{
		m_pParticleMgr->m_mModelView = m_pParticleMgr->m_mModelView * m_LocalSpaceTransform;
	}

	// Let the particle effect do any per-frame setup/processing here
	m_pSim->StartRender( m_pParticleMgr->m_mModelView );
}


void CParticleEffectBinding::RenderEnd( VMatrix &tempModel, VMatrix &tempView )
{
	if( IsEffectCameraSpace() )
	{
		// Make user clip planes work normally
		m_pParticleMgr->m_pMaterialSystem->EnableUserClipTransformOverride( false );

		// Reset the model matrix.
		m_pParticleMgr->m_pMaterialSystem->MatrixMode( MATERIAL_MODEL );
		m_pParticleMgr->m_pMaterialSystem->LoadMatrix( tempModel );

		// Reset the view matrix.
		m_pParticleMgr->m_pMaterialSystem->MatrixMode( MATERIAL_VIEW );
		m_pParticleMgr->m_pMaterialSystem->LoadMatrix( tempView );
	}
}


void CParticleEffectBinding::DoBucketSort( CEffectMaterial *pMaterial, float *zCoords, int nZCoords, float minZ, float maxZ )
{
	// Do an O(N) bucket sort. This helps the sort when there are lots of particles.
	#define NUM_BUCKETS	32
	Particle buckets[NUM_BUCKETS];
	for( int iBucket=0; iBucket < NUM_BUCKETS; iBucket++ )
	{
		buckets[iBucket].m_pPrev = buckets[iBucket].m_pNext = &buckets[iBucket];
	}
	
	// Sort into buckets.
	int iCurParticle = 0;
	Particle *pNext, *pCur;
	for( pCur=pMaterial->m_Particles.m_pNext; pCur != &pMaterial->m_Particles; pCur=pNext )
	{
		pNext = pCur->m_pNext;
		if( iCurParticle >= nZCoords )
			break;

		// Remove it..
		UnlinkParticle( pCur );

		// Add it to the appropriate bucket.
		float flPercent;
		if (maxZ == minZ)
			flPercent = 0;
		else
			flPercent = (zCoords[iCurParticle] - minZ) / (maxZ - minZ);

		int iAddBucket = (int)( flPercent * (NUM_BUCKETS - 0.0001f) );
		iAddBucket = NUM_BUCKETS - iAddBucket - 1;
		Assert( iAddBucket >= 0 && iAddBucket < NUM_BUCKETS );

		InsertParticleAfter( pCur, &buckets[iAddBucket] );

		++iCurParticle;
	}

	// Put the buckets back into the main list.
	for( int iReAddBucket=0; iReAddBucket < NUM_BUCKETS; iReAddBucket++ )
	{
		Particle *pListHead = &buckets[iReAddBucket];
		for( pCur=pListHead->m_pNext; pCur != pListHead; pCur=pNext )
		{
			pNext = pCur->m_pNext;
			InsertParticleAfter( pCur, &pMaterial->m_Particles );
			--iCurParticle;
		}
	}

	Assert(iCurParticle==0);
}


void CParticleEffectBinding::Init( CParticleMgr *pMgr, IParticleEffect *pSim )
{
	// Must Term before reinitializing.
	Assert( !m_pSim && !m_pParticleMgr );

	m_pSim = pSim;
	m_pParticleMgr = pMgr;
}


void CParticleEffectBinding::Term()
{
	if ( !m_pParticleMgr )
		return;

	// Free materials.
	FOR_EACH_LL( m_Materials, iMaterial )
	{
		CEffectMaterial *pMaterial = m_Materials[iMaterial];

		// Remove all particles tied to this effect.
		Particle *pNext = NULL;
		for(Particle *pCur = pMaterial->m_Particles.m_pNext; pCur != &pMaterial->m_Particles; pCur=pNext )
		{
			pNext = pCur->m_pNext;
			
			RemoveParticle( pCur );
		}
		
		delete pMaterial;
	}	
	m_Materials.Purge();

	memset( m_EffectMaterialHash, 0, sizeof( m_EffectMaterialHash ) );
}


void CParticleEffectBinding::RemoveParticle( Particle *pParticle )
{
	UnlinkParticle( pParticle );
	
	// Important that this is updated BEFORE NotifyDestroyParticle is called.
	--m_nActiveParticles;
	Assert( m_nActiveParticles >= 0 );

	// Let the effect do any necessary cleanup
	m_pSim->NotifyDestroyParticle(pParticle);

	// Remove it from the list of particles and deallocate
	m_pParticleMgr->FreeParticle(pParticle);
}


bool CParticleEffectBinding::RecalculateBoundingBox()
{
	if ( m_nActiveParticles == 0 )
	{
		m_Max = m_Min = m_pSim->GetSortOrigin();
		return false;
	}

	Vector bbMin(  1e28,  1e28,  1e28 );
	Vector bbMax( -1e28, -1e28, -1e28 );

	FOR_EACH_LL( m_Materials, iMaterial )
	{
		CEffectMaterial *pMaterial = m_Materials[iMaterial];
		
		for( Particle *pCur=pMaterial->m_Particles.m_pNext; pCur != &pMaterial->m_Particles; pCur=pCur->m_pNext )
		{
			VectorMin( bbMin, pCur->m_Pos, bbMin );
			VectorMax( bbMax, pCur->m_Pos, bbMax );
		}
	}

	// Get the bbox into world space.
	if ( m_bLocalSpaceTransformIdentity )
	{
		m_Min = bbMin;
		m_Max = bbMax;
	}
	else
	{
		TransformAABB( m_LocalSpaceTransform.As3x4(), bbMin, bbMax, m_Min, m_Max );
	}

	return true;
}


CEffectMaterial* CParticleEffectBinding::GetEffectMaterial( CParticleSubTexture *pSubTexture )
{
	// Hash the IMaterial pointer.
	unsigned long index = (((unsigned long)pSubTexture->m_pGroup) >> 6) % EFFECT_MATERIAL_HASH_SIZE;
	for ( CEffectMaterial *pCur=m_EffectMaterialHash[index]; pCur; pCur = pCur->m_pHashedNext )
	{
		if ( pCur->m_pGroup == pSubTexture->m_pGroup )
			return pCur;
	}

	CEffectMaterial *pEffectMat = new CEffectMaterial;
	pEffectMat->m_pGroup = pSubTexture->m_pGroup;
	pEffectMat->m_pHashedNext = m_EffectMaterialHash[index];
	m_EffectMaterialHash[index] = pEffectMat;

	m_Materials.AddToTail( pEffectMat );
	return pEffectMat;
}


//-----------------------------------------------------------------------------
// CParticleMgr
//-----------------------------------------------------------------------------

CParticleMgr::CParticleMgr()
{
	m_nToolParticleEffectId = 0;
	m_bUpdatingEffects = false;
	m_pMaterialSystem = NULL;

	memset( &m_DirectionalLight, 0, sizeof( m_DirectionalLight ) );

	m_FrameCode = 1;

	m_DefaultInvalidSubTexture.m_pGroup = &m_DefaultInvalidSubTexture.m_DefaultGroup;
	m_DefaultInvalidSubTexture.m_pMaterial = NULL;
	m_DefaultInvalidSubTexture.m_tCoordMins[0] = m_DefaultInvalidSubTexture.m_tCoordMins[1] = 0;
	m_DefaultInvalidSubTexture.m_tCoordMaxs[0] = m_DefaultInvalidSubTexture.m_tCoordMaxs[1] = 1;
	
	m_nCurrentParticlesAllocated = 0;

	SetDefLessFunc( m_effectFactories );
}

CParticleMgr::~CParticleMgr()
{
	Term();
}


//-----------------------------------------------------------------------------
// Initialization and shutdown
//-----------------------------------------------------------------------------
bool CParticleMgr::Init(unsigned long count, IMaterialSystem *pMaterials)
{
	Term();

	m_pMaterialSystem = pMaterials;

	return true;
}

void CParticleMgr::Term()
{
	// Free all the effects.
	int iNext;
	for ( int i=m_Effects.Head(); i != m_Effects.InvalidIndex(); i = iNext )
	{
		iNext = m_Effects.Next( i );
		m_Effects[i]->m_pSim->NotifyRemove();
	}
	m_Effects.Purge();

	m_SubTextures.PurgeAndDeleteElements();
	m_SubTextureGroups.PurgeAndDeleteElements();

	if ( m_pMaterialSystem )
	{
		m_pMaterialSystem->UncacheUnusedMaterials();
	}
	m_pMaterialSystem = NULL;
	
	Assert( m_nCurrentParticlesAllocated == 0 );
}

Particle *CParticleMgr::AllocParticle( int size )
{
	// Enforce max particle limit.
	if ( m_nCurrentParticlesAllocated >= MAX_TOTAL_PARTICLES )
		return NULL;
		
	Particle *pRet = (Particle *)malloc( size );
	if ( pRet )
		++m_nCurrentParticlesAllocated;

	return pRet;
}

void CParticleMgr::FreeParticle( Particle *pParticle )
{
	Assert( m_nCurrentParticlesAllocated > 0 );
	if ( pParticle )
		--m_nCurrentParticlesAllocated;
	
	free( pParticle );
}



//-----------------------------------------------------------------------------
// add a class that gets notified of entity events
//-----------------------------------------------------------------------------

void CParticleMgr::AddEffectListener( IClientParticleListener *pListener )
{
	int i = m_effectListeners.Find( pListener );
	if ( !m_effectListeners.IsValidIndex( i ) )
	{
		m_effectListeners.AddToTail( pListener );
	}
}

void CParticleMgr::RemoveEffectListener( IClientParticleListener *pListener )
{
	int i = m_effectListeners.Find( pListener );
	if ( m_effectListeners.IsValidIndex( i ) )
	{
		m_effectListeners.Remove( i );
	}
}



//-----------------------------------------------------------------------------
// registers effects classes, and create instances of these effects classes
//-----------------------------------------------------------------------------

void CParticleMgr::RegisterEffect( const char *pEffectType, CreateParticleEffectFN func )
{
#ifdef _DEBUG
	int i = m_effectFactories.Find( pEffectType );
	Assert( !m_effectFactories.IsValidIndex( i ) );
#endif

	m_effectFactories.Insert( pEffectType, func );
}

IParticleEffect *CParticleMgr::CreateEffect( const char *pEffectType )
{
	int i = m_effectFactories.Find( pEffectType );
	if ( !m_effectFactories.IsValidIndex( i ) )
	{
		Msg( "CParticleMgr::CreateEffect: factory not found for effect '%s'\n", pEffectType );
		return NULL;
	}

	CreateParticleEffectFN func = m_effectFactories[ i ];
	if ( func == NULL )
	{
		Msg( "CParticleMgr::CreateEffect: NULL factory for effect '%s'\n", pEffectType );
		return NULL;
	}

	return func();
}


//-----------------------------------------------------------------------------
// Adds and removes effects from our global list
//-----------------------------------------------------------------------------

bool CParticleMgr::AddEffect( CParticleEffectBinding *pEffect, IParticleEffect *pSim )
{
#ifdef _DEBUG
	FOR_EACH_LL( m_Effects, i )
	{
		if( m_Effects[i]->m_pSim == pSim )
		{
			Assert( !"CParticleMgr::AddEffect: added same effect twice" );
			return false;
		}
	}
#endif

	pEffect->Init( this, pSim );

	// Add it to the leaf system.
#if !defined( PARTICLEPROTOTYPE_APP )
	ClientLeafSystem()->CreateRenderableHandle( pEffect );
#endif

	pEffect->m_ListIndex = m_Effects.AddToTail( pEffect );

	Assert( pEffect->m_ListIndex != 0xFFFF );

	// notify listeners
	int nListeners = m_effectListeners.Count();
	for ( int i = 0; i < nListeners; ++i )
	{
		m_effectListeners[ i ]->OnParticleEffectAdded( pSim );
	}

	return true;
}


void CParticleMgr::RemoveEffect( CParticleEffectBinding *pEffect )
{
	// This prevents certain recursive situations where a NotifyRemove
	// call can wind up triggering another one, usually in an effect's
	// destructor.
	if( pEffect->GetRemovalInProgressFlag() )
		return;
	pEffect->SetRemovalInProgressFlag();

	// Don't call RemoveEffect while inside an IParticleEffect's Update() function.
	// Return false from the Update function instead.
	Assert( !m_bUpdatingEffects );

	// notify listeners
	int nListeners = m_effectListeners.Count();
	for ( int i = 0; i < nListeners; ++i )
	{
		m_effectListeners[ i ]->OnParticleEffectRemoved( pEffect->m_pSim );
	}

	// Take it out of the leaf system.
	ClientLeafSystem()->RemoveRenderable( pEffect->m_hRenderHandle );

	int listIndex = pEffect->m_ListIndex;
	if ( pEffect->m_pSim )
	{
		pEffect->m_pSim->NotifyRemove();
		m_Effects.Remove( listIndex );
		
	}
	else
	{
		Assert( listIndex == 0xFFFF );
	}
}


void CParticleMgr::RemoveAllEffects()
{
	int iNext;
	for ( int i=m_Effects.Head(); i != m_Effects.InvalidIndex(); i = iNext )
	{
		iNext = m_Effects.Next( i );
		RemoveEffect( m_Effects[i] );
	}
}


void CParticleMgr::IncrementFrameCode()
{
	VPROF( "CParticleMgr::IncrementFrameCode()" );

	++m_FrameCode;
	if ( m_FrameCode == 0 )
	{
		// Reset all the CParticleEffectBindings..
		FOR_EACH_LL( m_Effects, i )
		{
			m_Effects[i]->m_FrameCode = 0;
		}

		m_FrameCode = 1;
	}
}


//-----------------------------------------------------------------------------
// Main rendering loop
//-----------------------------------------------------------------------------
void CParticleMgr::Simulate( float flTimeDelta )
{
	g_nParticlesDrawn = 0;

	if(!m_pMaterialSystem)
	{
		Assert(false);
		return;
	}

	// Update all the effects.
	UpdateAllEffects( flTimeDelta );
}

void CParticleMgr::PostRender()
{
	VPROF("CParticleMgr::SimulateUndrawnEffects");

	// Simulate all effects that weren't drawn (if they have their 'always simulate' flag set).
	FOR_EACH_LL( m_Effects, i )
	{
		CParticleEffectBinding *pEffect = m_Effects[i];
		
		// Tell the effect if it was drawn or not.
		pEffect->SetWasDrawnPrevFrame( pEffect->WasDrawn() );

		// Now that we've rendered, clear this flag so it'll simulate next frame.
		pEffect->SetFlag( CParticleEffectBinding::FLAGS_FIRST_FRAME, false );	
	}
}


void CParticleMgr::DrawBeforeViewModelEffects()
{
	FOR_EACH_LL( m_Effects, i )
	{
		CParticleEffectBinding *pEffect = m_Effects[i];

		if ( pEffect->GetFlag( CParticleEffectBinding::FLAGS_DRAW_BEFORE_VIEW_MODEL ) )
		{
			Assert( !pEffect->WasDrawn() );
			pEffect->DrawModel( 1 );
		}
	}
}


void CParticleMgr::UpdateAllEffects( float flTimeDelta )
{
	m_bUpdatingEffects = true;

	if( flTimeDelta > 0.1f )
		flTimeDelta = 0.1f;

	FOR_EACH_LL( m_Effects, iEffect )
	{
		CParticleEffectBinding *pEffect = m_Effects[iEffect];

		// Don't update this effect if it will be removed.
		if( pEffect->GetRemoveFlag() )
			continue;

		// If this is a new effect, then update its bbox so it goes in the
		// right leaves (if it has particles).
		int bFirstUpdate = pEffect->GetNeedsBBoxUpdate();
		if ( bFirstUpdate )
		{
			// If the effect already disabled auto-updating of the bbox, then it should have
			// set the bbox by now and we can ignore this responsibility here.
			if ( !pEffect->GetAutoUpdateBBox() || pEffect->RecalculateBoundingBox() )
			{
				pEffect->SetNeedsBBoxUpdate( false );
			}
		}

		// This flag will get set to true if the effect is drawn through the leaf system.
		pEffect->SetDrawn( false );

		// Update the effect.
		pEffect->m_pSim->Update( flTimeDelta );

		if ( pEffect->GetFirstFrameFlag() )
			pEffect->SetFirstFrameFlag( false );
		else
			pEffect->SimulateParticles( flTimeDelta );

		// Update its position in the leaf system if its bbox changed.
		pEffect->DetectChanges();
	}

	m_bUpdatingEffects = false;

	// Remove any effects that were flagged to be removed.
	int iNext;
	for ( int i=m_Effects.Head(); i != m_Effects.InvalidIndex(); i=iNext )
	{
		iNext = m_Effects.Next( i );
		CParticleEffectBinding *pEffect = m_Effects[i];

		if( pEffect->GetRemoveFlag() )
		{
			RemoveEffect( pEffect );
		}
	}
}

CParticleSubTextureGroup* CParticleMgr::FindOrAddSubTextureGroup( IMaterial *pPageMaterial )
{
	for ( int i=0; i < m_SubTextureGroups.Count(); i++ )
	{
		if ( m_SubTextureGroups[i]->m_pPageMaterial == pPageMaterial )
			return m_SubTextureGroups[i];
	}

	CParticleSubTextureGroup *pGroup = new CParticleSubTextureGroup;
	m_SubTextureGroups.AddToTail( pGroup );
	pGroup->m_pPageMaterial = pPageMaterial;

	return pGroup;
}


PMaterialHandle CParticleMgr::GetPMaterial( const char *pMaterialName )
{
	if( !m_pMaterialSystem )
	{
		Assert(false);
		return NULL;
	}

	int hMat = m_SubTextures.Find( pMaterialName );
	if ( hMat == m_SubTextures.InvalidIndex() )
	{
		IMaterial *pIMaterial = m_pMaterialSystem->FindMaterial( pMaterialName, TEXTURE_GROUP_PARTICLE );
		if ( pIMaterial )
		{
			m_pMaterialSystem->Bind( pIMaterial, this );

			hMat = m_SubTextures.Insert( pMaterialName );
			CParticleSubTexture *pSubTexture = new CParticleSubTexture;
			m_SubTextures[hMat] = pSubTexture;
			pSubTexture->m_pMaterial = pIMaterial;

			// See if it's got a group name. If not, make a group with a special name.
			IMaterial *pPageMaterial = pIMaterial->GetMaterialPage();
			if ( pIMaterial->InMaterialPage() && pPageMaterial )
			{
				float flOffset[2], flScale[2];
				pIMaterial->GetMaterialOffset( flOffset );
				pIMaterial->GetMaterialScale( flScale );
				
				pSubTexture->m_tCoordMins[0] = (0*flScale[0] + flOffset[0]) * pPageMaterial->GetMappingWidth();
				pSubTexture->m_tCoordMaxs[0] = (1*flScale[0] + flOffset[0]) * pPageMaterial->GetMappingWidth();
				
				pSubTexture->m_tCoordMins[1] = (0*flScale[1] + flOffset[1]) * pPageMaterial->GetMappingHeight();
				pSubTexture->m_tCoordMaxs[1] = (1*flScale[1] + flOffset[1]) * pPageMaterial->GetMappingHeight();

				pSubTexture->m_pGroup = FindOrAddSubTextureGroup( pPageMaterial );
			}
			else
			{
				// Ok, this material isn't part of a group. Give it its own subtexture group.
				pSubTexture->m_pGroup = &pSubTexture->m_DefaultGroup;
				pSubTexture->m_DefaultGroup.m_pPageMaterial = pIMaterial;
				pPageMaterial = pIMaterial; // For tcoord scaling.
				
				pSubTexture->m_tCoordMins[0] = pSubTexture->m_tCoordMins[1] = 0;
				pSubTexture->m_tCoordMaxs[0] = pIMaterial->GetMappingWidth();
				pSubTexture->m_tCoordMaxs[1] = pIMaterial->GetMappingHeight();
			}

			// Rescale the texture coordinates.
			pSubTexture->m_tCoordMins[0] = (pSubTexture->m_tCoordMins[0] + 0.5f) / pPageMaterial->GetMappingWidth();
			pSubTexture->m_tCoordMins[1] = (pSubTexture->m_tCoordMins[1] + 0.5f) / pPageMaterial->GetMappingHeight();
			pSubTexture->m_tCoordMaxs[0] = (pSubTexture->m_tCoordMaxs[0] - 0.5f) / pPageMaterial->GetMappingWidth();
			pSubTexture->m_tCoordMaxs[1] = (pSubTexture->m_tCoordMaxs[1] - 0.5f) / pPageMaterial->GetMappingHeight();
		
			return pSubTexture;
		}
		else
		{
			return NULL;
		} 
	}
	else
	{
		return m_SubTextures[hMat];
	}
}


IMaterial* CParticleMgr::PMaterialToIMaterial( PMaterialHandle hMaterial ) const
{
	if ( hMaterial )
		return hMaterial->m_pMaterial;
	else
		return NULL;
}


void CParticleMgr::GetDirectionalLightInfo( CParticleLightInfo &info ) const
{
	info = m_DirectionalLight;
}


void CParticleMgr::SetDirectionalLightInfo( const CParticleLightInfo &info )
{
	m_DirectionalLight = info;
}

#ifndef _RETAIL
void CParticleMgr::SpewInfo( bool bDetail )
{
	DevMsg( "Particle Effect Systems:\n" );
	FOR_EACH_LL( m_Effects, i )
	{
		const char *pEffectName = m_Effects[i]->m_pSim->GetEffectName();
		DevMsg( "%3d: NumActive: %3d, AutoBBox: %3s \"%s\" \n", i, m_Effects[i]->m_nActiveParticles, m_Effects[i]->GetAutoUpdateBBox() ? "on" : "off", pEffectName );
	}
}
CON_COMMAND( cl_particles_dump_effects, "" )
{
	ParticleMgr()->SpewInfo( true );
}
#endif

// ------------------------------------------------------------------------------------ //
// ------------------------------------------------------------------------------------ //
float Helper_GetTime()
{
#if defined( PARTICLEPROTOTYPE_APP )
	static bool bStarted = false;
	static CCycleCount startTimer;
	if( !bStarted )
	{
		bStarted = true;
		startTimer.Sample();
	}

	CCycleCount curCount;
	curCount.Sample();

	CCycleCount elapsed;
	CCycleCount::Sub( curCount, startTimer, elapsed );

	return (float)elapsed.GetSeconds();
#else
	return gpGlobals->curtime;
#endif
}


float Helper_RandomFloat( float minVal, float maxVal )
{
#if defined( PARTICLEPROTOTYPE_APP )
	return Lerp( (float)rand() / RAND_MAX, minVal, maxVal );
#else
	return random->RandomFloat( minVal, maxVal );
#endif
}


int Helper_RandomInt( int minVal, int maxVal )
{
#if defined( PARTICLEPROTOTYPE_APP )
	return minVal + (rand() * (maxVal - minVal)) / RAND_MAX;
#else
	return random->RandomInt( minVal, maxVal );
#endif
}


float Helper_GetFrameTime()
{
#if defined( PARTICLEPROTOTYPE_APP )
	extern float g_ParticleAppFrameTime;
	return g_ParticleAppFrameTime;
#else
	return gpGlobals->frametime;
#endif
}
