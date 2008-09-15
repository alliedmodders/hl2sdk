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

bool IsEnd( char const* pLine )
{
	if (strncmp( "end", pLine, 3 ) != 0) 
		return false;
	return (pLine[3] == '\0') || (pLine[3] == '\n');
}


int SortAndBalanceBones( int iCount, int iMaxCount, int bones[], float weights[] )
{
	int i;

	// collapse duplicate bone weights
	for (i = 0; i < iCount-1; i++)
	{
		int j;
		for (j = i + 1; j < iCount; j++)
		{
			if (bones[i] == bones[j])
			{
				weights[i] += weights[j];
				weights[j] = 0.0;
			}
		}
	}

	// do sleazy bubble sort
	int bShouldSort;
	do {
		bShouldSort = false;
		for (i = 0; i < iCount-1; i++)
		{
			if (weights[i+1] > weights[i])
			{
				int j = bones[i+1]; bones[i+1] = bones[i]; bones[i] = j;
				float w = weights[i+1]; weights[i+1] = weights[i]; weights[i] = w;
				bShouldSort = true;
			}
		}
	} while (bShouldSort);

	// throw away all weights less than 1/20th
	while (iCount > 1 && weights[iCount-1] < 0.05)
	{
		iCount--;
	}

	// clip to the top iMaxCount bones
	if (iCount > iMaxCount)
	{
		iCount = iMaxCount;
	}

	float t = 0;
	for (i = 0; i < iCount; i++)
	{
		t += weights[i];
	}

	if (t <= 0.0)
	{
		// missing weights?, go ahead and evenly share?
		// FIXME: shouldn't this error out?
		t = 1.0 / iCount;

		for (i = 0; i < iCount; i++)
		{
			weights[i] = t;
		}
	}
	else
	{
		// scale to sum to 1.0
		t = 1.0 / t;

		for (i = 0; i < iCount; i++)
		{
			weights[i] = weights[i] * t;
		}
	}

	return iCount;
}



void Grab_Vertexlist( s_source_t *psource )
{
	while (1) 
	{
		if (fgets( g_szLine, sizeof( g_szLine ), g_fpInput ) != NULL) 
		{
			int j;
			int bone;
			Vector p;
			int		iCount, bones[4];
			float   weights[4];

			g_iLinecount++;

			// check for end
			if (IsEnd(g_szLine)) 
				return;


			int i = sscanf( g_szLine, "%d %d %f %f %f %d %d %f %d %f %d %f %d %f",
				&j, 
				&bone, 
				&p[0], &p[1], &p[2],
				&iCount,
				&bones[0], &weights[0], &bones[1], &weights[1], &bones[2], &weights[2], &bones[3], &weights[3] );
			
			if (i == 5)
			{
				if (bone < 0 || bone >= psource->numbones) 
				{
					MdlWarning( "bogus bone index\n" );
					MdlWarning( "%d %s :\n%s", g_iLinecount, g_szFilename, g_szLine );
					MdlError( "Exiting due to errors\n" );
				}

				VectorCopy( p, g_vertex[j] );
				g_bone[j].numbones = 1;
				g_bone[j].bone[0] = bone;
				g_bone[j].weight[0] = 1.0;
			} 
			else if (i > 5)
			{
				iCount = SortAndBalanceBones( iCount, MAXSTUDIOBONEWEIGHTS, bones, weights );

				VectorCopy( p, g_vertex[j] );
				g_bone[j].numbones = iCount;
				for (i = 0; i < iCount; i++)
				{
					g_bone[j].bone[i] = bones[i];
					g_bone[j].weight[i] = weights[i];
				}
			}
			else 
			{
				MdlError("%s: error on line %d: %s", g_szFilename, g_iLinecount, g_szLine );
			}
		}
	}
}



void Grab_Facelist( s_source_t *psource )
{
	while (1) 
	{
		if (fgets( g_szLine, sizeof( g_szLine ), g_fpInput ) != NULL) 
		{
			int j;
			s_tmpface_t f;

			g_iLinecount++;

			// check for end
			if (IsEnd(g_szLine)) 
				return;

			if (sscanf( g_szLine, "%d %d %d %d",
				&j, 
				&f.a, &f.b, &f.c) == 4)
			{
				g_face[j] = f;
			}
			else 
			{
				MdlError("%s: error on line %d: %s", g_szFilename, g_iLinecount, g_szLine );
			}
		}
	}
}



void Grab_Materiallist( s_source_t *psource )
{
	while (1) 
	{
		if (fgets( g_szLine, sizeof( g_szLine ), g_fpInput ) != NULL) 
		{
			// char name[256];
			char path[256];
			rgb2_t a, d, s;
			float g;
			int j;

			g_iLinecount++;

			// check for end
			if (IsEnd(g_szLine)) 
				return;

			if (sscanf( g_szLine, "%d  %f %f %f %f   %f %f %f %f  %f %f %f %f  %f \"%[^\"]s", 
				&j, 
				&a.r, &a.g, &a.b, &a.a,
				&d.r, &d.g, &d.b, &d.a,
				&s.r, &s.g, &s.b, &s.a,
				&g,
				path ) == 15)
			{
				if (path[0] == '\0')
					psource->texmap[j] = -1;
				else if (j < sizeof(psource->texmap))
					psource->texmap[j] = lookup_texture( path, sizeof( path ) );
				else
					MdlError( "Too many materials, max %d\n", sizeof(psource->texmap) );
			}
		}
	}
}


void Grab_Texcoordlist( s_source_t *psource )
{
	while (1) 
	{
		if (fgets( g_szLine, sizeof( g_szLine ), g_fpInput ) != NULL) 
		{
			int j;
			Vector2D t;

			g_iLinecount++;

			// check for end
			if (IsEnd(g_szLine)) 
				return;

			if (sscanf( g_szLine, "%d %f %f",
				&j, 
				&t[0], &t[1]) == 3)
			{
				t[1] = 1.0 - t[1];
				g_texcoord[j][0] = t[0];
				g_texcoord[j][1] = t[1];
			}
			else 
			{
				MdlError("%s: error on line %d: %s", g_szFilename, g_iLinecount, g_szLine );
			}
		}
	}
}





void Grab_Normallist( s_source_t *psource )
{
	while (1) 
	{
		if (fgets( g_szLine, sizeof( g_szLine ), g_fpInput ) != NULL) 
		{
			int j;
			int bone;
			Vector n;

			g_iLinecount++;

			// check for end
			if (IsEnd(g_szLine)) 
				return;


			if (sscanf( g_szLine, "%d %d %f %f %f",
				&j, 
				&bone, 
				&n[0], &n[1], &n[2]) == 5)
			{
				if (bone < 0 || bone >= psource->numbones) 
				{
					MdlWarning( "bogus bone index\n" );
					MdlWarning( "%d %s :\n%s", g_iLinecount, g_szFilename, g_szLine );
					MdlError( "Exiting due to errors\n" );
				}

				VectorCopy( n, g_normal[j] );
			}
			else 
			{
				MdlError("%s: error on line %d: %s", g_szFilename, g_iLinecount, g_szLine );
			}
		}
	}
}



void Grab_Faceattriblist( s_source_t *psource )
{
	while (1) 
	{
		if (fgets( g_szLine, sizeof( g_szLine ), g_fpInput ) != NULL) 
		{
			int j;
			int smooth;
			int material;
			s_tmpface_t f;
			unsigned short s;

			g_iLinecount++;

			// check for end
			if (IsEnd(g_szLine)) 
				return;

			if (sscanf( g_szLine, "%d %d %d %d %d %d %d %d %d",
				&j, 
				&material,
				&smooth,
				&f.ta, &f.tb, &f.tc,
				&f.na, &f.nb, &f.nc) == 9)
			{
				f.a = g_face[j].a;
				f.b = g_face[j].b;
				f.c = g_face[j].c;

				f.material = use_texture_as_material( psource->texmap[material] );
				if (f.material < 0)
					MdlError( "face %d references NULL texture %d\n", j, material );
				
				if (flip_triangles)
				{
					s = f.b;  f.b  = f.c;  f.c  = s;
					s = f.tb; f.tb = f.tc; f.tc = s;
					s = f.nb; f.nb = f.nc; f.nc = s;
				}

				g_face[j] = f;
			}
			else 
			{
				MdlError("%s: error on line %d: %s", g_szFilename, g_iLinecount, g_szLine );
			}
		}
	}
}


int closestNormal( int v, int n )
{
	float maxdot = -1.0;
	float dot;
	int	r = n;

	v_unify_t *cur = v_list[v];

	while (cur)
	{
		dot = DotProduct( g_normal[cur->n], g_normal[n] );
		if (dot > maxdot)
		{
			r = cur->n;
			maxdot = dot;
		}
		cur = cur->next;
	}
	
	return r;
}


int vlistCompare( const void *elem1, const void *elem2 )
{
	v_unify_t *u1 = &v_listdata[*(int *)elem1];
	v_unify_t *u2 = &v_listdata[*(int *)elem2];

	// sort by material
	if (u1->m < u2->m)
		return -1;
	if (u1->m > u2->m)
		return 1;

	// sort by last used
	if (u1->lastref < u2->lastref)
		return -1;
	if (u1->lastref > u2->lastref)
		return 1;

	return 0;
}



int AddToVlist( int v, int m, int n, int t, int firstref )
{
	v_unify_t *prev = NULL;
	v_unify_t *cur = v_list[v];

	while (cur)
	{
		if (cur->m == m && cur->n == n && cur->t == t)
		{
			cur->refcount++;
			return cur - v_listdata;
		}
		prev = cur;
		cur = cur->next;
	}

	if (numvlist >= MAXSTUDIOVERTS)
	{
		MdlError( "Too many unified vertices\n");
	}

	cur = &v_listdata[numvlist++];
	cur->lastref = -1;
	cur->refcount = 1;
	cur->firstref = firstref;
	cur->v = v;
	cur->m = m;
	cur->n = n;
	cur->t = t;

	if (prev)
	{
		prev->next = cur;
	}
	else
	{
		v_list[v] = cur;
	}

	return numvlist - 1;
}

void DecrementReferenceVlist( int uv, int numverts )
{
	if (uv < 0 || uv > MAXSTUDIOVERTS)
		MdlError( "decrement outside of range\n");

	v_listdata[uv].refcount--;

	if (v_listdata[uv].refcount == 0)
	{
		v_listdata[uv].lastref = numverts;
	}
	else if (v_listdata[uv].refcount < 0)
	{
		MdlError("<0 ref\n");
	}
}


int faceCompare( const void *elem1, const void *elem2 )
{
	int i1 = *(int *)elem1;
	int i2 = *(int *)elem2;

	// sort by material
	if (g_face[i1].material < g_face[i2].material)
		return -1;
	if (g_face[i1].material > g_face[i2].material)
		return 1;

	// sort by original usage
	if (i1 < i2)
		return -1;
	if (i1 > i2)
		return 1;

	return 0;
}

void UnifyIndices( s_source_t *psource )
{
	int i;

	static s_tmpface_t		tmpface[MAXSTUDIOTRIANGLES];	// mrm processed g_face
	static s_face_t			uface[MAXSTUDIOTRIANGLES];		// mrm processed unified face

	// clear v_list
	numvlist = 0;
	memset( v_list, 0, sizeof( v_list ) );
	memset( v_listdata, 0, sizeof( v_listdata ) );

	// create an list of all the 
	for (i = 0; i < g_numfaces; i++)
	{
		tmpface[i] = g_face[i];

		uface[i].a = AddToVlist( g_face[i].a, g_face[i].material, g_face[i].na, g_face[i].ta, g_numverts );
		uface[i].b = AddToVlist( g_face[i].b, g_face[i].material, g_face[i].nb, g_face[i].tb, g_numverts );
		uface[i].c = AddToVlist( g_face[i].c, g_face[i].material, g_face[i].nc, g_face[i].tc, g_numverts );

		// keep an original copy
		g_src_uface[i] = uface[i];
	}

	// printf("%d : %d %d %d\n", numvlist, g_numverts, g_numnormals, g_numtexcoords );
}

void CalcModelTangentSpaces( s_source_t *pSrc );

void BuildIndividualMeshes( s_source_t *psource )
{
	int i, j, k;
	
	// sort new vertices by materials, last used
	static int v_listsort[MAXSTUDIOVERTS];	// map desired order to vlist entry
	static int v_ilistsort[MAXSTUDIOVERTS]; // map vlist entry to desired order

	for (i = 0; i < numvlist; i++)
	{
		v_listsort[i] = i;
	}
	qsort( v_listsort, numvlist, sizeof( int ), vlistCompare );
	for (i = 0; i < numvlist; i++)
	{
		v_ilistsort[v_listsort[i]] = i;
	}


	// allocate memory
	psource->numvertices = numvlist;
	psource->vertex = (s_vertexinfo_t *)kalloc( psource->numvertices, sizeof( s_vertexinfo_t ) );
	psource->localBoneweight = (s_boneweight_t *)kalloc( psource->numvertices, sizeof( s_boneweight_t ) );

	// create arrays of unique vertexes, normals, texcoords.
	for (i = 0; i < psource->numvertices; i++)
	{
		j = v_listsort[i];

		VectorCopy( g_vertex[v_listdata[j].v], psource->vertex[i].position );
		VectorCopy( g_normal[v_listdata[j].n], psource->vertex[i].normal );		
		Vector2Copy( g_texcoord[v_listdata[j].t], psource->vertex[i].texcoord );

		psource->localBoneweight[i].numbones		= g_bone[v_listdata[j].v].numbones;
		int k;
		for( k = 0; k < MAXSTUDIOBONEWEIGHTS; k++ )
		{
			psource->localBoneweight[i].bone[k]		= g_bone[v_listdata[j].v].bone[k];
			psource->localBoneweight[i].weight[k]	= g_bone[v_listdata[j].v].weight[k];
		}

		// store a bunch of other info
		psource->vertex[i].material			= v_listdata[j].m;
		
		// always assume this is lod 1
		psource->vertex[i].bLoD				= 1;
#if 0
		psource->vertexInfo[i].firstref		= v_listdata[j].firstref;
		psource->vertexInfo[i].lastref		= v_listdata[j].lastref;
#endif
		// printf("%4d : %2d :  %6.2f %6.2f %6.2f\n", i, psource->boneweight[i].bone[0], psource->vertex[i][0], psource->vertex[i][1], psource->vertex[i][2] );
	}

	// sort faces by materials, last used.
	static int facesort[MAXSTUDIOTRIANGLES];	// map desired order to src_face entry
	static int ifacesort[MAXSTUDIOTRIANGLES];	// map src_face entry to desired order
	
	for (i = 0; i < g_numfaces; i++)
	{
		facesort[i] = i;
	}
	qsort( facesort, g_numfaces, sizeof( int ), faceCompare );
	for (i = 0; i < g_numfaces; i++)
	{
		ifacesort[facesort[i]] = i;
	}

	psource->numfaces = g_numfaces;
	// find first occurance for each material
	for (k = 0; k < MAXSTUDIOSKINS; k++)
	{
		psource->mesh[k].numvertices = 0;
		psource->mesh[k].vertexoffset = psource->numvertices;

		psource->mesh[k].numfaces = 0;
		psource->mesh[k].faceoffset = g_numfaces;
	}

	// find first and count of indices per material
	for (i = 0; i < psource->numvertices; i++)
	{
		k = psource->vertex[i].material;
		psource->mesh[k].numvertices++;
		if (psource->mesh[k].vertexoffset > i)
			psource->mesh[k].vertexoffset = i;
	}

	// find first and count of faces per material
	for (i = 0; i < psource->numfaces; i++)
	{
		k = g_face[facesort[i]].material;

		psource->mesh[k].numfaces++;
		if (psource->mesh[k].faceoffset > i)
			psource->mesh[k].faceoffset = i;
	}

	/*
	for (k = 0; k < MAXSTUDIOSKINS; k++)
	{
		printf("%d : %d:%d %d:%d\n", k, psource->mesh[k].numvertices, psource->mesh[k].vertexoffset, psource->mesh[k].numfaces, psource->mesh[k].faceoffset );
	}
	*/

	// create remapped faces
	psource->face = (s_face_t *)kalloc( psource->numfaces, sizeof( s_face_t ));
	for (k = 0; k < MAXSTUDIOSKINS; k++)
	{
		if (psource->mesh[k].numfaces)
		{
			psource->meshindex[psource->nummeshes] = k;

			for (i = psource->mesh[k].faceoffset; i < psource->mesh[k].numfaces + psource->mesh[k].faceoffset; i++)
			{
				j = facesort[i];

				psource->face[i].a = v_ilistsort[g_src_uface[j].a] - psource->mesh[k].vertexoffset;
				psource->face[i].b = v_ilistsort[g_src_uface[j].b] - psource->mesh[k].vertexoffset;
				psource->face[i].c = v_ilistsort[g_src_uface[j].c] - psource->mesh[k].vertexoffset;
				Assert( ((psource->face[i].a & 0xF0000000) == 0) && ((psource->face[i].b & 0xF0000000) == 0) && 
					((psource->face[i].c & 0xF0000000) == 0) );
				// printf("%3d : %4d %4d %4d\n", i, psource->face[i].a, psource->face[i].b, psource->face[i].c );
			}

			psource->nummeshes++;
		}
	}

	CalcModelTangentSpaces( psource );
}


void Grab_MRMFaceupdates( s_source_t *psource )
{
	while (1) 
	{
		if (fgets( g_szLine, sizeof( g_szLine ), g_fpInput ) != NULL) 
		{
			g_iLinecount++;

			// check for end
			if (IsEnd(g_szLine)) 
				return;
		}
	}
}

int Load_VRM ( s_source_t *psource )
{
	char	cmd[1024];
	int		option;

	if (!OpenGlobalFile( psource->filename ))
	{	
		return 0;
	}

	if( !g_quiet )
	{
		printf ("grabbing %s\n", psource->filename);
	}

	g_iLinecount = 0;

	while (fgets( g_szLine, sizeof( g_szLine ), g_fpInput ) != NULL) {
		g_iLinecount++;
		sscanf( g_szLine, "%1023s %d", cmd, &option );
		if (stricmp( cmd, "version" ) == 0) {
			if (option != 2) {
				MdlError("bad version\n");
			}
		}
		else if (stricmp( cmd, "name" ) == 0) {
		}
		else if (stricmp( cmd, "vertices" ) == 0) {
			g_numverts = option;
		}
		else if (stricmp( cmd, "faces" ) == 0) {
			g_numfaces = option;
		}
		else if (stricmp( cmd, "materials" ) == 0) {
			// doesn't matter;
		}
		else if (stricmp( cmd, "texcoords" ) == 0) {
			g_numtexcoords = option;
			if (option == 0)
				MdlError( "model has no texture coordinates\n");
		}
		else if (stricmp( cmd, "normals" ) == 0) {
			g_numnormals = option;
		}
		else if (stricmp( cmd, "tristrips" ) == 0) {
			// should be 0;
		}

		else if (stricmp( cmd, "vertexlist" ) == 0) {
			Grab_Vertexlist( psource );
		}
		else if (stricmp( cmd, "facelist" ) == 0) {
			Grab_Facelist( psource );
		}
		else if (stricmp( cmd, "materiallist" ) == 0) {
			Grab_Materiallist( psource );
		}
		else if (stricmp( cmd, "texcoordlist" ) == 0) {
			Grab_Texcoordlist( psource );
		}
		else if (stricmp( cmd, "normallist" ) == 0) {
			Grab_Normallist( psource );
		}
		else if (stricmp( cmd, "faceattriblist" ) == 0) {
			Grab_Faceattriblist( psource );
		}

		else if (stricmp( cmd, "MRM" ) == 0) {
		}
		else if (stricmp( cmd, "MRMvertices" ) == 0) {
		}
		else if (stricmp( cmd, "MRMfaces" ) == 0) {
		}
		else if (stricmp( cmd, "MRMfaceupdates" ) == 0) 
		{
			Grab_MRMFaceupdates( psource );
		}

		else if (stricmp( cmd, "nodes" ) == 0) {
			psource->numbones = Grab_Nodes( psource->localBone );
		}
		else if (stricmp( cmd, "skeleton" ) == 0) {
			Grab_Animation( psource );
		}
/*		
		else if (stricmp( cmd, "triangles" ) == 0) {
			Grab_Triangles( psource );
		}
*/
		else 
		{
			MdlError("unknown VRM command : %s \n", cmd );
		}
	}

	UnifyIndices( psource );
	BuildIndividualMeshes( psource );

	fclose( g_fpInput );

	return 1;
}

