//========= Copyright 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef DATAMAP_H
#define DATAMAP_H
#ifdef _WIN32
#pragma once
#endif

#ifndef VECTOR_H
#include "mathlib/vector.h"
#endif

#include "tier1/utlvector.h"

#include "tier0/memdbgon.h"

// SINGLE_INHERITANCE restricts the size of CBaseEntity pointers-to-member-functions to 4 bytes
class SINGLE_INHERITANCE CBaseEntity;
struct inputdata_t;

#define INVALID_TIME (FLT_MAX * -1.0) // Special value not rebased on save/load

enum class SpawnKeyType_t : uint8
{
	FIELD_VOID = 0,
	FIELD_FLOAT32,
	FIELD_STRING,
	FIELD_VECTOR,
	FIELD_QUATERNION,
	FIELD_INT32,
	FIELD_BOOLEAN,
	FIELD_INT16,
	FIELD_CHARACTER,
	FIELD_COLOR32,
	FIELD_EMBEDDED,
	FIELD_EHANDLE,
	FIELD_POSITION_VECTOR,
	FIELD_TIME,
	FIELD_TICK,
	FIELD_SOUNDNAME,
	FIELD_VECTOR2D,
	FIELD_INT64,
	FIELD_VECTOR4D,
	FIELD_UINT64,
	FIELD_UINT32,
	FIELD_UTLSTRINGTOKEN,
	FIELD_QANGLE,
	FIELD_NETWORK_ORIGIN_CELL_QUANTIZED_VECTOR,
	FIELD_HMATERIAL,
	FIELD_HMODEL,
	FIELD_NETWORK_QUANTIZED_VECTOR,
	FIELD_NETWORK_QUANTIZED_FLOAT,
	FIELD_DIRECTION_VECTOR_WORLDSPACE,
	FIELD_QANGLE_WORLDSPACE,
	FIELD_QUATERNION_WORLDSPACE,
	FIELD_UTLSTRING,
	FIELD_HRENDERTEXTURE,
	FIELD_HPARTICLESYSTEMDEFINITION,
	FIELD_UINT8,
	FIELD_UINT16,
	FIELD_HPOSTPROCESSING,
	FIELD_AMMO_INDEX,
	FIELD_MODIFIER_HANDLE,
	FIELD_HVDATA,
	FIELD_GLOBALSYMBOL,
	FIELD_NETWORK_QUANTIZED_VECTORWS,
	FIELD_NETWORK_ORIGIN_CELL_QUANTIZED_VECTORWS,

	FIELD_TYPECOUNT
};


//-----------------------------------------------------------------------------
// Field sizes... 
//-----------------------------------------------------------------------------
template <SpawnKeyType_t FIELD_TYPE>
class CDatamapFieldSizeDeducer
{
public:
	enum
	{
		SIZE = 0
	};

	static int FieldSize( )
	{
		return 0;
	}
};

#define DECLARE_FIELD_SIZE( _fieldType, _fieldSize )	\
	template< > class CDatamapFieldSizeDeducer<SpawnKeyType_t::_fieldType> { public: enum { SIZE = _fieldSize }; static int FieldSize() { return _fieldSize; } };
#define FIELD_SIZE( _fieldType )	CDatamapFieldSizeDeducer<SpawnKeyType_t::_fieldType>::SIZE
#define FIELD_BITS( _fieldType )	(FIELD_SIZE( _fieldType ) * 8)

DECLARE_FIELD_SIZE( FIELD_FLOAT32,		sizeof(float32))
DECLARE_FIELD_SIZE( FIELD_STRING,		sizeof(int))
DECLARE_FIELD_SIZE( FIELD_VECTOR,		3 * sizeof(float32))
DECLARE_FIELD_SIZE( FIELD_VECTOR2D,		2 * sizeof(float32))
DECLARE_FIELD_SIZE( FIELD_VECTOR4D,		4 * sizeof(float32))
DECLARE_FIELD_SIZE( FIELD_QUATERNION,	4 * sizeof(float32))
DECLARE_FIELD_SIZE( FIELD_INT32,		sizeof(int32))
DECLARE_FIELD_SIZE( FIELD_INT64,		sizeof(int64))
DECLARE_FIELD_SIZE( FIELD_BOOLEAN,		sizeof(char))
DECLARE_FIELD_SIZE( FIELD_INT16,		sizeof(int16))
DECLARE_FIELD_SIZE( FIELD_CHARACTER,	sizeof(char))
DECLARE_FIELD_SIZE( FIELD_COLOR32,		sizeof(int))
DECLARE_FIELD_SIZE( FIELD_EHANDLE,		sizeof(int))
DECLARE_FIELD_SIZE( FIELD_POSITION_VECTOR, 	3 * sizeof(float))
DECLARE_FIELD_SIZE( FIELD_TIME,			sizeof(float))
DECLARE_FIELD_SIZE( FIELD_TICK,			sizeof(int))
DECLARE_FIELD_SIZE( FIELD_SOUNDNAME,	sizeof(int))


#define ARRAYSIZE2D(p)		(sizeof(p)/sizeof(p[0][0]))
#define SIZE_OF_ARRAY(p)	_ARRAYSIZE(p)

#define _FIELD(name,fieldtype,count,flags,mapname)		{ SpawnKeyType_t::fieldtype, #name, offsetof(classNameTypedef, name), count, flags, mapname, NULL, sizeof( ((classNameTypedef *)0)->name ) }
#define DEFINE_FIELD_NULL	{ SpawnKeyType_t::FIELD_VOID,0,0,0,0,0,0,0}
#define DEFINE_FIELD(name,fieldtype)			_FIELD(name, fieldtype, 1,  FTYPEDESC_SAVE, NULL )
#define DEFINE_FIELD_NOT_SAVED(name,fieldtype)			_FIELD(name, fieldtype, 1, 0, NULL )

#define DEFINE_KEYFIELD(name,fieldtype, mapname) _FIELD(name, fieldtype, 1,  FTYPEDESC_KEY | FTYPEDESC_SAVE, mapname )
#define DEFINE_KEYFIELD_NOT_SAVED(name,fieldtype, mapname)_FIELD(name, fieldtype, 1,  FTYPEDESC_KEY, mapname )
#define DEFINE_AUTO_ARRAY(name,fieldtype)		_FIELD(name, fieldtype, SIZE_OF_ARRAY(((classNameTypedef *)0)->name), FTYPEDESC_SAVE, NULL )
#define DEFINE_AUTO_ARRAY_KEYFIELD(name,fieldtype,mapname)		_FIELD(name, fieldtype, SIZE_OF_ARRAY(((classNameTypedef *)0)->name), FTYPEDESC_SAVE, mapname )
#define DEFINE_ARRAY(name,fieldtype, count)		_FIELD(name, fieldtype, count, FTYPEDESC_SAVE, NULL )
#define DEFINE_ARRAY_NOT_SAVED(name,fieldtype, count)		_FIELD(name, fieldtype, count, 0, NULL )
#define DEFINE_GLOBAL_FIELD(name,fieldtype)	_FIELD(name, fieldtype, 1,  FTYPEDESC_GLOBAL | FTYPEDESC_SAVE, NULL )
#define DEFINE_GLOBAL_KEYFIELD(name,fieldtype, mapname)	_FIELD(name, fieldtype, 1,  FTYPEDESC_GLOBAL | FTYPEDESC_KEY | FTYPEDESC_SAVE, mapname )
#define DEFINE_AUTO_ARRAY2D(name,fieldtype)		_FIELD(name, fieldtype, ARRAYSIZE2D(((classNameTypedef *)0)->name), FTYPEDESC_SAVE, NULL )
// Used by byteswap datadescs
#define DEFINE_BITFIELD(name,fieldtype,bitcount)	DEFINE_ARRAY(name,fieldtype,((bitcount+FIELD_BITS(fieldtype)-1)&~(FIELD_BITS(fieldtype)-1)) / FIELD_BITS(fieldtype) )
#define DEFINE_INDEX(name,fieldtype)			_FIELD(name, fieldtype, 1,  FTYPEDESC_INDEX, NULL )

#define DEFINE_EMBEDDED( name )						\
	{ SpawnKeyType_t::FIELD_EMBEDDED, #name, offsetof(classNameTypedef, name), 1, FTYPEDESC_SAVE, NULL, &(((classNameTypedef *)0)->name.m_DataMap), sizeof( ((classNameTypedef *)0)->name ) }

#define DEFINE_EMBEDDED_OVERRIDE( name, overridetype )	\
	{ SpawnKeyType_t::FIELD_EMBEDDED, #name, offsetof(classNameTypedef, name), 1, FTYPEDESC_SAVE, NULL, &((overridetype *)0)->m_DataMap, sizeof( ((classNameTypedef *)0)->name ) }

#define DEFINE_EMBEDDEDBYREF( name )					\
	{ SpawnKeyType_t::FIELD_EMBEDDED, #name, offsetof(classNameTypedef, name), 1, FTYPEDESC_SAVE | FTYPEDESC_PTR, NULL, &(((classNameTypedef *)0)->name->m_DataMap), sizeof( *(((classNameTypedef *)0)->name) ) }

#define DEFINE_EMBEDDED_ARRAY( name, count )			\
	{ SpawnKeyType_t::FIELD_EMBEDDED, #name, offsetof(classNameTypedef, name), count, FTYPEDESC_SAVE, NULL, &(((classNameTypedef *)0)->name->m_DataMap), sizeof( ((classNameTypedef *)0)->name[0] ) }

#define DEFINE_EMBEDDED_AUTO_ARRAY( name )			\
	{ SpawnKeyType_t::FIELD_EMBEDDED, #name, offsetof(classNameTypedef, name), SIZE_OF_ARRAY( ((classNameTypedef *)0)->name ), FTYPEDESC_SAVE, NULL, &(((classNameTypedef *)0)->name->m_DataMap), sizeof( ((classNameTypedef *)0)->name[0] ) }

#ifndef NO_ENTITY_PREDICTION

// FTYPEDESC_KEY tells the prediction copy system to report the full nameof the field when reporting errors
#define DEFINE_PRED_TYPEDESCRIPTION( name, fieldtype )						\
	{ SpawnKeyType_t::FIELD_EMBEDDED, #name, offsetof(classNameTypedef, name), 1, FTYPEDESC_SAVE | FTYPEDESC_KEY, NULL, &fieldtype::m_PredMap }

#define DEFINE_PRED_TYPEDESCRIPTION_PTR( name, fieldtype )						\
	{ SpawnKeyType_t::FIELD_EMBEDDED, #name, offsetof(classNameTypedef, name), 1, FTYPEDESC_SAVE | FTYPEDESC_PTR | FTYPEDESC_KEY, NULL, &fieldtype::m_PredMap }

#else

#define DEFINE_PRED_TYPEDESCRIPTION( name, fieldtype )		DEFINE_FIELD_NULL
#define DEFINE_PRED_TYPEDESCRIPTION_PTR( name, fieldtype )	DEFINE_FIELD_NULL

#endif

// Extensions to datamap.h macros for predicted entities only
#define DEFINE_PRED_FIELD(name,fieldtype, flags)			_FIELD(name, fieldtype, 1,  flags, NULL )
#define DEFINE_PRED_ARRAY(name,fieldtype, count,flags)		_FIELD(name, fieldtype, count, flags, NULL )
#define DEFINE_FIELD_NAME(localname,netname,fieldtype)		_FIELD(localname, fieldtype, 1,  0, #netname )

//#define DEFINE_DATA( name, fieldextname, flags ) _FIELD(name, fieldtype, 1,  flags, extname )



#define FTYPEDESC_NONE				0

#define FTYPEDESC_GLOBAL			(1 << 0)		// This field is masked for global entity save/restore
#define FTYPEDESC_SAVE				(1 << 1)		// This field is saved to disk
#define FTYPEDESC_KEY				(1 << 2)		// This field can be requested and written to by string name at load time
#define FTYPEDESC_INPUT				(1 << 3)		// This field can be written to by string name at run time, and a function called
#define FTYPEDESC_OUTPUT			(1 << 4)		// This field propogates it's value to all targets whenever it changes
#define FTYPEDESC_FUNCTIONTABLE		(1 << 5)		// This is a table entry for a member function pointer
#define FTYPEDESC_PTR				(1 << 6)		// This field is a pointer, not an embedded object
#define FTYPEDESC_OVERRIDE			(1 << 7)		// The field is an override for one in a base class (only used by prediction system for now)

// Flags used by other systems (e.g., prediction system)
#define FTYPEDESC_INSENDTABLE		(1 << 8)		// This field is present in a network SendTable
#define FTYPEDESC_PRIVATE			(1 << 9)		// The field is local to the client or server only (not referenced by prediction code and not replicated by networking)
#define FTYPEDESC_NOERRORCHECK		(1 << 10)		// The field is part of the prediction typedescription, but doesn't get compared when checking for errors

#define FTYPEDESC_MODELINDEX		(1 << 11)		// The field is a model index (used for debugging output)

#define FTYPEDESC_INDEX				(1 << 12)		// The field is an index into file data, used for byteswapping. 

#define FTYPEDESC_OVERRIDE_RECURSIVE	(1 << 13)
#define FTYPEDESC_SCHEMA_INITIALIZED	(1 << 14)
#define FTYPEDESC_GEN_ARRAY_KEYNAMES_0	(1 << 15)
#define FTYPEDESC_GEN_ARRAY_KEYNAMES_1	(1 << 16)
#define FTYPEDESC_ADDITIONAL_FIELDS		(1 << 17)
#define FTYPEDESC_EXPLICIT_BASE			(1 << 18)
#define FTYPEDESC_PROCEDURAL_KEYFIELD	(1 << 19)
#define FTYPEDESC_ENUM					(1 << 20)	// Used if the typedesc is enum, no datamap_t info would be available
#define FTYPEDESC_REMOVED_KEYFIELD		(1 << 21)
#define FTYPEDESC_WAS_INPUT				(1 << 22)
#define FTYPEDESC_WAS_OUTPUT			(1 << 23)

#define TD_MSECTOLERANCE		0.001f		// This is a FIELD_FLOAT and should only be checked to be within 0.001 of the networked info

class ISaveRestoreOps;
class IPredictionCopyOps;

//
// Function prototype for all input handlers.
//
typedef void (CBaseEntity::*inputfunc_t)(inputdata_t &data);

struct datamap_t;

enum PredictionCopyType_t
{
	PC_NON_NETWORKED_ONLY = 0,
	PC_NETWORKED_ONLY,

	PC_COPYTYPE_COUNT,
	PC_EVERYTHING = PC_COPYTYPE_COUNT,
};

// Size: 0x38
struct typedescription_t
{
	SpawnKeyType_t		fieldType;
	const char			*fieldName;
	int					fieldOffset; // Local offset value
	unsigned short		fieldSize;
	int					flags;
	// the name of the variable in the map/fgd data, or the name of the action
	const char			*externalName;	

	// For embedding additional datatables inside this one
	union
	{
		datamap_t* td;
		const char* enumName;
	};

	// Stores the actual member variable size in bytes
	int					fieldSizeInBytes;
};

//-----------------------------------------------------------------------------
// Purpose: stores the list of objects in the hierarchy
//			used to iterate through an object's data descriptions
//-----------------------------------------------------------------------------
struct datamap_t
{
	typedescription_t	*dataDesc;
	int					dataNumFields;
	char const			*dataClassName;
	datamap_t			*baseMap;

#if defined( _DEBUG )
	bool				bValidityChecked;
#endif // _DEBUG
};


//-----------------------------------------------------------------------------
//
// Macros used to implement datadescs
//
#define DECLARE_FRIEND_DATADESC_ACCESS()	\
	template <typename T> friend void DataMapAccess(T *, datamap_t **p); \
	template <typename T> friend datamap_t *DataMapInit(T *);

#define DECLARE_SIMPLE_DATADESC() \
	static datamap_t m_DataMap; \
	static datamap_t *GetBaseMap(); \
	template <typename T> friend void DataMapAccess(T *, datamap_t **p); \
	template <typename T> friend datamap_t *DataMapInit(T *);

#define DECLARE_SIMPLE_DATADESC_INSIDE_NAMESPACE() \
	static datamap_t m_DataMap; \
	static datamap_t *GetBaseMap(); \
	template <typename T> friend void ::DataMapAccess(T *, datamap_t **p); \
	template <typename T> friend datamap_t *::DataMapInit(T *);

#define	DECLARE_DATADESC() \
	DECLARE_SIMPLE_DATADESC() \
	virtual datamap_t *GetDataDescMap( void );

#define BEGIN_DATADESC( className ) \
	datamap_t className::m_DataMap = { 0, 0, #className, NULL }; \
	datamap_t *className::GetDataDescMap( void ) { return &m_DataMap; } \
	datamap_t *className::GetBaseMap() { datamap_t *pResult; DataMapAccess((BaseClass *)NULL, &pResult); return pResult; } \
	BEGIN_DATADESC_GUTS( className )

#define BEGIN_DATADESC_NO_BASE( className ) \
	datamap_t className::m_DataMap = { 0, 0, #className, NULL }; \
	datamap_t *className::GetDataDescMap( void ) { return &m_DataMap; } \
	datamap_t *className::GetBaseMap() { return NULL; } \
	BEGIN_DATADESC_GUTS( className )

#define BEGIN_SIMPLE_DATADESC( className ) \
	datamap_t className::m_DataMap = { 0, 0, #className, NULL }; \
	datamap_t *className::GetBaseMap() { return NULL; } \
	BEGIN_DATADESC_GUTS( className )

#define BEGIN_SIMPLE_DATADESC_( className, BaseClass ) \
	datamap_t className::m_DataMap = { 0, 0, #className, NULL }; \
	datamap_t *className::GetBaseMap() { datamap_t *pResult; DataMapAccess((BaseClass *)NULL, &pResult); return pResult; } \
	BEGIN_DATADESC_GUTS( className )

#define BEGIN_DATADESC_GUTS( className ) \
	template <typename T> datamap_t *DataMapInit(T *); \
	template <> datamap_t *DataMapInit<className>( className * ); \
	namespace className##_DataDescInit \
	{ \
		datamap_t *g_DataMapHolder = DataMapInit( (className *)NULL ); /* This can/will be used for some clean up duties later */ \
	} \
	\
	template <> datamap_t *DataMapInit<className>( className * ) \
	{ \
		typedef className classNameTypedef; \
		static CDatadescGeneratedNameHolder nameHolder(#className); \
		className::m_DataMap.baseMap = className::GetBaseMap(); \
		static typedescription_t dataDesc[] = \
		{ \
		{ SpawnKeyType_t::FIELD_VOID,0,0,0,0,0,0,0}, /* so you can define "empty" tables */


#define BEGIN_DATADESC_GUTS_NAMESPACE( className, nameSpace ) \
	template <typename T> datamap_t *nameSpace::DataMapInit(T *); \
	template <> datamap_t *nameSpace::DataMapInit<className>( className * ); \
	namespace className##_DataDescInit \
	{ \
		datamap_t *g_DataMapHolder = nameSpace::DataMapInit( (className *)NULL ); /* This can/will be used for some clean up duties later */ \
	} \
	\
	template <> datamap_t *nameSpace::DataMapInit<className>( className * ) \
	{ \
		typedef className classNameTypedef; \
		static CDatadescGeneratedNameHolder nameHolder(#className); \
		className::m_DataMap.baseMap = className::GetBaseMap(); \
		static typedescription_t dataDesc[] = \
		{ \
		{ SpawnKeyType_t::FIELD_VOID,0,0,0,0,0,0,0}, /* so you can define "empty" tables */

#define END_DATADESC() \
		}; \
		\
		if ( sizeof( dataDesc ) > sizeof( dataDesc[0] ) ) \
		{ \
			classNameTypedef::m_DataMap.dataNumFields = SIZE_OF_ARRAY( dataDesc ) - 1; \
			classNameTypedef::m_DataMap.dataDesc 	  = &dataDesc[1]; \
		} \
		else \
		{ \
			classNameTypedef::m_DataMap.dataNumFields = 1; \
			classNameTypedef::m_DataMap.dataDesc 	  = dataDesc; \
		} \
		return &classNameTypedef::m_DataMap; \
	}

// used for when there is no data description
#define IMPLEMENT_NULL_SIMPLE_DATADESC( derivedClass ) \
	BEGIN_SIMPLE_DATADESC( derivedClass ) \
	END_DATADESC()

#define IMPLEMENT_NULL_SIMPLE_DATADESC_( derivedClass, baseClass ) \
	BEGIN_SIMPLE_DATADESC_( derivedClass, baseClass ) \
	END_DATADESC()

#define IMPLEMENT_NULL_DATADESC( derivedClass ) \
	BEGIN_DATADESC( derivedClass ) \
	END_DATADESC()

// helps get the offset of a bitfield
#define BEGIN_BITFIELD( name ) \
	union \
	{ \
		char name; \
		struct \
		{

#define END_BITFIELD() \
		}; \
	};

//-----------------------------------------------------------------------------
// Forward compatability with potential seperate byteswap datadescs

#define DECLARE_BYTESWAP_DATADESC() DECLARE_SIMPLE_DATADESC()
#define BEGIN_BYTESWAP_DATADESC(name) BEGIN_SIMPLE_DATADESC(name) 
#define BEGIN_BYTESWAP_DATADESC_(name,base) BEGIN_SIMPLE_DATADESC_(name,base) 
#define END_BYTESWAP_DATADESC() END_DATADESC()

//-----------------------------------------------------------------------------

template <typename T> 
inline void DataMapAccess(T *ignored, datamap_t **p)
{
	*p = &T::m_DataMap;
}

//-----------------------------------------------------------------------------

class CDatadescGeneratedNameHolder
{
public:
	CDatadescGeneratedNameHolder( const char *pszBase )
	 : m_pszBase(pszBase)
	{
		m_nLenBase = strlen( m_pszBase );
	}
	
	~CDatadescGeneratedNameHolder()
	{
		for ( int i = 0; i < m_Names.Count(); i++ )
		{
			delete m_Names[i];
		}
	}
	
	const char *GenerateName( const char *pszIdentifier )
	{
		char *pBuf = new char[m_nLenBase + strlen(pszIdentifier) + 1];
		strcpy( pBuf, m_pszBase );
		strcat( pBuf, pszIdentifier );
		m_Names.AddToTail( pBuf );
		return pBuf;
	}
	
private:
	const char *m_pszBase;
	size_t m_nLenBase;
	CUtlVector<char *> m_Names;
};

//-----------------------------------------------------------------------------

#include "tier0/memdbgoff.h"

#endif // DATAMAP_H
