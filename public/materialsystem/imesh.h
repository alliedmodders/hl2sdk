//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#ifndef IMESH_H
#define IMESH_H

#ifdef _WIN32
#pragma once
#endif

#include "interface.h"
#include "imaterialsystem.h"
#include <float.h>
#include <string.h>
#include "tier0/dbg.h"


//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
class IMaterial;
class CMeshBuilder;
class IMaterialVar;


//-----------------------------------------------------------------------------
// The Vertex Buffer interface
//-----------------------------------------------------------------------------
enum
{
	VERTEX_MAX_TEXTURE_COORDINATES = 4,
	BONE_MATRIX_INDEX_INVALID = 255
};


//-----------------------------------------------------------------------------
// The Mesh memory descriptor
//-----------------------------------------------------------------------------
struct MeshDescV1_t
{
	// These can be set to zero if there are pointers to dummy buffers, when the
	// actual buffer format doesn't contain the data but it needs to be safe to
	// use all the CMeshBuilder functions.
	int	m_VertexSize_Position;
	int m_VertexSize_BoneWeight;
	int m_VertexSize_BoneMatrixIndex;
	int	m_VertexSize_Normal;
	int	m_VertexSize_Color;
	int	m_VertexSize_Specular;
	int m_VertexSize_TexCoord[VERTEX_MAX_TEXTURE_COORDINATES];
	int m_VertexSize_TangentS;
	int m_VertexSize_TangentT;
#ifndef _XBOX
	int m_VertexSize_TangentSxT;
#endif
	int m_VertexSize_UserData;

	int m_ActualVertexSize;	// Size of the vertices.. Some of the m_VertexSize_ elements above
							// are set to this value and some are set to zero depending on which
							// fields exist in a buffer's vertex format.

	// The first vertex index
	int	m_FirstVertex;

	// Number of bone weights per vertex...
	int m_NumBoneWeights;

	// Pointers to our current vertex data
	float			*m_pPosition;

#ifndef _XBOX
	float			*m_pBoneWeight;
#else
	// compressed vertex
	unsigned char	*m_pBoneWeight;
#endif

#ifndef NEW_SKINNING
	unsigned char	*m_pBoneMatrixIndex;
#else
	float			*m_pBoneMatrixIndex;
#endif

#ifndef _XBOX
	float			*m_pNormal;
#else
	// compressed vertex
	unsigned char	*m_pNormal;
#endif

	unsigned char	*m_pColor;
	unsigned char	*m_pSpecular;
	float			*m_pTexCoord[VERTEX_MAX_TEXTURE_COORDINATES];

	// Tangent space *associated with one particular set of texcoords*
#ifndef _XBOX
	float			*m_pTangentS;
	float			*m_pTangentT;
	float			*m_pTangentSxT;
#else
	// compressed vertex
	unsigned char	*m_pTangentS;
	unsigned char	*m_pTangentT;
#endif

	// user data
#ifndef _XBOX
	float			*m_pUserData;
#else
	// compressed vertex
	unsigned char	*m_pUserData;
#endif

	// Pointers to the index data
	unsigned short	*m_pIndices;
};


#ifndef _XBOX
struct MeshDesc_t : public MeshDescV1_t
{
	// For b/w compatibility.
	void GetMeshDescV1( MeshDescV1_t &out ) const;

	// NEW SHIT AT THE END
	// Size of the index data measured in unsigned shorts. 
	// 1 if the device is active, 0 if the device isn't active.
	// Faster than doing if checks for null m_pIndices if someone is
	// trying to write the m_pIndices while the device is inactive.
	unsigned char m_IndexSize;
};
#else
struct MeshDesc_t
{
	// For b/w compatibility.
	void GetMeshDescV1( MeshDescV1_t &out ) const;

	// These can be set to zero if there are pointers to dummy buffers, when the
	// actual buffer format doesn't contain the data but it needs to be safe to
	// use all the CMeshBuilder functions.
	unsigned char m_VertexSize_Position;
	unsigned char m_VertexSize_BoneWeight;
	unsigned char m_VertexSize_BoneMatrixIndex;
	unsigned char m_VertexSize_Normal;
	unsigned char m_VertexSize_Color;
	unsigned char m_VertexSize_Specular;
	unsigned char m_VertexSize_TexCoord[VERTEX_MAX_TEXTURE_COORDINATES];
	unsigned char m_VertexSize_TangentS;
	unsigned char m_VertexSize_TangentT;
	unsigned char m_VertexSize_UserData;

	unsigned char m_ActualVertexSize;	// Size of the vertices.. Some of the m_VertexSize_ elements above
										// are set to this value and some are set to zero depending on which
										// fields exist in a buffer's vertex format.

	// Number of bone weights per vertex...
	unsigned char m_NumBoneWeights;

	// Size of the index data measured in unsigned shorts. 
	// 1 if the device is active, 0 if the device isn't active.
	// Faster than doing if checks for null m_pIndices if someone is
	// trying to write the m_pIndices while the device is inactive.
	unsigned char m_IndexSize;

	// The first vertex index
	int	m_FirstVertex;

	// Pointers to our current vertex data
	float			*m_pPosition;

	// compressed vertex
	unsigned char	*m_pBoneWeight;

	// compressed vertex
	unsigned char	*m_pUserData;

	// compressed vertex
	unsigned char	*m_pNormal;

	// compressed vertex
	unsigned char	*m_pTangentS;
	unsigned char	*m_pTangentT;


#	ifndef NEW_SKINNING
	unsigned char	*m_pBoneMatrixIndex;
#	else
	float			*m_pBoneMatrixIndex;
#	endif

	unsigned char	*m_pColor;
	unsigned char	*m_pSpecular;
	float			*m_pTexCoord[VERTEX_MAX_TEXTURE_COORDINATES];

	// Pointers to the index data
	unsigned short	*m_pIndices;
};
#endif

inline void MeshDesc_t::GetMeshDescV1( MeshDescV1_t &out ) const
{
	out.m_VertexSize_Position = out.m_VertexSize_Position;
	out.m_VertexSize_BoneWeight = out.m_VertexSize_BoneWeight;
	out.m_VertexSize_BoneMatrixIndex = out.m_VertexSize_BoneMatrixIndex;
	out.m_VertexSize_Normal = out.m_VertexSize_Normal;
	out.m_VertexSize_Color = out.m_VertexSize_Color;
	out.m_VertexSize_Specular = out.m_VertexSize_Specular;
	
	for ( int i=0; i < VERTEX_MAX_TEXTURE_COORDINATES; i++ )
	{
		out.m_VertexSize_TexCoord[i] = out.m_VertexSize_TexCoord[i];
		out.m_pTexCoord[i] = out.m_pTexCoord[i];
	}
	
	out.m_VertexSize_TangentS = out.m_VertexSize_TangentS;
	out.m_VertexSize_TangentT = out.m_VertexSize_TangentT;

#ifndef _XBOX
	out.m_VertexSize_TangentSxT = out.m_VertexSize_TangentSxT;
	out.m_pTangentSxT = out.m_pTangentSxT;
#endif

	out.m_VertexSize_UserData = out.m_VertexSize_UserData;
	out.m_ActualVertexSize = out.m_ActualVertexSize;
	out.m_NumBoneWeights = out.m_NumBoneWeights;
	out.m_FirstVertex = out.m_FirstVertex;
	out.m_pPosition = out.m_pPosition;
	out.m_pBoneWeight = out.m_pBoneWeight;
	out.m_pBoneMatrixIndex = out.m_pBoneMatrixIndex;
	out.m_pNormal = out.m_pNormal;
	out.m_pColor = out.m_pColor;
	out.m_pSpecular = out.m_pSpecular;
	out.m_pTangentS = out.m_pTangentS;
	out.m_pTangentT = out.m_pTangentT;
	out.m_pUserData = out.m_pUserData;
	out.m_pIndices = out.m_pIndices;
}


//-----------------------------------------------------------------------------
// Used in lists of indexed primitives.
//-----------------------------------------------------------------------------
class CPrimList
{
public:
				CPrimList();
				CPrimList( int firstIndex, int numIndices );

public:
	int			m_FirstIndex;
	int			m_NumIndices;
};


inline CPrimList::CPrimList()
{
}

inline CPrimList::CPrimList( int firstIndex, int numIndices )
{
	m_FirstIndex = firstIndex;
	m_NumIndices = numIndices;
}


//-----------------------------------------------------------------------------
// Standard vertex formats for models
//-----------------------------------------------------------------------------
struct ModelVertexDX7_t
{
	Vector			m_vecPosition;
	Vector2D		m_flBoneWeights;
	unsigned int	m_nBoneIndices;
	Vector			m_vecNormal;
	unsigned int	m_nColor;	// ARGB
	Vector2D		m_vecTexCoord;
};

struct ModelVertexDX8_t	: public ModelVertexDX7_t
{
	Vector4D		m_vecUserData;
};

//-----------------------------------------------------------------------------
// Interface to the mesh
//-----------------------------------------------------------------------------
abstract_class IMesh
{
public:
	// Old version, for backward compat
	// Locks/ unlocks the mesh, providing space for numVerts and numIndices.
	// numIndices of -1 means don't lock the index buffer...
	virtual void LockMeshOld( int numVerts, int numIndices, MeshDescV1_t& desc ) = 0;

	// Unlocks the mesh, indicating	how many verts and indices we actually used
	virtual void UnlockMeshOld( int numVerts, int numIndices, MeshDescV1_t& desc ) = 0;

	// Locks mesh for modifying
	virtual void ModifyBeginOld( int firstVertex, int numVerts, int firstIndex, int numIndices, MeshDescV1_t& desc ) = 0;
	virtual void ModifyEnd() = 0;

	// Helper methods to create various standard index buffer types
	virtual void GenerateSequentialIndexBuffer( unsigned short* pIndexMemory, 
												int numIndices, int firstVertex ) = 0;
	virtual void GenerateQuadIndexBuffer( unsigned short* pIndexMemory, 
											int numIndices, int firstVertex ) = 0;
	virtual void GeneratePolygonIndexBuffer( unsigned short* pIndexMemory, 
												int numIndices, int firstVertex ) = 0;
	virtual void GenerateLineStripIndexBuffer( unsigned short* pIndexMemory, 
												int numIndices, int firstVertex ) = 0;
	virtual void GenerateLineLoopIndexBuffer( unsigned short* pIndexMemory, 
												int numIndices, int firstVertex ) = 0;

	// returns the # of vertices (static meshes only)
	virtual int NumVertices() const = 0;

	// Sets/gets the primitive type
	virtual void SetPrimitiveType( MaterialPrimitiveType_t type ) = 0;
	
	// Draws the mesh
	virtual void Draw( int firstIndex = -1, int numIndices = 0 ) = 0;
	
	virtual void SetColorMesh( IMesh *pColorMesh ) = 0;

	// Draw a list of (lists of) primitives. Batching your lists together that use
	// the same lightmap, material, vertex and index buffers with multipass shaders
	// can drastically reduce state-switching overhead.
	// NOTE: this only works with STATIC meshes.
	virtual void Draw( CPrimList *pLists, int nLists ) = 0;

	// Copy verts and/or indices to a mesh builder. This only works for temp meshes!
	virtual void CopyToMeshBuilder( 
		int iStartVert,		// Which vertices to copy.
		int nVerts, 
		int iStartIndex,	// Which indices to copy.
		int nIndices, 
		int indexOffset,	// This is added to each index.
		CMeshBuilder &builder ) = 0;

	// Spews the mesh data
	virtual void Spew( int numVerts, int numIndices, const MeshDescV1_t & desc ) = 0;

	// Call this in debug mode to make sure our data is good.
	virtual void ValidateData( int numVerts, int numIndices, const MeshDescV1_t & desc ) = 0;
#ifndef _XBOX
	// Causes the software vertex shader to be applied to the mesh
	virtual void CallSoftwareVertexShader( CMeshBuilder *pMeshBuilder ) = 0;
#endif

	// New version
	// Locks/unlocks the mesh, providing space for numVerts and numIndices.
	// numIndices of -1 means don't lock the index buffer...
	virtual void LockMesh( int numVerts, int numIndices, MeshDesc_t& desc ) = 0;
	virtual void ModifyBegin( int firstVertex, int numVerts, int firstIndex, int numIndices, MeshDesc_t& desc ) = 0;
	virtual void UnlockMesh( int numVerts, int numIndices, MeshDesc_t& desc ) = 0;

#ifdef _XBOX
	virtual unsigned ComputeMemoryUsed() = 0;
#endif

	virtual int NumIndices() const = 0;
	virtual void ModifyBeginEx( bool bReadOnly, int firstVertex, int numVerts, int firstIndex, int numIndices, MeshDesc_t& desc ) = 0;

#ifndef _XBOX
	virtual void SetFlexMesh( IMesh *pMesh, int nVertexOffset ) = 0;
#else
	virtual void SetFlexMesh( IMesh *pMesh, float *pFlexVerts ) = 0;
#endif

	virtual void DisableFlexMesh() = 0;

	virtual void MarkAsDrawn() = 0;
};


#include "meshreader.h"


//-----------------------------------------------------------------------------
// Helper class used to define meshes
//-----------------------------------------------------------------------------
class CMeshBuilder : private MeshDesc_t
{
public:
	CMeshBuilder();

	// Locks the vertex buffer
	// (*cannot* use the Index() call below)
	void Begin( IMesh* pMesh, MaterialPrimitiveType_t type, int numPrimitives );

	// Locks the vertex buffer, can specify arbitrary index lists
	// (must use the Index() call below)
	void Begin( IMesh* pMesh, MaterialPrimitiveType_t type, int numVertices, int numIndices, int *nFirstVertex );
	void Begin( IMesh* pMesh, MaterialPrimitiveType_t type, int numVertices, int numIndices );

	// Use this when you're done writing
	// Set bDraw to true to call m_pMesh->Draw automatically.
	void End( bool spewData = false, bool bDraw = false );

	// Locks the vertex buffer to modify existing data
	// Passing numVertices == -1 says to lock all the vertices for modification.
	// Pass 0 for numIndices to not lock the index buffer.
	void BeginModify( IMesh* pMesh, int firstVertex = 0, int numVertices = -1, int firstIndex = 0, int numIndices = 0 );
	void EndModify( bool spewData = false );

	// A helper method since this seems to be done a whole bunch.
	void DrawQuad( IMesh* pMesh, float const* v1, float const* v2, 
		float const* v3, float const* v4, unsigned char const* pColor, bool wireframe = false );

	// returns the number of indices and vertices
	int NumVertices() const;
	int	NumIndices() const;

	// Resets the mesh builder so it points to the start of everything again
	void Reset();

	// Returns the size of the vertex
	int VertexSize() { return m_ActualVertexSize; }

	// Returns the base vertex memory pointer
	void* BaseVertexData();

	// Selects the nth Vertex and Index 
	void SelectVertex( int idx );
	void SelectIndex( int idx );

	// Given an index, point to the associated vertex
	void SelectVertexFromIndex( int idx );

	// Advances the current vertex and index by one
	void AdvanceVertex();
	void AdvanceVertices( int nVerts );
	void AdvanceIndex();
	void AdvanceIndices( int nIndices );

	int GetCurrentVertex();
	int GetCurrentIndex();

	// Data retrieval...
	float const* Position() const;
#ifndef _XBOX
	float const* Normal() const;
#else
	unsigned char const *PackedNormal() const;
#endif
	unsigned int Color() const;
	unsigned char *SpecularPtr() const;

	float const* TexCoord( int stage ) const;

#ifndef _XBOX
	float const* TangentS() const;
	float const* TangentT() const;
	float const* TangentSxT() const;
#else
	unsigned char const *PackedTangentS() const;
	unsigned char const *PackedTangentT() const;
#endif

#ifndef _XBOX
	float const* BoneWeight() const;
#else
	unsigned char const *PackedBoneWeight() const;
#endif
	int NumBoneWeights() const;
#ifndef NEW_SKINNING
	unsigned char* BoneMatrix() const;
#else
	float* BoneMatrix() const;
#endif
	unsigned short const* Index( ) const;

	// position setting
	void Position3f( float x, float y, float z );
	void Position3fv( float const *v );

	// normal setting
	void Normal3f( float nx, float ny, float nz );
	void Normal3fv( float const *n );
	void NormalDelta3fv( float const *n );
	void NormalDelta3f( float nx, float ny, float nz );

	// color setting
	void Color3f( float r, float g, float b );
	void Color3fv( float const *rgb );
	void Color4f( float r, float g, float b, float a );
	void Color4fv( float const *rgba );

	// Faster versions of color
	void Color3ub( unsigned char r, unsigned char g, unsigned char b );
	void Color3ubv( unsigned char const* rgb );
	void Color4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a );
	void Color4ubv( unsigned char const* rgba );

	// specular color setting
	void Specular3f( float r, float g, float b );
	void Specular3fv( const float *rgb );
	void Specular4f( float r, float g, float b, float a );
	void Specular4fv( const float *rgba );

	// Faster version of specular
	void Specular3ub( unsigned char r, unsigned char g, unsigned char b );
	void Specular3ubv( unsigned char const *c );
	void Specular4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a );
	void Specular4ubv( unsigned char const *c );

	// texture coordinate setting
	void TexCoord1f( int stage, float s );
	void TexCoord2f( int stage, float s, float t );
	void TexCoord2fv( int stage, float const *st );
	void TexCoord3f( int stage, float s, float t, float u );
	void TexCoord3fv( int stage, float const *stu );

	void TexCoordSubRect2f( int stage, float s, float t, float offsetS, float offsetT, float scaleS, float scaleT );
	void TexCoordSubRect2fv( int stage, const float *st, const float *offset, const float *scale );

	// tangent space 
	void TangentS3f( float sx, float sy, float sz );
	void TangentS3fv( float const* s );

	void TangentT3f( float tx, float ty, float tz );
	void TangentT3fv( float const* t );
#ifndef _XBOX
	void TangentSxT3f( float sxtx, float sxty, float sxtz );
	void TangentSxT3fv( float const* sxt );
#endif
	// bone weights
	void BoneWeight( int idx, float weight );

	// bone matrix index
	void BoneMatrix( int idx, int matrixIndex );
	
	// Generic per-vertex data
	void UserData( float const* pData );

	// Used to define the indices (only used if you aren't using primitives)
	void Index( unsigned short index );
	
	// Fast Index! No need to call advance index, and no random access allowed
	void FastIndex( unsigned short index );

	// Fast Vertex! No need to call advance vertex, and no random access allowed. 
	// WARNING - these are low level functions that are intended only for use
	// in the software vertex skinner.
#ifndef _XBOX
	void FastVertex( const ModelVertexDX7_t &vertex );
	void FastVertexSSE( const ModelVertexDX7_t &vertex );

	// store 4 dx7 vertices fast. for special sse dx7 pipeline
	void Fast4VerticesSSE( 
		ModelVertexDX7_t const *vtx_a,
		ModelVertexDX7_t const *vtx_b,
		ModelVertexDX7_t const *vtx_c,
		ModelVertexDX7_t const *vtx_d);

	void FastVertex( const ModelVertexDX8_t &vertex );
	void FastVertexSSE( const ModelVertexDX8_t &vertex );
#else
	void VertexDX8ToXBox( const ModelVertexDX8_t &vertex );
#endif

	// Add number of verts and current vert since FastVertexxx routines do not update.
	void FastAdvanceNVertices(int n);	

private:
	// Computes number of verts and indices 
	void ComputeNumVertsAndIndices( MaterialPrimitiveType_t type, 
							int numPrimitives );
	int IndicesFromVertices( MaterialPrimitiveType_t type, int numVerts ); 

	// Internal helper methods
	float const* OffsetFloatPointer( float const* pBufferPointer, int numVerts, int vertexSize ) const;
	float* OffsetFloatPointer( float* pBufferPointer, int numVerts, int vertexSize );
	void IncrementFloatPointer( float* &pBufferPointer, int vertexSize );

	// The mesh we're modifying
	IMesh* m_pMesh;

	// Used to make sure Begin/End calls and BeginModify/EndModify calls match.
	bool m_bModify;

	// Max number of indices and vertices
	int m_MaxVertices;
	int m_MaxIndices;

	// Number of indices and vertices
	int m_NumVertices;
	int m_NumIndices;

	// The current vertex and index
	mutable int m_CurrentVertex;
	mutable int m_CurrentIndex;

	// Generate indices?
	MaterialPrimitiveType_t m_Type;
	bool m_GenerateIndices;

	// Optimization: Pointer to the current pos, norm, texcoord, and color
	mutable float			*m_pCurrPosition;
#ifndef _XBOX
	mutable float			*m_pCurrNormal;
#else
	mutable unsigned char	*m_pCurrNormal;
#endif
	mutable float			*m_pCurrTexCoord[VERTEX_MAX_TEXTURE_COORDINATES];
	mutable unsigned char	*m_pCurrColor;
};


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------

inline CMeshBuilder::CMeshBuilder()	: m_pMesh(0), m_NumIndices(0), 
	m_NumVertices(0), m_CurrentVertex(0), m_CurrentIndex(0), m_MaxVertices(0),
	m_MaxIndices(0), m_GenerateIndices(false)
{
#ifdef _DEBUG
	m_pCurrPosition = NULL;
	m_pCurrNormal = NULL;
	m_pCurrColor = NULL;
	m_pCurrTexCoord[0] = NULL;
	m_pCurrTexCoord[1] = NULL;
	m_pCurrTexCoord[2] = NULL;
	m_pCurrTexCoord[3] = NULL;
	m_bModify = false;
#endif
}


//-----------------------------------------------------------------------------
// Computes the number of verts and indices based on primitive type and count
//-----------------------------------------------------------------------------

inline void CMeshBuilder::ComputeNumVertsAndIndices( MaterialPrimitiveType_t type, 
							int numPrimitives )
{
	switch(type)
	{
	case MATERIAL_POINTS:
		m_MaxVertices = m_MaxIndices = numPrimitives;
		break;

	case MATERIAL_LINES:
		m_MaxVertices = m_MaxIndices = numPrimitives * 2;
		break;

	case MATERIAL_LINE_STRIP:
		m_MaxVertices = numPrimitives + 1;
		m_MaxIndices = numPrimitives * 2;
		break;

	case MATERIAL_LINE_LOOP:
		m_MaxVertices = numPrimitives;
		m_MaxIndices = numPrimitives * 2;
		break;

	case MATERIAL_TRIANGLES:
		m_MaxVertices = m_MaxIndices = numPrimitives * 3;
		break;

	case MATERIAL_TRIANGLE_STRIP:
		m_MaxVertices = m_MaxIndices = numPrimitives + 2;
		break;

	case MATERIAL_QUADS:
		m_MaxVertices = numPrimitives * 4;
		m_MaxIndices = numPrimitives * 6;
		break;

	case MATERIAL_POLYGON:
		m_MaxVertices = numPrimitives;
		m_MaxIndices = (numPrimitives - 2) * 3;
		break;
				
	default:
		Assert(0);
	}

	// FIXME: need to get this from meshdx8.cpp, or move it to somewhere common
	Assert( m_MaxVertices <= 32768 );
	Assert( m_MaxIndices <= 32768 );
}

inline int CMeshBuilder::IndicesFromVertices( MaterialPrimitiveType_t type, int numVerts )
{
	if (type == MATERIAL_QUADS)
	{
		Assert( (numVerts & 0x3) == 0 );
		return (numVerts * 6) / 4;
	}
	else if (type == MATERIAL_POLYGON)
	{
		Assert( numVerts >= 3 );
		return (numVerts - 2) * 3;
	}
	else if (type == MATERIAL_LINE_STRIP)
	{
		Assert( numVerts >= 2 );
		return (numVerts - 1) * 2;
	}
	else if (type == MATERIAL_LINE_LOOP)
	{
		Assert( numVerts >= 3 );
		return numVerts * 2;
	}

	return numVerts;
}

//-----------------------------------------------------------------------------
// Begins modifying the mesh
//-----------------------------------------------------------------------------

inline void CMeshBuilder::Begin( IMesh* pMesh, MaterialPrimitiveType_t type, int numPrimitives )
{
	Assert( pMesh && (!m_pMesh) );
	m_pMesh = pMesh;
	m_bModify = false;

	Assert( type != MATERIAL_HETEROGENOUS );

	ComputeNumVertsAndIndices( type, numPrimitives );
	m_NumIndices = 0; m_NumVertices = 0;
	m_GenerateIndices = true;
	m_Type = type;

	if ((type == MATERIAL_QUADS) || (type == MATERIAL_POLYGON))
	{
		m_pMesh->SetPrimitiveType( MATERIAL_TRIANGLES );
	}
	else if ((type == MATERIAL_LINE_STRIP) || (type == MATERIAL_LINE_LOOP))
	{
		m_pMesh->SetPrimitiveType( MATERIAL_LINES );
	}
	else
	{
		m_pMesh->SetPrimitiveType( type );
	}

	// Lock the mesh
	m_pMesh->LockMesh( m_MaxVertices, m_MaxIndices, *this );

	// Point to the start of the index and vertex buffers
	Reset();
}

inline void CMeshBuilder::Begin( IMesh* pMesh, MaterialPrimitiveType_t type, int numVertices, int numIndices, int *nFirstVertex )
{
	Begin( pMesh, type, numVertices, numIndices );

	*nFirstVertex = m_FirstVertex * m_ActualVertexSize;
}

inline void CMeshBuilder::Begin( IMesh* pMesh, MaterialPrimitiveType_t type, int numVertices, int numIndices )
{
	Assert( pMesh && (!m_pMesh) );

	// NOTE: We can't specify the indices when we use quads, polygons, or
	// linestrips; they aren't actually directly supported by 
	// the material system
	Assert( (type != MATERIAL_QUADS) && (type != MATERIAL_POLYGON) &&
		(type != MATERIAL_LINE_STRIP) && (type != MATERIAL_LINE_LOOP));

	// Dx8 doesn't support indexed points...
	Assert( type != MATERIAL_POINTS );

	m_pMesh = pMesh;
	m_bModify = false;

	// Set the primitive type
	m_pMesh->SetPrimitiveType( type );

	m_MaxVertices = numVertices;
	m_MaxIndices = numIndices;
	m_NumIndices = 0; m_NumVertices = 0;
	m_GenerateIndices = false;
 	m_Type = type;

	// Lock the vertex and index buffer
	m_pMesh->LockMesh( m_MaxVertices, m_MaxIndices, *this );

	// Point to the start of the buffers..
	Reset();
}


//-----------------------------------------------------------------------------
// Use this when you're done modifying the mesh
//-----------------------------------------------------------------------------
inline void CMeshBuilder::End( bool spewData, bool bDraw )
{
	// Make sure they called Begin()
	Assert( !m_bModify );

	if (m_GenerateIndices)
	{
		m_NumIndices = IndicesFromVertices( m_Type, m_NumVertices ); 
		if (m_Type == MATERIAL_QUADS)
		{
			m_pMesh->GenerateQuadIndexBuffer( m_pIndices, m_NumIndices, m_FirstVertex );
		}
		else if (m_Type == MATERIAL_POLYGON)
		{
			m_pMesh->GeneratePolygonIndexBuffer( m_pIndices, m_NumIndices, m_FirstVertex );
		}
		else if (m_Type == MATERIAL_LINE_STRIP)
		{
			m_pMesh->GenerateLineStripIndexBuffer( m_pIndices, m_NumIndices, m_FirstVertex );
		}
		else if (m_Type == MATERIAL_LINE_LOOP)
		{
			m_pMesh->GenerateLineLoopIndexBuffer( m_pIndices, m_NumIndices, m_FirstVertex );
		}
		else if (m_Type != MATERIAL_POINTS)
		{
			m_pMesh->GenerateSequentialIndexBuffer( m_pIndices, m_NumIndices, m_FirstVertex );
		}
	}
#ifndef _XBOX
	if( m_NumVertices > 0 )
	{
		m_pMesh->CallSoftwareVertexShader( this );
	}
#endif
	if (spewData)
	{
		MeshDescV1_t v1;
		GetMeshDescV1( v1 );
		m_pMesh->Spew( m_NumVertices, m_NumIndices, v1 );
	}
	
#ifdef _DEBUG
	MeshDescV1_t v1;
	GetMeshDescV1( v1 );
	m_pMesh->ValidateData( m_NumVertices, m_NumIndices, v1 );
#endif

	// Unlock our buffers
	m_pMesh->UnlockMesh( m_NumVertices, m_NumIndices, *this );

	if( bDraw )
		m_pMesh->Draw();

	m_pMesh = 0;
	m_MaxVertices = m_MaxIndices = 0;

#ifdef _DEBUG
	// Null out our pointers...
	m_pCurrPosition = NULL;
	m_pCurrNormal = NULL;
	m_pCurrColor = NULL;
	m_pCurrTexCoord[0] = NULL;
	m_pCurrTexCoord[1] = NULL;
	m_pCurrTexCoord[2] = NULL;
	m_pCurrTexCoord[3] = NULL;
	memset( (MeshDesc_t*)this, 0, sizeof(MeshDesc_t) );
#endif
}


//-----------------------------------------------------------------------------
// Locks the vertex buffer to modify existing data
//-----------------------------------------------------------------------------

inline void CMeshBuilder::BeginModify( IMesh* pMesh, int firstVertex, int numVertices, int firstIndex, int numIndices )
{
	Assert( pMesh && (!m_pMesh) );

	if (numVertices < 0)
		numVertices = pMesh->NumVertices();
	
	m_pMesh = pMesh;
	m_MaxVertices = m_NumVertices = numVertices;
	m_MaxIndices = m_NumIndices = numIndices;
	m_GenerateIndices = false;
	m_bModify = true;

	// Locks mesh for modifying
	pMesh->ModifyBeginEx( false, firstVertex, numVertices, firstIndex, numIndices, *this );

	// Point to the start of the buffers..
	Reset();
}

inline void CMeshBuilder::EndModify( bool spewData )
{
	Assert( m_pMesh );
	Assert( m_bModify );	// Make sure they called BeginModify.

	if (spewData)
	{
		MeshDescV1_t v1;
		GetMeshDescV1( v1 );
		m_pMesh->Spew( m_NumVertices, m_NumIndices, v1 );
	}
#ifdef _DEBUG
	MeshDescV1_t v1;
	GetMeshDescV1( v1 );
	m_pMesh->ValidateData( m_NumVertices, m_NumIndices, v1 );
#endif

	// Unlocks mesh
	m_pMesh->ModifyEnd( );
	m_pMesh = 0;
	m_MaxVertices = m_MaxIndices = 0;

#ifdef _DEBUG
	// Null out our pointers...
	memset( (MeshDesc_t*)this, 0, sizeof(MeshDesc_t) );
#endif
}


//-----------------------------------------------------------------------------
// Resets the mesh builder so it points to the start of everything again
//-----------------------------------------------------------------------------
inline void CMeshBuilder::Reset()
{
	m_CurrentVertex = 0;
	m_CurrentIndex = 0;

	m_pCurrPosition = m_pPosition;
	m_pCurrNormal = m_pNormal;
	m_pCurrTexCoord[0] = m_pTexCoord[0];
	m_pCurrTexCoord[1] = m_pTexCoord[1];
	m_pCurrTexCoord[2] = m_pTexCoord[2];
	m_pCurrTexCoord[3] = m_pTexCoord[3];
	m_pCurrColor = m_pColor;
}


//-----------------------------------------------------------------------------
// returns a float 
//-----------------------------------------------------------------------------
inline float const* CMeshBuilder::OffsetFloatPointer( float const* pBufferPointer, int numVerts, int vertexSize ) const
{
	return reinterpret_cast<float const*>(
		reinterpret_cast<unsigned char const*>(pBufferPointer) + 
		numVerts * vertexSize);
}

inline float* CMeshBuilder::OffsetFloatPointer( float* pBufferPointer, int numVerts, int vertexSize )
{
	return reinterpret_cast<float*>(
		reinterpret_cast<unsigned char*>(pBufferPointer) + 
		numVerts * vertexSize);
}

inline void CMeshBuilder::IncrementFloatPointer( float* &pBufferPointer, int vertexSize )
{
	pBufferPointer = reinterpret_cast<float*>( reinterpret_cast<unsigned char*>(pBufferPointer) + vertexSize );
}


//-----------------------------------------------------------------------------
// Selects the current Vertex and Index 
//-----------------------------------------------------------------------------
inline void CMeshBuilder::SelectVertex( int idx )
{
	// NOTE: This index is expected to be relative 
	Assert( (idx >= 0) && (idx < m_MaxVertices) );
	m_CurrentVertex = idx;

	m_pCurrPosition    = OffsetFloatPointer( m_pPosition, m_CurrentVertex, m_VertexSize_Position );
#ifndef _XBOX
	m_pCurrNormal      = OffsetFloatPointer( m_pNormal, m_CurrentVertex, m_VertexSize_Normal );
#else
	m_pCurrNormal      = m_pNormal + m_CurrentVertex * m_VertexSize_Normal;
#endif
	m_pCurrTexCoord[0] = OffsetFloatPointer( m_pTexCoord[0], m_CurrentVertex, m_VertexSize_TexCoord[0] );
	m_pCurrTexCoord[1] = OffsetFloatPointer( m_pTexCoord[1], m_CurrentVertex, m_VertexSize_TexCoord[1] );
	m_pCurrTexCoord[2] = OffsetFloatPointer( m_pTexCoord[2], m_CurrentVertex, m_VertexSize_TexCoord[2] );
	m_pCurrTexCoord[3] = OffsetFloatPointer( m_pTexCoord[3], m_CurrentVertex, m_VertexSize_TexCoord[3] );
	m_pCurrColor       = m_pColor + m_CurrentVertex * m_VertexSize_Color;
}

inline void CMeshBuilder::SelectVertexFromIndex( int idx )
{
	// NOTE: This index is expected to be relative 
	int vertIdx = idx - m_FirstVertex;
	SelectVertex( vertIdx );
}

inline void CMeshBuilder::SelectIndex( int idx )
{
	Assert( (idx >= 0) && (idx < m_NumIndices) );
	m_CurrentIndex = idx * m_IndexSize;
}


//-----------------------------------------------------------------------------
// Advances the current vertex and index by one
//-----------------------------------------------------------------------------
inline void CMeshBuilder::AdvanceVertex()
{
	if (++m_CurrentVertex > m_NumVertices)
		m_NumVertices = m_CurrentVertex;
	
	IncrementFloatPointer( m_pCurrPosition, m_VertexSize_Position );
#ifndef _XBOX
	IncrementFloatPointer( m_pCurrNormal, m_VertexSize_Normal );
#else
	m_pCurrNormal += m_VertexSize_Normal;
#endif
	IncrementFloatPointer( m_pCurrTexCoord[0], m_VertexSize_TexCoord[0] );
	IncrementFloatPointer( m_pCurrTexCoord[1], m_VertexSize_TexCoord[1] );
	IncrementFloatPointer( m_pCurrTexCoord[2], m_VertexSize_TexCoord[2] );
	IncrementFloatPointer( m_pCurrTexCoord[3], m_VertexSize_TexCoord[3] );
	m_pCurrColor += m_VertexSize_Color;
}

inline void CMeshBuilder::AdvanceVertices( int nVerts )
{
	m_CurrentVertex += nVerts;
	if (m_CurrentVertex > m_NumVertices)
		m_NumVertices = m_CurrentVertex;
	
	IncrementFloatPointer( m_pCurrPosition, m_VertexSize_Position*nVerts );
#ifndef _XBOX
	IncrementFloatPointer( m_pCurrNormal, m_VertexSize_Normal*nVerts );
#else
	m_pCurrNormal +=  m_VertexSize_Normal*nVerts;
#endif
	IncrementFloatPointer( m_pCurrTexCoord[0], m_VertexSize_TexCoord[0]*nVerts );
	IncrementFloatPointer( m_pCurrTexCoord[1], m_VertexSize_TexCoord[1]*nVerts );
	IncrementFloatPointer( m_pCurrTexCoord[2], m_VertexSize_TexCoord[2]*nVerts );
	IncrementFloatPointer( m_pCurrTexCoord[3], m_VertexSize_TexCoord[3]*nVerts );
	m_pCurrColor += m_VertexSize_Color*nVerts;
}

inline void CMeshBuilder::AdvanceIndex()
{
	m_CurrentIndex += m_IndexSize;
	if ( m_CurrentIndex > m_NumIndices )
	{
		m_NumIndices = m_CurrentIndex; 
	}
}

inline void CMeshBuilder::AdvanceIndices( int nIndices )
{
	m_CurrentIndex += nIndices * m_IndexSize;
	if ( m_CurrentIndex > m_NumIndices )
	{
		m_NumIndices = m_CurrentIndex; 
	}
}

inline int CMeshBuilder::GetCurrentVertex()
{
	return m_CurrentVertex;
}

inline int CMeshBuilder::GetCurrentIndex()
{
	return m_CurrentIndex;
}


//-----------------------------------------------------------------------------
// A helper method since this seems to be done a whole bunch.
//-----------------------------------------------------------------------------
inline void CMeshBuilder::DrawQuad( IMesh* pMesh, float const* v1, float const* v2, 
	float const* v3, float const* v4, unsigned char const* pColor, bool wireframe )
{
	if (!wireframe)
	{
		Begin( pMesh, MATERIAL_TRIANGLE_STRIP, 2 );

		Position3fv (v1);
		Color4ubv( pColor );
		AdvanceVertex();

		Position3fv (v2);
		Color4ubv( pColor );
		AdvanceVertex();

		Position3fv (v4);
		Color4ubv( pColor );
		AdvanceVertex();

		Position3fv (v3);
		Color4ubv( pColor );
		AdvanceVertex();
	}
	else
	{
		Begin( pMesh, MATERIAL_LINE_LOOP, 4 );
		Position3fv (v1);
		Color4ubv( pColor );
		AdvanceVertex();

		Position3fv (v2);
		Color4ubv( pColor );
		AdvanceVertex();

		Position3fv (v3);
		Color4ubv( pColor );
		AdvanceVertex();

		Position3fv (v4);
		Color4ubv( pColor );
		AdvanceVertex();
	}

	End();
	pMesh->Draw();
}


//-----------------------------------------------------------------------------
// returns the number of indices and vertices
//-----------------------------------------------------------------------------

inline int CMeshBuilder::NumVertices() const
{
	return m_NumVertices;
}

inline int CMeshBuilder::NumIndices() const
{
	return m_NumIndices;
}

	
//-----------------------------------------------------------------------------
// Returns the base vertex memory pointer
//-----------------------------------------------------------------------------

inline void* CMeshBuilder::BaseVertexData()
{
	// FIXME: If there's no position specified, we need to find
	// the base address 
	Assert( m_pPosition );
	return m_pPosition;
}


//-----------------------------------------------------------------------------
// Data retrieval...
//-----------------------------------------------------------------------------
inline float const* CMeshBuilder::Position( ) const
{
	Assert( m_CurrentVertex < m_MaxVertices );
	return m_pCurrPosition;
}

#ifndef _XBOX
inline float const* CMeshBuilder::Normal()	const
{
	Assert( m_CurrentVertex < m_MaxVertices );
	return m_pCurrNormal;
}
#else
inline unsigned char const* CMeshBuilder::PackedNormal() const
{
	Assert( m_CurrentVertex < m_MaxVertices );
	return m_pCurrNormal;
}
#endif

inline unsigned int CMeshBuilder::Color( ) const
{
	// Swizzle it so it returns the same format as accepted by Color4ubv
	Assert( m_CurrentVertex < m_MaxVertices );
	return (m_pCurrColor[0] << 16) | (m_pCurrColor[1] << 8) | (m_pCurrColor[2]) | (m_pCurrColor[3] << 24);
}

inline unsigned char *CMeshBuilder::SpecularPtr() const
{
	Assert( m_CurrentVertex < m_MaxVertices );
	return m_pSpecular + m_CurrentVertex * m_VertexSize_Specular;
}

inline float const* CMeshBuilder::TexCoord( int stage ) const
{
	Assert( m_CurrentVertex < m_MaxVertices );
	return m_pCurrTexCoord[stage];
}

#ifndef _XBOX
inline float const* CMeshBuilder::TangentS() const
{
	Assert( m_CurrentVertex < m_MaxVertices );
	return OffsetFloatPointer( m_pTangentS, m_CurrentVertex, m_VertexSize_TangentS );
}
#else
inline unsigned char const* CMeshBuilder::PackedTangentS() const
{
	Assert( m_CurrentVertex < m_MaxVertices );
	return m_pTangentS;
}
#endif

#ifndef _XBOX
inline float const* CMeshBuilder::TangentT() const
{
	Assert( m_CurrentVertex < m_MaxVertices );
	return OffsetFloatPointer( m_pTangentT, m_CurrentVertex, m_VertexSize_TangentT );
}
#else
inline unsigned char const* CMeshBuilder::PackedTangentT() const
{
	Assert( m_CurrentVertex < m_MaxVertices );
	return m_pTangentT;
}
#endif

#ifndef _XBOX
inline float const* CMeshBuilder::TangentSxT( ) const
{
	Assert( m_CurrentVertex < m_MaxVertices );
	return OffsetFloatPointer( m_pTangentSxT, m_CurrentVertex, m_VertexSize_TangentSxT );
}
#endif

#ifndef _XBOX
inline float const* CMeshBuilder::BoneWeight() const
{
	Assert( m_CurrentVertex < m_MaxVertices );
	return OffsetFloatPointer( m_pBoneWeight, m_CurrentVertex, m_VertexSize_BoneWeight );
}
#else
inline unsigned char const* CMeshBuilder::PackedBoneWeight() const
{
	Assert( m_CurrentVertex < m_MaxVertices );
	return m_pBoneWeight + m_CurrentVertex * m_VertexSize_BoneWeight;
}
#endif

inline int CMeshBuilder::NumBoneWeights() const
{
	return m_NumBoneWeights;
}

#ifndef NEW_SKINNING
inline unsigned char* CMeshBuilder::BoneMatrix() const
{
	Assert( m_CurrentVertex < m_MaxVertices );
	return m_pBoneMatrixIndex + m_CurrentVertex * m_VertexSize_BoneMatrixIndex;
}
#else
inline float* CMeshBuilder::BoneMatrix() const
{
	Assert( m_CurrentVertex < m_MaxVertices );
	return m_pBoneMatrixIndex + m_CurrentVertex * m_VertexSize_BoneMatrixIndex;
}
#endif

inline unsigned short const* CMeshBuilder::Index( )	const
{
	Assert( m_CurrentIndex < m_MaxIndices );
	return &m_pIndices[m_CurrentIndex];
}


//-----------------------------------------------------------------------------
// Index
//-----------------------------------------------------------------------------
inline void CMeshBuilder::Index( unsigned short idx )
{
	Assert( m_pIndices );
	Assert( m_CurrentIndex < m_MaxIndices );
	m_pIndices[m_CurrentIndex] = (unsigned short)(m_FirstVertex + idx);
}


//-----------------------------------------------------------------------------
// Fast Index! No need to call advance index
//-----------------------------------------------------------------------------
inline void CMeshBuilder::FastIndex( unsigned short idx )
{
	Assert( m_pIndices );
	Assert( m_CurrentIndex < m_MaxIndices );
	m_pIndices[m_CurrentIndex] = (unsigned short)(m_FirstVertex + idx);
	m_CurrentIndex += m_IndexSize;
	m_NumIndices = m_CurrentIndex;	
}


//-----------------------------------------------------------------------------
// For use with the FastVertex methods, advances the current vertex by N
//-----------------------------------------------------------------------------
inline void CMeshBuilder::FastAdvanceNVertices( int n )
{
	m_CurrentVertex += n;
	m_NumVertices = m_CurrentVertex;
}


//-----------------------------------------------------------------------------
// Fast Vertex! No need to call advance vertex, and no random access allowed
//-----------------------------------------------------------------------------
#ifndef _XBOX
inline void CMeshBuilder::FastVertex( const ModelVertexDX7_t &vertex )
{
	Assert( m_CurrentVertex < m_MaxVertices );
	const void *pRead = &vertex;
	void *pCurrPos = m_pCurrPosition;

#ifdef _WIN32
	__asm
	{
		mov esi, pRead
		mov edi, pCurrPos

		movq mm0, [esi + 0]
		movq mm1, [esi + 8]
		movq mm2, [esi + 16]
		movq mm3, [esi + 24]
		movq mm4, [esi + 32]
		movq mm5, [esi + 40]

		movntq [edi + 0], mm0
		movntq [edi + 8], mm1
		movntq [edi + 16], mm2
		movntq [edi + 24], mm3
		movntq [edi + 32], mm4
		movntq [edi + 40], mm5

		emms
	}
#else
	Error( "Implement CMeshBuilder::FastVertex(dx7) ");
#endif

	IncrementFloatPointer( m_pCurrPosition, m_VertexSize_Position );
	//m_NumVertices = ++m_CurrentVertex;
}
#endif

#ifndef _XBOX
inline void CMeshBuilder::FastVertexSSE( const ModelVertexDX7_t &vertex )
{
	Assert( m_CurrentVertex < m_MaxVertices );
	const void *pRead = &vertex;
	void *pCurrPos = m_pCurrPosition;

#ifdef _WIN32
	__asm
	{
		mov esi, pRead
		mov edi, pCurrPos

		movaps xmm0, [esi + 0]
		movaps xmm1, [esi + 16]
		movaps xmm2, [esi + 32]

		movntps [edi + 0], xmm0
		movntps [edi + 16], xmm1
		movntps [edi + 32], xmm2
	}
#else
	Error( "Implement CMeshBuilder::FastVertexSSE(dx7)" );
#endif

	IncrementFloatPointer( m_pCurrPosition, m_VertexSize_Position );
	//m_NumVertices = ++m_CurrentVertex;
}
#endif

#ifndef _XBOX
inline void CMeshBuilder::Fast4VerticesSSE( 
	ModelVertexDX7_t const *vtx_a,
	ModelVertexDX7_t const *vtx_b,
	ModelVertexDX7_t const *vtx_c,
	ModelVertexDX7_t const *vtx_d)
{
	Assert( m_CurrentVertex < m_MaxVertices-3 );
	void *pCurrPos = m_pCurrPosition;

#ifdef _WIN32
	__asm
	{
		mov esi, vtx_a
		mov ebx, vtx_b

		mov edi, pCurrPos
        nop

		movaps xmm0, [esi + 0]
		movaps xmm1, [esi + 16]
		movaps xmm2, [esi + 32]
		movaps xmm3, [ebx + 0]
		movaps xmm4, [ebx + 16]
		movaps xmm5, [ebx + 32]

		mov esi, vtx_c
		mov ebx, vtx_d

		movntps [edi + 0], xmm0
		movntps [edi + 16], xmm1
		movntps [edi + 32], xmm2
		movntps [edi + 48], xmm3
		movntps [edi + 64], xmm4
		movntps [edi + 80], xmm5
		
		movaps xmm0, [esi + 0]
		movaps xmm1, [esi + 16]
		movaps xmm2, [esi + 32]
		movaps xmm3, [ebx + 0]
		movaps xmm4, [ebx + 16]
		movaps xmm5, [ebx + 32]

		movntps [edi + 0+96], xmm0
		movntps [edi + 16+96], xmm1
		movntps [edi + 32+96], xmm2
		movntps [edi + 48+96], xmm3
		movntps [edi + 64+96], xmm4
		movntps [edi + 80+96], xmm5

	}
#else
	Error( "Implement CMeshBuilder::Fast4VerticesSSE\n");
#endif
	IncrementFloatPointer( m_pCurrPosition, 4*m_VertexSize_Position );
}
#endif

#ifndef _XBOX
inline void CMeshBuilder::FastVertex( const ModelVertexDX8_t &vertex )
{
	Assert( m_CurrentVertex < m_MaxVertices );
	const void *pRead = &vertex;
 	void *pCurrPos = m_pCurrPosition;

#ifdef _WIN32
	__asm
	{
		mov esi, pRead
		mov edi, pCurrPos

		movq mm0, [esi + 0]
		movq mm1, [esi + 8]
		movq mm2, [esi + 16]
		movq mm3, [esi + 24]
		movq mm4, [esi + 32]
		movq mm5, [esi + 40]
		movq mm6, [esi + 48]
		movq mm7, [esi + 56]

		movntq [edi + 0], mm0
		movntq [edi + 8], mm1
		movntq [edi + 16], mm2
		movntq [edi + 24], mm3
		movntq [edi + 32], mm4
		movntq [edi + 40], mm5
		movntq [edi + 48], mm6
		movntq [edi + 56], mm7

		emms
	}
#else
	Error( "Implement CMeshBuilder::FastVertex(dx8)" );
#endif

	IncrementFloatPointer( m_pCurrPosition, m_VertexSize_Position );
//	m_NumVertices = ++m_CurrentVertex;
}
#endif

#ifndef _XBOX
inline void CMeshBuilder::FastVertexSSE( const ModelVertexDX8_t &vertex )
{
	Assert( m_CurrentVertex < m_MaxVertices );
	const void *pRead = &vertex;
	void *pCurrPos = m_pCurrPosition;

#ifdef _WIN32
	__asm
	{
		mov esi, pRead
		mov edi, pCurrPos

		movaps xmm0, [esi + 0]
		movaps xmm1, [esi + 16]
		movaps xmm2, [esi + 32]
		movaps xmm3, [esi + 48]

		movntps [edi + 0], xmm0
		movntps [edi + 16], xmm1
		movntps [edi + 32], xmm2
		movntps [edi + 48], xmm3
	}
#else
	Error( "Implement CMeshBuilder::FastVertexSSE((dx8)" );
#endif

	IncrementFloatPointer( m_pCurrPosition, m_VertexSize_Position );
//	m_NumVertices = ++m_CurrentVertex;
}
#endif

//-----------------------------------------------------------------------------
// Copies a formatted vertex into the xbox format
//-----------------------------------------------------------------------------
#ifdef _XBOX
inline void CMeshBuilder::VertexDX8ToXBox( const ModelVertexDX8_t &vertex )
{
	// xboxissue - the desire is to have the fastest source to destination copy
	// unpacked sources and packed targets prevents any SSE due to non-alignment
	// the sources *must* be changed to be in packed format
	Assert( m_CurrentVertex < m_MaxVertices );

	// get the start of the data
	unsigned char *pDst = (unsigned char*)m_pCurrPosition;

	memcpy( pDst, vertex.m_vecPosition.Base(), 3*sizeof(float) );
	pDst += 3*sizeof( float );

	if ( m_VertexSize_BoneWeight )
	{
		// pack bone weights
		Assert( vertex.m_flBoneWeights[0] >= 0 && vertex.m_flBoneWeights[0] <= 1.0f );
		Assert( vertex.m_flBoneWeights[1] >= 0 && vertex.m_flBoneWeights[1] <= 1.0f );
		((unsigned short*)pDst)[0] = Float2Int( vertex.m_flBoneWeights[0] * 32767.0f );
		((unsigned short*)pDst)[1] = Float2Int( vertex.m_flBoneWeights[1] * 32767.0f );
		pDst += 2*sizeof( unsigned short );

		if ( m_VertexSize_BoneMatrixIndex )
		{
			*(unsigned int*)pDst = vertex.m_nBoneIndices;
			pDst += sizeof( unsigned int );
		}
	}

	if ( m_VertexSize_Normal )
	{
		// pack normal
		PackNormal( (float*)vertex.m_vecNormal.Base(), (unsigned int*)pDst );
		pDst += sizeof( unsigned int );
	}

	if ( m_VertexSize_Color )
	{
		*(unsigned int*)pDst = vertex.m_nColor;
		pDst += sizeof( unsigned int );
	}

	if ( m_VertexSize_TexCoord[0] )
	{
		memcpy( pDst, vertex.m_vecTexCoord.Base(), 2*sizeof( float ) );
		pDst += 2*sizeof( float );
	}

	if ( m_VertexSize_UserData )
	{
		// pack user data
		// user data needs to be a valid normal, find source and fix
		Assert( vertex.m_vecUserData[0] >= -1.0f && vertex.m_vecUserData[0] <= 1.0f );
		Assert( vertex.m_vecUserData[1] >= -1.0f && vertex.m_vecUserData[1] <= 1.0f );
		Assert( vertex.m_vecUserData[2] >= -1.0f && vertex.m_vecUserData[2] <= 1.0f );
		// scale from [-1..1] to [0..255], fixup in shader
		pDst[0] = FastFToC( vertex.m_vecUserData[0]*0.5f + 0.5f );
		pDst[1] = FastFToC( vertex.m_vecUserData[1]*0.5f + 0.5f );
		pDst[2] = FastFToC( vertex.m_vecUserData[2]*0.5f + 0.5f );
		pDst[3] = vertex.m_vecUserData[3] < 0 ? 0 : 255;
		pDst += 4*sizeof( char );
	}

	// ensure code is synced with the mesh builder that established the offsets
	Assert( pDst - (unsigned char*)m_pCurrPosition == m_VertexSize_Position );

	IncrementFloatPointer( m_pCurrPosition, m_VertexSize_Position );
}
#endif

//-----------------------------------------------------------------------------
// Position setting methods
//-----------------------------------------------------------------------------
inline void	CMeshBuilder::Position3f( float x, float y, float z )
{
	Assert( m_pPosition && m_pCurrPosition );
	Assert( IsFinite(x) && IsFinite(y) && IsFinite(z) );
	float *pDst = m_pCurrPosition;
	*pDst++ = x;
	*pDst++ = y;
	*pDst = z;
}

inline void	CMeshBuilder::Position3fv( float const *v )
{
	Assert(v);
	Assert( m_pPosition && m_pCurrPosition );

	float *pDst = m_pCurrPosition;
	*pDst++ = *v++;
	*pDst++ = *v++;
	*pDst = *v;
}


//-----------------------------------------------------------------------------
// Normal setting methods
//-----------------------------------------------------------------------------
inline void	CMeshBuilder::Normal3f( float nx, float ny, float nz )
{
	Assert( m_pNormal );
	Assert( IsFinite(nx) && IsFinite(ny) && IsFinite(nz) );
	Assert( nx >= -1.05f && nx <= 1.05f );
	Assert( ny >= -1.05f && ny <= 1.05f );
	Assert( nz >= -1.05f && nz <= 1.05f );

#ifndef _XBOX
	float *pDst = m_pCurrNormal;
	*pDst++ = nx;
	*pDst++ = ny;
	*pDst = nz;
#else
	PackNormal( nx, ny, nz, (unsigned int*)m_pCurrNormal );
#endif
}

inline void	CMeshBuilder::Normal3fv( float const *n )
{
	Assert(n);
	Assert( m_pNormal && m_pCurrNormal );
	Assert( IsFinite(n[0]) && IsFinite(n[1]) && IsFinite(n[2]) );
	Assert( n[0] >= -1.05f && n[0] <= 1.05f );
	Assert( n[1] >= -1.05f && n[1] <= 1.05f );
	Assert( n[2] >= -1.05f && n[2] <= 1.05f );

#ifndef _XBOX
	float *pDst = m_pCurrNormal;
	*pDst++ = *n++;
	*pDst++ = *n++;
	*pDst = *n;
#else
	PackNormal( n[0], n[1], n[2], (unsigned int*)m_pCurrNormal );
#endif
}

inline void	CMeshBuilder::NormalDelta3f( float nx, float ny, float nz )
{
	Assert( m_pNormal );
	Assert( IsFinite(nx) && IsFinite(ny) && IsFinite(nz) );

	float *pDst = m_pCurrNormal;
	*pDst++ = nx;
	*pDst++ = ny;
	*pDst = nz;
}

inline void	CMeshBuilder::NormalDelta3fv( float const *n )
{
	Assert(n);
	Assert( m_pNormal && m_pCurrNormal );
	Assert( IsFinite(n[0]) && IsFinite(n[1]) && IsFinite(n[2]) );

	float *pDst = m_pCurrNormal;
	*pDst++ = *n++;
	*pDst++ = *n++;
	*pDst = *n;
}


//-----------------------------------------------------------------------------
// Color setting methods
//-----------------------------------------------------------------------------
inline void	CMeshBuilder::Color3f( float r, float g, float b )
{
	Assert( m_pColor && m_pCurrColor );
	Assert( IsFinite(r) && IsFinite(g) && IsFinite(b) );
	Assert( (r >= 0.0) && (g >= 0.0) && (b >= 0.0) );
	Assert( (r <= 1.0) && (g <= 1.0) && (b <= 1.0) );

	int col = (FastFToC(b)) | (FastFToC(g) << 8) | (FastFToC(r) << 16) | 0xFF000000;
	*(int*)m_pCurrColor = col;
}

inline void	CMeshBuilder::Color3fv( float const *rgb )
{
	Assert(rgb);
	Assert( m_pColor && m_pCurrColor );
	Assert( IsFinite(rgb[0]) && IsFinite(rgb[1]) && IsFinite(rgb[2]) );
	Assert( (rgb[0] >= 0.0) && (rgb[1] >= 0.0) && (rgb[2] >= 0.0) );
	Assert( (rgb[0] <= 1.0) && (rgb[1] <= 1.0) && (rgb[2] <= 1.0) );

	int col = (FastFToC(rgb[2])) | (FastFToC(rgb[1]) << 8) | (FastFToC(rgb[0]) << 16) | 0xFF000000;
	*(int*)m_pCurrColor = col;
}

inline void	CMeshBuilder::Color4f( float r, float g, float b, float a )
{
	Assert( m_pColor && m_pCurrColor );
	Assert( IsFinite(r) && IsFinite(g) && IsFinite(b) && IsFinite(a) );
	Assert( (r >= 0.0) && (g >= 0.0) && (b >= 0.0) && (a >= 0.0) );
	Assert( (r <= 1.0) && (g <= 1.0) && (b <= 1.0) && (a <= 1.0) );

	int col = (FastFToC(b)) | (FastFToC(g) << 8) | (FastFToC(r) << 16) | (FastFToC(a) << 24);
	*(int*)m_pCurrColor = col;
}

inline void	CMeshBuilder::Color4fv( float const *rgba )
{
	Assert(rgba);
	Assert( m_pColor && m_pCurrColor );
	Assert( IsFinite(rgba[0]) && IsFinite(rgba[1]) && IsFinite(rgba[2]) && IsFinite(rgba[3]) );
	Assert( (rgba[0] >= 0.0) && (rgba[1] >= 0.0) && (rgba[2] >= 0.0) && (rgba[3] >= 0.0) );
	Assert( (rgba[0] <= 1.0) && (rgba[1] <= 1.0) && (rgba[2] <= 1.0) && (rgba[3] <= 1.0) );

	int col = (FastFToC(rgba[2])) | (FastFToC(rgba[1]) << 8) | (FastFToC(rgba[0]) << 16) | (FastFToC(rgba[3]) << 24);
	*(int*)m_pCurrColor = col;
}


//-----------------------------------------------------------------------------
// Faster versions of color
//-----------------------------------------------------------------------------
inline void CMeshBuilder::Color3ub( unsigned char r, unsigned char g, unsigned char b )
{
	Assert( m_pColor && m_pCurrColor );
	int col = b | (g << 8) | (r << 16) | 0xFF000000;
	*(int*)m_pCurrColor = col;
}

inline void CMeshBuilder::Color3ubv( unsigned char const* rgb )
{
	Assert(rgb);
	Assert( m_pColor && m_pCurrColor );

	int col = rgb[2] | (rgb[1] << 8) | (rgb[0] << 16) | 0xFF000000;
	*(int*)m_pCurrColor = col;
}

inline void CMeshBuilder::Color4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a )
{
	Assert( m_pColor && m_pCurrColor );
	int col = b | (g << 8) | (r << 16) | (a << 24);
	*(int*)m_pCurrColor = col;
}

inline void CMeshBuilder::Color4ubv( unsigned char const* rgba )
{
	Assert( rgba );
	Assert( m_pColor && m_pCurrColor );
	int col = rgba[2] | (rgba[1] << 8) | (rgba[0] << 16) | (rgba[3] << 24);
	*(int*)m_pCurrColor = col;
}

inline void	CMeshBuilder::Specular3f( float r, float g, float b )
{
	Assert( m_pSpecular );
	Assert( IsFinite(r) && IsFinite(g) && IsFinite(b) );
	Assert( (r >= 0.0) && (g >= 0.0) && (b >= 0.0) );
	Assert( (r <= 1.0) && (g <= 1.0) && (b <= 1.0) );

	unsigned char* pSpecular = &m_pSpecular[m_CurrentVertex * m_VertexSize_Specular];
	int col = (FastFToC(b)) | (FastFToC(g) << 8) | (FastFToC(r) << 16) | 0xFF000000;
	*(int*)pSpecular = col;
}

inline void	CMeshBuilder::Specular3fv( float const *rgb )
{
	Assert(rgb);
	Assert( m_pSpecular );
	Assert( IsFinite(rgb[0]) && IsFinite(rgb[1]) && IsFinite(rgb[2]) );
	Assert( (rgb[0] >= 0.0) && (rgb[1] >= 0.0) && (rgb[2] >= 0.0) );
	Assert( (rgb[0] <= 1.0) && (rgb[1] <= 1.0) && (rgb[2] <= 1.0) );

	unsigned char* pSpecular = &m_pSpecular[m_CurrentVertex * m_VertexSize_Specular];
	int col = (FastFToC(rgb[2])) | (FastFToC(rgb[1]) << 8) | (FastFToC(rgb[0]) << 16) | 0xFF000000;
	*(int*)pSpecular = col;
}

inline void	CMeshBuilder::Specular4f( float r, float g, float b, float a )
{
	Assert( m_pSpecular );
	Assert( IsFinite(r) && IsFinite(g) && IsFinite(b) && IsFinite(a) );
	Assert( (r >= 0.0) && (g >= 0.0) && (b >= 0.0) && (a >= 0.0) );
	Assert( (r <= 1.0) && (g <= 1.0) && (b <= 1.0) && (a <= 1.0f) );

	unsigned char* pSpecular = &m_pSpecular[m_CurrentVertex * m_VertexSize_Specular];
	int col = (FastFToC(b)) | (FastFToC(g) << 8) | (FastFToC(r) << 16) | (FastFToC(a) << 24);
	*(int*)pSpecular = col;
}

inline void	CMeshBuilder::Specular4fv( float const *rgb )
{
	Assert(rgb);
	Assert( m_pSpecular );
	Assert( IsFinite(rgb[0]) && IsFinite(rgb[1]) && IsFinite(rgb[2]) && IsFinite(rgb[3]) );
	Assert( (rgb[0] >= 0.0) && (rgb[1] >= 0.0) && (rgb[2] >= 0.0) && (rgb[3] >= 0.0) );
	Assert( (rgb[0] <= 1.0) && (rgb[1] <= 1.0) && (rgb[2] <= 1.0) && (rgb[3] <= 1.0) );

	unsigned char* pSpecular = &m_pSpecular[m_CurrentVertex * m_VertexSize_Specular];
	int col = (FastFToC(rgb[2])) | (FastFToC(rgb[1]) << 8) | (FastFToC(rgb[0]) << 16) | (FastFToC(rgb[3]) << 24);
	*(int*)pSpecular = col;
}

inline void CMeshBuilder::Specular3ub( unsigned char r, unsigned char g, unsigned char b )
{
	Assert( m_pSpecular );
	unsigned char *pSpecular = &m_pSpecular[m_CurrentVertex * m_VertexSize_Specular];
	int col = b | (g << 8) | (r << 16) | 0xFF000000;
	*(int*)pSpecular = col;
}

inline void CMeshBuilder::Specular3ubv( unsigned char const *c )
{
	Assert( m_pSpecular );
	unsigned char *pSpecular = &m_pSpecular[m_CurrentVertex * m_VertexSize_Specular];
	int col = c[2] | (c[1] << 8) | (c[0] << 16) | 0xFF000000;
	*(int*)pSpecular = col;
}

inline void CMeshBuilder::Specular4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a )
{
	Assert( m_pSpecular );
	unsigned char *pSpecular = &m_pSpecular[m_CurrentVertex * m_VertexSize_Specular];
	int col = b | (g << 8) | (r << 16) | (a << 24);
	*(int*)pSpecular = col;
}

inline void CMeshBuilder::Specular4ubv( unsigned char const *c )
{
	Assert( m_pSpecular );
	unsigned char *pSpecular = &m_pSpecular[m_CurrentVertex * m_VertexSize_Specular];
	int col = c[2] | (c[1] << 8) | (c[0] << 16) | (c[3] << 24);
	*(int*)pSpecular = col;
}


//-----------------------------------------------------------------------------
// Texture coordinate setting methods
//-----------------------------------------------------------------------------
inline void CMeshBuilder::TexCoord1f( int stage, float s )
{
	Assert( m_pTexCoord[stage] && m_pCurrTexCoord[stage] );
	Assert( IsFinite(s) );

	float *pDst = m_pCurrTexCoord[stage];
	*pDst = s;
}

inline void	CMeshBuilder::TexCoord2f( int stage, float s, float t )
{
	Assert( m_pTexCoord[stage] && m_pCurrTexCoord[stage] );
	Assert( IsFinite(s) && IsFinite(t) );

	float *pDst = m_pCurrTexCoord[stage];
	*pDst++ = s;
	*pDst = t;
}

inline void	CMeshBuilder::TexCoord2fv( int stage, float const *st )
{
	Assert(st);
	Assert( m_pTexCoord[stage] && m_pCurrTexCoord[stage] );
	Assert( IsFinite(st[0]) && IsFinite(st[1]) );

	float *pDst = m_pCurrTexCoord[stage];
	*pDst++ = *st++;
	*pDst = *st;
}

inline void	CMeshBuilder::TexCoord3f( int stage, float s, float t, float u )
{
	// Tried to add too much!
	Assert( m_pTexCoord[stage] && m_pCurrTexCoord[stage] );
	Assert( IsFinite(s) && IsFinite(t)  && IsFinite(u) );
	float *pDst = m_pCurrTexCoord[stage];
	*pDst++ = s;
	*pDst++ = t;
	*pDst = u;
}

inline void	CMeshBuilder::TexCoord3fv( int stage, float const *stu )
{
	Assert(stu);
	Assert( m_pTexCoord[stage] && m_pCurrTexCoord[stage] );
	Assert( IsFinite(stu[0]) && IsFinite(stu[1]) && IsFinite(stu[2]) );

	float *pDst = m_pCurrTexCoord[stage];
	*pDst++ = *stu++;
	*pDst++ = *stu++;
	*pDst = *stu;
}

inline void CMeshBuilder::TexCoordSubRect2f( int stage, float s, float t, float offsetS, float offsetT, float scaleS, float scaleT )
{
	Assert( m_pTexCoord[stage] && m_pCurrTexCoord[stage] );
	Assert( IsFinite(s) && IsFinite(t) );

	float *pDst = m_pCurrTexCoord[stage];
	*pDst++ = ( s * scaleS ) + offsetS;
	*pDst = ( t * scaleT ) + offsetT;
}

inline void CMeshBuilder::TexCoordSubRect2fv( int stage, const float *st, const float *offset, const float *scale )
{
	Assert(st);
	Assert( m_pTexCoord[stage] && m_pCurrTexCoord[stage] );
	Assert( IsFinite(st[0]) && IsFinite(st[1]) );

	float *pDst = m_pCurrTexCoord[stage];
	*pDst++ = ( *st++ * *scale++ ) + *offset++;
	*pDst = ( *st * *scale ) + *offset;
}

//-----------------------------------------------------------------------------
// Tangent space setting methods
//-----------------------------------------------------------------------------

inline void CMeshBuilder::TangentS3f( float sx, float sy, float sz )
{
	Assert( m_pTangentS );
	Assert( IsFinite(sx) && IsFinite(sy) && IsFinite(sz) );

#ifndef _XBOX
	float* pTangentS = OffsetFloatPointer( m_pTangentS, m_CurrentVertex, m_VertexSize_TangentS );
	*pTangentS++ = sx;
	*pTangentS++ = sy;
	*pTangentS = sz;
#else
	unsigned char *pTangentS = m_pTangentS + m_CurrentVertex * m_VertexSize_TangentS;
	PackNormal( sx, sy, sz, (unsigned int*)pTangentS );
#endif
}

inline void CMeshBuilder::TangentS3fv( float const* s )
{
	Assert( s );
	Assert( m_pTangentS );
	Assert( IsFinite(s[0]) && IsFinite(s[1]) && IsFinite(s[2]) );

#ifndef _XBOX
	float* pTangentS = OffsetFloatPointer( m_pTangentS, m_CurrentVertex, m_VertexSize_TangentS );
	*pTangentS++ = *s++;
	*pTangentS++ = *s++;
	*pTangentS = *s;
#else
	unsigned char *pTangentS = m_pTangentS + m_CurrentVertex * m_VertexSize_TangentS;
	PackNormal( s[0], s[1], s[2], (unsigned int*)pTangentS );
#endif
}

inline void CMeshBuilder::TangentT3f( float tx, float ty, float tz )
{
	Assert( m_pTangentT );
	Assert( IsFinite(tx) && IsFinite(ty) && IsFinite(tz) );

#ifndef _XBOX
	float* pTangentT = OffsetFloatPointer( m_pTangentT, m_CurrentVertex, m_VertexSize_TangentT );
	*pTangentT++ = tx;
	*pTangentT++ = ty;
	*pTangentT = tz;
#else
	unsigned char *pTangentT = m_pTangentT + m_CurrentVertex * m_VertexSize_TangentT;
	PackNormal( tx, ty, tz, (unsigned int*)pTangentT );
#endif
}

inline void CMeshBuilder::TangentT3fv( float const* t )
{
	Assert( t );
	Assert( m_pTangentT );
	Assert( IsFinite(t[0]) && IsFinite(t[1]) && IsFinite(t[2]) );

#ifndef _XBOX
	float* pTangentT = OffsetFloatPointer( m_pTangentT, m_CurrentVertex, m_VertexSize_TangentT );
	*pTangentT++ = *t++;
	*pTangentT++ = *t++;
	*pTangentT = *t;
#else
	unsigned char *pTangentT = m_pTangentT + m_CurrentVertex * m_VertexSize_TangentT;
	PackNormal( t[0], t[1], t[2], (unsigned int*)pTangentT );
#endif
}

#ifndef _XBOX
inline void CMeshBuilder::TangentSxT3f( float sxtx, float sxty, float sxtz )
{
	Assert( m_pTangentSxT );
	Assert( IsFinite(sxtx) && IsFinite(sxty) && IsFinite(sxtz) );

	float* pTangentSxT = OffsetFloatPointer( m_pTangentSxT, m_CurrentVertex, m_VertexSize_TangentSxT );
	*pTangentSxT++ = sxtx;
	*pTangentSxT++ = sxty;
	*pTangentSxT = sxtz;
}
#endif

#ifndef _XBOX
inline void CMeshBuilder::TangentSxT3fv( float const* sxt )
{
	Assert( sxt );
	Assert( m_pTangentSxT );
	Assert( IsFinite(sxt[0]) && IsFinite(sxt[1]) && IsFinite(sxt[2]) );

	float* pTangentSxT = OffsetFloatPointer( m_pTangentSxT, m_CurrentVertex, m_VertexSize_TangentSxT );
	*pTangentSxT++ = *sxt++;
	*pTangentSxT++ = *sxt++;
	*pTangentSxT = *sxt;
}
#endif

//-----------------------------------------------------------------------------
// Bone weight setting methods
//-----------------------------------------------------------------------------

inline void CMeshBuilder::BoneWeight( int idx, float weight )
{
	Assert( m_pBoneWeight );
	Assert( IsFinite( weight ) );
	Assert( idx >= 0 );

	// This test is here because on fixed function, we store n-1
	// bone weights, but on vertex shaders, we store all n
	if ( idx < m_NumBoneWeights )
	{
#ifndef _XBOX
		float* pBoneWeight = OffsetFloatPointer( m_pBoneWeight, m_CurrentVertex, m_VertexSize_BoneWeight );
		pBoneWeight[idx] = weight;
#else
		Assert( weight >= 0 && weight <= 1.0f );
		unsigned short* pBoneWeight = (unsigned short*)(m_pBoneWeight + m_CurrentVertex * m_VertexSize_BoneWeight);
		pBoneWeight[idx] = Float2Int( weight * 32767.0f ); 
#endif
	}
}

inline void CMeshBuilder::BoneMatrix( int idx, int matrixIdx )
{
	Assert( m_pBoneMatrixIndex );
	Assert( idx >= 0 );
	Assert( idx < 4 );

	// garymcthack
	if ( matrixIdx == BONE_MATRIX_INDEX_INVALID )
	{
		matrixIdx = 0;
	}
	Assert( (matrixIdx >= 0) && (matrixIdx < 53) );
	
#ifndef NEW_SKINNING
	unsigned char* pBoneMatrix = &m_pBoneMatrixIndex[m_CurrentVertex * m_VertexSize_BoneMatrixIndex];
	pBoneMatrix[idx] = (unsigned char)matrixIdx;
#else
	float* pBoneMatrix = &m_pBoneMatrixIndex[m_CurrentVertex * m_VertexSize_BoneMatrixIndex];
	pBoneMatrix[idx] = matrixIdx;
#endif
}


//-----------------------------------------------------------------------------
// Generic per-vertex data setting method
//-----------------------------------------------------------------------------
inline void CMeshBuilder::UserData( float const* pData )
{
	Assert( pData );
#ifndef _XBOX
	int userDataSize = 4; // garymcthack
	float *pUserData = OffsetFloatPointer( m_pUserData, m_CurrentVertex, m_VertexSize_UserData );
	memcpy( pUserData, pData, sizeof( float ) * userDataSize );
#else
	// pack user data
	unsigned char *pUserData = (unsigned char*)(m_pUserData + m_CurrentVertex * m_VertexSize_UserData);

	// user data needs to be a valid normal, find source and fix
	Assert( pData[0] >= -1.0f && pData[0] <= 1.0f );
	Assert( pData[1] >= -1.0f && pData[1] <= 1.0f );
	Assert( pData[2] >= -1.0f && pData[2] <= 1.0f );

	// scale from [-1..1] to [0..255], fixup in shader
	pUserData[0] = FastFToC( pData[0]*0.5f + 0.5f );
	pUserData[1] = FastFToC( pData[1]*0.5f + 0.5f );
	pUserData[2] = FastFToC( pData[2]*0.5f + 0.5f );
	pUserData[3] = pData[3] < 0 ? 0 : 255;
#endif
}

#endif // IVERTEXBUFFER_H
