//========== Copyright � 2008, Valve Corporation, All rights reserved. ========
//
// Purpose: VScript
//
// Overview
// --------
// VScript is an abstract binding layer that allows code to expose itself to 
// multiple scripting languages in a uniform format. Code can expose 
// functions, classes, and data to the scripting languages, and can also 
// call functions that reside in scripts.
// 
// Initializing
// ------------
// 
// To create a script virtual machine (VM), grab the global instance of 
// IScriptManager, call CreateVM, then call Init on the returned VM. Right 
// now you can have multiple VMs, but only VMs for a specific language.
// 
// Exposing functions and classes
// ------------------------------
// 
// To expose a C++ function to the scripting system, you just need to fill out a 
// description block. Using templates, the system will automatically deduce 
// all of the binding requirements (parameters and return values). Functions 
// are limited as to what the types of the parameters can be. See ScriptVariant_t.
// 
// 		extern IScriptVM *pScriptVM;
// 		bool Foo( int );
// 		void Bar();
// 		float FooBar( int, const char * );
// 		float OverlyTechnicalName( bool );
// 
// 		void RegisterFuncs()
// 		{
// 			ScriptRegisterFunction( pScriptVM, Foo );
// 			ScriptRegisterFunction( pScriptVM, Bar );
// 			ScriptRegisterFunction( pScriptVM, FooBar );
// 			ScriptRegisterFunctionNamed( pScriptVM, OverlyTechnicalName, "SimpleName" );
// 		}
// 
// 		class CMyClass
// 		{
// 		public:
// 			bool Foo( int );
// 			void Bar();
// 			float FooBar( int, const char * );
// 			float OverlyTechnicalName( bool );
// 		};
// 
// 		BEGIN_SCRIPTDESC_ROOT( CMyClass )
// 			DEFINE_SCRIPTFUNC( Foo )
// 			DEFINE_SCRIPTFUNC( Bar )
// 			DEFINE_SCRIPTFUNC( FooBar )
// 			DEFINE_SCRIPTFUNC_NAMED( OverlyTechnicalName, "SimpleMemberName" )
// 		END_SCRIPTDESC();
// 
// 		class CMyDerivedClass : public CMyClass
// 		{
// 		public:
// 			float DerivedFunc() const;
// 		};
// 
// 		BEGIN_SCRIPTDESC( CMyDerivedClass, CMyClass )
// 			DEFINE_SCRIPTFUNC( DerivedFunc )
// 		END_SCRIPTDESC();
// 
// 		CMyDerivedClass derivedInstance;
// 
// 		void AnotherFunction()
// 		{
// 			// Manual class exposure
// 			pScriptVM->RegisterClass( GetScriptDescForClass( CMyClass ) );
// 
// 			// Auto registration by instance
// 			pScriptVM->RegisterInstance( &derivedInstance, "theInstance" );
// 		}
// 
// Classes with "DEFINE_SCRIPT_CONSTRUCTOR()" in their description can be instanced within scripts
//
// Scopes
// ------
// Scripts can either be run at the global scope, or in a user defined scope. In the latter case,
// all "globals" within the script are actually in the scope. This can be used to bind private
// data spaces with C++ objects.
//
// Calling a function on a script
// ------------------------------
// Generally, use the "Call" functions. This example is the equivalent of DoIt("Har", 6.0, 99).
//
// 		hFunction = pScriptVM->LookupFunction( "DoIt", hScope );
// 		pScriptVM->Call( hFunction, hScope, true, NULL, "Har", 6.0, 99 );
// 
//=============================================================================

#ifndef IVSCRIPT_H
#define IVSCRIPT_H

#include "platform.h"
#include "datamap.h"
#include "appframework/IAppSystem.h"
#include "tier1/functors.h"
#include "tier0/memdbgon.h"

#if defined( _WIN32 )
#pragma once
#endif

#ifdef VSCRIPT_DLL_EXPORT
#define VSCRIPT_INTERFACE	DLL_EXPORT
#define VSCRIPT_OVERLOAD	DLL_GLOBAL_EXPORT
#define VSCRIPT_CLASS		DLL_CLASS_EXPORT
#else
#define VSCRIPT_INTERFACE	DLL_IMPORT
#define VSCRIPT_OVERLOAD	DLL_GLOBAL_IMPORT
#define VSCRIPT_CLASS		DLL_CLASS_IMPORT
#endif

class CUtlBuffer;
class CCommand;
class CCommandContext;

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

class IScriptVM;

enum ScriptLanguage_t
{
	SL_NONE,
	SL_LUA,

	SL_DEFAULT = SL_LUA
};

class IScriptDebugger
{
public:
	virtual bool		StartDebugging( const char *pszIDEKey ) = 0;
	virtual void		StopDebugging() = 0;
	virtual void		ConnectVM( void * ) = 0;
	virtual void		Update( void * ) = 0;
	virtual const char	*GetIDEKey() = 0;
	virtual void		HandleOutputMsg( const char *, void * ) = 0;
	virtual void		HandleErrorMsg( const char *, void * ) = 0;
};

class IScriptManager : public IAppSystem
{
public:
	virtual IScriptVM *CreateVM( ScriptLanguage_t language = SL_DEFAULT ) = 0;
	virtual void DestroyVM( IScriptVM * ) = 0;
	virtual IScriptDebugger *GetDebugger() = 0;
};

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------

DECLARE_POINTER_HANDLE( HSCRIPT );
#define INVALID_HSCRIPT ((HSCRIPT)-1)

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------

#include "variant.h"

typedef uint8 ScriptDataType_t;
typedef CVariant ScriptVariant_t;

#define SCRIPT_VARIANT_NULL ScriptVariant_t()

template <typename T> struct ScriptDeducer { /*enum { FIELD_TYPE = FIELD_TYPEUNKNOWN };*/ };
#define DECLARE_DEDUCE_FIELDTYPE( fieldType, type ) template<> struct ScriptDeducer<type> { enum { FIELD_TYPE = fieldType }; };

DECLARE_DEDUCE_FIELDTYPE( FIELD_VOID, void );
DECLARE_DEDUCE_FIELDTYPE( FIELD_FLOAT, float );
DECLARE_DEDUCE_FIELDTYPE( FIELD_CSTRING, const char * );
DECLARE_DEDUCE_FIELDTYPE( FIELD_CSTRING, char * );
DECLARE_DEDUCE_FIELDTYPE( FIELD_VECTOR, Vector );
DECLARE_DEDUCE_FIELDTYPE( FIELD_VECTOR, const Vector & );
DECLARE_DEDUCE_FIELDTYPE( FIELD_INTEGER, int );
DECLARE_DEDUCE_FIELDTYPE( FIELD_BOOLEAN, bool );
DECLARE_DEDUCE_FIELDTYPE( FIELD_CHARACTER, char );
DECLARE_DEDUCE_FIELDTYPE( FIELD_HSCRIPT, HSCRIPT );
DECLARE_DEDUCE_FIELDTYPE( FIELD_VARIANT, ScriptVariant_t );

#define ScriptDeduceType( T ) ScriptDeducer<T>::FIELD_TYPE

//---------------------------------------------------------

struct ScriptFuncDescriptor_t
{
	ScriptFuncDescriptor_t()
	{
		m_pszFunction = NULL;
		m_ReturnType = FIELD_TYPEUNKNOWN;
		m_iVariantCount = 0;
		m_iParamCount = 0;
		memset(m_Parameters, 0, sizeof(m_Parameters));
		m_pszDescription = NULL;
		m_pszParameterNames = NULL;
	}

	const char *m_pszScriptName;
	const char *m_pszFunction;
	const char *m_pszDescription;
	ScriptDataType_t m_ReturnType;
	uint8 m_iVariantCount;
	uint8 m_iParamCount;
	ScriptDataType_t m_Parameters[12];

	// Any/all parameter names. Read as a buffer of null-termed strings.
	// If first is NULL / 0-len, no parameter names are present.
	const char *m_pszParameterNames;
};


//---------------------------------------------------------

// Prefix a script description with this in order to not show the function or class in help
#define SCRIPT_HIDE "@"

// Prefix a script description of a class to indicate it is a singleton and the single instance should be in the help
#define SCRIPT_SINGLETON "!"

// Prefix a script description with this to indicate it should be represented using an alternate name
#define SCRIPT_ALIAS( alias, description ) "#" alias ":" description

//---------------------------------------------------------

enum ScriptFuncBindingFlags_t
{
	SF_MEMBER_FUNC	= 0x01,
};

typedef bool (*ScriptBindingFunc_t)( void *pFunction, void *pContext, ScriptVariant_t *pArguments, int nArguments, ScriptVariant_t *pReturn );

struct ScriptFunctionBinding_t
{
	ScriptFuncDescriptor_t	m_desc;
	ScriptBindingFunc_t		m_pfnBinding;
	void *					m_pFunction;
	unsigned				m_flags;
};

//---------------------------------------------------------
class IScriptInstanceHelper
{
public:
	virtual void *GetProxied( void *p )												{ return p; }
	virtual bool ToString( void *p, char *pBuf, int bufSize )						{ return false; }
	virtual void *BindOnRead( HSCRIPT hInstance, void *pOld, const char *pszId )	{ return NULL; }
};

//---------------------------------------------------------

struct ScriptClassDesc_t
{
	ScriptClassDesc_t() : m_pszScriptName( 0 ), m_pszClassname( 0 ), m_pszDescription( 0 ), m_pBaseDesc( 0 ), m_pfnConstruct( 0 ), m_pfnDestruct( 0 ), pHelper(NULL) {}

	const char *						m_pszScriptName;
	const char *						m_pszClassname;
	const char *						m_pszDescription;
	ScriptClassDesc_t *					m_pBaseDesc;
	CUtlVector<ScriptFunctionBinding_t> m_FunctionBindings;

	void *(*m_pfnConstruct)();
	void (*m_pfnDestruct)( void *);
	IScriptInstanceHelper *				pHelper; // optional helper
};

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------

#include "vscript_templates.h"

// Lower level macro primitives
#define ScriptInitFunctionBinding( pScriptFunction, func )									ScriptInitFunctionBindingNamed( pScriptFunction, func, #func )
#define ScriptInitFunctionBindingNamed( pScriptFunction, func, scriptName )					do { ScriptInitFuncDescriptorNamed( (&(pScriptFunction)->m_desc), func, scriptName ); (pScriptFunction)->m_pfnBinding = ScriptCreateBinding( &func ); (pScriptFunction)->m_pFunction = (void *)&func; } while (0)

#define ScriptInitMemberFunctionBinding( pScriptFunction, class, func )						ScriptInitMemberFunctionBinding_( pScriptFunction, class, func, #func )
#define ScriptInitMemberFunctionBindingNamed( pScriptFunction, class, func, scriptName )	ScriptInitMemberFunctionBinding_( pScriptFunction, class, func, scriptName )
#define ScriptInitMemberFunctionBinding_( pScriptFunction, class, func, scriptName ) 		do { ScriptInitMemberFuncDescriptor_( (&(pScriptFunction)->m_desc), class, func, scriptName ); (pScriptFunction)->m_pfnBinding = ScriptCreateBinding( ((class *)0), &class::func ); 	(pScriptFunction)->m_pFunction = ScriptConvertFuncPtrToVoid( &class::func ); (pScriptFunction)->m_flags = SF_MEMBER_FUNC;  } while (0)

#define ScriptInitClassDesc( pClassDesc, class, pBaseClassDesc )							ScriptInitClassDescNamed( pClassDesc, class, pBaseClassDesc, #class )
#define ScriptInitClassDescNamed( pClassDesc, class, pBaseClassDesc, scriptName )			ScriptInitClassDescNamed_( pClassDesc, class, pBaseClassDesc, scriptName )
#define ScriptInitClassDescNoBase( pClassDesc, class )										ScriptInitClassDescNoBaseNamed( pClassDesc, class, #class )
#define ScriptInitClassDescNoBaseNamed( pClassDesc, class, scriptName )						ScriptInitClassDescNamed_( pClassDesc, class, NULL, scriptName )
#define ScriptInitClassDescNamed_( pClassDesc, class, pBaseClassDesc, scriptName )			do { (pClassDesc)->m_pszScriptName = scriptName; (pClassDesc)->m_pszClassname = #class; (pClassDesc)->m_pBaseDesc = pBaseClassDesc; } while ( 0 )

#define ScriptAddFunctionToClassDesc( pClassDesc, class, func, description  )				ScriptAddFunctionToClassDescNamed( pClassDesc, class, func, #func, description )
#define ScriptAddFunctionToClassDescNamed( pClassDesc, class, func, scriptName, description ) do { ScriptFunctionBinding_t *pBinding = &((pClassDesc)->m_FunctionBindings[(pClassDesc)->m_FunctionBindings.AddToTail()]); pBinding->m_desc.m_pszDescription = description; ScriptInitMemberFunctionBindingNamed( pBinding, class, func, scriptName );  } while (0)

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------

#define ScriptRegisterFunction( pVM, func, description )									ScriptRegisterFunctionNamed( pVM, func, #func, description )
#define ScriptRegisterFunctionNamed( pVM, func, scriptName, description )					do { static ScriptFunctionBinding_t binding; binding.m_desc.m_pszDescription = description; binding.m_desc.m_Parameters.RemoveAll(); ScriptInitFunctionBindingNamed( &binding, func, scriptName ); pVM->RegisterFunction( &binding ); } while (0)

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------

#define ALLOW_SCRIPT_ACCESS() 																template <typename T> friend ScriptClassDesc_t *GetScriptDesc(T *);

#define BEGIN_SCRIPTDESC( className, baseClass, description )								BEGIN_SCRIPTDESC_NAMED( className, baseClass, #className, description )
#define BEGIN_SCRIPTDESC_ROOT( className, description )										BEGIN_SCRIPTDESC_ROOT_NAMED( className, #className, description )

#ifdef MSVC
	#define DEFINE_SCRIPTDESC_FUNCTION( className, baseClass ) \
		ScriptClassDesc_t * GetScriptDesc( className * )
#else
	#define DEFINE_SCRIPTDESC_FUNCTION( className, baseClass ) \
		template <> ScriptClassDesc_t * GetScriptDesc<baseClass>( baseClass *); \
		template <> ScriptClassDesc_t * GetScriptDesc<className>( className *)
#endif

#define BEGIN_SCRIPTDESC_NAMED( className, baseClass, scriptName, description ) \
	ScriptClassDesc_t g_##className##_ScriptDesc; \
	DEFINE_SCRIPTDESC_FUNCTION( className, baseClass ) \
	{ \
		static bool bInitialized; \
		if ( bInitialized ) \
		{ \
			return &g_##className##_ScriptDesc; \
		} \
		\
		bInitialized = true; \
		\
		typedef className _className; \
		ScriptClassDesc_t *pDesc = &g_##className##_ScriptDesc; \
		pDesc->m_pszDescription = description; \
		ScriptInitClassDescNamed( pDesc, className, GetScriptDescForClass( baseClass ), scriptName ); \
		ScriptClassDesc_t *pInstanceHelperBase = pDesc->m_pBaseDesc; \
		while ( pInstanceHelperBase ) \
		{ \
			if ( pInstanceHelperBase->pHelper ) \
			{ \
				pDesc->pHelper = pInstanceHelperBase->pHelper; \
				break; \
			} \
			pInstanceHelperBase = pInstanceHelperBase->m_pBaseDesc; \
		}


#define BEGIN_SCRIPTDESC_ROOT_NAMED( className, scriptName, description ) \
	BEGIN_SCRIPTDESC_NAMED( className, ScriptNoBase_t, scriptName, description )

#define END_SCRIPTDESC() \
		return pDesc; \
	}

#define DEFINE_SCRIPTFUNC( func, description )												DEFINE_SCRIPTFUNC_NAMED( func, #func, description )
#define DEFINE_SCRIPTFUNC_NAMED( func, scriptName, description )							ScriptAddFunctionToClassDescNamed( pDesc, _className, func, scriptName, description );
#define DEFINE_SCRIPT_CONSTRUCTOR()															ScriptAddConstructorToClassDesc( pDesc, _className );
#define DEFINE_SCRIPT_INSTANCE_HELPER( p )													pDesc->pHelper = (p);

template <typename T> ScriptClassDesc_t *GetScriptDesc(T *);

struct ScriptNoBase_t;
template <> inline ScriptClassDesc_t *GetScriptDesc<ScriptNoBase_t>( ScriptNoBase_t *) { return NULL; }

#define GetScriptDescForClass( className ) GetScriptDesc( ( className *)NULL )

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------

template <typename T>
class CScriptConstructor
{
public:
	static void *Construct()		{ return new T; }
	static void Destruct( void *p )	{ delete (T *)p; }
};

#define ScriptAddConstructorToClassDesc( pClassDesc, class )								do { (pClassDesc)->m_pfnConstruct = &CScriptConstructor<class>::Construct; (pClassDesc)->m_pfnDestruct = &CScriptConstructor<class>::Destruct; } while (0)

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------

enum ScriptErrorLevel_t
{
	SCRIPT_LEVEL_WARNING	= 0,
	SCRIPT_LEVEL_ERROR,
};

typedef void ( *ScriptOutputFunc_t )( const char *pszText );
typedef bool ( *ScriptErrorFunc_t )( ScriptErrorLevel_t eLevel, const char *pszText );

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------

#ifdef RegisterClass
#undef RegisterClass
#endif

enum ScriptStatus_t
{
	SCRIPT_ERROR = -1,
	SCRIPT_DONE,
	SCRIPT_RUNNING,
};

class CSquirrelMetamethodDelegateImpl;

class IScriptVM
{
public:
	virtual bool Init() = 0;
	virtual void Shutdown() = 0;

	virtual ScriptLanguage_t GetLanguage() = 0;
	virtual const char *GetLanguageName() = 0;
	
	virtual void *GetInternalVM() = 0;

	virtual void AddSearchPath( const char *pszSearchPath ) = 0;
	
	virtual void ClearTypeMap() = 0;
	
	virtual void EnableLocalDiskAccess() = 0;
	
	virtual void ForwardConsoleCommand(const CCommandContext &, const CCommand &) = 0;

	//--------------------------------------------------------
 
 	virtual bool Frame( float simTime ) = 0;

	//--------------------------------------------------------
	// Simple script usage
	//--------------------------------------------------------
	virtual ScriptStatus_t Run( const char *pszScript, bool bWait = true ) = 0;
	inline ScriptStatus_t Run( const unsigned char *pszScript, bool bWait = true ) { return Run( (char *)pszScript, bWait ); }

	//--------------------------------------------------------
	// Compilation
	//--------------------------------------------------------
 	virtual HSCRIPT CompileScript( const char *pszScript, const char *pszId = NULL ) = 0;
	inline HSCRIPT CompileScript( const unsigned char *pszScript, const char *pszId = NULL ) { return CompileScript( (char *)pszScript, pszId ); }
	virtual void ReleaseScript( HSCRIPT ) = 0;

	//--------------------------------------------------------
	// Execution of compiled
	//--------------------------------------------------------
	virtual ScriptStatus_t Run( HSCRIPT hScript, HSCRIPT hScope = NULL, bool bWait = true ) = 0;
	virtual ScriptStatus_t Run( HSCRIPT hScript, bool bWait ) = 0;

	//--------------------------------------------------------
	// Scope
	//--------------------------------------------------------
	virtual HSCRIPT CreateScope( const char *pszScope, HSCRIPT hParent = NULL ) = 0;
	virtual void ReferenceScope( HSCRIPT hScript ) = 0;
	virtual void ReleaseScope( HSCRIPT hScript ) = 0;

	//--------------------------------------------------------
	// Script functions
	//--------------------------------------------------------
	virtual HSCRIPT LookupFunction( const char *pszFunction, HSCRIPT hScope = NULL, bool raw = false ) = 0;
	virtual void ReleaseFunction( HSCRIPT hScript ) = 0;

	//--------------------------------------------------------
	// Script functions (raw, use Call())
	//--------------------------------------------------------
	virtual ScriptStatus_t ExecuteFunction( HSCRIPT hFunction, ScriptVariant_t *pArgs, int nArgs, ScriptVariant_t *pReturn, HSCRIPT hScope, bool bWait ) = 0;

	//--------------------------------------------------------
	// External functions
	//--------------------------------------------------------
	virtual void RegisterFunction( ScriptFunctionBinding_t *pScriptFunction ) = 0;

	//--------------------------------------------------------
	// External classes
	//--------------------------------------------------------
	virtual bool RegisterScriptClass( ScriptClassDesc_t *pClassDesc ) = 0;

	//--------------------------------------------------------
	// External instances. Note class will be auto-registered.
	//--------------------------------------------------------

	virtual HSCRIPT RegisterInstance( ScriptClassDesc_t *pDesc, void *pInstance ) = 0;
	virtual void SetInstanceUniqeId( HSCRIPT hInstance, const char *pszId ) = 0;
	template <typename T> HSCRIPT RegisterInstance( T *pInstance )																	{ return RegisterInstance( GetScriptDesc( pInstance ), pInstance );	}
	template <typename T> HSCRIPT RegisterInstance( T *pInstance, const char *pszInstance, HSCRIPT hScope = NULL)					{ HSCRIPT hInstance = RegisterInstance( GetScriptDesc( pInstance ), pInstance ); SetValue( hScope, pszInstance, hInstance ); return hInstance; }
	virtual void RemoveInstance( HSCRIPT ) = 0;
	void RemoveInstance( HSCRIPT hInstance, const char *pszInstance, HSCRIPT hScope = NULL )										{ ClearValue( hScope, pszInstance ); RemoveInstance( hInstance ); }
	void RemoveInstance( const char *pszInstance, HSCRIPT hScope = NULL )															{ ScriptVariant_t val; if ( GetValue( hScope, pszInstance, &val ) ) { if ( val.m_type == FIELD_HSCRIPT ) { RemoveInstance( val, pszInstance, hScope ); } ReleaseValue( val ); } }

	virtual void *GetInstanceValue( HSCRIPT hInstance, ScriptClassDesc_t *pExpectedType = NULL ) = 0;

	//----------------------------------------------------------------------------

	virtual bool GenerateUniqueKey( const char *pszRoot, char *pBuf, int nBufSize ) = 0;

	//----------------------------------------------------------------------------

	virtual bool ValueExists( HSCRIPT hScope, const char *pszKey ) = 0;
	bool ValueExists( const char *pszKey )																							{ return ValueExists( NULL, pszKey ); }

	virtual bool SetValue( HSCRIPT hScope, const char *pszKey, const char *pszValue ) = 0;
	virtual bool SetValue( HSCRIPT hScope, const char *pszKey, const ScriptVariant_t &value ) = 0;
	virtual bool SetValue( HSCRIPT hScope, int nIndex, const ScriptVariant_t &value ) = 0;

	bool SetValue( const char *pszKey, const ScriptVariant_t &value )																{ return SetValue(NULL, pszKey, value ); }

	virtual bool SetEnumValue(HSCRIPT hScope, const char *pszEnumName, const char *pszValueName, int value, const char *pszDescription) = 0;
	virtual bool CreateKeyValuesFromTable(HSCRIPT hScope, const char* unk1, void* fUnk, void* unk2) = 0;

	virtual void CreateTable( ScriptVariant_t &Table ) = 0;
	virtual bool IsTable( HSCRIPT hScope ) = 0;
	virtual int	GetNumTableEntries( HSCRIPT hScope ) = 0;
	virtual int GetNumElements( HSCRIPT hScope ) = 0;
	virtual int GetKeyValue( HSCRIPT hScope, int nIterator, ScriptVariant_t *pKey, ScriptVariant_t *pValue ) = 0;

	virtual bool GetValue( HSCRIPT hScope, const char *pszKey, ScriptVariant_t *pValue ) = 0;
	virtual bool GetValue( HSCRIPT hScope, int nIndex, ScriptVariant_t *pValue ) = 0;
	bool GetValue( const char *pszKey, ScriptVariant_t *pValue )																	{ return GetValue(NULL, pszKey, pValue ); }
	virtual bool GetScalarValue( HSCRIPT hScope, ScriptVariant_t *pValue ) = 0;
	virtual void ReleaseValue( ScriptVariant_t &value ) = 0;

	virtual bool ClearValue( HSCRIPT hScope, const char *pszKey ) = 0;
	bool ClearValue( const char *pszKey)																							{ return ClearValue( NULL, pszKey ); }
	
	virtual HSCRIPT CreateArray( ScriptVariant_t & ) = 0;
	virtual bool IsArray( HSCRIPT hScope ) = 0;
	virtual int GetArrayCount( HSCRIPT hScope ) = 0;
	virtual void ArrayAddToTail( HSCRIPT hScope, const ScriptVariant_t &pValue ) = 0;
	
	//----------------------------------------------------------------------------

	virtual void WriteState( CUtlBuffer *pBuffer ) = 0;
	virtual void ReadState( CUtlBuffer *pBuffer ) = 0;
	
	virtual void CollectGarbage( const char *, bool ) = 0;

	virtual void DumpState() = 0;

	virtual void SetOutputCallback( ScriptOutputFunc_t pFunc ) = 0;
	virtual void SetErrorCallback( ScriptErrorFunc_t pFunc ) = 0;

	//----------------------------------------------------------------------------

	virtual bool RaiseException( const char *pszExceptionText ) = 0;

	virtual HSCRIPT GetRootTable() = 0;

	virtual HSCRIPT CopyHandle( HSCRIPT hScope ) = 0;

	virtual int GetIdentity( HSCRIPT hScope ) = 0;

	class ISquirrelMetamethodDelegate;

	virtual void *MakeSquirrelMetamethod_Get( HSCRIPT&, const char*, ISquirrelMetamethodDelegate *, bool ) = 0;
	virtual void DestroySquirrelMetamethod_Get( CSquirrelMetamethodDelegateImpl * ) = 0;

	virtual int GetKeyValue2( HSCRIPT hScope, int iterator, ScriptVariant_t *pKey, ScriptVariant_t *pValue ) = 0;

	//----------------------------------------------------------------------------
	// Call API
	//
	// Note for string and vector return types, the caller must delete the pointed to memory
	//----------------------------------------------------------------------------
	ScriptStatus_t Call( HSCRIPT hFunction, HSCRIPT hScope = NULL, bool bWait = true, ScriptVariant_t *pReturn = NULL )
	{
		return ExecuteFunction( hFunction, NULL, 0, pReturn, hScope, bWait );
	}

	template <typename ARG_TYPE_1>
	ScriptStatus_t Call( HSCRIPT hFunction, HSCRIPT hScope, bool bWait, ScriptVariant_t *pReturn, ARG_TYPE_1 arg1 )
	{
		ScriptVariant_t args[1]; args[0] = arg1;
		return ExecuteFunction( hFunction, args, Q_ARRAYSIZE(args), pReturn, hScope, bWait );
	}

	template <typename ARG_TYPE_1, typename ARG_TYPE_2>
	ScriptStatus_t Call( HSCRIPT hFunction, HSCRIPT hScope, bool bWait, ScriptVariant_t *pReturn, ARG_TYPE_1 arg1, ARG_TYPE_2 arg2 )
	{
		ScriptVariant_t args[2]; args[0] = arg1; args[1] = arg2;
		return ExecuteFunction( hFunction, args, Q_ARRAYSIZE(args), pReturn, hScope, bWait );
	}

	template <typename ARG_TYPE_1, typename ARG_TYPE_2, typename	ARG_TYPE_3>
	ScriptStatus_t Call( HSCRIPT hFunction, HSCRIPT hScope, bool bWait, ScriptVariant_t *pReturn, ARG_TYPE_1 arg1, ARG_TYPE_2 arg2, ARG_TYPE_3 arg3 )
	{
		ScriptVariant_t args[3]; args[0] = arg1; args[1] = arg2; args[2] = arg3;
		return ExecuteFunction( hFunction, args, Q_ARRAYSIZE(args), pReturn, hScope, bWait );
	}

	template <typename ARG_TYPE_1, typename ARG_TYPE_2, typename	ARG_TYPE_3,	typename ARG_TYPE_4>
	ScriptStatus_t Call( HSCRIPT hFunction, HSCRIPT hScope, bool bWait, ScriptVariant_t *pReturn, ARG_TYPE_1 arg1, ARG_TYPE_2 arg2, ARG_TYPE_3 arg3, ARG_TYPE_4 arg4 )
	{
		ScriptVariant_t args[4]; args[0] = arg1; args[1] = arg2; args[2] = arg3; args[3] = arg4;
		return ExecuteFunction( hFunction, args, Q_ARRAYSIZE(args), pReturn, hScope, bWait );
	}

	template <typename ARG_TYPE_1, typename ARG_TYPE_2, typename	ARG_TYPE_3,	typename ARG_TYPE_4, typename ARG_TYPE_5>
	ScriptStatus_t Call( HSCRIPT hFunction, HSCRIPT hScope, bool bWait, ScriptVariant_t *pReturn, ARG_TYPE_1 arg1, ARG_TYPE_2 arg2, ARG_TYPE_3 arg3, ARG_TYPE_4 arg4, ARG_TYPE_5 arg5 )
	{
		ScriptVariant_t args[5]; args[0] = arg1; args[1] = arg2; args[2] = arg3; args[3] = arg4; args[4] = arg5;
		return ExecuteFunction( hFunction, args, Q_ARRAYSIZE(args), pReturn, hScope, bWait );
	}

	template <typename ARG_TYPE_1, typename ARG_TYPE_2, typename	ARG_TYPE_3,	typename ARG_TYPE_4, typename ARG_TYPE_5, typename ARG_TYPE_6>
	ScriptStatus_t Call( HSCRIPT hFunction, HSCRIPT hScope, bool bWait, ScriptVariant_t *pReturn, ARG_TYPE_1 arg1, ARG_TYPE_2 arg2, ARG_TYPE_3 arg3, ARG_TYPE_4 arg4, ARG_TYPE_5 arg5, ARG_TYPE_6 arg6 )
	{
		ScriptVariant_t args[6]; args[0] = arg1; args[1] = arg2; args[2] = arg3; args[3] = arg4; args[4] = arg5; args[5] = arg6;
		return ExecuteFunction( hFunction, args, Q_ARRAYSIZE(args), pReturn, hScope, bWait );
	}

	template <typename ARG_TYPE_1, typename ARG_TYPE_2, typename	ARG_TYPE_3,	typename ARG_TYPE_4, typename ARG_TYPE_5, typename ARG_TYPE_6, typename ARG_TYPE_7>
	ScriptStatus_t Call( HSCRIPT hFunction, HSCRIPT hScope, bool bWait, ScriptVariant_t *pReturn, ARG_TYPE_1 arg1, ARG_TYPE_2 arg2, ARG_TYPE_3 arg3, ARG_TYPE_4 arg4, ARG_TYPE_5 arg5, ARG_TYPE_6 arg6, ARG_TYPE_7 arg7 )
	{
		ScriptVariant_t args[7]; args[0] = arg1; args[1] = arg2; args[2] = arg3; args[3] = arg4; args[4] = arg5; args[5] = arg6; args[6] = arg7;
		return ExecuteFunction( hFunction, args, Q_ARRAYSIZE(args), pReturn, hScope, bWait );
	}

	template <typename ARG_TYPE_1, typename ARG_TYPE_2, typename	ARG_TYPE_3,	typename ARG_TYPE_4, typename ARG_TYPE_5, typename ARG_TYPE_6, typename ARG_TYPE_7, typename ARG_TYPE_8>
	ScriptStatus_t Call( HSCRIPT hFunction, HSCRIPT hScope, bool bWait, ScriptVariant_t *pReturn, ARG_TYPE_1 arg1, ARG_TYPE_2 arg2, ARG_TYPE_3 arg3, ARG_TYPE_4 arg4, ARG_TYPE_5 arg5, ARG_TYPE_6 arg6, ARG_TYPE_7 arg7, ARG_TYPE_8 arg8 )
	{
		ScriptVariant_t args[8]; args[0] = arg1; args[1] = arg2; args[2] = arg3; args[3] = arg4; args[4] = arg5; args[5] = arg6; args[6] = arg7; args[7] = arg8;
		return ExecuteFunction( hFunction, args, Q_ARRAYSIZE(args), pReturn, hScope, bWait );
	}

	template <typename ARG_TYPE_1, typename ARG_TYPE_2, typename	ARG_TYPE_3,	typename ARG_TYPE_4, typename ARG_TYPE_5, typename ARG_TYPE_6, typename ARG_TYPE_7, typename ARG_TYPE_8, typename ARG_TYPE_9>
	ScriptStatus_t Call( HSCRIPT hFunction, HSCRIPT hScope, bool bWait, ScriptVariant_t *pReturn, ARG_TYPE_1 arg1, ARG_TYPE_2 arg2, ARG_TYPE_3 arg3, ARG_TYPE_4 arg4, ARG_TYPE_5 arg5, ARG_TYPE_6 arg6, ARG_TYPE_7 arg7, ARG_TYPE_8 arg8, ARG_TYPE_9 arg9 )
	{
		ScriptVariant_t args[9]; args[0] = arg1; args[1] = arg2; args[2] = arg3; args[3] = arg4; args[4] = arg5; args[5] = arg6; args[6] = arg7; args[7] = arg8; args[8] = arg9;
		return ExecuteFunction( hFunction, args, Q_ARRAYSIZE(args), pReturn, hScope, bWait );
	}

	template <typename ARG_TYPE_1, typename ARG_TYPE_2, typename	ARG_TYPE_3,	typename ARG_TYPE_4, typename ARG_TYPE_5, typename ARG_TYPE_6, typename ARG_TYPE_7, typename ARG_TYPE_8, typename ARG_TYPE_9, typename ARG_TYPE_10>
	ScriptStatus_t Call( HSCRIPT hFunction, HSCRIPT hScope, bool bWait, ScriptVariant_t *pReturn, ARG_TYPE_1 arg1, ARG_TYPE_2 arg2, ARG_TYPE_3 arg3, ARG_TYPE_4 arg4, ARG_TYPE_5 arg5, ARG_TYPE_6 arg6, ARG_TYPE_7 arg7, ARG_TYPE_8 arg8, ARG_TYPE_9 arg9, ARG_TYPE_10 arg10 )
	{
		ScriptVariant_t args[10]; args[0] = arg1; args[1] = arg2; args[2] = arg3; args[3] = arg4; args[4] = arg5; args[5] = arg6; args[6] = arg7; args[7] = arg8; args[8] = arg9; args[9] = arg10;
		return ExecuteFunction( hFunction, args, Q_ARRAYSIZE(args), pReturn, hScope, bWait );
	}

	template <typename ARG_TYPE_1, typename ARG_TYPE_2, typename	ARG_TYPE_3,	typename ARG_TYPE_4, typename ARG_TYPE_5, typename ARG_TYPE_6, typename ARG_TYPE_7, typename ARG_TYPE_8, typename ARG_TYPE_9, typename ARG_TYPE_10, typename ARG_TYPE_11>
	ScriptStatus_t Call( HSCRIPT hFunction, HSCRIPT hScope, bool bWait, ScriptVariant_t *pReturn, ARG_TYPE_1 arg1, ARG_TYPE_2 arg2, ARG_TYPE_3 arg3, ARG_TYPE_4 arg4, ARG_TYPE_5 arg5, ARG_TYPE_6 arg6, ARG_TYPE_7 arg7, ARG_TYPE_8 arg8, ARG_TYPE_9 arg9, ARG_TYPE_10 arg10, ARG_TYPE_11 arg11 )
	{
		ScriptVariant_t args[11]; args[0] = arg1; args[1] = arg2; args[2] = arg3; args[3] = arg4; args[4] = arg5; args[5] = arg6; args[6] = arg7; args[7] = arg8; args[8] = arg9; args[9] = arg10; args[10] = arg11;
		return ExecuteFunction( hFunction, args, Q_ARRAYSIZE(args), pReturn, hScope, bWait );
	}

	template <typename ARG_TYPE_1, typename ARG_TYPE_2, typename	ARG_TYPE_3,	typename ARG_TYPE_4, typename ARG_TYPE_5, typename ARG_TYPE_6, typename ARG_TYPE_7, typename ARG_TYPE_8, typename ARG_TYPE_9, typename ARG_TYPE_10, typename ARG_TYPE_11, typename ARG_TYPE_12>
	ScriptStatus_t Call( HSCRIPT hFunction, HSCRIPT hScope, bool bWait, ScriptVariant_t *pReturn, ARG_TYPE_1 arg1, ARG_TYPE_2 arg2, ARG_TYPE_3 arg3, ARG_TYPE_4 arg4, ARG_TYPE_5 arg5, ARG_TYPE_6 arg6, ARG_TYPE_7 arg7, ARG_TYPE_8 arg8, ARG_TYPE_9 arg9, ARG_TYPE_10 arg10, ARG_TYPE_11 arg11, ARG_TYPE_12 arg12 )
	{
		ScriptVariant_t args[12]; args[0] = arg1; args[1] = arg2; args[2] = arg3; args[3] = arg4; args[4] = arg5; args[5] = arg6; args[6] = arg7; args[7] = arg8; args[8] = arg9; args[9] = arg10; args[10] = arg11; args[11] = arg12;
		return ExecuteFunction( hFunction, args, Q_ARRAYSIZE(args), pReturn, hScope, bWait );
	}
};


//-----------------------------------------------------------------------------
// Script scope helper class
//-----------------------------------------------------------------------------

class CDefScriptScopeBase
{
public:
	static IScriptVM *GetVM()
	{
		extern IScriptVM *g_pScriptVM;
		return g_pScriptVM;
	}
};

template <class BASE_CLASS = CDefScriptScopeBase>
class CScriptScopeT : public CDefScriptScopeBase
{
public:
	CScriptScopeT() :
		m_hScope( INVALID_HSCRIPT ),
		m_flags( 0 )
	{
	}

	~CScriptScopeT()
	{
		Term();
	}

	bool IsInitialized()
	{
		return m_hScope != INVALID_HSCRIPT;
	}

	bool Init( const char *pszName )
	{
		m_hScope = GetVM()->CreateScope( pszName );
		return ( m_hScope != NULL );
	}

	bool Init( HSCRIPT hScope, bool bExternal = true )
	{
		if ( bExternal )
		{
			m_flags |= EXTERNAL;
		}
		m_hScope = hScope;
		return ( m_hScope != NULL );
	}

	bool InitGlobal()
	{
		Assert( 0 ); // todo [3/24/2008 tom]
		m_hScope = GetVM()->CreateScope( "" );
		return ( m_hScope != NULL );
	}

	void Term()
	{
		if ( m_hScope != INVALID_HSCRIPT )
		{
			IScriptVM *pVM = GetVM();
			if ( pVM )
			{
				for ( int i = 0; i < m_FuncHandles.Count(); i++ )
				{
					pVM->ReleaseFunction( *m_FuncHandles[i] );
				}
			}
			m_FuncHandles.Purge();
			if ( m_hScope && pVM && !(m_flags & EXTERNAL) )
			{
				pVM->ReleaseScope( m_hScope );
			}
			m_hScope = INVALID_HSCRIPT;
		}
		m_flags = 0;
	}

	void InvalidateCachedValues()
	{
		IScriptVM *pVM = GetVM();
		for ( int i = 0; i < m_FuncHandles.Count(); i++ )
		{
			if ( *m_FuncHandles[i] )
				pVM->ReleaseFunction( *m_FuncHandles[i] );
			*m_FuncHandles[i] = INVALID_HSCRIPT;
		}
		m_FuncHandles.RemoveAll();
	}

	operator HSCRIPT()
	{
		return ( m_hScope != INVALID_HSCRIPT ) ? m_hScope : NULL;
	}

	bool ValueExists( const char *pszKey )																							{ return GetVM()->ValueExists( m_hScope, pszKey ); }
	bool SetValue( const char *pszKey, const ScriptVariant_t &value )																{ return GetVM()->SetValue(m_hScope, pszKey, value ); }
	bool GetValue( const char *pszKey, ScriptVariant_t *pValue )																	{ return GetVM()->GetValue(m_hScope, pszKey, pValue ); }
	void ReleaseValue( ScriptVariant_t &value )																						{ GetVM()->ReleaseValue( value ); }
	bool ClearValue( const char *pszKey)																							{ return GetVM()->ClearValue( m_hScope, pszKey ); }

	ScriptStatus_t Run( HSCRIPT hScript )
	{
		InvalidateCachedValues();
		return GetVM()->Run( hScript, m_hScope );
	}

	ScriptStatus_t Run( const char *pszScriptText, const char *pszScriptName = NULL )
	{
		InvalidateCachedValues();
		HSCRIPT hScript = GetVM()->CompileScript( pszScriptText, pszScriptName );
		if ( hScript )
		{
			ScriptStatus_t result = GetVM()->Run( hScript, m_hScope );
			GetVM()->ReleaseScript( hScript );
			return result; 
		}
		return SCRIPT_ERROR;
	}

	ScriptStatus_t Run( const unsigned char *pszScriptText, const char *pszScriptName = NULL )
	{
		return Run( (const char *)pszScriptText, pszScriptName);
	}

	HSCRIPT LookupFunction( const char *pszFunction )
	{
		return GetVM()->LookupFunction( pszFunction, m_hScope );
	}

	void ReleaseFunction( HSCRIPT hScript )
	{
		GetVM()->ReleaseFunction( hScript );
	}

	bool FunctionExists( const char *pszFunction )
	{
		HSCRIPT hFunction = GetVM()->LookupFunction( pszFunction, m_hScope );
		GetVM()->ReleaseFunction( hFunction );
		return ( hFunction != NULL ) ;
	}

	//-----------------------------------------------------

	enum Flags_t
	{
		EXTERNAL = 0x01,
	};

	//-----------------------------------------------------

	ScriptStatus_t Call( HSCRIPT hFunction, ScriptVariant_t *pReturn = NULL )
	{
		return GetVM()->ExecuteFunction( hFunction, NULL, 0, pReturn, m_hScope, true );
	}

	template <typename ARG_TYPE_1>
	ScriptStatus_t Call( HSCRIPT hFunction, ScriptVariant_t *pReturn, ARG_TYPE_1 arg1 )
	{
		ScriptVariant_t args[1]; args[0] = arg1;
		return GetVM()->ExecuteFunction( hFunction, args, Q_ARRAYSIZE(args), pReturn, m_hScope, true );
	}

	template <typename ARG_TYPE_1, typename ARG_TYPE_2>
	ScriptStatus_t Call( HSCRIPT hFunction, ScriptVariant_t *pReturn, ARG_TYPE_1 arg1, ARG_TYPE_2 arg2 )
	{
		ScriptVariant_t args[2]; args[0] = arg1; args[1] = arg2;
		return GetVM()->ExecuteFunction( hFunction, args, Q_ARRAYSIZE(args), pReturn, m_hScope, true );
	}

	template <typename ARG_TYPE_1, typename ARG_TYPE_2, typename	ARG_TYPE_3>
	ScriptStatus_t Call( HSCRIPT hFunction, ScriptVariant_t *pReturn, ARG_TYPE_1 arg1, ARG_TYPE_2 arg2, ARG_TYPE_3 arg3 )
	{
		ScriptVariant_t args[3]; args[0] = arg1; args[1] = arg2; args[2] = arg3;
		return GetVM()->ExecuteFunction( hFunction, args, Q_ARRAYSIZE(args), pReturn, m_hScope, true );
	}

	template <typename ARG_TYPE_1, typename ARG_TYPE_2, typename	ARG_TYPE_3,	typename ARG_TYPE_4>
	ScriptStatus_t Call( HSCRIPT hFunction, ScriptVariant_t *pReturn, ARG_TYPE_1 arg1, ARG_TYPE_2 arg2, ARG_TYPE_3 arg3, ARG_TYPE_4 arg4 )
	{
		ScriptVariant_t args[4]; args[0] = arg1; args[1] = arg2; args[2] = arg3; args[3] = arg4;
		return GetVM()->ExecuteFunction( hFunction, args, Q_ARRAYSIZE(args), pReturn, m_hScope, true );
	}

	template <typename ARG_TYPE_1, typename ARG_TYPE_2, typename	ARG_TYPE_3,	typename ARG_TYPE_4, typename ARG_TYPE_5>
	ScriptStatus_t Call( HSCRIPT hFunction, ScriptVariant_t *pReturn, ARG_TYPE_1 arg1, ARG_TYPE_2 arg2, ARG_TYPE_3 arg3, ARG_TYPE_4 arg4, ARG_TYPE_5 arg5 )
	{
		ScriptVariant_t args[5]; args[0] = arg1; args[1] = arg2; args[2] = arg3; args[3] = arg4; args[4] = arg5;
		return GetVM()->ExecuteFunction( hFunction, args, Q_ARRAYSIZE(args), pReturn, m_hScope, true );
	}

	template <typename ARG_TYPE_1, typename ARG_TYPE_2, typename	ARG_TYPE_3,	typename ARG_TYPE_4, typename ARG_TYPE_5, typename ARG_TYPE_6>
	ScriptStatus_t Call( HSCRIPT hFunction, ScriptVariant_t *pReturn, ARG_TYPE_1 arg1, ARG_TYPE_2 arg2, ARG_TYPE_3 arg3, ARG_TYPE_4 arg4, ARG_TYPE_5 arg5, ARG_TYPE_6 arg6 )
	{
		ScriptVariant_t args[6]; args[0] = arg1; args[1] = arg2; args[2] = arg3; args[3] = arg4; args[4] = arg5; args[5] = arg6;
		return GetVM()->ExecuteFunction( hFunction, args, Q_ARRAYSIZE(args), pReturn, m_hScope, true );
	}

	template <typename ARG_TYPE_1, typename ARG_TYPE_2, typename	ARG_TYPE_3,	typename ARG_TYPE_4, typename ARG_TYPE_5, typename ARG_TYPE_6, typename ARG_TYPE_7>
	ScriptStatus_t Call( HSCRIPT hFunction, ScriptVariant_t *pReturn, ARG_TYPE_1 arg1, ARG_TYPE_2 arg2, ARG_TYPE_3 arg3, ARG_TYPE_4 arg4, ARG_TYPE_5 arg5, ARG_TYPE_6 arg6, ARG_TYPE_7 arg7 )
	{
		ScriptVariant_t args[7]; args[0] = arg1; args[1] = arg2; args[2] = arg3; args[3] = arg4; args[4] = arg5; args[5] = arg6; args[6] = arg7;
		return GetVM()->ExecuteFunction( hFunction, args, Q_ARRAYSIZE(args), pReturn, m_hScope, true );
	}

	template <typename ARG_TYPE_1, typename ARG_TYPE_2, typename	ARG_TYPE_3,	typename ARG_TYPE_4, typename ARG_TYPE_5, typename ARG_TYPE_6, typename ARG_TYPE_7, typename ARG_TYPE_8>
	ScriptStatus_t Call( HSCRIPT hFunction, ScriptVariant_t *pReturn, ARG_TYPE_1 arg1, ARG_TYPE_2 arg2, ARG_TYPE_3 arg3, ARG_TYPE_4 arg4, ARG_TYPE_5 arg5, ARG_TYPE_6 arg6, ARG_TYPE_7 arg7, ARG_TYPE_8 arg8 )
	{
		ScriptVariant_t args[8]; args[0] = arg1; args[1] = arg2; args[2] = arg3; args[3] = arg4; args[4] = arg5; args[5] = arg6; args[6] = arg7; args[7] = arg8;
		return GetVM()->ExecuteFunction( hFunction, args, Q_ARRAYSIZE(args), pReturn, m_hScope, true );
	}

	template <typename ARG_TYPE_1, typename ARG_TYPE_2, typename	ARG_TYPE_3,	typename ARG_TYPE_4, typename ARG_TYPE_5, typename ARG_TYPE_6, typename ARG_TYPE_7, typename ARG_TYPE_8, typename ARG_TYPE_9>
	ScriptStatus_t Call( HSCRIPT hFunction, ScriptVariant_t *pReturn, ARG_TYPE_1 arg1, ARG_TYPE_2 arg2, ARG_TYPE_3 arg3, ARG_TYPE_4 arg4, ARG_TYPE_5 arg5, ARG_TYPE_6 arg6, ARG_TYPE_7 arg7, ARG_TYPE_8 arg8, ARG_TYPE_9 arg9 )
	{
		ScriptVariant_t args[9]; args[0] = arg1; args[1] = arg2; args[2] = arg3; args[3] = arg4; args[4] = arg5; args[5] = arg6; args[6] = arg7; args[7] = arg8; args[8] = arg9;
		return GetVM()->ExecuteFunction( hFunction, args, Q_ARRAYSIZE(args), pReturn, m_hScope, true );
	}

	template <typename ARG_TYPE_1, typename ARG_TYPE_2, typename	ARG_TYPE_3,	typename ARG_TYPE_4, typename ARG_TYPE_5, typename ARG_TYPE_6, typename ARG_TYPE_7, typename ARG_TYPE_8, typename ARG_TYPE_9, typename ARG_TYPE_10>
	ScriptStatus_t Call( HSCRIPT hFunction, ScriptVariant_t *pReturn, ARG_TYPE_1 arg1, ARG_TYPE_2 arg2, ARG_TYPE_3 arg3, ARG_TYPE_4 arg4, ARG_TYPE_5 arg5, ARG_TYPE_6 arg6, ARG_TYPE_7 arg7, ARG_TYPE_8 arg8, ARG_TYPE_9 arg9, ARG_TYPE_10 arg10 )
	{
		ScriptVariant_t args[10]; args[0] = arg1; args[1] = arg2; args[2] = arg3; args[3] = arg4; args[4] = arg5; args[5] = arg6; args[6] = arg7; args[7] = arg8; args[8] = arg9; args[9] = arg10;
		return GetVM()->ExecuteFunction( hFunction, args, Q_ARRAYSIZE(args), pReturn, m_hScope, true );
	}

	template <typename ARG_TYPE_1, typename ARG_TYPE_2, typename	ARG_TYPE_3,	typename ARG_TYPE_4, typename ARG_TYPE_5, typename ARG_TYPE_6, typename ARG_TYPE_7, typename ARG_TYPE_8, typename ARG_TYPE_9, typename ARG_TYPE_10, typename ARG_TYPE_11>
	ScriptStatus_t Call( HSCRIPT hFunction, ScriptVariant_t *pReturn, ARG_TYPE_1 arg1, ARG_TYPE_2 arg2, ARG_TYPE_3 arg3, ARG_TYPE_4 arg4, ARG_TYPE_5 arg5, ARG_TYPE_6 arg6, ARG_TYPE_7 arg7, ARG_TYPE_8 arg8, ARG_TYPE_9 arg9, ARG_TYPE_10 arg10, ARG_TYPE_11 arg11 )
	{
		ScriptVariant_t args[11]; args[0] = arg1; args[1] = arg2; args[2] = arg3; args[3] = arg4; args[4] = arg5; args[5] = arg6; args[6] = arg7; args[7] = arg8; args[8] = arg9; args[9] = arg10; args[10] = arg11;
		return GetVM()->ExecuteFunction( hFunction, args, Q_ARRAYSIZE(args), pReturn, m_hScope, true );
	}

	template <typename ARG_TYPE_1, typename ARG_TYPE_2, typename	ARG_TYPE_3,	typename ARG_TYPE_4, typename ARG_TYPE_5, typename ARG_TYPE_6, typename ARG_TYPE_7, typename ARG_TYPE_8, typename ARG_TYPE_9, typename ARG_TYPE_10, typename ARG_TYPE_11, typename ARG_TYPE_12>
	ScriptStatus_t Call( HSCRIPT hFunction, ScriptVariant_t *pReturn, ARG_TYPE_1 arg1, ARG_TYPE_2 arg2, ARG_TYPE_3 arg3, ARG_TYPE_4 arg4, ARG_TYPE_5 arg5, ARG_TYPE_6 arg6, ARG_TYPE_7 arg7, ARG_TYPE_8 arg8, ARG_TYPE_9 arg9, ARG_TYPE_10 arg10, ARG_TYPE_11 arg11, ARG_TYPE_12 arg12 )
	{
		ScriptVariant_t args[12]; args[0] = arg1; args[1] = arg2; args[2] = arg3; args[3] = arg4; args[4] = arg5; args[5] = arg6; args[6] = arg7; args[7] = arg8; args[8] = arg9; args[9] = arg10; args[10] = arg11; args[11] = arg12;
		return GetVM()->ExecuteFunction( hFunction, args, Q_ARRAYSIZE(args), pReturn, m_hScope, true );
	}

	ScriptStatus_t Call( const char *pszFunction, ScriptVariant_t *pReturn = NULL )
	{
		HSCRIPT hFunction = GetVM()->LookupFunction( pszFunction, m_hScope );
		if ( !hFunction )
			return SCRIPT_ERROR;
		ScriptStatus_t status = GetVM()->ExecuteFunction( hFunction, NULL, 0, pReturn, m_hScope, true );
		GetVM()->ReleaseFunction( hFunction );
		return status;
	}

	template <typename ARG_TYPE_1>
	ScriptStatus_t Call( const char *pszFunction, ScriptVariant_t *pReturn, ARG_TYPE_1 arg1 )
	{
		ScriptVariant_t args[1]; args[0] = arg1;
		HSCRIPT hFunction = GetVM()->LookupFunction( pszFunction, m_hScope );
		if ( !hFunction )
			return SCRIPT_ERROR;
		ScriptStatus_t status = GetVM()->ExecuteFunction( hFunction, args, Q_ARRAYSIZE(args), pReturn, m_hScope, true );
		GetVM()->ReleaseFunction( hFunction );
		return status;
	}

	template <typename ARG_TYPE_1, typename ARG_TYPE_2>
	ScriptStatus_t Call( const char *pszFunction, ScriptVariant_t *pReturn, ARG_TYPE_1 arg1, ARG_TYPE_2 arg2 )
	{
		ScriptVariant_t args[2]; args[0] = arg1; args[1] = arg2;
		HSCRIPT hFunction = GetVM()->LookupFunction( pszFunction, m_hScope );
		if ( !hFunction )
			return SCRIPT_ERROR;
		ScriptStatus_t status = GetVM()->ExecuteFunction( hFunction, args, Q_ARRAYSIZE(args), pReturn, m_hScope, true );
		GetVM()->ReleaseFunction( hFunction );
		return status;
	}

	template <typename ARG_TYPE_1, typename ARG_TYPE_2, typename	ARG_TYPE_3>
	ScriptStatus_t Call( const char *pszFunction, ScriptVariant_t *pReturn, ARG_TYPE_1 arg1, ARG_TYPE_2 arg2, ARG_TYPE_3 arg3 )
	{
		ScriptVariant_t args[3]; args[0] = arg1; args[1] = arg2; args[2] = arg3;
		HSCRIPT hFunction = GetVM()->LookupFunction( pszFunction, m_hScope );
		if ( !hFunction )
			return SCRIPT_ERROR;
		ScriptStatus_t status = GetVM()->ExecuteFunction( hFunction, args, Q_ARRAYSIZE(args), pReturn, m_hScope, true );
		GetVM()->ReleaseFunction( hFunction );
		return status;
	}

	template <typename ARG_TYPE_1, typename ARG_TYPE_2, typename	ARG_TYPE_3,	typename ARG_TYPE_4>
	ScriptStatus_t Call( const char *pszFunction, ScriptVariant_t *pReturn, ARG_TYPE_1 arg1, ARG_TYPE_2 arg2, ARG_TYPE_3 arg3, ARG_TYPE_4 arg4 )
	{
		ScriptVariant_t args[4]; args[0] = arg1; args[1] = arg2; args[2] = arg3; args[3] = arg4;
		HSCRIPT hFunction = GetVM()->LookupFunction( pszFunction, m_hScope );
		if ( !hFunction )
			return SCRIPT_ERROR;
		ScriptStatus_t status = GetVM()->ExecuteFunction( hFunction, args, Q_ARRAYSIZE(args), pReturn, m_hScope, true );
		GetVM()->ReleaseFunction( hFunction );
		return status;
	}

	template <typename ARG_TYPE_1, typename ARG_TYPE_2, typename	ARG_TYPE_3,	typename ARG_TYPE_4, typename ARG_TYPE_5>
	ScriptStatus_t Call( const char *pszFunction, ScriptVariant_t *pReturn, ARG_TYPE_1 arg1, ARG_TYPE_2 arg2, ARG_TYPE_3 arg3, ARG_TYPE_4 arg4, ARG_TYPE_5 arg5 )
	{
		ScriptVariant_t args[5]; args[0] = arg1; args[1] = arg2; args[2] = arg3; args[3] = arg4; args[4] = arg5;
		HSCRIPT hFunction = GetVM()->LookupFunction( pszFunction, m_hScope );
		if ( !hFunction )
			return SCRIPT_ERROR;
		ScriptStatus_t status = GetVM()->ExecuteFunction( hFunction, args, Q_ARRAYSIZE(args), pReturn, m_hScope, true );
		GetVM()->ReleaseFunction( hFunction );
		return status;
	}

	template <typename ARG_TYPE_1, typename ARG_TYPE_2, typename	ARG_TYPE_3,	typename ARG_TYPE_4, typename ARG_TYPE_5, typename ARG_TYPE_6>
	ScriptStatus_t Call( const char *pszFunction, ScriptVariant_t *pReturn, ARG_TYPE_1 arg1, ARG_TYPE_2 arg2, ARG_TYPE_3 arg3, ARG_TYPE_4 arg4, ARG_TYPE_5 arg5, ARG_TYPE_6 arg6 )
	{
		ScriptVariant_t args[6]; args[0] = arg1; args[1] = arg2; args[2] = arg3; args[3] = arg4; args[4] = arg5; args[5] = arg6;
		HSCRIPT hFunction = GetVM()->LookupFunction( pszFunction, m_hScope );
		if ( !hFunction )
			return SCRIPT_ERROR;
		ScriptStatus_t status = GetVM()->ExecuteFunction( hFunction, args, Q_ARRAYSIZE(args), pReturn, m_hScope, true );
		GetVM()->ReleaseFunction( hFunction );
		return status;
	}

	template <typename ARG_TYPE_1, typename ARG_TYPE_2, typename	ARG_TYPE_3,	typename ARG_TYPE_4, typename ARG_TYPE_5, typename ARG_TYPE_6, typename ARG_TYPE_7>
	ScriptStatus_t Call( const char *pszFunction, ScriptVariant_t *pReturn, ARG_TYPE_1 arg1, ARG_TYPE_2 arg2, ARG_TYPE_3 arg3, ARG_TYPE_4 arg4, ARG_TYPE_5 arg5, ARG_TYPE_6 arg6, ARG_TYPE_7 arg7 )
	{
		ScriptVariant_t args[7]; args[0] = arg1; args[1] = arg2; args[2] = arg3; args[3] = arg4; args[4] = arg5; args[5] = arg6; args[6] = arg7;
		HSCRIPT hFunction = GetVM()->LookupFunction( pszFunction, m_hScope );
		if ( !hFunction )
			return SCRIPT_ERROR;
		ScriptStatus_t status = GetVM()->ExecuteFunction( hFunction, args, Q_ARRAYSIZE(args), pReturn, m_hScope, true );
		GetVM()->ReleaseFunction( hFunction );
		return status;
	}

	template <typename ARG_TYPE_1, typename ARG_TYPE_2, typename	ARG_TYPE_3,	typename ARG_TYPE_4, typename ARG_TYPE_5, typename ARG_TYPE_6, typename ARG_TYPE_7, typename ARG_TYPE_8>
	ScriptStatus_t Call( const char *pszFunction, ScriptVariant_t *pReturn, ARG_TYPE_1 arg1, ARG_TYPE_2 arg2, ARG_TYPE_3 arg3, ARG_TYPE_4 arg4, ARG_TYPE_5 arg5, ARG_TYPE_6 arg6, ARG_TYPE_7 arg7, ARG_TYPE_8 arg8 )
	{
		ScriptVariant_t args[8]; args[0] = arg1; args[1] = arg2; args[2] = arg3; args[3] = arg4; args[4] = arg5; args[5] = arg6; args[6] = arg7; args[7] = arg8;
		HSCRIPT hFunction = GetVM()->LookupFunction( pszFunction, m_hScope );
		if ( !hFunction )
			return SCRIPT_ERROR;
		ScriptStatus_t status = GetVM()->ExecuteFunction( hFunction, args, Q_ARRAYSIZE(args), pReturn, m_hScope, true );
		GetVM()->ReleaseFunction( hFunction );
		return status;
	}

	template <typename ARG_TYPE_1, typename ARG_TYPE_2, typename	ARG_TYPE_3,	typename ARG_TYPE_4, typename ARG_TYPE_5, typename ARG_TYPE_6, typename ARG_TYPE_7, typename ARG_TYPE_8, typename ARG_TYPE_9>
	ScriptStatus_t Call( const char *pszFunction, ScriptVariant_t *pReturn, ARG_TYPE_1 arg1, ARG_TYPE_2 arg2, ARG_TYPE_3 arg3, ARG_TYPE_4 arg4, ARG_TYPE_5 arg5, ARG_TYPE_6 arg6, ARG_TYPE_7 arg7, ARG_TYPE_8 arg8, ARG_TYPE_9 arg9 )
	{
		ScriptVariant_t args[9]; args[0] = arg1; args[1] = arg2; args[2] = arg3; args[3] = arg4; args[4] = arg5; args[5] = arg6; args[6] = arg7; args[7] = arg8; args[8] = arg9;
		HSCRIPT hFunction = GetVM()->LookupFunction( pszFunction, m_hScope );
		if ( !hFunction )
			return SCRIPT_ERROR;
		ScriptStatus_t status = GetVM()->ExecuteFunction( hFunction, args, Q_ARRAYSIZE(args), pReturn, m_hScope, true );
		GetVM()->ReleaseFunction( hFunction );
		return status;
	}

	template <typename ARG_TYPE_1, typename ARG_TYPE_2, typename	ARG_TYPE_3,	typename ARG_TYPE_4, typename ARG_TYPE_5, typename ARG_TYPE_6, typename ARG_TYPE_7, typename ARG_TYPE_8, typename ARG_TYPE_9, typename ARG_TYPE_10>
	ScriptStatus_t Call( const char *pszFunction, ScriptVariant_t *pReturn, ARG_TYPE_1 arg1, ARG_TYPE_2 arg2, ARG_TYPE_3 arg3, ARG_TYPE_4 arg4, ARG_TYPE_5 arg5, ARG_TYPE_6 arg6, ARG_TYPE_7 arg7, ARG_TYPE_8 arg8, ARG_TYPE_9 arg9, ARG_TYPE_10 arg10 )
	{
		ScriptVariant_t args[10]; args[0] = arg1; args[1] = arg2; args[2] = arg3; args[3] = arg4; args[4] = arg5; args[5] = arg6; args[6] = arg7; args[7] = arg8; args[8] = arg9; args[9] = arg10;
		HSCRIPT hFunction = GetVM()->LookupFunction( pszFunction, m_hScope );
		if ( !hFunction )
			return SCRIPT_ERROR;
		ScriptStatus_t status = GetVM()->ExecuteFunction( hFunction, args, Q_ARRAYSIZE(args), pReturn, m_hScope, true );
		GetVM()->ReleaseFunction( hFunction );
		return status;
	}

	template <typename ARG_TYPE_1, typename ARG_TYPE_2, typename	ARG_TYPE_3,	typename ARG_TYPE_4, typename ARG_TYPE_5, typename ARG_TYPE_6, typename ARG_TYPE_7, typename ARG_TYPE_8, typename ARG_TYPE_9, typename ARG_TYPE_10, typename ARG_TYPE_11>
	ScriptStatus_t Call( const char *pszFunction, ScriptVariant_t *pReturn, ARG_TYPE_1 arg1, ARG_TYPE_2 arg2, ARG_TYPE_3 arg3, ARG_TYPE_4 arg4, ARG_TYPE_5 arg5, ARG_TYPE_6 arg6, ARG_TYPE_7 arg7, ARG_TYPE_8 arg8, ARG_TYPE_9 arg9, ARG_TYPE_10 arg10, ARG_TYPE_11 arg11 )
	{
		ScriptVariant_t args[11]; args[0] = arg1; args[1] = arg2; args[2] = arg3; args[3] = arg4; args[4] = arg5; args[5] = arg6; args[6] = arg7; args[7] = arg8; args[8] = arg9; args[9] = arg10; args[10] = arg11;
		HSCRIPT hFunction = GetVM()->LookupFunction( pszFunction, m_hScope );
		if ( !hFunction )
			return SCRIPT_ERROR;
		ScriptStatus_t status = GetVM()->ExecuteFunction( hFunction, args, Q_ARRAYSIZE(args), pReturn, m_hScope, true );
		GetVM()->ReleaseFunction( hFunction );
		return status;
	}

	template <typename ARG_TYPE_1, typename ARG_TYPE_2, typename	ARG_TYPE_3,	typename ARG_TYPE_4, typename ARG_TYPE_5, typename ARG_TYPE_6, typename ARG_TYPE_7, typename ARG_TYPE_8, typename ARG_TYPE_9, typename ARG_TYPE_10, typename ARG_TYPE_11, typename ARG_TYPE_12>
	ScriptStatus_t Call( const char *pszFunction, ScriptVariant_t *pReturn, ARG_TYPE_1 arg1, ARG_TYPE_2 arg2, ARG_TYPE_3 arg3, ARG_TYPE_4 arg4, ARG_TYPE_5 arg5, ARG_TYPE_6 arg6, ARG_TYPE_7 arg7, ARG_TYPE_8 arg8, ARG_TYPE_9 arg9, ARG_TYPE_10 arg10, ARG_TYPE_11 arg11, ARG_TYPE_12 arg12 )
	{
		ScriptVariant_t args[12]; args[0] = arg1; args[1] = arg2; args[2] = arg3; args[3] = arg4; args[4] = arg5; args[5] = arg6; args[6] = arg7; args[7] = arg8; args[8] = arg9; args[9] = arg10; args[10] = arg11; args[11] = arg12;
		HSCRIPT hFunction = GetVM()->LookupFunction( pszFunction, m_hScope );
		if ( !hFunction )
			return SCRIPT_ERROR;
		ScriptStatus_t status = GetVM()->ExecuteFunction( hFunction, args, Q_ARRAYSIZE(args), pReturn, m_hScope, true );
		GetVM()->ReleaseFunction( hFunction );
		return status;
	}

protected:
	HSCRIPT m_hScope;
	int m_flags;
	CUtlVectorConservative<HSCRIPT *> m_FuncHandles;
};

typedef CScriptScopeT<> CScriptScope;

#define VScriptAddEnumToScope_( scope, enumVal, scriptName )	(scope).SetValue( scriptName, (int)enumVal )
#define VScriptAddEnumToScope( scope, enumVal )					VScriptAddEnumToScope_( scope, enumVal, #enumVal )

#define VScriptAddEnumToRoot( enumVal )					g_pScriptVM->SetValue( #enumVal, (int)enumVal )

//-----------------------------------------------------------------------------
// Script function proxy support
//-----------------------------------------------------------------------------

class CScriptFuncHolder
{
public:
	CScriptFuncHolder() : hFunction( INVALID_HSCRIPT ) {}
	bool IsValid()	{ return ( hFunction != INVALID_HSCRIPT ); }
	bool IsNull()	{ return ( !hFunction ); }
	HSCRIPT hFunction;
};

#define DEFINE_SCRIPT_PROXY_GUTS( FuncName, N ) \
	CScriptFuncHolder m_hScriptFunc_##FuncName; \
	template < typename RET_TYPE FUNC_TEMPLATE_ARG_PARAMS_##N> \
	bool FuncName( RET_TYPE *pRetVal FUNC_ARG_FORMAL_PARAMS_##N ) \
	{ \
		if ( !m_hScriptFunc_##FuncName.IsValid() ) \
		{ \
			m_hScriptFunc_##FuncName.hFunction = LookupFunction( #FuncName ); \
			m_FuncHandles.AddToTail( &m_hScriptFunc_##FuncName.hFunction ); \
		} \
		\
		if ( !m_hScriptFunc_##FuncName.IsNull() ) \
		{ \
			ScriptVariant_t returnVal; \
			ScriptStatus_t result = Call( m_hScriptFunc_##FuncName.hFunction, &returnVal, FUNC_CALL_ARGS_##N ); \
			if ( result != SCRIPT_ERROR ) \
			{ \
				returnVal.AssignTo( pRetVal ); \
				returnVal.Free(); \
				return true; \
			} \
		} \
		return false; \
	}

#define DEFINE_SCRIPT_PROXY_GUTS_NO_RETVAL( FuncName, N ) \
	CScriptFuncHolder m_hScriptFunc_##FuncName; \
	template < FUNC_SOLO_TEMPLATE_ARG_PARAMS_##N> \
	bool FuncName( FUNC_PROXY_ARG_FORMAL_PARAMS_##N ) \
	{ \
		if ( !m_hScriptFunc_##FuncName.IsValid() ) \
		{ \
			m_hScriptFunc_##FuncName.hFunction = LookupFunction( #FuncName ); \
			m_FuncHandles.AddToTail( &m_hScriptFunc_##FuncName.hFunction ); \
		} \
		\
		if ( !m_hScriptFunc_##FuncName.IsNull() ) \
		{ \
			ScriptStatus_t result = Call( m_hScriptFunc_##FuncName.hFunction, NULL, FUNC_CALL_ARGS_##N ); \
			if ( result != SCRIPT_ERROR ) \
			{ \
				return true; \
			} \
		} \
		return false; \
	}

#define DEFINE_SCRIPT_PROXY_0V( FuncName ) \
	CScriptFuncHolder m_hScriptFunc_##FuncName; \
	bool FuncName() \
	{ \
		if ( !m_hScriptFunc_##FuncName.IsValid() ) \
		{ \
			m_hScriptFunc_##FuncName.hFunction = LookupFunction( #FuncName ); \
			m_FuncHandles.AddToTail( &m_hScriptFunc_##FuncName.hFunction ); \
		} \
		\
		if ( !m_hScriptFunc_##FuncName.IsNull() ) \
		{ \
			ScriptStatus_t result = Call( m_hScriptFunc_##FuncName.hFunction, NULL ); \
			if ( result != SCRIPT_ERROR ) \
			{ \
				return true; \
			} \
		} \
		return false; \
	}

#define DEFINE_SCRIPT_PROXY_0( FuncName ) DEFINE_SCRIPT_PROXY_GUTS( FuncName, 0 )
#define DEFINE_SCRIPT_PROXY_1( FuncName ) DEFINE_SCRIPT_PROXY_GUTS( FuncName, 1 )
#define DEFINE_SCRIPT_PROXY_2( FuncName ) DEFINE_SCRIPT_PROXY_GUTS( FuncName, 2 )
#define DEFINE_SCRIPT_PROXY_3( FuncName ) DEFINE_SCRIPT_PROXY_GUTS( FuncName, 3 )
#define DEFINE_SCRIPT_PROXY_4( FuncName ) DEFINE_SCRIPT_PROXY_GUTS( FuncName, 4 )
#define DEFINE_SCRIPT_PROXY_5( FuncName ) DEFINE_SCRIPT_PROXY_GUTS( FuncName, 5 )
#define DEFINE_SCRIPT_PROXY_6( FuncName ) DEFINE_SCRIPT_PROXY_GUTS( FuncName, 6 )
#define DEFINE_SCRIPT_PROXY_7( FuncName ) DEFINE_SCRIPT_PROXY_GUTS( FuncName, 7 )
#define DEFINE_SCRIPT_PROXY_8( FuncName ) DEFINE_SCRIPT_PROXY_GUTS( FuncName, 8 )
#define DEFINE_SCRIPT_PROXY_9( FuncName ) DEFINE_SCRIPT_PROXY_GUTS( FuncName, 9 )
#define DEFINE_SCRIPT_PROXY_10( FuncName ) DEFINE_SCRIPT_PROXY_GUTS( FuncName, 10 )
#define DEFINE_SCRIPT_PROXY_11( FuncName ) DEFINE_SCRIPT_PROXY_GUTS( FuncName, 11 )
#define DEFINE_SCRIPT_PROXY_12( FuncName ) DEFINE_SCRIPT_PROXY_GUTS( FuncName, 12 )

#define DEFINE_SCRIPT_PROXY_1V( FuncName ) DEFINE_SCRIPT_PROXY_GUTS_NO_RETVAL( FuncName, 1 )
#define DEFINE_SCRIPT_PROXY_2V( FuncName ) DEFINE_SCRIPT_PROXY_GUTS_NO_RETVAL( FuncName, 2 )
#define DEFINE_SCRIPT_PROXY_3V( FuncName ) DEFINE_SCRIPT_PROXY_GUTS_NO_RETVAL( FuncName, 3 )
#define DEFINE_SCRIPT_PROXY_4V( FuncName ) DEFINE_SCRIPT_PROXY_GUTS_NO_RETVAL( FuncName, 4 )
#define DEFINE_SCRIPT_PROXY_5V( FuncName ) DEFINE_SCRIPT_PROXY_GUTS_NO_RETVAL( FuncName, 5 )
#define DEFINE_SCRIPT_PROXY_6V( FuncName ) DEFINE_SCRIPT_PROXY_GUTS_NO_RETVAL( FuncName, 6 )
#define DEFINE_SCRIPT_PROXY_7V( FuncName ) DEFINE_SCRIPT_PROXY_GUTS_NO_RETVAL( FuncName, 7 )
#define DEFINE_SCRIPT_PROXY_8V( FuncName ) DEFINE_SCRIPT_PROXY_GUTS_NO_RETVAL( FuncName, 8 )
#define DEFINE_SCRIPT_PROXY_9V( FuncName ) DEFINE_SCRIPT_PROXY_GUTS_NO_RETVAL( FuncName, 9 )
#define DEFINE_SCRIPT_PROXY_10V( FuncName ) DEFINE_SCRIPT_PROXY_GUTS_NO_RETVAL( FuncName, 10 )
#define DEFINE_SCRIPT_PROXY_11V( FuncName ) DEFINE_SCRIPT_PROXY_GUTS_NO_RETVAL( FuncName, 11 )
#define DEFINE_SCRIPT_PROXY_12V( FuncName ) DEFINE_SCRIPT_PROXY_GUTS_NO_RETVAL( FuncName, 12 )

//-----------------------------------------------------------------------------

#include "tier0/memdbgoff.h"

#endif // IVSCRIPT_H
