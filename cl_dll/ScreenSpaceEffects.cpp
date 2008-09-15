#include "cbase.h"

#include "keyvalues.h"
#include "cdll_client_int.h"
#include "view_scene.h"
#include "viewrender.h"
#include "vstdlib/icommandline.h"
#include "materialsystem/IMesh.h"
#include "materialsystem/IMaterial.h"
#include "materialsystem/IMaterialSystemHardwareConfig.h"
#include "materialsystem/IMaterialVar.h"
#include "materialsystem/IColorCorrection.h"

#include "ScreenSpaceEffects.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar mat_colorcorrection( "mat_colorcorrection", "1" );

//------------------------------------------------------------------------------
// CScreenSpaceEffectRegistration code
// Used to register and effect with the IScreenSpaceEffectManager
//------------------------------------------------------------------------------
CScreenSpaceEffectRegistration *CScreenSpaceEffectRegistration::s_pHead = NULL;

CScreenSpaceEffectRegistration::CScreenSpaceEffectRegistration( const char *pName, IScreenSpaceEffect *pEffect )
{
	m_pEffectName = pName;
	m_pEffect = pEffect;
	m_pNext = s_pHead;
	s_pHead = this;
}


//------------------------------------------------------------------------------
// CScreenSpaceEffectManager - Implementation of IScreenSpaceEffectManager
//------------------------------------------------------------------------------
class CScreenSpaceEffectManager : public IScreenSpaceEffectManager
{
public:

	virtual void InitScreenSpaceEffects( );
	virtual void ShutdownScreenSpaceEffects( );

	virtual IScreenSpaceEffect *GetScreenSpaceEffect( const char *pEffectName );

	virtual void SetScreenSpaceEffectParams( const char *pEffectName, KeyValues *params );
	virtual void SetScreenSpaceEffectParams( IScreenSpaceEffect *pEffect, KeyValues *params );
    
	virtual void EnableScreenSpaceEffect( const char *pEffectName );
	virtual void EnableScreenSpaceEffect( IScreenSpaceEffect *pEffect );

	virtual void DisableScreenSpaceEffect( const char *pEffectName );
	virtual void DisableScreenSpaceEffect( IScreenSpaceEffect *pEffect );

	virtual void DisableAllScreenSpaceEffects( );

	virtual void RenderEffects( int x, int y, int w, int h );
};

CScreenSpaceEffectManager g_ScreenSpaceEffectManager;
IScreenSpaceEffectManager *g_pScreenSpaceEffects = &g_ScreenSpaceEffectManager;

//---------------------------------------------------------------------------------------
// CScreenSpaceEffectManager::InitScreenSpaceEffects - Initialise all registered effects
//---------------------------------------------------------------------------------------
void CScreenSpaceEffectManager::InitScreenSpaceEffects( )
{
	if ( CommandLine()->FindParm( "-colorcorrection" ) || CommandLine()->FindParm( "-tools" ) )
	{
		GetScreenSpaceEffect( "colorcorrection" )->Enable( true );
	}

	if ( CommandLine()->FindParm( "-filmgrain" ) )
	{
		GetScreenSpaceEffect( "filmgrain" )->Enable( true );
	}

	for( CScreenSpaceEffectRegistration *pReg=CScreenSpaceEffectRegistration::s_pHead; pReg!=NULL; pReg=pReg->m_pNext )
	{
		IScreenSpaceEffect *pEffect = pReg->m_pEffect;
		if( pEffect )
		{
			bool bIsEnabled = pEffect->IsEnabled( );
			pEffect->Init( );
			pEffect->Enable( bIsEnabled );
		}
	}
}


//----------------------------------------------------------------------------------------
// CScreenSpaceEffectManager::ShutdownScreenSpaceEffects - Shutdown all registered effects
//----------------------------------------------------------------------------------------
void CScreenSpaceEffectManager::ShutdownScreenSpaceEffects( )
{
	for( CScreenSpaceEffectRegistration *pReg=CScreenSpaceEffectRegistration::s_pHead; pReg!=NULL; pReg=pReg->m_pNext )
	{
		IScreenSpaceEffect *pEffect = pReg->m_pEffect;
		if( pEffect )
		{
			pEffect->Shutdown( );
		}
	}
}


//---------------------------------------------------------------------------------------
// CScreenSpaceEffectManager::GetScreenSpaceEffect - Returns a point to the named effect
//---------------------------------------------------------------------------------------
IScreenSpaceEffect *CScreenSpaceEffectManager::GetScreenSpaceEffect( const char *pEffectName )
{
	for( CScreenSpaceEffectRegistration *pReg=CScreenSpaceEffectRegistration::s_pHead; pReg!=NULL; pReg=pReg->m_pNext )
	{
		if( !Q_stricmp( pReg->m_pEffectName, pEffectName ) )
		{
			IScreenSpaceEffect *pEffect = pReg->m_pEffect;
			return pEffect;
		}
	}

	Warning( "Could not find screen space effect %s\n", pEffectName );

	return NULL;
}


//---------------------------------------------------------------------------------------
// CScreenSpaceEffectManager::SetScreenSpaceEffectParams 
//	- Assign parameters to the specified effect
//---------------------------------------------------------------------------------------
void CScreenSpaceEffectManager::SetScreenSpaceEffectParams( const char *pEffectName, KeyValues *params )
{
	IScreenSpaceEffect *pEffect = GetScreenSpaceEffect( pEffectName );
	if( pEffect )
		SetScreenSpaceEffectParams( pEffect, params );
}

void CScreenSpaceEffectManager::SetScreenSpaceEffectParams( IScreenSpaceEffect *pEffect, KeyValues *params )
{
	if( pEffect )
		pEffect->SetParameters( params );
}


//---------------------------------------------------------------------------------------
// CScreenSpaceEffectManager::EnableScreenSpaceEffect
//	- Enables the specified effect
//---------------------------------------------------------------------------------------
void CScreenSpaceEffectManager::EnableScreenSpaceEffect( const char *pEffectName )
{
	IScreenSpaceEffect *pEffect = GetScreenSpaceEffect( pEffectName );
	if( pEffect )
		EnableScreenSpaceEffect( pEffect );
}

void CScreenSpaceEffectManager::EnableScreenSpaceEffect( IScreenSpaceEffect *pEffect )
{
	if( pEffect )
		pEffect->Enable( true );
}


//---------------------------------------------------------------------------------------
// CScreenSpaceEffectManager::DisableScreenSpaceEffect
//	- Disables the specified effect
//---------------------------------------------------------------------------------------
void CScreenSpaceEffectManager::DisableScreenSpaceEffect( const char *pEffectName )
{
	IScreenSpaceEffect *pEffect = GetScreenSpaceEffect( pEffectName );
	if( pEffect )
		DisableScreenSpaceEffect( pEffect );
}

void CScreenSpaceEffectManager::DisableScreenSpaceEffect( IScreenSpaceEffect *pEffect )
{
	if( pEffect )
		pEffect->Enable( false );
}


//---------------------------------------------------------------------------------------
// CScreenSpaceEffectManager::DisableAllScreenSpaceEffects
//	- Disables all registered screen space effects
//---------------------------------------------------------------------------------------
void CScreenSpaceEffectManager::DisableAllScreenSpaceEffects( )
{
	for( CScreenSpaceEffectRegistration *pReg=CScreenSpaceEffectRegistration::s_pHead; pReg!=NULL; pReg=pReg->m_pNext )
	{
		IScreenSpaceEffect *pEffect = pReg->m_pEffect;
		if( pEffect )
		{
			pEffect->Enable( false );
		}
	}
}


//---------------------------------------------------------------------------------------
// CScreenSpaceEffectManager::RenderEffects
//	- Renders all registered screen space effects
//---------------------------------------------------------------------------------------
void CScreenSpaceEffectManager::RenderEffects( int x, int y, int w, int h )
{
	for( CScreenSpaceEffectRegistration *pReg=CScreenSpaceEffectRegistration::s_pHead; pReg!=NULL; pReg=pReg->m_pNext )
	{
		IScreenSpaceEffect *pEffect = pReg->m_pEffect;
		if( pEffect )
		{
			pEffect->Render( x, y, w, h );
		}
	}
}


//------------------------------------------------------------------------------
// Color correction post-processing effect
//------------------------------------------------------------------------------
class CColorCorrectionEffect : public IScreenSpaceEffect
{
public:
	CColorCorrectionEffect( );
   ~CColorCorrectionEffect( );

	void Init( );
	void Shutdown( );

	void SetParameters( KeyValues *params );

	void Render( int x, int y, int w, int h );

	void Enable( bool bEnable );
	bool IsEnabled( );

private:

	bool				m_bEnable;

	CMaterialReference	m_Material;

	bool				m_bSplitScreen;

};

ADD_SCREENSPACE_EFFECT( CColorCorrectionEffect, colorcorrection );

//------------------------------------------------------------------------------
// CColorCorrectionEffect constructor
//------------------------------------------------------------------------------
CColorCorrectionEffect::CColorCorrectionEffect( )
{
	m_bSplitScreen = false;
}


//------------------------------------------------------------------------------
// CColorCorrectionEffect destructor
//------------------------------------------------------------------------------
CColorCorrectionEffect::~CColorCorrectionEffect( )
{
}


//------------------------------------------------------------------------------
// CColorCorrectionEffect init
//------------------------------------------------------------------------------
void CColorCorrectionEffect::Init( )
{
	m_Material.Init( "engine/colorcorrection", TEXTURE_GROUP_OTHER );

	m_bEnable = false;
}


//------------------------------------------------------------------------------
// CColorCorrectionEffect shutdown
//------------------------------------------------------------------------------
void CColorCorrectionEffect::Shutdown( )
{
	m_Material.Shutdown();
}

//------------------------------------------------------------------------------
// CColorCorrectionEffect enable
//------------------------------------------------------------------------------
void CColorCorrectionEffect::Enable( bool bEnable )
{
	if( g_pMaterialSystemHardwareConfig->GetDXSupportLevel()<90 && bEnable )
	{
		Msg( "Color correction can not be enabled when not in DX9 mode.\n" );
		bEnable = false;
	}
	else
	{
		m_bEnable = bEnable;
	}
}

bool CColorCorrectionEffect::IsEnabled( )
{
	return m_bEnable;
}

//------------------------------------------------------------------------------
// CColorCorrectionEffect SetParameters
//------------------------------------------------------------------------------
void CColorCorrectionEffect::SetParameters( KeyValues *params )
{
	if( params->GetDataType( "split_screen" ) == KeyValues::TYPE_STRING )
	{
		int ival = params->GetInt( "split_screen" );
		m_bSplitScreen = ival?true:false;

	}
}

//------------------------------------------------------------------------------
// CColorCorrectionEffect render
//------------------------------------------------------------------------------
void CColorCorrectionEffect::Render( int x, int y, int w, int h )
{
	// We're releasing the CS:S client before the engine with this interface, so we need to fail gracefully
	if( mat_colorcorrection.GetInt()==0 || !colorcorrection )
	{
		return;
	}

	if( g_pMaterialSystemHardwareConfig->GetDXSupportLevel()<90 )
	{
		mat_colorcorrection.SetValue( 0 );
		return;
	}
	
	colorcorrection->NormalizeWeights();

	if( colorcorrection->GetNumLookups()==0 )
	{
		return;
	}
	
	// Set up shader inputs (there has to be a better way to do this)
	
	int paramCount = m_Material->ShaderParamCount();
	IMaterialVar **pParams = m_Material->GetShaderParams();
	for( int i=0;i<paramCount;i++ )
	{
		IMaterialVar *pVar = pParams[i];
		
		if( !Q_stricmp( pVar->GetName(), "$weight_default" ) )
		{
			pVar->SetFloatValue( colorcorrection->GetLookupWeight(-1) );
		}
		else if( !Q_stricmp( pVar->GetName(), "$weight0" ) )
		{
			pVar->SetFloatValue( colorcorrection->GetLookupWeight(0) );
		}
		else if( !Q_stricmp( pVar->GetName(), "$weight1" ) )
		{
			pVar->SetFloatValue( colorcorrection->GetLookupWeight(1) );
		}
		else if( !Q_stricmp( pVar->GetName(), "$weight2" ) )
		{
			pVar->SetFloatValue( colorcorrection->GetLookupWeight(2) );
		}
		else if( !Q_stricmp( pVar->GetName(), "$weight3" ) )
		{
			pVar->SetFloatValue( colorcorrection->GetLookupWeight(3) );
		}
		else if( !Q_stricmp( pVar->GetName(), "$num_lookups" ) )
		{
			pVar->SetIntValue( colorcorrection->GetNumLookups() );
		}
		else if( !Q_stricmp( pVar->GetName(), "$use_fb_texture" ) )
		{
			pVar->SetIntValue( 1 );
		}
	}

	colorcorrection->ResetLookupWeights();

	// Render Effect
	Rect_t actualRect;
	UpdateScreenEffectTexture( 0, x, y, w, h, false, &actualRect );
	ITexture *pTexture = GetFullFrameFrameBufferTexture( 0 );

	if( m_bSplitScreen )
	{
		w /= 2;
		x += w;
		actualRect.x = actualRect.x + actualRect.width/2;
		actualRect.width = actualRect.width/2;
	}

	// Tempororary hack... Color correction was crashing on the first frame 
	// when run outside the debugger for some mods (DoD). This forces it to skip
	// a frame, ensuring we don't get the weird texture crash we otherwise would.
	// This will be removed when the true cause is found
	static bool bFirstFrame = true;
	if( !bFirstFrame )
	{
		materials->DrawScreenSpaceRectangle( m_Material, x, y, w, h,
												actualRect.x, actualRect.y, actualRect.x+actualRect.width-1, actualRect.y+actualRect.height-1, 
												pTexture->GetActualWidth(), pTexture->GetActualHeight() );
	}
	bFirstFrame = false;
}



//------------------------------------------------------------------------------
// Console Interface
//------------------------------------------------------------------------------
static void EnableColorCorrection( ConVar *var, char const *pOldString )
{
	if( var->GetBool() )
	{
		g_pScreenSpaceEffects->EnableScreenSpaceEffect( "colorcorrection" );
	}
	else
	{
		g_pScreenSpaceEffects->DisableScreenSpaceEffect( "colorcorrection" );
	}
}


static void SetScreenEffectParam()
{
	if( engine->Cmd_Argc() >= 4 )
	{
		const char *effect_type = engine->Cmd_Argv(1);

		int num_params = (engine->Cmd_Argc()-2)/2;
		if( engine->Cmd_Argc() != num_params*2+2 )
		{
			Msg( "Missing hald of key/value pair.\n" );
			return;
		}

		KeyValues *params = new KeyValues( "params" );
		for( int i=0;i<num_params;i++ )
		{
			params->SetString( engine->Cmd_Argv( i*2 + 2 ), engine->Cmd_Argv( i*2 + 3 ) );
		}

		g_pScreenSpaceEffects->SetScreenSpaceEffectParams( effect_type, params );
	}
}

// Console interface for setting screen space effect parameters
// Format:
//		set_effect_param effect_type param_name param_value
//	eg	set_effect_param 1 $noise_scale "255 255 255 0"
static ConCommand set_screen_effect_param( "set_screen_effect_param", SetScreenEffectParam, "Set a parameter for one of the screen space effects.", FCVAR_CHEAT );
