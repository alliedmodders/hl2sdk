//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//


//
// studiomdl.c: generates a studio .mdl file from a .qc script
// models/<scriptname>.mdl.
//


#pragma warning( disable : 4244 )
#pragma warning( disable : 4237 )
#pragma warning( disable : 4305 )


#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <math.h>

#include "cmdlib.h"
#include "scriplib.h"
#include "mathlib.h"
#include "studio.h"
#include "studiomdl.h"
//#include "..\..\dlls\activity.h"

bool IsEnd( char const* pLine );
int SortAndBalanceBones( int iCount, int iMaxCount, int bones[], float weights[] );

int AddToVlist( int v, int m, int n, int t, int firstref );
void DecrementReferenceVlist( int uv, int numverts );
int faceCompare( const void *elem1, const void *elem2 );


void UnifyIndices( s_source_t *psource );
void BuildIndividualMeshes( s_source_t *psource );


int Load_OBJ( s_source_t *psource )
{
	char	cmd[1024];
	int		i, j;
	int		material = 0;

	if (!OpenGlobalFile( psource->filename ))
	{	
		return 0;
	}

	if( !g_quiet )
	{
		printf ("grabbing %s\n", psource->filename);
	}

	g_iLinecount = 0;

	psource->numbones = 1;
	strcpy( psource->localBone[0].name, "default" );
	psource->localBone[0].parent = -1;

	psource->numframes = 1;
	psource->startframe = 0;
	psource->endframe = 0;
	psource->rawanim[0] = (s_bone_t *)kalloc( 1, sizeof( s_bone_t ) );
	psource->rawanim[0][0].pos.Init();
	psource->rawanim[0][0].rot.Init();
	Build_Reference( psource );

	while (fgets( g_szLine, sizeof( g_szLine ), g_fpInput ) != NULL) {
		g_iLinecount++;
		Vector tmp;

		if (strncmp( g_szLine, "v ", 2 ) == 0)
		{
			i = g_numverts++;

			sscanf( g_szLine, "v %f %f %f", &g_vertex[i].x, &g_vertex[i].y, &g_vertex[i].z );
			g_bone[i].numbones = 1;
			g_bone[i].bone[0] = 0;
			g_bone[i].weight[0] = 1.0;
		}
		else if (strncmp( g_szLine, "vn ", 3 ) == 0)
		{
			i = g_numnormals++;
			sscanf( g_szLine, "vn %f %f %f", &g_normal[i].x, &g_normal[i].y, &g_normal[i].z );
		}
		else if (strncmp( g_szLine, "vt ", 3 ) == 0)
		{
			i = g_numtexcoords++;
			sscanf( g_szLine, "vt %f %f", &g_texcoord[i].x, &g_texcoord[i].y );
			g_texcoord[i].y = 1.0 - g_texcoord[i].y;

		}
		else if (strncmp( g_szLine, "usemtl ", 7 ) == 0)
		{
			sscanf( g_szLine, "usemtl %s", &cmd );

			int texture = lookup_texture( cmd, sizeof( cmd ) );
			psource->texmap[texture] = texture;	// hack, make it 1:1
			material = use_texture_as_material( texture );
		}
		else if (strncmp( g_szLine, "f ", 2 ) == 0)
		{
			int v0, n0, t0;
			int v1, n1, t1;
			int v2, n2, t2;
			int v3, n3, t3;

			s_tmpface_t f;

			i = g_numfaces++;

			j = sscanf( g_szLine, "f %d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d", &v0, &t0, &n0, &v1, &t1, &n1, &v2, &t2, &n2, &v3, &t3, &n3 );

			f.material = material;
			f.a = v0 - 1; f.na = n0 - 1, f.ta = t0 - 1;
			f.b = v2 - 1; f.nb = n2 - 1, f.tb = t2 - 1;
			f.c = v1 - 1; f.nc = n1 - 1, f.tc = t1 - 1;

			Assert( v0 <= g_numverts && v1 <= g_numverts && v2 <= g_numverts );
			Assert( n0 <= g_numnormals && n1 <= g_numnormals && n2 <= g_numnormals );
			Assert( t0 <= g_numtexcoords && t1 <= g_numtexcoords && t2 <= g_numtexcoords );

			g_face[i] = f;

			if (j == 12)
			{
				i = g_numfaces++;
				f.a = v0 - 1; f.na = n0 - 1, f.ta = t0 - 1;
				f.b = v3 - 1; f.nb = n3 - 1, f.tb = t3 - 1;
				f.c = v2 - 1; f.nc = n2 - 1, f.tc = t2 - 1;
				g_face[i] = f;
			}
		}
	}

	UnifyIndices( psource );

	BuildIndividualMeshes( psource );

	fclose( g_fpInput );

	return 1;
}



int AppendVTAtoOBJ( s_source_t *psource, char *filename, int frame )
{
	char	cmd[1024];
	int		i, j;
	int		material = 0;

	Vector tmp;
	matrix3x4_t m;

	AngleMatrix( RadianEuler( 1.570796, 0, 0 ), m );

	if (!OpenGlobalFile( filename ))
	{	
		return 0;
	}

	if( !g_quiet )
	{
		printf ("grabbing %s\n", filename );
	}

	g_iLinecount = 0;

	g_numverts = g_numnormals = g_numtexcoords = g_numfaces = 0;

	while (fgets( g_szLine, sizeof( g_szLine ), g_fpInput ) != NULL) {
		g_iLinecount++;
		Vector tmp;

		if (strncmp( g_szLine, "v ", 2 ) == 0)
		{
			i = g_numverts++;

			sscanf( g_szLine, "v %f %f %f", &tmp.x, &tmp.y, &tmp.z );
			VectorTransform( tmp, m, g_vertex[i] );

			// printf("%f %f %f\n", g_vertex[i].x, g_vertex[i].y, g_vertex[i].z );

			g_bone[i].numbones = 1;
			g_bone[i].bone[0] = 0;
			g_bone[i].weight[0] = 1.0;
		}
		else if (strncmp( g_szLine, "vn ", 3 ) == 0)
		{
			i = g_numnormals++;
			sscanf( g_szLine, "vn %f %f %f", &tmp.x, &tmp.y, &tmp.z );
			VectorTransform( tmp, m, g_normal[i] );
		}
		else if (strncmp( g_szLine, "vt ", 3 ) == 0)
		{
			i = g_numtexcoords++;
			sscanf( g_szLine, "vt %f %f", &g_texcoord[i].x, &g_texcoord[i].y );
		}
		else if (strncmp( g_szLine, "usemtl ", 7 ) == 0)
		{
			sscanf( g_szLine, "usemtl %s", &cmd );

			int texture = lookup_texture( cmd, sizeof( cmd ) );
			psource->texmap[texture] = texture;	// hack, make it 1:1
			material = use_texture_as_material( texture );
		}
		else if (strncmp( g_szLine, "f ", 2 ) == 0)
		{
			int v0, n0, t0;
			int v1, n1, t1;
			int v2, n2, t2;
			int v3, n3, t3;

			s_tmpface_t f;

			i = g_numfaces++;

			j = sscanf( g_szLine, "f %d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d", &v0, &t0, &n0, &v1, &t1, &n1, &v2, &t2, &n2, &v3, &t3, &n3 );

			f.material = material;
			f.a = v0 - 1; f.na = n0 - 1, f.ta = 0;
			f.b = v2 - 1; f.nb = n2 - 1, f.tb = 0;
			f.c = v1 - 1; f.nc = n1 - 1, f.tc = 0;

			Assert( v0 <= g_numverts && v1 <= g_numverts && v2 <= g_numverts );
			Assert( n0 <= g_numnormals && n1 <= g_numnormals && n2 <= g_numnormals );

			g_face[i] = f;

			if (j == 12)
			{
				i = g_numfaces++;
				f.a = v0 - 1; f.na = n0 - 1, f.ta = 0;
				f.b = v3 - 1; f.nb = n3 - 1, f.tb = 0;
				f.c = v2 - 1; f.nc = n2 - 1, f.tc = 0;
				g_face[i] = f;
			}
		}
	}


	UnifyIndices( psource );

	if (frame == 0)
	{
		psource->numbones = 1;
		strcpy( psource->localBone[0].name, "default" );
		psource->localBone[0].parent = -1;

		psource->numframes = 1;
		psource->startframe = 0;
		psource->endframe = 0;
		psource->rawanim[0] = (s_bone_t *)kalloc( 1, sizeof( s_bone_t ) );
		psource->rawanim[0][0].pos.Init();
		psource->rawanim[0][0].rot = RadianEuler( 1.570796, 0.0, 0.0 );
		Build_Reference( psource );

		BuildIndividualMeshes( psource );
	}

	// printf("%d %d : %d\n", g_numverts, g_numnormals, numvlist );

	int t = frame;
	int count = numvlist;

	psource->numvanims[t] = count;
	psource->vanim[t] = (s_vertanim_t *)kalloc( count, sizeof( s_vertanim_t ) );
	for (i = 0; i < count; i++)
	{
		psource->vanim[t][i].vertex = i;
		psource->vanim[t][i].pos = g_vertex[v_listdata[i].v];
		psource->vanim[t][i].normal = g_normal[v_listdata[i].n];
	}

	fclose( g_fpInput );

	return 1;
}
