//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Empty implementation of shader API
//
//===========================================================================//

/* This is totally reverse-engineered code and may be wrong */

#include "interfaces/interfaces.h"
#include "shaderapi/shareddefs.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imesh.h"
#include "materialsystem/idebugtextureinfo.h"
#include "ishaderutil.h"
#include "ishadershadow.h"
#include "ishaderapi.h"
#include "IHardwareConfigInternal.h"
#include "tier0/dbg.h"

//-----------------------------------------------------------------------------
// Empty mesh
//-----------------------------------------------------------------------------
class CEmptyMesh : public IMesh
{
public:
	CEmptyMesh( bool bIsDynamic );
	virtual ~CEmptyMesh();

	bool Lock( int nMaxIndexCount, bool bAppend, IndexDesc_t &desc );
	bool Lock( int nVertexCount, bool bAppend, VertexDesc_t &desc );
	void Unlock( int nWrittenIndexCount, IndexDesc_t &desc );
	void Unlock( int nVertexCount, VertexDesc_t &desc );
	void ModifyBegin( bool bReadOnly, int nFirstIndex, int nIndexCount, IndexDesc_t &desc );
	void ModifyBegin( int firstVertex, int numVerts, int firstIndex, int numIndices, MeshDesc_t &desc );
	void ModifyEnd( IndexDesc_t &desc );
	void ModifyEnd( MeshDesc_t &desc );
	void Spew( int nIndexCount, const IndexDesc_t &desc );
	void Spew( int nVertexCount, const VertexDesc_t &desc );
	void Spew( int numVerts, int numIndices, const MeshDesc_t &desc );
	void ValidateData( int nIndexCount, const IndexDesc_t &desc );
	void ValidateData( int nVertexCount, const VertexDesc_t &desc );
	void ValidateData( int numVerts, int numIndices, const MeshDesc_t &desc );
	bool IsDynamic() const;
	void BeginCastBuffer( VertexFormat_t format );
	void BeginCastBuffer( MaterialIndexFormat_t format );
	void EndCastBuffer();
	int GetRoomRemaining() const;
	MaterialIndexFormat_t IndexFormat() const;
	void LockMesh( int numVerts, int numIndices, MeshDesc_t &desc, MeshBuffersAllocationSettings_t *pSettings );
	void UnlockMesh( int numVerts, int numIndices, MeshDesc_t &desc );
	void ModifyBeginEx( bool bReadOnly, int firstVertex, int numVerts, int firstIndex, int numIndices, MeshDesc_t &desc );
	int VertexCount() const;
	void SetPrimitiveType( MaterialPrimitiveType_t type );
	void Draw( int firstIndex, int numIndices );
	void Draw( CPrimList *pLists, int nLists );
	void DrawModulated( const Vector4D &vecDiffuseModulation, int firstIndex, int numIndices );
	void CopyToMeshBuilder(int iStartVert, int nVerts, int iStartIndex, int nIndices, int indexOffset, CMeshBuilder &builder);
	IMaterial *GetMaterial();
	void SetColorMesh( IMesh *pColorMesh, int nVertexOffset );
	int IndexCount() const;
	void SetFlexMesh( IMesh *pMesh, int nVertexOffset );
	void DisableFlexMesh();
	void MarkAsDrawn();
	VertexFormat_t GetVertexFormat() const;
	IMesh *GetMesh();
	unsigned int ComputeMemoryUsed();
	void *AccessRawHardwareDataStream( uint8 nRawStreamIndex, uint32 numBytes, uint32 uiFlags, void *pvContext );
	ICachedPerFrameMeshData *GetCachedPerFrameMeshData();
	void ReconstructFromCachedPerFrameMeshData( ICachedPerFrameMeshData *pData );
private:
	unsigned char *m_pVertexMemory;
	bool m_bIsDynamic;
};

//-----------------------------------------------------------------------------
// Empty shader device manager
//-----------------------------------------------------------------------------
class CShaderDeviceMgrEmpty : public CBaseAppSystem<IShaderDeviceMgr>
{
public:
	bool Connect( CreateInterfaceFn factory );
	void Disconnect();

	void *QueryInterface( const char *pInterfaceName );

	InitReturnVal_t Init();
	void Shutdown();

	int	GetAdapterCount() const;
	void GetAdapterInfo( int nAdapter, MaterialAdapterInfo_t &info ) const;
	bool GetRecommendedVideoConfig( int nAdapter, KeyValues *pConfiguration );
	bool GetRecommendedConfigurationInfo( int nAdapter, int nDXLevel, KeyValues *pConfiguration );
	int GetModeCount( int nAdapter ) const;
	void GetModeInfo( ShaderDisplayMode_t *pInfo, int nAdapter, int nMode ) const;
	void GetCurrentModeInfo( ShaderDisplayMode_t *pInfo, int nAdapter ) const;
	bool SetAdapter( int nAdapter, int nFlags );
	CreateInterfaceFn SetMode( void *hWnd, int nAdapter, const ShaderDeviceInfo_t &mode ); 
	void AddModeChangeCallback( ShaderModeChangeCallbackFunc_t func );
	void RemoveModeChangeCallback( ShaderModeChangeCallbackFunc_t func );
	void AddDeviceDependentObject( IShaderDeviceDependentObject *pObject );
	void RemoveDeviceDependentObject( IShaderDeviceDependentObject *pObject );
};

//-----------------------------------------------------------------------------
// Empty shader device
//-----------------------------------------------------------------------------
class CShaderDeviceEmpty : public IShaderDevice
{
public:
	CShaderDeviceEmpty();

	int GetCurrentAdapter() const;
	bool IsUsingGraphics() const;
	void SpewDriverInfo() const;
	ImageFormat GetBackBufferFormat() const;
	void GetBackBufferDimensions( int &width, int &height ) const;
	const AspectRatioInfo_t &GetAspectRatioInfo() const;
	int StencilBufferBits() const;
	bool IsAAEnabled() const;
	void Present();
	void GetWindowSize( int &nWidth, int &nHeight ) const;
	bool AddView( void *hWnd );
	void RemoveView( void *hWnd );
	void SetView( void *hWnd );
	void ReleaseResources( bool bReleaseManagedResources );
	void ReacquireResources();
	IMesh *CreateStaticMesh( VertexFormat_t vertexFormat, const char *pTextureBudgetGroup, IMaterial *pMaterial, VertexStreamSpec_t *pStreamSpec );
	void DestroyStaticMesh( IMesh *mesh );
	IShaderBuffer *CompileShader( const char *pProgram, size_t nBufLen, const char *pShaderVersion );
	VertexShaderHandle_t CreateVertexShader( IShaderBuffer *pShaderBuffer );
	void DestroyVertexShader( VertexShaderHandle_t hShader );
	GeometryShaderHandle_t CreateGeometryShader( IShaderBuffer *pShaderBuffer );
	void DestroyGeometryShader( GeometryShaderHandle_t hShader );
	PixelShaderHandle_t CreatePixelShader( IShaderBuffer *pShaderBuffer );
	void DestroyPixelShader( PixelShaderHandle_t hShader );
	IVertexBuffer *CreateVertexBuffer( ShaderBufferType_t type, VertexFormat_t fmt, int nVertexCount, const char *pBudgetGroup );
	void DestroyVertexBuffer( IVertexBuffer *pVertexBuffer );
	IIndexBuffer *CreateIndexBuffer( ShaderBufferType_t bufferType, MaterialIndexFormat_t fmt, int nIndexCount, const char *pBudgetGroup );
	void DestroyIndexBuffer( IIndexBuffer *pIndexBuffer );
	IVertexBuffer *GetDynamicVertexBuffer( int nStreamID, VertexFormat_t vertexFormat, bool bBuffered );
	IIndexBuffer *GetDynamicIndexBuffer();
	void SetHardwareGammaRamp( float fGamma, float fGammaTVRangeMin, float fGammaTVRangeMax, float fGammaTVExponent, bool bTVEnabled );
	void EnableNonInteractiveMode( MaterialNonInteractiveMode_t mode, ShaderNonInteractiveInfo_t *pInfo );
	void RefreshFrontBufferNonInteractive();
	void HandleThreadEvent( uint32 threadEvent );
	void DoStartupShaderPreloading();
	const char *GetDeviceName();
private:
	CEmptyMesh m_Mesh;
	CEmptyMesh m_DynamicMesh;
};

//-----------------------------------------------------------------------------
// Empty shader shadow
//-----------------------------------------------------------------------------
class CShaderShadowEmpty : public IShaderShadow
{
public:
	CShaderShadowEmpty();
	virtual ~CShaderShadowEmpty();

	void SetDefaultState();
	void DepthFunc( ShaderDepthFunc_t depthFunc );
	void EnableDepthWrites( bool bEnable );
	void EnableDepthTest( bool bEnable );
	void EnablePolyOffset( PolygonOffsetMode_t nOffsetMode );
	void EnableColorWrites( bool bEnable );
	void EnableAlphaWrites( bool bEnable );
	void EnableBlending( bool bEnable );
	void EnableBlendingForceOpaque( bool bEnable );
	void BlendFunc( ShaderBlendFactor_t srcFactor, ShaderBlendFactor_t dstFactor );
	void EnableAlphaTest( bool bEnable );
	void AlphaFunc( ShaderAlphaFunc_t alphaFunc, float alphaRef );
	void PolyMode( ShaderPolyModeFace_t face, ShaderPolyMode_t polyMode );
	void EnableCulling( bool bEnable );
	void VertexShaderVertexFormat( unsigned int nFlags, int nTexCoordCount, int *pTexCoordDimensions, int nUserDataSize );
	void EnableTexture( Sampler_t sampler, bool bEnable );
	void EnableVertexTexture( VertexTextureSampler_t sampler, bool bEnable );
	void EnableBlendingSeparateAlpha( bool bEnable );
	void BlendFuncSeparateAlpha( ShaderBlendFactor_t srcFactor, ShaderBlendFactor_t dstFactor );
	void SetVertexShader( const char *pFileName, int nStaticVshIndex );
	void SetPixelShader( const char *pFileName, int nStaticPshIndex );
	void EnableSRGBWrite( bool bEnable );
	void EnableSRGBRead( Sampler_t sampler, bool bEnable );
	void FogMode( ShaderFogMode_t fogMode, bool bEnable );
	void DisableFogGammaCorrection( bool bDisable );
	virtual void ExecuteCommandBuffer( uint8 *pBuf );
	void EnableAlphaToCoverage( bool bEnable );
	virtual void SetShadowDepthFiltering( Sampler_t stage );
	void BlendOp( ShaderBlendOp_t blendOp );
	void BlendOpSeparateAlpha( ShaderBlendOp_t blendOp );
	float GetLightMapScaleFactor() const;
public:
	bool m_IsTranslucent;
	bool m_ForceOpaque;
	bool m_IsAlphaTested;
	bool m_bIsDepthWriteEnabled;
	bool m_bUsesVertexAndPixelShaders;
};

//-----------------------------------------------------------------------------
// Empty implementation of shader API
//-----------------------------------------------------------------------------
class CShaderAPIEmpty : public IShaderAPI, public IHardwareConfigInternal, public IDebugTextureInfo
{
public:
	CShaderAPIEmpty();
	virtual ~CShaderAPIEmpty();

	virtual bool IsDebugTextureListFresh( int numFramesAllowed );
	virtual bool SetDebugTextureRendering( bool bEnable );
	virtual void EnableDebugTextureList( bool bEnable );
	virtual void EnableGetAllTextures( bool bEnable );
	virtual KeyValues *LockDebugTextureList();
	virtual void UnlockDebugTextureList();
	virtual KeyValues *GetDebugTextureList();
	virtual int GetTextureMemoryUsed( TextureMemoryType eTextureMemory );

	virtual void GetBackBufferDimensions( int &width, int &height ) const;
	virtual const AspectRatioInfo_t &GetAspectRatioInfo() const;
	virtual void GetCurrentRenderTargetDimensions( int &nWidth, int &nHeight ) const;
	virtual void GetCurrentViewport( int &nX, int &nY, int &nWidth, int &nHeight ) const;
	virtual void GetCurrentColorCorrection( ShaderColorCorrectionInfo_t *pInfo );
	virtual void SetViewports( int nCount, const ShaderViewport_t *pViewports, bool setImmediately );
	virtual int GetViewports( ShaderViewport_t *pViewports, int nMax ) const;
	virtual void ClearBuffers( bool bClearColor, bool bClearDepth, bool bClearStencil, int renderTargetWidth, int renderTargetHeight );
	virtual void ClearBuffersEx( bool bClearRed, bool bClearGreen, bool bClearBlue, bool bClearAlpha, unsigned char r, unsigned char b, unsigned char g, unsigned char a );
	virtual void ClearColor3ub( unsigned char r, unsigned char g, unsigned char b );
	virtual void ClearColor4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a );
	virtual void BindVertexShader( VertexShaderHandle_t hVertexShader );
	virtual void BindGeometryShader( GeometryShaderHandle_t hGeometryShader );
	virtual void BindPixelShader( PixelShaderHandle_t hPixelShader );
	virtual void SetRasterState( const ShaderRasterState_t &state );
	virtual void MarkUnusedVertexFields( unsigned int nFlags, int nTexCoordCount, bool *pUnusedTexCoords );
	virtual bool OwnGPUResources( bool bEnable );
	virtual void OnPresent();
	virtual bool DoRenderTargetsNeedSeparateDepthBuffer() const;
	virtual void ClearSnapshots();
	virtual bool SetMode( void *hwnd, int nAdapter, const ShaderDeviceInfo_t &info );
	virtual void ChangeVideoMode( const ShaderDeviceInfo_t &info );
	virtual void DXSupportLevelChanged( int nDXLevel );
	virtual void EnableUserClipTransformOverride( bool bEnable );
	virtual void UserClipTransform( const VMatrix &worldToView );
	virtual void SetDefaultState();
	virtual StateSnapshot_t TakeSnapshot();
	virtual bool IsTranslucent( StateSnapshot_t id ) const;
	virtual bool IsAlphaTested( StateSnapshot_t id ) const;
	virtual bool UsesVertexAndPixelShaders( StateSnapshot_t id ) const;
	virtual bool IsDepthWriteEnabled( StateSnapshot_t id ) const;
	virtual VertexFormat_t ComputeVertexFormat( int numSnapshots, StateSnapshot_t *pIds ) const;
	virtual VertexFormat_t ComputeVertexUsage( int numSnapshots, StateSnapshot_t *pIds ) const;
	virtual void BeginPass( StateSnapshot_t snapshot );
	void UseSnapshot( StateSnapshot_t snapshot );
	CMeshBuilder *GetVertexModifyBuilder();
	virtual void SetLightingState( const MaterialLightingState_t &state );
	virtual void SetLights( int nCount, const LightDesc_t *pDesc );
	virtual void SetLightingOrigin( Vector vLightingOrigin );
	virtual void SetAmbientLightCube( Vector4D cube[6] );
	virtual void CopyRenderTargetToTexture( ShaderAPITextureHandle_t textureHandle );
	virtual void CopyRenderTargetToTextureEx( ShaderAPITextureHandle_t textureHandle, int nRenderTargetID, Rect_t *pSrcRect, Rect_t *pDstRect );
	virtual void CopyTextureToRenderTargetEx( int nRenderTargetID, ShaderAPITextureHandle_t textureHandle, Rect_t *pSrcRect, Rect_t *pDstRect );
	virtual void SetNumBoneWeights( int numBones );
	virtual void EnableHWMorphing( bool bEnable );
	virtual IMesh *GetDynamicMesh( IMaterial *pMaterial, int nHWSkinBoneCount, bool bBuffered, IMesh *pVertexOverride, IMesh *pIndexOverride);
	virtual IMesh *GetDynamicMeshEx( IMaterial *pMaterial, VertexFormat_t vertexFormat, int nHWSkinBoneCount, bool bBuffered, IMesh *pVertexOverride, IMesh *pIndexOverride );
	virtual IMesh *GetFlexMesh();
	virtual void RenderPass( const unsigned char *pInstanceCommandBuffer, int nPass, int nPassCount );
	virtual void MatrixMode( MaterialMatrixMode_t matrixMode );
	virtual void PushMatrix();
	virtual void PopMatrix();
	virtual void LoadMatrix( float *m );
	virtual void LoadBoneMatrix( int boneIndex, const float *m );
	virtual void MultMatrix( float *m );
	virtual void MultMatrixLocal( float *m );
	virtual void GetActualProjectionMatrix( float *m );
	virtual void GetMatrix( MaterialMatrixMode_t matrixMode, float *dst );
	virtual void LoadIdentity();
	virtual void LoadCameraToWorld();
	virtual void Ortho( double left, double right, double bottom, double top, double zNear, double zFar );
	virtual void PerspectiveX( double fovx, double aspect, double zNear, double zFar );
	virtual void PerspectiveOffCenterX( double fovx, double aspect, double zNear, double zFar, double bottom, double top, double left, double right );
	virtual void PickMatrix( int x, int y, int width, int height );
	virtual void Rotate( float angle, float x, float y, float z );
	virtual void Translate( float x, float y, float z );
	virtual void Scale( float x, float y, float z );
	virtual void ScaleXY( float x, float y );
	void FogMode( MaterialFogMode_t fogMode );
	virtual void FogStart( float fStart );
	virtual void FogEnd( float fEnd );
	virtual void SetFogZ( float fogZ );
	virtual void FogMaxDensity( float flMaxDensity );
	virtual void GetFogDistances( float *fStart, float *fEnd, float *fFogZ );
	void FogColor3f( float r, float g, float b );
	void FogColor3fv( float const *rgb );
	void FogColor3ub( unsigned char r, unsigned char g, unsigned char b );
	void FogColor3ubv( unsigned char const *rgb );
	virtual void SceneFogColor3ub( unsigned char r, unsigned char g, unsigned char b );
	virtual void SceneFogMode( MaterialFogMode_t fogMode );
	virtual void GetSceneFogColor( unsigned char *rgb );
	virtual MaterialFogMode_t GetSceneFogMode();
	virtual int GetPixelFogCombo();
	virtual void SetHeightClipZ( float z ); 
	virtual void SetHeightClipMode( enum MaterialHeightClipMode_t heightClipMode ); 
	virtual void SetClipPlane( int index, const float *pPlane );
	virtual void EnableClipPlane( int index, bool bEnable );
	virtual void SetFastClipPlane( const float *pPlane );
	virtual void EnableFastClip( bool bEnable );
	virtual int GetCurrentDynamicVBSize();
	virtual int GetCurrentDynamicVBSize( int nIndex );
	virtual void DestroyVertexBuffers( bool bExitingLevel );
	virtual void GetGPUMemoryStats( GPUMemoryStats &stats );
	virtual void SetVertexShaderIndex( int vshIndex );
	virtual void SetPixelShaderIndex( int pshIndex );
	virtual void SetVertexShaderConstant( int var, float const *pVec, int numConst, bool bForce );
	virtual void SetBooleanVertexShaderConstant( int var, BOOL const *pVec, int numBools, bool bForce );
	virtual void SetIntegerVertexShaderConstant( int var, int const *pVec, int numIntVecs, bool bForce );
	virtual void SetPixelShaderConstant( int var, float const *pVec, int numConst, bool bForce );
	virtual void SetBooleanPixelShaderConstant( int var, BOOL const *pVec, int numBools, bool bForce);
	virtual void SetIntegerPixelShaderConstant( int var, int const *pVec, int numIntVecs, bool bForce);
	virtual void InvalidateDelayedShaderConstants();
	virtual float GammaToLinear_HardwareSpecific( float fGamma ) const;
	virtual float LinearToGamma_HardwareSpecific( float fLinear ) const;
	virtual void SetLinearToGammaConversionTextures( ShaderAPITextureHandle_t hSRGBWriteEnabledTexture, ShaderAPITextureHandle_t hIdentityTexture );
	virtual void CullMode( MaterialCullMode_t cullMode );
	virtual void FlipCullMode();
	virtual void ForceDepthFuncEquals( bool bEnable );
	virtual void OverrideDepthEnable( bool bEnable, bool bDepthEnable, bool bDepthTestEnable );
	virtual void OverrideAlphaWriteEnable( bool bOverrideEnable, bool bAlphaWriteEnable );
	virtual void OverrideColorWriteEnable( bool bOverrideEnable, bool bColorWriteEnable );
	virtual void ShadeMode( ShaderShadeMode_t mode );
	virtual void Bind( IMaterial *pMaterial );
	virtual ImageFormat GetNearestSupportedFormat( ImageFormat fmt, bool bFilteringRequired ) const;
	virtual ImageFormat GetNearestRenderTargetFormat( ImageFormat fmt ) const;
	virtual void BindTexture( Sampler_t stage, TextureBindFlags_t nBindFlags, ShaderAPITextureHandle_t textureHandle );
	virtual void BindVertexTexture( VertexTextureSampler_t nSampler, ShaderAPITextureHandle_t textureHandle );
	virtual void SetRenderTarget( ShaderAPITextureHandle_t colorTextureHandle, ShaderAPITextureHandle_t depthTextureHandle );
	virtual void SetRenderTargetEx( int nRenderTargetID, ShaderAPITextureHandle_t colorTextureHandle, ShaderAPITextureHandle_t depthTextureHandle );
	virtual void ModifyTexture( ShaderAPITextureHandle_t textureHandle );
	virtual void TexImage2D( int level, int cubeFaceID, ImageFormat dstFormat, int zOffset, int width, int height, ImageFormat srcFormat, bool bSrcIsTiled, void *imageData );
	virtual void TexSubImage2D( int level, int cubeFaceID, int xOffset, int yOffset, int zOffset, int width, int height, ImageFormat srcFormat, int srcStride, bool bSrcIsTiled, void *imageData );
	virtual bool TexLock( int level, int cubeFaceID, int xOffset, int yOffset, int width, int height, CPixelWriter &writer );
	virtual void TexUnlock();
	virtual void UpdateTexture( int xOffset, int yOffset, int w, int h, ShaderAPITextureHandle_t hDstTexture, ShaderAPITextureHandle_t hSrcTexture );
	virtual void *LockTex( ShaderAPITextureHandle_t hTexture );
	virtual void UnlockTex( ShaderAPITextureHandle_t hTexture );
	virtual void *GetD3DTexturePtr( ShaderAPITextureHandle_t hTexture );
	virtual void TexMinFilter( ShaderTexFilterMode_t texFilterMode );
	virtual void TexMagFilter( ShaderTexFilterMode_t texFilterMode );
	virtual void TexWrap( ShaderTexCoordComponent_t coord, ShaderTexWrapMode_t wrapMode );
	virtual void TexSetPriority( int priority );
	virtual ShaderAPITextureHandle_t CreateTexture( int width, int height, int depth, ImageFormat dstImageFormat, int numMipLevels, int numCopies, int flags, const char *pDebugName, const char *pTextureGroupName );
	virtual void CreateTextures( ShaderAPITextureHandle_t *pHandles, int count, int width, int height, int depth, ImageFormat dstImageFormat, int numMipLevels, int numCopies, int flags, const char *pDebugName, const char *pTextureGroupName );
	virtual ShaderAPITextureHandle_t CreateDepthTexture( ImageFormat renderTargetFormat, int width, int height, const char *pDebugName, bool bTexture, bool bAliasDepthTextureOverSceneDepthX360 );
	virtual void DeleteTexture( ShaderAPITextureHandle_t textureHandle );
	virtual bool IsTexture( ShaderAPITextureHandle_t textureHandle );
	virtual bool IsTextureResident( ShaderAPITextureHandle_t textureHandle );
	virtual void ClearBuffersObeyStencil( bool bClearColor, bool bClearDepth );
	virtual void ClearBuffersObeyStencilEx( bool bClearColor, bool bClearAlpha, bool bClearDepth );
	virtual void PerformFullScreenStencilOperation();
	virtual void ReadPixels( int x, int y, int width, int height, unsigned char *data, ImageFormat dstFormat, ITexture *pTexture );
	virtual void ReadPixelsAsync( int x, int y, int width, int height, unsigned char *data, ImageFormat dstFormat, ITexture *pTexture, CThreadEvent *pEvent );
	virtual void ReadPixelsAsyncGetResult( int x, int y, int width, int height, unsigned char *data, ImageFormat dstFormat, CThreadEvent *pEvent );
	virtual void ReadPixels( Rect_t *pSrcRect, Rect_t *pDstRect, unsigned char *data, ImageFormat dstFormat, int nDstStride );
	virtual int SelectionMode( bool selectionMode );
	virtual void SelectionBuffer( unsigned int *pBuffer, int size );
	virtual void ClearSelectionNames();
	virtual void LoadSelectionName( int name );
	virtual void PushSelectionName( int name );
	virtual void PopSelectionName();
	virtual void FlushHardware();
	virtual void ResetRenderState( bool bFullReset );
	virtual void SetScissorRect( int nLeft, int nTop, int nRight, int nBottom, bool bEnableScissor );
	virtual bool CanDownloadTextures() const;
	virtual void BeginFrame();
	virtual void EndFrame();
	virtual double CurrentTime() const;
	virtual void GetWorldSpaceCameraPosition( float *pPos ) const;
	virtual void GetWorldSpaceCameraDirection( float *pDir ) const;
	bool HasDestAlphaBuffer() const;
	bool HasStencilBuffer() const;
	virtual int MaxViewports() const;
	virtual void OverrideStreamOffsetSupport( bool bOverrideEnabled, bool bEnableSupport );
	virtual ShadowFilterMode_t GetShadowFilterMode( bool bForceLowQualityShadows, bool bPS30 ) const;
	virtual int StencilBufferBits() const;
	virtual int GetFrameBufferColorDepth() const;
	virtual int GetSamplerCount() const;
	virtual int GetVertexSamplerCount() const;
	virtual bool HasSetDeviceGammaRamp() const;
	bool SupportsCompressedTextures() const;
	virtual VertexCompressionType_t SupportsCompressedVertices() const;
	virtual bool SupportsStaticControlFlow() const;
	virtual int MaximumAnisotropicLevel() const;
	virtual int MaxTextureWidth() const;
	virtual int MaxTextureHeight() const;
	virtual int MaxTextureAspectRatio() const;
	virtual int GetDXSupportLevel() const;
	virtual int GetMinDXSupportLevel() const;
	virtual bool SupportsShadowDepthTextures() const;
	virtual ImageFormat GetShadowDepthTextureFormat() const;
	virtual ImageFormat GetHighPrecisionShadowDepthTextureFormat() const;
	virtual ImageFormat GetNullTextureFormat() const;
	virtual const char *GetShaderDLLName() const;
	virtual int TextureMemorySize() const;
	virtual bool SupportsMipmappedCubemaps() const;
	virtual int NumVertexShaderConstants() const;
	int NumBooleanVertexShaderConstants() const;
	int NumIntegerVertexShaderConstants() const;
	virtual int NumPixelShaderConstants() const;
	virtual int MaxNumLights() const;
	virtual int MaxVertexShaderBlendMatrices() const;
	virtual int MaxUserClipPlanes() const;
	virtual bool UseFastClipping() const;
	virtual bool SpecifiesFogColorInLinearSpace() const;
	virtual bool SupportsSRGB() const;
	virtual bool FakeSRGBWrite() const;
	virtual bool CanDoSRGBReadFromRTs() const;
	virtual bool SupportsGLMixedSizeTargets() const;
	virtual const char *GetHWSpecificShaderDLLName() const;
	virtual bool NeedsAAClamp() const;
	virtual int MaxHWMorphBatchCount() const;
	virtual int GetMaxDXSupportLevel() const;
	virtual bool ReadPixelsFromFrontBuffer() const;
	virtual bool PreferDynamicTextures() const;
	virtual void ForceHardwareSync();
	virtual int GetCurrentNumBones() const;
	virtual bool IsHWMorphingEnabled() const;
	virtual void GetDX9LightState( LightState_t *state ) const;
	virtual MaterialFogMode_t GetCurrentFogType() const;
	void RecordString( const char *pStr );
	virtual void EvictManagedResources();
	virtual void GetLightmapDimensions( int *w, int *h );
	virtual void SyncToken( const char *pToken );
	virtual void SetStandardVertexShaderConstants( float fOverbright );
	virtual void SetAnisotropicLevel( int nAnisotropyLevel );
	virtual bool SupportsHDR() const;
	virtual HDRType_t GetHDRType() const;
	virtual HDRType_t GetHardwareHDRType() const;
	virtual bool NeedsATICentroidHack() const;
	virtual bool SupportsColorOnSecondStream() const;
	virtual bool SupportsStaticPlusDynamicLighting() const;
	virtual bool SupportsStreamOffset() const;
	virtual void CommitPixelShaderLighting( int pshReg );
	virtual ShaderAPIOcclusionQuery_t CreateOcclusionQueryObject();
	virtual void DestroyOcclusionQueryObject( ShaderAPIOcclusionQuery_t );
	virtual void BeginOcclusionQueryDrawing( ShaderAPIOcclusionQuery_t );
	virtual void EndOcclusionQueryDrawing( ShaderAPIOcclusionQuery_t );
	virtual int OcclusionQuery_GetNumPixelsRendered( ShaderAPIOcclusionQuery_t hQuery, bool bFlush );
	virtual void AcquireThreadOwnership();
	virtual void ReleaseThreadOwnership();
	virtual bool SupportsBorderColor() const;
	virtual bool SupportsFetch4() const;
	virtual void EnableBuffer2FramesAhead( bool bEnable );
	virtual float GetShadowDepthBias() const;
	virtual float GetShadowSlopeScaleDepthBias() const;
	virtual bool PreferZPrepass() const;
	virtual bool SuppressPixelShaderCentroidHackFixup() const;
	virtual bool PreferTexturesInHWMemory() const;
	virtual bool PreferHardwareSync() const;
	virtual bool IsUnsupported() const;
	virtual void SetDepthFeatheringPixelShaderConstant( int iConstant, float fDepthBlendScale );
	virtual TessellationMode_t GetTessellationMode() const;
	virtual void SetPixelShaderFogParams( int reg );
	virtual bool InFlashlightMode() const;
	virtual bool IsRenderingPaint() const;
	virtual bool InEditorMode() const;
	virtual void BindStandardTexture( Sampler_t stage, TextureBindFlags_t nBindFlags, StandardTextureId_t id );
	virtual void BindStandardVertexTexture( VertexTextureSampler_t sampler, StandardTextureId_t id );
	virtual void GetStandardTextureDimensions( int *pWidth, int *pHeight, StandardTextureId_t id );
	virtual float GetSubDHeight();
	virtual bool IsStereoActiveThisFrame() const;
	virtual void SetFlashlightState( const FlashlightState_t &state, const VMatrix &worldToTexture );
	virtual void SetFlashlightStateEx( const FlashlightState_t &state, const VMatrix &worldToTexture, ITexture *pFlashlightDepthTexture );
	virtual bool IsCascadedShadowMapping() const;
	virtual void SetCascadedShadowMapping( bool bEnable );
	virtual void SetCascadedShadowMappingState( const CascadedShadowMappingState_t &state, ITexture *pDepthTextureAtlas );
	virtual const CascadedShadowMappingState_t &GetCascadedShadowMappingState( ITexture **pDepthTextureAtlas ) const;
	virtual const FlashlightState_t &GetFlashlightState( VMatrix &worldToTexture ) const;
	virtual const FlashlightState_t &GetFlashlightStateEx( VMatrix &worldToTexture, ITexture **pFlashlightDepthTexture ) const;
	virtual void GetFlashlightShaderInfo( bool *pShadowsEnabled, bool *pUberLight ) const;
	virtual float GetFlashlightAmbientOcclusion() const;
	virtual void ClearVertexAndPixelShaderRefCounts();
	virtual void PurgeUnusedVertexAndPixelShaders();
	virtual void DrawInstances( int nInstanceCount, const MeshInstanceData_t *pInstance );
	virtual bool IsAAEnabled() const;
	virtual int GetVertexTextureCount() const;
	virtual int GetMaxVertexTextureDimension() const;
	virtual int MaxTextureDepth() const;
	virtual void SetFlexWeights( int nFirstWeight, int nCount, const MorphWeight_t *pWeights );
	virtual ITexture *GetRenderTargetEx( int nRenderTargetID ) const;
	virtual void SetToneMappingScaleLinear( const Vector &scale );
	virtual const Vector &GetToneMappingScaleLinear( void ) const;
	virtual void HandleDeviceLost();
	virtual void EnableLinearColorSpaceFrameBuffer( bool bEnable );
	virtual void SetFullScreenTextureHandle( ShaderAPITextureHandle_t h );
	virtual void SetFloatRenderingParameter(int parm_number, float value);
	virtual void SetIntRenderingParameter(int parm_number, int value);
	virtual void SetTextureRenderingParameter(int parm_number, ITexture *pTexture);
	virtual void SetVectorRenderingParameter(int parm_number, Vector const &value);
	virtual float GetFloatRenderingParameter(int parm_number) const;
	virtual int GetIntRenderingParameter(int parm_number) const;
	virtual ITexture *GetTextureRenderingParameter(int parm_number) const;
	virtual Vector GetVectorRenderingParameter(int parm_number) const;
	virtual void SetStencilState( const ShaderStencilState_t &state );
	virtual void ClearStencilBufferRectangle( int xmin, int ymin, int xmax, int ymax, int value );
	virtual void GetMaxToRender( IMesh *pMesh, bool bMaxUntilFlush, int *pMaxVerts, int *pMaxIndices );
	virtual int GetMaxVerticesToRender( IMaterial *pMaterial );
	virtual int GetMaxIndicesToRender();
	virtual int CompareSnapshots( StateSnapshot_t snapshot0, StateSnapshot_t snapshot1 );
	virtual void DisableAllLocalLights();
	virtual bool SupportsMSAAMode( int nMSAAMode );
	virtual bool SupportsCSAAMode( int nNumSamples, int nQualityLevel );
	virtual void BeginPIXEvent( unsigned long color, const char *szName );
	virtual void EndPIXEvent();
	virtual void SetPIXMarker( unsigned long color, const char *szName );
	virtual void ComputeVertexDescription( unsigned char *pBuffer, VertexFormat_t vertexFormat, MeshDesc_t &desc ) const;
	virtual int VertexFormatSize( VertexFormat_t vertexFormat ) const;
	virtual bool SupportsShadowDepthTextures();
	virtual bool SupportsFetch4();
	virtual int NeedsShaderSRGBConversion() const;
	virtual bool UsesSRGBCorrectBlending() const;
	virtual bool HasFastVertexTextures() const;
	virtual bool ActualHasFastVertexTextures() const;
	virtual void SetShadowDepthBiasFactors( float fShadowSlopeScaleDepthBias, float fShadowDepthBias );
	virtual void SetDisallowAccess( bool );
	virtual void EnableShaderShaderMutex( bool );
	virtual void ShaderLock();
	virtual void ShaderUnlock();
	virtual void BindVertexBuffer( int nStreamID, IVertexBuffer *pVertexBuffer, int nOffsetInBytes, int nFirstVertex, int nVertexCount, VertexFormat_t fmt, int nRepetitions ) ;
	virtual void BindIndexBuffer( IIndexBuffer *pIndexBuffer, int nOffsetInBytes ) ;
	virtual void Draw( MaterialPrimitiveType_t primitiveType, int nFirstIndex, int nIndexCount );
	virtual int GetVertexBufferCompression() const;
	virtual bool ShouldWriteDepthToDestAlpha() const;
	virtual bool SupportsHDRMode( HDRType_t nHDRMode ) const;
	virtual bool IsDX10Card() const;
	virtual void PushDeformation( const DeformationBase_t *Deformation );
	virtual void PopDeformation();
	virtual int GetNumActiveDeformations() const;
	virtual int GetPackedDeformationInformation( int nMaskOfUnderstoodDeformations, float *pConstantValuesOut, int nBufferSize, int nMaximumDeformations, int *pNumDefsOut ) const;
	virtual void SetStandardTextureHandle( StandardTextureId_t nId, ShaderAPITextureHandle_t nHandle );
	virtual void ExecuteCommandBuffer( uint8 *pCmdBuffer );
	virtual bool GetHDREnabled() const;
	virtual void SetHDREnabled( bool bEnable );
	virtual void SetTextureFilterMode( Sampler_t sampler, TextureFilterMode_t nMode );
	virtual void SetScreenSizeForVPOS( int pshReg );
	virtual void SetVSNearAndFarZ( int vshReg );
	virtual float GetFarZ();
	virtual void EnableSinglePassFlashlightMode( bool bEnable );
	virtual bool SinglePassFlashlightModeEnabled();
	virtual void FlipCulling( bool bFlipCulling );
	virtual void UpdateGameTime( float flTime );
	virtual bool IsStereoSupported( void ) const;
	virtual void UpdateStereoTexture( ShaderAPITextureHandle_t texHandle, bool *pStereoActiveThisFrame );
	virtual void SetSRGBWrite( bool bState );
	virtual void PrintfVA( char *fmt, va_list vargs );
	virtual void Printf( char *fmt, ... );
	virtual float Knob( char *knobname, float *setvalue );
	virtual void AddShaderComboInformation( const ShaderComboSemantics_t *pSemantics );
	virtual void SpinPresent( unsigned int nFrames );
	virtual void AntiAliasingHint( int a1 );
	virtual bool SupportsCascadedShadowMapping() const;
	virtual CSMQualityMode_t GetCSMQuality() const;
	virtual bool SupportsBilinearPCFSampling() const;
	virtual CSMShaderMode_t GetCSMShaderMode( CSMQualityMode_t nQualityLevel ) const;
	virtual void EnableAlphaToCoverage();
	virtual void DisableAlphaToCoverage();
	virtual float GetLightMapScaleFactor( void ) const;
	virtual ShaderAPITextureHandle_t FindTexture( const char *pDebugName );
	virtual void GetTextureDimensions( ShaderAPITextureHandle_t hTexture, int &nWidth, int &nHeight, int &nDepth );
	virtual void SetVertexShaderViewProj();
	virtual void UpdateVertexShaderMatrix( int );
	virtual void SetVertexShaderModelViewProjAndModelView();
	virtual void SetVertexShaderCameraPos();
	virtual int SetSkinningMatrices( const MeshInstanceData_t & );
	virtual void BeginGeneratingCSMs();
	virtual void EndGeneratingCSMs();
	virtual void PerpareForCascadeDraw( int, float, float );
	virtual bool GetCSMAccurateBlending() const;
	virtual void SetCSMAccurateBlending( bool bEnable );
	virtual bool SupportsResolveDepth() const;
	virtual bool HasFullResolutionDepthTexture() const;
	virtual int NumBooleanPixelShaderConstants() const;
	virtual int NumIntegerPixelShaderConstants() const;
	virtual ShaderAPITextureHandle_t GetStandardTextureHandle( StandardTextureId_t nID );
	virtual bool IsStandardTextureHandleValid( StandardTextureId_t nId ); 
	virtual int Unknown0( int a1, void **a2, int *a3 );
	virtual void Unknown1();
	virtual void Unknown2( int a1, void *a2, int a3 );
	virtual void *Unknown3( void *a1, int a2 );
	virtual void *Unknown4( void *a1, int a2 );
	virtual void Unknown5( void **a1, void **a2, char *a3 );
	virtual void Unknown6( void *a1, int a2 );
private:
	enum
	{
		TRANSLUCENT = 0x01,
		ALPHATESTED = 0x02,
		VERTEX_AND_PIXEL_SHADERS = 0x04,
		DEPTH_WRITE_ENABLED = 0x08
	};

	CEmptyMesh m_Mesh;
};

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------
IShaderUtil *g_pShaderUtil;

static CShaderDeviceMgrEmpty s_ShaderDeviceMgrEmpty;
static CShaderDeviceEmpty s_ShaderDeviceEmpty;

static CShaderAPIEmpty g_ShaderAPIEmpty;
static CShaderShadowEmpty g_ShaderShadow;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CShaderDeviceMgrEmpty, IShaderDeviceMgr, SHADER_DEVICE_MGR_INTERFACE_VERSION, s_ShaderDeviceMgrEmpty );
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CShaderDeviceEmpty, IShaderDevice, SHADER_DEVICE_INTERFACE_VERSION, s_ShaderDeviceEmpty );
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CShaderShadowEmpty, IShaderShadow, SHADERSHADOW_INTERFACE_VERSION, g_ShaderShadow );
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CShaderAPIEmpty, IShaderAPI, SHADERAPI_INTERFACE_VERSION, g_ShaderAPIEmpty );
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CShaderAPIEmpty, IMaterialSystemHardwareConfig, MATERIALSYSTEM_HARDWARECONFIG_INTERFACE_VERSION, g_ShaderAPIEmpty );
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CShaderAPIEmpty, IDebugTextureInfo, DEBUG_TEXTURE_INFO_VERSION, g_ShaderAPIEmpty );

//-----------------------------------------------------------------------------
// Shader interface factory
//-----------------------------------------------------------------------------
void *ShaderInterfaceFactory(const char *pName, int *pReturnCode)
{
	void *pInterface = NULL;

	if (V_strcmp(pName, SHADER_DEVICE_INTERFACE_VERSION) == 0)
		pInterface = &s_ShaderDeviceEmpty;
	else if (V_strcmp(pName, SHADERAPI_INTERFACE_VERSION) == 0)
		pInterface = &g_ShaderAPIEmpty;
	else if (V_strcmp(pName, SHADERSHADOW_INTERFACE_VERSION) == 0)
		pInterface = &g_ShaderShadow;

	if (pReturnCode)
		*pReturnCode = pInterface ? IFACE_OK : IFACE_FAILED;

	return pInterface;
}

//-----------------------------------------------------------------------------
// Empty mesh
//-----------------------------------------------------------------------------
CEmptyMesh::CEmptyMesh( bool bIsDynamic )
{
	m_bIsDynamic = bIsDynamic;
	m_pVertexMemory = new unsigned char[1024];
}

CEmptyMesh::~CEmptyMesh()
{
	delete[] m_pVertexMemory;
}

bool CEmptyMesh::Lock( int nMaxIndexCount, bool bAppend, IndexDesc_t &desc )
{
	static unsigned short s_BogusIndex;

	desc.m_pIndices = &s_BogusIndex;
	desc.m_nIndexSize = 0;
	desc.m_nFirstIndex = 0;
	desc.m_nOffset = 0;

	return true;
}

bool CEmptyMesh::Lock( int nVertexCount, bool bAppend, VertexDesc_t &desc )
{
	desc.m_pPosition = (float *)m_pVertexMemory;
	desc.m_pNormal = (float *)m_pVertexMemory;
	desc.m_pColor = m_pVertexMemory;

	for (int i = 0; i < VERTEX_MAX_TEXTURE_COORDINATES; i++)
		desc.m_pTexCoord[i] = (float *)m_pVertexMemory;

	desc.m_pBoneWeight = (float *)m_pVertexMemory;
	desc.m_pBoneMatrixIndex = (unsigned char *)m_pVertexMemory;
	desc.m_pTangentS = (float *)m_pVertexMemory;
	desc.m_pTangentT = (float *)m_pVertexMemory;
	desc.m_pUserData = (float *)m_pVertexMemory;
	desc.m_NumBoneWeights = 2;

	desc.m_VertexSize_Position = 0;
	desc.m_VertexSize_BoneWeight = 0;
	desc.m_VertexSize_BoneMatrixIndex = 0;
	desc.m_VertexSize_Normal = 0;
	desc.m_VertexSize_Color = 0;

	for (int i = 0; i < VERTEX_MAX_TEXTURE_COORDINATES; i++)
		desc.m_VertexSize_TexCoord[i] = 0;

	desc.m_VertexSize_TangentS = 0;
	desc.m_VertexSize_TangentT = 0;
	desc.m_VertexSize_UserData = 0;
	desc.m_ActualVertexSize = 0;
	desc.m_nFirstVertex = 0;
	desc.m_nOffset = 0;

	return true;
}

void CEmptyMesh::Unlock( int nWrittenIndexCount, IndexDesc_t &desc )
{

}

void CEmptyMesh::Unlock( int nVertexCount, VertexDesc_t &desc )
{

}

void CEmptyMesh::ModifyBegin( bool bReadOnly, int nFirstIndex, int nIndexCount, IndexDesc_t &desc )
{
	Lock(nIndexCount, false, desc);
}

void CEmptyMesh::ModifyBegin( int firstVertex, int numVerts, int firstIndex, int numIndices, MeshDesc_t &desc )
{
	ModifyBeginEx(false, firstVertex, numVerts, firstIndex, numIndices, desc);
}

void CEmptyMesh::ModifyEnd( IndexDesc_t &desc )
{

}

void CEmptyMesh::ModifyEnd( MeshDesc_t &desc )
{

}

void CEmptyMesh::Spew( int nIndexCount, const IndexDesc_t &desc )
{

}

void CEmptyMesh::Spew( int nVertexCount, const VertexDesc_t &desc )
{

}

void CEmptyMesh::Spew( int numVerts, int numIndices, const MeshDesc_t &desc )
{

}

void CEmptyMesh::ValidateData( int nIndexCount, const IndexDesc_t &desc )
{

}

void CEmptyMesh::ValidateData( int nVertexCount, const VertexDesc_t &desc )
{

}

void CEmptyMesh::ValidateData( int numVerts, int numIndices, const MeshDesc_t &desc )
{

}

bool CEmptyMesh::IsDynamic() const
{
	return m_bIsDynamic;
}

void CEmptyMesh::BeginCastBuffer( VertexFormat_t format )
{

}

void CEmptyMesh::BeginCastBuffer( MaterialIndexFormat_t format )
{

}

void CEmptyMesh::EndCastBuffer()
{

}

int CEmptyMesh::GetRoomRemaining() const
{
	return 0;
}

MaterialIndexFormat_t CEmptyMesh::IndexFormat() const
{
	return MATERIAL_INDEX_FORMAT_UNKNOWN;
}

void CEmptyMesh::LockMesh( int numVerts, int numIndices, MeshDesc_t &desc, MeshBuffersAllocationSettings_t *pSettings )
{
	Lock(numVerts, false, (VertexDesc_t &)desc);
	Lock(numIndices, false, (IndexDesc_t &)desc);
}

void CEmptyMesh::UnlockMesh( int numVerts, int numIndices, MeshDesc_t &desc )
{

}

void CEmptyMesh::ModifyBeginEx( bool bReadOnly, int firstVertex, int numVerts, int firstIndex, int numIndices, MeshDesc_t &desc )
{
	Lock(numVerts, false, (VertexDesc_t &)desc);
	Lock(numIndices, false, (IndexDesc_t &)desc);
}

int CEmptyMesh::VertexCount() const
{
	return 0;
}

void CEmptyMesh::SetPrimitiveType( MaterialPrimitiveType_t type )
{

}

void CEmptyMesh::Draw( int firstIndex, int numIndices )
{

}

void CEmptyMesh::Draw( CPrimList *pLists, int nLists )
{

}

void CEmptyMesh::DrawModulated( const Vector4D &vecDiffuseModulation, int firstIndex, int numIndices )
{

}

void CEmptyMesh::CopyToMeshBuilder( int iStartVert, int nVerts, int iStartIndex, int nIndices, int indexOffset, CMeshBuilder &builder )
{

}

IMaterial *CEmptyMesh::GetMaterial()
{
	return NULL;
}

void CEmptyMesh::SetColorMesh( IMesh *pColorMesh, int nVertexOffset )
{

}

int CEmptyMesh::IndexCount() const
{
	return 0;
}

void CEmptyMesh::SetFlexMesh( IMesh *pMesh, int nVertexOffset )
{

}

void CEmptyMesh::DisableFlexMesh()
{

}

void CEmptyMesh::MarkAsDrawn()
{

}

VertexFormat_t CEmptyMesh::GetVertexFormat() const
{
	return 1;
}

IMesh *CEmptyMesh::GetMesh()
{
	return this;
}

unsigned int CEmptyMesh::ComputeMemoryUsed()
{
	return 0;
}

void *CEmptyMesh::AccessRawHardwareDataStream( uint8 nRawStreamIndex, uint32 numBytes, uint32 uiFlags, void *pvContext )
{
	return NULL;
}

ICachedPerFrameMeshData *CEmptyMesh::GetCachedPerFrameMeshData()
{
	return NULL;
}

void CEmptyMesh::ReconstructFromCachedPerFrameMeshData( ICachedPerFrameMeshData *pData )
{

}

//-----------------------------------------------------------------------------
// Empty shader device manager
//-----------------------------------------------------------------------------
bool CShaderDeviceMgrEmpty::Connect( CreateInterfaceFn factory )
{
	g_pShaderUtil = (IShaderUtil *)factory(SHADER_UTIL_INTERFACE_VERSION, NULL);
	return true;
}

void CShaderDeviceMgrEmpty::Disconnect()
{
	g_pShaderUtil = NULL;
}

void *CShaderDeviceMgrEmpty::QueryInterface( const char *pInterfaceName )
{
	if (V_strcmp(pInterfaceName, SHADER_DEVICE_MGR_INTERFACE_VERSION) == 0)
		return this;
	else if (V_strcmp(pInterfaceName, MATERIALSYSTEM_HARDWARECONFIG_INTERFACE_VERSION) == 0)
		return (IHardwareConfigInternal *)&g_ShaderAPIEmpty;

	return NULL;
}

InitReturnVal_t CShaderDeviceMgrEmpty::Init()
{
	return INIT_OK;
}

void CShaderDeviceMgrEmpty::Shutdown()
{

}

int CShaderDeviceMgrEmpty::GetAdapterCount() const
{
	return 0;
}

void CShaderDeviceMgrEmpty::GetAdapterInfo( int nAdapter, MaterialAdapterInfo_t &info ) const
{
	memset(&info, 0, sizeof(MaterialAdapterInfo_t));
	info.m_nDXSupportLevel = 90;
}

bool CShaderDeviceMgrEmpty::GetRecommendedConfigurationInfo( int nAdapter, int nDXLevel, KeyValues *pConfiguration )
{
	return true;
}

int CShaderDeviceMgrEmpty::GetModeCount( int nAdapter ) const
{
	return 0;
}

void CShaderDeviceMgrEmpty::GetModeInfo( ShaderDisplayMode_t *pInfo, int nAdapter, int nMode ) const
{

}

void CShaderDeviceMgrEmpty::GetCurrentModeInfo( ShaderDisplayMode_t *pInfo, int nAdapter ) const
{

}

bool CShaderDeviceMgrEmpty::SetAdapter( int nAdapter, int nFlags )
{
	return true;
}

CreateInterfaceFn CShaderDeviceMgrEmpty::SetMode( void *hWnd, int nAdapter, const ShaderDeviceInfo_t &mode )
{
	return ShaderInterfaceFactory;
}

void CShaderDeviceMgrEmpty::AddModeChangeCallback( ShaderModeChangeCallbackFunc_t func )
{

}

void CShaderDeviceMgrEmpty::RemoveModeChangeCallback( ShaderModeChangeCallbackFunc_t func )
{

}

bool CShaderDeviceMgrEmpty::GetRecommendedVideoConfig( int nAdapter, KeyValues *pConfiguration )
{
	return true;
}

void CShaderDeviceMgrEmpty::AddDeviceDependentObject( IShaderDeviceDependentObject *pObject )
{

}

void CShaderDeviceMgrEmpty::RemoveDeviceDependentObject( IShaderDeviceDependentObject *pObject )
{

}

//-----------------------------------------------------------------------------
// Empty shader device
//-----------------------------------------------------------------------------
CShaderDeviceEmpty::CShaderDeviceEmpty() : m_Mesh(false), m_DynamicMesh(true)
{

}

int CShaderDeviceEmpty::GetCurrentAdapter() const
{
	return 0;
}

bool CShaderDeviceEmpty::IsUsingGraphics() const
{
	return false;
}

void CShaderDeviceEmpty::SpewDriverInfo() const
{
	Warning("Empty shader\n");
}

ImageFormat CShaderDeviceEmpty::GetBackBufferFormat() const
{
	return IMAGE_FORMAT_RGB888;
}

void CShaderDeviceEmpty::GetBackBufferDimensions( int &width, int &height ) const
{
	width = 1024;
	height = 768;
}

const AspectRatioInfo_t &CShaderDeviceEmpty::GetAspectRatioInfo() const
{
	static const AspectRatioInfo_t dummy = { false, false, (4.0f / 3.0f), (4.0f / 3.0f), 1.0f, 1.0f, false };
	return dummy;
}

int CShaderDeviceEmpty::StencilBufferBits() const
{
	return 0;
}

bool CShaderDeviceEmpty::IsAAEnabled() const
{
	return false;
}

void CShaderDeviceEmpty::Present()
{

}

void CShaderDeviceEmpty::GetWindowSize( int &nWidth, int &nHeight ) const
{
	nWidth = 0;
	nHeight = 0;
}

bool CShaderDeviceEmpty::AddView( void *hWnd )
{
	return true;
}

void CShaderDeviceEmpty::RemoveView( void *hWnd )
{

}

void CShaderDeviceEmpty::SetView( void *hWnd )
{

}

void CShaderDeviceEmpty::ReleaseResources( bool bReleaseManagedResources )
{

}

void CShaderDeviceEmpty::ReacquireResources()
{

}

IMesh *CShaderDeviceEmpty::CreateStaticMesh( VertexFormat_t vertexFormat, const char *pTextureBudgetGroup, IMaterial *pMaterial, VertexStreamSpec_t *pStreamSpec )
{
	return &m_Mesh;
}

void CShaderDeviceEmpty::DestroyStaticMesh( IMesh *mesh )
{

}

IShaderBuffer *CShaderDeviceEmpty::CompileShader( const char *pProgram, size_t nBufLen, const char *pShaderVersion )
{
	return NULL;
}

VertexShaderHandle_t CShaderDeviceEmpty::CreateVertexShader( IShaderBuffer *pShaderBuffer )
{
	return VERTEX_SHADER_HANDLE_INVALID;
}

void CShaderDeviceEmpty::DestroyVertexShader( VertexShaderHandle_t hShader )
{

}

GeometryShaderHandle_t CShaderDeviceEmpty::CreateGeometryShader( IShaderBuffer *pShaderBuffer )
{
	return GEOMETRY_SHADER_HANDLE_INVALID;
}

void CShaderDeviceEmpty::DestroyGeometryShader( GeometryShaderHandle_t hShader )
{

}

PixelShaderHandle_t CShaderDeviceEmpty::CreatePixelShader( IShaderBuffer *pShaderBuffer )
{
	return PIXEL_SHADER_HANDLE_INVALID;
}

void CShaderDeviceEmpty::DestroyPixelShader( PixelShaderHandle_t hShader )
{

}

IVertexBuffer *CShaderDeviceEmpty::CreateVertexBuffer( ShaderBufferType_t type, VertexFormat_t fmt, int nVertexCount, const char *pBudgetGroup )
{
	if (IsDynamicBufferType(type))
		return &m_DynamicMesh;
	else
		return &m_Mesh;
}

void CShaderDeviceEmpty::DestroyVertexBuffer( IVertexBuffer *pVertexBuffer )
{

}

IIndexBuffer *CShaderDeviceEmpty::CreateIndexBuffer( ShaderBufferType_t bufferType, MaterialIndexFormat_t fmt, int nIndexCount, const char *pBudgetGroup )
{
	if (IsDynamicBufferType(bufferType))
		return &m_DynamicMesh;
	else
		return &m_Mesh;
}

void CShaderDeviceEmpty::DestroyIndexBuffer( IIndexBuffer *pIndexBuffer )
{

}

IVertexBuffer *CShaderDeviceEmpty::GetDynamicVertexBuffer( int nStreamID, VertexFormat_t vertexFormat, bool bBuffered )
{
	return &m_DynamicMesh;
}

IIndexBuffer *CShaderDeviceEmpty::GetDynamicIndexBuffer()
{
	return &m_Mesh;
}

void CShaderDeviceEmpty::SetHardwareGammaRamp( float fGamma, float fGammaTVRangeMin, float fGammaTVRangeMax, float fGammaTVExponent, bool bTVEnabled )
{

}

void CShaderDeviceEmpty::EnableNonInteractiveMode( MaterialNonInteractiveMode_t mode, ShaderNonInteractiveInfo_t *pInfo )
{

}

void CShaderDeviceEmpty::RefreshFrontBufferNonInteractive()
{

}

void CShaderDeviceEmpty::HandleThreadEvent( uint32 threadEvent )
{

}

void CShaderDeviceEmpty::DoStartupShaderPreloading()
{

}

const char *CShaderDeviceEmpty::GetDeviceName()
{
	return "";
}

//-----------------------------------------------------------------------------
// Empty shader shadow
//-----------------------------------------------------------------------------
CShaderShadowEmpty::CShaderShadowEmpty()
{
	m_IsTranslucent = false;
	m_ForceOpaque = false;
	m_IsAlphaTested = false;
	m_bIsDepthWriteEnabled = true;
	m_bUsesVertexAndPixelShaders = false;
}

CShaderShadowEmpty::~CShaderShadowEmpty()
{

}

void CShaderShadowEmpty::SetDefaultState()
{
	m_IsTranslucent = false;
	m_ForceOpaque = false;
	m_IsAlphaTested = false;
	m_bIsDepthWriteEnabled = true;
	m_bUsesVertexAndPixelShaders = false;
}

void CShaderShadowEmpty::DepthFunc( ShaderDepthFunc_t depthFunc )
{

}

void CShaderShadowEmpty::EnableDepthWrites( bool bEnable )
{
	m_bIsDepthWriteEnabled = bEnable;
}

void CShaderShadowEmpty::EnableDepthTest( bool bEnable )
{

}

void CShaderShadowEmpty::EnablePolyOffset( PolygonOffsetMode_t nOffsetMode )
{

}

void CShaderShadowEmpty::EnableColorWrites( bool bEnable )
{

}

void CShaderShadowEmpty::EnableAlphaWrites( bool bEnable )
{

}

void CShaderShadowEmpty::EnableBlending( bool bEnable )
{
	m_IsTranslucent = bEnable;
	m_ForceOpaque = false;
}

void CShaderShadowEmpty::EnableBlendingForceOpaque( bool bEnable )
{
	m_IsTranslucent = bEnable;
	m_ForceOpaque = true;
}

void CShaderShadowEmpty::BlendFunc( ShaderBlendFactor_t srcFactor, ShaderBlendFactor_t dstFactor )
{

}

void CShaderShadowEmpty::EnableAlphaTest( bool bEnable )
{
	m_IsAlphaTested = bEnable;
}

void CShaderShadowEmpty::AlphaFunc( ShaderAlphaFunc_t alphaFunc, float alphaRef )
{

}

void CShaderShadowEmpty::PolyMode( ShaderPolyModeFace_t face, ShaderPolyMode_t polyMode )
{

}

void CShaderShadowEmpty::EnableCulling( bool bEnable )
{

}

void CShaderShadowEmpty::VertexShaderVertexFormat( unsigned int nFlags, int nTexCoordCount, int *pTexCoordDimensions, int nUserDataSize )
{

}

void CShaderShadowEmpty::EnableTexture( Sampler_t sampler, bool bEnable )
{

}

void CShaderShadowEmpty::EnableVertexTexture( VertexTextureSampler_t sampler, bool bEnable )
{

}

void CShaderShadowEmpty::EnableBlendingSeparateAlpha( bool bEnable )
{

}

void CShaderShadowEmpty::BlendFuncSeparateAlpha( ShaderBlendFactor_t srcFactor, ShaderBlendFactor_t dstFactor )
{

}

void CShaderShadowEmpty::SetVertexShader( const char *pFileName, int nStaticVshIndex )
{
	m_bUsesVertexAndPixelShaders = (pFileName != NULL);
}

void CShaderShadowEmpty::SetPixelShader( const char *pFileName, int nStaticPshIndex )
{
	m_bUsesVertexAndPixelShaders = (pFileName != NULL);
}

void CShaderShadowEmpty::EnableSRGBWrite( bool bEnable )
{

}

void CShaderShadowEmpty::EnableSRGBRead( Sampler_t sampler, bool bEnable )
{

}

void CShaderShadowEmpty::FogMode( ShaderFogMode_t fogMode, bool bEnable )
{

}

void CShaderShadowEmpty::DisableFogGammaCorrection( bool bDisable )
{

}

void CShaderShadowEmpty::ExecuteCommandBuffer( uint8 *pBuf )
{

}

void CShaderShadowEmpty::EnableAlphaToCoverage( bool bEnable )
{

}

void CShaderShadowEmpty::SetShadowDepthFiltering( Sampler_t stage )
{

}

void CShaderShadowEmpty::BlendOp( ShaderBlendOp_t blendOp )
{

}

void CShaderShadowEmpty::BlendOpSeparateAlpha( ShaderBlendOp_t blendOp )
{

}

float CShaderShadowEmpty::GetLightMapScaleFactor() const
{
	return 1.0f;
}

//-----------------------------------------------------------------------------
// Empty implementation of shader API
//-----------------------------------------------------------------------------
CShaderAPIEmpty::CShaderAPIEmpty() : m_Mesh(false)
{

}

CShaderAPIEmpty::~CShaderAPIEmpty()
{

}

bool CShaderAPIEmpty::IsDebugTextureListFresh( int numFramesAllowed )
{
	return false;
}

bool CShaderAPIEmpty::SetDebugTextureRendering( bool bEnable )
{
	return false;
}

void CShaderAPIEmpty::EnableDebugTextureList( bool bEnable )
{

}

void CShaderAPIEmpty::EnableGetAllTextures( bool bEnable )
{

}

KeyValues *CShaderAPIEmpty::LockDebugTextureList()
{
	return NULL;
}

void CShaderAPIEmpty::UnlockDebugTextureList()
{

}

KeyValues *CShaderAPIEmpty::GetDebugTextureList()
{
	return NULL;
}

int CShaderAPIEmpty::GetTextureMemoryUsed( TextureMemoryType eTextureMemory )
{
	return 0;
}

void CShaderAPIEmpty::GetBackBufferDimensions( int &width, int &height ) const
{
	width = 1024;
	height = 768;
}

const AspectRatioInfo_t &CShaderAPIEmpty::GetAspectRatioInfo() const
{
	static const AspectRatioInfo_t dummy = { false, false, (4.0f / 3.0f), (4.0f / 3.0f), 1.0f, 1.0f, false };
	return dummy;
}

void CShaderAPIEmpty::GetCurrentRenderTargetDimensions( int &nWidth, int &nHeight ) const
{
	nWidth = 1024;
	nHeight = 768;
}

void CShaderAPIEmpty::GetCurrentViewport( int &nX, int &nY, int &nWidth, int &nHeight ) const
{
	nX = 0;
	nY = 0;
	nWidth = 1024;
	nHeight = 768;
}

void CShaderAPIEmpty::GetCurrentColorCorrection( ShaderColorCorrectionInfo_t *pInfo )
{
	pInfo->m_bIsEnabled = false;
	pInfo->m_nLookupCount = 0;
	pInfo->m_flDefaultWeight = 0.0f;
}

void CShaderAPIEmpty::SetViewports( int nCount, const ShaderViewport_t *pViewports, bool setImmediately )
{

}

int CShaderAPIEmpty::GetViewports( ShaderViewport_t *pViewports, int nMax ) const
{
	return 1;
}

void CShaderAPIEmpty::ClearBuffers( bool bClearColor, bool bClearDepth, bool bClearStencil, int renderTargetWidth, int renderTargetHeight )
{

}

void CShaderAPIEmpty::ClearBuffersEx( bool bClearRed, bool bClearGreen, bool bClearBlue, bool bClearAlpha, unsigned char r, unsigned char b, unsigned char g, unsigned char a )
{

}

void CShaderAPIEmpty::ClearColor3ub( unsigned char r, unsigned char g, unsigned char b )
{

}

void CShaderAPIEmpty::ClearColor4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a )
{

}

void CShaderAPIEmpty::BindVertexShader( VertexShaderHandle_t hVertexShader )
{

}

void CShaderAPIEmpty::BindGeometryShader( GeometryShaderHandle_t hGeometryShader )
{

}

void CShaderAPIEmpty::BindPixelShader( PixelShaderHandle_t hPixelShader )
{

}

void CShaderAPIEmpty::SetRasterState( const ShaderRasterState_t &state )
{

}

void CShaderAPIEmpty::MarkUnusedVertexFields( unsigned int nFlags, int nTexCoordCount, bool *pUnusedTexCoords )
{

}

bool CShaderAPIEmpty::OwnGPUResources( bool bEnable )
{
	return false;
}

void CShaderAPIEmpty::OnPresent()
{

}

bool CShaderAPIEmpty::DoRenderTargetsNeedSeparateDepthBuffer() const
{
	return false;
}

void CShaderAPIEmpty::ClearSnapshots()
{

}

bool CShaderAPIEmpty::SetMode( void *hwnd, int nAdapter, const ShaderDeviceInfo_t &info )
{
	return true;
}

void CShaderAPIEmpty::ChangeVideoMode( const ShaderDeviceInfo_t &info )
{

}

void CShaderAPIEmpty::DXSupportLevelChanged( int nDXLevel )
{

}

void CShaderAPIEmpty::EnableUserClipTransformOverride( bool bEnable )
{

}

void CShaderAPIEmpty::UserClipTransform( const VMatrix &worldToView )
{

}

void CShaderAPIEmpty::SetDefaultState()
{

}

StateSnapshot_t CShaderAPIEmpty::TakeSnapshot()
{
	StateSnapshot_t id = 0;
	if (g_ShaderShadow.m_IsTranslucent)
		id |= TRANSLUCENT;
	if (g_ShaderShadow.m_IsAlphaTested)
		id |= ALPHATESTED;
	if (g_ShaderShadow.m_bUsesVertexAndPixelShaders)
		id |= VERTEX_AND_PIXEL_SHADERS;
	if (g_ShaderShadow.m_bIsDepthWriteEnabled)
		id |= DEPTH_WRITE_ENABLED;
	return id;
}

bool CShaderAPIEmpty::IsTranslucent( StateSnapshot_t id ) const
{
	return (id & TRANSLUCENT) != 0;
}

bool CShaderAPIEmpty::IsAlphaTested( StateSnapshot_t id ) const
{
	return (id & ALPHATESTED) != 0;
}

bool CShaderAPIEmpty::UsesVertexAndPixelShaders( StateSnapshot_t id ) const
{
	return (id & VERTEX_AND_PIXEL_SHADERS) != 0;
}

bool CShaderAPIEmpty::IsDepthWriteEnabled( StateSnapshot_t id ) const
{
	return (id & DEPTH_WRITE_ENABLED) != 0;
}

VertexFormat_t CShaderAPIEmpty::ComputeVertexFormat( int numSnapshots, StateSnapshot_t *pIds ) const
{
	return 0;
}

VertexFormat_t CShaderAPIEmpty::ComputeVertexUsage( int numSnapshots, StateSnapshot_t *pIds ) const
{
	return 0;
}

void CShaderAPIEmpty::BeginPass( StateSnapshot_t snapshot )
{

}

void CShaderAPIEmpty::UseSnapshot( StateSnapshot_t snapshot )
{

}

CMeshBuilder *CShaderAPIEmpty::GetVertexModifyBuilder()
{
	return NULL;
}

void CShaderAPIEmpty::SetLightingState( const MaterialLightingState_t &state )
{

}

void CShaderAPIEmpty::SetLights( int nCount, const LightDesc_t *pDesc )
{

}

void CShaderAPIEmpty::SetLightingOrigin( Vector vLightingOrigin )
{

}

void CShaderAPIEmpty::SetAmbientLightCube( Vector4D cube[6] )
{

}

void CShaderAPIEmpty::CopyRenderTargetToTexture( ShaderAPITextureHandle_t textureHandle )
{

}

void CShaderAPIEmpty::CopyRenderTargetToTextureEx( ShaderAPITextureHandle_t textureHandle, int nRenderTargetID, Rect_t *pSrcRect, Rect_t *pDstRect )
{

}

void CShaderAPIEmpty::CopyTextureToRenderTargetEx( int nRenderTargetID, ShaderAPITextureHandle_t textureHandle, Rect_t *pSrcRect, Rect_t *pDstRect )
{

}

void CShaderAPIEmpty::SetNumBoneWeights( int numBones )
{

}

void CShaderAPIEmpty::EnableHWMorphing( bool bEnable )
{

}

IMesh *CShaderAPIEmpty::GetDynamicMesh( IMaterial *pMaterial, int nHWSkinBoneCount, bool bBuffered, IMesh *pVertexOverride, IMesh *pIndexOverride )
{
	return &m_Mesh;
}

IMesh *CShaderAPIEmpty::GetDynamicMeshEx( IMaterial *pMaterial, VertexFormat_t vertexFormat, int nHWSkinBoneCount, bool bBuffered, IMesh *pVertexOverride, IMesh *pIndexOverride )
{
	return &m_Mesh;
}

IMesh *CShaderAPIEmpty::GetFlexMesh()
{
	return &m_Mesh;
}

void CShaderAPIEmpty::RenderPass( const unsigned char *pInstanceCommandBuffer, int nPass, int nPassCount )
{

}

void CShaderAPIEmpty::MatrixMode( MaterialMatrixMode_t matrixMode )
{

}

void CShaderAPIEmpty::PushMatrix()
{

}

void CShaderAPIEmpty::PopMatrix()
{

}

void CShaderAPIEmpty::LoadMatrix( float *m )
{

}

void CShaderAPIEmpty::LoadBoneMatrix( int boneIndex, const float *m )
{

}

void CShaderAPIEmpty::MultMatrix( float *m )
{

}

void CShaderAPIEmpty::MultMatrixLocal( float *m )
{

}

void CShaderAPIEmpty::GetActualProjectionMatrix( float *m )
{

}

void CShaderAPIEmpty::GetMatrix( MaterialMatrixMode_t matrixMode, float *dst )
{

}

void CShaderAPIEmpty::LoadIdentity()
{

}

void CShaderAPIEmpty::LoadCameraToWorld()
{

}

void CShaderAPIEmpty::Ortho( double left, double right, double bottom, double top, double zNear, double zFar )
{

}

void CShaderAPIEmpty::PerspectiveX( double fovx, double aspect, double zNear, double zFar )
{

}

void CShaderAPIEmpty::PerspectiveOffCenterX( double fovx, double aspect, double zNear, double zFar, double bottom, double top, double left, double right )
{

}

void CShaderAPIEmpty::PickMatrix( int x, int y, int width, int height )
{

}

void CShaderAPIEmpty::Rotate( float angle, float x, float y, float z )
{

}

void CShaderAPIEmpty::Translate( float x, float y, float z )
{

}

void CShaderAPIEmpty::Scale( float x, float y, float z )
{

}

void CShaderAPIEmpty::ScaleXY( float x, float y )
{

}

void CShaderAPIEmpty::FogMode( MaterialFogMode_t fogMode )
{

}

void CShaderAPIEmpty::FogStart( float fStart )
{

}

void CShaderAPIEmpty::FogEnd( float fEnd )
{

}

void CShaderAPIEmpty::SetFogZ( float fogZ )
{

}

void CShaderAPIEmpty::FogMaxDensity( float flMaxDensity )
{

}

void CShaderAPIEmpty::GetFogDistances( float *fStart, float *fEnd, float *fFogZ )
{

}

void CShaderAPIEmpty::FogColor3f( float r, float g, float b )
{

}

void CShaderAPIEmpty::FogColor3fv( float const *rgb )
{

}

void CShaderAPIEmpty::FogColor3ub( unsigned char r, unsigned char g, unsigned char b )
{

}

void CShaderAPIEmpty::FogColor3ubv( unsigned char const *rgb )
{

}

void CShaderAPIEmpty::SceneFogColor3ub( unsigned char r, unsigned char g, unsigned char b )
{

}

void CShaderAPIEmpty::SceneFogMode( MaterialFogMode_t fogMode )
{

}

void CShaderAPIEmpty::GetSceneFogColor( unsigned char *rgb )
{

}

MaterialFogMode_t CShaderAPIEmpty::GetSceneFogMode()
{
	return MATERIAL_FOG_NONE;
}

int CShaderAPIEmpty::GetPixelFogCombo()
{
	return 0;
}

void CShaderAPIEmpty::SetHeightClipZ( float z )
{

}

void CShaderAPIEmpty::SetHeightClipMode( enum MaterialHeightClipMode_t heightClipMode )
{

}

void CShaderAPIEmpty::SetClipPlane( int index, const float *pPlane )
{

}

void CShaderAPIEmpty::EnableClipPlane( int index, bool bEnable )
{

}

void CShaderAPIEmpty::SetFastClipPlane( const float *pPlane )
{

}

void CShaderAPIEmpty::EnableFastClip( bool bEnable )
{

}

int CShaderAPIEmpty::GetCurrentDynamicVBSize()
{
	return 0;
}

int CShaderAPIEmpty::GetCurrentDynamicVBSize( int nIndex )
{
	return 0;
}

void CShaderAPIEmpty::DestroyVertexBuffers( bool bExitingLevel )
{

}

void CShaderAPIEmpty::GetGPUMemoryStats( GPUMemoryStats &stats )
{

}

void CShaderAPIEmpty::SetVertexShaderIndex( int vshIndex )
{

}

void CShaderAPIEmpty::SetPixelShaderIndex( int pshIndex )
{

}

void CShaderAPIEmpty::SetVertexShaderConstant( int var, float const *pVec, int numConst, bool bForce )
{

}

void CShaderAPIEmpty::SetBooleanVertexShaderConstant( int var, BOOL const *pVec, int numBools, bool bForce )
{

}

void CShaderAPIEmpty::SetIntegerVertexShaderConstant( int var, int const *pVec, int numIntVecs, bool bForce )
{

}

void CShaderAPIEmpty::SetPixelShaderConstant( int var, float const *pVec, int numConst, bool bForce )
{

}

void CShaderAPIEmpty::SetBooleanPixelShaderConstant( int var, BOOL const *pVec, int numBools, bool bForce )
{

}

void CShaderAPIEmpty::SetIntegerPixelShaderConstant( int var, int const *pVec, int numIntVecs, bool bForce )
{

}

void CShaderAPIEmpty::InvalidateDelayedShaderConstants()
{

}

float CShaderAPIEmpty::GammaToLinear_HardwareSpecific( float fGamma ) const
{
	return 0.0f;
}

float CShaderAPIEmpty::LinearToGamma_HardwareSpecific( float fLinear ) const
{
	return 0.0f;
}

void CShaderAPIEmpty::SetLinearToGammaConversionTextures( ShaderAPITextureHandle_t hSRGBWriteEnabledTexture, ShaderAPITextureHandle_t hIdentityTexture )
{

}

void CShaderAPIEmpty::CullMode( MaterialCullMode_t cullMode )
{

}

void CShaderAPIEmpty::FlipCullMode()
{

}

void CShaderAPIEmpty::ForceDepthFuncEquals( bool bEnable )
{

}

void CShaderAPIEmpty::OverrideDepthEnable( bool bEnable, bool bDepthEnable, bool bDepthTestEnable )
{

}

void CShaderAPIEmpty::OverrideAlphaWriteEnable( bool bOverrideEnable, bool bAlphaWriteEnable )
{

}

void CShaderAPIEmpty::OverrideColorWriteEnable( bool bOverrideEnable, bool bColorWriteEnable )
{

}

void CShaderAPIEmpty::ShadeMode( ShaderShadeMode_t mode )
{

}

void CShaderAPIEmpty::Bind( IMaterial *pMaterial )
{

}

ImageFormat CShaderAPIEmpty::GetNearestSupportedFormat( ImageFormat fmt, bool bFilteringRequired ) const
{
	return fmt;
}

ImageFormat CShaderAPIEmpty::GetNearestRenderTargetFormat( ImageFormat fmt ) const
{
	return fmt;
}

void CShaderAPIEmpty::BindTexture( Sampler_t stage, TextureBindFlags_t nBindFlags, ShaderAPITextureHandle_t textureHandle )
{

}

void CShaderAPIEmpty::BindVertexTexture( VertexTextureSampler_t nSampler, ShaderAPITextureHandle_t textureHandle )
{

}

void CShaderAPIEmpty::SetRenderTarget( ShaderAPITextureHandle_t colorTextureHandle, ShaderAPITextureHandle_t depthTextureHandle )
{

}

void CShaderAPIEmpty::SetRenderTargetEx( int nRenderTargetID, ShaderAPITextureHandle_t colorTextureHandle, ShaderAPITextureHandle_t depthTextureHandle )
{

}

void CShaderAPIEmpty::ModifyTexture( ShaderAPITextureHandle_t textureHandle )
{

}

void CShaderAPIEmpty::TexImage2D( int level, int cubeFaceID, ImageFormat dstFormat, int zOffset, int width, int height, ImageFormat srcFormat, bool bSrcIsTiled, void *imageData )
{

}

void CShaderAPIEmpty::TexSubImage2D( int level, int cubeFaceID, int xOffset, int yOffset, int zOffset, int width, int height, ImageFormat srcFormat, int srcStride, bool bSrcIsTiled, void *imageData )
{

}

bool CShaderAPIEmpty::TexLock( int level, int cubeFaceID, int xOffset, int yOffset, int width, int height, CPixelWriter &writer )
{
	return false;
}

void CShaderAPIEmpty::TexUnlock()
{

}

void CShaderAPIEmpty::UpdateTexture( int xOffset, int yOffset, int w, int h, ShaderAPITextureHandle_t hDstTexture, ShaderAPITextureHandle_t hSrcTexture )
{

}

void *CShaderAPIEmpty::LockTex( ShaderAPITextureHandle_t hTexture )
{
	return NULL;
}

void CShaderAPIEmpty::UnlockTex( ShaderAPITextureHandle_t hTexture )
{

}

void *CShaderAPIEmpty::GetD3DTexturePtr( ShaderAPITextureHandle_t hTexture )
{
	return NULL;
}

void CShaderAPIEmpty::TexMinFilter( ShaderTexFilterMode_t texFilterMode )
{

}

void CShaderAPIEmpty::TexMagFilter( ShaderTexFilterMode_t texFilterMode )
{

}

void CShaderAPIEmpty::TexWrap( ShaderTexCoordComponent_t coord, ShaderTexWrapMode_t wrapMode )
{

}

void CShaderAPIEmpty::TexSetPriority( int priority )
{

}

ShaderAPITextureHandle_t CShaderAPIEmpty::CreateTexture( int width, int height, int depth, ImageFormat dstImageFormat, int numMipLevels, int numCopies, int flags, const char *pDebugName, const char *pTextureGroupName )
{
	return INVALID_SHADERAPI_TEXTURE_HANDLE;
}

void CShaderAPIEmpty::CreateTextures( ShaderAPITextureHandle_t *pHandles, int count, int width, int height, int depth, ImageFormat dstImageFormat, int numMipLevels, int numCopies, int flags, const char *pDebugName, const char *pTextureGroupName )
{
	if (count > 0)
	{
		memset(pHandles, 0, sizeof(ShaderAPITextureHandle_t) * count);
	}
}

ShaderAPITextureHandle_t CShaderAPIEmpty::CreateDepthTexture( ImageFormat renderTargetFormat, int width, int height, const char *pDebugName, bool bTexture, bool bAliasDepthTextureOverSceneDepthX360 )
{
	return INVALID_SHADERAPI_TEXTURE_HANDLE;
}

void CShaderAPIEmpty::DeleteTexture( ShaderAPITextureHandle_t textureHandle )
{

}

bool CShaderAPIEmpty::IsTexture( ShaderAPITextureHandle_t textureHandle )
{
	return true;
}

bool CShaderAPIEmpty::IsTextureResident( ShaderAPITextureHandle_t textureHandle )
{
	return false;
}

void CShaderAPIEmpty::ClearBuffersObeyStencil( bool bClearColor, bool bClearDepth )
{

}

void CShaderAPIEmpty::ClearBuffersObeyStencilEx( bool bClearColor, bool bClearAlpha, bool bClearDepth )
{

}

void CShaderAPIEmpty::PerformFullScreenStencilOperation()
{

}

void CShaderAPIEmpty::ReadPixels( int x, int y, int width, int height, unsigned char *data, ImageFormat dstFormat, ITexture *pTexture )
{

}

void CShaderAPIEmpty::ReadPixelsAsync( int x, int y, int width, int height, unsigned char *data, ImageFormat dstFormat, ITexture *pTexture, CThreadEvent *pEvent )
{

}

void CShaderAPIEmpty::ReadPixelsAsyncGetResult( int x, int y, int width, int height, unsigned char *data, ImageFormat dstFormat, CThreadEvent *pEvent )
{

}

void CShaderAPIEmpty::ReadPixels( Rect_t *pSrcRect, Rect_t *pDstRect, unsigned char *data, ImageFormat dstFormat, int nDstStride )
{

}

int CShaderAPIEmpty::SelectionMode( bool selectionMode )
{
	return 0;
}

void CShaderAPIEmpty::SelectionBuffer( unsigned int *pBuffer, int size )
{

}

void CShaderAPIEmpty::ClearSelectionNames()
{

}

void CShaderAPIEmpty::LoadSelectionName( int name )
{

}

void CShaderAPIEmpty::PushSelectionName( int name )
{

}

void CShaderAPIEmpty::PopSelectionName()
{

}

void CShaderAPIEmpty::FlushHardware()
{
	
}

void CShaderAPIEmpty::ResetRenderState( bool bFullReset )
{

}

void CShaderAPIEmpty::SetScissorRect( int nLeft, int nTop, int nRight, int nBottom, bool bEnableScissor )
{

}

bool CShaderAPIEmpty::CanDownloadTextures() const
{
	return false;
}

void CShaderAPIEmpty::BeginFrame()
{

}

void CShaderAPIEmpty::EndFrame()
{

}

double CShaderAPIEmpty::CurrentTime() const
{
	return Plat_FloatTime();
}

void CShaderAPIEmpty::GetWorldSpaceCameraPosition( float *pPos ) const
{

}

void CShaderAPIEmpty::GetWorldSpaceCameraDirection( float *pDir ) const
{

}

bool CShaderAPIEmpty::HasDestAlphaBuffer() const
{
	return false;
}

bool CShaderAPIEmpty::HasStencilBuffer() const
{
	return false;
}

int CShaderAPIEmpty::MaxViewports() const
{
	return 1;
}

void CShaderAPIEmpty::OverrideStreamOffsetSupport( bool bOverrideEnabled, bool bEnableSupport )
{

}

ShadowFilterMode_t CShaderAPIEmpty::GetShadowFilterMode( bool bForceLowQualityShadows, bool bPS30 ) const
{
	return SHADOWFILTERMODE_DEFAULT;
}

int CShaderAPIEmpty::StencilBufferBits() const
{
	return 0;
}

int CShaderAPIEmpty::GetFrameBufferColorDepth() const
{
	return 0;
}

int CShaderAPIEmpty::GetSamplerCount() const
{
	return 4;
}

int CShaderAPIEmpty::GetVertexSamplerCount() const
{
	return 0;
}

bool CShaderAPIEmpty::HasSetDeviceGammaRamp() const
{
	return false;
}

bool CShaderAPIEmpty::SupportsCompressedTextures() const
{
	return false;
}

VertexCompressionType_t CShaderAPIEmpty::SupportsCompressedVertices() const
{
	return VERTEX_COMPRESSION_NONE;
}

bool CShaderAPIEmpty::SupportsStaticControlFlow() const
{
	return false;
}

int CShaderAPIEmpty::MaximumAnisotropicLevel() const
{
	return 0;
}

int CShaderAPIEmpty::MaxTextureWidth() const
{
	return 16384;
}

int CShaderAPIEmpty::MaxTextureHeight() const
{
	return 16384;
}

int CShaderAPIEmpty::MaxTextureAspectRatio() const
{
	return 16384;
}

int CShaderAPIEmpty::GetDXSupportLevel() const
{
	return 90;
}

int CShaderAPIEmpty::GetMinDXSupportLevel() const
{
	return 90;
}

bool CShaderAPIEmpty::SupportsShadowDepthTextures() const
{
	return false;
}

bool CShaderAPIEmpty::SupportsShadowDepthTextures()
{
	return false;
}

ImageFormat CShaderAPIEmpty::GetShadowDepthTextureFormat() const
{
	return IMAGE_FORMAT_RGBA8888;
}

ImageFormat CShaderAPIEmpty::GetHighPrecisionShadowDepthTextureFormat() const
{
	return IMAGE_FORMAT_RGBA8888;
}

ImageFormat CShaderAPIEmpty::GetNullTextureFormat() const
{
	return IMAGE_FORMAT_RGBA8888;
}

const char *CShaderAPIEmpty::GetShaderDLLName() const
{
	return "UNKNOWN";
}

int CShaderAPIEmpty::TextureMemorySize() const
{
	return 0x4000000;
}

bool CShaderAPIEmpty::SupportsMipmappedCubemaps() const
{
	return true;
}

int CShaderAPIEmpty::NumVertexShaderConstants() const
{
	return 128;
}

int CShaderAPIEmpty::NumBooleanVertexShaderConstants() const
{
	return 0;
}

int CShaderAPIEmpty::NumIntegerVertexShaderConstants() const
{
	return 0;
}

int CShaderAPIEmpty::NumPixelShaderConstants() const
{
	return 8;
}

int CShaderAPIEmpty::MaxNumLights() const
{
	return 4;
}

int CShaderAPIEmpty::MaxVertexShaderBlendMatrices() const
{
	return 0;
}

int CShaderAPIEmpty::MaxUserClipPlanes() const
{
	return 0;
}

bool CShaderAPIEmpty::UseFastClipping() const
{
	return false;
}

bool CShaderAPIEmpty::SpecifiesFogColorInLinearSpace() const
{
	return false;
}

bool CShaderAPIEmpty::SupportsSRGB() const
{
	return false;
}

bool CShaderAPIEmpty::FakeSRGBWrite() const
{
	return false;
}

bool CShaderAPIEmpty::CanDoSRGBReadFromRTs() const
{
	return true;
}

bool CShaderAPIEmpty::SupportsGLMixedSizeTargets() const
{
	return false;
}

const char *CShaderAPIEmpty::GetHWSpecificShaderDLLName() const
{
	return NULL;
}

bool CShaderAPIEmpty::NeedsAAClamp() const
{
	return false;
}

int CShaderAPIEmpty::MaxHWMorphBatchCount() const
{
	return 0;
}

int CShaderAPIEmpty::GetMaxDXSupportLevel() const
{
	return 90;
}

bool CShaderAPIEmpty::ReadPixelsFromFrontBuffer() const
{
	return true;
}

bool CShaderAPIEmpty::PreferDynamicTextures() const
{
	return false;
}

void CShaderAPIEmpty::ForceHardwareSync()
{

}

int CShaderAPIEmpty::GetCurrentNumBones() const
{
	return 0;
}

bool CShaderAPIEmpty::IsHWMorphingEnabled() const
{
	return false;
}

void CShaderAPIEmpty::GetDX9LightState( LightState_t *state ) const
{
	state->m_nNumLights = 0;
	state->m_bAmbientLight = false;
	state->m_bStaticLight = false;
}

MaterialFogMode_t CShaderAPIEmpty::GetCurrentFogType() const
{
	return MATERIAL_FOG_NONE;
}

void CShaderAPIEmpty::RecordString( const char *pStr )
{

}

void CShaderAPIEmpty::EvictManagedResources()
{

}

void CShaderAPIEmpty::GetLightmapDimensions( int *w, int *h )
{
	g_pShaderUtil->GetLightmapDimensions(w, h);
}

void CShaderAPIEmpty::SyncToken( const char *pToken )
{

}

void CShaderAPIEmpty::SetStandardVertexShaderConstants( float fOverbright )
{

}

void CShaderAPIEmpty::SetAnisotropicLevel( int nAnisotropyLevel )
{

}

bool CShaderAPIEmpty::SupportsHDR() const
{
	return false;
}

HDRType_t CShaderAPIEmpty::GetHDRType() const
{
	return HDR_TYPE_NONE;
}

HDRType_t CShaderAPIEmpty::GetHardwareHDRType() const
{
	return HDR_TYPE_NONE;
}

bool CShaderAPIEmpty::NeedsATICentroidHack() const
{
	return false;
}

bool CShaderAPIEmpty::SupportsColorOnSecondStream() const
{
	return false;
}

bool CShaderAPIEmpty::SupportsStaticPlusDynamicLighting() const
{
	return false;
}

bool CShaderAPIEmpty::SupportsStreamOffset() const
{
	return false;
}

void CShaderAPIEmpty::CommitPixelShaderLighting( int pshReg )
{

}

ShaderAPIOcclusionQuery_t CShaderAPIEmpty::CreateOcclusionQueryObject()
{
	return INVALID_SHADERAPI_OCCLUSION_QUERY_HANDLE;
}

void CShaderAPIEmpty::DestroyOcclusionQueryObject( ShaderAPIOcclusionQuery_t )
{

}

void CShaderAPIEmpty::BeginOcclusionQueryDrawing( ShaderAPIOcclusionQuery_t )
{

}

void CShaderAPIEmpty::EndOcclusionQueryDrawing( ShaderAPIOcclusionQuery_t )
{

}

int CShaderAPIEmpty::OcclusionQuery_GetNumPixelsRendered( ShaderAPIOcclusionQuery_t hQuery, bool bFlush )
{
	return 0;
}

void CShaderAPIEmpty::AcquireThreadOwnership()
{

}

void CShaderAPIEmpty::ReleaseThreadOwnership()
{

}

bool CShaderAPIEmpty::SupportsBorderColor() const
{
	return false;
}

bool CShaderAPIEmpty::SupportsFetch4() const
{
	return false;
}

bool CShaderAPIEmpty::SupportsFetch4()
{
	return false;
}

void CShaderAPIEmpty::EnableBuffer2FramesAhead( bool bEnable )
{

}

float CShaderAPIEmpty::GetShadowDepthBias() const
{
	return 0.0f;
}

float CShaderAPIEmpty::GetShadowSlopeScaleDepthBias() const
{
	return 0.0f;
}

bool CShaderAPIEmpty::PreferZPrepass() const
{
	return false;
}

bool CShaderAPIEmpty::SuppressPixelShaderCentroidHackFixup() const
{
	return true;
}

bool CShaderAPIEmpty::PreferTexturesInHWMemory() const
{
	return false;
}

bool CShaderAPIEmpty::PreferHardwareSync() const
{
	return false;
}

bool CShaderAPIEmpty::IsUnsupported() const
{
	return false;
}

void CShaderAPIEmpty::SetDepthFeatheringPixelShaderConstant( int iConstant, float fDepthBlendScale )
{

}

TessellationMode_t CShaderAPIEmpty::GetTessellationMode() const
{
	return TESSELLATION_MODE_DISABLED;
}

void CShaderAPIEmpty::SetPixelShaderFogParams( int reg )
{

}

bool CShaderAPIEmpty::InFlashlightMode() const
{
	return false;
}

bool CShaderAPIEmpty::IsRenderingPaint() const
{
	return false;
}

bool CShaderAPIEmpty::InEditorMode() const
{
	return false;
}

void CShaderAPIEmpty::BindStandardTexture( Sampler_t stage, TextureBindFlags_t nBindFlags, StandardTextureId_t id )
{

}

void CShaderAPIEmpty::BindStandardVertexTexture( VertexTextureSampler_t sampler, StandardTextureId_t id )
{

}

void CShaderAPIEmpty::GetStandardTextureDimensions( int *pWidth, int *pHeight, StandardTextureId_t id )
{
	*pWidth = 0;
	*pHeight = 0;
}

float CShaderAPIEmpty::GetSubDHeight()
{
	return 0.0f;
}

bool CShaderAPIEmpty::IsStereoActiveThisFrame() const
{
	return false;
}

void CShaderAPIEmpty::SetFlashlightState( const FlashlightState_t &state, const VMatrix &worldToTexture )
{

}

void CShaderAPIEmpty::SetFlashlightStateEx( const FlashlightState_t &state, const VMatrix &worldToTexture, ITexture *pFlashlightDepthTexture )
{

}

bool CShaderAPIEmpty::IsCascadedShadowMapping() const
{
	return false;
}

void CShaderAPIEmpty::SetCascadedShadowMapping( bool bEnable )
{

}

void CShaderAPIEmpty::SetCascadedShadowMappingState( const CascadedShadowMappingState_t &state, ITexture *pDepthTextureAtlas )
{

}

const CascadedShadowMappingState_t &CShaderAPIEmpty::GetCascadedShadowMappingState( ITexture **pDepthTextureAtlas ) const
{
	static CascadedShadowMappingState_t dummy;
	return dummy;
}

const FlashlightState_t &CShaderAPIEmpty::GetFlashlightState( VMatrix &worldToTexture ) const
{
	static const FlashlightState_t blah;
	return blah;
}

const FlashlightState_t &CShaderAPIEmpty::GetFlashlightStateEx( VMatrix &worldToTexture, ITexture **pFlashlightDepthTexture ) const
{
	static const FlashlightState_t blah;
	return blah;
}

void CShaderAPIEmpty::GetFlashlightShaderInfo( bool *pShadowsEnabled, bool *pUberLight ) const
{
	*pShadowsEnabled = false;
	*pUberLight = false;
}

float CShaderAPIEmpty::GetFlashlightAmbientOcclusion() const
{
	return 1.0f;
}

void CShaderAPIEmpty::ClearVertexAndPixelShaderRefCounts()
{

}

void CShaderAPIEmpty::PurgeUnusedVertexAndPixelShaders()
{

}

void CShaderAPIEmpty::DrawInstances( int nInstanceCount, const MeshInstanceData_t *pInstance )
{

}

bool CShaderAPIEmpty::IsAAEnabled() const
{
	return false;
}

int CShaderAPIEmpty::GetVertexTextureCount() const
{
	return 0;
}

int CShaderAPIEmpty::GetMaxVertexTextureDimension() const
{
	return 0;
}

int CShaderAPIEmpty::MaxTextureDepth() const
{
	return 0;
}

void CShaderAPIEmpty::SetFlexWeights( int nFirstWeight, int nCount, const MorphWeight_t *pWeights )
{

}

ITexture *CShaderAPIEmpty::GetRenderTargetEx( int nRenderTargetID ) const
{
	return NULL;
}

void CShaderAPIEmpty::SetToneMappingScaleLinear( const Vector &scale )
{

}

const Vector &CShaderAPIEmpty::GetToneMappingScaleLinear( void ) const
{
	static const Vector dummy;
	return dummy;
}

void CShaderAPIEmpty::HandleDeviceLost()
{

}

void CShaderAPIEmpty::EnableLinearColorSpaceFrameBuffer( bool bEnable )
{

}

void CShaderAPIEmpty::SetFullScreenTextureHandle( ShaderAPITextureHandle_t h )
{

}

void CShaderAPIEmpty::SetFloatRenderingParameter( int parm_number, float value )
{

}

void CShaderAPIEmpty::SetIntRenderingParameter( int parm_number, int value )
{

}

void CShaderAPIEmpty::SetTextureRenderingParameter( int parm_number, ITexture *pTexture )
{

}

void CShaderAPIEmpty::SetVectorRenderingParameter( int parm_number, Vector const &value )
{

}

float CShaderAPIEmpty::GetFloatRenderingParameter( int parm_number ) const
{
	return 0.0f;
}

int CShaderAPIEmpty::GetIntRenderingParameter( int parm_number ) const
{
	return 0;
}

ITexture *CShaderAPIEmpty::GetTextureRenderingParameter( int parm_number ) const
{
	return NULL;
}

Vector CShaderAPIEmpty::GetVectorRenderingParameter( int parm_number ) const
{
	return Vector(0.0f, 0.0f, 0.0f);
}

void CShaderAPIEmpty::SetStencilState( const ShaderStencilState_t &state )
{

}

void CShaderAPIEmpty::ClearStencilBufferRectangle( int xmin, int ymin, int xmax, int ymax, int value )
{

}

void CShaderAPIEmpty::GetMaxToRender( IMesh *pMesh, bool bMaxUntilFlush, int *pMaxVerts, int *pMaxIndices )
{
	*pMaxVerts = 32768;
	*pMaxIndices = 32768;
}

int CShaderAPIEmpty::GetMaxVerticesToRender( IMaterial *pMaterial )
{
	return 32768;
}

int CShaderAPIEmpty::GetMaxIndicesToRender()
{
	return 32768;
}

int CShaderAPIEmpty::CompareSnapshots( StateSnapshot_t snapshot0, StateSnapshot_t snapshot1 )
{
	return 0;
}

void CShaderAPIEmpty::DisableAllLocalLights()
{

}

bool CShaderAPIEmpty::SupportsMSAAMode( int nMSAAMode )
{
	return false;
}

bool CShaderAPIEmpty::SupportsCSAAMode( int nNumSamples, int nQualityLevel )
{
	return false;
}

void CShaderAPIEmpty::BeginPIXEvent( unsigned long color, const char *szName )
{

}

void CShaderAPIEmpty::EndPIXEvent()
{

}

void CShaderAPIEmpty::SetPIXMarker( unsigned long color, const char *szName )
{

}

void CShaderAPIEmpty::ComputeVertexDescription( unsigned char *pBuffer, VertexFormat_t vertexFormat, MeshDesc_t &desc ) const
{

}

int CShaderAPIEmpty::VertexFormatSize( VertexFormat_t vertexFormat ) const
{
	return 0;
}

int CShaderAPIEmpty::NeedsShaderSRGBConversion() const
{
	return 0;
}

bool CShaderAPIEmpty::UsesSRGBCorrectBlending() const
{
	return false;
}

bool CShaderAPIEmpty::HasFastVertexTextures() const
{
	return false;
}

bool CShaderAPIEmpty::ActualHasFastVertexTextures() const
{
	return false;
}

void CShaderAPIEmpty::SetShadowDepthBiasFactors( float fShadowSlopeScaleDepthBias, float fShadowDepthBias )
{

}

void CShaderAPIEmpty::SetDisallowAccess( bool )
{

}

void CShaderAPIEmpty::EnableShaderShaderMutex( bool )
{

}

void CShaderAPIEmpty::ShaderLock()
{

}

void CShaderAPIEmpty::ShaderUnlock()
{

}

void CShaderAPIEmpty::BindVertexBuffer( int nStreamID, IVertexBuffer *pVertexBuffer, int nOffsetInBytes, int nFirstVertex, int nVertexCount, VertexFormat_t fmt, int nRepetitions )
{

}

void CShaderAPIEmpty::BindIndexBuffer( IIndexBuffer *pIndexBuffer, int nOffsetInBytes )
{

}

void CShaderAPIEmpty::Draw( MaterialPrimitiveType_t primitiveType, int nFirstIndex, int nIndexCount )
{

}

int CShaderAPIEmpty::GetVertexBufferCompression() const
{
	return 0;
}

bool CShaderAPIEmpty::ShouldWriteDepthToDestAlpha() const
{
	return false;
}

bool CShaderAPIEmpty::SupportsHDRMode( HDRType_t nHDRMode ) const
{
	return false;
}

bool CShaderAPIEmpty::IsDX10Card() const
{
	return false;
}

void CShaderAPIEmpty::PushDeformation( const DeformationBase_t *Deformation )
{

}

void CShaderAPIEmpty::PopDeformation()
{

}

int CShaderAPIEmpty::GetNumActiveDeformations() const
{
	return 0;
}

int CShaderAPIEmpty::GetPackedDeformationInformation( int nMaskOfUnderstoodDeformations, float *pConstantValuesOut, int nBufferSize, int nMaximumDeformations, int *pNumDefsOut ) const
{
	*pNumDefsOut = 0;
	return 0;
}

void CShaderAPIEmpty::SetStandardTextureHandle( StandardTextureId_t nId, ShaderAPITextureHandle_t nHandle )
{

}

void CShaderAPIEmpty::ExecuteCommandBuffer( uint8 *pCmdBuffer )
{

}

bool CShaderAPIEmpty::GetHDREnabled() const
{
	return true;
}

void CShaderAPIEmpty::SetHDREnabled( bool bEnable )
{

}

void CShaderAPIEmpty::SetTextureFilterMode( Sampler_t sampler, TextureFilterMode_t nMode )
{

}

void CShaderAPIEmpty::SetScreenSizeForVPOS( int pshReg )
{

}

void CShaderAPIEmpty::SetVSNearAndFarZ( int vshReg )
{

}

float CShaderAPIEmpty::GetFarZ()
{
	return 1000.0f;
}

void CShaderAPIEmpty::EnableSinglePassFlashlightMode( bool bEnable )
{

}

bool CShaderAPIEmpty::SinglePassFlashlightModeEnabled()
{
	return false;
}

void CShaderAPIEmpty::FlipCulling( bool bFlipCulling )
{

}

void CShaderAPIEmpty::UpdateGameTime( float flTime )
{

}

bool CShaderAPIEmpty::IsStereoSupported( void ) const
{
	return false;
}

void CShaderAPIEmpty::UpdateStereoTexture( ShaderAPITextureHandle_t texHandle, bool *pStereoActiveThisFrame )
{

}

void CShaderAPIEmpty::SetSRGBWrite( bool bState )
{

}

void CShaderAPIEmpty::PrintfVA( char *fmt, va_list vargs )
{

}

void CShaderAPIEmpty::Printf( char *fmt, ... )
{

}

float CShaderAPIEmpty::Knob( char *knobname, float *setvalue )
{
	return 0.0f;
}

void CShaderAPIEmpty::AddShaderComboInformation( const ShaderComboSemantics_t *pSemantics )
{

}

void CShaderAPIEmpty::SpinPresent( unsigned int nFrames )
{

}

void CShaderAPIEmpty::AntiAliasingHint( int a1 )
{

}

bool CShaderAPIEmpty::SupportsCascadedShadowMapping() const
{
	return false;
}

CSMQualityMode_t CShaderAPIEmpty::GetCSMQuality() const
{
	return CSMQUALITY_VERY_LOW;
}

bool CShaderAPIEmpty::SupportsBilinearPCFSampling() const
{
	return true;
}

CSMShaderMode_t CShaderAPIEmpty::GetCSMShaderMode( CSMQualityMode_t nQualityLevel ) const
{
	return CSMSHADERMODE_LOW_OR_VERY_LOW;
}

void CShaderAPIEmpty::EnableAlphaToCoverage()
{

}

void CShaderAPIEmpty::DisableAlphaToCoverage()
{

}

float CShaderAPIEmpty::GetLightMapScaleFactor( void ) const
{
	return 1.0f;
}

ShaderAPITextureHandle_t CShaderAPIEmpty::FindTexture( const char *pDebugName )
{
	return INVALID_SHADERAPI_TEXTURE_HANDLE;
}

void CShaderAPIEmpty::GetTextureDimensions( ShaderAPITextureHandle_t hTexture, int &nWidth, int &nHeight, int &nDepth )
{

}

void CShaderAPIEmpty::SetVertexShaderViewProj()
{

}

void CShaderAPIEmpty::UpdateVertexShaderMatrix( int )
{

}

void CShaderAPIEmpty::SetVertexShaderModelViewProjAndModelView()
{

}

void CShaderAPIEmpty::SetVertexShaderCameraPos()
{

}

int CShaderAPIEmpty::SetSkinningMatrices( const MeshInstanceData_t & )
{
	return 0;
}

void CShaderAPIEmpty::BeginGeneratingCSMs()
{

}

void CShaderAPIEmpty::EndGeneratingCSMs()
{

}

void CShaderAPIEmpty::PerpareForCascadeDraw( int, float, float )
{

}

bool CShaderAPIEmpty::GetCSMAccurateBlending() const
{
	return false;
}

void CShaderAPIEmpty::SetCSMAccurateBlending( bool bEnable )
{

}

bool CShaderAPIEmpty::SupportsResolveDepth() const
{
	return false;
}

bool CShaderAPIEmpty::HasFullResolutionDepthTexture() const
{
	return false;
}

int CShaderAPIEmpty::NumBooleanPixelShaderConstants() const
{
	return 0;
}

int CShaderAPIEmpty::NumIntegerPixelShaderConstants() const
{
	return 0;
}

ShaderAPITextureHandle_t CShaderAPIEmpty::GetStandardTextureHandle(StandardTextureId_t nID)
{
	return 0;
}

bool CShaderAPIEmpty::IsStandardTextureHandleValid( StandardTextureId_t nId )
{
	return false;
}

int CShaderAPIEmpty::Unknown0( int a1, void **a2, int *a3 )
{
	*a2 = nullptr;
	*a3 = 0;
	return 0;
}

void CShaderAPIEmpty::Unknown1()
{

}

void CShaderAPIEmpty::Unknown2( int a1, void *a2, int a3 )
{

}

void *CShaderAPIEmpty::Unknown3( void *a1, int a2 )
{
	return nullptr;
}

void *CShaderAPIEmpty::Unknown4( void *a1, int a2 )
{
	return nullptr;
}

void CShaderAPIEmpty::Unknown5( void **a1, void **a2, char *a3 )
{

}

void CShaderAPIEmpty::Unknown6( void *a1, int a2 )
{

}
