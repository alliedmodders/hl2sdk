//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef IDMATTRIBUTE_H
#define IDMATTRIBUTE_H
#ifdef _WIN32
#pragma once
#endif

#include "datamodel/attributeflags.h"
#include "datamodel/dmelementref.h"
#include "tier1/utlvector.h"
#include "tier1/utlstring.h"
#include "color.h"
#include "vector2D.h"
#include "vector.h"
#include "vector4D.h"
#include "VMatrix.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class IDmElementInternal;
class IDmElement;
struct DmObjectId_t;
class CUtlBuffer;
class IDmAttributeInternal;

inline bool IsInterpolableType( DmAttributeType_t type )
{
	return  ( type == AT_FLOAT ) ||
			( type == AT_COLOR ) ||
			( type == AT_VECTOR2 ) ||
			( type == AT_VECTOR3 ) ||
			( type == AT_QANGLE ) ||
			( type == AT_QUATERNION );
}


inline bool IsValueType( DmAttributeType_t type )
{
	return type >= AT_FIRST_VALUE_TYPE && type < AT_FIRST_ARRAY_TYPE;
}

inline bool IsArrayType( DmAttributeType_t type )
{
	return type >= AT_FIRST_ARRAY_TYPE && type < AT_TYPE_COUNT;
}

inline DmAttributeType_t ValueTypeToArrayType( DmAttributeType_t type )
{
	Assert( IsValueType( type ) );
	return ( DmAttributeType_t )( ( type - AT_FIRST_VALUE_TYPE ) + AT_FIRST_ARRAY_TYPE );
}

inline DmAttributeType_t ArrayTypeToValueType( DmAttributeType_t type )
{
	Assert( IsArrayType( type ) );
	return ( DmAttributeType_t )( ( type - AT_FIRST_ARRAY_TYPE ) + AT_FIRST_VALUE_TYPE );
}

/*
template< class T >
class CDmAttributeArray
{
public:
	void	Set( int i, const T& value );

	const T& operator[]( int i ) const;
	const T& Element( int i ) const;

	int		Count() const;
	bool	IsValidIndex( int i ) const;

	// Adds an element, uses default constructor
	int		AddToHead();
	int		AddToTail();
	int		InsertBefore( int elem );
	int		InsertAfter( int elem );

	// Adds an element, uses copy constructor
	int		AddToHead( const T& src );
	int		AddToTail( const T& src );
	int		InsertBefore( int elem, const T& src );
	int		InsertAfter( int elem, const T& src );

	// Makes sure we have enough memory allocated to store a requested # of elements
	void	EnsureCapacity( int num );
	void	FastRemove( int elem );	// doesn't preserve order
	void	Remove( int elem );		// preserves order, shifts elements
	void	RemoveAll();				// doesn't deallocate memory

	// Memory deallocation
	void	Purge();

	T &operator=( const T& src );

	static int InvalidIndex( void );

private:
	CUtlVector< T >	m_Value;
};
*/


//-----------------------------------------------------------------------------
// Attribute info... 
//-----------------------------------------------------------------------------
template <class T>
class CDmAttributeInfo
{
public:
	static DmAttributeType_t AttributeType( )
	{
		return AT_UNKNOWN;
	}

	static const char *AttributeTypeName()
	{
		return "unknown";
	}

	static void SetDefaultValue( T& value )
	{
	}
};


#define DECLARE_ATTRIBUTE_TYPE( _className, _attributeType, _attributeName, _defaultSetStatement ) \
	template< > class CDmAttributeInfo< _className >						\
	{																		\
	public:																	\
		static DmAttributeType_t AttributeType() { return _attributeType; } \
		static const char *AttributeTypeName() { return _attributeName; }		\
		static void SetDefaultValue( _className& value ) { _defaultSetStatement }	\
	};																		\

#define DECLARE_ATTRIBUTE_ARRAY_TYPE( _className, _attributeType, _attributeName )\
	template< > class CDmAttributeInfo< CUtlVector<_className> >			\
	{																			\
	public:																		\
		static DmAttributeType_t AttributeType() { return _attributeType; }		\
		static const char *AttributeTypeName() { return _attributeName; }		\
		static void SetDefaultValue( CUtlVector< _className >& value ) { value.RemoveAll(); }	\
	};																			\

// NOTE: If you add an attribute type here, also add it to the list of DEFINE_ATTRIBUTE_TYPES in dmattribute.cpp
DECLARE_ATTRIBUTE_TYPE( int,					AT_INT,					"int",			value = 0; )
DECLARE_ATTRIBUTE_TYPE( float,					AT_FLOAT,				"float",		value = 0.0f; )
DECLARE_ATTRIBUTE_TYPE( bool,					AT_BOOL,				"bool",			value = false; )
DECLARE_ATTRIBUTE_TYPE( Color,					AT_COLOR,				"color",		value.SetColor( 0, 0, 0, 255 ); )
DECLARE_ATTRIBUTE_TYPE( Vector2D,				AT_VECTOR2,				"vector2",		value.Init( 0.0f, 0.0f ); )
DECLARE_ATTRIBUTE_TYPE( Vector,					AT_VECTOR3,				"vector3",		value.Init( 0.0f, 0.0f, 0.0f ); )
DECLARE_ATTRIBUTE_TYPE( Vector4D,				AT_VECTOR4,				"vector4",		value.Init( 0.0f, 0.0f, 0.0f, 0.0f ); )
DECLARE_ATTRIBUTE_TYPE( QAngle,					AT_QANGLE,				"qangle",		value.Init( 0.0f, 0.0f, 0.0f ); )
DECLARE_ATTRIBUTE_TYPE( Quaternion,				AT_QUATERNION,			"quaternion",	value.Init( 0.0f, 0.0f, 0.0f, 1.0f ); )
DECLARE_ATTRIBUTE_TYPE( VMatrix,				AT_VMATRIX,				"matrix",		MatrixSetIdentity( value ); )
DECLARE_ATTRIBUTE_TYPE( CUtlString,				AT_STRING,				"string",		value.Set( NULL ); )
DECLARE_ATTRIBUTE_TYPE( CUtlBinaryBlock,		AT_VOID,				"binary",		value.Set( NULL, 0 ); )
DECLARE_ATTRIBUTE_TYPE( DmObjectId_t,			AT_OBJECTID,			"elementid",	g_pDataModel->Invalidate( &value ); )
DECLARE_ATTRIBUTE_TYPE( CDmElementRef,			AT_ELEMENT,				"element",		value.Set( DMELEMENT_HANDLE_INVALID ); )

DECLARE_ATTRIBUTE_ARRAY_TYPE( int,				AT_INT_ARRAY,			"int_array" )
DECLARE_ATTRIBUTE_ARRAY_TYPE( float,			AT_FLOAT_ARRAY,			"float_array" )
DECLARE_ATTRIBUTE_ARRAY_TYPE( bool,				AT_BOOL_ARRAY,			"bool_array" )
DECLARE_ATTRIBUTE_ARRAY_TYPE( Color,			AT_COLOR_ARRAY,			"color_array" )
DECLARE_ATTRIBUTE_ARRAY_TYPE( Vector2D,			AT_VECTOR2_ARRAY,		"vector2_array" )
DECLARE_ATTRIBUTE_ARRAY_TYPE( Vector,			AT_VECTOR3_ARRAY,		"vector3_array" )
DECLARE_ATTRIBUTE_ARRAY_TYPE( Vector4D,			AT_VECTOR4_ARRAY,		"vector4_array" )
DECLARE_ATTRIBUTE_ARRAY_TYPE( QAngle,			AT_QANGLE_ARRAY,		"qangle_array" )
DECLARE_ATTRIBUTE_ARRAY_TYPE( Quaternion,		AT_QUATERNION_ARRAY,	"quaternion_array" )
DECLARE_ATTRIBUTE_ARRAY_TYPE( VMatrix,			AT_VMATRIX_ARRAY,		"matrix_array" )
DECLARE_ATTRIBUTE_ARRAY_TYPE( CUtlString,		AT_STRING_ARRAY,		"string_array" )
DECLARE_ATTRIBUTE_ARRAY_TYPE( CUtlBinaryBlock,	AT_VOID_ARRAY,			"binary_array" )
DECLARE_ATTRIBUTE_ARRAY_TYPE( DmObjectId_t,		AT_OBJECTID_ARRAY,		"elementid_array" )
DECLARE_ATTRIBUTE_ARRAY_TYPE( CDmElementRef,	AT_ELEMENT_ARRAY,		"element_array" )


//-----------------------------------------------------------------------------
// Purpose: A general purpose pAttribute.  Eventually will be extensible to arbitrary user types
//-----------------------------------------------------------------------------
abstract_class IDmAttribute
{
public:
	// Returns the type
	virtual	DmAttributeType_t GetType() const = 0;

	// Returns the name. NOTE: The utlsymbol
	// can be turned into a string by using g_pDataModel->String();
	virtual const char *GetName() const = 0;
	virtual UtlSymId_t GetNameSymbol() const = 0;
	virtual void	Rename( char const *newName ) = 0;

	// Returns the next attribute
	virtual IDmAttribute *NextAttribute() = 0;
	virtual const IDmAttribute *NextAttribute() const = 0;

	// Returns the owner
	virtual IDmElementInternal *GetOwner() = 0;

	// Methods related to flags
	virtual void	AddFlag( int flags ) = 0;
	virtual void	RemoveFlag( int flags ) = 0;
	virtual void	ClearFlags() = 0;
	virtual int		GetFlags() const = 0;
	virtual bool	IsFlagSet( int flags ) const = 0;

	// Is the attribute defined?
	virtual bool	IsDefined() const = 0;
	virtual void	MakeUndefined() = 0;

	// Is modification allowed in this phase?
	virtual bool	ModificationAllowed() const = 0;
	virtual bool	MarkDirty() = 0;

	// Copies w/ type conversion (if possible) from another attribute
	virtual void	SetValue( const IDmAttribute *pAttribute ) = 0;
	virtual void	SetValue( DmAttributeType_t valueType, void *pValue ) = 0;
	virtual void*	GetValueUntyped() = 0; // used with GetType() for use w/ SetValue( type, void* )

	// Serialization
	virtual bool	Serialize( CUtlBuffer &buf ) const = 0;
	virtual bool	Unserialize( CUtlBuffer &buf ) = 0;

	// Does this attribute serialize on multiple lines?
	virtual bool	SerializesOnMultipleLines() const = 0;

	// Get the attribute/create an attribute handle
	virtual DmAttributeHandle_t GetHandle( bool bCreate = true ) = 0;

	// Notify external elements upon change ( Calls OnAttributeChanged )
	// Pass false here to stop notification
	virtual void NotifyWhenChanged( DmElementHandle_t h, bool bNotify ) = 0;
};


//-----------------------------------------------------------------------------
// Interfaces for single-valued attributes
//-----------------------------------------------------------------------------
template <class T>
abstract_class IDmAttributeTyped : public IDmAttribute
{
public:
	virtual const T&	GetValue() const = 0;
	virtual void		SetValue( const T &value ) = 0;

	// inhertited from IDmAttribute - redeclared here so that SetValue( const T& ) doens't override
	virtual void		SetValue( const IDmAttribute *pAttribute ) = 0;
	virtual void		SetValue( DmAttributeType_t valueType, void *pValue ) = 0;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
abstract_class IDmAttributeElement : public IDmAttributeTyped< CDmElementRef >
{
public:
	virtual void		SetTypeSymbol( UtlSymId_t typeSymbol ) = 0;
	virtual UtlSymId_t	GetTypeSymbol() const = 0;
};

//-----------------------------------------------------------------------------
// Interface for array attributes
//-----------------------------------------------------------------------------
abstract_class IDmAttributeArray : public IDmAttribute
{
public:
	virtual int		Count() const = 0;
	virtual bool	IsValidIndex( int i ) const = 0;

	// modification methods that take a type and a void*
	virtual void*	GetUntyped( int i ) = 0;
	virtual void	Set( int i, DmAttributeType_t valueType, void *pValue ) = 0;
	virtual int		AddToHead( DmAttributeType_t valueType, void *pValue ) = 0;
	virtual int		AddToTail( DmAttributeType_t valueType, void *pValue ) = 0;
	virtual int		InsertBefore( int elem, DmAttributeType_t valueType, void *pValue ) = 0;
	virtual int		InsertAfter( int elem, DmAttributeType_t valueType, void *pValue ) = 0;

	// Adds an element, uses default constructor
	virtual int		AddToHead() = 0;
	virtual int		AddToTail() = 0;
	virtual int		InsertBefore( int elem ) = 0;
	virtual int		InsertAfter( int elem ) = 0;

	// Makes sure we have enough memory allocated to store a requested # of elements
	virtual void	EnsureCapacity( int num ) = 0;
	virtual void	EnsureCount( int num ) = 0;
	virtual void	Swap( int i, int j ) = 0;
	virtual void	FastRemove( int elem ) = 0;	// doesn't preserve order
	virtual void	Remove( int elem ) = 0;		// preserves order, shifts elements
	virtual void	RemoveAll() = 0;				// doesn't deallocate memory
	virtual void	RemoveMultiple( int elem, int num ) = 0;	// preserves order, shifts elements

	// Memory deallocation
	virtual void	Purge() = 0;

	virtual int		InvalidIndex( void ) = 0;

	// Serialization of a single element. 
	// First version of UnserializeElement adds to tail if it worked
	// Second version overwrites, but does not add, the element at the specified index 
	virtual bool	SerializeElement( int nElement, CUtlBuffer &buf ) const = 0;
	virtual bool	UnserializeElement( CUtlBuffer &buf ) = 0;
	virtual bool	UnserializeElement( int nElement, CUtlBuffer &buf ) = 0;
};

template< class T >
abstract_class IDmAttributeTypedArray : public IDmAttributeArray
{
public:
	virtual void	Set( int i, const T& value ) = 0;

	virtual const T& Get( int i ) const = 0;
	virtual const CUtlVector< T >& GetValue() const = 0;

	// Adds an element, uses copy constructor
	virtual int		AddToHead( const T& src ) = 0;
	virtual int		AddToTail( const T& src ) = 0;
	virtual int		InsertBefore( int elem, const T& src ) = 0;
	virtual int		InsertAfter( int elem, const T& src ) = 0;

	virtual void	CopyFrom( const CUtlVector< T >& src ) = 0;
	virtual void	CopyArray( const T *pArray, int size ) = 0;

	virtual void	Swap( CUtlVector< T > &array ) = 0;
	virtual void	Swap( int i, int j ) = 0; // redeclare so it doesn't get hidden
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
abstract_class IDmAttributeElementArray : public IDmAttributeTypedArray< CDmElementRef >
{
public:
	virtual void		SetTypeSymbol( UtlSymId_t typeSymbol ) = 0;
	virtual UtlSymId_t	GetTypeSymbol() const = 0;
};


//-----------------------------------------------------------------------------
// Set value helper functions
//-----------------------------------------------------------------------------
template< class T >
inline void DmAttributeSetValue( IDmAttribute *pAttribute, const T &value )
{
	IDmAttributeTyped<T> *pAttributeTyped = static_cast< IDmAttributeTyped<T>* >( pAttribute );
	pAttributeTyped->SetValue( value );
}

// TEST - to find where we're accidentally calling <DmElementHandle_t>
template<>
inline void DmAttributeSetValue( IDmAttribute *pAttribute, const DmElementHandle_t &value )
{
	Assert( 0 );
}

inline void DmAttributeSetValue( IDmAttribute *pAttribute, const char *value )
{
	IDmAttributeTyped<CUtlString> *pAttributeTyped = static_cast< IDmAttributeTyped<CUtlString>* >( pAttribute );

	int nLen = value ? Q_strlen( value ) + 1 : 0;
	CUtlString str( value, nLen );
	pAttributeTyped->SetValue( str );
}

inline void DmAttributeSetValue( IDmAttribute *pAttribute, const void *value, size_t size )
{
	IDmAttributeTyped< CUtlBinaryBlock > *pAttributeTyped = static_cast< IDmAttributeTyped< CUtlBinaryBlock >* >( pAttribute );
	pAttributeTyped->SetValue( CUtlBinaryBlock( value, size ) );
}

inline void DmAttributeElementSetValue( IDmAttribute *pAttribute, IDmElement *value )
{
	IDmAttributeTyped<CDmElementRef> *pAttributeTyped = static_cast< IDmAttributeTyped<CDmElementRef>* >( pAttribute );
	pAttributeTyped->SetValue( value ? value->GetHandle() : DMELEMENT_HANDLE_INVALID );
}


//-----------------------------------------------------------------------------
// Get value helper functions
//-----------------------------------------------------------------------------
template< class T >
inline const T& DmAttributeGetValue( const IDmAttribute *pAttribute )
{
	const IDmAttributeTyped<T> *pAttributeTyped = static_cast< const IDmAttributeTyped<T>* >( pAttribute );
	return pAttributeTyped->GetValue( );
}

template< class T >
inline const CUtlVector<T>& DmAttributeArrayGetValue( const IDmAttribute *pAttribute )
{
	Assert( pAttribute->GetType() == CDmAttributeInfo< CUtlVector<T> >::AttributeType() );

	const IDmAttributeTypedArray<T> *pAttributeTyped = static_cast< const IDmAttributeTypedArray<T>* >( pAttribute );
	return pAttributeTyped->GetValue( );
}

inline IDmElement *DmAttributeElementGetValue( const IDmAttribute *pAttribute )
{
	DmElementHandle_t hElement = DmAttributeGetValue<CDmElementRef>( pAttribute ).Get();
	return g_pDataModel->GetElement( hElement );
}

#endif // IDMATTRIBUTE_H
