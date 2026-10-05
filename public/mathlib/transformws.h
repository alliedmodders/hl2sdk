#ifndef TRANSFORM_WORLD_SPACE_H
#define TRANSFORM_WORLD_SPACE_H

#ifdef _WIN32
#pragma once
#endif

#include "platform.h"
#include "mathlib/vectorws.h"
#include "mathlib/mathlib.h"

class ALIGN16 CTransformWS
{
public:
	CTransformWS() {}
	CTransformWS( const VectorWS &v, const QuaternionWS &q ) : m_vPosition(v), m_orientation(q) {}

	bool IsValid() const
	{
		return m_vPosition.IsValid() && m_orientation.IsValid();
	}

	bool operator==(const CTransformWS& v) const;
	bool operator!=(const CTransformWS& v) const;

	void SetToIdentity();
	
public:
	VectorWS m_vPosition;
	QuaternionWS m_orientation;

} ALIGN16_POST;

inline void CTransformWS::SetToIdentity()
{
	m_vPosition = vec3ws_origin;
	m_orientation = quatws_identity;
}

inline bool CTransformWS::operator==(const CTransformWS& t) const
{
	return t.m_vPosition == m_vPosition && t.m_orientation == m_orientation;
}

inline bool CTransformWS::operator!=(const CTransformWS& t) const
{
	return t.m_vPosition != m_vPosition || t.m_orientation != m_orientation;
}

#endif // TRANSFORM_WORLD_SPACE_H
