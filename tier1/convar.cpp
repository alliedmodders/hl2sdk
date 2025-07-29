
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
#include "icvar.h"
#include "tier0/dbg.h"
#include "Color.h"
#if defined( _X360 )
#include "xbox/xbox_console.h"
#endif
#include "tier0/memdbgon.h"

// AMNOTE: Define this if you need cvars/concommands to respect flags sanitization rules
// #define SANITIZE_CVAR_FLAGS 1

//-----------------------------------------------------------------------------
// Statically constructed list of ConVars/ConCommands, 
// used for registering them with the ICVar interface
//-----------------------------------------------------------------------------
static FnConVarRegisterCallback s_ConVarRegCB = nullptr;
static FnConCommandRegisterCallback s_ConCommandRegCB = nullptr;
static uint64 s_nCVarFlag = 0;
static bool s_bRegistered = false;

class ConCommandRegList
{
public:
	struct Entry_t
	{
		ConCommandCreation_t m_Info;
		ConCommandRef *m_Command = nullptr;
	};

	static void RegisterConCommand( const Entry_t &cmd )
	{
		*cmd.m_Command = g_pCVar->RegisterConCommand( cmd.m_Info, s_nCVarFlag );
		if(!cmd.m_Command->IsValidRef())
		{
			Plat_FatalErrorFunc( "RegisterConCommand: Unknown error registering con command \"%s\"!\n", cmd.m_Info.m_pszName );
			DebuggerBreakIfDebugging();
		}
		else if(s_ConCommandRegCB)
			s_ConCommandRegCB( cmd.m_Command );
	}

	static void RegisterAll()
	{
		if (!s_bConCommandsRegistered && g_pCVar)
		{
			s_bConCommandsRegistered = true;

			ConCommandRegList *prev = nullptr;
			for(auto list = s_pRoot; list; list = prev)
			{
				for(size_t i = 0; i < list->m_nSize; i++)
				{
					RegisterConCommand( list->m_Entries[i] );
				}

				prev = list->m_pPrev;
				delete list;
			};
		}
	}

	static void AddToList( const Entry_t &cmd )
	{
		if(s_bConCommandsRegistered)
		{
			RegisterConCommand( cmd );
			return;
		}

		auto list = s_pRoot;

		if(!list || list->m_nSize >= (sizeof( m_Entries ) / sizeof( m_Entries[0] )))
		{
			list = new ConCommandRegList;
			list->m_nSize = 0;
			list->m_pPrev = s_pRoot;

			s_pRoot = list;
		}

		list->m_Entries[list->m_nSize++] = cmd;
	}

private:
	uint32 m_nSize;
	Entry_t m_Entries[100];
	ConCommandRegList *m_pPrev;

public:
	static bool s_bConCommandsRegistered;
	static ConCommandRegList *s_pRoot;
};

bool ConCommandRegList::s_bConCommandsRegistered = false;
ConCommandRegList *ConCommandRegList::s_pRoot = nullptr;

void SetupConCommand( ConCommand *cmd, const ConCommandCreation_t& info )
{
	ConCommandRegList::Entry_t entry;
	entry.m_Info = info;
	entry.m_Command = cmd;

	ConCommandRegList::AddToList( entry );
}

void UnRegisterConCommand( ConCommand *cmd )
{
	if(cmd->IsValidRef())
	{
		if(g_pCVar)
			g_pCVar->UnregisterConCommandCallbacks( *cmd );

		cmd->InvalidateRef();
	}
}

class ConVarRegList
{
public:
	struct Entry_t
	{
		ConVarCreation_t m_Info;

		ConVarRefAbstract *m_pConVar = nullptr;
		ConVarData **m_pConVarData = nullptr;
	};

	static void RegisterConVar( const Entry_t &cvar )
	{
		g_pCVar->RegisterConVar( cvar.m_Info, s_nCVarFlag, cvar.m_pConVar, cvar.m_pConVarData );
		if(!cvar.m_pConVar->IsValidRef())
		{
			Plat_FatalErrorFunc( "RegisterConVar: Unknown error registering convar \"%s\"!\n", cvar.m_Info.m_pszName );
			DebuggerBreakIfDebugging();
		}
		// Don't let references pass as a newly registered cvar
		else if(s_ConVarRegCB && (cvar.m_Info.m_nFlags & FCVAR_REFERENCE) == 0)
			s_ConVarRegCB( cvar.m_pConVar );
	}

	static void RegisterAll()
	{
		if(!s_bConVarsRegistered && g_pCVar)
		{
			s_bConVarsRegistered = true;

			ConVarRegList *prev = nullptr;
			for(auto list = s_pRoot; list; list = prev)
			{
				for(size_t i = 0; i < list->m_nSize; i++)
				{
					RegisterConVar( list->m_Entries[i] );
				}

				prev = list->m_pPrev;
				delete list;
			};
		}
	}

	static void AddToList( const Entry_t &cvar )
	{
		if(s_bConVarsRegistered)
		{
			RegisterConVar( cvar );
			return;
		}

		auto list = s_pRoot;

		if(!list || list->m_nSize >= (sizeof( m_Entries ) / sizeof( m_Entries[0] )))
		{
			list = new ConVarRegList;
			list->m_nSize = 0;
			list->m_pPrev = s_pRoot;

			s_pRoot = list;
		}

		list->m_Entries[list->m_nSize++] = cvar;
	}

private:
	uint32 m_nSize;
	Entry_t m_Entries[100];
	ConVarRegList *m_pPrev;

public:
	static bool s_bConVarsRegistered;
	static ConVarRegList *s_pRoot;
};

bool ConVarRegList::s_bConVarsRegistered = false;
ConVarRegList *ConVarRegList::s_pRoot = nullptr;

void SetupConVar( ConVarRefAbstract *cvar, ConVarData **cvar_data, ConVarCreation_t &info )
{
	ConVarRegList::Entry_t entry;
	entry.m_Info = info;
	entry.m_pConVar = cvar;
	entry.m_pConVarData = cvar_data;

	ConVarRegList::AddToList( entry );
}

void UnRegisterConVar( ConVarRef *cvar )
{
	if(cvar->IsValidRef())
	{
		if(g_pCVar)
			g_pCVar->UnregisterConVarCallbacks( *cvar );

		cvar->InvalidateRef();
	}
}

uint64 SanitiseConVarFlags( uint64 flags )
{
#ifdef SANITIZE_CVAR_FLAGS
	if(!CommandLine()->HasParm( "-tools" )
		&& (flags & (FCVAR_DEVELOPMENTONLY
					| FCVAR_ARCHIVE
					| FCVAR_USERINFO
					| FCVAR_CHEAT
					| FCVAR_RELEASE
					| FCVAR_SERVER_CAN_EXECUTE
					| FCVAR_CLIENT_CAN_EXECUTE
					| FCVAR_CLIENTCMD_CAN_EXECUTE)) == 0)
	{
		flags |= FCVAR_DEFENSIVE | FCVAR_DEVELOPMENTONLY;
	}
#endif
	return flags;
}

//-----------------------------------------------------------------------------
// Called by the framework to register ConCommandBases with the ICVar
//-----------------------------------------------------------------------------
void ConVar_Register( uint64 nCVarFlag, FnConVarRegisterCallback cvar_reg_cb, FnConCommandRegisterCallback cmd_reg_cb )
{
	if ( !g_pCVar || s_bRegistered )
	{
		return;
	}

	s_bRegistered = true;
	s_nCVarFlag = nCVarFlag;
	s_ConVarRegCB = cvar_reg_cb;
	s_ConCommandRegCB = cmd_reg_cb;

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
// Tokenizer class
//-----------------------------------------------------------------------------
CCommand::CCommand()
{
	EnsureBuffers();
	Reset();
}

CCommand::CCommand( int nArgC, const char **ppArgV ) : CCommand()
{
	Assert( nArgC > 0 );

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

void CCommand::EnsureBuffers()
{
	m_ArgSBuffer.SetSize( MaxCommandLength() );
	m_ArgvBuffer.SetSize( MaxCommandLength() );
}

void CCommand::Reset()
{
	m_nArgv0Size = 0;
	m_ArgSBuffer.Base()[0] = '\0';
	m_Args.RemoveAll();
}

characterset_t* CCommand::DefaultBreakSet()
{
	return g_pCVar->GetCharacterSet();
}

bool CCommand::Tokenize( CUtlString pCommand, characterset_t *pBreakSet )
{
	if(m_ArgSBuffer.Count() == 0)
		EnsureBuffers();

	Reset();

	if ( pCommand.IsEmpty() )
		return false;

	// Use default break set
	if ( !pBreakSet )
	{
		pBreakSet = DefaultBreakSet();
	}

	// Copy the current command into a temp buffer
	// NOTE: This is here to avoid the pointers returned by DequeueNextCommand
	// to become invalid by calling AddText. Is there a way we can avoid the memcpy?
	int nLen = pCommand.Length();
	if ( nLen >= m_ArgSBuffer.Count() - 1 )
	{
		Warning( "CCommand::Tokenize: Encountered command which overflows the tokenizer buffer.. Skipping!\n" );
		return false;
	}

	memmove( m_ArgSBuffer.Base(), pCommand, nLen + 1 );

	// Parse the current command into the current command buffer
	CUtlBuffer bufParse( m_ArgSBuffer.Base(), nLen, CUtlBuffer::TEXT_BUFFER | CUtlBuffer::READ_ONLY);
	int nArgvBufferSize = 0;
	while ( bufParse.IsValid() )
	{
		char *pArgvBuf = &m_ArgvBuffer[nArgvBufferSize];
		int nMaxLen = m_ArgvBuffer.Count() - nArgvBufferSize;
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
		nArgvBufferSize += nSize + 1;
		
		if(nArgvBufferSize >= m_ArgvBuffer.Count())
			break;
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

ConCommandRef::ConCommandRef( const char *name, bool allow_developer )
{
	*this = g_pCVar->FindConCommand( name, allow_developer );
}

ConCommandData *ConCommandRef::GetRawData()
{
	return g_pCVar->GetConCommandData( *this );
}

void ConCommandRef::Dispatch( const CCommandContext &context, const CCommand &command )
{
	g_pCVar->DispatchConCommand( *this, context, command );
}

void ConCommand::Create( const char* pName, const ConCommandCallbackInfo_t &cb, const char* pHelpString, uint64 flags, const ConCommandCompletionCallbackInfo_t &completion_cb )
{
	// Name should be static data
	Assert(pName);

	ConCommandCreation_t info;
	info.m_pszName = pName;
	info.m_pszHelpString = pHelpString;
	info.m_nFlags = SanitiseConVarFlags( flags );
	info.m_CBInfo = cb;
	info.m_CompletionCBInfo = completion_cb;

	SetupConCommand( this, info );
}

void ConCommand::Destroy()
{
	UnRegisterConCommand( this );
}

//-----------------------------------------------------------------------------
//
// Console Variables
//
//-----------------------------------------------------------------------------

int ConVarData::GetMaxSplitScreenSlots() const
{
	if((m_nFlags & FCVAR_PER_USER) != 0)
		return g_pCVar->GetMaxSplitScreenSlots();

	return 1;
}

CVValue_t *ConVarData::Value( CSplitScreenSlot slot ) const
{
	if(slot.Get() == -1)
		slot = CSplitScreenSlot( 0 );

	if(!IsSlotInRange( slot ))
		return nullptr;

	return (CVValue_t *)&m_Values[GetDataByteSize() * slot.Get()];
}

CVValue_t *ConVarData::ValueOrDefault( CSplitScreenSlot slot ) const
{
	CVValue_t *value = Value( slot );
	return value ? value : m_defaultValue;
}

bool ConVarData::IsAllSetToDefault() const
{
	for(int i = 0; i < GetMaxSplitScreenSlots(); i++)
	{
		if(!IsSetToDefault( i ))
			return false;
	}

	return true;
}

void ConVarData::MinValueToString( CBufferString &buf ) const
{
	if(HasMinValue())
		TypeTraits()->ValueToString( m_minValue, buf );
	else
		buf.Insert( 0, "" );
}

void ConVarData::MaxValueToString( CBufferString &buf ) const
{
	if(HasMaxValue())
		TypeTraits()->ValueToString( m_maxValue, buf );
	else
		buf.Insert( 0, "" );
}

ConVarRef::ConVarRef( const char *name, bool allow_developer )
{
	*this = g_pCVar->FindConVar( name, allow_developer );
}

void ConVarRefAbstract::Init( ConVarRef ref, EConVarType type )
{
	m_ConVarData = nullptr;

	if(g_pCVar)
		m_ConVarData = g_pCVar->GetConVarData( ref );
	
	if(!m_ConVarData)
		InvalidateConVarData( type );
}

void ConVarRefAbstract::InvalidateConVarData( EConVarType type )
{
	InvalidateRef();

	if(type == EConVarType_Invalid)
		m_ConVarData = GetInvalidConVarData( EConVarType_Invalid );
	else
		m_ConVarData = GetCvarTypeTraits( type )->m_InvalidCvarData;
}

void ConVarRefAbstract::CallChangeCallbacks( CSplitScreenSlot slot, CVValue_t *new_value, CVValue_t *prev_value, const char *new_str, const char *prev_str )
{
	if(slot.Get() == -1)
		slot = CSplitScreenSlot( 0 );

	g_pCVar->CallChangeCallback( *this, slot, new_value, prev_value );
	g_pCVar->CallGlobalChangeCallbacks( this, slot, new_str, prev_str );
}

void ConVarRefAbstract::SetOrQueueValueInternal( CSplitScreenSlot slot, CVValue_t *value )
{
	if(m_ConVarData->IsFlagSet( FCVAR_PERFORMING_CALLBACKS ))
		QueueSetValueInternal( slot, value );
	else
		SetValueInternal( slot, value );
}

void ConVarRefAbstract::QueueSetValueInternal( CSplitScreenSlot slot, CVValue_t *value )
{
	if(slot.Get() == -1)
		slot = CSplitScreenSlot( 0 );

	TypeTraits()->Clamp( value, m_ConVarData->MinValue(), m_ConVarData->MaxValue() );

	if(!m_ConVarData->IsEqual( slot, value ))
		g_pCVar->QueueThreadSetValue( this, slot, value );
}

void ConVarRefAbstract::SetValueInternal( CSplitScreenSlot slot, CVValue_t *value )
{
	CVValue_t *curr_value = m_ConVarData->ValueOrDefault( slot );

	CVValue_t prev;
	TypeTraits()->Construct( &prev );

	TypeTraits()->Copy( &prev, *curr_value );
	TypeTraits()->Destruct( curr_value );

	TypeTraits()->Construct( curr_value );
	TypeTraits()->Copy( curr_value, *value );
	m_ConVarData->Clamp( slot );

	if(!m_ConVarData->IsEqual( slot, &prev ))
	{
		CBufferString prev_str, new_str;

		TypeTraits()->ValueToString( &prev, prev_str );
		TypeTraits()->ValueToString( curr_value, new_str );

		m_ConVarData->IncrementTimesChanged();

		CallChangeCallbacks( slot, curr_value, &prev, new_str.Get(), prev_str.Get() );
	}

	TypeTraits()->Destruct( &prev );
}

bool ConVarRefAbstract::SetString( CUtlString string, CSplitScreenSlot slot )
{
	CVValue_t *value = m_ConVarData->Value( slot );

	if(!value)
		return true;
	
	if(GetType() != EConVarType_String)
		string.Trim( "\t\n\v\f\r " );

	CVValue_t new_value;
	TypeTraits()->Construct( &new_value );

	bool success = false;
	if(TypeTraits()->StringToValue( string.Get(), &new_value ))
	{
		SetOrQueueValueInternal( slot, &new_value );
		success = true;
	}

	TypeTraits()->Destruct( &new_value );
	return success;
}

void ConVarRefAbstract::Revert( CSplitScreenSlot slot )
{
	CBufferString buf;
	GetDefaultAsString( buf );
	SetString( buf.Get(), slot );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ConVar_PrintDescription( const ConVarRefAbstract *ref )
{
	Assert( ref );

	static struct
	{
		uint64 m_Flag;
		const char *m_Name;
	} s_FlagsMap[] = {
		{ FCVAR_GAMEDLL, "game" },
		{ FCVAR_CLIENTDLL, "client" },
		{ FCVAR_ARCHIVE, "archive" },
		{ FCVAR_NOTIFY, "notify" },
		{ FCVAR_SPONLY, "singleplayer" },
		{ FCVAR_NOT_CONNECTED, "notconnected" },
		{ FCVAR_CHEAT, "cheat" },
		{ FCVAR_REPLICATED, "replicated" },
		{ FCVAR_SERVER_CAN_EXECUTE, "server_can_execute" },
		{ FCVAR_CLIENTCMD_CAN_EXECUTE, "clientcmd_can_execute" },
		{ FCVAR_USERINFO, "userinfo" },
		{ FCVAR_PER_USER, "per_user" }
	};

	CBufferStringN<4096> desc;
	CBufferString buf;

	ref->GetValueAsString( buf );
	desc.AppendFormat( "\"%s\" = \"%s\"", ref->GetName(), buf.Get() );

	if(!ref->IsSetToDefault())
	{
		ref->GetDefaultAsString( buf );
		desc.AppendFormat( " ( def. \"%s\" )", buf.Get() );
	}

	if(ref->HasMin())
	{
		ref->GetMinAsString( buf );
		desc.AppendFormat( " min. %s", buf.Get() );
	}

	if(ref->HasMax())
	{
		ref->GetMaxAsString( buf );
		desc.AppendFormat( " max. %s", buf.Get() );
	}

	for(size_t i = 0; i < (sizeof( s_FlagsMap ) / sizeof( s_FlagsMap[0] )); i++)
	{
		if(ref->IsFlagSet( s_FlagsMap[i].m_Flag ))
			desc.AppendFormat( " %s", s_FlagsMap[i].m_Name );
	}

	if(ref->HasHelpText())
		ConMsg( "%-120s - %s\n", desc.Get(), ref->GetHelpText() );
	else
		ConMsg( "%-120s\n", desc.Get() );
}
