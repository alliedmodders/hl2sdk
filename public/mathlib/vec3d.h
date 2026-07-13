#ifndef VEC3D_H
#define VEC3D_H

#ifdef _WIN32
#pragma once
#endif

#include <math.h>

#include "tier0/dbg.h"
#include "tier0/platform.h"

//=========================================================
// Templated 3D vector.
//=========================================================
template <typename T>
class Vec3D
{
public:
	// Members
	T x, y, z;

	// Construction. Default leaves the components uninitialized.
	Vec3D() {}
	Vec3D( T ix, T iy, T iz ) : x( ix ), y( iy ), z( iz ) {}

	// Initialization
	void Init( T ix = 0, T iy = 0, T iz = 0 ) { x = ix; y = iy; z = iz; }
	void Zero() { x = y = z = 0; }

	// Base address
	T* Base() { return &x; }
	const T* Base() const { return &x; }

	// array access
	T operator[]( int i ) const { Assert( i >= 0 && i < 3 ); return Base()[i]; }
	T& operator[]( int i ) { Assert( i >= 0 && i < 3 ); return Base()[i]; }

	// equality
	bool operator==( const Vec3D& v ) const { return v.x == x && v.y == y && v.z == z; }
	bool operator!=( const Vec3D& v ) const { return v.x != x || v.y != y || v.z != z; }

	// arithmetic
	Vec3D& operator+=( const Vec3D& v ) { x += v.x; y += v.y; z += v.z; return *this; }
	Vec3D& operator-=( const Vec3D& v ) { x -= v.x; y -= v.y; z -= v.z; return *this; }
	Vec3D& operator*=( T s ) { x *= s; y *= s; z *= s; return *this; }
	Vec3D& operator/=( T s ) { x /= s; y /= s; z /= s; return *this; }

	Vec3D operator-() const { return Vec3D( -x, -y, -z ); }
	Vec3D operator+( const Vec3D& v ) const { return Vec3D( x + v.x, y + v.y, z + v.z ); }
	Vec3D operator-( const Vec3D& v ) const { return Vec3D( x - v.x, y - v.y, z - v.z ); }
	Vec3D operator*( T s ) const { return Vec3D( x * s, y * s, z * s ); }
	Vec3D operator/( T s ) const { return Vec3D( x / s, y / s, z / s ); }

	void Negate() { x = -x; y = -y; z = -z; }

	// products / magnitude
	T Dot( const Vec3D& v ) const { return x * v.x + y * v.y + z * v.z; }
	T LengthSqr() const { return x * x + y * y + z * z; }
	T Length() const { return (T)sqrt( (double)LengthSqr() ); }
};

#endif // VEC3D_H
