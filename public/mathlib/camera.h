//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef CAMERA_H
#define CAMERA_H

#ifdef _WIN32
#pragma once
#endif

#include <math.h>
#include <float.h>

// For vec_t, put this somewhere else?
#include "tier0/basetypes.h"
#include "mathlib/vector.h"

#include "tier0/dbg.h"
#include "mathlib/vector2d.h"
#include "mathlib/math_pfns.h"
#include "mathlib/vmatrix.h"
#include "mathlib/ssemath.h"
#include "datamap.h"

#include "tier0/memalloc.h"
// declarations for camera and frustum

struct Camera_t
{
	Vector m_origin;
	QAngle m_angles;
	float m_flFOVX;		// FOV for X/width
	float m_flZNear;
	float m_flZFar;
};


//-----------------------------------------------------------------------------
// accessors for generated matrices
//-----------------------------------------------------------------------------
void ComputeViewMatrix( VMatrix *pWorldToView, const Camera_t& camera );
void ComputeViewMatrix( matrix3x4_t *pWorldToView, const Camera_t& camera );
void ComputeViewMatrix( matrix3x4_t *pWorldToView, matrix3x4_t *pWorldToCamera, const Camera_t &camera );
void ComputeProjectionMatrix( VMatrix *pCameraToProjection, const Camera_t& camera, int width, int height );
void ComputeProjectionMatrix( VMatrix *pCameraToProjection, float flZNear, float flZFar, float flFOVX, float flAspectRatio );


//-----------------------------------------------------------------------------
// Computes the screen space position given a screen size
//-----------------------------------------------------------------------------
void ComputeScreenSpacePosition( Vector2D *pScreenPosition, const Vector &vecWorldPosition, 
	const Camera_t &camera, int width, int height );


//--------------------------------------------------------------------------------------
// AABB
//--------------------------------------------------------------------------------------
struct AABB_t
{
	DECLARE_BYTESWAP_DATADESC();

	Vector				m_vMinBounds;
	Vector				m_vMaxBounds;

	Vector GetCenter() const { return ( m_vMaxBounds + m_vMinBounds ) / 2.0f; }

	float GetMinDistToPoint( const Vector &vPoint ) const
	{
		return CalcDistanceToAABB( m_vMinBounds, m_vMaxBounds, vPoint );
	}

	void CreatePlanesFrom( Vector4D *pPlanes ) const
	{
		// X
		pPlanes[0] = Vector4D( 1, 0, 0, -m_vMaxBounds.x  );
		pPlanes[1] = Vector4D( -1, 0, 0, m_vMinBounds.x );

		// Y
		pPlanes[2] = Vector4D( 0, 1, 0, -m_vMaxBounds.y );
		pPlanes[3] = Vector4D( 0, -1, 0, m_vMinBounds.y );

		// Z
		pPlanes[4] = Vector4D( 0, 0, 1, -m_vMaxBounds.z );
		pPlanes[5] = Vector4D( 0, 0, -1, m_vMinBounds.z );
	}

	// set the aabb to be invalid (max < min )
	void MakeInvalid( void )
	{
		m_vMinBounds.Init( FLT_MAX, FLT_MAX, FLT_MAX );
		m_vMaxBounds.Init( -FLT_MAX, -FLT_MAX, -FLT_MAX );
	}

};

class CFrustum
{
public:
	CFrustum()
	{
	}

	~CFrustum()
	{
	}
	
	bool BoundingVolumeIntersectsFrustum( AABB_t const &box ) const
	{
		Vector vMins = box.m_vMinBounds - m_camera.m_origin;
		Vector vMaxs = box.m_vMaxBounds - m_camera.m_origin;
		return !m_frustum.CullBox( vMins, vMaxs );
	}

	bool BoundingVolumeIntersectsFrustum( Vector const &mins, Vector const &maxes ) const
	{
		Vector vMins = mins - m_camera.m_origin;
		Vector vMaxs = maxes - m_camera.m_origin;
		return !m_frustum.CullBox( vMins, vMaxs );
	}

	bool BoundingVolumeIntersectsFrustum( AABB_t const &box, Vector &vOriginShift ) const
	{
		Vector vMins = box.m_vMinBounds - m_camera.m_origin - vOriginShift;
		Vector vMaxs = box.m_vMaxBounds - m_camera.m_origin - vOriginShift;
		return !m_frustum.CullBox( vMins, vMaxs );
	}

	FORCEINLINE bool CheckBoxInline( const VectorAligned &mins, const VectorAligned &maxs )
	{
		fltx4 fl4Origin = LoadAlignedSIMD( &m_camera.m_origin );
		fltx4 mins4 = SubSIMD( LoadAlignedSIMD( &mins.x ), fl4Origin );
		fltx4 maxs4 = SubSIMD( LoadAlignedSIMD( &maxs.x ), fl4Origin );
	
		fltx4 minx = SplatXSIMD(mins4);
		fltx4 miny = SplatYSIMD(mins4);
		fltx4 minz = SplatZSIMD(mins4);
		fltx4 maxx = SplatXSIMD(maxs4);
		fltx4 maxy = SplatYSIMD(maxs4);
		fltx4 maxz = SplatZSIMD(maxs4);

		// compute the dot product of the normal and the farthest corner
		// dotBack0 = DotProduct( normal, normals.x < 0 ? mins.x : maxs.x );
		for ( int i = 0; i < 2; i++ )
		{
			fltx4 xTotalBack = MulSIMD( m_frustum.planes[i].nX, MaskedAssign( m_frustum.planes[i].xSign, minx, maxx ) );
			fltx4 yTotalBack = MulSIMD( m_frustum.planes[i].nY, MaskedAssign( m_frustum.planes[i].ySign, miny, maxy ) );
			fltx4 zTotalBack = MulSIMD( m_frustum.planes[i].nZ, MaskedAssign( m_frustum.planes[i].zSign, minz, maxz ) );
			fltx4 dotBack = AddSIMD( xTotalBack, AddSIMD(yTotalBack, zTotalBack) );
			// if plane of the farthest corner is behind the plane, then the box is completely outside this plane
#if _X360
			if  ( !XMVector3GreaterOrEqual( dotBack, m_frustum.planes[i].dist ) )
				return false;
#else
			fltx4 isOut = CmpLtSIMD( dotBack, m_frustum.planes[i].dist );
			if ( IsAnyNegative(isOut) )
				return false;
#endif
		}
		return true;
	}

	void GetNearAndFarPlanesAroundBox( float *pNear, float *pFar, AABB_t const &inBox, Vector &vOriginShift ) const
	{
		AABB_t box = inBox;
		box.m_vMinBounds -= vOriginShift;
		box.m_vMaxBounds -= vOriginShift;

		Vector vCorners[8];
		vCorners[0] = box.m_vMinBounds;
		vCorners[1] = Vector( box.m_vMinBounds.x, box.m_vMinBounds.y, box.m_vMaxBounds.z );
		vCorners[2] = Vector( box.m_vMinBounds.x, box.m_vMaxBounds.y, box.m_vMinBounds.z );
		vCorners[3] = Vector( box.m_vMinBounds.x, box.m_vMaxBounds.y, box.m_vMaxBounds.z );

		vCorners[4] = Vector( box.m_vMaxBounds.x, box.m_vMinBounds.y, box.m_vMinBounds.z );
		vCorners[5] = Vector( box.m_vMaxBounds.x, box.m_vMinBounds.y, box.m_vMaxBounds.z );
		vCorners[6] = Vector( box.m_vMaxBounds.x, box.m_vMaxBounds.y, box.m_vMinBounds.z );
		vCorners[7] = box.m_vMaxBounds;

		float flNear = FLT_MAX;//m_camera.m_flZNear;
		float flFar = -FLT_MAX;//m_camera.m_flZFar;
		for ( int i=0; i<8; ++i )
		{
			Vector vDelta = vCorners[i] - m_camera.m_origin;
			float flDist = DotProduct( m_forward, vDelta );
			flNear = MIN( flNear, flDist );
			flFar = MAX( flFar, flDist );
		}

		*pNear = flNear;
		*pFar = flFar;
	}

	void UpdateFrustumFromCamera()
	{
		ComputeViewMatrix( &m_worldToView, &m_cameraToWorld, m_camera );
		ComputeProjectionMatrix( &m_projection, m_camera.m_flZNear, m_camera.m_flZFar, m_camera.m_flFOVX, m_flAspect );

		m_viewProj = (m_projection * VMatrix(m_worldToView)).Transpose();
		MatrixGetColumn( m_cameraToWorld, 0, m_forward );
		MatrixGetColumn( m_cameraToWorld, 1, m_left );
		MatrixGetColumn( m_cameraToWorld, 2, m_up );

		// NOTE: Don't pass camera origin here.  Compute it locally so that voxels can be translated here
		// and the plane constants retain precision over large world coordinate spaces
		// Also the voxel renderer needs the camera information anyway to compute LOD so this is
		// convenient
		GeneratePerspectiveFrustum( vec3_origin, m_camera.m_angles, m_camera.m_flZNear, m_camera.m_flZFar, m_camera.m_flFOVX, m_flAspect, m_frustum );
	}



	// Just cram this here since until I figure out how to convert from direction to angles
	static VMatrix ViewMatrixRH( Vector &vEye, Vector &vAt, Vector &vUp )
	{
		Vector xAxis, yAxis;
		Vector zAxis = vEye - vAt;
		xAxis = CrossProduct( vUp, zAxis );
		yAxis = CrossProduct( zAxis, xAxis );
		xAxis.NormalizeInPlace();
		yAxis.NormalizeInPlace();
		zAxis.NormalizeInPlace();
		float flDotX = -DotProduct( xAxis, vEye );
		float flDotY = -DotProduct( yAxis, vEye );
		float flDotZ = -DotProduct( zAxis, vEye );
		VMatrix mRet( 
			xAxis.x, yAxis.x, zAxis.x, 0,
			xAxis.y, yAxis.y, zAxis.y, 0,
			xAxis.z, yAxis.z, zAxis.z, 0,
			flDotX, flDotY, flDotZ, 1 );
		return mRet.Transpose();
	}

	VMatrix OrthoMatrixRH( float flWidth, float flHeight, float flNear, float flFar )
	{
		float flDelta = flNear - flFar;

		VMatrix mRet( 
			2.0f / flWidth, 0, 0, 0,
			0, 2.0f / flHeight, 0, 0,
			0, 0, 1.0f / flDelta, 0,
			0, 0, flNear / flDelta, 1 );
		return mRet.Transpose();
	}


	const Vector &Forward() const { return m_forward; }
	const Vector &Left() const { return m_left; }
	const Vector &Up() const { return m_up; }

	void SetView( VMatrix &mView ) { m_worldToView = mView.As3x4(); }
	void SetProj( VMatrix &mProj ) { m_projection = mProj; }
	void CalcViewProj( ) { m_viewProj = (m_projection * VMatrix(m_worldToView)).Transpose(); }
	void SetFrustum( const Frustum_t &frustum ) { m_frustum = frustum; }
	void SetForward( Vector &forward ) { m_forward = forward; }
	void SetNearFarPlanes( float flNear, float flFar ) { m_camera.m_flZNear = flNear; m_camera.m_flZFar = flFar; }
	void SetCameraPosition( const Vector &origin ) { m_camera.m_origin = origin; }
	void SetCameraAngles( const QAngle &angles ) { m_camera.m_angles = angles; }
	void SetAspect( float flAspect ) { m_flAspect = flAspect; }
	void SetFOV( float flFOV ) { m_camera.m_flFOVX = flFOV; }

	const Vector &GetCameraPosition() const { return m_camera.m_origin; }
	const QAngle &GetCameraAngles() const { return m_camera.m_angles; }
	const matrix3x4_t &GetView() const { return m_worldToView; }
	const VMatrix &GetProj() const { return m_projection; }
	const VMatrix &GetViewProj() const { return m_viewProj; }
	float GetAspect() const { return m_flAspect; }
	float GetNearPlane() const { return m_camera.m_flZNear; }
	float GetFarPlane() const { return m_camera.m_flZFar; }
	float GetFOV() const { return m_camera.m_flFOVX; }

	void InitCamera( const Vector &origin, const QAngle &angles, float flNear, float flFar, float flFOV, float flAspect )
	{
		m_camera.m_origin = origin;
		m_camera.m_angles = angles;
		m_camera.m_flZNear = flNear;
		m_camera.m_flZFar = flFar;
		m_camera.m_flFOVX = flFOV;
		m_flAspect = flAspect;
	}

protected:
	ALIGN16 Frustum_t	m_frustum;
	ALIGN16 Camera_t	m_camera;
	float		m_flAspect;
	Vector		m_forward;
	Vector		m_left;
	Vector		m_up;

	matrix3x4_t	m_cameraToWorld;
	matrix3x4_t	m_worldToView;
	VMatrix	m_viewToWorld;
	VMatrix m_projection;
	VMatrix m_viewProj;
};

#endif // CAMERA_H

