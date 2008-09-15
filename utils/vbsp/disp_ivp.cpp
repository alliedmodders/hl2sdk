//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//
#include "vbsp.h"
#include "disp_vbsp.h"
#include "builddisp.h"
#include "disp_common.h"
#include "ivp.h"
#include "disp_ivp.h"
#include "vphysics_interface.h"
#include "utlrbtree.h"


struct disp_grid_t
{
	int				gridIndex;
	CUtlVector<int> dispList;
};

static CUtlVector<disp_grid_t> gDispGridList;



disp_grid_t &FindOrInsertGrid( int gridIndex )
{
	// linear search is slow, but only a few grids will be present
	for ( int i = gDispGridList.Count()-1; i >= 0; i-- )
	{
		if ( gDispGridList[i].gridIndex == gridIndex )
		{
			return gDispGridList[i];
		}
	}
	int index = gDispGridList.AddToTail();
	gDispGridList[index].gridIndex = gridIndex;

	// must be empty
	Assert( gDispGridList[index].dispList.Count() == 0 );

	return gDispGridList[index];
}

// UNDONE: Tune these or adapt them to map size or triangle count?
#define DISP_GRID_SIZEX	4096
#define DISP_GRID_SIZEY	4096
#define DISP_GRID_SIZEZ 8192

int Disp_GridIndex( CCoreDispInfo *pDispInfo )
{
	// quick hash the center into the grid and put the whole terrain in that grid
	Vector mins, maxs;
	pDispInfo->GetNode(0)->GetBoundingBox( mins, maxs );
	Vector center;
	center = 0.5 * (mins + maxs);
	// make sure it's positive
	center += Vector(MAX_COORD_INTEGER,MAX_COORD_INTEGER,MAX_COORD_INTEGER);
	int gridX = center.x / DISP_GRID_SIZEX;
	int gridY = center.y / DISP_GRID_SIZEY;
	int gridZ = center.z / DISP_GRID_SIZEZ;

	gridX &= 0xFF;
	gridY &= 0xFF;
	gridZ &= 0xFF;
	return MAKEID( gridX, gridY, gridZ, 0 );
}

void AddToGrid( int gridIndex, int dispIndex )
{
	disp_grid_t &grid = FindOrInsertGrid( gridIndex );
	grid.dispList.AddToTail( dispIndex );
}

MaterialSystemMaterial_t GetMatIDFromDisp( mapdispinfo_t *pMapDisp )
{
	texinfo_t *pTexInfo = &texinfo[pMapDisp->face.texinfo];
	dtexdata_t *pTexData = GetTexData( pTexInfo->texdata );
	MaterialSystemMaterial_t matID = FindOriginalMaterial( TexDataStringTable_GetString( pTexData->nameStringTableID ), NULL, true );
	return matID;
}

// adds all displacement faces as a series of convex objects
// UNDONE: Only add the displacements for this model?
void Disp_AddCollisionModels( CUtlVector<CPhysCollisionEntry *> &collisionList, dmodel_t *pModel, int contentsMask)
{
	int dispIndex;

	// Add each displacement to the grid hash
	for ( dispIndex = 0; dispIndex < g_CoreDispInfos.Count(); dispIndex++ )	
	{
		CCoreDispInfo *pDispInfo = g_CoreDispInfos[ dispIndex ];
		mapdispinfo_t *pMapDisp = &mapdispinfo[ dispIndex ];

		// not solid for this pass
		if ( !(pMapDisp->contents & contentsMask) )
			continue;

		int gridIndex = Disp_GridIndex( pDispInfo );
		AddToGrid( gridIndex, dispIndex );
	}

	// now make a polysoup for the terrain in each grid
	for ( int grid = 0; grid < gDispGridList.Count(); grid++ )
	{
		int triCount = 0;
		CPhysPolysoup *pTerrainPhysics = physcollision->PolysoupCreate();

		// iterate the displacements in this grid
		for ( int listIndex = 0; listIndex < gDispGridList[grid].dispList.Count(); listIndex++ )
		{
			dispIndex = gDispGridList[grid].dispList[listIndex];
			CCoreDispInfo *pDispInfo = g_CoreDispInfos[ dispIndex ];
			mapdispinfo_t *pMapDisp = &mapdispinfo[ dispIndex ];

			// Get the material id.
			MaterialSystemMaterial_t matID = GetMatIDFromDisp( pMapDisp );

			// Build a triangle list. This shares the tesselation code with the engine.
			CUtlVector<unsigned short> indices;
			CVBSPTesselateHelper helper;
			helper.m_pIndices = &indices;
			helper.m_pActiveVerts = pDispInfo->GetAllowedVerts().Base();
			helper.m_pPowerInfo = pDispInfo->GetPowerInfo();

			::TesselateDisplacement( &helper );

			Assert( indices.Count() > 0 );
			Assert( indices.Count() % 3 == 0 );	// Make sure indices are a multiple of 3.
			int nTriCount = indices.Count() / 3;
			triCount += nTriCount;
			if ( triCount >= 65536 )
			{
				// don't put more than 64K tris in any single collision model
				CPhysCollide *pCollide = physcollision->ConvertPolysoupToCollide( pTerrainPhysics, false );
				if ( pCollide )
				{
					collisionList.AddToTail( new CPhysCollisionEntryStaticMesh( pCollide, NULL ) );	
				}
				// Throw this polysoup away and start over for the remaining triangles
				physcollision->PolysoupDestroy( pTerrainPhysics );
				pTerrainPhysics = physcollision->PolysoupCreate();
				triCount = nTriCount;
			}
			Vector tmpVerts[3];
			for ( int iTri = 0; iTri < nTriCount; ++iTri )
			{
				float flAlphaTotal = 0.0f;
				for ( int iTriVert = 0; iTriVert < 3; ++iTriVert )
				{
					pDispInfo->GetVert( indices[iTri*3+iTriVert], tmpVerts[iTriVert] );
					flAlphaTotal += pDispInfo->GetAlpha( indices[iTri*3+iTriVert] );
				}

				int nProp = g_SurfaceProperties[texinfo[pMapDisp->face.texinfo].texdata];
				if ( flAlphaTotal > DISP_ALPHA_PROP_DELTA )
				{
					int nProp2 = GetSurfaceProperties2( matID, "surfaceprop2" );
					if ( nProp2 != -1 )
					{
						nProp = nProp2;
					}
				}
				int nMaterialIndex = RemapWorldMaterial( nProp );
				physcollision->PolysoupAddTriangle( pTerrainPhysics, tmpVerts[0], tmpVerts[1], tmpVerts[2], nMaterialIndex );
			}
		}

		// convert the whole grid's polysoup to a collide and store in the collision list
		CPhysCollide *pCollide = physcollision->ConvertPolysoupToCollide( pTerrainPhysics, false );
		if ( pCollide )
		{
			collisionList.AddToTail( new CPhysCollisionEntryStaticMesh( pCollide, NULL ) );	
		}
		// now that we have the collide, we're done with the soup
		physcollision->PolysoupDestroy( pTerrainPhysics );
	}
}

