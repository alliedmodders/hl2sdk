//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "C_Point_Camera.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_CLIENTCLASS_DT( C_PointCamera, DT_PointCamera, CPointCamera )
	RecvPropFloat( RECVINFO( m_FOV ) ), 
	RecvPropFloat( RECVINFO( m_Resolution ) ), 
	RecvPropInt( RECVINFO( m_bFogEnable ) ),
	RecvPropInt( RECVINFO( m_FogColor ) ),
	RecvPropFloat( RECVINFO( m_flFogStart ) ), 
	RecvPropFloat( RECVINFO( m_flFogEnd ) ), 
	RecvPropInt( RECVINFO( m_bActive ) ),
	RecvPropInt( RECVINFO( m_bUseScreenAspectRatio ) ),
END_RECV_TABLE()

C_EntityClassList<C_PointCamera> g_PointCameraList;
C_PointCamera *C_EntityClassList<C_PointCamera>::m_pClassList = NULL;

C_PointCamera* GetPointCameraList()
{
	return g_PointCameraList.m_pClassList;
}

C_PointCamera::C_PointCamera()
{
	m_bActive = false;
	m_bFogEnable = false;

	g_PointCameraList.Insert( this );
}

C_PointCamera::~C_PointCamera()
{
	g_PointCameraList.Remove( this );
}

bool C_PointCamera::ShouldDraw()
{
	return false;
}

float C_PointCamera::GetFOV()
{
	return m_FOV;
}

float C_PointCamera::GetResolution()
{
	return m_Resolution;
}

bool C_PointCamera::IsFogEnabled()
{
	return m_bFogEnable;
}

void C_PointCamera::GetFogColor( unsigned char &r, unsigned char &g, unsigned char &b )
{
	r = m_FogColor.r;
	g = m_FogColor.g;
	b = m_FogColor.b;
}

float C_PointCamera::GetFogStart()
{
	return m_flFogStart;
}

float C_PointCamera::GetFogEnd()
{
	return m_flFogEnd;
}

bool C_PointCamera::IsActive()
{
	return m_bActive;
}

