//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// studio_render.cpp: routines for drawing Half-Life 3DStudio models
// updates:
// 1-4-99	fixed AdvanceFrame wraping bug

#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>

#include <windows.h> // for OutputDebugString. . has to be a better way!


#include "ViewerSettings.h"
#include "StudioModel.h"
#include "vphysics/constraints.h"
#include "physmesh.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "matsyswin.h"
#include "istudiorender.h"
#include "utldict.h"
#include "filesystem.h"
#include "studio_render.h"
#include "materialsystem/IMesh.h"
#include "bone_setup.h"
#include "materialsystem/MaterialSystem_Config.h"
#include "MDLViewer.h"
#include "bone_accessor.h"
#include "debugdrawmodel.h"

// FIXME:
extern ViewerSettings g_viewerSettings;
int g_dxlevel = 0;

#pragma warning( disable : 4244 ) // double to float

////////////////////////////////////////////////////////////////////////

CStudioHdr		*g_pCacheHdr = NULL;

Vector			g_flexedverts[MAXSTUDIOVERTS];
Vector			g_flexednorms[MAXSTUDIOVERTS];
int				g_flexages[MAXSTUDIOVERTS];

Vector			*g_pflexedverts;
Vector			*g_pflexednorms;
int				*g_pflexages;

int				g_smodels_total;				// cookie

matrix3x4_t		g_viewtransform;				// view transformation
//matrix3x4_t	g_posetoworld[MAXSTUDIOBONES];	// bone transformation matrix

static int			maxNumVertices;
static int			first = 1;

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
mstudioseqdesc_t &StudioModel::GetSeqDesc( int seq )
{
	CStudioHdr *pStudioHdr = GetStudioHdr();
	return pStudioHdr->pSeqdesc( seq );
}

mstudioanimdesc_t &StudioModel::GetAnimDesc( int anim )
{
	CStudioHdr *pStudioHdr = GetStudioHdr();
	return pStudioHdr->pAnimdesc( anim );
}


//-----------------------------------------------------------------------------
// Purpose: Keeps a global clock to autoplay sequences to run from
//			Also deals with speedScale changes
//-----------------------------------------------------------------------------
float GetAutoPlayTime( void )
{
	static int g_prevTicks;
	static float g_time;

	int ticks = GetTickCount();
	// limit delta so that float time doesn't overflow
	if (g_prevTicks == 0)
		g_prevTicks = ticks;

	g_time += ( (ticks - g_prevTicks) / 1000.0f ) * g_viewerSettings.speedScale;
	g_prevTicks = ticks;

	return g_time;
}


//-----------------------------------------------------------------------------
// Purpose: Keeps a global clock for "realtime" overlays to run from
//-----------------------------------------------------------------------------
float GetRealtimeTime( void )
{
	// renamed static's so debugger doesn't get confused and show the wrong one
	static int g_prevTicksRT;
	static float g_timeRT;

	int ticks = GetTickCount();
	// limit delta so that float time doesn't overflow
	if (g_prevTicksRT == 0)
		g_prevTicksRT = ticks;

	g_timeRT += ( (ticks - g_prevTicksRT) / 1000.0f );
	g_prevTicksRT = ticks;

	return g_timeRT;
}

void StudioModel::AdvanceFrame( float dt )
{
	if (dt > 0.1)
		dt = 0.1f;

	m_dt = dt;

	float t = GetDuration( );

	if (t > 0)
	{
		if (dt > 0)
		{
			m_cycle += dt / t;
			m_sequencetime += dt;

			// wrap
			m_cycle -= (int)(m_cycle);
		}
	}
	else
	{
		m_cycle = 0;
	}

	
	for (int i = 0; i < MAXSTUDIOANIMLAYERS; i++)
	{
		t = GetDuration( m_Layer[i].m_sequence );
		if (t > 0)
		{
			if (dt > 0)
			{
				m_Layer[i].m_cycle += (dt / t) * m_Layer[i].m_playbackrate;
				m_Layer[i].m_cycle -= (int)(m_Layer[i].m_cycle);
			}
		}
		else
		{
			m_Layer[i].m_cycle = 0;
		}
	}
}

float StudioModel::GetInterval( void )
{
	return m_dt;
}

float StudioModel::GetCycle( void )
{
	return m_cycle;
}

float StudioModel::GetFrame( void )
{
	return GetCycle() * GetMaxFrame();
}

int StudioModel::GetMaxFrame( void )
{
	CStudioHdr *pStudioHdr = GetStudioHdr();
	return Studio_MaxFrame( pStudioHdr, m_sequence, m_poseparameter );
}

int StudioModel::SetFrame( int frame )
{
	CStudioHdr *pStudioHdr = GetStudioHdr();
	if ( !pStudioHdr )
		return 0;

	if ( frame <= 0 )
		frame = 0;

	int maxFrame = GetMaxFrame();
	if ( frame >= maxFrame )
	{
		frame = maxFrame;
		m_cycle = 0.99999;
		return frame;
	}

	m_cycle = frame / (float)maxFrame;
	return frame;
}


float StudioModel::GetCycle( int iLayer )
{
	if (iLayer == 0)
	{
		return m_cycle;
	}
	else if (iLayer <= MAXSTUDIOANIMLAYERS)
	{
		int index = iLayer - 1;
		return m_Layer[index].m_cycle;
	}
	return 0;
}


float StudioModel::GetFrame( int iLayer )
{
	return GetCycle( iLayer ) * GetMaxFrame( iLayer );
}


int StudioModel::GetMaxFrame( int iLayer )
{
	CStudioHdr *pStudioHdr = GetStudioHdr();
	if ( pStudioHdr )
	{
		if (iLayer == 0)
			return Studio_MaxFrame( pStudioHdr, m_sequence, m_poseparameter );

		if (iLayer <= MAXSTUDIOANIMLAYERS)
		{
			int index = iLayer - 1;
			return Studio_MaxFrame( pStudioHdr, m_Layer[index].m_sequence, m_poseparameter );
		}
	}

	return 0;
}


int StudioModel::SetFrame( int iLayer, int frame )
{
	CStudioHdr *pStudioHdr = GetStudioHdr();
	if ( !pStudioHdr )
		return 0;

	if ( frame <= 0 )
		frame = 0;

	int maxFrame = GetMaxFrame( iLayer );
	float cycle = 0;
	if (maxFrame)
	{
		if ( frame >= maxFrame )
		{
			frame = maxFrame;
			cycle = 0.99999;
		}
		cycle = frame / (float)maxFrame;
	}

	if (iLayer == 0)
	{
		m_cycle = cycle;
	}
	else if (iLayer <= MAXSTUDIOANIMLAYERS)
	{
		int index = iLayer - 1;
		m_Layer[index].m_cycle = cycle;
	}

	return frame;
}



//-----------------------------------------------------------------------------
// Purpose: Maps from local axis (X,Y,Z) to Half-Life (PITCH,YAW,ROLL) axis/rotation mappings
//-----------------------------------------------------------------------------
static int RemapAxis( int axis )
{
	switch( axis )
	{
	case 0:
		return 2;
	case 1:
		return 0;
	case 2:
		return 1;
	}

	return 0;
}

void StudioModel::Physics_SetPreview( int previewBone, int axis, float t )
{
	m_physPreviewBone = previewBone;
	m_physPreviewAxis = axis;
	m_physPreviewParam = t;
}


void StudioModel::OverrideBones( bool *override )
{
	matrix3x4_t basematrix;
	matrix3x4_t bonematrix;

	QAngle tmp;
	// offset for the base pose to world transform of 90 degrees around up axis
	tmp[0] = 0; tmp[1] = 90; tmp[2] = 0;
	AngleMatrix( tmp, bonematrix );
	ConcatTransforms( g_viewtransform, bonematrix, basematrix );

	for ( int i = 0; i < m_pPhysics->Count(); i++ )
	{
		CPhysmesh *pmesh = m_pPhysics->GetMesh( i );
		// BUGBUG: Cache this if you care about performance!
		int boneIndex = FindBone(pmesh->m_boneName);
		
		// bone is not constrained, don't override rotations
		if ( pmesh->m_constraint.parentIndex == 0 && pmesh->m_constraint.childIndex == 0 )
		{
			boneIndex = -1;
		}

		if ( boneIndex >= 0 )
		{
			matrix3x4_t *parentMatrix = &basematrix;
			override[boneIndex] = true;
			int parentBone = -1;
			if ( pmesh->m_constraint.parentIndex >= 0 )
			{
				parentBone = FindBone( m_pPhysics->GetMesh(pmesh->m_constraint.parentIndex)->m_boneName );
			}
			if ( parentBone >= 0 )
			{
				parentMatrix = g_pStudioRender->GetBoneToWorld( parentBone );
			}

			if ( m_physPreviewBone == i )
			{
				matrix3x4_t tmpmatrix;
				QAngle rot;
				constraint_axislimit_t *axis = pmesh->m_constraint.axes + m_physPreviewAxis;

				int hlAxis = RemapAxis( m_physPreviewAxis );
				rot.Init();
				rot[hlAxis] = axis->minRotation + (axis->maxRotation - axis->minRotation) * m_physPreviewParam;
				AngleMatrix( rot, tmpmatrix );
				ConcatTransforms( pmesh->m_matrix, tmpmatrix, bonematrix );
			}
			else
			{
				MatrixCopy( pmesh->m_matrix, bonematrix );
			}

			ConcatTransforms(*parentMatrix, bonematrix, *g_pStudioRender->GetBoneToWorld( boneIndex ));
		}
	}
}


int StudioModel::BoneMask( void )
{
	int lod = g_viewerSettings.autoLOD ? 0 : g_viewerSettings.lod;

	int mask = BONE_USED_BY_VERTEX_AT_LOD(lod);
	if (g_viewerSettings.showAttachments || g_viewerSettings.m_iEditAttachment != -1 || m_nSolveHeadTurn != 0 || LookupAttachment( "eyes" ) != -1)
	{
		mask |= BONE_USED_BY_ATTACHMENT;
	}

	if (g_viewerSettings.showHitBoxes)
	{
		mask |= BONE_USED_BY_HITBOX;
	}

	mask |= BONE_USED_BY_BONE_MERGE;

	return mask;
	// return BONE_USED_BY_ANYTHING_AT_LOD( lod );

	// return BONE_USED_BY_ANYTHING;
}

void StudioModel::SetUpBones( bool mergeBones )
{
	int					i, j;

	mstudiobone_t		*pbones;

	static Vector		pos[MAXSTUDIOBONES];
	matrix3x4_t			bonematrix;
	static Quaternion	q[MAXSTUDIOBONES];
	bool				override[MAXSTUDIOBONES];

	static matrix3x4_t	boneCache[MAXSTUDIOBONES];

	// For blended transitions
	static Vector		pos2[MAXSTUDIOBONES];
	static Quaternion	q2[MAXSTUDIOBONES];

	CStudioHdr *pStudioHdr = GetStudioHdr();
	mstudioseqdesc_t	&seqdesc = pStudioHdr->pSeqdesc( m_sequence );

	QAngle a1;
	Vector p1;
	MatrixAngles( g_viewtransform, a1, p1 );
	CIKContext *pIK = NULL;
	m_ik.Init( pStudioHdr, a1, p1, GetRealtimeTime(), m_iFramecounter, BoneMask( ) );
	if ( g_viewerSettings.enableIK )
	{
		pIK = &m_ik;
	}

	InitPose(  pStudioHdr, pos, q );
	
	AccumulatePose( pStudioHdr, pIK, pos, q, m_sequence, m_cycle, m_poseparameter, BoneMask( ), 1.0, GetRealtimeTime() );

	if ( g_viewerSettings.blendSequenceChanges &&
		m_sequencetime < m_blendtime && 
		m_prevsequence != m_sequence &&
		m_prevsequence < pStudioHdr->GetNumSeq() &&
		!(seqdesc.flags & STUDIO_SNAP) )
	{
		// Make sure frame is valid
		if ( m_prevcycle >= 1.0 )
		{
			m_prevcycle = 0.0f;
		}

		float s = 1.0 - ( m_sequencetime / m_blendtime );
		s = 3 * s * s - 2 * s * s * s;

		AccumulatePose( pStudioHdr, NULL, pos, q, m_prevsequence, m_prevcycle, m_poseparameter, BoneMask( ), s, GetRealtimeTime() );
		// Con_DPrintf("%d %f : %d %f : %f\n", pev->sequence, f, pev->prevsequence, pev->prevframe, s );
	}
	else
	{
		m_prevcycle = m_cycle;
	}

	int iMaxPriority = 0;
	for (i = 0; i < MAXSTUDIOANIMLAYERS; i++)
	{
		if (m_Layer[i].m_weight > 0)
		{
			iMaxPriority = max( m_Layer[i].m_priority, iMaxPriority );
		}
	}

	for (j = 0; j <= iMaxPriority; j++)
	{
		for (i = 0; i < MAXSTUDIOANIMLAYERS; i++)
		{
			if (m_Layer[i].m_priority == j && m_Layer[i].m_weight > 0)
			{
				AccumulatePose( pStudioHdr, pIK, pos, q, m_Layer[i].m_sequence, m_Layer[i].m_cycle, m_poseparameter, BoneMask( ), m_Layer[i].m_weight, GetRealtimeTime() );
			}
		}
	}

	if (m_nSolveHeadTurn != 0)
	{
		GetBodyPoseParametersFromFlex( );
	}

	SetHeadPosition( pos, q );

	CIKContext auto_ik;
	auto_ik.Init( pStudioHdr, a1, p1, 0.0, 0, BoneMask( ) );

	CalcAutoplaySequences( pStudioHdr, &auto_ik, pos, q, m_poseparameter, BoneMask( ), GetAutoPlayTime() );

	CalcBoneAdj( pStudioHdr, pos, q, m_controller, BoneMask( ) );

	CBoneBitList boneComputed;
	if (pIK)
	{
		Vector deltaPos;
		QAngle deltaAngles;

		GetMovement( m_prevIKCycles, deltaPos, deltaAngles );

		Vector tmp;
		VectorRotate( deltaPos, g_viewtransform, tmp );
		deltaPos = tmp;

		pIK->UpdateTargets( pos, q, g_pStudioRender->GetBoneToWorld(0), boneComputed );

		// FIXME: check number of slots?
		for (int i = 0; i < pIK->m_target.Count(); i++)
		{
			trace_t tr;
			CIKTarget *pTarget = &pIK->m_target[i];

			switch( pTarget->type )
			{
			case IK_GROUND:
				{
					// drawLine( pTarget->est.pos, pTarget->est.pos + pTarget->offset.pos, 0, 255, 0 );

					// hack in movement
					pTarget->est.pos -= deltaPos;

					matrix3x4_t invViewTransform;
					MatrixInvert( g_viewtransform, invViewTransform );
					Vector tmp;
					VectorTransform( pTarget->est.pos, invViewTransform, tmp );
					tmp.z = pTarget->est.floor;
					VectorTransform( tmp, g_viewtransform, pTarget->est.pos );
					Vector p1;
					Quaternion q1;
					MatrixAngles( g_viewtransform, q1, p1 );
					pTarget->est.q = q1;

					float color[4] = { 0, 0, 0, 0 };
					float wirecolor[4] = { 1, 1, 0, 1 };
					if (pTarget->est.latched > 0.0)
					{
						wirecolor[1] = 1.0 - pTarget->est.flWeight;
					}
					else
					{
						wirecolor[0] = 1.0 - pTarget->est.flWeight;
					}

					Vector p0 = tmp + Vector( -pTarget->est.radius, -pTarget->est.radius, 0 );
					Vector p2 = tmp + Vector( pTarget->est.radius, pTarget->est.radius, 0 );
					drawTransparentBox( p0, p2, g_viewtransform, color, wirecolor );


					if (!g_viewerSettings.enableTargetIK)
					{
						pTarget->est.flWeight = 0.0;
					}
				}
				break;
			case IK_ATTACHMENT:
				{
					matrix3x4_t m;

					QuaternionMatrix( pTarget->est.q, pTarget->est.pos, m );

					drawTransform( m, 4 );
				}
				break;
			}

			// drawLine( pTarget->est.pos, pTarget->latched.pos, 255, 0, 0 );
		}
		
		pIK->SolveDependencies( pos, q, g_pStudioRender->GetBoneToWorld(0), boneComputed );
	}

	pbones = pStudioHdr->pBone( 0 );

	memset( override, 0, sizeof(bool)*pStudioHdr->numbones() );

	if ( g_viewerSettings.showPhysicsPreview )
	{
		OverrideBones( override );
	}

	for (i = 0; i < pStudioHdr->numbones(); i++) 
	{
		if ( !(pStudioHdr->pBone( i )->flags & BoneMask()))
		{
			int j, k;
			for (j = 0; j < 3; j++)
			{
				for (k = 0; k < 4; k++)
				{
					(*g_pStudioRender->GetBoneToWorld( i ))[j][k] = VEC_T_NAN;
				}
			}
			continue;
		}

		if ( override[i] )
		{
			continue;
		}
		else if (boneComputed.IsBoneMarked(i))
		{
			// already calculated
		}
		else if (CalcProceduralBone( pStudioHdr, i, CBoneAccessor( g_pStudioRender->GetBoneToWorldArray() ) ))
		{
			continue;
		}
		else
		{
			QuaternionMatrix( q[i], bonematrix );

			bonematrix[0][3] = pos[i][0];
			bonematrix[1][3] = pos[i][1];
			bonematrix[2][3] = pos[i][2];
			if (pbones[i].parent == -1) 
			{
				ConcatTransforms (g_viewtransform, bonematrix, *g_pStudioRender->GetBoneToWorld( i ));
				// MatrixCopy(bonematrix, g_bonetoworld[i]);
			} 
			else 
			{
				ConcatTransforms (*g_pStudioRender->GetBoneToWorld( pbones[i].parent ), bonematrix, *g_pStudioRender->GetBoneToWorld( i ) );
			}
		}

		if (!mergeBones)
		{
			g_pCacheHdr = pStudioHdr;
			MatrixCopy( *g_pStudioRender->GetBoneToWorld( i ), boneCache[i] );
		}
		else if (g_pCacheHdr)
		{
			for (j = 0; j < g_pCacheHdr->numbones(); j++)
			{
				if (stricmp( pStudioHdr->pBone( i )->pszName(), g_pCacheHdr->pBone( j )->pszName() ) == 0)
				{
					break;
				}
			}
			if (j < g_pCacheHdr->numbones())
			{
				MatrixCopy( boneCache[j], *g_pStudioRender->GetBoneToWorld( i ) );
			}
		}
	}

	if (g_viewerSettings.showAttachments)
	{
		// drawTransform( g_pStudioRender->GetBoneToWorld( 0 ) );
	}
}



/*
================
StudioModel::SetupLighting
	set some global variables based on entity position
inputs:
outputs:
================
*/
void StudioModel::SetupLighting ( )
{
	LightDesc_t light[2];

	light[0].m_Type = MATERIAL_LIGHT_DIRECTIONAL;
	light[0].m_Attenuation0 = 1.0f;
	light[0].m_Attenuation1 = 0.0;
	light[0].m_Attenuation2 = 0.0;
	light[0].m_Color[0] = g_viewerSettings.lColor[0];
	light[0].m_Color[1] = g_viewerSettings.lColor[1];
	light[0].m_Color[2] = g_viewerSettings.lColor[2];
	light[0].m_Range = 2000;

	// DEBUG: Spin the light around the head for debugging
//	g_viewerSettings.lightrot = QAngle( 0, 0, 0 );
//	g_viewerSettings.lightrot.y = fmod( (90 * GetTickCount( ) / 1000.0), 360.0);

	AngleVectors( g_viewerSettings.lightrot, &light[0].m_Direction, NULL, NULL );
	g_pStudioRender->SetLocalLights( 1, light );

#if 0
	light[1].m_Type = MATERIAL_LIGHT_DIRECTIONAL;
	light[1].m_Attenuation0 = 1.0f;
	light[1].m_Attenuation1 = 0.0;
	light[1].m_Attenuation2 = 0.0;
	light[1].m_Range = 2000;
	light[1].m_Color[0] = g_viewerSettings.lColor[2];
	light[1].m_Color[1] = g_viewerSettings.lColor[1];
	light[1].m_Color[2] = g_viewerSettings.lColor[0];
	light[1].m_Direction.x = -light[0].m_Direction.y;
	light[1].m_Direction.y = light[0].m_Direction.x;
	light[1].m_Direction.z = light[0].m_Direction.z;
	g_pStudioRender->SetLocalLights( 2, light );
#endif

	int i;
	for( i = 0; i < g_pStudioRender->GetNumAmbientLightSamples(); i++ )
	{
		m_AmbientLightColors[i][0] = g_viewerSettings.aColor[0];
		m_AmbientLightColors[i][1] = g_viewerSettings.aColor[1];
		m_AmbientLightColors[i][2] = g_viewerSettings.aColor[2];
	}

	//m_AmbientLightColors[0][0] = 1.0;
	//m_AmbientLightColors[0][1] = 1.0;
	//m_AmbientLightColors[0][2] = 1.0;

	g_pStudioRender->SetAmbientLightColors( m_AmbientLightColors );
}


int FindBoneIndex( CStudioHdr *pstudiohdr, const char *pName )
{
	mstudiobone_t *pbones = pstudiohdr->pBone( 0 );
	for (int i = 0; i < pstudiohdr->numbones(); i++)
	{
		if ( !strcmpi( pName, pbones[i].pszName() ) )
			return i;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Find the named bone index, -1 if not found
// Input  : *pName - bone name
//-----------------------------------------------------------------------------
int StudioModel::FindBone( const char *pName )
{
	CStudioHdr *pStudioHdr = GetStudioHdr();
	return FindBoneIndex( pStudioHdr, pName );
}


int StudioModel::Physics_GetBoneIndex( const char *pName )
{
	for (int i = 0; i < m_pPhysics->Count(); i++)
	{
		CPhysmesh *pmesh = m_pPhysics->GetMesh(i);
		if ( !strcmpi( pName, pmesh[i].m_boneName ) )
			return i;
	}

	return -1;
}


/*
=================
StudioModel::SetupModel
	based on the body part, figure out which mesh it should be using.
inputs:
	currententity
outputs:
	pstudiomesh
	pmdl
=================
*/

void StudioModel::SetupModel ( int bodypart )
{
	int index;

	CStudioHdr *pStudioHdr = GetStudioHdr();
	if (bodypart > pStudioHdr->numbodyparts())
	{
		// Con_DPrintf ("StudioModel::SetupModel: no such bodypart %d\n", bodypart);
		bodypart = 0;
	}

	mstudiobodyparts_t   *pbodypart = pStudioHdr->pBodypart( bodypart );

	index = m_bodynum / pbodypart->base;
	index = index % pbodypart->nummodels;

	m_pmodel = pbodypart->pModel( index );

	if(first){
		maxNumVertices = m_pmodel->numvertices;
		first = 0;
	}
}


static IMaterial *g_pAlpha;


//-----------------------------------------------------------------------------
// Draws a box, not wireframed
//-----------------------------------------------------------------------------

void StudioModel::drawBox (Vector const *v, float const * color )
{
	IMesh* pMesh = g_pMaterialSystem->GetDynamicMesh( );

	CMeshBuilder meshBuilder;

	// The four sides
	meshBuilder.Begin( pMesh, MATERIAL_TRIANGLE_STRIP, 2 * 4 );
	for (int i = 0; i < 10; i++)
	{
		meshBuilder.Position3fv (v[i & 7].Base() );
		meshBuilder.Color4fv( color );
		meshBuilder.AdvanceVertex();
	}
	meshBuilder.End();
	pMesh->Draw();

	// top and bottom
	meshBuilder.Begin( pMesh, MATERIAL_TRIANGLE_STRIP, 2 );

	meshBuilder.Position3fv (v[6].Base());
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv (v[0].Base());
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv (v[4].Base());
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv (v[2].Base());
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();

	meshBuilder.Begin( pMesh, MATERIAL_TRIANGLE_STRIP, 2 );

	meshBuilder.Position3fv (v[1].Base());
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv (v[7].Base());
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv (v[3].Base());
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv (v[5].Base());
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}

//-----------------------------------------------------------------------------
// Draws a wireframed box
//-----------------------------------------------------------------------------

void StudioModel::drawWireframeBox (Vector const *v, float const* color )
{
	IMesh* pMesh = g_pMaterialSystem->GetDynamicMesh( );

	CMeshBuilder meshBuilder;

	// The four sides
	meshBuilder.Begin( pMesh, MATERIAL_LINES, 4 );
	for (int i = 0; i < 10; i++)
	{
		meshBuilder.Position3fv (v[i & 7].Base());
		meshBuilder.Color4fv( color );
		meshBuilder.AdvanceVertex();
	}
	meshBuilder.End();
	pMesh->Draw();

	// top and bottom
	meshBuilder.Begin( pMesh, MATERIAL_LINE_STRIP, 4 );

	meshBuilder.Position3fv (v[6].Base());
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv (v[0].Base());
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv (v[2].Base());
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv (v[4].Base());
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv (v[6].Base());
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();

	meshBuilder.Begin( pMesh, MATERIAL_LINE_STRIP, 4 );

	meshBuilder.Position3fv (v[1].Base());
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv (v[7].Base());
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv (v[5].Base());
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv (v[3].Base());
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv (v[1].Base());
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}

//-----------------------------------------------------------------------------
// Draws the position and axies of a transformation matrix, x=red,y=green,z=blue
//-----------------------------------------------------------------------------
void StudioModel::drawTransform( matrix3x4_t& m, float flLength )
{
	IMesh* pMesh = g_pMaterialSystem->GetDynamicMesh( );
	CMeshBuilder meshBuilder;

	for (int k = 0; k < 3; k++)
	{
		static unsigned char color[3][3] =
		{
			{ 255, 0, 0 },
			{ 0, 255, 0 },
			{ 0, 0, 255 }
		};

		meshBuilder.Begin( pMesh, MATERIAL_LINES, 1 );

		meshBuilder.Color3ubv( color[k] );
		meshBuilder.Position3f( m[0][3], m[1][3], m[2][3]);
		meshBuilder.AdvanceVertex();

		meshBuilder.Color3ubv( color[k] );
		meshBuilder.Position3f( m[0][3] + m[0][k] * flLength, m[1][3] + m[1][k] * flLength, m[2][3] + m[2][k] * flLength);
		meshBuilder.AdvanceVertex();

		meshBuilder.End();
		pMesh->Draw();
	}
}

void drawLine( Vector const &p1, Vector const &p2, int r, int g, int b, bool noDepthTest, float duration )
{
	g_pStudioModel->drawLine( p1, p2, r, g, b );
}


void StudioModel::drawLine( Vector const &p1, Vector const &p2, int r, int g, int b )
{
	g_pMaterialSystem->Bind( g_materialLines );

	IMesh* pMesh = g_pMaterialSystem->GetDynamicMesh( );
	CMeshBuilder meshBuilder;

	meshBuilder.Begin( pMesh, MATERIAL_LINES, 1 );

	meshBuilder.Color3ub( r, g, b );
	meshBuilder.Position3f( p1.x, p1.y, p1.z );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color3ub( r, g, b );
	meshBuilder.Position3f(  p2.x, p2.y, p2.z );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}


//-----------------------------------------------------------------------------
// Draws a transparent box with a wireframe outline
//-----------------------------------------------------------------------------
void StudioModel::drawTransparentBox( Vector const &bbmin, Vector const &bbmax, 
					const matrix3x4_t& m, float const *color, float const *wirecolor )
{
	Vector v[8], v2[8];

	v[0][0] = bbmin[0];
	v[0][1] = bbmax[1];
	v[0][2] = bbmin[2];

	v[1][0] = bbmin[0];
	v[1][1] = bbmin[1];
	v[1][2] = bbmin[2];

	v[2][0] = bbmax[0];
	v[2][1] = bbmax[1];
	v[2][2] = bbmin[2];

	v[3][0] = bbmax[0];
	v[3][1] = bbmin[1];
	v[3][2] = bbmin[2];

	v[4][0] = bbmax[0];
	v[4][1] = bbmax[1];
	v[4][2] = bbmax[2];

	v[5][0] = bbmax[0];
	v[5][1] = bbmin[1];
	v[5][2] = bbmax[2];

	v[6][0] = bbmin[0];
	v[6][1] = bbmax[1];
	v[6][2] = bbmax[2];

	v[7][0] = bbmin[0];
	v[7][1] = bbmin[1];
	v[7][2] = bbmax[2];

	VectorTransform (v[0], m, v2[0]);
	VectorTransform (v[1], m, v2[1]);
	VectorTransform (v[2], m, v2[2]);
	VectorTransform (v[3], m, v2[3]);
	VectorTransform (v[4], m, v2[4]);
	VectorTransform (v[5], m, v2[5]);
	VectorTransform (v[6], m, v2[6]);
	VectorTransform (v[7], m, v2[7]);
	
	g_pMaterialSystem->Bind( g_pAlpha );
	drawBox( v2, color );

	g_pMaterialSystem->Bind( g_materialBones );
	drawWireframeBox( v2, wirecolor );
}


#define	MAXPRINTMSG	4096
static void StudioRender_Warning( const char *fmt, ... )
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
	
	va_start( argptr, fmt );
	vsprintf( msg, fmt, argptr );
	va_end( argptr );

//	Msg( mwWarning, "StudioRender: %s", msg );
	OutputDebugString( msg );
}

void StudioModel::UpdateStudioRenderConfig( bool bWireframe, bool bZBufferWireframe, bool bNormals, bool bTangentFrame )
{
	StudioRenderConfig_t config;
	memset( &config, 0, sizeof( config ) );
	config.pConPrintf = StudioRender_Warning;
	config.pConDPrintf = StudioRender_Warning;
	config.fEyeShiftX = 0.0f;
	config.fEyeShiftY = 0.0f;
	config.fEyeShiftZ = 0.0f;
	config.fEyeSize = 0;
	config.eyeGloss = 1;
	config.drawEntities = 1;
	config.skin = 0;
	config.fullbright = 0;
	config.bEyeMove = true;
	config.bWireframe = bWireframe;

	if ( g_viewerSettings.renderMode == RM_WIREFRAME || g_viewerSettings.softwareSkin || config.bWireframe || bNormals || bTangentFrame )
	{
		config.bSoftwareSkin = true;
	}
	else
	{
		config.bSoftwareSkin = false;
	}

	config.bSoftwareLighting = false;
	config.bNoHardware = false;
	config.bNoSoftware = false;
	config.bTeeth = true;
	config.bEyes = true;
	config.bFlex = true;
	config.SetNormals( bNormals );
	config.SetTangentFrame( bTangentFrame );
	config.SetZBufferedWireframe( bZBufferWireframe );
	config.bShowEnvCubemapOnly = false;
	g_pStudioRender->UpdateConfig( config );

	MaterialSystem_Config_t matSysConfig = g_pMaterialSystem->GetCurrentConfigForVideoCard();
	extern void InitMaterialSystemConfig(MaterialSystem_Config_t *pConfig);
	InitMaterialSystemConfig( &matSysConfig );
	matSysConfig.nFullbright = 0;
	if( g_viewerSettings.renderMode == RM_SMOOTHSHADED )
	{
		matSysConfig.nFullbright = 2;
	}

	if ( g_dxlevel != 0 )
	{
		matSysConfig.dxSupportLevel = g_dxlevel;
	}
	g_pMaterialSystem->OverrideConfig( matSysConfig, false );
}


//-----------------------------------------------------------------------------
// Draws the skeleton
//-----------------------------------------------------------------------------

void StudioModel::DrawBones( )
{
	// draw bones
	if (!g_viewerSettings.showBones && (g_viewerSettings.highlightBone < 0))
		return;

	CStudioHdr *pStudioHdr = GetStudioHdr();
	mstudiobone_t *pbones = pStudioHdr->pBone( 0 );

	g_pMaterialSystem->Bind( g_materialBones );

	IMesh* pMesh = g_pMaterialSystem->GetDynamicMesh( );
	CMeshBuilder meshBuilder;

	bool drawRed = (g_viewerSettings.highlightBone >= 0);

	for (int i = 0; i < pStudioHdr->numbones(); i++)
	{
		if ( !(pStudioHdr->pBone( i )->flags & BoneMask()))
		{
			continue;
		}

		if (pbones[i].parent >= 0)
		{
			int j = pbones[i].parent;
			if ((g_viewerSettings.highlightBone < 0 ) || (j == g_viewerSettings.highlightBone))
			{
				meshBuilder.Begin( pMesh, MATERIAL_LINES, 1 );

				if (drawRed)
					meshBuilder.Color3ub( 255, 255, 0 );
				else
					meshBuilder.Color3ub( 0, 255, 255 );
				meshBuilder.Position3f( (*g_pStudioRender->GetBoneToWorld( j ))[0][3], (*g_pStudioRender->GetBoneToWorld( j ))[1][3], (*g_pStudioRender->GetBoneToWorld( j ))[2][3]);
				meshBuilder.AdvanceVertex();

				if (drawRed)
					meshBuilder.Color3ub( 255, 255, 0 );
				else
					meshBuilder.Color3ub( 0, 255, 255 );
				meshBuilder.Position3f( (*g_pStudioRender->GetBoneToWorld( i ))[0][3], (*g_pStudioRender->GetBoneToWorld( i ))[1][3], (*g_pStudioRender->GetBoneToWorld( i ))[2][3]);
				meshBuilder.AdvanceVertex();

				meshBuilder.End();
				pMesh->Draw();
			}
		}

		if (g_viewerSettings.highlightBone >= 0)
		{
			if (i != g_viewerSettings.highlightBone)
				continue;
		}

		drawTransform( *g_pStudioRender->GetBoneToWorld( i ) );
	}

	// manadatory to access correct verts
	SetCurrentModel();

	// highlight used vertices with point
	/*
	if (g_viewerSettings.highlightBone >= 0)
	{
		int k, j, n;
		for (i = 0; i < pStudioHdr->numbodyparts; i++)
		{
			for (j = 0; j < pStudioHdr->pBodypart( i )->nummodels; j++)
			{
				mstudiomodel_t *pModel = pStudioHdr->pBodypart( i )->pModel( j );

				const mstudio_modelvertexdata_t *vertData = pModel->GetVertexData();

				meshBuilder.Begin( pMesh, MATERIAL_POINTS, 1 );

				for (k = 0; k < pModel->numvertices; k++)
				{
					for (n = 0; n < vertData->BoneWeights( k )->numbones; n++)
					{
						if (vertData->BoneWeights( k )->bone[n] == g_viewerSettings.highlightBone)
						{
							Vector tmp;
							Transform( *vertData->Position( k ), vertData->BoneWeights( k ), tmp );

							meshBuilder.Color3ub( 0, 255, 255 );
							meshBuilder.Position3f( tmp.x, tmp.y, tmp.z );
							meshBuilder.AdvanceVertex();
							break;
						}
					}
				}

				meshBuilder.End();
				pMesh->Draw();
			}
		}
	}
	*/
}

//-----------------------------------------------------------------------------
// Draws attachments
//-----------------------------------------------------------------------------

void StudioModel::DrawAttachments( )
{
	if (!g_viewerSettings.showAttachments)
		return;

	g_pMaterialSystem->Bind( g_materialBones );

	CStudioHdr *pStudioHdr = GetStudioHdr();
	for (int i = 0; i < pStudioHdr->GetNumAttachments(); i++)
	{
		mstudioattachment_t &pattachments = (mstudioattachment_t &)pStudioHdr->pAttachment( i );

		matrix3x4_t world;
		ConcatTransforms( *g_pStudioRender->GetBoneToWorld( pStudioHdr->GetAttachmentBone( i ) ), pattachments.local, world );

		drawTransform( world );
	}
}


void StudioModel::DrawEditAttachment()
{
	CStudioHdr *pStudioHdr = GetStudioHdr();
	int iEditAttachment = g_viewerSettings.m_iEditAttachment;
	if ( iEditAttachment >= 0 && iEditAttachment < pStudioHdr->GetNumAttachments() )
	{
		g_pMaterialSystem->Bind( g_materialBones );
		
		mstudioattachment_t &pAttachment = (mstudioattachment_t &)pStudioHdr->pAttachment( iEditAttachment );

		matrix3x4_t world;
		ConcatTransforms( *g_pStudioRender->GetBoneToWorld( pStudioHdr->GetAttachmentBone( iEditAttachment ) ), pAttachment.local, world );

		drawTransform( world );
	}
}


//-----------------------------------------------------------------------------
// Draws hitboxes
//-----------------------------------------------------------------------------


static float hullcolor[8][4] = 
{
	{ 1.0, 1.0, 1.0, 1.0 },
	{ 1.0, 0.5, 0.5, 1.0 },
	{ 0.5, 1.0, 0.5, 1.0 },
	{ 1.0, 1.0, 0.5, 1.0 },
	{ 0.5, 0.5, 1.0, 1.0 },
	{ 1.0, 0.5, 1.0, 1.0 },
	{ 0.5, 1.0, 1.0, 1.0 },
	{ 1.0, 1.0, 1.0, 1.0 }
};


void StudioModel::DrawHitboxes( )
{
	CStudioHdr *pStudioHdr = GetStudioHdr();
	if (!g_pAlpha)
	{
		g_pAlpha = g_pMaterialSystem->FindMaterial("debug/debughitbox", TEXTURE_GROUP_OTHER, false);
	}

	if (g_viewerSettings.showHitBoxes || (g_viewerSettings.highlightHitbox >= 0))
	{
		int hitboxset = g_MDLViewer->GetCurrentHitboxSet();

		for (unsigned short j = m_HitboxSets[ hitboxset ].Head(); j != m_HitboxSets[ hitboxset ].InvalidIndex(); j = m_HitboxSets[ hitboxset ].Next(j) )
		{
			// Only draw one hitbox if we've selected it.
			if ((g_viewerSettings.highlightHitbox >= 0) && 
				(g_viewerSettings.highlightHitbox != j))
				continue;

			mstudiobbox_t *pBBox = &m_HitboxSets[ hitboxset ][j];

			float interiorcolor[4];
			int c = pBBox->group % 8;
			interiorcolor[0] = hullcolor[c][0] * 0.7;
			interiorcolor[1] = hullcolor[c][1] * 0.7;
			interiorcolor[2] = hullcolor[c][2] * 0.7;
			interiorcolor[3] = hullcolor[c][3] * 0.4;

			drawTransparentBox( pBBox->bbmin, pBBox->bbmax, *g_pStudioRender->GetBoneToWorld( pBBox->bone ), interiorcolor, hullcolor[ c ] );
		}
	}

	/*
	float color2[] = { 0, 0.7, 1, 0.6 };
	float wirecolor2[] = { 0, 1, 1, 1.0 };
	drawTransparentBox( pStudioHdr->min, pStudioHdr->max, g_viewtransform, color2, wirecolor2 );
	*/

	if (g_viewerSettings.showSequenceBoxes)
	{
		float color[] = { 0.7, 1, 0, 0.6 };
		float wirecolor[] = { 1, 1, 0, 1.0 };

		drawTransparentBox( pStudioHdr->pSeqdesc( m_sequence ).bbmin, pStudioHdr->pSeqdesc( m_sequence ).bbmax, g_viewtransform, color, wirecolor );
	}
}

void StudioModel::DrawIllumPosition( )
{
	if( !g_viewerSettings.showIllumPosition )
		return;

	CStudioHdr *pStudioHdr = GetStudioHdr();

	Vector modelPt0;
	Vector modelPt1;
	Vector worldPt0;
	Vector worldPt1;

	// draw axis through illum position
	VectorCopy(pStudioHdr->illumposition(), modelPt0);
	VectorCopy(pStudioHdr->illumposition(), modelPt1);
	modelPt0.x -= 4;
	modelPt1.x += 4;
	VectorTransform (modelPt0, g_viewtransform, worldPt0);
	VectorTransform (modelPt1, g_viewtransform, worldPt1);
	drawLine( worldPt0, worldPt1, 255, 0, 0 );

	VectorCopy(pStudioHdr->illumposition(), modelPt0);
	VectorCopy(pStudioHdr->illumposition(), modelPt1);
	modelPt0.y -= 4;
	modelPt1.y += 4;
	VectorTransform (modelPt0, g_viewtransform, worldPt0);
	VectorTransform (modelPt1, g_viewtransform, worldPt1);
	drawLine( worldPt0, worldPt1, 0, 255, 0 );

	VectorCopy(pStudioHdr->illumposition(), modelPt0);
	VectorCopy(pStudioHdr->illumposition(), modelPt1);
	modelPt0.z -= 4;
	modelPt1.z += 4;
	VectorTransform (modelPt0, g_viewtransform, worldPt0);
	VectorTransform (modelPt1, g_viewtransform, worldPt1);
	drawLine( worldPt0, worldPt1, 0, 0, 255 );

}

//-----------------------------------------------------------------------------
// Draws the physics model
//-----------------------------------------------------------------------------

void StudioModel::DrawPhysicsModel( )
{
	if (!g_viewerSettings.showPhysicsModel)
		return;

	if ( g_viewerSettings.renderMode == RM_WIREFRAME && m_pPhysics->Count() == 1 )
	{
		// show the convex pieces in solid
		DrawPhysConvex( m_pPhysics->GetMesh(0), g_materialFlatshaded );
	}
	else
	{
		for (int i = 0; i < m_pPhysics->Count(); i++)
		{
			float red[] = { 1.0, 0, 0, 0.25 };
			float yellow[] = { 1.0, 1.0, 0, 0.5 };

			CPhysmesh *pmesh = m_pPhysics->GetMesh(i);
			int boneIndex = FindBone(pmesh->m_boneName);

			if ( boneIndex >= 0 )
			{
				if ( (i+1) == g_viewerSettings.highlightPhysicsBone )
				{
					DrawPhysmesh( pmesh, boneIndex, g_materialBones, red );
				}
				else
				{
					if ( g_viewerSettings.highlightPhysicsBone < 1 )
					{
						// yellow for most
						DrawPhysmesh( pmesh, boneIndex, g_materialBones, yellow );
					}
				}
			}
			else
			{
				DrawPhysmesh( pmesh, -1, g_materialBones, red );
			}
		}
	}
}




void StudioModel::SetViewTarget( void )
{
	// only valid if the attachment bones are used
	if ((BoneMask() & BONE_USED_BY_ATTACHMENT) == 0)
	{
		return;
	}

	int iEyeAttachment = LookupAttachment( "eyes" );
	
	if (iEyeAttachment == -1)
		return;

	Vector local;
	Vector tmp;

	CStudioHdr *pStudioHdr = GetStudioHdr();
	mstudioattachment_t &patt = (mstudioattachment_t &)pStudioHdr->pAttachment( iEyeAttachment );
	matrix3x4_t attToWorld;
	ConcatTransforms( *g_pStudioRender->GetBoneToWorld( pStudioHdr->GetAttachmentBone( iEyeAttachment ) ), patt.local, attToWorld ); 

	// look forward
	local = Vector( 32, 0, 0 );
	if (m_flHeadTurn != 0.0f)
	{
		// aim the eyes
		VectorITransform( m_vecEyeTarget, attToWorld, local );
	}
	
	float flDist = local.Length();

	VectorNormalize( local );

	// calculate animated eye deflection
	Vector eyeDeflect;
	QAngle eyeAng( GetFlexController("eyes_updown"), GetFlexController("eyes_rightleft"), 0 );

	// debugoverlay->AddTextOverlay( m_vecOrigin + Vector( 0, 0, 64 ), 0, 0, "%.2f %.2f", eyeAng.x, eyeAng.y );

	AngleVectors( eyeAng, &eyeDeflect );
	eyeDeflect.x = 0;

	// reduce deflection the more the eye is off center
	// FIXME: this angles make no damn sense
	eyeDeflect = eyeDeflect * (local.x * local.x);
	local = local + eyeDeflect;
	VectorNormalize( local );

	// check to see if the eye is aiming outside a 30 degree cone
	if (local.x < 0.866) // cos(30)
	{
		// if so, clamp it to 30 degrees offset
		// debugoverlay->AddTextOverlay( m_vecOrigin + Vector( 0, 0, 64 ), 0, 0, "%.2f %.2f %.2f", local.x, local.y, local.z );
		local.x = 0;
		float d = local.LengthSqr();
		if (d > 0.0)
		{
			d = sqrt( (1.0 - 0.866 * 0.866) / (local.y*local.y + local.z*local.z) );
			local.x = 0.866;
			local.y = local.y * d;
			local.z = local.z * d;
		}
		else
		{
			local.x = 1.0;
		}
	}
	local = local * flDist;
	VectorTransform( local, attToWorld, tmp );

	g_pStudioRender->SetEyeViewTarget( tmp );
}


float UTIL_VecToYaw( const matrix3x4_t& matrix, const Vector &vec )
{
	Vector tmp = vec;
	VectorNormalize( tmp );

	float x = matrix[0][0] * tmp.x + matrix[1][0] * tmp.y + matrix[2][0] * tmp.z;
	float y = matrix[0][1] * tmp.x + matrix[1][1] * tmp.y + matrix[2][1] * tmp.z;

	if (x == 0.0f && y == 0.0f)
		return 0.0f;
	
	float yaw = atan2( -y, x );

	yaw = RAD2DEG(yaw);

	if (yaw < 0)
		yaw += 360;

	return yaw;
}


float UTIL_VecToPitch( const matrix3x4_t& matrix, const Vector &vec )
{
	Vector tmp = vec;
	VectorNormalize( tmp );

	float x = matrix[0][0] * tmp.x + matrix[1][0] * tmp.y + matrix[2][0] * tmp.z;
	float z = matrix[0][2] * tmp.x + matrix[1][2] * tmp.y + matrix[2][2] * tmp.z;

	if (x == 0.0f && z == 0.0f)
		return 0.0f;
	
	float pitch = atan2( z, x );

	pitch = RAD2DEG(pitch);

	if (pitch < 0)
		pitch += 360;

	return pitch;
}


float UTIL_AngleDiff( float destAngle, float srcAngle )
{
	float delta;

	delta = destAngle - srcAngle;
	if ( destAngle > srcAngle )
	{
		while ( delta >= 180 )
			delta -= 360;
	}
	else
	{
		while ( delta <= -180 )
			delta += 360;
	}
	return delta;
}


void StudioModel::UpdateBoneChain(
	Vector pos[], 
	Quaternion q[], 
	int	iBone,
	matrix3x4_t *pBoneToWorld )
{
	matrix3x4_t bonematrix;

	QuaternionMatrix( q[iBone], pos[iBone], bonematrix );

	CStudioHdr *pStudioHdr = GetStudioHdr();
	int parent = pStudioHdr->pBone( iBone )->parent;
	if (parent == -1) 
	{
		ConcatTransforms( g_viewtransform, bonematrix, pBoneToWorld[iBone] );
	}
	else
	{
		// evil recursive!!!
		UpdateBoneChain( pos, q, parent, pBoneToWorld );
		ConcatTransforms( pBoneToWorld[parent], bonematrix, pBoneToWorld[iBone] );
	}
}




void StudioModel::GetBodyPoseParametersFromFlex( )
{
	float flGoal;

	flGoal = GetFlexController( "move_rightleft" );
	SetPoseParameter( "body_trans_Y", flGoal );
	
	flGoal = GetFlexController( "move_forwardback" );
	SetPoseParameter( "body_trans_X", flGoal );

	flGoal = GetFlexController( "move_updown" );
	SetPoseParameter( "body_lift", flGoal );

	flGoal = GetFlexController( "body_rightleft" ) + GetBodyYaw();
	SetPoseParameter( "body_yaw", flGoal );

	flGoal = GetFlexController( "body_updown" );
	SetPoseParameter( "body_pitch", flGoal );

	flGoal = GetFlexController( "body_tilt" );
	SetPoseParameter( "body_roll", flGoal );

	flGoal = GetFlexController( "chest_rightleft" ) + GetSpineYaw();
	SetPoseParameter( "spine_yaw", flGoal );

	flGoal = GetFlexController( "chest_updown" );
	SetPoseParameter( "spine_pitch", flGoal );

	flGoal = GetFlexController( "chest_tilt" );
	SetPoseParameter( "spine_roll", flGoal );

	flGoal = GetFlexController( "head_forwardback" );
	SetPoseParameter( "neck_trans", flGoal );

	flGoal = GetFlexController( "gesture_updown" );
	SetPoseParameter( "gesture_height", flGoal );

	flGoal = GetFlexController( "gesture_rightleft" );
	SetPoseParameter( "gesture_width", flGoal );
}




void StudioModel::SetHeadPosition( Vector pos[], Quaternion q[] )
{
	static Vector		pos2[MAXSTUDIOBONES];
	static Quaternion	q2[MAXSTUDIOBONES];
	Vector vTargetPos = Vector( 0, 0, 0 );

	if (m_nSolveHeadTurn == 0)
		return;

	if (m_dt == 0.0f)
	{
		m_dt = m_dt;
	}
	else
	{
		m_dt = m_dt;
	}

	// GetAttachment( "eyes", vEyePosition, vEyeAngles );
	int iEyeAttachment = LookupAttachment( "forward" );
	if (iEyeAttachment == -1)
		return;

	CStudioHdr *pStudioHdr = GetStudioHdr();
	mstudioattachment_t &patt = (mstudioattachment_t &)pStudioHdr->pAttachment( iEyeAttachment );

	matrix3x4_t attToWorld;
	int iBone =  pStudioHdr->GetAttachmentBone( iEyeAttachment );
	BuildBoneChain( pStudioHdr, g_viewtransform, pos, q, iBone, g_pStudioRender->GetBoneToWorld( 0 ) );
	ConcatTransforms( *g_pStudioRender->GetBoneToWorld( iBone ), patt.local, attToWorld );

	Vector vDefault;
	VectorRotate( Vector( 100, 0, 0 ), attToWorld, vDefault );

  	int iLoops = 1;
  	float dt = m_dt;
  	if (m_nSolveHeadTurn == 2)
  	{
  		iLoops = 100;
  		dt = 0.1;
  	}

	Vector vEyes;
	MatrixPosition( attToWorld, vEyes );

	Vector vForward = m_vecHeadTarget - vEyes;
	vTargetPos = vForward * m_flHeadTurn + vDefault * (1 - m_flHeadTurn);

	SetPoseParameter( "head_pitch", 0.0 );
	SetPoseParameter( "head_yaw", 0.0 );
	SetPoseParameter( "head_roll", 0.0 );
	SetHeadPosition( attToWorld, vTargetPos, dt );

	// Msg( "yaw %f pitch %f\n", vEyeAngles.y, vEyeAngles.x );
}



float StudioModel::SetHeadPosition( matrix3x4_t& attToWorld, Vector const &vTargetPos, float dt )
{
	float flDiff;
	int iPose;
	QAngle vEyeAngles;
	float flMoved = 0.0f;
	matrix3x4_t targetXform, invAttToWorld;
	matrix3x4_t headXform;

	// align current "forward direction" to target direction
	targetXform = attToWorld;
	Studio_AlignIKMatrix( targetXform, vTargetPos );

	// calc head movement needed
	MatrixInvert( attToWorld, invAttToWorld );
	ConcatTransforms( invAttToWorld, targetXform, headXform );

	MatrixAngles( headXform, vEyeAngles );

	// FIXME: add chest compression

	// Msg( "yaw %f pitch %f\n", vEyeAngles.y, vEyeAngles.x );

	float flMin, flMax;

#if 1
	//--------------------------------------
	// Set head yaw
	//--------------------------------------
	// flDiff = vEyeAngles.y + GetFlexController( "head_rightleft" );
	iPose = LookupPoseParameter( "head_yaw" );
	GetPoseParameterRange( iPose, &flMin, &flMax );
	flDiff = RangeCompressor( vEyeAngles.y + GetFlexController( "head_rightleft" ), flMin, flMax, 0.0 );
	SetPoseParameter( iPose, flDiff );
#endif

#if 1
	//--------------------------------------
	// Set head pitch
	//--------------------------------------
	iPose = LookupPoseParameter( "head_pitch" );
	GetPoseParameterRange( iPose, &flMin, &flMax );
	flDiff = RangeCompressor( vEyeAngles.x + GetFlexController( "head_updown" ), flMin, flMax, 0.0 );
	SetPoseParameter( iPose, flDiff );
#endif

#if 1
	//--------------------------------------
	// Set head roll
	//--------------------------------------
	iPose = LookupPoseParameter( "head_roll" );
	GetPoseParameterRange( iPose, &flMin, &flMax );
	flDiff = RangeCompressor( vEyeAngles.z + GetFlexController( "head_tilt" ), flMin, flMax, 0.0 );
	SetPoseParameter( iPose, flDiff );
#endif

	return flMoved;
}


DrawModelInfo_t g_DrawModelInfo;
bool g_bDrawModelInfoValid = false;
/*
================
StudioModel::DrawModel
inputs:
	currententity
	r_entorigin
================
*/
int StudioModel::DrawModel( bool mergeBones )
{
	MDLCACHE_CRITICAL_SECTION_( g_pMDLCache );

	CStudioHdr *pStudioHdr = GetStudioHdr();
	if (!pStudioHdr)
		return 0;

	g_smodels_total++; // render data cache cookie

	// JasonM & garymcthack - should really only do this once a frame and at init time.
	UpdateStudioRenderConfig( g_viewerSettings.renderMode == RM_WIREFRAME, false,
							  g_viewerSettings.showNormals,
							  g_viewerSettings.showTangentFrame );

	// NOTE: UpdateStudioRenderConfig can delete the studio hdr
	pStudioHdr = GetStudioHdr();
	if ( !pStudioHdr || pStudioHdr->numbodyparts() == 0)
		return 0;

	// Construct a transform to apply to the model. The camera is stuck in a fixed position
	AngleMatrix( m_angles, g_viewtransform );

	Vector vecModelOrigin;
	VectorMultiply( m_origin, -1.0f, vecModelOrigin );
	MatrixSetColumn( vecModelOrigin, 3, g_viewtransform );
	
	// These values HAVE to be sent down for LOD to work correctly.
	Vector viewOrigin, viewRight, viewUp, viewPlaneNormal;
	g_pStudioRender->SetViewState( vec3_origin, Vector(0, 1, 0), Vector(0, 0, 1), Vector( 1, 0, 0 ) );

	//	g_pStudioRender->SetEyeViewTarget( viewOrigin );
	
	SetUpBones( mergeBones );

	SetViewTarget( );

	SetupLighting( );


	// process flex controllers
	extern float g_flexdescweight[MAXSTUDIOFLEXDESC]; // garymcthack
	extern float g_flexdescweight2[MAXSTUDIOFLEXDESC]; // garymcthack

	int i;

	for (i = 0; i < pStudioHdr->numflexdesc(); i++)
	{
		g_flexdescweight[i] = 0.0;
	}

	RunFlexRules();

	float d = 0.8;

	if (m_dt != 0)
	{
		d = ExponentialDecay( 0.8, 0.033, m_dt );
	}

	for (i = 0; i < pStudioHdr->numflexdesc(); i++)
	{
		g_flexdescweight2[i] = g_flexdescweight2[i] * d + g_flexdescweight[i] * (1 - d);
	}
	g_pStudioRender->SetFlexWeights( MAXSTUDIOFLEXDESC, g_flexdescweight, g_flexdescweight2 );

	
	// draw

	g_pStudioRender->SetAlphaModulation( 1.0f );

	int count = 0;

	g_bDrawModelInfoValid = true;
	memset( &g_DrawModelInfo, 0, sizeof( g_DrawModelInfo ) );
	g_DrawModelInfo.m_pStudioHdr = (studiohdr_t *)pStudioHdr->GetRenderHdr();
	g_DrawModelInfo.m_pHardwareData = GetHardwareData();
	g_DrawModelInfo.m_Decals = STUDIORENDER_DECAL_INVALID;
	g_DrawModelInfo.m_Skin = m_skinnum;
	g_DrawModelInfo.m_Body = m_bodynum;
	g_DrawModelInfo.m_HitboxSet = g_MDLViewer->GetCurrentHitboxSet();
	g_DrawModelInfo.m_pClientEntity = NULL;
	g_DrawModelInfo.m_Lod = g_viewerSettings.autoLOD ? -1 : g_viewerSettings.lod;
	g_DrawModelInfo.m_ppColorMeshes = NULL;

	if( g_viewerSettings.renderMode == RM_BONEWEIGHTS )
	{
		g_DrawModelInfo.m_Lod = 0;
		DebugDrawModelBoneWeights( g_pStudioRender, g_DrawModelInfo, vecModelOrigin, &m_LodUsed, &m_LodMetric );
	}
	else
	{
		// Draw the model normally (may include normal and/or tangent line segments)
		count = g_pStudioRender->DrawModel( g_DrawModelInfo, vecModelOrigin, &m_LodUsed, &m_LodMetric );

		// Optionally overlay wireframe...
		if ( g_viewerSettings.overlayWireframe && !(g_viewerSettings.renderMode == RM_WIREFRAME) )
		{
			// Set the state to trigger wireframe rendering
			UpdateStudioRenderConfig( true, true, false, false );

			// Draw the wireframe over top of the model
			count = g_pStudioRender->DrawModel( g_DrawModelInfo, vecModelOrigin, &m_LodUsed, &m_LodMetric );

			// Restore the studio render config
			UpdateStudioRenderConfig( g_viewerSettings.renderMode == RM_WIREFRAME, false,
										g_viewerSettings.showNormals,
										g_viewerSettings.showTangentFrame );
		}
	}


	DrawBones();
	DrawAttachments();
	DrawEditAttachment();
	DrawHitboxes();
	DrawPhysicsModel();
	DrawIllumPosition();

	// Only draw the shadow if the ground is also drawn
	if ( g_viewerSettings.showShadow &&  g_viewerSettings.showGround )
	{
		matrix3x4_t invViewTransform;

		MatrixInvert( g_viewtransform, invViewTransform );

		for (int i = 0; i < pStudioHdr->numbones(); i++) 
		{
			matrix3x4_t *pMatrix = g_pStudioRender->GetBoneToWorld( i );

			matrix3x4_t tmp1;

			ConcatTransforms( invViewTransform, *pMatrix, tmp1 );
			tmp1[2][0] = 0.0;
			tmp1[2][1] = 0.0;
			tmp1[2][2] = 0.0;
			tmp1[2][3] = 0.05;
			ConcatTransforms( g_viewtransform, tmp1, *pMatrix );
		}
		g_DrawModelInfo.m_Lod = GetHardwareData()->m_NumLODs - 1;

		float zero[4] = { 0, 0, 0, 0 };
		g_pStudioRender->SetColorModulation( zero );
		g_pStudioRender->ForcedMaterialOverride( g_materialShadow );

		// Turn off any wireframe, normals or tangent frame display for the drop shadow
		UpdateStudioRenderConfig( false, false, false, false );

		g_pStudioRender->DrawModel( g_DrawModelInfo, vecModelOrigin, NULL, NULL );

		// Restore the studio render config
		UpdateStudioRenderConfig( g_viewerSettings.renderMode == RM_WIREFRAME, false,
								  g_viewerSettings.showNormals,
								  g_viewerSettings.showTangentFrame );

		g_pStudioRender->ForcedMaterialOverride( NULL );
		float one[4] = { 1, 1, 1, 1 };
		g_pStudioRender->SetColorModulation( one );
	}

	return count;
}


void StudioModel::DrawPhysmesh( CPhysmesh *pMesh, int boneIndex, IMaterial* pMaterial, float* color )
{
	matrix3x4_t *pMatrix;
	if ( boneIndex >= 0 )
	{
		pMatrix = g_pStudioRender->GetBoneToWorld( boneIndex );
	}
	else
	{
		pMatrix = &g_viewtransform;
	}

	g_pMaterialSystem->Bind( pMaterial );
	IMesh* pMatMesh = g_pMaterialSystem->GetDynamicMesh( );

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMatMesh, MATERIAL_TRIANGLES, pMesh->m_vertCount/3 );

	int vertIndex = 0;
	for ( int i = 0; i < pMesh->m_vertCount; i+=3 )
	{
		Vector v;
		
		VectorTransform (pMesh->m_pVerts[vertIndex], *pMatrix, v);
		meshBuilder.Position3fv( v.Base() );
		meshBuilder.Color4fv( color );
		meshBuilder.AdvanceVertex();

		vertIndex ++;
		VectorTransform (pMesh->m_pVerts[vertIndex], *pMatrix, v);
		meshBuilder.Position3fv( v.Base() );
		meshBuilder.Color4fv( color );
		meshBuilder.AdvanceVertex();

		vertIndex ++;
		VectorTransform (pMesh->m_pVerts[vertIndex], *pMatrix, v);
		meshBuilder.Position3fv( v.Base() );							 
		meshBuilder.Color4fv( color );
		meshBuilder.AdvanceVertex();

		vertIndex ++;
	}
	meshBuilder.End();
	pMatMesh->Draw();
}


void RandomColor( float *color, int key )
{
	static bool first = true;
	static colorVec colors[256];

	if ( first )
	{
		int r, g, b;
		first = false;
		for ( int i = 0; i < 256; i++ )
		{
			do 
			{
				r = rand()&255;
				g = rand()&255;
				b = rand()&255;
			} while ( (r+g+b)<256 );
			colors[i].r = r;
			colors[i].g = g;
			colors[i].b = b;
			colors[i].a = 255;
		}
	}

	int index = key & 255;
	color[0] = colors[index].r * (1.f / 255.f);
	color[1] = colors[index].g * (1.f / 255.f);
	color[2] = colors[index].b * (1.f / 255.f);
	color[3] = colors[index].a * (1.f / 255.f);
}

void StudioModel::DrawPhysConvex( CPhysmesh *pMesh, IMaterial* pMaterial )
{
	matrix3x4_t &matrix = g_viewtransform;

	g_pMaterialSystem->Bind( pMaterial );

	for ( int i = 0; i < pMesh->m_pCollisionModel->ConvexCount(); i++ )
	{
		float color[4];
		RandomColor( color, i );
		IMesh* pMatMesh = g_pMaterialSystem->GetDynamicMesh( );
		CMeshBuilder meshBuilder;
		int triCount = pMesh->m_pCollisionModel->TriangleCount( i );
		meshBuilder.Begin( pMatMesh, MATERIAL_TRIANGLES, triCount );

		for ( int j = 0; j < triCount; j++ )
		{
			Vector objectSpaceVerts[3];
			pMesh->m_pCollisionModel->GetTriangleVerts( i, j, objectSpaceVerts );

			for ( int k = 0; k < 3; k++ )
			{
				Vector v;
				
				VectorTransform (objectSpaceVerts[k], matrix, v);
				meshBuilder.Position3fv( v.Base() );
				meshBuilder.Color4fv( color );
				meshBuilder.AdvanceVertex();
			}
		}
		meshBuilder.End();
		pMatMesh->Draw();
	}
}


void StudioModel::Transform( Vector const &in1, mstudioboneweight_t const *pboneweight, Vector &out1 )
{
	if (pboneweight->numbones == 1)
	{
		VectorTransform( in1, *g_pStudioRender->GetPoseToWorld(pboneweight->bone[0]), out1 );
	}
	else
	{
		Vector out2;

		VectorFill( out1, 0 );

		for (int i = 0; i < pboneweight->numbones; i++)
		{
			VectorTransform( in1, *g_pStudioRender->GetPoseToWorld(pboneweight->bone[i]), out2 );
			VectorMA( out1, pboneweight->weight[i], out2, out1 );
		}
	}
}


void StudioModel::Rotate( Vector const &in1, mstudioboneweight_t const *pboneweight, Vector &out1 )
{
	if (pboneweight->numbones == 1)
	{
		VectorRotate( in1, *g_pStudioRender->GetPoseToWorld(pboneweight->bone[0]), out1 );
	}
	else
	{
		Vector out2;

		VectorFill( out1, 0 );

		for (int i = 0; i < pboneweight->numbones; i++)
		{
			VectorRotate( in1, *g_pStudioRender->GetPoseToWorld(pboneweight->bone[i]), out2 );
			VectorMA( out1, pboneweight->weight[i], out2, out1 );
		}
		VectorNormalize( out1 );
	}
}







/*
================

================
*/


int StudioModel::GetLodUsed( void )
{
	return m_LodUsed;
}

float StudioModel::GetLodMetric( void )
{
	return m_LodMetric;
}


const char *StudioModel::GetKeyValueText( int iSequence )
{
	CStudioHdr *pStudioHdr = GetStudioHdr();
	return Studio_GetKeyValueText( pStudioHdr, iSequence );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : solve - 
//-----------------------------------------------------------------------------
void StudioModel::SetSolveHeadTurn( int solve )
{
	m_nSolveHeadTurn = solve;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int StudioModel::GetSolveHeadTurn() const
{
	return m_nSolveHeadTurn;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : target - 
//-----------------------------------------------------------------------------
void StudioModel::ClearHeadTarget( void )
{
	m_flHeadTurn = 0.0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : target - 
//-----------------------------------------------------------------------------
void StudioModel::SetHeadTarget( const Vector& target, float turn )
{
	if (m_flHeadTurn == 0)
	{
		m_vecHeadTarget = target;
		m_flHeadTurn = turn;
	}
	else
	{
		m_flHeadTurn = m_flHeadTurn * (1 - turn) + turn;
		float w = turn / m_flHeadTurn;
		m_vecHeadTarget = m_vecHeadTarget * (1 - w) + target * w;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : target - 
//-----------------------------------------------------------------------------
void StudioModel::GetHeadTarget( Vector& target ) const
{
	target = m_vecHeadTarget;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : target - 
//-----------------------------------------------------------------------------
void StudioModel::SetEyeTarget( const Vector& target )
{
	m_vecEyeTarget = target;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : target - 
//-----------------------------------------------------------------------------
void StudioModel::GetEyeTarget( Vector& target ) const
{
	target = m_vecEyeTarget;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : hastarget - 
//-----------------------------------------------------------------------------
void StudioModel::SetHasLookTarget( bool hastarget )
{
	m_bHasLookTarget = hastarget;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool StudioModel::GetHasLookTarget() const
{
	return m_bHasLookTarget;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output :
//-----------------------------------------------------------------------------
void StudioModel::SetModelYaw( float flYaw )
{
	m_flModelYaw = flYaw;
}

float StudioModel::GetModelYaw( void ) const 
{
	return m_flModelYaw;
}

void StudioModel::SetBodyYaw( float flYaw )
{
	m_flBodyYaw = flYaw;
}

float StudioModel::GetBodyYaw( void ) const 
{
	return m_flBodyYaw;
}

void StudioModel::SetSpineYaw( float flYaw )
{
	m_flSpineYaw = flYaw;
}

float StudioModel::GetSpineYaw( void ) const 
{
	return m_flSpineYaw;
}

