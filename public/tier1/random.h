//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Random number generator
//
// $Workfile: $
// $NoKeywords: $
//===========================================================================//

#ifndef VSTDLIB_RANDOM_H
#define VSTDLIB_RANDOM_H

#include "platform.h"
#include "tier0/basetypes.h"
#include "tier0/threadtools.h"

#define NTAB 32

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning( disable:4251 )
#endif

//-----------------------------------------------------------------------------
// A generator of uniformly distributed random numbers
//-----------------------------------------------------------------------------
class IUniformRandomStream
{
public:
	// Sets the seed of the random number generator
	virtual void	SetSeed( int iSeed ) = 0;

	// Generates random numbers
	virtual float	RandomFloat( float flMinVal = 0.0f, float flMaxVal = 1.0f ) = 0;
	virtual int		RandomInt( int iMinVal, int iMaxVal ) = 0;
	virtual float	RandomFloatExp( float flMinVal = 0.0f, float flMaxVal = 1.0f, float flExponent = 1.0f ) = 0;
};


//-----------------------------------------------------------------------------
// The standard generator of uniformly distributed random numbers
//-----------------------------------------------------------------------------
class DLL_CLASS_IMPORT CUniformRandomStream : public IUniformRandomStream
{
public:
	CUniformRandomStream();

	// Sets the seed of the random number generator
	virtual void	SetSeed( int iSeed );

	// Generates random numbers
	virtual float	RandomFloat( float flMinVal = 0.0f, float flMaxVal = 1.0f );
	virtual int		RandomInt( int iMinVal, int iMaxVal );
	virtual float	RandomFloatExp( float flMinVal = 0.0f, float flMaxVal = 1.0f, float flExponent = 1.0f );

private:
	int		GenerateRandomNumber();

	int m_idum;
	int m_iy;
	int m_iv[NTAB];

	CThreadFastMutex m_mutex;
};


//-----------------------------------------------------------------------------
// A generator of gaussian distributed random numbers
//-----------------------------------------------------------------------------
class DLL_CLASS_IMPORT CGaussianRandomStream
{
public:
	// Passing in NULL will cause the gaussian stream to use the
	// installed global random number generator
	CGaussianRandomStream( IUniformRandomStream *pUniformStream = NULL );

	// Attaches to a random uniform stream
	void	AttachToStream( IUniformRandomStream *pUniformStream = NULL );

	// Generates random numbers
	float	RandomFloat( float flMean = 0.0f, float flStdDev = 1.0f );

private:
	IUniformRandomStream *m_pUniformStream;
	bool	m_bHaveValue;
	float	m_flRandomValue;

	CThreadFastMutex m_mutex;
};


//-----------------------------------------------------------------------------
// A couple of convenience functions to access the library's global uniform stream
//-----------------------------------------------------------------------------
DLL_IMPORT void	RandomSeed( int iSeed );
DLL_IMPORT float	RandomFloat( float flMinVal = 0.0f, float flMaxVal = 1.0f );
DLL_IMPORT float	RandomFloatExp( float flMinVal = 0.0f, float flMaxVal = 1.0f, float flExponent = 1.0f );
DLL_IMPORT int	RandomInt( int iMinVal, int iMaxVal );
DLL_IMPORT float	RandomGaussianFloat( float flMean = 0.0f, float flStdDev = 1.0f );


//-----------------------------------------------------------------------------
// Installs a global random number generator, which will affect the Random functions above
//-----------------------------------------------------------------------------
DLL_IMPORT void	InstallUniformRandomStream( IUniformRandomStream *pStream );

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif // VSTDLIB_RANDOM_H



