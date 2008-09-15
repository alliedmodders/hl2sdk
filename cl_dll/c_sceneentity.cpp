//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "networkstringtable_clientdll.h"
#include "dt_utlvector_recv.h"
#include "choreoevent.h"
#include "choreoactor.h"
#include "choreochannel.h"
#include "choreoscene.h"
#include "filesystem.h"
#include "ichoreoeventcallback.h"
#include "scenefilecache/ISceneFileCache.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_SceneEntity : public C_BaseEntity, public IChoreoEventCallback
{
	friend class CChoreoEventCallback;

public:
	DECLARE_CLASS( C_SceneEntity, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	C_SceneEntity( void );
	~C_SceneEntity( void );

	// From IChoreoEventCallback
	virtual void			StartEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );
	virtual void			EndEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );
	virtual void			ProcessEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );
	virtual bool			CheckEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );


	virtual void PostDataUpdate( DataUpdateType_t updateType );
	virtual void PreDataUpdate( DataUpdateType_t updateType );

	virtual void ClientThink();

	void					OnResetClientTime();

private:


	// Scene load/unload
	CChoreoScene			*LoadScene( const char *filename );
	void					LoadSceneFromFile( const char *filename );
	void					UnloadScene( void );

	C_BaseFlex				*FindNamedActor( CChoreoActor *pChoreoActor );

	virtual void			DispatchStartFlexAnimation( CChoreoScene *scene, C_BaseFlex *actor, CChoreoEvent *event );
	virtual void			DispatchEndFlexAnimation( CChoreoScene *scene, C_BaseFlex *actor, CChoreoEvent *event );
	virtual void			DispatchStartExpression( CChoreoScene *scene, C_BaseFlex *actor, CChoreoEvent *event );
	virtual void			DispatchEndExpression( CChoreoScene *scene, C_BaseFlex *actor, CChoreoEvent *event );

	char const				*GetSceneFileName();

	void					DoThink( float frametime );

	void					ClearSceneEvents( CChoreoScene *scene, bool canceled );

private:

	void					CheckQueuedEvents();
	void					WipeQueuedEvents();
	void					QueueStartEvent( float starttime, CChoreoScene *scene, CChoreoEvent *event );

	bool		m_bIsPlayingBack;
	bool		m_bPaused;
	float		m_flCurrentTime;
	float		m_flForceClientTime;
	int			m_nSceneStringIndex;

	CUtlVector< CHandle< C_BaseFlex > > m_hActorList;		

private:
	bool		m_bWasPlaying;

	CChoreoScene *m_pScene;

	struct QueuedEvents_t
	{
		float			starttime;
		CChoreoScene	*scene;
		CChoreoEvent	*event;
	};

	CUtlVector< QueuedEvents_t > m_QueuedEvents;
};

//-----------------------------------------------------------------------------
// Purpose: Decodes animtime and notes when it changes
// Input  : *pStruct - ( C_BaseEntity * ) used to flag animtime is changine
//			*pVarData - 
//			*pIn - 
//			objectID - 
//-----------------------------------------------------------------------------
void RecvProxy_ForcedClientTime( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_SceneEntity *pScene = reinterpret_cast< C_SceneEntity * >( pStruct );
	*(float *)pOut = pData->m_Value.m_Float;
	pScene->OnResetClientTime();
}

#if defined( CSceneEntity )
#undef CSceneEntity
#endif

IMPLEMENT_CLIENTCLASS_DT(C_SceneEntity, DT_SceneEntity, CSceneEntity)
	RecvPropInt(RECVINFO(m_nSceneStringIndex)),
	RecvPropBool(RECVINFO(m_bIsPlayingBack)),
	RecvPropBool(RECVINFO(m_bPaused)),
	RecvPropFloat(RECVINFO(m_flForceClientTime), 0, RecvProxy_ForcedClientTime ),
	RecvPropUtlVector( 
		RECVINFO_UTLVECTOR( m_hActorList ), 
		MAX_ACTORS_IN_SCENE,
		RecvPropEHandle(NULL, 0, 0)),
END_RECV_TABLE()

C_SceneEntity::C_SceneEntity( void )
{
	m_pScene = NULL;
}

C_SceneEntity::~C_SceneEntity( void )
{
	UnloadScene();
}

void C_SceneEntity::OnResetClientTime()
{
	m_flCurrentTime = m_flForceClientTime;
}

char const *C_SceneEntity::GetSceneFileName()
{
	return g_pStringTableClientSideChoreoScenes->GetString( m_nSceneStringIndex );
}

void C_SceneEntity::PostDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PostDataUpdate( updateType );

	char const *str = GetSceneFileName();

	if ( updateType == DATA_UPDATE_CREATED )
	{
		Assert( str && str[ 0 ] );
		if (  str && str[ 0 ] )
		{
			LoadSceneFromFile( str );

			// Kill everything except flex events
			Assert( m_pScene );
			if ( m_pScene )
			{
				int types[ 2 ];
				types[ 0 ] =  CChoreoEvent::FLEXANIMATION;
				types[ 1 ] =  CChoreoEvent::EXPRESSION;
				m_pScene->RemoveEventsExceptTypes( types, 2 );
			}

			SetNextClientThink( CLIENT_THINK_ALWAYS );
		}
	}

	// Playback state changed...
	if ( m_bWasPlaying != m_bIsPlayingBack )
	{
		for(int i = 0; i < m_hActorList.Count() ; ++i )
		{
			C_BaseFlex *actor = m_hActorList[ i ].Get();
			if ( !actor )
				continue;

			Assert( m_pScene );

			if ( m_pScene )
			{
				ClearSceneEvents( m_pScene, false );

				if ( m_bIsPlayingBack )
				{
					m_pScene->ResetSimulation();
					actor->StartChoreoScene( m_pScene );
				}
				else
				{
					m_pScene->ResetSimulation();
					actor->RemoveChoreoScene( m_pScene );
				}
			}
		}
	}
}

void C_SceneEntity::PreDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PreDataUpdate( updateType );

	m_bWasPlaying = m_bIsPlayingBack;
}

//-----------------------------------------------------------------------------
// Purpose: Called every frame that an event is active (Start/EndEvent as also
//  called)
// Input  : *event - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
void C_SceneEntity::ProcessEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	return;
}



//-----------------------------------------------------------------------------
// Purpose: Called for events that are part of a pause condition
// Input  : *event - 
// Output : Returns true on event completed, false on non-completion.
//-----------------------------------------------------------------------------
bool C_SceneEntity::CheckEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	return true;
}

C_BaseFlex *C_SceneEntity::FindNamedActor( CChoreoActor *pChoreoActor )
{
	if ( !m_pScene )
		return NULL;

	int idx = m_pScene->FindActorIndex( pChoreoActor );
	if ( idx < 0 || idx >= m_hActorList.Count() )
		return NULL;

	return m_hActorList[ idx ].Get();
}

//-----------------------------------------------------------------------------
// Purpose: All events are leading edge triggered
// Input  : currenttime - 
//			*event - 
//-----------------------------------------------------------------------------
void C_SceneEntity::StartEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	Assert( event );

	if ( !Q_stricmp( event->GetName(), "NULL" ) )
 	{
 		Scene_Printf( "%s : %8.2f:  ignored %s\n", GetSceneFileName(), currenttime, event->GetDescription() );
 		return;
 	}
 

	C_BaseFlex *pActor = NULL;
	CChoreoActor *actor = event->GetActor();
	if ( actor )
	{
		pActor = FindNamedActor( actor );
		if ( NULL == pActor )
		{
			// This can occur if we haven't been networked an actor yet... we need to queue it so that we can 
			//  fire off the start event as soon as we have the actor resident on the client.
			QueueStartEvent( currenttime, scene, event );
			return;
		}
	}

	Scene_Printf( "%s : %8.2f:  start %s\n", GetSceneFileName(), currenttime, event->GetDescription() );

	switch ( event->GetType() )
	{
	case CChoreoEvent::FLEXANIMATION:
		{
			if ( pActor )
			{
				DispatchStartFlexAnimation( scene, pActor, event );
			}
		}
		break;
	case CChoreoEvent::EXPRESSION:
		{
			if ( pActor )
			{
				DispatchStartExpression( scene, pActor, event );
			}
		}
		break;
	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : currenttime - 
//			*event - 
//-----------------------------------------------------------------------------
void C_SceneEntity::EndEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	Assert( event );

	if ( !Q_stricmp( event->GetName(), "NULL" ) )
 	{
 		return;
 	}

	C_BaseFlex *pActor = NULL;
	CChoreoActor *actor = event->GetActor();
	if ( actor )
	{
		pActor = FindNamedActor( actor );
	}

	Scene_Printf( "%s : %8.2f:  finish %s\n", GetSceneFileName(), currenttime, event->GetDescription() );

	switch ( event->GetType() )
	{
	case CChoreoEvent::FLEXANIMATION:
		{
			if ( pActor )
			{
				DispatchEndFlexAnimation( scene, pActor, event );
			}
		}
		break;
	case CChoreoEvent::EXPRESSION:
		{
			if ( pActor )
			{
				DispatchEndExpression( scene, pActor, event );
			}
		}
		break;
	default:
		break;
	}
}

CChoreoScene *C_SceneEntity::LoadScene( const char *filename )
{
	char loadfile[ 512 ];
	Q_strncpy( loadfile, filename, sizeof( loadfile ) );
	Q_SetExtension( loadfile, ".vcd", sizeof( loadfile ) );
	Q_FixSlashes( loadfile );

	char *buffer = NULL;
	size_t bufsize = scenefilecache->GetSceneBufferSize( loadfile );
	if ( bufsize <= 0 )
		return NULL;

	buffer = new char[ bufsize ];
	if ( !scenefilecache->GetSceneData( filename, (byte *)buffer, bufsize ) )
	{
		delete[] buffer;
		return NULL;
	}

	g_TokenProcessor.SetBuffer( buffer );
	CChoreoScene *scene = ChoreoLoadScene( loadfile, this, &g_TokenProcessor, Scene_Printf );

	delete[] buffer;
	return scene;
}

extern "C" void _stdcall Sleep( unsigned long dwMilliseconds );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *filename - 
//-----------------------------------------------------------------------------
void C_SceneEntity::LoadSceneFromFile( const char *filename )
{
	UnloadScene();

	int sleepCount = 0;
	while ( scenefilecache->IsStillAsyncLoading( filename ) )
	{
		::Sleep( 10 );
		++sleepCount;

		if ( sleepCount > 10 )
		{
			Assert( 0 );
			break;
		}
	}
	m_pScene = LoadScene( filename );
}

void C_SceneEntity::ClearSceneEvents( CChoreoScene *scene, bool canceled )
{
	if ( !m_pScene )
		return;

	Scene_Printf( "%s : %8.2f:  clearing events\n", GetSceneFileName(), m_flCurrentTime );

	int i;
	for ( i = 0 ; i < m_pScene->GetNumActors(); i++ )
	{
		C_BaseFlex *pActor = FindNamedActor( m_pScene->GetActor( i ) );
		if ( !pActor )
			continue;

		// Clear any existing expressions
		pActor->ClearSceneEvents( scene, canceled );
	}

	WipeQueuedEvents();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_SceneEntity::UnloadScene( void )
{
	WipeQueuedEvents();

	if ( m_pScene )
	{
		ClearSceneEvents( m_pScene, false );
		for ( int i = 0 ; i < m_pScene->GetNumActors(); i++ )
		{
			C_BaseFlex *pTestActor = FindNamedActor( m_pScene->GetActor( i ) );

			if ( !pTestActor )
				continue;
		
			pTestActor->RemoveChoreoScene( m_pScene );
		}
	}
	delete m_pScene;
	m_pScene = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//			*event - 
//-----------------------------------------------------------------------------
void C_SceneEntity::DispatchStartFlexAnimation( CChoreoScene *scene, C_BaseFlex *actor, CChoreoEvent *event )
{
	actor->AddSceneEvent( scene, event );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//			*event - 
//-----------------------------------------------------------------------------
void C_SceneEntity::DispatchEndFlexAnimation( CChoreoScene *scene, C_BaseFlex *actor, CChoreoEvent *event )
{
	actor->RemoveSceneEvent( scene, event, false );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//			*event - 
//-----------------------------------------------------------------------------
void C_SceneEntity::DispatchStartExpression( CChoreoScene *scene, C_BaseFlex *actor, CChoreoEvent *event )
{
	actor->AddSceneEvent( scene, event );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//			*event - 
//-----------------------------------------------------------------------------
void C_SceneEntity::DispatchEndExpression( CChoreoScene *scene, C_BaseFlex *actor, CChoreoEvent *event )
{
	actor->RemoveSceneEvent( scene, event, false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_SceneEntity::DoThink( float frametime )
{
	if ( !m_pScene )
		return;

	if ( !m_bIsPlayingBack )
	{
		WipeQueuedEvents();
		return;
	}

	CheckQueuedEvents();

	if ( m_bPaused )
	{
		return;
	}

	// Msg( "CL:  %d, %f for %s\n", gpGlobals->tickcount, m_flCurrentTime, m_pScene->GetFilename() );

	// Tell scene to go
	m_pScene->Think( m_flCurrentTime );
	// Drive simulation time for scene
	m_flCurrentTime += gpGlobals->frametime;
}

void C_SceneEntity::ClientThink()
{
	DoThink( gpGlobals->frametime );
}

void C_SceneEntity::CheckQueuedEvents()
{
// Check for duplicates
	CUtlVector< QueuedEvents_t > events;
	events = m_QueuedEvents;
	m_QueuedEvents.RemoveAll();

	int c = events.Count();
	for ( int i = 0; i < c; ++i )
	{
		const QueuedEvents_t& check = events[ i ];
		
		// Retry starting this event
		StartEvent( check.starttime, check.scene, check.event );
	}
}

void C_SceneEntity::WipeQueuedEvents()
{
	m_QueuedEvents.Purge();
}

void C_SceneEntity::QueueStartEvent( float starttime, CChoreoScene *scene, CChoreoEvent *event )
{
	// Check for duplicates
	int c = m_QueuedEvents.Count();
	for ( int i = 0; i < c; ++i )
	{
		const QueuedEvents_t& check = m_QueuedEvents[ i ];
		if ( check.scene == scene && 
			 check.event == event )
			return;
	}

	QueuedEvents_t qe;
	qe.scene = scene;
	qe.event = event;
	qe.starttime = starttime;
	m_QueuedEvents.AddToTail( qe );
}

CON_COMMAND( scenefilecache_status, "Print current scene file cache status." )
{
	scenefilecache->OutputStatus();
}