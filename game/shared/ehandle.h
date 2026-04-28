//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef EHANDLE_H
#define EHANDLE_H
#ifdef _WIN32
#pragma once
#endif

#include "entity2/entitysystem.h"
#include "entityhandle.h"

// -------------------------------------------------------------------------------------------------- //
// Game-code CBaseHandle implementation.
// -------------------------------------------------------------------------------------------------- //

inline CEntityInstance* CEntityHandle::Get() const
{
	return GameEntitySystem()->GetEntityInstance( *this );
}

inline CEntityHandle CEntityHandle::FromPackedInt( int packed_int_handle )
{
	if(packed_int_handle == 0xFFFFFF)
		return CEntityHandle();

	CEntityIndex index = packed_int_handle & 0x3FFF;
	auto serial = (packed_int_handle >> 14) & 0x3FF;

	auto entity = GameEntitySystem()->GetEntityInstance( index );
	if(!entity)
		return CEntityHandle();

	auto ent_handle = entity->GetRefEHandle();

	// Since we don't have the full serial part, validate the one we have
	if((ent_handle.GetSerialNumber() & 0x3FF) != serial)
		return CEntityHandle();

	return ent_handle;
}

// -------------------------------------------------------------------------------------------------- //
// CHandle.
// -------------------------------------------------------------------------------------------------- //
template< class T >
class CHandle : public CEntityHandle
{
public:

			CHandle();
			CHandle( int iEntry, int iSerialNumber );
			CHandle( const CBaseHandle &handle );
			CHandle( T *pVal );

	// The index should have come from a call to ToInt(). If it hasn't, you're in trouble.
	static CHandle<T> FromIndex( int index );

	T*		Get() const;
	void	Set( const T* pVal );

			operator T*();
			operator T*() const;

	bool	operator !() const;
	bool	operator==( T *val ) const;
	bool	operator!=( T *val ) const;
	const CBaseHandle& operator=( const T *val );

	T*		operator->() const;
};


// ----------------------------------------------------------------------- //
// Inlines.
// ----------------------------------------------------------------------- //

template<class T>
CHandle<T>::CHandle()
{
}


template<class T>
CHandle<T>::CHandle( int iEntry, int iSerialNumber )
{
	Init( iEntry, iSerialNumber );
}


template<class T>
CHandle<T>::CHandle( const CBaseHandle &handle )
	: CBaseHandle( handle )
{
}


template<class T>
CHandle<T>::CHandle( T *pObj )
{
	Term();
	Set( pObj );
}


template<class T>
inline CHandle<T> CHandle<T>::FromIndex( int index )
{
	CHandle<T> ret;
	ret.m_Index = index;
	return ret;
}


template<class T>
inline T* CHandle<T>::Get() const
{
	return (T*)CBaseHandle::Get();
}


template<class T>
inline CHandle<T>::operator T *() 
{ 
	return Get( ); 
}

template<class T>
inline CHandle<T>::operator T *() const
{ 
	return Get( ); 
}


template<class T>
inline bool CHandle<T>::operator !() const
{
	return !Get();
}

template<class T>
inline bool CHandle<T>::operator==( T *val ) const
{
	return Get() == val;
}

template<class T>
inline bool CHandle<T>::operator!=( T *val ) const
{
	return Get() != val;
}

template<class T>
void CHandle<T>::Set( const T* pVal )
{
	CBaseHandle::Set(pVal);
}

template<class T>
inline const CBaseHandle& CHandle<T>::operator=( const T *val )
{
	Set( val );
	return *this;
}

template<class T>
T* CHandle<T>::operator -> () const
{
	return Get();
}


#endif // EHANDLE_H
