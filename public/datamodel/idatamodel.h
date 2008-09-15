//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef IDATAMODEL_H
#define IDATAMODEL_H
#ifdef _WIN32
#pragma once
#endif

#include "tier1/interface.h"
#include "tier1/utlvector.h"
#include "tier1/utlsymbol.h"
#include "appframework/IAppSystem.h"

//-----------------------------------------------------------------------------
// Attribute chunk type enum
//-----------------------------------------------------------------------------
enum DmAttributeType_t
{
        AT_UNKNOWN = 0,

        AT_FIRST_VALUE_TYPE,

        AT_ELEMENT = AT_FIRST_VALUE_TYPE,
        AT_INT,
        AT_FLOAT,
        AT_BOOL,
        AT_STRING,
        AT_VOID,
        AT_OBJECTID,
        AT_COLOR, //rgba
        AT_VECTOR2,
        AT_VECTOR3,
        AT_VECTOR4,
        AT_QANGLE,
        AT_QUATERNION,
        AT_VMATRIX,

        AT_FIRST_ARRAY_TYPE,

        AT_ELEMENT_ARRAY = AT_FIRST_ARRAY_TYPE,
        AT_INT_ARRAY,
        AT_FLOAT_ARRAY,
        AT_BOOL_ARRAY,
        AT_STRING_ARRAY,
        AT_VOID_ARRAY,
        AT_OBJECTID_ARRAY,
        AT_COLOR_ARRAY,
        AT_VECTOR2_ARRAY,
        AT_VECTOR3_ARRAY,
        AT_VECTOR4_ARRAY,
        AT_QANGLE_ARRAY,
        AT_QUATERNION_ARRAY,
        AT_VMATRIX_ARRAY,

        AT_TYPE_COUNT,
};


#include "datamodel/idmelement.h"


//-----------------------------------------------------------------------------
// Forward declarations: 
//-----------------------------------------------------------------------------
class IDmElementInternal;
class IDmAttribute;
class IDmElement;
class IDmeOperator;
class IElementForKeyValueCallback;
struct DmObjectId_t;
struct DmValueBase_t;
class CUtlBuffer;
class KeyValues;
class CUtlSymbolTable;
class CUtlCharConversion;


//-----------------------------------------------------------------------------
// element framework phases
//-----------------------------------------------------------------------------
enum DmPhase_t
{
	PH_EDIT,
	PH_EDIT_APPLY,
	PH_EDIT_RESOLVE,
	PH_DEPENDENCY,
	PH_OPERATE,
	PH_OPERATE_RESOLVE,
	PH_OUTPUT,
};


//-----------------------------------------------------------------------------
// Handle to an IDmAttribute
//-----------------------------------------------------------------------------
enum DmAttributeHandle_t
{  
	DMATTRIBUTE_HANDLE_INVALID = 0xffffffff
};

//-----------------------------------------------------------------------------
// Handle to an IDmAttribute
//-----------------------------------------------------------------------------
enum DmElementIterator_t
{  
	DMELEMENT_ITERATOR_INVALID = 0xffffffff
};


//-----------------------------------------------------------------------------
// element framework interface
//-----------------------------------------------------------------------------
abstract_class IDmElementFramework : public IAppSystem
{
public:
	// Methods of IAppSystem
	virtual bool Connect( CreateInterfaceFn factory ) = 0;
	virtual void Disconnect() = 0;
	virtual void *QueryInterface( const char *pInterfaceName ) = 0;
	virtual InitReturnVal_t Init() = 0;
	virtual void Shutdown() = 0;

	virtual DmPhase_t GetPhase() = 0;

	virtual void SetOperators( const CUtlVector< IDmeOperator* > &operators ) = 0;

	virtual void BeginEdit() = 0; // ends in edit phase, forces apply/resolve if from edit phase
	virtual void Operate() = 0; // ends in output phase
};

extern IDmElementFramework *g_pDmElementFramework;

#define VDMELEMENTFRAMEWORK_VERSION	"VDmElementFrameworkVersion001"


//-----------------------------------------------------------------------------
// datamodel operator interface - for all elements that need to be sorted in the operator dependency graph
//-----------------------------------------------------------------------------
abstract_class IDmeOperator
{
public:
	virtual bool IsDirty() = 0; // ie needs to operate
	virtual void Operate() = 0;

	virtual void GetInputAttributes ( CUtlVector< IDmAttribute * > &attrs ) = 0;
	virtual void GetOutputAttributes( CUtlVector< IDmAttribute * > &attrs ) = 0;
};

//-----------------------------------------------------------------------------
// Method of calling back into a class which contains a DmeElement.
//-----------------------------------------------------------------------------
struct DmObjectId_t
{
	unsigned char m_Value[16];
};
	
inline bool operator ==( const DmObjectId_t& lhs, const DmObjectId_t& rhs )
{
	return !Q_memcmp( (void *)&lhs.m_Value[ 0 ], (void *)&rhs.m_Value[ 0 ], sizeof( lhs.m_Value ) );
}

//-----------------------------------------------------------------------------
// Class factory methods: 
//-----------------------------------------------------------------------------
class IDmElementFactory
{
public:
	// Creation, destruction
	virtual IDmElement* Create( DmElementHandle_t handle, const char *pElementType, const char *pElementName, const DmObjectId_t &id ) = 0;
	virtual void Destroy( DmElementHandle_t hElement ) = 0;
};


//-----------------------------------------------------------------------------
// Various serialization methods can be installed into the data model factory 
//-----------------------------------------------------------------------------
class IDmSerializer
{
public:
	// Does this format store its version type in the file?
	virtual bool StoresVersionInFile() const = 0;

	// Is this format a binary format?
	virtual bool IsBinaryFormat() const = 0;

	// Write into the UtlBuffer, return true if successful
	virtual bool Serialize( CUtlBuffer &buf, const char *pFormatName, IDmElementInternal *pRoot ) = 0;

	// Read from the UtlBuffer, return true if successful, and return the read-in root in ppRoot.
	// NOTE: The file name passed in is only for error messages
	virtual bool Unserialize( CUtlBuffer &buf, const char *pFormatName, const char *pFileName, IDmElementInternal **ppRoot ) = 0;

	// Converts pSourceRoot from pSourceFormatName to pDestFormatName, returns false if the conversion fails or is not a known conversion
	virtual bool ConvertFrom( IDmElementInternal *pSourceRoot, const char *pSourceFormatName, const char *pDestFormatName ) = 0;
};

//-----------------------------------------------------------------------------
// Interface for callbacks to supply element types for specific keys inside keyvalues files
//-----------------------------------------------------------------------------
class IElementForKeyValueCallback
{
public:
	virtual const char *GetElementForKeyValue( const char *pszKeyName, int iNestingLevel ) = 0;
};

//-----------------------------------------------------------------------------
// Purpose: Helper for debugging undo system
//-----------------------------------------------------------------------------
struct UndoInfo_t
{
	bool		terminator;
	char const	*desc;
	char const	*undo;
	char const	*redo;
	int			numoperations;
};


//-----------------------------------------------------------------------------
// handle to an undo context
//-----------------------------------------------------------------------------
enum DmUndoContext_t
{
	DMUNDO_CONTEXT_INVALID = 0xffffffff
};

enum DmClipboardContext_t
{
	DMCLIPBOARD_CONTEXT_INVALID = 0xffffffff
};

abstract_class IUndoElement
{
protected:
	explicit IUndoElement( char const *desc ) :
		m_pDesc( desc ),
		m_bEndOfStream( false )
	{
	}
public:
	bool				IsEndOfStream() const 
	{ 
		return m_bEndOfStream; 
	}

	virtual void		Undo() = 0;
	virtual void		Redo() = 0;

	virtual char const *UndoDesc() = 0;
	virtual char const *RedoDesc() = 0;

	virtual char const	*GetDesc()
	{
		return m_pDesc;
	}

	virtual void		Release() = 0;

protected:
	virtual				~IUndoElement() { }

private:
	void				SetEndOfStream( bool end )
	{ 
		m_bEndOfStream = end; 
	}

	bool				m_bEndOfStream;
	char const			*m_pDesc;

	friend class CUndoManager;
};

//-----------------------------------------------------------------------------
// Main interface for creation of all IDmeElements: 
//-----------------------------------------------------------------------------
#define VDATAMODEL_INTERFACE_VERSION	"VDataModelVersion001"
class IDataModel : public IAppSystem
{
public:	
	virtual IDmElementInternal	*CreateElementInternal() = 0;

	// Installs factories used to instance elements
	virtual void			AddElementFactory( const char *pElementTypeName, IDmElementFactory *pFactory ) = 0;

	// This factory will be used to instance all elements whose type name isn't found.
	virtual void			SetDefaultElementFactory( IDmElementFactory *pFactory ) = 0;

	virtual int				GetFirstFactory() const = 0;
	virtual int				GetNextFactory( int index ) const = 0;
	virtual bool			IsValidFactory( int index ) const = 0;
	virtual char const		*GetFactoryName( int index ) const = 0;

	// create/destroy element methods - proxies to installed element factories
	virtual DmElementHandle_t	CreateElement( CUtlSymbol typeSymbol, const char *pElementName = NULL, const DmObjectId_t *pObjectID = NULL ) = 0;
	virtual DmElementHandle_t	CreateElement( const char *pElementTypeName, const char *pElementName = NULL, const DmObjectId_t *pObjectID = NULL ) = 0;
	virtual void				DestroyElement( DmElementHandle_t hElement ) = 0;

	// element handle related methods
	virtual	IDmElement*			GetElement		    ( DmElementHandle_t hElement ) const = 0;
	virtual IDmElementInternal	*GetElementInternal ( DmElementHandle_t hElement ) const = 0;
	virtual const char*			GetElementType	    ( DmElementHandle_t hElement ) const = 0;
	virtual const char*			GetElementName	    ( DmElementHandle_t hElement ) const = 0;
	virtual const DmObjectId_t&	GetElementId	    ( DmElementHandle_t hElement ) const = 0;

	virtual const char		*GetAttributeNameForType( DmAttributeType_t attType ) const = 0;
	virtual DmAttributeType_t GetAttributeTypeForName( const char *name ) const = 0;

	// Methods related to object ids
	virtual void			CreateObjectId( DmObjectId_t *pDest ) const = 0;
	virtual void			Invalidate( DmObjectId_t *pDest ) const = 0;
	virtual bool			IsValid( const DmObjectId_t &id ) const = 0;
	virtual bool			IsEqual( const DmObjectId_t &id1, const DmObjectId_t &id2 ) const = 0;
	virtual void			ToString( const DmObjectId_t &id, char *pBuf, int nMaxLen ) const = 0;
	virtual bool			FromString( DmObjectId_t *pDest, const char *pBuf, int nMaxLen = 0 ) const = 0;
	virtual void			Copy( const DmObjectId_t &src, DmObjectId_t *pDest ) const = 0;
	virtual bool			Serialize( CUtlBuffer &buf, const DmObjectId_t &src ) const = 0;
	virtual bool			Unserialize( CUtlBuffer &buf, DmObjectId_t *pDest ) const = 0;

	// Adds various serializers
	virtual void			AddSerializer( const char *pFormatName, const char *pExtension, const char *pDescription, IDmSerializer *pSerializer ) = 0;

	// Gets at serializers
	virtual int				GetSerializerCount() const = 0;
	virtual const char *	GetSerializerName( int i ) const = 0;

	// Serialization of a element tree into a utlbuffer
	virtual bool			Serialize( CUtlBuffer &outBuf, const char *pFormatName, DmElementHandle_t hRoot ) = 0;

	// Unserializes, returns the root of the unserialized tree in hRoot 
	// NOTE: Format name is only used here for those formats which don't store
	// the format name in the file. Use NULL for those formats which store the 
	// format name in the file.
	// The file name passed in is simply for error messages
	virtual bool			Unserialize( CUtlBuffer &inBuf, const char *pFormatName, bool bCreateUnknownTypes, const char *pFileName, DmElementHandle_t &hRoot ) = 0;

	// Global symbol table for the datamodel system
	virtual UtlSymId_t		GetSymbol( const char *pString ) = 0;
	virtual const char *	GetString( UtlSymId_t sym ) const = 0;

	// Returns the total number of elements allocated at the moment
	virtual int				GetMaxNumberOfElements() const = 0;
	virtual int				GetElementsAllocatedSoFar() const = 0;
	virtual int				GetAllocatedAttributeCount() const = 0;
	virtual int				GetAllocatedElementCount() const = 0;
	virtual DmElementIterator_t	FirstAllocatedElement() const = 0;
	virtual DmElementIterator_t	NextAllocatedElement( DmElementIterator_t it ) const = 0;
	virtual IDmElement*		GetAllocatedElement( DmElementIterator_t it ) = 0;

	// Undo/Redo support
	virtual void			SetUndoEnabled( bool enable ) = 0;
	virtual bool			IsUndoEnabled() const = 0;
	virtual bool			IsDirty() const = 0;
	virtual bool			CanUndo() const = 0;
	virtual bool			CanRedo() const = 0;
	virtual void			StartUndo( char const *undodesc, char const *redodesc ) = 0;
	virtual void			FinishUndo() = 0;
	virtual void			AbortUndoableOperation() = 0; // called instead of FinishUndo, essentially performs and Undo() and WipeRedo() if any undo items have been added to the stack
	virtual void			ClearRedo() = 0;
	virtual char const		*GetUndoDesc() = 0;
	virtual char const		*GetRedoDesc() = 0;
	// From the UI, perform the Undo operation
	virtual void			Undo() = 0;
	virtual void			Redo() = 0;

	// Wipes out all Undo data
	virtual void			ClearUndo() = 0;

	virtual void			GetUndoInfo( CUtlVector< UndoInfo_t >& list ) = 0;

	// Creates/destroys undo contexts. All undo operations
	// are performed on the current undo context.
	virtual DmUndoContext_t	CreateUndoContext( int nMaxUndoDepth ) = 0;
	virtual void			DestroyUndoContext( DmUndoContext_t hContext ) = 0;

	// Sets a particular undo context to be the active one
	virtual void			SetUndoContext( DmUndoContext_t hContext ) = 0;

	virtual void AddUndoElement( IUndoElement *pElement ) = 0;
	virtual char const *GetUndoDescInternal( char const *context ) = 0;
	virtual char const *GetRedoDescInternal( char const *context ) = 0;

	// Gets at serializer info
	// FIXME: Move with the other serializer methods if we change interface version
	virtual const char *	GetSerializerExtension( int i ) const = 0;
	virtual const char *	GetSerializerDescription( int i ) const = 0;

	// Dme Clipboard operations

	// Creates/destroys clipboard contexts. All clipboard operations
	// are performed on the current context.
	virtual DmClipboardContext_t	CreateClipboardContext() = 0;
	virtual void			DestroyClipboardContext( DmClipboardContext_t hContext ) = 0;

	// Sets a particular undo context to be the active one
	virtual void			SetClipboardContext( DmClipboardContext_t hContext ) = 0;

	virtual void			EmptyClipboard() = 0;
	virtual void			SetClipboardData( CUtlVector< KeyValues * >& data ) = 0;
	virtual void			AddToClipboardData( KeyValues *add ) = 0;
	virtual void			GetClipboardData( CUtlVector< KeyValues * >& data ) = 0;

	virtual bool			IsSerializerBinary( const char *pFormatName ) const = 0;
	virtual int				MaxDmxHeaderSize() const = 0;
	virtual bool			GetSerializerType( CUtlBuffer &buf, char *pFormatName, int nMaxLen ) const = 0;
	virtual bool			DoesSerializerStoreVersionInFile( const char *pFormatName ) const = 0;

	// For serialization, set the delimiter rules
	// These methods are meant to be used by importer/exporters
	virtual void			SetSerializationDelimiter( CUtlCharConversion *pConv ) = 0;
	virtual void			SetSerializationArrayDelimiter( const char *pDelimiter ) = 0;

	// Handles to attributes
	virtual IDmAttribute *	GetAttribute( DmAttributeHandle_t h ) = 0;
	virtual bool			IsAttributeHandleValid( DmAttributeHandle_t h ) const = 0;

	// Always create elements, even if their type isn't found
	virtual void			CreateUnknownElements( bool bEnable ) = 0;

	// Finds a serializer by name
	virtual IDmSerializer*	FindSerializer( const char *pFormatName ) = 0;

	// Sets the name of the DME element to create in keyvalues serialization
	virtual void			SetKeyValuesElementCallback( IElementForKeyValueCallback *pCallbackInterface ) = 0;
	virtual const char		*GetKeyValuesElementName( const char *pszKeyName, int iNestingLevel ) = 0;
};


//-----------------------------------------------------------------------------
// Defined in tier1 lib, defined here also so you don't have to include tier1.h also
//-----------------------------------------------------------------------------
extern IDataModel *g_pDataModel;


class CUndoElementExternal : public IUndoElement
{
	typedef IUndoElement BaseClass;
public:
	CUndoElementExternal( IDataModel *dm, char const *desc )
		: BaseClass( desc )
	{
		m_UndoDesc = dm->GetUndoDescInternal( desc );
		m_RedoDesc = dm->GetRedoDescInternal( desc );
	}

	virtual char const *UndoDesc()
	{
		return m_UndoDesc.String();
	}

	virtual char const *RedoDesc()
	{
		return m_RedoDesc.String();
	}

	virtual void Release()
	{
		delete this;
	}

protected:
	CUtlSymbol	m_UndoDesc;
	CUtlSymbol	m_RedoDesc;
};

//-----------------------------------------------------------------------------
// Purpose: Simple helper class
//-----------------------------------------------------------------------------
class CUndoScopeGuard
{
public:
	explicit CUndoScopeGuard( IDataModel *dm, char const *udesc, char const *rdesc = NULL ) :
		m_pDm( dm )
	{
		if ( m_pDm )
		{
			m_pDm->StartUndo( udesc, rdesc ? rdesc : udesc );
		}
	}

	~CUndoScopeGuard()
	{
		if ( m_pDm )
		{
			m_pDm->FinishUndo();
		}
	}

	void	Release()
	{
		if ( m_pDm )
		{
			m_pDm->FinishUndo();
		}
		m_pDm = NULL;
	}

	void	Abort()
	{
		if ( m_pDm )
		{
			m_pDm->AbortUndoableOperation();
		}
		m_pDm = NULL;
	}

private:
	IDataModel	*m_pDm;
};

//-----------------------------------------------------------------------------
// Purpose: Simple helper class to disable Undo/Redo operations when in scope
//-----------------------------------------------------------------------------
class CChangeUndoScopeGuard
{
public:
	CChangeUndoScopeGuard( IDataModel *dm, bool newState ) :
		m_pDm( dm )
	{
		if ( m_pDm )
		{
			m_bOldValue = m_pDm->IsUndoEnabled();
			m_pDm->SetUndoEnabled( newState );
		}
	};
	~CChangeUndoScopeGuard()
	{
		if ( m_pDm )
		{
			m_pDm->SetUndoEnabled( m_bOldValue );
		}
	}

	void		Release()
	{
		// Releases the guard...
		if ( m_pDm )
		{
			m_pDm->SetUndoEnabled( m_bOldValue );
		}
		m_pDm = NULL;
	}

private:
	IDataModel	*m_pDm;
	bool		m_bOldValue;
};

class CDisableUndoScopeGuard : public CChangeUndoScopeGuard
{
	typedef CChangeUndoScopeGuard BaseClass;
public:
	CDisableUndoScopeGuard( IDataModel *dm ) : BaseClass( dm, false ) { }
};

class CEnableUndoScopeGuard : public CChangeUndoScopeGuard
{
	typedef CChangeUndoScopeGuard BaseClass;
public:
	CEnableUndoScopeGuard( IDataModel *dm ) : BaseClass( dm, true ) { }
};

#endif // IDATAMODEL_H
