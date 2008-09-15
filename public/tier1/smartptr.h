//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SMARTPTR_H
#define SMARTPTR_H
#ifdef _WIN32
#pragma once
#endif


class CRefCountAccessor
{
public:
	template< class T >
	static void AddRef( T *pObj )
	{
		pObj->AddRef();
	}

	template< class T >
	static void Release( T *pObj )
	{
		pObj->Release();
	}
};

// This can be used if your objects use AddReference/ReleaseReference function names.
class CRefCountAccessorLongName
{
public:
	template< class T >
	static void AddRef( T *pObj )
	{
		pObj->AddReference();
	}

	template< class T >
	static void Release( T *pObj )
	{
		pObj->ReleaseReference();
	}
};


// Smart pointers can be used to automatically free an object when nobody points
// at it anymore. Things contained in smart pointers must implement AddRef and Release
// functions. If those functions are private, then the class must make
// CRefCountAccessor a friend.
template<class T, class RefCountAccessor=CRefCountAccessor>
class CSmartPtr
{
public:
					CSmartPtr();
					CSmartPtr( T *pObj );
					CSmartPtr( const CSmartPtr<T,RefCountAccessor> &other );
					~CSmartPtr();

	T*				operator=( T *pObj );
	void			operator=( const CSmartPtr<T,RefCountAccessor> &other );
	const T*		operator->() const;
	T*				operator->();
	bool			operator!() const;
	bool			operator==( const T *pOther ) const;
	bool			IsValid() const; // Tells if the pointer is valid.
	T*				GetObject() const; // Get temporary object pointer, don't store it for later reuse!

private:
	T				*m_pObj;
};


template< class T, class RefCountAccessor >
inline CSmartPtr<T,RefCountAccessor>::CSmartPtr()
{
	m_pObj = NULL;
}

template< class T, class RefCountAccessor >
inline CSmartPtr<T,RefCountAccessor>::CSmartPtr( T *pObj )
{
	m_pObj = NULL;
	*this = pObj;
}

template< class T, class RefCountAccessor >
inline CSmartPtr<T,RefCountAccessor>::CSmartPtr( const CSmartPtr<T,RefCountAccessor> &other )
{
	m_pObj = NULL;
	*this = other;
}

template< class T, class RefCountAccessor >
inline CSmartPtr<T,RefCountAccessor>::~CSmartPtr()
{
	if ( m_pObj )
	{
		RefCountAccessor::Release( m_pObj );
	}
}

template< class T, class RefCountAccessor >
inline T* CSmartPtr<T,RefCountAccessor>::operator=( T *pObj )
{
	if ( pObj == m_pObj )
		return pObj;

	if ( pObj )
		RefCountAccessor::AddRef( pObj );

	if ( m_pObj )
		RefCountAccessor::Release( m_pObj );

	m_pObj = pObj;
	return pObj;
}

template< class T, class RefCountAccessor >
inline void CSmartPtr<T,RefCountAccessor>::operator=( const CSmartPtr<T,RefCountAccessor> &other )
{
	*this = other.m_pObj;
}

template< class T, class RefCountAccessor >
inline const T* CSmartPtr<T,RefCountAccessor>::operator->() const
{
	return m_pObj;
}

template< class T, class RefCountAccessor >
inline T* CSmartPtr<T,RefCountAccessor>::operator->()
{
	return m_pObj;
}

template< class T, class RefCountAccessor >
inline bool CSmartPtr<T,RefCountAccessor>::operator!() const
{
	return !m_pObj;
}

template< class T, class RefCountAccessor >
inline bool CSmartPtr<T,RefCountAccessor>::operator==( const T *pOther ) const
{
	return m_pObj == pOther;
}

template< class T, class RefCountAccessor >
inline bool CSmartPtr<T,RefCountAccessor>::IsValid() const
{
	return m_pObj != NULL;
}

template< class T, class RefCountAccessor >
inline T* CSmartPtr<T,RefCountAccessor>::GetObject() const
{
	return m_pObj;
}




#endif // SMARTPTR_H
