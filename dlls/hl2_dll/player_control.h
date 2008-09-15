//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PLAYER_CONTROL_H
#define PLAYER_CONTROL_H
#ifdef _WIN32
#pragma once
#endif

#include "basecombatcharacter.h"


enum PC_TraceDir_t
{
	PC_TRACE_LEFT,
	PC_TRACE_RIGHT,
	PC_TRACE_UP,
	PC_TRACE_DOWN,
	PC_TRACE_FORWARD,
	PC_TRACE_LAST,
};

#define PCONTROL_CLIP_DIST		1600

class CPlayer_Control : public CBaseCombatCharacter
{
public:
	DECLARE_CLASS( CPlayer_Control, CBaseCombatCharacter );

	void	ControlActivate( void );
	void	ControlDeactivate( void );

	void	Precache(void);
	void	Spawn(void);
	bool	CreateVPhysics( void );
	int		UpdateTransmitState();
	
	// Inputs
	void InputActivate( inputdata_t &inputdata );
	void InputDeactivate( inputdata_t &inputdata );
	void InputSetThrust( inputdata_t &inputdata );
	void InputSetSideThrust( inputdata_t &inputdata );

	float				PlayerControlClipDistance(void);

	bool				m_bActive;

	// Static
	int					m_nPClipTraceDir;
	float				m_flCurrMinClipDist;
	float				m_flLastMinClipDist;
	
	int					m_nSaveFOV;
	Vector				m_vSaveOrigin;
	QAngle				m_vSaveAngles;
	MoveType_t			m_nSaveMoveType;
	MoveCollide_t		m_nSaveMoveCollide;
	Vector				m_vSaveViewOffset;
	CBaseCombatWeapon*	m_pSaveWeapon;

	DECLARE_DATADESC();
};

#endif // PLAYER_CONTROL_H
