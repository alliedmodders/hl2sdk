//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef DMATTRIBUTEVAR_H
#define DMATTRIBUTEVAR_H
#ifdef _WIN32
#pragma once
#endif

#include "datamodel/idmattribute.h"
#include "tier1/utlvector.h"
#include "color.h"
#include "vector2D.h"
#include "vector.h"
#include "vector4D.h"
#include "VMatrix.h"

//-----------------------------------------------------------------------------
// Helper template for external attributes
//-----------------------------------------------------------------------------
template< class Type >
class CDmAttributeVar
{
public:
	CDmAttributeVar( )
	{
		m_pAttribute = NULL;
		CDmAttributeInfo<Type>::SetDefaultValue( m_Value );
	}

	void Init( IDmElement *pOwner, const char *pAttributeName, int flags = 0 )
	{
		Assert( pOwner );
		m_pAttribute = pOwner->GetDmElementInternal()->AddExternalAttribute( pAttributeName, CDmAttributeInfo<Type>::AttributeType(), &m_Value );
		Assert( m_pAttribute );
		if ( flags )
			m_pAttribute->AddFlag( flags );
	}

	void InitAndSet( IDmElement *pOwner, const char *pAttributeName, const Type &value, int flags = 0 )
	{
		Init( pOwner, pAttributeName );
		Set( value );

		// this has to happen AFTER set so the set happens before FATTRIB_READONLY
		if ( flags )
			m_pAttribute->AddFlag( flags );
	}

	const Type& Set( const Type &val )
	{
		Assert( m_pAttribute );
		if ( !m_pAttribute->MarkDirty() )
		{
			Assert( 0 );
			static Type s_value;
			return s_value;
		}

		DmAttributeSetValue( m_pAttribute, val );
		return m_Value;
	}

	const Type& operator=( const Type &val ) 
	{
		return Set( val );
	}

	const Type& operator+=( const Type &val ) 
	{
		return Set( m_Value + val );
	}

	const Type& operator-=( const Type &val ) 
	{
		return Set( m_Value - val );
	}
	
	const Type& operator/=( const Type &val ) 
	{
		return Set( m_Value / val );
	}
	
	const Type& operator*=( const Type &val ) 
	{
		return Set( m_Value * val );
	}
	
	const Type& operator^=( const Type &val ) 
	{
		return Set( m_Value ^ val );
	}

	const Type& operator|=( const Type &val ) 
	{
		return Set( m_Value | val );
	}

	const Type& operator&=( const Type &val ) 
	{	
		return Set( m_Value & val );
	}

	Type operator++()
	{
		return Set( m_Value + 1 );
	}

	Type operator--()
	{
		return Set( m_Value - 1 );
	}
	
	Type operator++( int ) // postfix version..
	{
		Type oldValue = m_Value;
		Set( m_Value + 1 );
		return oldValue;
	}

	Type operator--( int ) // postfix version..
	{
		Type oldValue = m_Value;
		Set( m_Value - 1 );
		return oldValue;
	}

	operator const Type&() const 
	{
		return m_Value; 
	}
	
	const Type& Get() const 
	{
		return m_Value; 
	}
	
	const Type* operator->() const 
	{
		return &m_Value; 
	}

	IDmAttribute *GetAttribute()
	{
		Assert( m_pAttribute );
		return m_pAttribute;
	}

	const IDmAttribute *GetAttribute() const
	{
		Assert( m_pAttribute );
		return m_pAttribute;
	}

	bool IsDirty() const
	{
		Assert( m_pAttribute );
		return m_pAttribute->IsFlagSet( FATTRIB_DIRTY );
	}

protected:
	Type m_Value;
	IDmAttribute *m_pAttribute;
};


//-----------------------------------------------------------------------------
// Specialization for color
//-----------------------------------------------------------------------------
class CDmAttributeVarColor : public CDmAttributeVar< Color >
{
public:
	inline void SetColor( int r, int g, int b, int a = 0 )
	{
		Color clr( r, g, b, a );
		DmAttributeSetValue( m_pAttribute, clr );
	}

	inline void SetRed( int r )
	{
		Color org = m_Value;
		org[ 0 ] = r;
		DmAttributeSetValue( m_pAttribute, org );
	}
	
	inline void SetGreen( int g )
	{
		Color org = m_Value;
		org[ 1 ] = g;
		DmAttributeSetValue( m_pAttribute, org );
	}

	inline void SetBlue( int b )
	{
		Color org = m_Value;
		org[ 2 ] = b;
		DmAttributeSetValue( m_pAttribute, org );
	}

	inline void SetAlpha( int a )
	{
		Color org = m_Value;
		org[ 3 ] = a;
		DmAttributeSetValue( m_pAttribute, org );
	}

	inline unsigned char r() const
	{
		return (unsigned char)m_Value.r();
	}

	inline unsigned char g() const
	{
		return (unsigned char)m_Value.g();
	}

	inline unsigned char b() const
	{
		return (unsigned char)m_Value.b();
	}

	inline unsigned char a() const
	{
		return (unsigned char)m_Value.a();
	}

	const unsigned char &operator[](int index) const
	{
		return m_Value[index];
	}

	void SetRawColor( int color )
	{
		Color clr;
		clr.SetRawColor( color );
		DmAttributeSetValue( m_pAttribute, clr );
	}
};


//-----------------------------------------------------------------------------
// Specialization for object ids
//-----------------------------------------------------------------------------
class CDmAttributeVarObjectId : public CDmAttributeVar< DmObjectId_t >
{
public:
	void CreateObjectId( )
	{ 
		DmObjectId_t id;
		g_pDataModel->CreateObjectId( &id );
		DmAttributeSetValue( m_pAttribute, id );
	}

	void Invalidate( )
	{
		DmObjectId_t id;
		g_pDataModel->Invalidate( &id );
		DmAttributeSetValue( m_pAttribute, id );
	}

	bool IsValid( ) const
	{
		return g_pDataModel->IsValid( m_Value );
	}

	bool IsEqual( const DmObjectId_t &id ) const
	{
		return g_pDataModel->IsEqual( m_Value, id );
	}

	const DmObjectId_t &operator=( const DmObjectId_t& src )
	{
		DmAttributeSetValue( m_pAttribute, src );
		return m_Value;
	}

	const DmObjectId_t& Set( const DmObjectId_t &src )
	{
		DmAttributeSetValue( m_pAttribute, src );
		return m_Value;
	}
};


//-----------------------------------------------------------------------------
// Specialization for string
//-----------------------------------------------------------------------------
class CDmAttributeVarString : public CDmAttributeVar< CUtlString >
{
public:
	const char *Get( ) const
	{
		return m_Value.Get();
	}

	operator const char*() const
	{
		return m_Value.Get();
	}

	void Set( const char *pValue )
	{
		CUtlString str( pValue, pValue ? Q_strlen( pValue ) + 1 : 0 );
		DmAttributeSetValue( m_pAttribute, str );
	}

	// Returns strlen
	int	Length() const
	{
		return m_Value.Length();
	}

	CDmAttributeVarString &operator=( const char *src )
	{
		Set( src );
		return *this;
	}
};


//-----------------------------------------------------------------------------
// Specialization for binary block
//-----------------------------------------------------------------------------
class CDmAttributeVarBinaryBlock : public CDmAttributeVar< CUtlBinaryBlock >
{
public:
	void Get( void *pValue, int nMaxLen ) const
	{
		m_Value.Get( pValue, nMaxLen );
	}

	void Set( const void *pValue, int nLen )
	{
		CUtlBinaryBlock block( pValue, nLen );
		DmAttributeSetValue( m_pAttribute, block );
	}

	const void *Get() const
	{
		return m_Value.Get();
	}

	const unsigned char& operator[]( int i ) const
	{
		return m_Value[i];
	}

	// Returns strlen
	int	Length() const
	{
		return m_Value.Length();
	}
};


//-----------------------------------------------------------------------------
// Specialization for elements
//-----------------------------------------------------------------------------
template <class T>
class CDmAttributeVarElement : public CDmAttributeVar< CDmElementRef >
{
	typedef CDmAttributeVar< CDmElementRef > BaseClass;

public:
	void InitAndCreate( IDmElement *pOwner, const char *pAttributeName, const char *pElementName = NULL, int flags = 0 )
	{
		Init( pOwner, pAttributeName );
	
		DmElementHandle_t hElement = g_pDataModel->CreateElement( g_pDataModel->GetString( T::GetStaticTypeSymbol() ), pElementName );
		Assert( m_pAttribute );
		CDmElementRef ref( hElement );
		DmAttributeSetValue( m_pAttribute, ref );
		g_pDataModel->GetElementInternal( hElement )->Release(); // Create incremented once, so did SetValue, so release once

		// this has to happen AFTER set so the set happens before FATTRIB_READONLY
		m_pAttribute->AddFlag( flags | FATTRIB_MUSTCOPY );
	}

	void Init( IDmElement *pOwner, const char *pAttributeName, int flags = 0 )
	{
		BaseClass::Init( pOwner, pAttributeName );
	
		IDmAttributeElement *pAttributeElement = static_cast< IDmAttributeElement * >( m_pAttribute );
		Assert( pAttributeElement );
		pAttributeElement->SetTypeSymbol( T::GetStaticTypeSymbol() );
		if ( flags )
			m_pAttribute->AddFlag( flags );
	}

	UtlSymId_t GetElementType() const
	{
		return m_Value.GetElementType();
	}

	T* Get() const
	{
		return static_cast<T*>( g_pDataModel->GetElement( m_Value.Get() ) );
	}

	const T* operator->() const 
	{
		return Get();
	}

	operator T*() const
	{
		return Get();
	}

	void Set( T* pElement )
	{
		CDmElementRef ref( pElement ? pElement->GetHandle() : DMELEMENT_HANDLE_INVALID );
		DmAttributeSetValue( m_pAttribute, ref );
	}

	template <class S>
	CDmAttributeVarElement<T> &operator=( S* pElement )
	{
		Set( static_cast<T*>( pElement ) );
		return *this;
	}

	template <class S>
	CDmAttributeVarElement<T> &operator=( const CDmAttributeVarElement<S>& src )
	{
		Set( static_cast<T*>( src.Get() ) );
		return *this;
	}

	template <class S>
	bool operator==( const CDmAttributeVarElement<S>& src )	const
	{
		return m_Value == src.m_Value;
	}

	template <class S>
	bool operator!=( const CDmAttributeVarElement<S>& src )	const
	{
		return m_Value != src.m_Value;
	}
};


//-----------------------------------------------------------------------------
// Helper template for external vector attributes
//-----------------------------------------------------------------------------
template< class T >
class CDmAttributeVarUtlVector
{
public:

	CDmAttributeVarUtlVector( )
	{
		m_pAttribute = NULL;
		CDmAttributeInfo<CUtlVector<T> >::SetDefaultValue( m_Value );
	}

	void Init( IDmElement *pOwner, const char *pAttributeName, int flags = 0 )
	{
		Assert( pOwner );
		IDmAttribute *pAttribute = pOwner->GetDmElementInternal()->AddExternalAttribute( pAttributeName, CDmAttributeInfo<CUtlVector<T> >::AttributeType(), &m_Value );
		m_pAttribute = static_cast< IDmAttributeTypedArray< T >* >( pAttribute );
		Assert( m_pAttribute );
		if ( flags )
			m_pAttribute->AddFlag( flags );
	}

	const CUtlVector<T> &Get() const
	{
		return m_Value;
	}

	int Find( const T &value ) const
	{
		return m_Value.Find( value );
	}

	void	Set( int i, const T& value )
	{
		m_pAttribute->Set( i, value );
	}

	const T& operator[]( int i ) const
	{
		return m_Value[ i ];
	}

	const T& Get( int i ) const
	{
		return m_Value[ i ];
	}

	int		Count() const
	{
		return m_Value.Count();
	}

	bool	IsValidIndex( int i ) const
	{
		return m_Value.IsValidIndex( i );
	}

	void	Swap( int i, int j )
	{
		m_pAttribute->Swap( i, j );
	}

	void	Swap( CUtlVector< T > &array )
	{
		m_pAttribute->Swap( array );
	}

	// Adds an element, uses default constructor
	int		AddToHead()
	{
		return m_pAttribute->AddToHead();
	}
	int		AddToTail()
	{
		return m_pAttribute->AddToTail();
	}
	int		InsertBefore( int elem )
	{
		return m_pAttribute->InsertBefore( elem );
	}
	int		InsertAfter( int elem )
	{
		return m_pAttribute->InsertAfter( elem );
	}

	// Adds an element, uses copy constructor
	int		AddToHead( const T& src )
	{
		return m_pAttribute->AddToHead( src );
	}
	int		AddToTail( const T& src )
	{
		return m_pAttribute->AddToTail( src );
	}
	int		InsertBefore( int elem, const T& src )
	{
		return m_pAttribute->InsertBefore( elem, src );
	}
	int		InsertAfter( int elem, const T& src )
	{
		return m_pAttribute->InsertAfter( elem, src );
	}

	// Makes sure we have enough memory allocated to store a requested # of elements
	void	EnsureCapacity( int num )
	{
		m_Value.EnsureCapacity( num );
	}
	void	EnsureCount( int num )
	{
		m_Value.EnsureCount( num );
	}
	void	FastRemove( int elem )
	{
		m_pAttribute->FastRemove( elem );
	}
	void	Remove( int elem )
	{
		m_pAttribute->Remove( elem );
	}
	void	RemoveAll()
	{
		m_pAttribute->RemoveAll();
	}

	void	RemoveMultiple( int elem, int num )
	{
		m_pAttribute->RemoveMultiple( elem, num );
	}

	// Memory deallocation
	void	Purge()
	{
		m_pAttribute->Purge();
	}

	void CopyArray( const T *pArray, int size )
	{
		m_pAttribute->CopyArray( pArray, size );
	}

	const CUtlVector<T> &operator=( const CUtlVector<T>& src )
	{
		m_pAttribute->CopyFrom( src );
		return m_Value;
	}

	int InvalidIndex( void ) const
	{
		return m_Value.InvalidIndex();
	}

	const T *Base() const
	{
		return m_Value.Base();
	}

	IDmAttributeArray *GetAttribute()
	{
		Assert( m_pAttribute );
		return m_pAttribute;
	}

	const IDmAttributeArray *GetAttribute() const
	{
		Assert( m_pAttribute );
		return m_pAttribute;
	}

	bool IsDirty() const
	{
		return m_pAttribute->IsFlagSet( FATTRIB_DIRTY );
	}

protected:
	CUtlVector< T > m_Value;
	IDmAttributeTypedArray< T > *m_pAttribute;
};


//-----------------------------------------------------------------------------
// Specialization for string arrays
//-----------------------------------------------------------------------------
class CDmAttributeVarStringArray : public CDmAttributeVarUtlVector< CUtlString >
{
public:
	const char *operator[]( int i ) const
	{
		return m_Value[ i ].Get();
	}

	const char *Get( int i ) const
	{
		return m_Value[ i ].Get();
	}

	const CUtlVector< CUtlString > &Get() const
	{
		return m_Value;
	}

	void Set( int i, const char * pValue )
	{
		CUtlString str( pValue, Q_strlen( pValue ) + 1 );
		m_pAttribute->Set( i, str );
	}

	// Adds an element, uses copy constructor
	int	AddToHead( const char *pValue )
	{
		CUtlString str( pValue, Q_strlen( pValue ) + 1 );
		return m_pAttribute->AddToHead( str );
	}

	int	AddToTail( const char *pValue )
	{
		CUtlString str( pValue, Q_strlen( pValue ) + 1 );
		return m_pAttribute->AddToTail( str );
	}

	int	InsertBefore( int elem, const char *pValue )
	{
		CUtlString str( pValue, Q_strlen( pValue ) + 1 );
		return m_pAttribute->InsertBefore( elem, str );
	}

	int	InsertAfter( int elem, const char *pValue )
	{
		CUtlString str( pValue, Q_strlen( pValue ) + 1 );
		return m_pAttribute->InsertAfter( elem, str );
	}

	// Returns strlen of element i
	int	Length( int i ) const
	{
		return m_Value[i].Length();
	}
};


//-----------------------------------------------------------------------------
// Specialization for elements
//-----------------------------------------------------------------------------
template <class T>
class CDmAttributeVarElementArray : public CDmAttributeVarUtlVector< CDmElementRef >
{
	typedef CDmAttributeVarUtlVector< CDmElementRef > BaseClass;

public:
	void Init( IDmElement *pOwner, const char *pAttributeName, int flags = 0 )
	{
		BaseClass::Init( pOwner, pAttributeName );
	
		IDmAttributeElementArray *pAttributeElement = static_cast< IDmAttributeElementArray * >( m_pAttribute );
		Assert( pAttributeElement );
		pAttributeElement->SetTypeSymbol( T::GetStaticTypeSymbol() );
		if ( flags )
			m_pAttribute->AddFlag( flags );
	}

	UtlSymId_t GetElementType() const
	{
		return m_Value.GetElementType();
	}

public:
	T *operator[]( int i ) const
	{
		return static_cast<T*>( g_pDataModel->GetElement( m_Value[i].Get() ) );
	}

	T *Get( int i ) const
	{
		return static_cast<T*>( g_pDataModel->GetElement( m_Value[i].Get() ) );
	}

	const CUtlVector< CDmElementRef > &Get() const
	{
		return m_Value;
	}

	void Set( int i, T *pElement )
	{
		CDmElementRef ref( pElement ? pElement->GetHandle() : DMELEMENT_HANDLE_INVALID );
		m_pAttribute->Set( i, ref );
	}

	// Adds an element, uses copy constructor
	int	AddToHead( T *pValue )
	{
		CDmElementRef ref( pValue ? pValue->GetHandle() : DMELEMENT_HANDLE_INVALID );
		return m_pAttribute->AddToHead( ref );
	}

	int	AddToTail( T *pValue )
	{
		CDmElementRef ref( pValue ? pValue->GetHandle() : DMELEMENT_HANDLE_INVALID );
		return m_pAttribute->AddToTail( ref );
	}

	int	InsertBefore( int elem, T *pValue )
	{
		CDmElementRef ref( pValue ? pValue->GetHandle() : DMELEMENT_HANDLE_INVALID );
		return m_pAttribute->InsertBefore( elem, ref );
	}

	int	InsertAfter( int elem, T *pValue )
	{
		CDmElementRef ref( pValue ? pValue->GetHandle() : DMELEMENT_HANDLE_INVALID );
		return m_pAttribute->InsertAfter( elem, ref );
	}
};


#endif // DMATTRIBUTEVAR_H
