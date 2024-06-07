#ifndef TRANSFORM_H
#define TRANSFORM_H

#ifdef _WIN32
#pragma once
#endif

#include "mathlib/vector.h"
#include "mathlib/mathlib.h"

class ALIGN16 CTransform
{
public:
	CTransform() {}
	CTransform( const Vector &v, const Quaternion &q ) : m_vPosition(v), m_orientation(q) {}

	bool IsValid() const
	{
		return m_vPosition.IsValid() && m_orientation.IsValid();
	}

	bool operator==(const CTransform& v) const;
	bool operator!=(const CTransform& v) const;

	void SetToIdentity();
	
public:
	VectorAligned m_vPosition;
	QuaternionAligned m_orientation;

} ALIGN16_POST;

inline void CTransform::SetToIdentity()
{
	m_vPosition = vec3_origin;
	m_vPosition.w = 1.0f;
	m_orientation = quat_identity;
}

inline bool CTransform::operator==(const CTransform& t) const
{
	return t.m_vPosition == m_vPosition && t.m_orientation == m_orientation;
}

inline bool CTransform::operator!=(const CTransform& t) const
{
	return t.m_vPosition != m_vPosition || t.m_orientation != m_orientation;
}

#endif // TRANSFORM_H
