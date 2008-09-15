//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef INTERPOLATEDVAR_H
#define INTERPOLATEDVAR_H
#ifdef _WIN32
#pragma once
#endif

#include "tier1/utllinkedlist.h"
#include "rangecheckedvar.h"
#include "lerp_functions.h"
#include "animationlayer.h"
#include "convar.h"


#include "tier0/memdbgon.h"

#define COMPARE_HISTORY(a,b) \
	( memcmp( m_VarHistory[a].value, m_VarHistory[b].value, sizeof(Type)*m_nMaxCount ) == 0 ) 			

// Define this to have it measure whether or not the interpolated entity list
// is accurate.
//#define INTERPOLATEDVAR_PARANOID_MEASUREMENT


#define LATCH_ANIMATION_VAR  (1<<0)		// use AnimTime as sample basis
#define LATCH_SIMULATION_VAR (1<<1)		// use SimulationTime as sample basis

#define EXCLUDE_AUTO_LATCH			(1<<2)
#define EXCLUDE_AUTO_INTERPOLATE	(1<<3)

#define INTERPOLATE_LINEAR_ONLY		(1<<4)	// don't do hermite interpolation



#define EXTRA_INTERPOLATION_HISTORY_STORED 0.05f	// It stores this much extra interpolation history,
													// so you can always call Interpolate() this far
													// in the past from your last call and be able to 
													// get an interpolated value.

// this global keeps the last known server packet tick (to avoid calling engine->GetLastTimestamp() all the time)
extern float g_flLastPacketTimestamp;

inline void Interpolation_SetLastPacketTimeStamp( float timestamp)
{
	Assert( timestamp > 0 );
	g_flLastPacketTimestamp = timestamp;
}


// Before calling Interpolate(), you can use this use this to setup the context if 
// you want to enable extrapolation.
class CInterpolationContext
{
public:
	
	CInterpolationContext()
	{
		m_bOldAllowExtrapolation = s_bAllowExtrapolation;
		m_flOldLastTimeStamp = s_flLastTimeStamp;

		// By default, disable extrapolation unless they call EnableExtrapolation.
		s_bAllowExtrapolation = false;

		// this is the context stack
		m_pNext = s_pHead;
		s_pHead = this;
	}
	
	~CInterpolationContext()
	{
		// restore values from prev stack element
		s_bAllowExtrapolation = m_bOldAllowExtrapolation;
		s_flLastTimeStamp = m_flOldLastTimeStamp;

		Assert( s_pHead == this );
		s_pHead = m_pNext;
	}

	static void EnableExtrapolation(bool state)
	{
		s_bAllowExtrapolation = state;
	}

	static bool IsThereAContext()
	{
		return s_pHead != NULL;
	}

	static bool IsExtrapolationAllowed()
	{
		return s_bAllowExtrapolation;
	}

	static void SetLastTimeStamp(float timestamp)
	{
		s_flLastTimeStamp = timestamp;
	}
	
	static float GetLastTimeStamp()
	{
		return s_flLastTimeStamp;
	}


private:

	CInterpolationContext *m_pNext;
	bool m_bOldAllowExtrapolation;
	float m_flOldLastTimeStamp;

	static CInterpolationContext *s_pHead;
	static bool s_bAllowExtrapolation;
	static float s_flLastTimeStamp;
};


extern ConVar cl_extrapolate_amount;


template< class T >
inline T ExtrapolateInterpolatedVarType( const T &oldVal, const T &newVal, float divisor, float flExtrapolationAmount )
{
	return newVal;
}

inline Vector ExtrapolateInterpolatedVarType( const Vector &oldVal, const Vector &newVal, float divisor, float flExtrapolationAmount )
{
	return Lerp( 1.0f + flExtrapolationAmount * divisor, oldVal, newVal );
}

inline float ExtrapolateInterpolatedVarType( const float &oldVal, const float &newVal, float divisor, float flExtrapolationAmount )
{
	return Lerp( 1.0f + flExtrapolationAmount * divisor, oldVal, newVal );
}

inline QAngle ExtrapolateInterpolatedVarType( const QAngle &oldVal, const QAngle &newVal, float divisor, float flExtrapolationAmount )
{
	return Lerp<QAngle>( 1.0f + flExtrapolationAmount * divisor, oldVal, newVal );
}


// -------------------------------------------------------------------------------------------------------------- //
// IInterpolatedVar interface.
// -------------------------------------------------------------------------------------------------------------- //

abstract_class IInterpolatedVar
{
public:
	virtual		 ~IInterpolatedVar() {}

	virtual void Setup( void *pValue, int type ) = 0;
	virtual void SetInterpolationAmount( float seconds ) = 0;
	
	// Returns true if the new value is different from the prior most recent value.
	virtual bool NoteChanged( float changetime ) = 0;
	virtual void Reset() = 0;
	
	// Returns 1 if the value will always be the same if currentTime is always increasing.
	virtual int Interpolate( float currentTime ) = 0;
	
	virtual int	 GetType() const = 0;
	virtual void RestoreToLastNetworked() = 0;
	virtual void Copy( IInterpolatedVar *pSrc ) = 0;

	virtual const char *GetDebugName() = 0;
	virtual void SetDebugName( const char* pName )	= 0;
};


// -------------------------------------------------------------------------------------------------------------- //
// CInterpolatedVarArrayBase - the main implementation of IInterpolatedVar.
// -------------------------------------------------------------------------------------------------------------- //

template< typename Type > 
class CInterpolatedVarArrayBase : public IInterpolatedVar
{
public:
	friend class CInterpolatedVarPrivate;

	CInterpolatedVarArrayBase( const char *pDebugName="no debug name" );
	virtual ~CInterpolatedVarArrayBase();

	
// IInterpolatedVar overrides.
public:
	
	virtual void Setup( void *pValue, int type );
	virtual void SetInterpolationAmount( float seconds );
	virtual bool NoteChanged( float changetime );
	virtual void Reset();
	virtual int Interpolate( float currentTime );
	virtual int GetType() const;
	virtual void RestoreToLastNetworked();
	virtual void Copy( IInterpolatedVar *pInSrc );
	virtual const char *GetDebugName() { return m_pDebugName; }


public:

	// Just like the IInterpolatedVar functions, but you can specify an interpolation amount.
	bool NoteChanged( float changetime, float interpolation_amount );
	int Interpolate( float currentTime, float interpolation_amount );

	void GetDerivative( Type *pOut, float currentTime );
	void GetDerivative_SmoothVelocity( Type *pOut, float currentTime );	// See notes on ::Derivative_HermiteLinearVelocity for info.

	void ClearHistory();
	void AddToHead( float changeTime, const Type* values, bool bFlushNewer );
	const Type&	GetPrev( int iArrayIndex=0 ) const;
	const Type&	GetCurrent( int iArrayIndex=0 ) const;
	
	// Returns the time difference betweem the most recent sample and its previous sample.
	float	GetInterval() const;
	bool	IsValidIndex( int i );
	Type	*GetHistoryValue( int index, float& changetime, int iArrayIndex=0 );
	int		GetHead();
	int		GetNext( int i );
	void SetHistoryValuesForItem( int item, Type& value );
	void	SetLooping( bool looping, int iArrayIndex=0 );
	
	void SetMaxCount( int newmax );
	int GetMaxCount() const;

	// Get the time of the oldest entry.
	float GetOldestEntry();

	// set a debug name (if not provided by constructor)
	void	SetDebugName(const char *pName ) { m_pDebugName = pName; }

	bool GetInterpolationInfo( float currentTime, int *pNewer, int *pOlder, int *pOldest );

protected:

	struct CInterpolatedVarEntry
	{
		CInterpolatedVarEntry()
		{
			value = NULL;
		}

		float		changetime;
		Type *		value;
	};

	typedef CUtlPtrLinkedList< CInterpolatedVarEntry > CVarHistory;
	friend class CInterpolationInfo;

	class CInterpolationInfo
	{
	public:
		bool m_bHermite;
		typename CInterpolatedVarArrayBase::CVarHistory::IndexType_t oldest;	// Only set if using hermite.
		typename CInterpolatedVarArrayBase::CVarHistory::IndexType_t older;
		typename CInterpolatedVarArrayBase::CVarHistory::IndexType_t newer;
		float frac;
	};


protected:

	void RemoveOldEntries( float oldesttime );
	void RemoveEntriesPreviousTo( float flTime );

	bool GetInterpolationInfo( 
		CInterpolationInfo *pInfo,
		float currentTime, 
		float interpolation_amount,
		int *pNoMoreChanges );

	void TimeFixup_Hermite( 
		CInterpolatedVarEntry &fixup,
		CInterpolatedVarEntry*& prev, 
		CInterpolatedVarEntry*& start, 
		CInterpolatedVarEntry*& end	);

	// Force the time between prev and start to be dt (and extend prev out farther if necessary).
	void TimeFixup2_Hermite( 
		CInterpolatedVarEntry &fixup,
		CInterpolatedVarEntry*& prev, 
		CInterpolatedVarEntry*& start, 
		float dt
		);

	void _Extrapolate( 
		Type *pOut,
		CInterpolatedVarEntry *pOld,
		CInterpolatedVarEntry *pNew,
		float flDestinationTime,
		float flMaxExtrapolationAmount
		);

	void _Interpolate( Type *out, float frac, CInterpolatedVarEntry *start, CInterpolatedVarEntry *end );
	void _Interpolate_Hermite( Type *out, float frac, CInterpolatedVarEntry *pOriginalPrev, CInterpolatedVarEntry *start, CInterpolatedVarEntry *end, bool looping = false );
	
	void _Derivative_Hermite( Type *out, float frac, CInterpolatedVarEntry *pOriginalPrev, CInterpolatedVarEntry *start, CInterpolatedVarEntry *end );
	void _Derivative_Hermite_SmoothVelocity( Type *out, float frac, CInterpolatedVarEntry *b, CInterpolatedVarEntry *c, CInterpolatedVarEntry *d );
	void _Derivative_Linear( Type *out, CInterpolatedVarEntry *start, CInterpolatedVarEntry *end );
	
	bool ValidOrder();

	// Get the next element in VarHistory, or just return i if it's an invalid index.
	int SafeNext( int i );


protected:
	// The underlying data element
	Type								*m_pValue;
	CVarHistory							m_VarHistory;
	// Store networked values so when we latch we can detect which values were changed via networking
	Type *								m_LastNetworkedValue;
	float								m_LastNetworkedTime;
	byte								m_fType;
	byte								m_nMaxCount;
	byte *								m_bLooping;
	float								m_InterpolationAmount;
	const char *						m_pDebugName;
};


template< typename Type > 
inline CInterpolatedVarArrayBase<Type>::CInterpolatedVarArrayBase( const char *pDebugName )
{
	COMPILE_TIME_ASSERT( sizeof(CVarHistory::IndexType_t) == sizeof(int) );

	m_pDebugName = pDebugName;
	m_pValue = NULL;
	m_fType = LATCH_ANIMATION_VAR;
	m_InterpolationAmount = 0.0f;
	m_nMaxCount = 0;
	m_LastNetworkedTime = 0;
	m_LastNetworkedValue = NULL;
	m_bLooping = NULL;
}

template< typename Type > 
inline CInterpolatedVarArrayBase<Type>::~CInterpolatedVarArrayBase()
{
	ClearHistory();
	delete [] m_bLooping;
	delete [] m_LastNetworkedValue;
}

template< typename Type > 
inline void CInterpolatedVarArrayBase<Type>::Setup( void *pValue, int type )
{
	m_pValue = ( Type * )pValue;
	m_fType = type;
}

template< typename Type > 
inline void CInterpolatedVarArrayBase<Type>::SetInterpolationAmount( float seconds )
{
	m_InterpolationAmount = seconds;
}

template< typename Type > 
inline int CInterpolatedVarArrayBase<Type>::GetType() const
{
	return m_fType;
}


template< typename Type > 
inline bool CInterpolatedVarArrayBase<Type>::NoteChanged( float changetime, float interpolation_amount )
{
	Assert( m_pValue );

	// This is a big optimization where it can potentially avoid expensive interpolation
	// involving this variable if it didn't get an actual new value in here.
	bool bRet = true;
	CVarHistory::IndexType_t iHead = m_VarHistory.Head();

	if ( iHead != CVarHistory::InvalidIndex() && 
		 memcmp( m_pValue, m_VarHistory[iHead].value, sizeof( Type ) * m_nMaxCount ) == 0 )
	{
		bRet = false;
	}

	AddToHead( changetime, m_pValue, true );

	memcpy( m_LastNetworkedValue, m_pValue, m_nMaxCount * sizeof( Type ) );
	m_LastNetworkedTime = g_flLastPacketTimestamp;
	
	// Since we don't clean out the old entries until Interpolate(), make sure that there
	// aren't any super old entries hanging around.
	RemoveOldEntries( gpGlobals->curtime - interpolation_amount - 2.0f );
	
	return bRet;
}


template< typename Type > 
inline bool CInterpolatedVarArrayBase<Type>::NoteChanged( float changetime )
{
	return NoteChanged( changetime, m_InterpolationAmount );
}


template< typename Type > 
inline void CInterpolatedVarArrayBase<Type>::RestoreToLastNetworked()
{
	Assert( m_pValue );
	memcpy( m_pValue, m_LastNetworkedValue, m_nMaxCount * sizeof( Type ) );
}

template< typename Type > 
inline void CInterpolatedVarArrayBase<Type>::ClearHistory()
{
	CVarHistory::IndexType_t i = m_VarHistory.Head();
	while ( i != CVarHistory::InvalidIndex() )
	{
		delete [] m_VarHistory[i].value;
		i = m_VarHistory.Next( i );
	}
	m_VarHistory.RemoveAll();
}

template< typename Type > 
inline void CInterpolatedVarArrayBase<Type>::AddToHead( float changeTime, const Type* values, bool bFlushNewer )
{
	MEM_ALLOC_CREDIT_CLASS();
	CVarHistory::IndexType_t newslot;
	
	if ( bFlushNewer )
	{
		// Get rid of anything that has a timestamp after this sample. The server might have
		// corrected our clock and moved us back, so our current changeTime is less than a 
		// changeTime we added samples during previously.
		CVarHistory::IndexType_t insertSpot = m_VarHistory.Head();
		while ( insertSpot != CVarHistory::InvalidIndex() )
		{
			CVarHistory::IndexType_t next = m_VarHistory.Next( insertSpot );
			CInterpolatedVarEntry *check = &m_VarHistory[ insertSpot ];
			if ( (check->changetime+0.0001f) >= changeTime )
			{
				delete [] m_VarHistory[insertSpot].value;
				m_VarHistory.Remove( insertSpot );
			}
			else
			{
				break;
			}
			insertSpot = next;
		}

		newslot = m_VarHistory.AddToHead();
	}
	else
	{
		CVarHistory::IndexType_t insertSpot = m_VarHistory.Head();
		while ( insertSpot != CVarHistory::InvalidIndex() )
		{
			CInterpolatedVarEntry *check = &m_VarHistory[ insertSpot ];
			if ( check->changetime <= changeTime )
				break;

			insertSpot = m_VarHistory.Next( insertSpot );
		}

		if ( insertSpot == CVarHistory::InvalidIndex() )
		{
			newslot = m_VarHistory.AddToTail();
		}
		else
		{
			newslot = m_VarHistory.InsertBefore( insertSpot );
		}
	}

	CInterpolatedVarEntry *e = &m_VarHistory[ newslot ];
	e->changetime	= changeTime;
	e->value = new Type[m_nMaxCount];
	memcpy( e->value, values, m_nMaxCount*sizeof(Type) );
}

template< typename Type > 
inline void CInterpolatedVarArrayBase<Type>::Reset()
{
	ClearHistory();

	if ( m_pValue )
	{
		AddToHead( gpGlobals->curtime, m_pValue, false );
		AddToHead( gpGlobals->curtime, m_pValue, false );
		AddToHead( gpGlobals->curtime, m_pValue, false );

		memcpy( m_LastNetworkedValue, m_pValue, m_nMaxCount * sizeof( Type ) );
	}
}


template< typename Type > 
inline float CInterpolatedVarArrayBase<Type>::GetOldestEntry()
{
	float lastVal = 0;
	for ( CVarHistory::IndexType_t i = m_VarHistory.Head(); i != CVarHistory::InvalidIndex(); i = m_VarHistory.Next( i ) )
	{
		lastVal = m_VarHistory[i].changetime;
	}
	return lastVal;
}


template< typename Type > 
inline void CInterpolatedVarArrayBase<Type>::RemoveOldEntries( float oldesttime )
{
	int c = 0;
	CVarHistory::IndexType_t next;
	// Always leave three of entries in the list...
	for ( CVarHistory::IndexType_t i = m_VarHistory.Head(); i != CVarHistory::InvalidIndex(); c++, i = next )
	{
		next = m_VarHistory.Next( i );

		// Always leave elements 0 1 and 2 alone...
		if ( c <= 2 )
			continue;

		CInterpolatedVarEntry *h = &m_VarHistory[ i ];
		// Remove everything off the end until we find the first one that's not too old
		if ( h->changetime > oldesttime )
			continue;

		// Unlink rest of chain
		delete [] m_VarHistory[i].value;
		m_VarHistory.Remove( i );
	}
}


template< typename Type > 
inline int CInterpolatedVarArrayBase<Type>::SafeNext( int i )
{
	if ( IsValidIndex( i ) )
		return GetNext( i );
	else
		return i;
}


template< typename Type > 
inline void CInterpolatedVarArrayBase<Type>::RemoveEntriesPreviousTo( float flTime )
{
	CVarHistory::IndexType_t i = m_VarHistory.Head();
	// Find the 2 samples spanning this time.
	for ( ; i != CVarHistory::InvalidIndex(); i=m_VarHistory.Next( i ) )
	{
		if ( m_VarHistory[i].changetime < flTime )
		{
			// We need to preserve this sample (ie: the one right before this timestamp)
			// and the sample right before it (for hermite blending), and we can get rid
			// of everything else.
			i = (CVarHistory::IndexType_t)SafeNext( (int)i );
			i = (CVarHistory::IndexType_t)SafeNext( (int)i );
			i = (CVarHistory::IndexType_t)SafeNext( (int)i );	// We keep this one for _Derivative_Hermite_SmoothVelocity.
			
			break;
		}
	}

	// Now remove all samples starting with i.
	CVarHistory::IndexType_t next;
	for ( ; i != CVarHistory::InvalidIndex(); i=next )
	{
		next = m_VarHistory.Next( i );
		delete [] m_VarHistory[i].value;
		m_VarHistory.Remove( i );
	}
}


template< typename Type > 
inline bool CInterpolatedVarArrayBase<Type>::GetInterpolationInfo( 
	typename CInterpolatedVarArrayBase<Type>::CInterpolationInfo *pInfo,
	float currentTime, 
	float interpolation_amount,
	int *pNoMoreChanges
	)
{
	Assert( m_pValue );

	CVarHistory &varHistory = m_VarHistory;

	float targettime = currentTime - interpolation_amount;
	CVarHistory::IndexType_t i;

	pInfo->m_bHermite = false;
	pInfo->frac = 0;
	pInfo->oldest = pInfo->older = pInfo->newer = varHistory.InvalidIndex();
	
	for ( i = m_VarHistory.Head(); i != varHistory.InvalidIndex(); i = varHistory.Next( i ) )
	{
		pInfo->older = i;
		
		float older_change_time = m_VarHistory[ i ].changetime;
		if ( older_change_time == 0.0f )
			break;

		if ( targettime < older_change_time )
		{
			pInfo->newer = pInfo->older;
			continue;
		}

		if ( pInfo->newer == varHistory.InvalidIndex() )
		{
			// Have it linear interpolate between the newest 2 entries.
			pInfo->newer = pInfo->older; 

			// Since the time given is PAST all of our entries, then as long
			// as time continues to increase, we'll be returning the same value.
			if ( pNoMoreChanges )
				*pNoMoreChanges = 1;
			return true;
		}

		float newer_change_time = varHistory[ pInfo->newer ].changetime;
		float dt = newer_change_time - older_change_time;
		if ( dt > 0.0001f )
		{
			pInfo->frac = ( targettime - older_change_time ) / ( newer_change_time - older_change_time );
			pInfo->frac = min( pInfo->frac, 2.0f );

			CVarHistory::IndexType_t oldestindex = varHistory.Next( i );
														    
			if ( !(m_fType & INTERPOLATE_LINEAR_ONLY) && oldestindex != varHistory.InvalidIndex() )
			{
				pInfo->oldest = oldestindex;
				float oldest_change_time = varHistory[ oldestindex ].changetime;
				float dt2 = older_change_time - oldest_change_time;
				if ( dt2 > 0.0001f )
				{
					pInfo->m_bHermite = true;
				}
			}

			// If pInfo->newer is the most recent entry we have, and all 2 or 3 other
			// entries are identical, then we're always going to return the same value
			// if currentTime increases.
			if ( pNoMoreChanges && pInfo->newer == m_VarHistory.Head() )
			{
				 if ( COMPARE_HISTORY( pInfo->newer, pInfo->older ) )
				 {
					if ( !pInfo->m_bHermite || COMPARE_HISTORY( pInfo->newer, pInfo->oldest ) )
						*pNoMoreChanges = 1;
				 }
			}
		}
		return true;
	}

	// Didn't find any, return last entry???
	if ( pInfo->newer != varHistory.InvalidIndex() )
	{
		pInfo->older = pInfo->newer;
		return true;
	}


	// This is the single-element case
	pInfo->newer = pInfo->older;
	return (pInfo->older != varHistory.InvalidIndex());
}


template< typename Type > 
inline bool CInterpolatedVarArrayBase<Type>::GetInterpolationInfo( float currentTime, int *pNewer, int *pOlder, int *pOldest )
{
	CInterpolationInfo info;
	bool result = GetInterpolationInfo( &info, currentTime, m_InterpolationAmount, NULL );

	if (pNewer)
		*pNewer = (int)info.newer;

	if (pOlder)
		*pOlder = (int)info.older;

	if (pOldest)
		*pOldest = (int)info.oldest;

	return result;
}



template< typename Type > 
inline int CInterpolatedVarArrayBase<Type>::Interpolate( float currentTime, float interpolation_amount )
{
	int noMoreChanges = 0;
	
	CInterpolationInfo info;
	if (!GetInterpolationInfo( &info, currentTime, interpolation_amount, &noMoreChanges ))
		return noMoreChanges;

	
	CVarHistory &history = m_VarHistory;

#ifdef INTERPOLATEDVAR_PARANOID_MEASUREMENT
	Type *backupValues = (Type*)_alloca( m_nMaxCount * sizeof(Type) );
	memcpy( backupValues, m_pValue, sizeof( Type ) * m_nMaxCount );
#endif

	if ( info.m_bHermite )
	{
		// base cast, we have 3 valid sample point
		_Interpolate_Hermite( m_pValue, info.frac, &history[info.oldest], &history[info.older], &history[info.newer] );
	}
	else if ( info.newer == info.older  )
	{
		// This means the server clock got way behind the client clock. Extrapolate the value here based on its
		// previous velocity (out to a certain amount).
		int realOlder = SafeNext( (int)info.newer );
		if ( CInterpolationContext::IsExtrapolationAllowed() &&
			IsValidIndex( realOlder ) &&
			history[(CVarHistory::IndexType_t)realOlder].changetime != 0.0 &&
			interpolation_amount > 0.000001f &&
			CInterpolationContext::GetLastTimeStamp() <= m_LastNetworkedTime )
		{
			// At this point, we know we're out of data and we have the ability to get a velocity to extrapolate with.
			//
			// However, we only want to extraploate if the server is choking. We don't want to extrapolate if 
			// the object legimately stopped moving and the server stopped sending updates for it.
			//
			// The way we know that the server is choking is if we haven't heard ANYTHING from it for a while.
			// The server's update interval should be at least as often as our interpolation amount (otherwise,
			// we wouldn't have the ability to interpolate).
			//
			// So right here, if we see that we haven't gotten any server updates since the last interpolation
			// history update to this entity (and since we're in here, we know that we're out of interpolation data),
			// then we can assume that the server is choking and decide to extrapolate.
			//
			// The End

			// Use the velocity here (extrapolate up to 1/4 of a second).
			_Extrapolate( m_pValue, &history[(CVarHistory::IndexType_t)realOlder], &history[info.newer], currentTime - interpolation_amount, cl_extrapolate_amount.GetFloat() );
		}
		else
		{
			_Interpolate( m_pValue, info.frac, &history[info.older], &history[info.newer] );
		}
	}
	else
	{
		_Interpolate( m_pValue, info.frac, &history[info.older], &history[info.newer] );
	}

#ifdef INTERPOLATEDVAR_PARANOID_MEASUREMENT
	if ( memcmp( backupValues, m_pValue, sizeof( Type ) * m_nMaxCount ) != 0 )
	{
		extern int g_nInterpolatedVarsChanged;
		extern bool g_bRestoreInterpolatedVarValues;
		
		++g_nInterpolatedVarsChanged;

		// This undoes the work that we do in here so if someone is in the debugger, they
		// can find out which variable changed.
		if ( g_bRestoreInterpolatedVarValues )
		{
			memcpy( m_pValue, backupValues, sizeof( Type ) * m_nMaxCount );
			return noMoreChanges;
		}
	}
#endif

	// Clear out all entries before the oldest since we should never access them again.
	// Usually, Interpolate() calls never go backwards in time, but C_BaseAnimating::BecomeRagdollOnClient for one
	// goes slightly back in time
	RemoveEntriesPreviousTo( currentTime - interpolation_amount - EXTRA_INTERPOLATION_HISTORY_STORED );
	return noMoreChanges;
}


template< typename Type > 
void CInterpolatedVarArrayBase<Type>::GetDerivative( Type *pOut, float currentTime )
{
	CInterpolationInfo info;
	if (!GetInterpolationInfo( &info, currentTime, m_InterpolationAmount, NULL ))
		return;

	if ( info.m_bHermite )
	{
		_Derivative_Hermite( pOut, info.frac, &m_VarHistory[info.oldest], &m_VarHistory[info.older], &m_VarHistory[info.newer] );
	}
	else
	{
		_Derivative_Linear( pOut, &m_VarHistory[info.older], &m_VarHistory[info.newer] );
	}
}


template< typename Type > 
void CInterpolatedVarArrayBase<Type>::GetDerivative_SmoothVelocity( Type *pOut, float currentTime )
{
	CInterpolationInfo info;
	if (!GetInterpolationInfo( &info, currentTime, m_InterpolationAmount, NULL ))
		return;

	CVarHistory &history = m_VarHistory;
	bool bExtrapolate = false;
	int realOlder = 0;
	
	if ( info.m_bHermite )
	{
		_Derivative_Hermite_SmoothVelocity( pOut, info.frac, &history[info.oldest], &history[info.older], &history[info.newer] );
		return;
	}
	else if ( info.newer == info.older && CInterpolationContext::IsExtrapolationAllowed() )
	{
		// This means the server clock got way behind the client clock. Extrapolate the value here based on its
		// previous velocity (out to a certain amount).
		realOlder = SafeNext( (int)info.newer );
		if ( IsValidIndex( realOlder ) && history[(CVarHistory::IndexType_t)realOlder].changetime != 0.0 )
		{
			// At this point, we know we're out of data and we have the ability to get a velocity to extrapolate with.
			//
			// However, we only want to extraploate if the server is choking. We don't want to extrapolate if 
			// the object legimately stopped moving and the server stopped sending updates for it.
			//
			// The way we know that the server is choking is if we haven't heard ANYTHING from it for a while.
			// The server's update interval should be at least as often as our interpolation amount (otherwise,
			// we wouldn't have the ability to interpolate).
			//
			// So right here, if we see that we haven't gotten any server updates for a whole interpolation 
			// interval, then we know the server is choking.
			//
			// The End
			if ( m_InterpolationAmount > 0.000001f &&
				 CInterpolationContext::GetLastTimeStamp() <= (currentTime - m_InterpolationAmount) )
			{
				bExtrapolate = true;
			}
		}
	}

	if ( bExtrapolate )
	{
		// Get the velocity from the last segment.
		_Derivative_Linear( pOut, &history[(CVarHistory::IndexType_t)realOlder], &history[info.newer] );

		// Now ramp it to zero after cl_extrapolate_amount..
		float flDestTime = currentTime - m_InterpolationAmount;
		float diff = flDestTime - history[info.newer].changetime;
		diff = clamp( diff, 0, cl_extrapolate_amount.GetFloat() * 2 );
		if ( diff > cl_extrapolate_amount.GetFloat() )
		{
			float scale = 1 - (diff - cl_extrapolate_amount.GetFloat()) / cl_extrapolate_amount.GetFloat();
			for ( int i=0; i < m_nMaxCount; i++ )
			{
				pOut[i] *= scale;
			}
		}
	}
	else
	{
		_Derivative_Linear( pOut, &history[info.older], &history[info.newer] );
	}

}


template< typename Type > 
inline int CInterpolatedVarArrayBase<Type>::Interpolate( float currentTime )
{
	return Interpolate( currentTime, m_InterpolationAmount );
}

template< typename Type > 
inline void CInterpolatedVarArrayBase<Type>::Copy( IInterpolatedVar *pInSrc )
{
	CInterpolatedVarArrayBase<Type> *pSrc = dynamic_cast< CInterpolatedVarArrayBase<Type>* >( pInSrc );

	if ( !pSrc || pSrc->m_nMaxCount != m_nMaxCount )
	{
		Assert( false );
		return;
	}

	Assert( (m_fType & ~EXCLUDE_AUTO_INTERPOLATE) == (pSrc->m_fType & ~EXCLUDE_AUTO_INTERPOLATE) );
	Assert( m_pDebugName == pSrc->GetDebugName() );

	for ( int i=0; i < m_nMaxCount; i++ )
	{
		m_LastNetworkedValue[i] = pSrc->m_LastNetworkedValue[i];
		m_bLooping[i] = pSrc->m_bLooping[i];
	}

	m_LastNetworkedTime = pSrc->m_LastNetworkedTime;

	// Copy the entries.
	m_VarHistory.RemoveAll();

	CVarHistory::IndexType_t newslot;
	for ( CVarHistory::IndexType_t srcCur=pSrc->m_VarHistory.Head(); srcCur != CVarHistory::InvalidIndex(); srcCur = pSrc->m_VarHistory.Next( srcCur ) )
	{
		newslot = m_VarHistory.AddToTail();

		CInterpolatedVarEntry *dest = &m_VarHistory[newslot];
		CInterpolatedVarEntry *src	= &pSrc->m_VarHistory[srcCur];
		dest->changetime = src->changetime;
		dest->value = new Type[m_nMaxCount];
		memcpy( dest->value, src->value, m_nMaxCount*sizeof(Type) );
	}
}

template< typename Type > 
inline const Type& CInterpolatedVarArrayBase<Type>::GetPrev( int iArrayIndex ) const
{
	Assert( m_pValue );
	Assert( iArrayIndex >= 0 && iArrayIndex < m_nMaxCount );

	CVarHistory::IndexType_t ihead = m_VarHistory.Head();
	if ( ihead != CVarHistory::InvalidIndex() )
	{
		ihead = m_VarHistory.Next( ihead );
		if ( ihead != CVarHistory::InvalidIndex() )
		{
			CInterpolatedVarEntry const *h = &m_VarHistory[ ihead ];
			return h->value[ iArrayIndex ];
		}
	}
	return m_pValue[ iArrayIndex ];
}

template< typename Type > 
inline const Type& CInterpolatedVarArrayBase<Type>::GetCurrent( int iArrayIndex ) const
{
	Assert( m_pValue );
	Assert( iArrayIndex >= 0 && iArrayIndex < m_nMaxCount );

	CVarHistory::IndexType_t ihead = m_VarHistory.Head();
	if ( ihead != CVarHistory::InvalidIndex() )
	{
		CInterpolatedVarEntry const *h = &m_VarHistory[ ihead ];
		return h->value[ iArrayIndex ];
	}
	return m_pValue[ iArrayIndex ];
}

template< typename Type > 
inline float CInterpolatedVarArrayBase<Type>::GetInterval() const
{	
	CVarHistory::IndexType_t head = m_VarHistory.Head();
	if ( head != CVarHistory::InvalidIndex() )
	{
		int next = m_VarHistory.Next( head );
		if ( next != CVarHistory::InvalidIndex() )
		{
			CInterpolatedVarEntry const *h = &m_VarHistory[ head ];
			CInterpolatedVarEntry const *n = &m_VarHistory[ next ];
			
			return ( h->changetime - n->changetime );
		}
	}

	return 0.0f;
}

template< typename Type > 
inline bool	CInterpolatedVarArrayBase<Type>::IsValidIndex( int i )
{
	return m_VarHistory.IsValidIndex( (CVarHistory::IndexType_t)i );
}

template< typename Type > 
inline Type	*CInterpolatedVarArrayBase<Type>::GetHistoryValue( int index, float& changetime, int iArrayIndex )
{
	Assert( iArrayIndex >= 0 && iArrayIndex < m_nMaxCount );
	if ( (CVarHistory::IndexType_t)index == CVarHistory::InvalidIndex() )
	{
		changetime = 0.0f;
		return NULL;
	}

	CInterpolatedVarEntry *entry = &m_VarHistory[ (CVarHistory::IndexType_t)index ];
	changetime = entry->changetime;
	return &entry->value[ iArrayIndex ];
}

template< typename Type > 
inline int CInterpolatedVarArrayBase<Type>::GetHead()
{
	return (int)m_VarHistory.Head();
}

template< typename Type > 
inline int CInterpolatedVarArrayBase<Type>::GetNext( int i )
{
	return (int)m_VarHistory.Next( (CVarHistory::IndexType_t)i );
}

template< typename Type > 
inline void CInterpolatedVarArrayBase<Type>::SetHistoryValuesForItem( int item, Type& value )
{
	Assert( item >= 0 && item < m_nMaxCount );

	CVarHistory::IndexType_t i;
	for ( i = m_VarHistory.Head(); i != CVarHistory::InvalidIndex(); i = m_VarHistory.Next( i ) )
	{
		CInterpolatedVarEntry *entry = &m_VarHistory[ i ];
		entry->value[ item ] = value;
	}
}

template< typename Type > 
inline void	CInterpolatedVarArrayBase<Type>::SetLooping( bool looping, int iArrayIndex )
{
	Assert( iArrayIndex >= 0 && iArrayIndex < m_nMaxCount );
	m_bLooping[ iArrayIndex ] = looping;
}

template< typename Type > 
inline void	CInterpolatedVarArrayBase<Type>::SetMaxCount( int newmax )
{
	bool changed = ( newmax != m_nMaxCount ) ? true : false;
	m_nMaxCount = newmax;
	// Wipe everything any time this changes!!!
	if ( changed )
	{
		delete [] m_bLooping;
		delete [] m_LastNetworkedValue;
		m_bLooping = new byte[m_nMaxCount];
		m_LastNetworkedValue = new Type[m_nMaxCount];
		memset( m_bLooping, 0, sizeof(byte) * m_nMaxCount);
		memset( m_LastNetworkedValue, 0, sizeof(Type) * m_nMaxCount);

		Reset();
	}
}


template< typename Type > 
inline int CInterpolatedVarArrayBase<Type>::GetMaxCount() const
{
	return m_nMaxCount;
}


template< typename Type > 
inline void CInterpolatedVarArrayBase<Type>::_Interpolate( Type *out, float frac, CInterpolatedVarEntry *start, CInterpolatedVarEntry *end )
{
	Assert( start );
	Assert( end );
	
	if ( start == end )
	{
		// quick exit
		for ( int i = 0; i < m_nMaxCount; i++ )
		{
			out[i] = end->value[i];
			Lerp_Clamp( out[i] );
		}
		return;
	}

	Assert( frac >= 0.0f && frac <= 1.0f );

	// Note that QAngle has a specialization that will do quaternion interpolation here...
	for ( int i = 0; i < m_nMaxCount; i++ )
	{
		if ( m_bLooping[ i ] )
		{
			out[i] = LoopingLerp( frac, start->value[i], end->value[i] );
		}
		else
		{
			out[i] = Lerp( frac, start->value[i], end->value[i] );
		}
		Lerp_Clamp( out[i] );
	}
}


template< typename Type > 
inline void CInterpolatedVarArrayBase<Type>::_Extrapolate( 
	Type *pOut,
	CInterpolatedVarEntry *pOld,
	CInterpolatedVarEntry *pNew,
	float flDestinationTime,
	float flMaxExtrapolationAmount
	)
{
	if ( fabs( pOld->changetime - pNew->changetime ) < 0.001f || flDestinationTime <= pNew->changetime )
	{
		for ( int i=0; i < m_nMaxCount; i++ )
			pOut[i] = pNew->value[i];
	}
	else
	{
		float flExtrapolationAmount = min( flDestinationTime - pNew->changetime, flMaxExtrapolationAmount );

		float divisor = 1.0f / (pNew->changetime - pOld->changetime);
		for ( int i=0; i < m_nMaxCount; i++ )
		{
			pOut[i] = ExtrapolateInterpolatedVarType( pOld->value[i], pNew->value[i], divisor, flExtrapolationAmount );
		}
	}
}


template< typename Type > 
inline void CInterpolatedVarArrayBase<Type>::TimeFixup2_Hermite( 
	typename CInterpolatedVarArrayBase<Type>::CInterpolatedVarEntry &fixup,
	typename CInterpolatedVarArrayBase<Type>::CInterpolatedVarEntry*& prev, 
	typename CInterpolatedVarArrayBase<Type>::CInterpolatedVarEntry*& start, 
	float dt1
	)
{
	float dt2 = start->changetime - prev->changetime;

	// If times are not of the same interval renormalize the earlier sample to allow for uniform hermite spline interpolation
	if ( fabs( dt1 - dt2 ) > 0.0001f &&
		dt2 > 0.0001f )
	{
		// Renormalize
		float frac = dt1 / dt2;

		// Fixed interval into past
		fixup.changetime = start->changetime - dt1;

		for ( int i = 0; i < m_nMaxCount; i++ )
		{
			fixup.value[i] = Lerp( 1-frac, prev->value[i], start->value[i] );
		}

		// Point previous sample at fixed version
		prev = &fixup;
	}
}


template< typename Type > 
inline void CInterpolatedVarArrayBase<Type>::TimeFixup_Hermite( 
	typename CInterpolatedVarArrayBase<Type>::CInterpolatedVarEntry &fixup,
	typename CInterpolatedVarArrayBase<Type>::CInterpolatedVarEntry*& prev, 
	typename CInterpolatedVarArrayBase<Type>::CInterpolatedVarEntry*& start, 
	typename CInterpolatedVarArrayBase<Type>::CInterpolatedVarEntry*& end	)
{
	TimeFixup2_Hermite( fixup, prev, start, end->changetime - start->changetime );
}


template< typename Type > 
inline void CInterpolatedVarArrayBase<Type>::_Interpolate_Hermite( 
	Type *out, 
	float frac, 
	CInterpolatedVarEntry *prev, 
	CInterpolatedVarEntry *start, 
	CInterpolatedVarEntry *end, 
	bool looping )
{
	Assert( start );
	Assert( end );

	// Disable range checks because we can produce weird values here and it's not an error.
	// After interpolation, we will clamp the values.
	CDisableRangeChecks disableRangeChecks; 

	CInterpolatedVarEntry fixup;
	fixup.value = (Type*)_alloca( sizeof(Type) * m_nMaxCount );
	TimeFixup_Hermite( fixup, prev, start, end );

	for( int i = 0; i < m_nMaxCount; i++ )
	{
		// Note that QAngle has a specialization that will do quaternion interpolation here...
		if ( m_bLooping[ i ] )
		{
			out[ i ] = LoopingLerp_Hermite( frac, prev->value[i], start->value[i], end->value[i] );
		}
		else
		{
			out[ i ] = Lerp_Hermite( frac, prev->value[i], start->value[i], end->value[i] );
		}

		// Clamp the output from interpolation. There are edge cases where something like m_flCycle
		// can get set to a really high or low value when we set it to zero after a really small
		// time interval (the hermite blender will think it's got a really high velocity and
		// skyrocket it off into la-la land).
		Lerp_Clamp( out[i] );
	}
}

template< typename Type > 
inline void CInterpolatedVarArrayBase<Type>::_Derivative_Hermite( 
	Type *out, 
	float frac, 
	CInterpolatedVarEntry *prev, 
	CInterpolatedVarEntry *start, 
	CInterpolatedVarEntry *end )
{
	Assert( start );
	Assert( end );

	// Disable range checks because we can produce weird values here and it's not an error.
	// After interpolation, we will clamp the values.
	CDisableRangeChecks disableRangeChecks; 

	CInterpolatedVarEntry fixup;
	fixup.value = (Type*)_alloca( sizeof(Type) * m_nMaxCount );
	TimeFixup_Hermite( fixup, prev, start, end );

	float divisor = 1.0f / (end->changetime - start->changetime);

	for( int i = 0; i < m_nMaxCount; i++ )
	{
		Assert( !m_bLooping[ i ] );
		out[i] = Derivative_Hermite( frac, prev->value[i], start->value[i], end->value[i] );
		out[i] *= divisor;
	}
}


template< typename Type > 
inline void CInterpolatedVarArrayBase<Type>::_Derivative_Hermite_SmoothVelocity( 
	Type *out, 
	float frac, 
	CInterpolatedVarEntry *b, 
	CInterpolatedVarEntry *c, 
	CInterpolatedVarEntry *d )
{
	CInterpolatedVarEntry fixup;
	fixup.value = (Type*)_alloca( sizeof(Type) * m_nMaxCount );
	TimeFixup_Hermite( fixup, b, c, d );
	for ( int i=0; i < m_nMaxCount; i++ )
	{
		Type prevVel = (c->value[i] - b->value[i]) / (c->changetime - b->changetime);
		Type curVel  = (d->value[i] - c->value[i]) / (d->changetime - c->changetime);
		out[i] = Lerp( frac, prevVel, curVel );
	}
}


template< typename Type > 
inline void CInterpolatedVarArrayBase<Type>::_Derivative_Linear( 
	Type *out, 
	CInterpolatedVarEntry *start, 
	CInterpolatedVarEntry *end )
{
	if ( start == end || fabs( start->changetime - end->changetime ) < 0.0001f )
	{
		for( int i = 0; i < m_nMaxCount; i++ )
		{
			out[ i ] = start->value[i] * 0;
		}
	}
	else 
	{
		float divisor = 1.0f / (end->changetime - start->changetime);
		for( int i = 0; i < m_nMaxCount; i++ )
		{
			out[ i ] = (end->value[i] - start->value[i]) * divisor;
		}
	}
}


template< typename Type > 
inline bool CInterpolatedVarArrayBase<Type>::ValidOrder()
{
	float newestchangetime = 0.0f;
	bool first = true;
	for ( int i = GetHead(); IsValidIndex( i ); i = GetNext( i ) )
	{
		CInterpolatedVarEntry *entry = &m_VarHistory[ i ];
		if ( first )
		{
			first = false;
			newestchangetime = entry->changetime;
			continue;
		}

		// They should get older as wel walk backwards
		if ( entry->changetime > newestchangetime )
		{
			Assert( 0 );
			return false;
		}

		newestchangetime = entry->changetime;
	}

	return true;
}

template< typename Type, int COUNT>
class CInterpolatedVarArray : public CInterpolatedVarArrayBase<Type>
{
public:
	CInterpolatedVarArray( const char *pDebugName = "no debug name" )
		: CInterpolatedVarArrayBase<Type>( pDebugName )
	{
		SetMaxCount( COUNT );
	}
};


// -------------------------------------------------------------------------------------------------------------- //
// CInterpolatedVar.
// -------------------------------------------------------------------------------------------------------------- //

template< typename Type >
class CInterpolatedVar : public CInterpolatedVarArray< Type, 1 >
{
public:
	CInterpolatedVar( const char *pDebugName= NULL );
};

template< typename Type >
inline CInterpolatedVar<Type>::CInterpolatedVar( const char *pDebugName )
	: CInterpolatedVarArray< Type, 1 >( pDebugName )
{
}

#include "tier0/memdbgoff.h"

#endif // INTERPOLATEDVAR_H
