//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "vbsp.h"

/*
==============================================================================

PORTAL FILE GENERATION

Save out name.prt for qvis to read
==============================================================================
*/


#define	PORTALFILE	"PRT1"

FILE	*pf;
int		num_visclusters;				// clusters the player can be in
int		num_visportals;

void WriteFloat (FILE *f, vec_t v)
{
	if ( fabs(v - RoundInt(v)) < 0.001 )
		fprintf (f,"%i ",(int)RoundInt(v));
	else
		fprintf (f,"%f ",v);
}

dportal_t *AddBSPPortal( portal_t *p, Vector& normal, float dist, qboolean backwards )
{
	int			i;
	dportal_t	*bspPortal;
	winding_t	*w;

	bspPortal = dportals + numportals;
	numportals++;

	w = p->winding;

	if ( numportals >= MAX_MAP_PORTALS )
	{
		Error( "Too many portals\n" );
	}

	if ( backwards )
	{
		bspPortal->cluster[0] = p->nodes[1]->cluster;
		bspPortal->cluster[1] = p->nodes[0]->cluster;
	}
	else
	{
		bspPortal->cluster[0] = p->nodes[0]->cluster;
		bspPortal->cluster[1] = p->nodes[1]->cluster;
	}

	bspPortal->planenum = FindFloatPlane( normal, dist );
	bspPortal->numportalverts = w->numpoints;
	bspPortal->firstportalvert = numportalverts;	// bottom of list
	
	// append poly to list
	for ( i = 0; i < w->numpoints; i++ )
	{
		dportalverts[numportalverts] = GetVertexnum( w->p[i] );
		numportalverts++;
		if ( numportalverts >= MAX_MAP_PORTALVERTS )
		{
			Error( "Too many portal verts" );
		}
	}

	return bspPortal;
}


/*
=================
WritePortalFile_r
=================
*/
void WritePortalFile_r (node_t *node)
{
	int			i, s;	
	portal_t	*p;
	winding_t	*w;
	Vector		normal;
	vec_t		dist;

	// decision node
	if (node->planenum != PLANENUM_LEAF)
	{
		WritePortalFile_r (node->children[0]);
		WritePortalFile_r (node->children[1]);
		return;
	}
	
	if (node->contents & CONTENTS_SOLID)
		return;

	for (p = node->portals ; p ; p=p->next[s])
	{
		w = p->winding;
		s = (p->nodes[1] == node);
		if (w && p->nodes[0] == node)
		{
			qboolean backwards;

			// Check visibility through this portal -- if not an open portal, it's a visible, solid face/polygon, so skip it
			if (!Portal_VisFlood (p))
				continue;
		// write out to the file
		
		// sometimes planes get turned around when they are very near
		// the changeover point between different axis.  interpret the
		// plane the same way vis will, and flip the side orders if needed
			// FIXME: is this still relevent?
			WindingPlane (w, normal, &dist);
			if ( DotProduct (p->plane.normal, normal) < 0.99 )
			{	// backwards...
				fprintf (pf,"%i %i %i ",w->numpoints, p->nodes[1]->cluster, p->nodes[0]->cluster);
				backwards = true;
			}
			else
			{
				fprintf (pf,"%i %i %i ",w->numpoints, p->nodes[0]->cluster, p->nodes[1]->cluster);
				backwards = false;
			}
			
			AddBSPPortal( p, normal, dist, backwards );

			for (i=0 ; i<w->numpoints ; i++)
			{
				fprintf (pf,"(");
				WriteFloat (pf, w->p[i][0]);
				WriteFloat (pf, w->p[i][1]);
				WriteFloat (pf, w->p[i][2]);
				fprintf (pf,") ");
			}
			fprintf (pf,"\n");
		}
	}

}

/*
================
FillLeafNumbers_r

All of the leafs under node will have the same cluster
================
*/
void FillLeafNumbers_r (node_t *node, int num)
{
	if (node->planenum == PLANENUM_LEAF)
	{
		if (node->contents & CONTENTS_SOLID)
			node->cluster = -1;
		else
			node->cluster = num;
		return;
	}
	node->cluster = num;
	FillLeafNumbers_r (node->children[0], num);
	FillLeafNumbers_r (node->children[1], num);
}

/*
================
NumberLeafs_r
================
*/
void NumberLeafs_r (node_t *node)
{
	portal_t	*p;

	if (node->planenum != PLANENUM_LEAF)
	{	// decision node
		node->cluster = -99;
		NumberLeafs_r (node->children[0]);
		NumberLeafs_r (node->children[1]);
		return;
	}
	
	// either a leaf or a detail cluster

	if ( node->contents & CONTENTS_SOLID )
	{	// solid block, viewpoint never inside
		node->cluster = -1;
		return;
	}

	FillLeafNumbers_r (node, num_visclusters);
	num_visclusters++;

	// count the portals
	for (p = node->portals ; p ; )
	{
		if (p->nodes[0] == node)		// only write out from first leaf
		{
			if (Portal_VisFlood (p))
				num_visportals++;
			p = p->next[0];
		}
		else
			p = p->next[1];		
	}
}


/*
================
CreateVisPortals_r
================
*/
void CreateVisPortals_r (node_t *node)
{
	// stop as soon as we get to a leaf
	if (node->planenum == PLANENUM_LEAF )
		return;

	MakeNodePortal (node);
	SplitNodePortals (node);

	CreateVisPortals_r (node->children[0]);
	CreateVisPortals_r (node->children[1]);
}

int		clusterleaf;
void SaveClusters_r (node_t *node)
{
	if (node->planenum == PLANENUM_LEAF)
	{
		dleafs[clusterleaf++].cluster = node->cluster;
		return;
	}
	SaveClusters_r (node->children[0]);
	SaveClusters_r (node->children[1]);
}

//-----------------------------------------------------------------------------
// Purpose: Build dclusterportals and dclusters
//-----------------------------------------------------------------------------
void SaveClusterPortalList( void )
{
	int i, j;
	unsigned short *list = dclusterportals;
	dportal_t *p;

	// SaveClusters_r counts this, store it in the model
	numclusters = clusterleaf;
	
	// clear the cluster portal list so we can build it
	numclusterportals = 0;

	// Go through each cluster, find all portals leading into or out of it and add them
	// its cluster list in dclusterportals
	for ( i = 0; i < numclusters; i++ )
	{
		dclusters[i].numportals = 0;
		dclusters[i].firstportal = numclusterportals;

		p = dportals;
		for ( j = 0; j < numportals; j++ )
		{
			if ( p->cluster[0] == i || p->cluster[1] == i )
			{
				dclusters[i].numportals++;
				*list = (unsigned short )j;
				list++;
				numclusterportals++;
			}
			p++;
		}
		if ( numclusterportals > numportals*2 )
			Error( "numclusterportals != numportals * 2" );
	}
}

/*
================
WritePortalFile
================
*/
void WritePortalFile (tree_t *tree)
{
	char	filename[1024];
	node_t *headnode;
	int start = Plat_FloatTime();

	qprintf ("--- WritePortalFile ---\n");

	sprintf (filename, "%s.prt", source);
	Msg ("writing %s...", filename);

	headnode = tree->headnode;

	FreeTreePortals_r (headnode);
	MakeHeadnodePortals (tree);

	CreateVisPortals_r (headnode);

// set the cluster field in every leaf and count the total number of portals
	num_visclusters = 0;
	num_visportals = 0;
	NumberLeafs_r (headnode);
	
// write the file
	pf = fopen (filename, "w");
	if (!pf)
		Error ("Error opening %s", filename);
		
	fprintf (pf, "%s\n", PORTALFILE);
	fprintf (pf, "%i\n", num_visclusters);
	fprintf (pf, "%i\n", num_visportals);

	qprintf ("%5i visclusters\n", num_visclusters);
	qprintf ("%5i visportals\n", num_visportals);

	WritePortalFile_r (headnode);

	fclose (pf);

	// we need to store the clusters out now because ordering
	// issues made us do this after writebsp...
	clusterleaf = 1;
	SaveClusters_r (headnode);

	SaveClusterPortalList();

	Msg("done (%d)\n", (int)(Plat_FloatTime() - start) );
}

