//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Draws grasses and other small objects  
//
// $Revision: $
// $NoKeywords: $
//===========================================================================//
#include "cbase.h"
#include "DetailObjectSystem.h"
#include "GameBspFile.h"
#include "UtlBuffer.h"
#include "tier1/utlmap.h"
#include "view.h"
#include "ClientMode.h"
#include "IViewRender.h"
#include "BSPTreeData.h"
#include "tier0/vprof.h"
#include "engine/ivmodelinfo.h"
#include "materialsystem/IMesh.h"
#include "model_types.h"
#include "env_detail_controller.h"
#include "vstdlib/icommandline.h"
#include "c_world.h"

#if defined(DOD_DLL) || defined(CSTRIKE_DLL)
#define USE_DETAIL_SHAPES
#endif

#ifdef USE_DETAIL_SHAPES
#include "engine/ivdebugoverlay.h"
#include "playerenumerator.h"
#endif

#include "materialsystem/imaterialsystemhardwareconfig.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define DETAIL_SPRITE_MATERIAL		"detail/detailsprites"

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
struct model_t;


ConVar cl_detaildist( "cl_detaildist", "1200", 0, "Distance at which detail props are no longer visible" );
ConVar cl_detailfade( "cl_detailfade", "400", 0, "Distance across which detail props fade in" );
#if defined( USE_DETAIL_SHAPES ) 
ConVar cl_detail_max_sway( "cl_detail_max_sway", "0", FCVAR_ARCHIVE, "Amplitude of the detail prop sway" );
ConVar cl_detail_avoid_radius( "cl_detail_avoid_radius", "0", FCVAR_ARCHIVE, "radius around detail sprite to avoid players" );
ConVar cl_detail_avoid_force( "cl_detail_avoid_force", "0", FCVAR_ARCHIVE, "force with which to avoid players ( in units, percentage of the width of the detail sprite )" );
ConVar cl_detail_avoid_recover_speed( "cl_detail_avoid_recover_speed", "0", FCVAR_ARCHIVE, "how fast to recover position after avoiding players" );
#endif

// Per detail instance information
struct DetailModelAdvInfo_t
{
	// precaculated angles for shaped sprites
	Vector m_vecAnglesForward[3];
	Vector m_vecAnglesRight[3];		// better to save this mem and calc per sprite ?
	Vector m_vecAnglesUp[3];

	// direction we're avoiding the player
	Vector m_vecCurrentAvoid;

	// yaw to sway on
	float m_flSwayYaw;

	// size of the shape
	float m_flShapeSize;

	int m_iShapeAngle;
	float m_flSwayAmount;

};

//-----------------------------------------------------------------------------
// Detail models
//-----------------------------------------------------------------------------
struct SptrintInfo_t
{
	unsigned short	m_nSpriteIndex;
	float16			m_flScale;
};

class CDetailModel : public IClientUnknown, public IClientRenderable
{
	DECLARE_CLASS_NOBASE( CDetailModel );

public:
	CDetailModel();
	~CDetailModel();


	// Initialization
	bool InitCommon( int index, const Vector& org, const QAngle& angles );
	bool Init( int index, const Vector& org, const QAngle& angles, model_t* pModel, 
		ColorRGBExp32 lighting, int lightstyle, unsigned char lightstylecount, int orientation );

	bool InitSprite( int index, const Vector& org, const QAngle& angles, unsigned short nSpriteIndex, 
		ColorRGBExp32 lighting, int lightstyle, unsigned char lightstylecount, int orientation, float flScale,
		unsigned char type, unsigned char shapeAngle, unsigned char shapeSize, unsigned char swayAmount );

	void SetAlpha( unsigned char alpha ) { m_Alpha = alpha; }


	// IClientUnknown overrides.
public:

	virtual IClientUnknown*		GetIClientUnknown()		{ return this; }
	virtual ICollideable*		GetCollideable()		{ return 0; }		// Static props DO implement this.
	virtual IClientNetworkable*	GetClientNetworkable()	{ return 0; }
	virtual IClientRenderable*	GetClientRenderable()	{ return this; }
	virtual IClientEntity*		GetIClientEntity()		{ return 0; }
	virtual C_BaseEntity*		GetBaseEntity()			{ return 0; }
	virtual IClientThinkable*	GetClientThinkable()	{ return 0; }


	// IClientRenderable overrides.
public:

	virtual int					GetBody() { return 0; }
	virtual const Vector&		GetRenderOrigin( );
	virtual const QAngle&		GetRenderAngles( );
	virtual const matrix3x4_t &	RenderableToWorldTransform();
	virtual bool				ShouldDraw();
	virtual bool				IsTransparent( void );
	virtual const model_t*		GetModel( ) const;
	virtual int					DrawModel( int flags );
	virtual void				ComputeFxBlend( );
	virtual int					GetFxBlend( );
	virtual bool				SetupBones( matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime );
	virtual void				SetupWeights( void );
	virtual void				DoAnimationEvents( void );
	virtual void				GetRenderBounds( Vector& mins, Vector& maxs );
	virtual IPVSNotify*			GetPVSNotifyInterface();
	virtual void				GetRenderBoundsWorldspace( Vector& mins, Vector& maxs );
	virtual bool				ShouldReceiveProjectedTextures( int flags );
	virtual bool				GetShadowCastDistance( float *pDist, ShadowType_t shadowType ) const			{ return false; }
	virtual bool				GetShadowCastDirection( Vector *pDirection, ShadowType_t shadowType ) const	{ return false; }
	virtual bool				UsesFrameBufferTexture();
	virtual bool				LODTest() { return true; }

	virtual ClientShadowHandle_t	GetShadowHandle() const;
	virtual ClientRenderHandle_t&	RenderHandle();
	virtual void				GetShadowRenderBounds( Vector &mins, Vector &maxs, ShadowType_t shadowType );
	virtual bool IsShadowDirty( )			     { return false; }
	virtual void MarkShadowDirty( bool bDirty )  {}
	virtual IClientRenderable *GetShadowParent() { return NULL; }
	virtual IClientRenderable *FirstShadowChild(){ return NULL; }
	virtual IClientRenderable *NextShadowPeer()  { return NULL; }
	virtual ShadowType_t		ShadowCastType() { return SHADOWS_NONE; }
	virtual void CreateModelInstance()			 {}
	virtual ModelInstanceHandle_t GetModelInstance() { return MODEL_INSTANCE_INVALID; }
	virtual int					LookupAttachment( const char *pAttachmentName ) { return -1; }
	virtual bool				GetAttachment( int number, matrix3x4_t &matrix );
	virtual	bool				GetAttachment( int number, Vector &origin, QAngle &angles );
	virtual float *				GetRenderClipPlane( void ) { return NULL; }
	virtual int					GetSkin( void ) { return 0; }

	void GetColorModulation( float* color );

	// Computes the render angles for screen alignment
	void ComputeAngles( void );

	// Calls the correct rendering func
	void DrawSprite( CMeshBuilder &meshBuilder );

	// Returns the number of quads the sprite will draw
	int QuadsToDraw() const;

	// Draw functions for the different types of sprite
	void DrawTypeSprite( CMeshBuilder &meshBuilder );

#ifdef USE_DETAIL_SHAPES
	void DrawTypeShapeCross( CMeshBuilder &meshBuilder );
	void DrawTypeShapeTri( CMeshBuilder &meshBuilder );

	// check for players nearby and angle away from them
	void UpdatePlayerAvoid( void );

	void InitShapedSprite( unsigned char shapeAngle, unsigned char shapeSize, unsigned char swayAmount );
	void InitShapeTri();
	void InitShapeCross();

	void DrawSwayingQuad( CMeshBuilder &meshBuilder, Vector vecOrigin, Vector vecSway, Vector2D texul, Vector2D texlr, unsigned char *color,
		Vector width, Vector height );
#endif

	int GetType() const { return m_Type; }
	unsigned char GetAlpha() const { return m_Alpha; }

	bool IsDetailModelTranslucent();

	// IHandleEntity stubs.
public:
	virtual void SetRefEHandle( const CBaseHandle &handle )	{ Assert( false ); }
	virtual const CBaseHandle& GetRefEHandle() const		{ Assert( false ); return *((CBaseHandle*)0); }

	//---------------------------------
	struct LightStyleInfo_t
	{
		unsigned int	m_LightStyle:24;
		unsigned int	m_LightStyleCount:8;
	};

protected:
	Vector	m_Origin;
	QAngle	m_Angles;

	ColorRGBExp32	m_Color;

	unsigned char	m_Orientation:2;
	unsigned char	m_Type:2;
	unsigned char	m_bHasLightStyle:1;
	unsigned char	m_bFlipped:1;

	unsigned char	m_Alpha;

	static CUtlMap<CDetailModel *, LightStyleInfo_t> gm_LightStylesMap;

#pragma warning( disable : 4201 ) //warning C4201: nonstandard extension used : nameless struct/union
	union
	{
		model_t* m_pModel;
		SptrintInfo_t m_SpriteInfo;
	};
#pragma warning( default : 4201 )

#ifdef USE_DETAIL_SHAPES
	// pointer to advanced properties
	DetailModelAdvInfo_t *m_pAdvInfo;
#endif
};

static ConVar mat_fullbright( "mat_fullbright", "0", FCVAR_CHEAT ); // hook into engine's cvars..
extern ConVar r_DrawDetailProps;


//-----------------------------------------------------------------------------
// Dictionary for detail sprites
//-----------------------------------------------------------------------------
struct DetailPropSpriteDict_t
{
	Vector2D	m_UL;		// Coordinate of upper left
	Vector2D	m_LR;		// Coordinate of lower right
	Vector2D	m_TexUL;	// Texcoords of upper left
	Vector2D	m_TexLR;	// Texcoords of lower left
};


//-----------------------------------------------------------------------------
// Responsible for managing detail objects
//-----------------------------------------------------------------------------
class CDetailObjectSystem : public IDetailObjectSystem, public ISpatialLeafEnumerator
{
public:
	virtual char const *Name() { return "DetailObjectSystem"; }

	// constructor, destructor
	CDetailObjectSystem();
	virtual ~CDetailObjectSystem();

	virtual bool IsPerFrame() { return false; }

	// Init, shutdown
	virtual bool Init()
	{
		m_flDefaultFadeStart = cl_detailfade.GetFloat();
		m_flDefaultFadeEnd = cl_detaildist.GetFloat();
		return true;
	}
	virtual void Shutdown() {}

	// Level init, shutdown
	virtual void LevelInitPreEntity();
	virtual void LevelInitPostEntity();
	virtual void LevelShutdownPreEntity();
	virtual void LevelShutdownPostEntity();

	virtual void OnSave() {}
	virtual void OnRestore() {}
	virtual void SafeRemoveIfDesired() {}

    // Gets a particular detail object
	virtual IClientRenderable* GetDetailModel( int idx );

	// Prepares detail for rendering 
	virtual void BuildDetailObjectRenderLists( );

	// Renders all opaque detail objects in a particular set of leaves
	virtual void RenderOpaqueDetailObjects( int nLeafCount, LeafIndex_t *pLeafList );

	// Renders all translucent detail objects in a particular set of leaves
	virtual void RenderTranslucentDetailObjects( const Vector &viewOrigin, const Vector &viewForward, int nLeafCount, LeafIndex_t *pLeafList );

	// Renders all translucent detail objects in a particular leaf up to a particular point
	virtual void RenderTranslucentDetailObjectsInLeaf( const Vector &viewOrigin, const Vector &viewForward, int nLeaf, const Vector *pVecClosestPoint );

	// Call this before rendering translucent detail objects
	virtual void BeginTranslucentDetailRendering( );

	// Method of ISpatialLeafEnumerator
	bool EnumerateLeaf( int leaf, int context );

	DetailPropLightstylesLump_t& DetailLighting( int i ) { return m_DetailLighting[i]; }
	DetailPropSpriteDict_t& DetailSpriteDict( int i ) { return m_DetailSpriteDict[i]; }

private:
	struct DetailModelDict_t
	{
		model_t* m_pModel;
	};

	struct EnumContext_t
	{
		float m_MaxSqDist;
		float m_FadeSqDist;
		float m_FalloffFactor;
		int	m_BuildWorldListNumber;
	};

	struct SortInfo_t
	{
		int m_nIndex;
		float m_flDistance;
	};

	enum
	{
		MAX_SPRITES_PER_LEAF = 4096
	};

	// Unserialization
	void UnserializeModelDict( CUtlBuffer& buf );
	void UnserializeDetailSprites( CUtlBuffer& buf );
	void UnserializeModels( CUtlBuffer& buf );
	void UnserializeModelLighting( CUtlBuffer& buf );

	// Count the number of detail sprites in the leaf list
	int CountSpritesInLeafList( int nLeafCount, LeafIndex_t *pLeafList );

	// Count the number of detail sprite quads in the leaf list
	int CountSpriteQuadsInLeafList( int nLeafCount, LeafIndex_t *pLeafList );

	// Sorts sprites in back-to-front order
	static int __cdecl SortFunc( const void *arg1, const void *arg2 );
	int SortSpritesBackToFront( int nLeaf, const Vector &viewOrigin, const Vector &viewForward, SortInfo_t *pSortInfo );

	// For fast detail object insertion
	IterationRetval_t EnumElement( int userId, int context );

	CUtlVector<DetailModelDict_t>			m_DetailObjectDict;
	CUtlVector<CDetailModel>				m_DetailObjects;
	CUtlVector<DetailPropSpriteDict_t>		m_DetailSpriteDict;
	CUtlVector<DetailPropLightstylesLump_t>	m_DetailLighting;

	// Necessary to get sprites to batch correctly
	CMaterialReference m_DetailSpriteMaterial;
	CMaterialReference m_DetailWireframeMaterial;

	// State stored off for rendering detail sprites in a single leaf
	int m_nSpriteCount;
	int m_nFirstSprite;
	int m_nSortedLeaf;
	SortInfo_t m_pSortInfo[MAX_SPRITES_PER_LEAF];

	float m_flDefaultFadeStart;
	float m_flDefaultFadeEnd;
};


//-----------------------------------------------------------------------------
// System for dealing with detail objects
//-----------------------------------------------------------------------------
static CDetailObjectSystem s_DetailObjectSystem;

IDetailObjectSystem* DetailObjectSystem()
{
	return &s_DetailObjectSystem;
}



//-----------------------------------------------------------------------------
// Initialization
//-----------------------------------------------------------------------------

CUtlMap<CDetailModel *, CDetailModel::LightStyleInfo_t> CDetailModel::gm_LightStylesMap( DefLessFunc( CDetailModel * ) );

bool CDetailModel::InitCommon( int index, const Vector& org, const QAngle& angles )
{
	VectorCopy( org, m_Origin );
	VectorCopy( angles, m_Angles );
	m_Alpha = 255;

	return true;
}


//-----------------------------------------------------------------------------
// Inline methods
//-----------------------------------------------------------------------------

// NOTE: If DetailPropType_t enum changes, change CDetailModel::QuadsToDraw
static int s_pQuadCount[4] =
{
	0, //DETAIL_PROP_TYPE_MODEL
	1, //DETAIL_PROP_TYPE_SPRITE
	4, //DETAIL_PROP_TYPE_SHAPE_CROSS
	3, //DETAIL_PROP_TYPE_SHAPE_TRI
};

inline int CDetailModel::QuadsToDraw() const
{
	return s_pQuadCount[m_Type];
}

	
//-----------------------------------------------------------------------------
// Data accessors
//-----------------------------------------------------------------------------
const Vector& CDetailModel::GetRenderOrigin( void )
{
	return m_Origin;
}

const QAngle& CDetailModel::GetRenderAngles( void )
{
	return m_Angles;
}

const matrix3x4_t &CDetailModel::RenderableToWorldTransform()
{
	// Setup our transform.
	static matrix3x4_t mat;
	AngleMatrix( GetRenderAngles(), GetRenderOrigin(), mat );
	return mat;
}

bool CDetailModel::GetAttachment( int number, matrix3x4_t &matrix )
{
	MatrixCopy( RenderableToWorldTransform(), matrix );
	return true;
}

bool CDetailModel::GetAttachment( int number, Vector &origin, QAngle &angles )
{
	origin = m_Origin;
	angles = m_Angles;
	return true;
}

bool CDetailModel::IsTransparent( void )
{
	return (m_Alpha < 255) || modelinfo->IsTranslucent(m_pModel);
}

bool CDetailModel::ShouldDraw()
{
	// Don't draw in commander mode
	return g_pClientMode->ShouldDrawDetailObjects();
}

void CDetailModel::GetRenderBounds( Vector& mins, Vector& maxs )
{
	int nModelType = modelinfo->GetModelType( m_pModel );
	if (nModelType == mod_studio || nModelType == mod_brush)
	{
		modelinfo->GetModelRenderBounds( GetModel(), mins, maxs );
	}
	else
	{
		mins.Init( 0,0,0 );
		maxs.Init( 0,0,0 );
	}
}

IPVSNotify* CDetailModel::GetPVSNotifyInterface()
{
	return NULL;
}

void CDetailModel::GetRenderBoundsWorldspace( Vector& mins, Vector& maxs )
{
	DefaultRenderBoundsWorldspace( this, mins, maxs );
}

bool CDetailModel::ShouldReceiveProjectedTextures( int flags )
{
	return false;
}

bool CDetailModel::UsesFrameBufferTexture()
{
	return false;
}

void CDetailModel::GetShadowRenderBounds( Vector &mins, Vector &maxs, ShadowType_t shadowType )
{
	GetRenderBounds( mins, maxs );
}

ClientShadowHandle_t CDetailModel::GetShadowHandle() const
{
	return CLIENTSHADOW_INVALID_HANDLE;
}

ClientRenderHandle_t& CDetailModel::RenderHandle()
{
	AssertMsg( 0, "CDetailModel has no render handle" );
	return *((ClientRenderHandle_t*)NULL);
}	


//-----------------------------------------------------------------------------
// Render setup
//-----------------------------------------------------------------------------
bool CDetailModel::SetupBones( matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime )
{
	if (!m_pModel)
		return false;

	// Setup our transform.
	matrix3x4_t parentTransform;
	const QAngle &vRenderAngles = GetRenderAngles();
	const Vector &vRenderOrigin = GetRenderOrigin();
	AngleMatrix( vRenderAngles, parentTransform );
	parentTransform[0][3] = vRenderOrigin.x;
	parentTransform[1][3] = vRenderOrigin.y;
	parentTransform[2][3] = vRenderOrigin.z;

	// Just copy it on down baby
	studiohdr_t *pStudioHdr = modelinfo->GetStudiomodel( m_pModel );
	for (int i = 0; i < pStudioHdr->numbones; i++) 
	{
		MatrixCopy( parentTransform, pBoneToWorldOut[i] );
	}

	return true;
}

void	CDetailModel::SetupWeights( void )
{
}

void	CDetailModel::DoAnimationEvents( void )
{
}


//-----------------------------------------------------------------------------
// Render baby!
//-----------------------------------------------------------------------------
const model_t* CDetailModel::GetModel( ) const
{
	return m_pModel;
}

int CDetailModel::DrawModel( int flags )
{
	if ((m_Alpha == 0) || (!m_pModel))
		return 0;

	int drawn = modelrender->DrawModel( 
		flags, 
		this,
		MODEL_INSTANCE_INVALID,
		-1,		// no entity index
		m_pModel,
		m_Origin,
		m_Angles,
		0,	// skin
		0,	// body
		0  // hitboxset
		);
	return drawn;
}


//-----------------------------------------------------------------------------
// Determine alpha and blend amount for transparent objects based on render state info
//-----------------------------------------------------------------------------
void CDetailModel::ComputeFxBlend( )
{
	// Do nothing, it's already calculate in our m_Alpha
}

int CDetailModel::GetFxBlend( )
{
	return m_Alpha;
}

//-----------------------------------------------------------------------------
// Detail models stuff
//-----------------------------------------------------------------------------
CDetailModel::CDetailModel()
{
	m_Color.r = m_Color.g = m_Color.b = 255;
	m_Color.exponent = 0;
	m_bFlipped = 0;
	m_bHasLightStyle = 0;

#ifdef USE_DETAIL_SHAPES
	m_pAdvInfo = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
CDetailModel::~CDetailModel()
{
#ifdef USE_DETAIL_SHAPES
	// delete advanced
	if ( m_pAdvInfo )
	{
		delete m_pAdvInfo;
		m_pAdvInfo = NULL;
	}
#endif

	if ( m_bHasLightStyle )
		gm_LightStylesMap.Remove( this );
}


//-----------------------------------------------------------------------------
// Initialization
//-----------------------------------------------------------------------------
bool CDetailModel::Init( int index, const Vector& org, const QAngle& angles, 
	model_t* pModel, ColorRGBExp32 lighting, int lightstyle, unsigned char lightstylecount, 
	int orientation)
{
	m_Color = lighting;
	if ( lightstylecount > 0)
	{
		m_bHasLightStyle = 1;
		int iInfo = gm_LightStylesMap.Insert( this );
		if ( lightstyle >= 0x1000000 || lightstylecount >= 100  )
			Error( "Light style overflow\n" );
		gm_LightStylesMap[iInfo].m_LightStyle = lightstyle;
		gm_LightStylesMap[iInfo].m_LightStyleCount = lightstylecount;
	}
	m_Orientation = orientation;
	m_Type = DETAIL_PROP_TYPE_MODEL;
	m_pModel = pModel;
	return InitCommon( index, org, angles );
}

bool CDetailModel::InitSprite( int index, const Vector& org, const QAngle& angles, unsigned short nSpriteIndex, 
	ColorRGBExp32 lighting, int lightstyle, unsigned char lightstylecount, int orientation, float flScale,
	unsigned char type, unsigned char shapeAngle, unsigned char shapeSize, unsigned char swayAmount )
{
	m_Color = lighting;
	if ( lightstylecount > 0)
	{
		m_bHasLightStyle = 1;
		int iInfo = gm_LightStylesMap.Insert( this );
		if ( lightstyle >= 0x1000000 || lightstylecount >= 100  )
			Error( "Light style overflow\n" );
		gm_LightStylesMap[iInfo].m_LightStyle = lightstyle;
		gm_LightStylesMap[iInfo].m_LightStyleCount = lightstylecount;
	}
	m_Orientation = orientation;
	m_SpriteInfo.m_nSpriteIndex = nSpriteIndex;
	m_Type = type;
	m_SpriteInfo.m_flScale.SetFloat( flScale );

#ifdef USE_DETAIL_SHAPES
	m_pAdvInfo = NULL;
	Assert( type <= 3 );
	// precalculate angles for shapes
	if ( type == DETAIL_PROP_TYPE_SHAPE_TRI || type == DETAIL_PROP_TYPE_SHAPE_CROSS || swayAmount > 0 )
	{
		m_Angles = angles;
		InitShapedSprite( shapeAngle, shapeSize, swayAmount);
	}

#endif

	m_bFlipped = ( (index & 0x1) == 1 );
	return InitCommon( index, org, angles );
}

#ifdef USE_DETAIL_SHAPES
void CDetailModel::InitShapedSprite( unsigned char shapeAngle, unsigned char shapeSize, unsigned char swayAmount )
{
	// Set up pointer to advanced shape properties object ( per instance )
	Assert( m_pAdvInfo == NULL );
	m_pAdvInfo = new DetailModelAdvInfo_t;
	Assert( m_pAdvInfo );

	if ( m_pAdvInfo )
	{
		m_pAdvInfo->m_iShapeAngle = shapeAngle;
		m_pAdvInfo->m_flSwayAmount = (float)swayAmount / 255.0f;
		m_pAdvInfo->m_flShapeSize = (float)shapeSize / 255.0f;
		m_pAdvInfo->m_vecCurrentAvoid = vec3_origin;
		m_pAdvInfo->m_flSwayYaw = random->RandomFloat( 0, 180 );
	}

	switch ( m_Type )
	{
	case DETAIL_PROP_TYPE_SHAPE_TRI:
		InitShapeTri();
		break;

	case DETAIL_PROP_TYPE_SHAPE_CROSS:
		InitShapeCross();
		break;

	default:	// sprite will get here
		break;
	}
}

void CDetailModel::InitShapeTri( void )
{
	// store the three sets of directions
	matrix3x4_t matrix;

	// Convert roll/pitch only to matrix
	AngleMatrix( m_Angles, matrix );

	// calculate the vectors for the three sides so they can be used in the sorting test
	// as well as in drawing
	for ( int i=0; i<3; i++ )
	{		
		// Convert desired rotation to angles
		QAngle anglesRotated( m_pAdvInfo->m_iShapeAngle, i*120, 0 );

		Vector rotForward, rotRight, rotUp;
		AngleVectors( anglesRotated, &rotForward, &rotRight, &rotUp );

		// Rotate direction vectors
		VectorRotate( rotForward, matrix, m_pAdvInfo->m_vecAnglesForward[i] );
		VectorRotate( rotRight, matrix, m_pAdvInfo->m_vecAnglesRight[i] );
		VectorRotate( rotUp, matrix, m_pAdvInfo->m_vecAnglesUp[i] );
	}
}

void CDetailModel::InitShapeCross( void )
{
	AngleVectors( m_Angles,
		&m_pAdvInfo->m_vecAnglesForward[0],
		&m_pAdvInfo->m_vecAnglesRight[0],
		&m_pAdvInfo->m_vecAnglesUp[0] );
}
#endif

//-----------------------------------------------------------------------------
// Color, alpha modulation
//-----------------------------------------------------------------------------
void CDetailModel::GetColorModulation( float *color )
{
	if (mat_fullbright.GetInt() == 1)
	{
		color[0] = color[1] = color[2] = 1.0f;
		return;
	}

	Vector tmp;
	Vector normal( 1, 0, 0);
	engine->ComputeDynamicLighting( m_Origin, &normal, tmp );

	float val = engine->LightStyleValue( 0 );
	color[0] = tmp[0] + val * TexLightToLinear( m_Color.r, m_Color.exponent );
	color[1] = tmp[1] + val * TexLightToLinear( m_Color.g, m_Color.exponent );
	color[2] = tmp[2] + val * TexLightToLinear( m_Color.b, m_Color.exponent );

	// Add in the lightstyles
	if ( m_bHasLightStyle )
	{
		int iInfo = gm_LightStylesMap.Find( this );
		Assert( iInfo != gm_LightStylesMap.InvalidIndex() );
		if ( iInfo != gm_LightStylesMap.InvalidIndex() )
		{
			int nLightStyles = gm_LightStylesMap[iInfo].m_LightStyleCount;
			int iLightStyle = gm_LightStylesMap[iInfo].m_LightStyle;
			for (int i = 0; i < nLightStyles; ++i)
			{
				DetailPropLightstylesLump_t& lighting = s_DetailObjectSystem.DetailLighting( iLightStyle + i );
				val = engine->LightStyleValue( lighting.m_Style );
				if (val != 0)
				{
					color[0] += val * TexLightToLinear( lighting.m_Lighting.r, lighting.m_Lighting.exponent ); 
					color[1] += val * TexLightToLinear( lighting.m_Lighting.g, lighting.m_Lighting.exponent ); 
					color[2] += val * TexLightToLinear( lighting.m_Lighting.b, lighting.m_Lighting.exponent ); 
				}
			}
		}
	}

	// Gamma correct....
	engine->LinearToGamma( color, color );
}


//-----------------------------------------------------------------------------
// Is the model itself translucent, regardless of modulation?
//-----------------------------------------------------------------------------
bool CDetailModel::IsDetailModelTranslucent()
{
	// FIXME: This is only true for my first pass of this feature
	if (m_Type >= DETAIL_PROP_TYPE_SPRITE)
		return true;

	return modelinfo->IsTranslucent(GetModel());
}


//-----------------------------------------------------------------------------
// Computes the render angles for screen alignment
//-----------------------------------------------------------------------------
void CDetailModel::ComputeAngles( void )
{
	switch( m_Orientation )
	{
	case 0:
		break;

	case 1:
		{
			Vector vecDir;
			VectorSubtract( CurrentViewOrigin(), m_Origin, vecDir );
			VectorAngles( vecDir, m_Angles );
		}
		break;

	case 2:
		{
			Vector vecDir;
			VectorSubtract( CurrentViewOrigin(), m_Origin, vecDir );
			vecDir.z = 0.0f;
			VectorAngles( vecDir, m_Angles );
		}
		break;
	}
}


//-----------------------------------------------------------------------------
// Select which rendering func to call
//-----------------------------------------------------------------------------
void CDetailModel::DrawSprite( CMeshBuilder &meshBuilder )
{
	switch( m_Type )
	{
#ifdef USE_DETAIL_SHAPES
	case DETAIL_PROP_TYPE_SHAPE_CROSS:
		DrawTypeShapeCross( meshBuilder );
		break;

	case DETAIL_PROP_TYPE_SHAPE_TRI:
		DrawTypeShapeTri( meshBuilder );
		break;
#endif
	case DETAIL_PROP_TYPE_SPRITE:
		DrawTypeSprite( meshBuilder );
		break;

	default:
		Assert(0);
		break;
	}
}


//-----------------------------------------------------------------------------
// Draws the single sprite type
//-----------------------------------------------------------------------------
void CDetailModel::DrawTypeSprite( CMeshBuilder &meshBuilder )
{
	Assert( m_Type == DETAIL_PROP_TYPE_SPRITE );

	Vector vecColor;
	GetColorModulation( vecColor.Base() );

	unsigned char color[4];
	color[0] = (unsigned char)(vecColor[0] * 255.0f);
	color[1] = (unsigned char)(vecColor[1] * 255.0f);
	color[2] = (unsigned char)(vecColor[2] * 255.0f);
	color[3] = m_Alpha;

	DetailPropSpriteDict_t &dict = s_DetailObjectSystem.DetailSpriteDict( m_SpriteInfo.m_nSpriteIndex );

	Vector vecOrigin, dx, dy;
	AngleVectors( m_Angles, NULL, &dx, &dy );

	Vector2D ul, lr;
	float scale = m_SpriteInfo.m_flScale.GetFloat();
	Vector2DMultiply( dict.m_UL, scale, ul );
	Vector2DMultiply( dict.m_LR, scale, lr );

#ifdef USE_DETAIL_SHAPES
	UpdatePlayerAvoid();

	Vector vecSway = vec3_origin;

	if ( m_pAdvInfo )
	{
		vecSway = m_pAdvInfo->m_vecCurrentAvoid * m_SpriteInfo.m_flScale.GetFloat();
		float flSwayAmplitude = m_pAdvInfo->m_flSwayAmount * cl_detail_max_sway.GetFloat();
		if ( flSwayAmplitude > 0 )
		{
			// sway based on time plus a random seed that is constant for this instance of the sprite
			vecSway += dx * sin(gpGlobals->curtime+m_Origin.x) * flSwayAmplitude;
		}
	}
#endif

	VectorMA( m_Origin, ul.x, dx, vecOrigin );
	VectorMA( vecOrigin, ul.y, dy, vecOrigin );
	dx *= (lr.x - ul.x);
	dy *= (lr.y - ul.y);

	Vector2D texul, texlr;
	texul = dict.m_TexUL;
	texlr = dict.m_TexLR;

	if ( !m_bFlipped )
	{
		texul.x = dict.m_TexLR.x;
		texlr.x = dict.m_TexUL.x;
	}

#ifndef USE_DETAIL_SHAPES
	meshBuilder.Position3fv( vecOrigin.Base() );
#else
	meshBuilder.Position3fv( (vecOrigin+vecSway).Base() );
#endif

	meshBuilder.Color4ubv( color );
	meshBuilder.TexCoord2fv( 0, texul.Base() );
	meshBuilder.AdvanceVertex();

	vecOrigin += dy;
	meshBuilder.Position3fv( vecOrigin.Base() );
	meshBuilder.Color4ubv( color );
	meshBuilder.TexCoord2f( 0, texul.x, texlr.y );
	meshBuilder.AdvanceVertex();

	vecOrigin += dx;
	meshBuilder.Position3fv( vecOrigin.Base() );
	meshBuilder.Color4ubv( color );
	meshBuilder.TexCoord2fv( 0, texlr.Base() );
	meshBuilder.AdvanceVertex();

	vecOrigin -= dy;
#ifndef USE_DETAIL_SHAPES
	meshBuilder.Position3fv( vecOrigin.Base() );
#else
	meshBuilder.Position3fv( (vecOrigin+vecSway).Base() );
#endif
	meshBuilder.Color4ubv( color );
	meshBuilder.TexCoord2f( 0, texlr.x, texul.y );
	meshBuilder.AdvanceVertex();
}

//-----------------------------------------------------------------------------
// draws a procedural model, cross shape
// two perpendicular sprites
//-----------------------------------------------------------------------------
#ifdef USE_DETAIL_SHAPES
void CDetailModel::DrawTypeShapeCross( CMeshBuilder &meshBuilder )
{
	Assert( m_Type == DETAIL_PROP_TYPE_SHAPE_CROSS );

	Vector vecColor;
	GetColorModulation( vecColor.Base() );

	unsigned char color[4];
	color[0] = (unsigned char)(vecColor[0] * 255.0f);
	color[1] = (unsigned char)(vecColor[1] * 255.0f);
	color[2] = (unsigned char)(vecColor[2] * 255.0f);
	color[3] = m_Alpha;

	DetailPropSpriteDict_t &dict = s_DetailObjectSystem.DetailSpriteDict( m_SpriteInfo.m_nSpriteIndex );

	Vector2D texul, texlr;
	texul = dict.m_TexUL;
	texlr = dict.m_TexLR;

	// What a shameless re-use of bits (m_pModel == 0 when it should be flipped horizontally)
	if ( !m_pModel )
	{
		texul.x = dict.m_TexLR.x;
		texlr.x = dict.m_TexUL.x;
	}

	Vector2D texumid, texlmid;
	texumid.y = texul.y;
	texlmid.y = texlr.y;
	texumid.x = texlmid.x = ( texul.x + texlr.x ) / 2;

	Vector2D texll;
	texll.x = texul.x;
	texll.y = texlr.y;

	Vector2D ul, lr;
	float flScale = m_SpriteInfo.m_flScale.GetFloat();
	Vector2DMultiply( dict.m_UL, flScale, ul );
	Vector2DMultiply( dict.m_LR, flScale, lr );

	float flSizeX = ( lr.x - ul.x ) / 2;
	float flSizeY = ( lr.y - ul.y );

	UpdatePlayerAvoid();

	// sway based on time plus a random seed that is constant for this instance of the sprite
	Vector vecSway = ( m_pAdvInfo->m_vecCurrentAvoid * flSizeX * 2 );
	float flSwayAmplitude = m_pAdvInfo->m_flSwayAmount * cl_detail_max_sway.GetFloat();
	if ( flSwayAmplitude > 0 )
	{
		vecSway += UTIL_YawToVector( m_pAdvInfo->m_flSwayYaw ) * sin(gpGlobals->curtime+m_Origin.x) * flSwayAmplitude;
	}

	Vector vecOrigin;
	VectorMA( m_Origin, ul.y, m_pAdvInfo->m_vecAnglesUp[0], vecOrigin );

	Vector forward, right, up;
	forward = m_pAdvInfo->m_vecAnglesForward[0] * flSizeX;
	right = m_pAdvInfo->m_vecAnglesRight[0] * flSizeX;
	up = m_pAdvInfo->m_vecAnglesUp[0] * flSizeY;

	// figure out drawing order so the branches sort properly
	// do dot products with the forward and right vectors to determine the quadrant the viewer is in
	// assume forward points North , right points East
	/*
			   N
			   |
			3  |  0
		  W---------E
			2  |  1
			   |
			   S
	*/ 
	// eg if they are in quadrant 0, set iBranch to 0, and the draw order will be
	// 0, 1, 2, 3, or South, west, north, east
	Vector viewOffset = CurrentViewOrigin() - m_Origin;
	bool bForward = ( DotProduct( forward, viewOffset ) > 0 );
	bool bRight = ( DotProduct( right, viewOffset ) > 0 );	
	int iBranch = bForward ? ( bRight ? 0 : 3 ) : ( bRight ? 1 : 2 );

	//debugoverlay->AddLineOverlay( m_Origin, m_Origin + right * 20, 255, 0, 0, true, 0.01 );
	//debugoverlay->AddLineOverlay( m_Origin, m_Origin + forward * 20, 0, 0, 255, true, 0.01 );

	int iDrawn = 0;
	while( iDrawn < 4 )
	{
		switch( iBranch )
		{
		case 0:		// south
			DrawSwayingQuad( meshBuilder, vecOrigin, vecSway, texumid, texlr, color, -forward, up );
			break;
		case 1:		// west
			DrawSwayingQuad( meshBuilder, vecOrigin, vecSway, texumid, texll, color, -right, up );
			break;
		case 2:		// north
			DrawSwayingQuad( meshBuilder, vecOrigin, vecSway, texumid, texll, color, forward, up );
			break;
		case 3:		// east
			DrawSwayingQuad( meshBuilder, vecOrigin, vecSway, texumid, texlr, color, right, up );
			break;
		}

		iDrawn++;
		iBranch++;
		if ( iBranch > 3 )
			iBranch = 0;
	}	
}
#endif

//-----------------------------------------------------------------------------
// draws a procedural model, tri shape
//-----------------------------------------------------------------------------
#ifdef USE_DETAIL_SHAPES
void CDetailModel::DrawTypeShapeTri( CMeshBuilder &meshBuilder )
{
	Assert( m_Type == DETAIL_PROP_TYPE_SHAPE_TRI );

	Vector vecColor;
	GetColorModulation( vecColor.Base() );

	unsigned char color[4];
	color[0] = (unsigned char)(vecColor[0] * 255.0f);
	color[1] = (unsigned char)(vecColor[1] * 255.0f);
	color[2] = (unsigned char)(vecColor[2] * 255.0f);
	color[3] = m_Alpha;

	DetailPropSpriteDict_t &dict = s_DetailObjectSystem.DetailSpriteDict( m_SpriteInfo.m_nSpriteIndex );

	Vector2D texul, texlr;
	texul = dict.m_TexUL;
	texlr = dict.m_TexLR;

	// What a shameless re-use of bits (m_pModel == 0 when it should be flipped horizontally)
	if ( !m_pModel )
	{
		texul.x = dict.m_TexLR.x;
		texlr.x = dict.m_TexUL.x;
	}

	Vector2D ul, lr;
	float flScale = m_SpriteInfo.m_flScale.GetFloat();
	Vector2DMultiply( dict.m_UL, flScale, ul );
	Vector2DMultiply( dict.m_LR, flScale, lr );

	// sort the sides relative to the view origin
	Vector viewOffset = CurrentViewOrigin() - m_Origin;

	// three sides, A, B, C, counter-clockwise from A is the unrotated side
	bool bOutsideA = DotProduct( m_pAdvInfo->m_vecAnglesForward[0], viewOffset ) > 0;
	bool bOutsideB = DotProduct( m_pAdvInfo->m_vecAnglesForward[1], viewOffset ) > 0;
	bool bOutsideC = DotProduct( m_pAdvInfo->m_vecAnglesForward[2], viewOffset ) > 0;

	int iBranch = 0;
	if ( bOutsideA && !bOutsideB )
		iBranch = 1;
	else if ( bOutsideB && !bOutsideC )
		iBranch = 2;

	float flHeight, flWidth;
	flHeight = (lr.y - ul.y);
	flWidth = (lr.x - ul.x);

	Vector vecSway;
	Vector vecOrigin;
	Vector vecHeight, vecWidth;

	UpdatePlayerAvoid();

	Vector vecSwayYaw = UTIL_YawToVector( m_pAdvInfo->m_flSwayYaw );
	float flSwayAmplitude = m_pAdvInfo->m_flSwayAmount * cl_detail_max_sway.GetFloat();

	int iDrawn = 0;
	while( iDrawn < 3 )
	{
		vecHeight = m_pAdvInfo->m_vecAnglesUp[iBranch] * flHeight;
		vecWidth = m_pAdvInfo->m_vecAnglesRight[iBranch] * flWidth;

		VectorMA( m_Origin, ul.x, m_pAdvInfo->m_vecAnglesRight[iBranch], vecOrigin );
		VectorMA( vecOrigin, ul.y, m_pAdvInfo->m_vecAnglesUp[iBranch], vecOrigin );
		VectorMA( vecOrigin, m_pAdvInfo->m_flShapeSize*flWidth, m_pAdvInfo->m_vecAnglesForward[iBranch], vecOrigin );

		// sway is calculated per side so they don't sway exactly the same
		Vector vecSway = ( m_pAdvInfo->m_vecCurrentAvoid * flWidth ) + 
			vecSwayYaw * sin(gpGlobals->curtime+m_Origin.x+iBranch) * flSwayAmplitude;

		DrawSwayingQuad( meshBuilder, vecOrigin, vecSway, texul, texlr, color, vecWidth, vecHeight );
		
		iDrawn++;
		iBranch++;
		if ( iBranch > 2 )
			iBranch = 0;
	}	
}
#endif

//-----------------------------------------------------------------------------
// checks for nearby players and pushes the detail to the side
//-----------------------------------------------------------------------------
#ifdef USE_DETAIL_SHAPES
void CDetailModel::UpdatePlayerAvoid( void )
{
	float flForce = cl_detail_avoid_force.GetFloat();

	if ( flForce < 0.1 )
		return;

	if ( m_pAdvInfo == NULL )
		return;

	// get players in a radius
	float flRadius = cl_detail_avoid_radius.GetFloat();
	float flRecoverSpeed = cl_detail_avoid_recover_speed.GetFloat();

	Vector vecAvoid;
	C_BaseEntity *pEnt;

	float flMaxForce = 0;
	Vector vecMaxAvoid(0,0,0);

	CPlayerEnumerator avoid( flRadius, m_Origin );
	partition->EnumerateElementsInSphere( PARTITION_CLIENT_SOLID_EDICTS, m_Origin, flRadius, false, &avoid );

	// Okay, decide how to avoid if there's anything close by
	int c = avoid.GetObjectCount();
	for ( int i=0; i<c+1; i++ )	// +1 for the local player we tack on the end
	{
		if ( i == c )
		{
			pEnt = C_BasePlayer::GetLocalPlayer();
			if ( !pEnt ) continue;
		}
		else
			pEnt = avoid.GetObject( i );
		
		vecAvoid = m_Origin - pEnt->GetAbsOrigin();
		vecAvoid.z = 0;

		float flDist = vecAvoid.Length2D();

		if ( flDist > flRadius )
			continue;

		float flForceScale = RemapValClamped( flDist, 0, flRadius, flForce, 0.0 );

		if ( flForceScale > flMaxForce )
		{
			flMaxForce = flForceScale;
			vecAvoid.NormalizeInPlace();
			vecAvoid *= flMaxForce;
			vecMaxAvoid = vecAvoid;
		}
	}

	// if we are being moved, move fast. Else we recover at a slow rate
	if ( vecMaxAvoid.Length2D() > m_pAdvInfo->m_vecCurrentAvoid.Length2D() )
		flRecoverSpeed = 10;	// fast approach

	m_pAdvInfo->m_vecCurrentAvoid[0] = Approach( vecMaxAvoid[0], m_pAdvInfo->m_vecCurrentAvoid[0], flRecoverSpeed );
	m_pAdvInfo->m_vecCurrentAvoid[1] = Approach( vecMaxAvoid[1], m_pAdvInfo->m_vecCurrentAvoid[1], flRecoverSpeed );
	m_pAdvInfo->m_vecCurrentAvoid[2] = Approach( vecMaxAvoid[2], m_pAdvInfo->m_vecCurrentAvoid[2], flRecoverSpeed );
}
#endif

//-----------------------------------------------------------------------------
// draws a quad that sways on the top two vertices
// pass vecOrigin as the top left vertex position
//-----------------------------------------------------------------------------
#ifdef USE_DETAIL_SHAPES
void CDetailModel::DrawSwayingQuad( CMeshBuilder &meshBuilder, Vector vecOrigin, Vector vecSway, Vector2D texul, Vector2D texlr, unsigned char *color,
								   Vector width, Vector height )
{
	meshBuilder.Position3fv( (vecOrigin + vecSway).Base() );
	meshBuilder.TexCoord2fv( 0, texul.Base() );
	meshBuilder.Color4ubv( color );
	meshBuilder.AdvanceVertex();

	vecOrigin += height;
	meshBuilder.Position3fv( vecOrigin.Base() );
	meshBuilder.TexCoord2f( 0, texul.x, texlr.y );
	meshBuilder.Color4ubv( color );
	meshBuilder.AdvanceVertex();

	vecOrigin += width;
	meshBuilder.Position3fv( vecOrigin.Base() );
	meshBuilder.TexCoord2fv( 0, texlr.Base() );
	meshBuilder.Color4ubv( color );
	meshBuilder.AdvanceVertex();

	vecOrigin -= height;
	meshBuilder.Position3fv( (vecOrigin + vecSway).Base() );
	meshBuilder.TexCoord2f( 0, texlr.x, texul.y );
	meshBuilder.Color4ubv( color );
	meshBuilder.AdvanceVertex();
}
#endif

//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------
CDetailObjectSystem::CDetailObjectSystem() : m_DetailSpriteDict( 0, 32 ), m_DetailObjectDict( 0, 32 )
{
	BuildExponentTable();
}

CDetailObjectSystem::~CDetailObjectSystem()
{
}

	   
//-----------------------------------------------------------------------------
// Level init, shutdown
//-----------------------------------------------------------------------------
void CDetailObjectSystem::LevelInitPreEntity()
{
	// Prepare the translucent detail sprite material; we only have 1!
	m_DetailSpriteMaterial.Init( "detail/detailsprites", TEXTURE_GROUP_OTHER );
	m_DetailWireframeMaterial.Init( "debug/debugspritewireframe", TEXTURE_GROUP_OTHER );

	// Version check
	if (engine->GameLumpVersion( GAMELUMP_DETAIL_PROPS ) < 4)
	{
		Warning("Map uses old detail prop file format.. ignoring detail props\n");
		return;
	}

	// Unserialize
	int size = engine->GameLumpSize( GAMELUMP_DETAIL_PROPS );
	CUtlMemory<unsigned char> fileMemory;
	fileMemory.EnsureCapacity( size );
	if (engine->LoadGameLump( GAMELUMP_DETAIL_PROPS, fileMemory.Base(), size ))
	{
		CUtlBuffer buf( fileMemory.Base(), size, CUtlBuffer::READ_ONLY );
		UnserializeModelDict( buf );

		switch (engine->GameLumpVersion( GAMELUMP_DETAIL_PROPS ) )
		{
		case 4:
			UnserializeDetailSprites( buf );
			UnserializeModels( buf );
			break;
		}
	}

	if ( m_DetailObjects.Count() != 0 )
	{
		// There are detail objects in the level, so precache the material
		PrecacheMaterial( DETAIL_SPRITE_MATERIAL );
	}

	int detailPropLightingLump;
	if( g_pMaterialSystemHardwareConfig->GetHDRType() != HDR_TYPE_NONE )
	{
		detailPropLightingLump = GAMELUMP_DETAIL_PROP_LIGHTING_HDR;
	}
	else
	{
		detailPropLightingLump = GAMELUMP_DETAIL_PROP_LIGHTING;
	}
	size = engine->GameLumpSize( detailPropLightingLump );

	fileMemory.EnsureCapacity( size );
	if (engine->LoadGameLump( detailPropLightingLump, fileMemory.Base(), size ))
	{
		CUtlBuffer buf( fileMemory.Base(), size, CUtlBuffer::READ_ONLY );
		UnserializeModelLighting( buf );
	}
}


void CDetailObjectSystem::LevelInitPostEntity()
{
	const char *pDetailSpriteMaterial = DETAIL_SPRITE_MATERIAL;
	C_World *pWorld = GetClientWorldEntity();
	if ( pWorld && pWorld->GetDetailSpriteMaterial() && *(pWorld->GetDetailSpriteMaterial()) )
	{
		pDetailSpriteMaterial = pWorld->GetDetailSpriteMaterial(); 
	}
	m_DetailSpriteMaterial.Init( pDetailSpriteMaterial, TEXTURE_GROUP_OTHER );

	if ( GetDetailController() )
	{
		cl_detailfade.SetValue( min( m_flDefaultFadeStart, GetDetailController()->m_flFadeStartDist ) );
		cl_detaildist.SetValue( min( m_flDefaultFadeEnd, GetDetailController()->m_flFadeEndDist ) );
	}
	else
	{
		// revert to default values if the map doesn't specify
		cl_detailfade.SetValue( m_flDefaultFadeStart );
		cl_detaildist.SetValue( m_flDefaultFadeEnd );
	}
}

void CDetailObjectSystem::LevelShutdownPreEntity()
{
	m_DetailObjects.Purge();
	m_DetailObjectDict.Purge();
	m_DetailSpriteDict.Purge();
	m_DetailLighting.Purge();
	m_DetailSpriteMaterial.Shutdown();
}

void CDetailObjectSystem::LevelShutdownPostEntity()
{
	m_DetailWireframeMaterial.Shutdown();
}

//-----------------------------------------------------------------------------
// Before each view, blat out the stored detail sprite state
//-----------------------------------------------------------------------------
void CDetailObjectSystem::BeginTranslucentDetailRendering( )
{
	m_nSortedLeaf = -1;
	m_nSpriteCount = m_nFirstSprite = 0;
}


//-----------------------------------------------------------------------------
// Gets a particular detail object
//-----------------------------------------------------------------------------
IClientRenderable* CDetailObjectSystem::GetDetailModel( int idx )
{
	if ( IsPC() )
	{
		// FIXME: This is necessary because we have intermixed models + sprites
		// in a single list (m_DetailObjects)
		if (m_DetailObjects[idx].GetType() != DETAIL_PROP_TYPE_MODEL)
			return NULL;

		return &m_DetailObjects[idx];
	}
	else
	{
		return NULL;
	}
}


//-----------------------------------------------------------------------------
// Unserialization
//-----------------------------------------------------------------------------
void CDetailObjectSystem::UnserializeModelDict( CUtlBuffer& buf )
{
	int count = buf.GetInt();
	m_DetailObjectDict.EnsureCapacity( count );
	while ( --count >= 0 )
	{
		DetailObjectDictLump_t lump;
		buf.Get( &lump, sizeof(DetailObjectDictLump_t) );
		
		DetailModelDict_t dict;
		dict.m_pModel = (model_t *)engine->LoadModel( lump.m_Name, true );

		// Don't allow vertex-lit models
		if (modelinfo->IsModelVertexLit(dict.m_pModel))
		{
			Warning("Detail prop model %s is using vertex-lit materials!\nIt must use unlit materials!\n", lump.m_Name );
			dict.m_pModel = (model_t *)engine->LoadModel( "models/error.mdl" );
		}

		m_DetailObjectDict.AddToTail( dict );
	}
}

void CDetailObjectSystem::UnserializeDetailSprites( CUtlBuffer& buf )
{
	int count = buf.GetInt();
	m_DetailSpriteDict.EnsureCapacity( count );
	while ( --count >= 0 )
	{
		int i = m_DetailSpriteDict.AddToTail();
		buf.Get( &m_DetailSpriteDict[i], sizeof(DetailSpriteDictLump_t) );
	}
}


void CDetailObjectSystem::UnserializeModelLighting( CUtlBuffer& buf )
{
	int count = buf.GetInt();
	m_DetailLighting.EnsureCapacity( count );
	while ( --count >= 0 )
	{
		int i = m_DetailLighting.AddToTail();
		buf.Get( &m_DetailLighting[i], sizeof(DetailPropLightstylesLump_t) );
	}
}


//-----------------------------------------------------------------------------
// Unserialize all models
//-----------------------------------------------------------------------------
void CDetailObjectSystem::UnserializeModels( CUtlBuffer& buf )
{
	int firstDetailObject = 0;
	int detailObjectCount = 0;
	int detailObjectLeaf = -1;

	int count = buf.GetInt();
	m_DetailObjects.EnsureCapacity( count );

	while ( --count >= 0 )
	{
		DetailObjectLump_t lump;
		buf.Get( &lump, sizeof(DetailObjectLump_t) );
		
		// We rely on the fact that details objects are sorted by leaf in the
		// bsp file for this
		if ( detailObjectLeaf != lump.m_Leaf )
		{
			if (detailObjectLeaf != -1)
			{
				ClientLeafSystem()->SetDetailObjectsInLeaf( detailObjectLeaf, 
					firstDetailObject, detailObjectCount );
			}

			detailObjectLeaf = lump.m_Leaf;
			firstDetailObject = m_DetailObjects.Count();
			detailObjectCount = 0;
		}

		if ( lump.m_Type == DETAIL_PROP_TYPE_MODEL )
		{
			if ( IsPC() )
			{
				int newObj = m_DetailObjects.AddToTail();
				m_DetailObjects[newObj].Init( newObj, lump.m_Origin, lump.m_Angles, 
					m_DetailObjectDict[lump.m_DetailModel].m_pModel, lump.m_Lighting,
					lump.m_LightStyles, lump.m_LightStyleCount, lump.m_Orientation );
				++detailObjectCount;
			}
		}
		else
		{
			int newObj = m_DetailObjects.AddToTail();
			m_DetailObjects[newObj].InitSprite( newObj, lump.m_Origin, lump.m_Angles, 
				lump.m_DetailModel, lump.m_Lighting,
				lump.m_LightStyles, lump.m_LightStyleCount, lump.m_Orientation, lump.m_flScale,
				lump.m_Type, lump.m_ShapeAngle, lump.m_ShapeSize, lump.m_SwayAmount );
			++detailObjectCount;
		}
	}

	if (detailObjectLeaf != -1)
	{
		ClientLeafSystem()->SetDetailObjectsInLeaf( detailObjectLeaf, 
			firstDetailObject, detailObjectCount );
	}
}



//-----------------------------------------------------------------------------
// Renders all opaque detail objects in a particular set of leaves
//-----------------------------------------------------------------------------
void CDetailObjectSystem::RenderOpaqueDetailObjects( int nLeafCount, LeafIndex_t *pLeafList )
{
	// FIXME: Implement!
}


//-----------------------------------------------------------------------------
// Count the number of detail sprites in the leaf list
//-----------------------------------------------------------------------------
int CDetailObjectSystem::CountSpritesInLeafList( int nLeafCount, LeafIndex_t *pLeafList )
{
	VPROF_BUDGET( "CDetailObjectSystem::CountSpritesInLeafList", VPROF_BUDGETGROUP_DETAILPROP_RENDERING );
	int nPropCount = 0;
	int nFirstDetailObject, nDetailObjectCount;
	for ( int i = 0; i < nLeafCount; ++i )
	{
		// FIXME: This actually counts *everything* in the leaf, which is ok for now
		// given how we're using it
		ClientLeafSystem()->GetDetailObjectsInLeaf( pLeafList[i], nFirstDetailObject, nDetailObjectCount );
		nPropCount += nDetailObjectCount;
	}

	return nPropCount;
}


//-----------------------------------------------------------------------------
// Count the number of detail sprite quads in the leaf list
//-----------------------------------------------------------------------------
int CDetailObjectSystem::CountSpriteQuadsInLeafList( int nLeafCount, LeafIndex_t *pLeafList )
{
#ifdef USE_DETAIL_SHAPES
	VPROF_BUDGET( "CDetailObjectSystem::CountSpritesInLeafList", VPROF_BUDGETGROUP_DETAILPROP_RENDERING );
	int nQuadCount = 0;
	int nFirstDetailObject, nDetailObjectCount;
	for ( int i = 0; i < nLeafCount; ++i )
	{
		// FIXME: This actually counts *everything* in the leaf, which is ok for now
		// given how we're using it
		ClientLeafSystem()->GetDetailObjectsInLeaf( pLeafList[i], nFirstDetailObject, nDetailObjectCount );
		for ( int j = 0; j < nDetailObjectCount; ++j )
		{
			nQuadCount += m_DetailObjects[j + nFirstDetailObject].QuadsToDraw();
		}
	}

	return nQuadCount;
#else
	return CountSpritesInLeafList( nLeafCount, pLeafList );
#endif
}


//-----------------------------------------------------------------------------
// Sorts sprites in back-to-front order
//-----------------------------------------------------------------------------
int __cdecl CDetailObjectSystem::SortFunc( const void *arg1, const void *arg2 )
{
	// Therefore, things that are farther away in front of us (has a greater + distance)
	// need to appear at the front of the list, hence the somewhat misleading code below
	float flDelta = ((SortInfo_t*)arg1)->m_flDistance - ((SortInfo_t*)arg2)->m_flDistance;
	if ( flDelta > 0 )
		return -1;
	if ( flDelta < 0 )
		return 1;
	return 0;
}

int CDetailObjectSystem::SortSpritesBackToFront( int nLeaf, const Vector &viewOrigin, const Vector &viewForward, SortInfo_t *pSortInfo )
{
	VPROF_BUDGET( "CDetailObjectSystem::SortSpritesBackToFront", VPROF_BUDGETGROUP_DETAILPROP_RENDERING );
	int nFirstDetailObject, nDetailObjectCount;
	ClientLeafSystem()->GetDetailObjectsInLeaf( nLeaf, nFirstDetailObject, nDetailObjectCount );

	float flFactor = 1.0f;
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pLocalPlayer )
	{
		flFactor = pLocalPlayer->GetFOVDistanceAdjustFactor();
	}
	float flMaxSqDist = cl_detaildist.GetFloat() * cl_detaildist.GetFloat();
 	float flFadeSqDist = cl_detaildist.GetFloat() - cl_detailfade.GetFloat();
	flMaxSqDist /= flFactor;
	flFadeSqDist /= flFactor;
	if (flFadeSqDist > 0)
	{
		flFadeSqDist *= flFadeSqDist;
	}
	else 
	{
		flFadeSqDist = 0;
	}
	float flFalloffFactor = 255.0f / (flMaxSqDist - flFadeSqDist);

	Vector vecDelta;
	int nCount = 0;
	for ( int j = 0; j < nDetailObjectCount; ++j )
	{
		CDetailModel &model = m_DetailObjects[nFirstDetailObject + j];

		Vector v;
		VectorSubtract( model.GetRenderOrigin(), viewOrigin, v );
		float flSqDist = v.LengthSqr();
		if ( flSqDist >= flMaxSqDist )
			continue;

		if ((flFadeSqDist > 0) && (flSqDist > flFadeSqDist))
		{
			model.SetAlpha( flFalloffFactor * ( flMaxSqDist - flSqDist ) );
		}
		else
		{
			model.SetAlpha( 255 );
		}

		// Perform screen alignment if necessary.
		model.ComputeAngles();

		if ( IsPC() && ( (model.GetType() == DETAIL_PROP_TYPE_MODEL) || (model.GetAlpha() == 0) ) )
			continue;

		pSortInfo[nCount].m_nIndex = nFirstDetailObject + j;

		// Compute distance from the camera to each object
		VectorSubtract( model.GetRenderOrigin(), viewOrigin, vecDelta );
		pSortInfo[nCount].m_flDistance = vecDelta.LengthSqr(); //DotProduct( viewForward, vecDelta );
		++nCount;
	}

	qsort( pSortInfo, nCount, sizeof(SortInfo_t), SortFunc ); 
	return nCount;
}


//-----------------------------------------------------------------------------
// Renders all translucent detail objects in a particular set of leaves
//-----------------------------------------------------------------------------
void CDetailObjectSystem::RenderTranslucentDetailObjects( const Vector &viewOrigin, const Vector &viewForward, int nLeafCount, LeafIndex_t *pLeafList )
{
	VPROF_BUDGET( "CDetailObjectSystem::RenderTranslucentDetailObjects", VPROF_BUDGETGROUP_DETAILPROP_RENDERING );
	if (nLeafCount == 0)
		return;

	// We better not have any partially drawn leaf of detail sprites!
	Assert( m_nSpriteCount == m_nFirstSprite );

	// Here, we must draw all detail objects back-to-front
	// FIXME: Cache off a sorted list so we don't have to re-sort every frame

	// Count the total # of detail quads we possibly could render
	int nQuadCount = CountSpriteQuadsInLeafList( nLeafCount, pLeafList );
	if ( nQuadCount == 0 )
		return;

	materials->MatrixMode( MATERIAL_MODEL );
	materials->PushMatrix();
	materials->LoadIdentity();

	IMaterial *pMaterial = m_DetailSpriteMaterial;
	if ( ShouldDrawInWireFrameMode() || r_DrawDetailProps.GetInt() == 2 )
	{
		pMaterial = m_DetailWireframeMaterial;
	}

	CMeshBuilder meshBuilder;
	IMesh *pMesh = materials->GetDynamicMesh( true, NULL, NULL, pMaterial );

	int nMaxVerts, nMaxIndices;
	materials->GetMaxToRender( pMesh, false, &nMaxVerts, &nMaxIndices );
	int nMaxQuadsToDraw = nMaxIndices / 6;
	if ( nMaxQuadsToDraw > nMaxVerts / 4 ) 
	{
		nMaxQuadsToDraw = nMaxVerts / 4;
	}

	int nQuadsToDraw = nQuadCount;
	if ( nQuadsToDraw > nMaxQuadsToDraw )
	{
		nQuadsToDraw = nMaxQuadsToDraw;
	}

	meshBuilder.Begin( pMesh, MATERIAL_QUADS, nQuadsToDraw );

	int nQuadsDrawn = 0;
	for ( int i = 0; i < nLeafCount; ++i )
	{
		int nLeaf = pLeafList[i];

		int nFirstDetailObject, nDetailObjectCount;
		ClientLeafSystem()->GetDetailObjectsInLeaf( nLeaf, nFirstDetailObject, nDetailObjectCount );

		// Sort detail sprites in each leaf independently; then render them
		SortInfo_t *pSortInfo = (SortInfo_t *)stackalloc( nDetailObjectCount * sizeof(SortInfo_t) );
		int nCount = SortSpritesBackToFront( nLeaf, viewOrigin, viewForward, pSortInfo );

		for ( int j = 0; j < nCount; ++j )
		{
			CDetailModel &model = m_DetailObjects[ pSortInfo[j].m_nIndex ];
			int nQuadsInModel = model.QuadsToDraw();

			// Prevent the batches from getting too large
			if ( nQuadsDrawn + nQuadsInModel > nQuadsToDraw )
			{
				meshBuilder.End();
				pMesh->Draw();

				nQuadCount -= nQuadsDrawn;
				nQuadsToDraw = nQuadCount;
				if (nQuadsToDraw > nMaxQuadsToDraw)
				{
					nQuadsToDraw = nMaxQuadsToDraw;
				}

				meshBuilder.Begin( pMesh, MATERIAL_QUADS, nQuadsToDraw );
				nQuadsDrawn = 0;
			}

			model.DrawSprite( meshBuilder );

			nQuadsDrawn += nQuadsInModel;
		}
	}

	meshBuilder.End();
	pMesh->Draw();

	materials->PopMatrix();
}


//-----------------------------------------------------------------------------
// Renders a subset of the detail objects in a particular leaf (for interleaving with other translucent entities)
//-----------------------------------------------------------------------------
void CDetailObjectSystem::RenderTranslucentDetailObjectsInLeaf( const Vector &viewOrigin, const Vector &viewForward, int nLeaf, const Vector *pVecClosestPoint )
{
	VPROF_BUDGET( "CDetailObjectSystem::RenderTranslucentDetailObjectsInLeaf", VPROF_BUDGETGROUP_DETAILPROP_RENDERING );

	// We may have already sorted this leaf. If not, sort the leaf.
	if ( m_nSortedLeaf != nLeaf )
	{
		m_nSortedLeaf = nLeaf;
		m_nSpriteCount = 0;
		m_nFirstSprite = 0;

		// Count the total # of detail sprites we possibly could render
		LeafIndex_t nLeafIndex = nLeaf;
		int nSpriteCount = CountSpritesInLeafList( 1, &nLeafIndex );
		Assert( nSpriteCount <= MAX_SPRITES_PER_LEAF ); 
		if (nSpriteCount == 0)
			return;

		// Sort detail sprites in each leaf independently; then render them
		m_nSpriteCount = SortSpritesBackToFront( nLeaf, viewOrigin, viewForward, m_pSortInfo );
		Assert( m_nSpriteCount <= nSpriteCount );
	}

	// No more to draw? Bye!
	if ( m_nSpriteCount == m_nFirstSprite )
		return;

	float flMinDistance = 0.0f;
	if ( pVecClosestPoint )
	{
		Vector vecDelta;
		VectorSubtract( *pVecClosestPoint, viewOrigin, vecDelta );
		flMinDistance = vecDelta.LengthSqr();
	}

	if ( m_pSortInfo[m_nFirstSprite].m_flDistance < flMinDistance )
		return;

	materials->MatrixMode( MATERIAL_MODEL );
	materials->PushMatrix();
	materials->LoadIdentity();

	IMaterial *pMaterial = m_DetailSpriteMaterial;
	if ( ShouldDrawInWireFrameMode() || r_DrawDetailProps.GetInt() == 2 )
	{
		pMaterial = m_DetailWireframeMaterial;
	}

	CMeshBuilder meshBuilder;
	IMesh *pMesh = materials->GetDynamicMesh( true, NULL, NULL, pMaterial );

	int nMaxVerts, nMaxIndices;
	materials->GetMaxToRender( pMesh, false, &nMaxVerts, &nMaxIndices );

	// needs to be * 4 since there are a max of 4 quads per detail object
	int nQuadCount = ( m_nSpriteCount - m_nFirstSprite ) * 4;
	int nMaxQuadsToDraw = nMaxIndices/6;
	if ( nMaxQuadsToDraw > nMaxVerts / 4 )
	{
		nMaxQuadsToDraw = nMaxVerts / 4;
	}
	int nQuadsToDraw = nQuadCount;
	if ( nQuadsToDraw > nMaxQuadsToDraw )
	{
		nQuadsToDraw = nMaxQuadsToDraw;
	}

	meshBuilder.Begin( pMesh, MATERIAL_QUADS, nQuadsToDraw );

	int nQuadsDrawn = 0;
	while ( m_nFirstSprite < m_nSpriteCount && m_pSortInfo[m_nFirstSprite].m_flDistance >= flMinDistance )
	{
		CDetailModel &model = m_DetailObjects[m_pSortInfo[m_nFirstSprite].m_nIndex];
		int nQuadsInModel = model.QuadsToDraw();
		if ( nQuadsDrawn + nQuadsInModel > nQuadsToDraw )
		{
			meshBuilder.End();
			pMesh->Draw();

			nQuadCount = ( m_nSpriteCount - m_nFirstSprite ) * 4;
			nQuadsToDraw = nQuadCount;
			if (nQuadsToDraw > nMaxQuadsToDraw)
			{
				nQuadsToDraw = nMaxQuadsToDraw;
			}

			meshBuilder.Begin( pMesh, MATERIAL_QUADS, nQuadsToDraw );
			nQuadsDrawn = 0;
		}

		model.DrawSprite( meshBuilder );
		++m_nFirstSprite;
		nQuadsDrawn += nQuadsInModel;
	}
	meshBuilder.End();
	pMesh->Draw();

	materials->PopMatrix();
}


//-----------------------------------------------------------------------------
// Gets called each view
//-----------------------------------------------------------------------------
bool CDetailObjectSystem::EnumerateLeaf( int leaf, int context )
{
	VPROF_BUDGET( "CDetailObjectSystem::EnumerateLeaf", VPROF_BUDGETGROUP_DETAILPROP_RENDERING );
	Vector v;
	int firstDetailObject, detailObjectCount;

	EnumContext_t* pCtx = (EnumContext_t*)context;
	ClientLeafSystem()->DrawDetailObjectsInLeaf( leaf, pCtx->m_BuildWorldListNumber, 
		firstDetailObject, detailObjectCount );

	if ( IsPC() )
	{
		// Compute the translucency. Need to do it now cause we need to
		// know that when we're rendering (opaque stuff is rendered first)
		for ( int i = 0; i < detailObjectCount; ++i)
		{
			// Calculate distance (badly)
			CDetailModel& model = m_DetailObjects[firstDetailObject+i];
			VectorSubtract( model.GetRenderOrigin(), CurrentViewOrigin(), v );

			float sqDist = v.LengthSqr();

			if ( sqDist < pCtx->m_MaxSqDist )
			{
				if ((pCtx->m_FadeSqDist > 0) && (sqDist > pCtx->m_FadeSqDist))
				{
					model.SetAlpha( pCtx->m_FalloffFactor * (pCtx->m_MaxSqDist - sqDist ) );
				}
				else
				{
					model.SetAlpha( 255 );
				}

				// Perform screen alignment if necessary.
				model.ComputeAngles();
			}
			else
			{
				model.SetAlpha( 0 );
			}
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
// Gets called each view
//-----------------------------------------------------------------------------
void CDetailObjectSystem::BuildDetailObjectRenderLists( )
{
	VPROF_BUDGET( "CDetailObjectSystem::BuildDetailObjectRenderLists", VPROF_BUDGETGROUP_DETAILPROP_RENDERING );
	
	if (!g_pClientMode->ShouldDrawDetailObjects() || (r_DrawDetailProps.GetInt() == 0))
		return;

	// Don't bother doing any of this if the level doesn't have detail props.
	if ( m_DetailObjects.Count() == 0 )
		return;

	EnumContext_t ctx;
 	ctx.m_BuildWorldListNumber = view->BuildWorldListsNumber();

	if ( IsPC() )
	{
		// We need to recompute translucency information for all detail props
		for (int i = m_DetailObjectDict.Size(); --i >= 0; )
		{
			if (modelinfo->ModelHasMaterialProxy( m_DetailObjectDict[i].m_pModel ))
			{
				modelinfo->RecomputeTranslucency( m_DetailObjectDict[i].m_pModel );
			}
		}

		float factor = 1.0f;
		C_BasePlayer *local = C_BasePlayer::GetLocalPlayer();
		if ( local )
		{
			factor = local->GetFOVDistanceAdjustFactor();
		}

		// Compute factors to optimize rendering of the detail models
		ctx.m_MaxSqDist = cl_detaildist.GetFloat() * cl_detaildist.GetFloat();
		ctx.m_FadeSqDist = cl_detaildist.GetFloat() - cl_detailfade.GetFloat();

		ctx.m_MaxSqDist /= factor;
		ctx.m_FadeSqDist /= factor;

		if (ctx.m_FadeSqDist > 0)
		{
			ctx.m_FadeSqDist *= ctx.m_FadeSqDist;
		}
		else 
		{
			ctx.m_FadeSqDist = 0;
		}
		ctx.m_FalloffFactor = 255.0f / (ctx.m_MaxSqDist - ctx.m_FadeSqDist);
	}

	ISpatialQuery* pQuery = engine->GetBSPTreeQuery();
	pQuery->EnumerateLeavesInSphere( CurrentViewOrigin(), 
		cl_detaildist.GetFloat(), this, (int)&ctx );
}

