#ifndef RAY_H
#define RAY_H
#ifdef _WIN32
#pragma once
#endif

#include "mathlib/vector.h"

enum RayType_t : uint8
{
	RAY_TYPE_LINE = 0,
	RAY_TYPE_SPHERE,
	RAY_TYPE_HULL,
	RAY_TYPE_CAPSULE,
	RAY_TYPE_MESH,
};

//-----------------------------------------------------------------------------
// A ray...
//-----------------------------------------------------------------------------
struct Ray_t
{
	Ray_t() { Init( Vector( 0.0f, 0.0f, 0.0f ) ); }
	Ray_t( const Vector& vStartOffset ) { Init( vStartOffset ); }
	Ray_t( const Vector& vCenter, float flRadius ) { Init( vCenter, flRadius ); }
	Ray_t( const Vector& vMins, const Vector& vMaxs ) { Init( vMins, vMaxs ); }
	Ray_t( const Vector& vCenterA, const Vector& vCenterB, float flRadius ) { Init( vCenterA, vCenterB, flRadius ); }
	Ray_t( const Vector& vMins, const Vector& vMaxs, const Vector* pVertices, int nNumVertices ) { Init( vMins, vMaxs, pVertices, nNumVertices ); }
	
	void Init( const Vector& vStartOffset )
	{
		m_Line.m_vStartOffset = vStartOffset;
		m_Line.m_flRadius = 0.0f;
		m_eType = RAY_TYPE_LINE;
	}
	
	void Init( const Vector& vCenter, float flRadius )
	{
		if ( flRadius > 0.0f )
		{
			m_Sphere.m_vCenter = vCenter;
			m_Sphere.m_flRadius = flRadius;
			m_eType = RAY_TYPE_SPHERE;
		}
		else
		{
			Init( vCenter );
		}
	}
	
	void Init( const Vector& vMins, const Vector& vMaxs )
	{
		if ( vMins != vMaxs )
		{
			m_Hull.m_vMins = vMins;
			m_Hull.m_vMaxs = vMaxs;
			m_eType = RAY_TYPE_HULL;
		}
		else
		{
			Init( vMins );
		}
	}
	
	void Init( const Vector& vCenterA, const Vector& vCenterB, float flRadius )
	{
		if ( vCenterA != vCenterB )
		{
			if ( flRadius > 0.0f )
			{
				m_Capsule.m_vCenter[0] = vCenterA;
				m_Capsule.m_vCenter[1] = vCenterB;
				m_Capsule.m_flRadius = flRadius;
				m_eType = RAY_TYPE_CAPSULE;
			}
			else
			{
				Init( vCenterA, vCenterB );
			}
		}
		else
		{
			Init( vCenterA, flRadius );
		}
	}
	
	void Init( const Vector& vMins, const Vector& vMaxs, const Vector* pVertices, int nNumVertices )
	{
		m_Mesh.m_vMins = vMins;
		m_Mesh.m_vMaxs = vMaxs;
		m_Mesh.m_pVertices = pVertices;
		m_Mesh.m_nNumVertices = nNumVertices;
		m_eType = RAY_TYPE_MESH;
	}
	
	struct Line_t
	{
		Vector m_vStartOffset;
		float m_flRadius;
	};
	
	struct Sphere_t
	{
		Vector m_vCenter;
		float m_flRadius;
	};
	
	struct Hull_t
	{
		Vector m_vMins;
		Vector m_vMaxs;
	};
	
	struct Capsule_t
	{
		Vector m_vCenter[2];
		float m_flRadius;
	};
	
	struct Mesh_t
	{
		Vector m_vMins;
		Vector m_vMaxs;
		const Vector* m_pVertices;
		int m_nNumVertices;
	};
	
	union
	{
		Line_t 		m_Line;
		Sphere_t 	m_Sphere;
		Hull_t 		m_Hull;
		Capsule_t 	m_Capsule;
		Mesh_t 		m_Mesh;
	};
	
	RayType_t m_eType;
};


#endif // RAY_H
