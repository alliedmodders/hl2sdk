
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

//-----------------------------------------------------------------------------
// Statically constructed list of ConCommandBases, 
// used for registering them with the ICVar interface
//-----------------------------------------------------------------------------
static int64 s_nCVarFlag = 0;
static bool s_bRegistered = false;

void RegisterCommand( ConCommandCreation_t& cmd )
{
	*cmd.m_pHandle = g_pCVar->RegisterConCommand( cmd, s_nCVarFlag );
	if ( !cmd.m_pHandle->IsValid() )
	{
		Plat_FatalErrorFunc( "RegisterConCommand: Unknown error registering con command \"%s\"!\n", cmd.m_pszName );
		DebuggerBreakIfDebugging( );
	}
}

void UnRegisterCommand( ConCommandHandle& cmd )
{
	if ( cmd.IsValid() )
	{
		g_pCVar->UnregisterConCommand( cmd );

		cmd.Invalidate();
	}
}

class ConCommandRegList
{
public:
	static ConCommandRegList* GetRegList()
	{
		static ConCommandRegList* list = new ConCommandRegList();
		return list;
	}

	static void RegisterAll()
	{
		if (!s_bConCommandsRegistered && g_pCVar)
		{
			s_bConCommandsRegistered = true;

			ConCommandRegList* list = GetRegList();
			FOR_EACH_VEC( list->m_Vec, i )
			{
				RegisterCommand( list->m_Vec[i] );
			}
			delete list;
		}
	}
private:
	friend void AddCommand( ConCommandCreation_t& cmd );

	void Add( const ConCommandCreation_t& cmd )
	{
		m_Vec.AddToTail( cmd );
	}

	CUtlVector<ConCommandCreation_t> m_Vec;
public:
	static bool s_bConCommandsRegistered;
};
bool ConCommandRegList::s_bConCommandsRegistered = false;

void AddCommand( ConCommandCreation_t& cmd )
{
	if (ConCommandRegList::s_bConCommandsRegistered && s_bRegistered)
	{
		RegisterCommand( cmd );
		return;
	}

	ConCommandRegList::GetRegList()->Add( cmd );
}

void RegisterConVar( ConVarCreation_t& cvar )
{
	g_pCVar->RegisterConVar( cvar, s_nCVarFlag, cvar.m_pHandle, cvar.m_pConVarData );
	if (!cvar.m_pHandle->IsValid())
	{
		Plat_FatalErrorFunc( "RegisterConVar: Unknown error registering convar \"%s\"!\n", cvar.m_pszName );
		DebuggerBreakIfDebugging();
	}
}

void UnRegisterConVar( ConVarHandle& cvar )
{
	if (cvar.IsValid())
	{
		g_pCVar->UnregisterConVar( cvar );

		cvar.Invalidate();
	}
}

class ConVarRegList
{
public:
	static ConVarRegList* GetRegList()
	{
		static ConVarRegList* list = new ConVarRegList();
		return list;
	}

	static void RegisterAll()
	{
		if ( !s_bConVarsRegistered && g_pCVar )
		{
			s_bConVarsRegistered = true;

			ConVarRegList* list = GetRegList();
			FOR_EACH_VEC( list->m_Vec, i )
			{
				RegisterConVar( list->m_Vec[i] );
			}
			delete list;
		}
	}

private:
	friend void SetupConVar( ConVarCreation_t& cvar );

	void Add( const ConVarCreation_t& cvar )
	{
		m_Vec.AddToTail( cvar );
	}

	CUtlVector<ConVarCreation_t> m_Vec;
public:
	static bool s_bConVarsRegistered;
};

bool ConVarRegList::s_bConVarsRegistered = false;

void SetupConVar( ConVarCreation_t& cvar )
{
	if ( ConVarRegList::s_bConVarsRegistered )
	{
		RegisterConVar(cvar);
		return;
	}
	
	ConVarRegList::GetRegList()->Add( cvar );
}

//-----------------------------------------------------------------------------
// Called by the framework to register ConCommandBases with the ICVar
//-----------------------------------------------------------------------------
void ConVar_Register( int64 nCVarFlag )
{
	if ( !g_pCVar || s_bRegistered )
	{
		return;
	}

	s_bRegistered = true;
	s_nCVarFlag = nCVarFlag;

	ConCommandRegList::RegisterAll();
	ConVarRegList::RegisterAll();
}

void ConVar_Unregister( )
{
	if ( !g_pCVar || !s_bRegistered )
		return;

	s_bRegistered = false;
}

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

ConCommand::ConCommand( const char *pName, FnCommandCallback_t callback, const char *pHelpString /*= 0*/, int64 flags /*= 0*/, FnCommandCompletionCallback completionFunc /*= 0*/ )
{
	ConCommandCreation_t creation;
	creation.m_fnCallback.m_fnCommandCallback = callback;
	creation.m_fnCallback.m_bIsInterface = false;
	creation.m_fnCallback.m_bIsVoidCallback = false;
	creation.m_fnCallback.m_bIsContextLess = false;

	creation.m_fnCompletionCallback = completionFunc ? completionFunc : DefaultCompletionFunc;
	creation.m_bHasCompletionCallback = completionFunc != 0 ? true : false;
	creation.m_bIsInterface = false;

	// Setup the rest
	Create( pName, pHelpString, flags, creation );
}

ConCommand::ConCommand( const char *pName, FnCommandCallbackVoid_t callback, const char *pHelpString /*= 0*/, int64 flags /*= 0*/, FnCommandCompletionCallback completionFunc /*= 0*/ )
{
	ConCommandCreation_t creation;
	creation.m_fnCallback.m_fnVoidCommandCallback = callback;
	creation.m_fnCallback.m_bIsInterface = false;
	creation.m_fnCallback.m_bIsVoidCallback = true;
	creation.m_fnCallback.m_bIsContextLess = false;

	creation.m_fnCompletionCallback = completionFunc ? completionFunc : DefaultCompletionFunc;
	creation.m_bHasCompletionCallback = completionFunc != nullptr ? true : false;
	creation.m_bIsInterface = false;

	// Setup the rest
	Create( pName, pHelpString, flags, creation );
}

ConCommand::ConCommand( const char *pName, FnCommandCallbackNoContext_t callback, const char *pHelpString /*= 0*/, int64 flags /*= 0*/, FnCommandCompletionCallback completionFunc /*= 0*/ )
{
	ConCommandCreation_t creation;
	creation.m_fnCallback.m_fnContextlessCommandCallback = callback;
	creation.m_fnCallback.m_bIsInterface = false;
	creation.m_fnCallback.m_bIsVoidCallback = false;
	creation.m_fnCallback.m_bIsContextLess = true;

	creation.m_fnCompletionCallback = completionFunc ? completionFunc : DefaultCompletionFunc;
	creation.m_bHasCompletionCallback = completionFunc != nullptr ? true : false;
	creation.m_bIsInterface = false;

	// Setup the rest
	Create( pName, pHelpString, flags, creation );
}

ConCommand::ConCommand( const char *pName, ICommandCallback *pCallback, const char *pHelpString /*= 0*/, int64 flags /*= 0*/, ICommandCompletionCallback *pCompletionCallback /*= 0*/ )
{
	ConCommandCreation_t creation;
	creation.m_fnCallback.m_pCommandCallback = pCallback;
	creation.m_fnCallback.m_bIsInterface = true;
	creation.m_fnCallback.m_bIsVoidCallback = false;
	creation.m_fnCallback.m_bIsContextLess = false;

	creation.m_pCommandCompletionCallback = pCompletionCallback;
	creation.m_bHasCompletionCallback = pCompletionCallback != nullptr ? true : false;
	creation.m_bIsInterface = true;

	// Setup the rest
	Create( pName, pHelpString, flags, creation );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pName - 
//			callback - 
//			*pHelpString - 
//			flags - 
//-----------------------------------------------------------------------------
void ConCommand::Create( const char* pName, const char* pHelpString, int64_t flags, ConCommandCreation_t& setup )
{
	static const char* empty_string = "";

	// Name should be static data
	Assert(pName);
	setup.m_pszName = pName;
	setup.m_pszHelpString = pHelpString ? pHelpString : empty_string;

	setup.m_nFlags = flags;

#ifdef ALLOW_DEVELOPMENT_CVARS
	setup.m_nFlags &= ~FCVAR_DEVELOPMENTONLY;
#endif
	setup.m_pHandle = &this->m_Handle;

	AddCommand( setup );
}

void ConCommand::Destroy()
{
	UnRegisterCommand(this->m_Handle);
}

//-----------------------------------------------------------------------------
//
// Console Variables
//
//-----------------------------------------------------------------------------

#ifdef CONVAR_WORK_FINISHED

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
