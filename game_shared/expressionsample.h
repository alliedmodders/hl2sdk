//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef EXPRESSIONSAMPLE_H
#define EXPRESSIONSAMPLE_H
#ifdef _WIN32
#pragma once
#endif

#include "interpolatortypes.h"

#pragma pack(1)
struct EdgeInfo_t
{
	EdgeInfo_t() : 
		m_bActive( false ),
		m_CurveType( CURVE_DEFAULT ),
		m_flZeroPos( 0.0f )
	{
	}

	bool			m_bActive;
	unsigned short 	m_CurveType;
	float			m_flZeroPos;
};

struct CExpressionSample
{
	CExpressionSample() :
		value( 0.0f ),
		time( 0.0f )
	{
		selected = 0;
		m_curvetype = CURVE_DEFAULT;
	}

	void SetCurveType( int curveType )
	{
		m_curvetype = curveType;
	}

	int	GetCurveType() const
	{
		return m_curvetype;
	}

	// Height
	float			value;
	// time from start of event
	float			time;

	unsigned short	selected	: 1;
private:
	unsigned short	m_curvetype	: 15;
};
#pragma pack()

//-----------------------------------------------------------------------------
// Purpose: Provides generic access to scene or event ramp data
//-----------------------------------------------------------------------------
class ICurveDataAccessor
{
public:
	virtual bool	CurveHasEndTime() = 0; // only matters for events
	virtual int		CurveGetSampleCount() = 0;
	virtual CExpressionSample *CurveGetBoundedSample( int idx, bool& bClamped ) = 0;
	virtual int		GetDefaultCurveType() = 0;
};

#endif // EXPRESSIONSAMPLE_H
