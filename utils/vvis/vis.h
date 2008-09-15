//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// vis.h

#include "cmdlib.h"
#include "mathlib.h"
#include "bsplib.h"


//#define	MAX_PORTALS	32768
#define	MAX_PORTALS	65536

#define	PORTALFILE	"PRT1"

extern bool g_bUseRadius;			
extern double g_VisRadius;			

typedef struct
{
	Vector		normal;
	float		dist;
} plane_t;

#define MAX_POINTS_ON_WINDING	64
#define	MAX_POINTS_ON_FIXED_WINDING	12

typedef struct
{
	qboolean	original;			// don't free, it's part of the portal
	int		numpoints;
	Vector	points[MAX_POINTS_ON_FIXED_WINDING];			// variable sized
} winding_t;

winding_t	*NewWinding (int points);
void		FreeWinding (winding_t *w);
winding_t	*CopyWinding (winding_t *w);


typedef enum {stat_none, stat_working, stat_done} vstatus_t;
struct portal_t
{
	plane_t		plane;	// normal pointing into neighbor
	int			leaf;	// neighbor
	
	Vector		origin;	// for fast clip testing
	float		radius;

	winding_t	*winding;
	vstatus_t	status;
	byte		*portalfront;	// [portals], preliminary
	byte		*portalflood;	// [portals], intermediate
	byte		*portalvis;		// [portals], final

	int			nummightsee;	// bit count on portalflood for sort
};

struct sep_t
{
	sep_t		*next;
	plane_t		plane;		// from portal is on positive side
};


typedef struct passage_s
{
	struct passage_s	*next;
	int			from, to;		// leaf numbers
	sep_t				*planes;
} passage_t;

#define	MAX_PORTALS_ON_LEAF		128
typedef struct leaf_s
{
	int			numportals;
	passage_t	*passages;
	portal_t	*portals[MAX_PORTALS_ON_LEAF];
} leaf_t;

	
typedef struct pstack_s
{
	byte		mightsee[MAX_PORTALS/8];		// bit string
	struct pstack_s	*next;
	leaf_t		*leaf;
	portal_t	*portal;	// portal exiting
	winding_t	*source;
	winding_t	*pass;

	winding_t	windings[3];	// source, pass, temp in any order
	int			freewindings[3];

	plane_t		portalplane;
} pstack_t;

typedef struct
{
	portal_t	*base;
	int			c_chains;
	pstack_t	pstack_head;
} threaddata_t;

extern	int			g_numportals;
extern	int			portalclusters;

extern	portal_t	*portals;
extern	leaf_t		*leafs;

extern	int			c_portaltest, c_portalpass, c_portalcheck;
extern	int			c_portalskip, c_leafskip;
extern	int			c_vistest, c_mighttest;
extern	int			c_chains;

extern	byte	*vismap, *vismap_p, *vismap_end;	// past visfile

extern	int			testlevel;

extern	byte		*uncompressed;

extern	int		leafbytes, leaflongs;
extern	int		portalbytes, portallongs;


void LeafFlow (int leafnum);


void BasePortalVis (int iThread, int portalnum);
void BetterPortalVis (int portalnum);
void PortalFlow (int iThread, int portalnum);

extern	portal_t	*sorted_portals[MAX_MAP_PORTALS*2];

int CountBits (byte *bits, int numbits);

#define CheckBit( bitstring, bitNumber )	( (bitstring)[ ((bitNumber) >> 3) ] & ( 1 << ( (bitNumber) & 7 ) ) )
#define SetBit( bitstring, bitNumber )	( (bitstring)[ ((bitNumber) >> 3) ] |= ( 1 << ( (bitNumber) & 7 ) ) )
#define ClearBit( bitstring, bitNumber )	( (bitstring)[ ((bitNumber) >> 3) ] &= ~( 1 << ( (bitNumber) & 7 ) ) )
