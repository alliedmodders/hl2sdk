
//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "basetypes.h"
#include "tier1/convar.h"
#include "tier1/strtools.h"
#include "tier1/characterset.h"
#include "tier1/utlvector.h"
#include "tier1/utlbuffer.h"
#include "tier1/tier1.h"
#include "tier1/convar_serverbounded.h"
#include "icvar.h"
#include "tier0/dbg.h"
#include "Color.h"
#if defined( _X360 )
#include "xbox/xbox_console.h"
#endif
#include "tier0/memdbgon.h"


// Comment this out when we release.
//#define ALLOW_DEVELOPMENT_CVARS


//-----------------------------------------------------------------------------
// Statically constructed list of ConCommandBases, 
// used for registering them with the ICVar interface
//-----------------------------------------------------------------------------
static int64 s_nCVarFlag = 0;
static bool s_bRegistered = false;

class ConCommandRegList;
class ConCommandRegList
{
public:
	static void RegisterCommand(ConCommand* pCmd)
	{
		if (s_bConCommandsRegistered)
		{
			ConCommandHandle hndl = g_pCVar->RegisterConCommand(pCmd, s_nCVarFlag);
			if (!hndl.IsValid())
			{
				Plat_FatalErrorFunc("RegisterConCommand: Unknown error registering con command \"%s\"!\n", pCmd->GetName());
				DebuggerBreakIfDebugging();
			}

			pCmd->SetHandle(hndl);
		}
		else
		{
			GetCommandRegList()->AddToTail(pCmd);
		}
	}

	static void RegisterAll()
	{
		if (!s_bConCommandsRegistered && g_pCVar)
		{
			s_bConCommandsRegistered = true;

			for(int i = 0; i < GetCommandRegList()->Count(); i++)
			{
				ConCommand *pCmd = GetCommandRegList()->Element(i);
				ConCommandHandle hndl = g_pCVar->RegisterConCommand(pCmd, s_nCVarFlag);
				pCmd->SetHandle(hndl);

				if (!hndl.IsValid())
				{
					Plat_FatalErrorFunc("RegisterConCommand: Unknown error registering con command \"%s\"!\n", pCmd->GetName());
					DebuggerBreakIfDebugging();
				}
			}
		}
	}
private:

	// GAMMACASE: Required to prevent static initialization order problem https://isocpp.org/wiki/faq/ctors#static-init-order
	static CUtlVector<ConCommand *> *GetCommandRegList()
	{
		static CUtlVector<ConCommand *> s_ConCommandRegList;
		return &s_ConCommandRegList;
	}

	static bool s_bConCommandsRegistered;
};

bool ConCommandRegList::s_bConCommandsRegistered = false;

#ifdef CONVAR_WORK_FINISHED
template <typename ToCheck, std::size_t ExpectedSize, std::size_t RealSize = sizeof(ToCheck)>
void check_size() {
	static_assert(ExpectedSize == RealSize, "Size mismatch");
};

class ConVarRegList;
static ConVarRegList* s_pConVarRegList = nullptr;

class ConVarRegList
{
public:
	ConVarRegList()
	{
		check_size<ConVar, 0x60>();
		check_size<ConVarRegList, 11216>();
	}

	static bool AreConVarsRegistered()
	{
		return s_bConVarsRegistered;
	}

	static void RegisterAll()
	{
		if (!s_bConVarsRegistered && g_pCVar)
		{
			s_bConVarsRegistered = true;

			ConVarRegList* pList = s_pConVarRegList;
			while (pList != nullptr)
			{
				FOR_EACH_VEC(s_pConVarRegList->m_Vec, i)
				{
					ConVar* pConVar = &pList->m_Vec[i];
					ConVarHandle hndl;
					//g_pCVar->RegisterConVar(pConVar, s_nCVarFlag, hndl);
					//pConVar->SetHandle(hndl);

					if (!hndl.IsValid())
					{
						Plat_FatalErrorFunc("RegisterConCommand: Unknown error registering convar \"%s\"!\n", pConVar->GetName());
						DebuggerBreakIfDebugging();
					}
				}

				ConVarRegList *pNext = pList->m_pNext;
				delete pList;
				pList = pNext;
			}

			s_pConVarRegList = nullptr;
		}
	}
private:
	CUtlVectorFixed<ConVar, 100> m_Vec;
	ConVarRegList* m_pNext = nullptr;

	static bool s_bConVarsRegistered;
};

bool ConVarRegList::s_bConVarsRegistered = false;
#endif // CONVAR_WORK_FINISHED

//-----------------------------------------------------------------------------
// Called by the framework to register ConCommandBases with the ICVar
//-----------------------------------------------------------------------------
void ConVar_Register( int64 nCVarFlag)
{
	if ( !g_pCVar || s_bRegistered )
		return;

	s_bRegistered = true;
	s_nCVarFlag = nCVarFlag;

	ConCommandRegList::RegisterAll();
#ifdef CONVAR_WORK_FINISHED
	ConVarRegList::RegisterAll();
#endif // CONVAR_WORK_FINISHED
}

void ConVar_Unregister( )
{
	if ( !g_pCVar || !s_bRegistered )
		return;

	s_bRegistered = false;
}


//-----------------------------------------------------------------------------
// Purpose: Default constructor
//-----------------------------------------------------------------------------
ConCommandBase::ConCommandBase( void )
{
	m_pszName       = NULL;
	m_pszHelpString = NULL;

	m_nFlags = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ConCommandBase::~ConCommandBase( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Return name of the command/var
// Output : const char
//-----------------------------------------------------------------------------
const char *ConCommandBase::GetName( void ) const
{
	return m_pszName;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flag - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool ConCommandBase::IsFlagSet( int64 flag ) const
{
	return ( flag & m_nFlags ) ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flags - 
//-----------------------------------------------------------------------------
void ConCommandBase::AddFlags( int64 flags )
{
	m_nFlags |= flags;

#ifdef ALLOW_DEVELOPMENT_CVARS
	m_nFlags &= ~FCVAR_DEVELOPMENTONLY;
#endif
}

void ConCommandBase::RemoveFlags( int64 flags )
{
	m_nFlags &= ~flags;
}

int64 ConCommandBase::GetFlags( void ) const
{
	return m_nFlags;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *ConCommandBase::GetHelpText( void ) const
{
	return m_pszHelpString;
}

//-----------------------------------------------------------------------------
//
// Con Commands start here
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Global methods
//-----------------------------------------------------------------------------
static characterset_t s_BreakSet;
static bool s_bBuiltBreakSet = false;


//-----------------------------------------------------------------------------
// Tokenizer class
//-----------------------------------------------------------------------------
CCommand::CCommand()
{
	if ( !s_bBuiltBreakSet )
	{
		s_bBuiltBreakSet = true;
		CharacterSetBuild( &s_BreakSet, "{}()':" );
	}

	Reset();
}

CCommand::CCommand( int nArgC, const char **ppArgV )
{
	Assert( nArgC > 0 );

	if ( !s_bBuiltBreakSet )
	{
		s_bBuiltBreakSet = true;
		CharacterSetBuild( &s_BreakSet, "{}()':" );
	}

	Reset();

	char *pBuf = m_ArgvBuffer.Base();
	char *pSBuf = m_ArgSBuffer.Base();
	for ( int i = 0; i < nArgC; ++i )
	{
		m_Args.AddToTail( pBuf );
		int nLen = V_strlen( ppArgV[i] );
		memcpy( pBuf, ppArgV[i], nLen+1 );
		if ( i == 0 )
		{
			m_nArgv0Size = nLen;
		}
		pBuf += nLen+1;

		bool bContainsSpace = strchr( ppArgV[i], ' ' ) != NULL;
		if ( bContainsSpace )
		{
			*pSBuf++ = '\"';
		}
		memcpy( pSBuf, ppArgV[i], nLen );
		pSBuf += nLen;
		if ( bContainsSpace )
		{
			*pSBuf++ = '\"';
		}

		if ( i != nArgC - 1 )
		{
			*pSBuf++ = ' ';
		}
	}
}

void CCommand::Reset()
{
	m_nArgv0Size = 0;
	m_ArgSBuffer.RemoveAll();
	m_ArgvBuffer.RemoveAll();
	m_Args.RemoveAll();
}

characterset_t* CCommand::DefaultBreakSet()
{
	return &s_BreakSet;
}

bool CCommand::Tokenize( const char *pCommand, characterset_t *pBreakSet )
{
	Reset();
	if ( !pCommand )
		return false;

	// Use default break set
	if ( !pBreakSet )
	{
		pBreakSet = &s_BreakSet;
	}

	// Copy the current command into a temp buffer
	// NOTE: This is here to avoid the pointers returned by DequeueNextCommand
	// to become invalid by calling AddText. Is there a way we can avoid the memcpy?
	int nLen = V_strlen( pCommand );
	if ( nLen >= COMMAND_MAX_LENGTH - 1 )
	{
		Warning( "CCommand::Tokenize: Encountered command which overflows the tokenizer buffer.. Skipping!\n" );
		return false;
	}

	memcpy( m_ArgSBuffer.Base(), pCommand, nLen + 1 );

	// Parse the current command into the current command buffer
	CUtlBuffer bufParse( m_ArgSBuffer.Base(), nLen, CUtlBuffer::TEXT_BUFFER | CUtlBuffer::READ_ONLY);
	int nArgvBufferSize = 0;
	while ( bufParse.IsValid() && ( m_Args.Count() < COMMAND_MAX_ARGC ) )
	{
		char *pArgvBuf = &m_ArgvBuffer[nArgvBufferSize];
		int nMaxLen = COMMAND_MAX_LENGTH - nArgvBufferSize;
		int nStartGet = bufParse.TellGet();
		int	nSize = bufParse.ParseToken( pBreakSet, pArgvBuf, nMaxLen );
		if ( nSize < 0 )
			break;

		// Check for overflow condition
		if ( nMaxLen == nSize )
		{
			Reset();
			return false;
		}

		if ( m_Args.Count() == 1 )
		{
			// Deal with the case where the arguments were quoted
			m_nArgv0Size = bufParse.TellGet();
			bool bFoundEndQuote = m_ArgSBuffer[m_nArgv0Size-1] == '\"';
			if ( bFoundEndQuote )
			{
				--m_nArgv0Size;
			}
			m_nArgv0Size -= nSize;
			Assert( m_nArgv0Size != 0 );

			// The StartGet check is to handle this case: "foo"bar
			// which will parse into 2 different args. ArgS should point to bar.
			bool bFoundStartQuote = ( m_nArgv0Size > nStartGet ) && ( m_ArgSBuffer[m_nArgv0Size-1] == '\"' );
			Assert( bFoundEndQuote == bFoundStartQuote );
			if ( bFoundStartQuote )
			{
				--m_nArgv0Size;
			}
		}

		m_Args.AddToTail( pArgvBuf );
		if( m_Args.Count() >= COMMAND_MAX_ARGC )
		{
			Warning( "CCommand::Tokenize: Encountered command which overflows the argument buffer.. Clamped!\n" );
		}

		nArgvBufferSize += nSize + 1;
		Assert( nArgvBufferSize <= COMMAND_MAX_LENGTH );
	}

	return true;
}


//-----------------------------------------------------------------------------
// Helper function to parse arguments to commands.
//-----------------------------------------------------------------------------
int CCommand::FindArg( const char *pName ) const
{
	int nArgC = ArgC();
	for ( int i = 1; i < nArgC; i++ )
	{
		if ( !V_stricmp_fast( Arg(i), pName ) )
			return (i+1) < nArgC ? i+1 : -1;
	}
	return -1;
}

int CCommand::FindArgInt( const char *pName, int nDefaultVal ) const
{
	int idx = FindArg( pName );
	if ( idx != -1 )
		return V_atoi( m_Args[idx] );
	else
		return nDefaultVal;
}


//-----------------------------------------------------------------------------
// Default console command autocompletion function 
//-----------------------------------------------------------------------------
int DefaultCompletionFunc( const char *partial, CUtlVector< CUtlString > &commands )
{
	return 0;
}


ConCommand::ConCommand( ConCommandRefAbstract *pReferenceOut, const char *pName, FnCommandCallback_t callback, const char *pHelpString /*= 0*/, int64 flags /*= 0*/, FnCommandCompletionCallback completionFunc /*= 0*/ )
{
	m_Callback.m_fnCommandCallback = callback;
	m_Callback.m_bUsingCommandCallbackInterface = false;
	m_Callback.m_bHasVoidCommandCallback = false;
	m_Callback.m_bHasContextlessCommandCallback = false;

	m_fnCompletionCallback = completionFunc ? completionFunc : DefaultCompletionFunc;
	m_bHasCompletionCallback = completionFunc != 0 ? true : false;
	m_bUsingCommandCompletionInterface = false;

	m_pReference = pReferenceOut;
	m_pReference->handle.Reset();

	// Setup the rest
	Create( pName, pHelpString, flags );
}

ConCommand::ConCommand( ConCommandRefAbstract *pReferenceOut, const char *pName, FnCommandCallbackVoid_t callback, const char *pHelpString /*= 0*/, int64 flags /*= 0*/, FnCommandCompletionCallback completionFunc /*= 0*/ )
{
	m_Callback.m_fnVoidCommandCallback = callback;
	m_Callback.m_bUsingCommandCallbackInterface = false;
	m_Callback.m_bHasVoidCommandCallback = true;
	m_Callback.m_bHasContextlessCommandCallback = false;

	m_fnCompletionCallback = completionFunc ? completionFunc : DefaultCompletionFunc;
	m_bHasCompletionCallback = completionFunc != 0 ? true : false;
	m_bUsingCommandCompletionInterface = false;

	m_pReference = pReferenceOut;
	m_pReference->handle.Reset();

	// Setup the rest
	Create( pName, pHelpString, flags );
}

ConCommand::ConCommand( ConCommandRefAbstract *pReferenceOut, const char *pName, FnCommandCallbackNoContext_t callback, const char *pHelpString /*= 0*/, int64 flags /*= 0*/, FnCommandCompletionCallback completionFunc /*= 0*/ )
{
	m_Callback.m_fnContextlessCommandCallback = callback;
	m_Callback.m_bUsingCommandCallbackInterface = false;
	m_Callback.m_bHasVoidCommandCallback = false;
	m_Callback.m_bHasContextlessCommandCallback = true;

	m_fnCompletionCallback = completionFunc ? completionFunc : DefaultCompletionFunc;
	m_bHasCompletionCallback = completionFunc != 0 ? true : false;
	m_bUsingCommandCompletionInterface = false;

	m_pReference = pReferenceOut;
	m_pReference->handle.Reset();

	// Setup the rest
	Create(pName, pHelpString, flags);
}

ConCommand::ConCommand( ConCommandRefAbstract *pReferenceOut, const char *pName, ICommandCallback *pCallback, const char *pHelpString /*= 0*/, int64 flags /*= 0*/, ICommandCompletionCallback *pCompletionCallback /*= 0*/ )
{
	m_Callback.m_pCommandCallback = pCallback;
	m_Callback.m_bUsingCommandCallbackInterface = true;
	m_Callback.m_bHasVoidCommandCallback = false;
	m_Callback.m_bHasContextlessCommandCallback = false;

	m_pCommandCompletionCallback = pCompletionCallback;
	m_bHasCompletionCallback = true;
	m_bUsingCommandCompletionInterface = true;

	m_pReference = pReferenceOut;
	m_pReference->handle.Reset();

	// Setup the rest
	Create( pName, pHelpString, flags );
}

//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
ConCommand::~ConCommand( void )
{
	ConCommandRefAbstract *pRef = GetRef();
	if ( pRef )
	{
		pRef->handle.Unregister();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Used internally by OneTimeInit to initialize.
//-----------------------------------------------------------------------------
void ConCommand::Init()
{
	ConCommandRegList::RegisterCommand( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pName - 
//			callback - 
//			*pHelpString - 
//			flags - 
//-----------------------------------------------------------------------------
void ConCommand::Create( const char* pName, const char* pHelpString /*= 0*/, int64 flags /*= 0*/ )
{
	static const char* empty_string = "";

	// Name should be static data
	Assert(pName);
	m_pszName = pName;
	m_pszHelpString = pHelpString ? pHelpString : empty_string;

	m_nFlags = flags;

#ifdef ALLOW_DEVELOPMENT_CVARS
	m_nFlags &= ~FCVAR_DEVELOPMENTONLY;
#endif

	Init();
}

void ConCommand::Shutdown()
{
	GetRef()->handle.Unregister();
}


//-----------------------------------------------------------------------------
// Purpose: Invoke the function if there is one
//-----------------------------------------------------------------------------
void ConCommandHandle::Dispatch( const CCommandContext &context, const CCommand &command )
{
	ConCommand *pCommand = g_pCVar->GetCommand( *this );
	if ( pCommand->m_Callback.m_fnCommandCallback )
	{
		if ( pCommand->m_Callback.m_bUsingCommandCallbackInterface )
		{
			pCommand->m_Callback.m_pCommandCallback->CommandCallback( context, command );
		}
		else if ( pCommand->m_Callback.m_bHasVoidCommandCallback )
		{
			pCommand->m_Callback.m_fnVoidCommandCallback();
		}
		else if ( pCommand->m_Callback.m_bHasContextlessCommandCallback )
		{
			pCommand->m_Callback.m_fnContextlessCommandCallback( command );
		}
		else
		{
			pCommand->m_Callback.m_fnCommandCallback( context, command );
		}
	}

	// Command without callback!!!
	AssertMsg(0, ("Encountered ConCommand without a callback!\n"));
}

bool ConCommandHandle::HasCallback() const
{
	ConCommand *pCommand = g_pCVar->GetCommand( *this );
	return pCommand->m_Callback.m_fnCommandCallback != nullptr;
}

void ConCommandHandle::Unregister()
{
	if (IsValid())
	{
		if ( g_pCVar )
			g_pCVar->UnregisterConCommand( *this );

		Reset();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Calls the autocompletion method to get autocompletion suggestions
//-----------------------------------------------------------------------------
int	ConCommand::AutoCompleteSuggest( const char *partial, CUtlVector< CUtlString > &commands )
{
	if (m_bUsingCommandCompletionInterface)
	{
		if ( !m_pCommandCompletionCallback )
			return 0;
		return m_pCommandCompletionCallback->CommandCompletionCallback( partial, commands );
	}

	Assert( m_fnCompletionCallback );
	if ( !m_fnCompletionCallback )
		return 0;

	return m_fnCompletionCallback( partial, commands );
}


//-----------------------------------------------------------------------------
// Returns true if the console command can autocomplete 
//-----------------------------------------------------------------------------
bool ConCommand::CanAutoComplete( void )
{
	return m_bHasCompletionCallback;
}


#ifdef CONVAR_WORK_FINISHED
//-----------------------------------------------------------------------------
//
// Console Variables
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Various constructors
//-----------------------------------------------------------------------------
ConVar::ConVar( const char *pName, const char *pDefaultValue, int64 flags /* = 0 */ )
{
	Create( pName, pDefaultValue, flags );
}

ConVar::ConVar( const char *pName, const char *pDefaultValue, int64 flags, const char *pHelpString )
{
	Create( pName, pDefaultValue, flags, pHelpString );
}

ConVar::ConVar( const char *pName, const char *pDefaultValue, int64 flags, const char *pHelpString, bool bMin, float fMin, bool bMax, float fMax )
{
	Create( pName, pDefaultValue, flags, pHelpString, bMin, fMin, bMax, fMax );
}

ConVar::ConVar( const char *pName, const char *pDefaultValue, int64 flags, const char *pHelpString, FnChangeCallback_t callback )
{
	Create( pName, pDefaultValue, flags, pHelpString, false, 0.0, false, 0.0, callback );
}

ConVar::ConVar( const char *pName, const char *pDefaultValue, int64 flags, const char *pHelpString, bool bMin, float fMin, bool bMax, float fMax, FnChangeCallback_t callback )
{
	Create( pName, pDefaultValue, flags, pHelpString, bMin, fMin, bMax, fMax, callback );
}


//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
ConVar::~ConVar( void )
{
	if ( m_Value.m_pszString )
	{
		delete[] m_Value.m_pszString;
		m_Value.m_pszString = NULL;
	}
}


//-----------------------------------------------------------------------------
// Install a change callback (there shouldn't already be one....)
//-----------------------------------------------------------------------------
void ConVar::InstallChangeCallback( FnChangeCallback_t callback, bool bInvoke )
{
	if (callback)
	{
		if (m_fnChangeCallbacks.Find(callback) != -1)
		{
			m_fnChangeCallbacks.AddToTail(callback);
			if (bInvoke)
				callback(this, m_Value.m_pszString, m_Value.m_fValue);
		}
		else
		{
			Warning("InstallChangeCallback ignoring duplicate change callback!!!\n");
		}
	}
	else
	{
		Warning("InstallChangeCallback called with NULL callback, ignoring!!!\n");
	}
}

bool ConVar::IsFlagSet( int64 flag ) const
{
	return ( flag & m_pParent->m_nFlags ) ? true : false;
}

const char *ConVar::GetHelpText( void ) const
{
	return m_pParent->m_pszHelpString;
}

void ConVar::AddFlags( int64 flags )
{
	m_pParent->m_nFlags |= flags;

#ifdef ALLOW_DEVELOPMENT_CVARS
	m_pParent->m_nFlags &= ~FCVAR_DEVELOPMENTONLY;
#endif
}

int64 ConVar::GetFlags( void ) const
{
	return m_pParent->m_nFlags;
}

const char *ConVar::GetName( void ) const
{
	return m_pParent->m_pszName;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
//-----------------------------------------------------------------------------
void ConVar::Init()
{
	BaseClass::Init();
}

const char *ConVar::GetBaseName( void ) const
{
	return m_pParent->m_pszName;
}

int ConVar::GetSplitScreenPlayerSlot( void ) const
{
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *value - 
//-----------------------------------------------------------------------------
void ConVar::InternalSetValue( const char *value )
{
	float fNewValue;
	char  tempVal[ 32 ];
	char  *val;

	Assert(m_pParent == this); // Only valid for root convars.

	float flOldValue = m_Value.m_fValue;

	val = (char *)value;
	fNewValue = ( float )atof( value );

	if ( ClampValue( fNewValue ) )
	{
		Q_snprintf( tempVal,sizeof(tempVal), "%f", fNewValue );
		val = tempVal;
	}
	
	// Redetermine value
	m_Value.m_fValue		= fNewValue;
	m_Value.m_nValue		= ( int )( fNewValue );

	if ( !( m_nFlags & FCVAR_NEVER_AS_STRING ) )
	{
		ChangeStringValue( val, flOldValue );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *tempVal - 
//-----------------------------------------------------------------------------
void ConVar::ChangeStringValue( const char *tempVal, float flOldValue )
{
	Assert( !( m_nFlags & FCVAR_NEVER_AS_STRING ) );

 	char* pszOldValue = (char*)stackalloc( m_Value.m_StringLength );
	memcpy( pszOldValue, m_Value.m_pszString, m_Value.m_StringLength );
	
	int len = Q_strlen(tempVal) + 1;

	if ( len > m_Value.m_StringLength)
	{
		if (m_Value.m_pszString)
		{
			delete[] m_Value.m_pszString;
		}

		m_Value.m_pszString	= new char[len];
		m_Value.m_StringLength = len;
	}

	memcpy( m_Value.m_pszString, tempVal, len );

	// Invoke any necessary callback function
	for (int i = 0; i < m_fnChangeCallbacks.Count(); i++)
	{
		m_fnChangeCallbacks[i]( this, pszOldValue, flOldValue );
	}

	if (g_pCVar)
		g_pCVar->CallGlobalChangeCallbacks( this, pszOldValue, flOldValue );
}

//-----------------------------------------------------------------------------
// Purpose: Check whether to clamp and then perform clamp
// Input  : value - 
// Output : Returns true if value changed
//-----------------------------------------------------------------------------
bool ConVar::ClampValue( float& value )
{
	if ( m_bHasMin && ( value < m_fMinVal ) )
	{
		value = m_fMinVal;
		return true;
	}
	
	if ( m_bHasMax && ( value > m_fMaxVal ) )
	{
		value = m_fMaxVal;
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *value - 
//-----------------------------------------------------------------------------
void ConVar::InternalSetFloatValue( float fNewValue )
{
	if ( fNewValue == m_Value.m_fValue )
		return;

	Assert( m_pParent == this ); // Only valid for root convars.

	// Check bounds
	ClampValue( fNewValue );

	// Redetermine value
	float flOldValue = m_Value.m_fValue;
	m_Value.m_fValue		= fNewValue;
	m_Value.m_nValue		= ( int )fNewValue;

	if ( !( m_nFlags & FCVAR_NEVER_AS_STRING ) )
	{
		char tempVal[ 32 ];
		Q_snprintf( tempVal, sizeof( tempVal), "%f", m_Value.m_fValue );
		ChangeStringValue( tempVal, flOldValue );
	}
	else
	{
		Assert( m_fnChangeCallbacks.Count() == 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *value - 
//-----------------------------------------------------------------------------
void ConVar::InternalSetIntValue( int nValue )
{
	if ( nValue == m_Value.m_nValue )
		return;

	Assert( m_pParent == this ); // Only valid for root convars.

	float fValue = (float)nValue;
	if ( ClampValue( fValue ) )
	{
		nValue = ( int )( fValue );
	}

	// Redetermine value
	float flOldValue = m_Value.m_fValue;
	m_Value.m_fValue		= fValue;
	m_Value.m_nValue		= nValue;

	if ( !( m_nFlags & FCVAR_NEVER_AS_STRING ) )
	{
		char tempVal[ 32 ];
		Q_snprintf( tempVal, sizeof( tempVal ), "%d", m_Value.m_nValue );
		ChangeStringValue( tempVal, flOldValue );
	}
	else
	{
		Assert( m_fnChangeCallbacks.Count() == 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *value - 
//-----------------------------------------------------------------------------
void ConVar::InternalSetColorValue( Color cValue )
{
	int color = cValue.GetRawColor();
	InternalSetIntValue( color );
}

//-----------------------------------------------------------------------------
// Purpose: Private creation
//-----------------------------------------------------------------------------
void ConVar::Create( const char *pName, const char *pDefaultValue, int64 flags /*= 0*/,
	const char *pHelpString /*= NULL*/, bool bMin /*= false*/, float fMin /*= 0.0*/,
	bool bMax /*= false*/, float fMax /*= false*/, FnChangeCallback_t callback /*= NULL*/ )
{
	static const char *empty_string = "";

	m_pParent = this;

	// Name should be static data
	m_pszDefaultValue = pDefaultValue ? pDefaultValue : empty_string;
	Assert( m_pszDefaultValue );

	m_Value.m_StringLength = strlen( m_pszDefaultValue ) + 1;
	m_Value.m_pszString = new char[m_Value.m_StringLength];
	memcpy( m_Value.m_pszString, m_pszDefaultValue, m_Value.m_StringLength );
	
	m_bHasMin = bMin;
	m_fMinVal = fMin;
	m_bHasMax = bMax;
	m_fMaxVal = fMax;
	
	if (callback)
		m_fnChangeCallbacks.AddToTail(callback);

	m_Value.m_fValue = ( float )atof( m_Value.m_pszString );

	// Bounds Check, should never happen, if it does, no big deal
	if ( m_bHasMin && ( m_Value.m_fValue < m_fMinVal ) )
	{
		Assert( 0 );
	}

	if ( m_bHasMax && ( m_Value.m_fValue > m_fMaxVal ) )
	{
		Assert( 0 );
	}

	m_Value.m_nValue = ( int )m_Value.m_fValue;

	BaseClass::Create( pName, pHelpString, flags );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *value - 
//-----------------------------------------------------------------------------
void ConVar::SetValue(const char *value)
{
	ConVar *var = ( ConVar * )m_pParent;
	var->InternalSetValue( value );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : value - 
//-----------------------------------------------------------------------------
void ConVar::SetValue( float value )
{
	ConVar *var = ( ConVar * )m_pParent;
	var->InternalSetFloatValue( value );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : value - 
//-----------------------------------------------------------------------------
void ConVar::SetValue( int value )
{
	ConVar *var = ( ConVar * )m_pParent;
	var->InternalSetIntValue( value );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : value - 
//-----------------------------------------------------------------------------
void ConVar::SetValue( Color value )
{
	ConVar *var = ( ConVar * )m_pParent;
	var->InternalSetColorValue( value );
}

//-----------------------------------------------------------------------------
// Purpose: Reset to default value
//-----------------------------------------------------------------------------
void ConVar::Revert( void )
{
	// Force default value again
	ConVar *var = ( ConVar * )m_pParent;
	var->SetValue( var->m_pszDefaultValue );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : minVal - 
// Output : true if there is a min set
//-----------------------------------------------------------------------------
bool ConVar::GetMin( float& minVal ) const
{
	minVal = m_pParent->m_fMinVal;
	return m_pParent->m_bHasMin;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : maxVal - 
//-----------------------------------------------------------------------------
bool ConVar::GetMax( float& maxVal ) const
{
	maxVal = m_pParent->m_fMaxVal;
	return m_pParent->m_bHasMax;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *ConVar::GetDefault( void ) const
{
	return m_pParent->m_pszDefaultValue;
}


//-----------------------------------------------------------------------------
// This version is simply used to make reading convars simpler.
// Writing convars isn't allowed in this mode
//-----------------------------------------------------------------------------
class CEmptyConVar : public ConVar
{
public:
	CEmptyConVar() : ConVar( "", "0" ) {}
	// Used for optimal read access
	virtual void SetValue( const char *pValue ) {}
	virtual void SetValue( float flValue ) {}
	virtual void SetValue( int nValue ) {}
	virtual void SetValue( Color cValue ) {}
	virtual const char *GetName( void ) const { return ""; }
	virtual bool IsFlagSet( int nFlags ) const { return false; }
};

static CEmptyConVar s_EmptyConVar;

ConVarRef::ConVarRef( const char *pName )
{
	Init( pName, false );
}

ConVarRef::ConVarRef( const char *pName, bool bIgnoreMissing )
{
	Init( pName, bIgnoreMissing );
}

void ConVarRef::Init( const char *pName, bool bIgnoreMissing )
{
	m_pConVar = g_pCVar ? g_pCVar->FindVar( pName ) : &s_EmptyConVar;
	if ( !m_pConVar )
	{
		m_pConVar = &s_EmptyConVar;
	}
	m_pConVarState = static_cast< ConVar * >( m_pConVar );
	if( !IsValid() )
	{
		static bool bFirst = true;
		if ( g_pCVar || bFirst )
		{
			if ( !bIgnoreMissing )
			{
				Warning( "ConVarRef %s doesn't point to an existing ConVar\n", pName );
			}
			bFirst = false;
		}
	}
}

ConVarRef::ConVarRef( IConVar *pConVar )
{
	m_pConVar = pConVar ? pConVar : &s_EmptyConVar;
	m_pConVarState = static_cast< ConVar * >( m_pConVar );
}

bool ConVarRef::IsValid() const
{
	return m_pConVar != &s_EmptyConVar;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ConVar_PrintFlags( const ConCommandBase *var )
{
	bool any = false;
	if ( var->IsFlagSet( FCVAR_GAMEDLL ) )
	{
		ConMsg( " game" );
		any = true;
	}

	if ( var->IsFlagSet( FCVAR_CLIENTDLL ) )
	{
		ConMsg( " client" );
		any = true;
	}

	if ( var->IsFlagSet( FCVAR_ARCHIVE ) )
	{
		ConMsg( " archive" );
		any = true;
	}

	if ( var->IsFlagSet( FCVAR_NOTIFY ) )
	{
		ConMsg( " notify" );
		any = true;
	}

	if ( var->IsFlagSet( FCVAR_SPONLY ) )
	{
		ConMsg( " singleplayer" );
		any = true;
	}

	if ( var->IsFlagSet( FCVAR_NOT_CONNECTED ) )
	{
		ConMsg( " notconnected" );
		any = true;
	}

	if ( var->IsFlagSet( FCVAR_CHEAT ) )
	{
		ConMsg( " cheat" );
		any = true;
	}

	if ( var->IsFlagSet( FCVAR_REPLICATED ) )
	{
		ConMsg( " replicated" );
		any = true;
	}

	if ( var->IsFlagSet( FCVAR_SERVER_CAN_EXECUTE ) )
	{
		ConMsg( " server_can_execute" );
		any = true;
	}

	if ( var->IsFlagSet( FCVAR_CLIENTCMD_CAN_EXECUTE ) )
	{
		ConMsg( " clientcmd_can_execute" );
		any = true;
	}

	if ( any )
	{
		ConMsg( "\n" );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ConVar_PrintDescription( const ConCommandBase *pVar )
{
	bool bMin, bMax;
	float fMin, fMax;
	const char *pStr;

	Assert( pVar );

	Color clr;
	clr.SetColor( 255, 100, 100, 255 );

	if ( !pVar->IsCommand() )
	{
		ConVar *var = ( ConVar * )pVar;
		const ConVar_ServerBounded *pBounded = dynamic_cast<const ConVar_ServerBounded*>( var );

		bMin = var->GetMin( fMin );
		bMax = var->GetMax( fMax );

		const char *value = NULL;
		char tempVal[ 32 ];

		if ( pBounded || var->IsFlagSet( FCVAR_NEVER_AS_STRING ) )
		{
			value = tempVal;
			
			int intVal = pBounded ? pBounded->GetInt() : var->GetInt();
			float floatVal = pBounded ? pBounded->GetFloat() : var->GetFloat();

			if ( fabs( (float)intVal - floatVal ) < 0.000001 )
			{
				Q_snprintf( tempVal, sizeof( tempVal ), "%d", intVal );
			}
			else
			{
				Q_snprintf( tempVal, sizeof( tempVal ), "%f", floatVal );
			}
		}
		else
		{
			value = var->GetString();
		}

		if ( value )
		{
			ConColorMsg( clr, "\"%s\" = \"%s\"", var->GetName(), value );

			if ( stricmp( value, var->GetDefault() ) )
			{
				ConMsg( " ( def. \"%s\" )", var->GetDefault() );
			}
		}

		if ( bMin )
		{
			ConMsg( " min. %f", fMin );
		}
		if ( bMax )
		{
			ConMsg( " max. %f", fMax );
		}

		ConMsg( "\n" );

		// Handled virtualized cvars.
		if ( pBounded && fabs( pBounded->GetFloat() - var->GetFloat() ) > 0.0001f )
		{
			ConColorMsg( clr, "** NOTE: The real value is %.3f but the server has temporarily restricted it to %.3f **\n",
				var->GetFloat(), pBounded->GetFloat() );
		}
	}
	else
	{
		ConCommand *var = ( ConCommand * )pVar;

		ConColorMsg( clr, "\"%s\"\n", var->GetName() );
	}

	ConVar_PrintFlags( pVar );

	pStr = pVar->GetHelpText();
	if ( pStr && pStr[0] )
	{
		ConMsg( " - %s\n", pStr );
	}
}
#endif // CONVAR_WORK_FINISHED
