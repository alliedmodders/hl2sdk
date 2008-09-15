//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =====//
//
// Purpose: 
//
//===========================================================================//

#include "cbase.h"
#include "toolframework_client.h"
#include "igamesystem.h"
#include "toolframework/iclientenginetools.h"
#include "client_factorylist.h"

class CToolFrameworkClient : public CBaseGameSystemPerFrame
{
public:
	// Methods of IGameSystem
	virtual bool	Init();
	virtual void	LevelInitPreEntity();
	virtual void	LevelInitPostEntity();
	virtual void	LevelShutdownPreEntity();
	virtual void	LevelShutdownPostEntity();
	virtual void	PreRender();
	virtual void	PostRender();

public:
	// Other public methods
	void PostToolMessage( HTOOLHANDLE hEntity, KeyValues *msg );
	void AdjustEngineViewport( int& x, int& y, int& width, int& height );
	bool SetupEngineView( Vector &origin, QAngle &angles, float &fov );
	bool SetupEngineMicrophone( Vector &origin, QAngle &angles );
	bool IsThirdPersonCamera();

private:
	IClientEngineTools	*m_pTools;
};


//-----------------------------------------------------------------------------
// Singleton
//-----------------------------------------------------------------------------
static CToolFrameworkClient g_ToolFrameworkClient;

IGameSystem *ToolFrameworkClientSystem()
{
	return &g_ToolFrameworkClient;
}

bool CToolFrameworkClient::Init()
{
	factorylist_t list;
	FactoryList_Retrieve( list );

	m_pTools = ( IClientEngineTools * )list.appSystemFactory( VCLIENTENGINETOOLS_INTERFACE_VERSION, NULL );
	return ( m_pTools != NULL );
}

void CToolFrameworkClient::LevelInitPreEntity()
{
	if ( m_pTools )
	{
		m_pTools->LevelInitPreEntityAllTools();
	}
}

void CToolFrameworkClient::LevelInitPostEntity()
{
	if ( m_pTools )
	{
		m_pTools->LevelInitPostEntityAllTools();
	}
}

void CToolFrameworkClient::LevelShutdownPreEntity()
{
	if ( m_pTools )
	{
		m_pTools->LevelShutdownPreEntityAllTools();
	}
}

void CToolFrameworkClient::LevelShutdownPostEntity()
{
	if ( m_pTools )
	{
		m_pTools->LevelShutdownPostEntityAllTools();
	}
}

void CToolFrameworkClient::PreRender()
{
	if ( m_pTools )
	{
		m_pTools->PreRenderAllTools();
	}
}

void CToolFrameworkClient::PostRender()
{
	if ( m_pTools )
	{
		m_pTools->PostRenderAllTools();
	}
}


//-----------------------------------------------------------------------------
// Should we render with a 3rd person camera?
//-----------------------------------------------------------------------------
bool CToolFrameworkClient::IsThirdPersonCamera()
{
	if ( !m_pTools )
		return false;
	return m_pTools->IsThirdPersonCamera( );
}

bool ToolFramework_IsThirdPersonCamera( )
{
	return g_ToolFrameworkClient.IsThirdPersonCamera( );
}


//-----------------------------------------------------------------------------
// Posts a message to all tools
//-----------------------------------------------------------------------------
void CToolFrameworkClient::PostToolMessage( HTOOLHANDLE hEntity, KeyValues *msg )
{
	if ( m_pTools )
	{
		m_pTools->PostToolMessage( hEntity, msg );
	}
}

void ToolFramework_PostToolMessage( HTOOLHANDLE hEntity, KeyValues *msg )
{
	g_ToolFrameworkClient.PostToolMessage( hEntity, msg );
}


//-----------------------------------------------------------------------------
// View manipulation
//-----------------------------------------------------------------------------
void CToolFrameworkClient::AdjustEngineViewport( int& x, int& y, int& width, int& height )
{
	if ( m_pTools )
	{
		m_pTools->AdjustEngineViewport( x, y, width, height );
	}
}

void ToolFramework_AdjustEngineViewport( int& x, int& y, int& width, int& height )
{
	g_ToolFrameworkClient.AdjustEngineViewport( x, y, width, height );
}


//-----------------------------------------------------------------------------
// View manipulation
//-----------------------------------------------------------------------------
bool CToolFrameworkClient::SetupEngineView( Vector &origin, QAngle &angles, float &fov )
{
	if ( !m_pTools )
		return false;

	return m_pTools->SetupEngineView( origin, angles, fov );
}

bool ToolFramework_SetupEngineView( Vector &origin, QAngle &angles, float &fov )
{
	return g_ToolFrameworkClient.SetupEngineView( origin, angles, fov );
}

//-----------------------------------------------------------------------------
// microphone manipulation
//-----------------------------------------------------------------------------
bool CToolFrameworkClient::SetupEngineMicrophone( Vector &origin, QAngle &angles )
{
	if ( !m_pTools )
		return false;

	return m_pTools->SetupEngineMicrophone( origin, angles );
}

bool ToolFramework_SetupEngineMicrophone( Vector &origin, QAngle &angles )
{
	return g_ToolFrameworkClient.SetupEngineMicrophone( origin, angles );
}
