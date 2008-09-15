//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "particlemgr.h"
#include "particle_prototype.h"
#include "particle_util.h"
#include "c_te_particlesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define DUST_STARTSIZE		16
#define DUST_ENDSIZE		48
#define	DUST_RADIUS			32.0f
#define	DUST_STARTALPHA		0.3f
#define	DUST_ENDALPHA		0.0f
#define	DUST_LIFETIME		2.0f

static Vector g_AntlionDustColor( 0.3f, 0.25f, 0.2f );

extern IPhysicsSurfaceProps *physprops;

class CAntlionDustEmitter : public CSimpleEmitter
{
public:
	static CAntlionDustEmitter *Create( const char *debugname )
	{
		return new CAntlionDustEmitter( debugname );
	}

	void UpdateVelocity( SimpleParticle *pParticle, float timeDelta )
	{
		//FIXME: Incorrect
		pParticle->m_vecVelocity *= 0.9f;
	}

private:
	CAntlionDustEmitter( const char *pDebugName ) : CSimpleEmitter( pDebugName ) {}
	CAntlionDustEmitter( const CAntlionDustEmitter & ); // not defined, not accessible
};

//==================================================
// C_TEAntlionDust
//==================================================

class C_TEAntlionDust: public C_TEParticleSystem
{
public:

	DECLARE_CLASS( C_TEAntlionDust, C_TEParticleSystem );
	DECLARE_CLIENTCLASS();

				C_TEAntlionDust();
	virtual		~C_TEAntlionDust();

//C_BaseEntity
public:
	virtual void	PostDataUpdate( DataUpdateType_t updateType );
	virtual	bool	ShouldDraw() { return true; }


//IParticleEffect
public:
	virtual void RenderParticles( CParticleRenderIterator *pIterator );
	virtual void SimulateParticles( CParticleSimulateIterator *pIterator );

public:
	PMaterialHandle	m_MaterialHandle;

	Vector		m_vecOrigin;
	QAngle		m_vecAngles;
	bool		m_bBlockedSpawner;

protected:
	void		GetDustColor( Vector &color );
};

// Expose to the particle app.
EXPOSE_PROTOTYPE_EFFECT( AntlionDust, C_TEAntlionDust );

IMPLEMENT_CLIENTCLASS_EVENT_DT( C_TEAntlionDust, DT_TEAntlionDust, CTEAntlionDust )
	RecvPropVector(RECVINFO( m_vecOrigin )),
	RecvPropVector(RECVINFO( m_vecAngles )),
	RecvPropBool(RECVINFO( m_bBlockedSpawner )),
END_RECV_TABLE()

//==================================================
// C_TEAntlionDust
//==================================================

C_TEAntlionDust::C_TEAntlionDust()
{
	m_MaterialHandle	= INVALID_MATERIAL_HANDLE;
	m_vecOrigin.Init();
	m_vecAngles.Init();
	m_bBlockedSpawner = false;
}

C_TEAntlionDust::~C_TEAntlionDust()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bNewEntity - whether or not to start a new entity
//-----------------------------------------------------------------------------

void C_TEAntlionDust::PostDataUpdate( DataUpdateType_t updateType )
{
	CSmartPtr<CAntlionDustEmitter> pDustEmitter = CAntlionDustEmitter::Create( "TEAntlionDust" );

	if ( !pDustEmitter )
	{
		assert(0);
		return;
	}

	pDustEmitter->SetSortOrigin( m_vecOrigin );
	pDustEmitter->SetNearClip( 32, 64 );
	pDustEmitter->GetBinding().SetBBox( m_vecOrigin - Vector( 32, 32, 32 ), m_vecOrigin + Vector( 32, 32, 32 ) );

	m_MaterialHandle = pDustEmitter->GetPMaterial( "particle/particle_smokegrenade" );

	Vector	offset;
	Vector	vecColor;
	GetDustColor( vecColor );

	SimpleParticle	*pParticle;

	int iParticleCount = 48;

	if ( m_bBlockedSpawner == true )
	{
		iParticleCount = 24;
	}

	//Spawn the dust
	for ( int i = 0; i < iParticleCount; i++ )
	{
		//Offset this dust puff's origin
		offset[0] = random->RandomFloat( -DUST_RADIUS, DUST_RADIUS );
		offset[1] = random->RandomFloat( -DUST_RADIUS, DUST_RADIUS );
		offset[2] = random->RandomFloat(  -16, 8 );
		
		offset += m_vecOrigin;

		pParticle = (SimpleParticle *) pDustEmitter->AddParticle( sizeof(SimpleParticle), m_MaterialHandle, offset );

		if ( pParticle )
		{
			pParticle->m_flDieTime	= 2.0f;
			pParticle->m_flLifetime	= 0.0f;
			
			Vector	dir		= pParticle->m_Pos - m_vecOrigin;
			float	length	= VectorNormalize( dir );

			pParticle->m_vecVelocity	= dir * ( length + random->RandomFloat( 64.0f, 128.0f ) );

			float	colorRamp = random->RandomFloat( 0.5f, 1.0f );
			Vector	color = vecColor*colorRamp;

			color[0] = clamp( color[0], 0.0f, 1.0f );
			color[1] = clamp( color[1], 0.0f, 1.0f );
			color[2] = clamp( color[2], 0.0f, 1.0f );

			color *= 255;

			pParticle->m_uchColor[0]	= color[0];
			pParticle->m_uchColor[1]	= color[1];
			pParticle->m_uchColor[2]	= color[2];

			pParticle->m_uchStartAlpha	= random->RandomFloat( 16, 64 );
			pParticle->m_uchEndAlpha	= 0;

			pParticle->m_uchStartSize	= random->RandomInt( 16, 32 );
			pParticle->m_uchEndSize		= pParticle->m_uchStartSize * 3;
			pParticle->m_flRoll			= random->RandomInt( 0, 360 );
			pParticle->m_flRollDelta	= random->RandomFloat( -0.2f, 0.2f );
		}
	}
}

void GetColorForSurface( trace_t *trace, Vector *color );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &color - 
//-----------------------------------------------------------------------------
void C_TEAntlionDust::GetDustColor( Vector &color )
{
	trace_t	tr;
	UTIL_TraceLine( m_vecOrigin+Vector(0,0,1), m_vecOrigin+Vector(0,0,-32), 
		MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &tr );

	if ( tr.fraction < 1.0f )
	{
		GetColorForSurface( &tr, &color );
	}
	else
	{
		//Fill in a fallback
		color = g_AntlionDustColor;
	}
}

void C_TEAntlionDust::RenderParticles( CParticleRenderIterator *pIterator )
{
}

void C_TEAntlionDust::SimulateParticles( CParticleSimulateIterator *pIterator )
{
}

