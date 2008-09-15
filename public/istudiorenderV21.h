//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef ISTUDIORENDER_V21_H
#define ISTUDIORENDER_V21_H
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"
#include "vector.h"
#include "vector4d.h"
#include "utlbuffer.h"
#include "utlvector.h"
#include "materialsystem/imaterialsystem.h"

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
struct studiohdr_t;
struct studiomeshdata_t;
class Vector;
struct LightDesc_t;
class IMaterial;
struct studiohwdata_t;
struct Ray_t;
class Vector4D;
class IMaterialSystem;
struct matrix3x4_t;
class IMesh;
struct vertexFileHeader_t;
struct FlashlightState_t;
class VMatrix;
namespace OptimizedModel { struct FileHeader_t; }

namespace StudioRenderV21
{

// undone: what's the standard for function type naming?
typedef void (*StudioRender_Printf_t)( const char *fmt, ... );

struct StudioRenderConfig_t
{
	StudioRender_Printf_t pConPrintf;
	StudioRender_Printf_t pConDPrintf;
	float fEyeShiftX;	// eye X position
	float fEyeShiftY;	// eye Y position
	float fEyeShiftZ;	// eye Z position
	float fEyeSize;		// adjustment to iris textures
	int eyeGloss;		// wet eyes
	int drawEntities;
	int skin;
	int fullbright;
	bool bEyeMove;		// look around
	bool bSoftwareSkin;
	bool bNoHardware;
	bool bNoSoftware;
	bool bTeeth;
	bool bEyes;
	bool bFlex;
	bool bWireframe;
	bool bNormals;
	bool bSoftwareLighting;
	bool bShowEnvCubemapOnly;
	int maxDecalsPerModel;
	bool bWireframeDecals;
	float fEyeGlintPixelWidthLODThreshold;
	int rootLOD;
};


//-----------------------------------------------------------------------------
// Studio render interface
//-----------------------------------------------------------------------------

#define STUDIO_RENDER_INTERFACE_VERSION_21 "VStudioRender021"

typedef unsigned short StudioDecalHandle_t;

enum
{
	STUDIORENDER_DECAL_INVALID = (StudioDecalHandle_t)~0
};

enum
{
	ADDDECAL_TO_ALL_LODS = -1
};

//-----------------------------------------------------------------------------
// DrawModel flags
//-----------------------------------------------------------------------------
enum
{
	STUDIORENDER_DRAW_ENTIRE_MODEL		= 0,
	STUDIORENDER_DRAW_OPAQUE_ONLY		= 0x01,
	STUDIORENDER_DRAW_TRANSLUCENT_ONLY	= 0x02,
	STUDIORENDER_DRAW_GROUP_MASK		= 0x03,

	STUDIORENDER_DRAW_NO_FLEXES			= 0x04,
	STUDIORENDER_DRAW_STATIC_LIGHTING	= 0x08,


	STUDIORENDER_DRAW_ACCURATETIME		= 0x10,		// Use accurate timing when drawing the model.
	STUDIORENDER_DRAW_NO_SHADOWS		= 0x20,
	STUDIORENDER_DRAW_GET_PERF_STATS	= 0x40,

	STUDIORENDER_DRAW_WIREFRAME			= 0x80,
};


//-----------------------------------------------------------------------------
// What kind of material override is it?
//-----------------------------------------------------------------------------
enum OverrideType_t
{
	OVERRIDE_NORMAL = 0,
	OVERRIDE_BUILD_SHADOWS,
};


//-----------------------------------------------------------------------------
// DrawModel info
//-----------------------------------------------------------------------------

// Special flag for studio models that have a compiled in shadow lod version
// It's negative 2 since positive numbers == use a regular slot and -1 means
//  have studiorender compute a value instead
enum
{
	USESHADOWLOD = -2,
};

// beyond this number of materials, you won't get info back from DrawModel
#define MAX_DRAW_MODEL_INFO_MATERIALS 8

struct DrawModelInfo_t
{
	studiohdr_t *m_pStudioHdr;
	studiohwdata_t *m_pHardwareData;
	StudioDecalHandle_t m_Decals;
	int m_Skin;
	int m_Body;
	int m_HitboxSet;
	void *m_pClientEntity;
	int m_Lod;
	IMesh **m_ppColorMeshes;
	int m_ActualTriCount; 
	int m_TextureMemoryBytes;
	int m_NumHardwareBones;
	int m_NumBatches;
	int m_NumMaterials;
	CFastTimer m_RenderTime;
	CUtlVectorFixed<IMaterial *,MAX_DRAW_MODEL_INFO_MATERIALS> m_Materials;

	bool			m_bStaticLighting;
	Vector			m_vecAmbientCube[6];		// ambient, and lights that aren't in locallight[]
	int				m_nLocalLightCount;
	LightDesc_t		m_LocalLightDescs[4];

	DrawModelInfo_t() {}

private:
	// No copy constructors allowed (see LightDesc_t).
	DrawModelInfo_t( const DrawModelInfo_t &vOther );
};

struct GetTriangles_Vertex_t
{
	Vector m_Position;
	Vector m_Normal;
	Vector4D m_TangentS;
	Vector2D m_TexCoord;
	Vector4D m_BoneWeight;
	int m_BoneIndex[4];
	int m_NumBones;
};

struct GetTriangles_MaterialBatch_t
{
	IMaterial *m_pMaterial;
	CUtlVector<GetTriangles_Vertex_t> m_Verts;
	CUtlVector<int> m_TriListIndices;
};

struct GetTriangles_Output_t
{
	CUtlVector<GetTriangles_MaterialBatch_t> m_MaterialBatches;
};

//-----------------------------------------------------------------------------
// Cache Callback Function
// implementation can either statically persist data (tools) or lru cache (engine) it.
// caller returns base pointer to resident data.
// code expectes data to be dynamic and invokes cache callback prior to iterative access.
// virtualModel is member passed in via studiohdr_t and passed back for model identification.
//-----------------------------------------------------------------------------
#define STUDIO_DATA_CACHE_INTERFACE_VERSION_3 "VStudioDataCache003"
 
abstract_class IStudioDataCache
{
public:
	virtual bool VerifyHeaders( studiohdr_t *pStudioHdr ) = 0;
	virtual vertexFileHeader_t *CacheVertexData( studiohdr_t *pStudioHdr ) = 0;
	virtual OptimizedModel::FileHeader_t *CacheIndexData( studiohdr_t *pStudioHdr ) = 0;
};

//-----------------------------------------------------------------------------
// Studio render interface
//-----------------------------------------------------------------------------
abstract_class IStudioRender
{
public:
	// Initializes, shutdowns the studio render library
	virtual bool Init( CreateInterfaceFn materialSystemFactory, CreateInterfaceFn materialSystemHWConfigFactory,
		CreateInterfaceFn convarFactory, CreateInterfaceFn studioDataCacheFactory ) = 0;
	virtual void Shutdown( void ) = 0;

	virtual void BeginFrame( void ) = 0;
	virtual void EndFrame( void ) = 0;

	// Updates the rendering configuration 
	virtual void UpdateConfig( const StudioRenderConfig_t& config ) = 0;

	virtual bool LoadModel( studiohdr_t *pStudioHdr, studiohwdata_t	*pHardwareData ) = 0;
	
	// since studiomeshes are allocated inside of the lib, they need to be freed there as well.
	virtual void UnloadModel( studiohwdata_t *pHardwareData ) = 0;

	// This is needed to do eyeglint and calculate the correct texcoords for the eyes.
	virtual void SetEyeViewTarget( const Vector& worldPosition ) = 0;
		
	virtual int GetNumAmbientLightSamples() = 0;
	
	virtual const Vector *GetAmbientLightDirections() = 0;

	// assumes that the arraysize is the same as returned from GetNumAmbientLightSamples
	virtual void SetAmbientLightColors( const Vector *pAmbientOnlyColors ) = 0;
	
	virtual void SetLocalLights( int numLights, const LightDesc_t *pLights ) = 0;

	virtual void SetViewState( 	
		const Vector& viewOrigin,
		const Vector& viewRight,
		const Vector& viewUp,
		const Vector& viewPlaneNormal ) = 0;
	
	virtual void SetFlexWeights( int numWeights, const float *pWeights ) = 0;
	virtual void SetFlexWeights( int numWeights, const float *pWeights, const float *pDelayedWeights ) = 0;
	
	// fixme: these interfaces sucks. . use 'em to get this stuff working with the client dll
	// and then interate
	virtual matrix3x4_t* GetPoseToWorld(int i) = 0; // this will be hidden enntually (computed internally)
	virtual matrix3x4_t* GetBoneToWorld(int i) = 0;

	// NOTE: this array must have space for MAXSTUDIOBONES.
	virtual matrix3x4_t* GetBoneToWorldArray() = 0;
	
	// LOD stuff
	virtual int GetNumLODs( const studiohwdata_t &hardwareData ) const = 0;
	virtual float GetLODSwitchValue( const studiohwdata_t &hardwareData, int lod ) const = 0;
	virtual void SetLODSwitchValue( studiohwdata_t &hardwareData, int lod, float switchValue ) = 0;

	// Sets the color modulation
	virtual void SetColorModulation( float const* pColor ) = 0;
	virtual void SetAlphaModulation( float alpha ) = 0;
	
	// returns the number of triangles rendered.
	virtual int DrawModel( DrawModelInfo_t& info, const Vector &modelOrigin,
		int *pLodUsed, float *pMetric, int flags = STUDIORENDER_DRAW_ENTIRE_MODEL ) = 0;

	// Causes a material to be used instead of the materials the model was compiled with
	virtual void ForcedMaterialOverride( IMaterial *newMaterial, OverrideType_t nOverrideType = OVERRIDE_NORMAL ) = 0;

	// Create, destroy list of decals for a particular model
	virtual StudioDecalHandle_t CreateDecalList( studiohwdata_t *pHardwareData ) = 0;
	virtual void DestroyDecalList( StudioDecalHandle_t handle ) = 0;

	// Add decals to a decal list by doing a planar projection along the ray
	// The BoneToWorld matrices must be set before this is called
	virtual void AddDecal( StudioDecalHandle_t handle, studiohdr_t *pStudioHdr, const Ray_t & ray, 
		const Vector& decalUp, IMaterial* pDecalMaterial, float radius, int body, bool noPokethru = false, int maxLODToDecal = ADDDECAL_TO_ALL_LODS ) = 0;

	// Remove all the decals on a model
	virtual void RemoveAllDecals( StudioDecalHandle_t handle, studiohdr_t *pStudioHdr ) = 0;

	// Compute the lighting at a point and normal
	virtual void ComputeLighting( const Vector* pAmbient, int lightCount,
		LightDesc_t* pLights, const Vector& pt, const Vector& normal, Vector& lighting ) = 0;

	// Refresh the studiohdr since it was lost...
	virtual void RefreshStudioHdr( studiohdr_t* pStudioHdr, studiohwdata_t* pHardwareData ) = 0;

	// Used for the mat_stub console command.
	virtual void Mat_Stub( IMaterialSystem *pMatSys ) = 0;

	// Shadow state (affects the models as they are rendered)
	virtual void AddShadow( IMaterial* pMaterial, void* pProxyData, FlashlightState_t *m_pFlashlightState = NULL, VMatrix *pWorldToTexture = NULL ) = 0;
	virtual void ClearAllShadows() = 0;

	// Gets the model LOD; pass in the screen size in pixels of a sphere 
	// of radius 1 that has the same origin as the model to get the LOD out...
	virtual int ComputeModelLod( studiohwdata_t* pHardwareData, float unitSphereSize, float *pMetric = NULL ) = 0;

	// Return a number that is usable for budgets, etc.
	// Things that we care about:
	// 1) effective triangle count (factors in batch sizes, state changes, etc)
	// 2) texture memory usage
	virtual void GetPerfStats( DrawModelInfo_t &info, CUtlBuffer *pSpewBuf = NULL ) const = 0;

	virtual void GetTriangles( DrawModelInfo_t& info,
								 GetTriangles_Output_t &out ) = 0;
};

} // end namespace

// FIXME: Currently necessary for expose macro to work
class IStudioRenderV21 : public StudioRenderV21::IStudioRender
{
public:
};


#endif // ISTUDIORENDER_V21_H
