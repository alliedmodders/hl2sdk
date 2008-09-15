//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

//
// write.c: writes a studio .mdl file
//

#pragma warning( disable : 4244 )
#pragma warning( disable : 4237 )
#pragma warning( disable : 4305 )


#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <limits.h>

#include "cmdlib.h"
#include "scriplib.h"
#include "mathlib.h"
#include "studio.h"
#include "studiomdl.h"
#include "collisionmodel.h"
#include "optimize.h"
#include "matsys.h"
#include "materialsystem/IMaterial.h"
#include "materialsystem/IMaterialVar.h"
#include "perfstats.h"

int totalframes = 0;
float totalseconds = 0;
extern int numcommandnodes;

// WriteFile is the only externally visible function in this file.
// pData points to the current location in an output buffer and pStart is
// the beginning of the buffer.

bool FixupToSortedLODVertexes( studiohdr_t *pStudioHdr );
bool Clamp_RootLOD(  studiohdr_t *phdr );


/*
============
WriteModel
============
*/

static byte *pData;
static byte *pStart;
static byte *pBlockData;
static byte *pBlockStart;

#undef ALIGN16
#undef ALIGN32
#define ALIGN4( a ) a = (byte *)((int)((byte *)a + 3) & ~ 3)
#define ALIGN16( a ) a = (byte *)((int)((byte *)a + 15) & ~ 15)
#define ALIGN32( a ) a = (byte *)((int)((byte *)a + 31) & ~ 31)
#define ALIGN64( a ) a = (byte *)((int)((byte *)a + 63) & ~ 63)
#define ALIGN512( a ) a = (byte *)((int)((byte *)a + 511) & ~ 511)
// make sure kalloc aligns to maximum alignment size

#define FILEBUFFER (8 * 1024 * 1024)

void WriteSeqKeyValues( mstudioseqdesc_t *pseqdesc, CUtlVector< char > *pKeyValue );

//-----------------------------------------------------------------------------
// Purpose: stringtable is a session global string table.
//-----------------------------------------------------------------------------

struct stringtable_t
{
	byte	*base;
	int		*ptr;
	const char	*string;
	int		dupindex;
	byte	*addr;
};

static int numStrings;
static stringtable_t strings[32768];

static void BeginStringTable(  )
{
	strings[0].base = NULL;
	strings[0].ptr = NULL;
	strings[0].string = "";
	strings[0].dupindex = -1;
	numStrings = 1;
}

//-----------------------------------------------------------------------------
// Purpose: add a string to the file-global string table.
//			Keep track of fixup locations
//-----------------------------------------------------------------------------
static void AddToStringTable( void *base, int *ptr, const char *string )
{
	for (int i = 0; i < numStrings; i++)
	{
		if (!string || !strcmp( string, strings[i].string ))
		{
			strings[numStrings].base = (byte *)base;
			strings[numStrings].ptr = ptr;
			strings[numStrings].string = string;
			strings[numStrings].dupindex = i;
			numStrings++;
			return;
		}
	}

	strings[numStrings].base = (byte *)base;
	strings[numStrings].ptr = ptr;
	strings[numStrings].string = string;
	strings[numStrings].dupindex = -1;
	numStrings++;
}

//-----------------------------------------------------------------------------
// Purpose: Write out stringtable
//			fixup local pointers
//-----------------------------------------------------------------------------
static byte *WriteStringTable( byte *pData )
{
	// force null at first address
	strings[0].addr = pData;
	*pData = '\0';
	pData++;

	// save all the rest
	for (int i = 1; i < numStrings; i++)
	{
		if (strings[i].dupindex == -1)
		{
			// not in table yet
			// calc offset relative to local base
			*strings[i].ptr = pData - strings[i].base;
			// keep track of address in case of duplication
			strings[i].addr = pData;
			// copy string data, add a terminating \0
			strcpy( (char *)pData, strings[i].string );
			pData += strlen( strings[i].string );
			*pData = '\0';
			pData++;
		}
		else
		{
			// already in table, calc offset of existing string relative to local base
			*strings[i].ptr = strings[strings[i].dupindex].addr - strings[i].base;
		}
	}
	ALIGN4( pData );
	return pData;
}


// compare function for qsort below
static int BoneNameCompare( const void *elem1, const void *elem2 )
{
	int index1 = *(byte *)elem1;
	int index2 = *(byte *)elem2;

	// compare bones by name
	return strcmpi( g_bonetable[index1].name, g_bonetable[index2].name );
}


static void WriteBoneInfo( studiohdr_t *phdr )
{
	int i, j, k;
	mstudiobone_t *pbone;
	mstudiobonecontroller_t *pbonecontroller;
	mstudioattachment_t *pattachment;
	mstudiobbox_t *pbbox;

	// save bone info
	pbone = (mstudiobone_t *)pData;
	phdr->numbones = IsChar( g_numbones );
	phdr->boneindex = IsInt24( pData - pStart );

	char* pSurfacePropName = GetDefaultSurfaceProp( );
	AddToStringTable( phdr, &phdr->surfacepropindex, pSurfacePropName );
	phdr->contents = GetDefaultContents();

	for (i = 0; i < g_numbones; i++) 
	{
		AddToStringTable( &pbone[i], &pbone[i].sznameindex, g_bonetable[i].name );
		pbone[i].parent			= g_bonetable[i].parent;
		pbone[i].flags			= g_bonetable[i].flags;
		pbone[i].procindex		= 0;
		pbone[i].physicsbone	= g_bonetable[i].physicsBoneIndex;
		pbone[i].pos			= g_bonetable[i].pos;
		pbone[i].rot			= g_bonetable[i].rot;
		pbone[i].posscale		= g_bonetable[i].posscale;
		pbone[i].rotscale		= g_bonetable[i].rotscale;
		MatrixInvert( g_bonetable[i].boneToPose, pbone[i].poseToBone );
		pbone[i].qAlignment		= g_bonetable[i].qAlignment;

		AngleQuaternion( RadianEuler( g_bonetable[i].rot[0], g_bonetable[i].rot[1], g_bonetable[i].rot[2] ), pbone[i].quat );
		QuaternionAlign( pbone[i].qAlignment, pbone[i].quat, pbone[i].quat );

		pSurfacePropName = GetSurfaceProp( g_bonetable[i].name );
		AddToStringTable( &pbone[i], &pbone[i].surfacepropidx, pSurfacePropName );
		pbone[i].contents = GetContents( g_bonetable[i].name );
	}

	pData += g_numbones * sizeof( mstudiobone_t );
	ALIGN4( pData );

	// save procedural bone info
	if (g_numaxisinterpbones)
	{
		mstudioaxisinterpbone_t *pProc = (mstudioaxisinterpbone_t *)pData;
		for (i = 0; i < g_numaxisinterpbones; i++)
		{
			j = g_axisinterpbonemap[i];
			k = g_axisinterpbones[j].bone;
			pbone[k].procindex		= (byte *)&pProc[i] - (byte *)&pbone[k];
			pbone[k].proctype		= STUDIO_PROC_AXISINTERP;
			// printf("bone %d %d\n", j, pbone[k].procindex );
			pProc[i].control		= g_axisinterpbones[j].control;
			pProc[i].axis			= g_axisinterpbones[j].axis;
			for (k = 0; k < 6; k++)
			{
				VectorCopy( g_axisinterpbones[j].pos[k], pProc[i].pos[k] );
				pProc[i].quat[k] = g_axisinterpbones[j].quat[k];
			}
		}
		pData += g_numaxisinterpbones * sizeof( mstudioaxisinterpbone_t );
		ALIGN4( pData );
	}

	if (g_numquatinterpbones)
	{
		mstudioquatinterpbone_t *pProc = (mstudioquatinterpbone_t *)pData;
		pData += g_numquatinterpbones * sizeof( mstudioquatinterpbone_t );
		ALIGN4( pData );

		for (i = 0; i < g_numquatinterpbones; i++)
		{
			j = g_quatinterpbonemap[i];
			k = g_quatinterpbones[j].bone;
			pbone[k].procindex		= (byte *)&pProc[i] - (byte *)&pbone[k];
			pbone[k].proctype		= STUDIO_PROC_QUATINTERP;
			// printf("bone %d %d\n", j, pbone[k].procindex );
			pProc[i].control		= g_quatinterpbones[j].control;

			mstudioquatinterpinfo_t *pTrigger = (mstudioquatinterpinfo_t *)pData;
			pProc[i].numtriggers	= g_quatinterpbones[j].numtriggers;
			pProc[i].triggerindex	= (byte *)pTrigger - (byte *)&pProc[i];
			pData += pProc[i].numtriggers * sizeof( mstudioquatinterpinfo_t );

			for (k = 0; k < pProc[i].numtriggers; k++)
			{
				pTrigger[k].inv_tolerance	= 1.0 / g_quatinterpbones[j].tolerance[k];
				pTrigger[k].trigger		= g_quatinterpbones[j].trigger[k];
				pTrigger[k].pos			= g_quatinterpbones[j].pos[k];
				pTrigger[k].quat		= g_quatinterpbones[j].quat[k];
			}
		}
	}

	// write aim at bones
	if (g_numaimatbones)
	{
		mstudioaimatbone_t *pProc = (mstudioaimatbone_t *)pData;
		for (i = 0; i < g_numaimatbones; i++)
		{
			j = g_aimatbonemap[i];
			k = g_aimatbones[j].bone;
			pbone[k].procindex		= (byte *)&pProc[i] - (byte *)&pbone[k];
			pbone[k].proctype		= g_aimatbones[j].aimAttach == -1 ? STUDIO_PROC_AIMATBONE : STUDIO_PROC_AIMATATTACH;
			pProc[i].parent			= g_aimatbones[j].parent;
			pProc[i].aim			= g_aimatbones[j].aimAttach == -1 ? g_aimatbones[j].aimBone : g_aimatbones[j].aimAttach;
			pProc[i].aimvector		= g_aimatbones[j].aimvector;
			pProc[i].upvector		= g_aimatbones[j].upvector;
			pProc[i].basepos		= g_aimatbones[j].basepos;
		}
		pData += g_numaimatbones * sizeof( mstudioaimatbone_t );
		ALIGN4( pData );
	}

	// map g_bonecontroller to bones
	for (i = 0; i < g_numbones; i++) 
	{
		for (j = 0; j < 6; j++)	
		{
			pbone[i].bonecontroller[j] = -1;
		}
	}
	
	for (i = 0; i < g_numbonecontrollers; i++) 
	{
		j = g_bonecontroller[i].bone;
		switch( g_bonecontroller[i].type & STUDIO_TYPES )
		{
		case STUDIO_X:
			pbone[j].bonecontroller[0] = i;
			break;
		case STUDIO_Y:
			pbone[j].bonecontroller[1] = i;
			break;
		case STUDIO_Z:
			pbone[j].bonecontroller[2] = i;
			break;
		case STUDIO_XR:
			pbone[j].bonecontroller[3] = i;
			break;
		case STUDIO_YR:
			pbone[j].bonecontroller[4] = i;
			break;
		case STUDIO_ZR:
			pbone[j].bonecontroller[5] = i;
			break;
		default:
			MdlError("unknown g_bonecontroller type\n");
		}
	}

	// save g_bonecontroller info
	pbonecontroller = (mstudiobonecontroller_t *)pData;
	phdr->numbonecontrollers = IsChar( g_numbonecontrollers );
	phdr->bonecontrollerindex = IsInt24( pData - pStart );

	for (i = 0; i < g_numbonecontrollers; i++) 
	{
		pbonecontroller[i].bone			= g_bonecontroller[i].bone;
		pbonecontroller[i].inputfield	= g_bonecontroller[i].inputfield;
		pbonecontroller[i].type			= g_bonecontroller[i].type;
		pbonecontroller[i].start		= g_bonecontroller[i].start;
		pbonecontroller[i].end			= g_bonecontroller[i].end;
	}
	pData += g_numbonecontrollers * sizeof( mstudiobonecontroller_t );
	ALIGN4( pData );

	// save attachment info
	pattachment = (mstudioattachment_t *)pData;
	phdr->numlocalattachments = IsChar( g_numattachments );
	phdr->localattachmentindex = IsInt24( pData - pStart );

	for (i = 0; i < g_numattachments; i++) 
	{
		pattachment[i].localbone			= g_attachment[i].bone;
		AddToStringTable( &pattachment[i], &pattachment[i].sznameindex, g_attachment[i].name );
		MatrixCopy( g_attachment[i].local, pattachment[i].local );
		pattachment[i].flags = g_attachment[i].flags;
	}
	pData += g_numattachments * sizeof( mstudioattachment_t );
	ALIGN4( pData );
	
	// save hitbox sets
	phdr->numhitboxsets = IsChar( g_hitboxsets.Size() );

	// Remember start spot
	mstudiohitboxset_t *hitboxset = (mstudiohitboxset_t *)pData;
	phdr->hitboxsetindex = IsInt24( pData - pStart );

	pData += phdr->numhitboxsets * sizeof( mstudiohitboxset_t );
	ALIGN4( pData );

	for ( int s = 0; s < g_hitboxsets.Size(); s++, hitboxset++ )
	{
		s_hitboxset *set = &g_hitboxsets[ s ];

		AddToStringTable( hitboxset, &hitboxset->sznameindex, set->hitboxsetname );

		hitboxset->numhitboxes = set->numhitboxes;
		hitboxset->hitboxindex = ( pData - (byte *)hitboxset );

		// save bbox info
		pbbox = (mstudiobbox_t *)pData;
		for (i = 0; i < hitboxset->numhitboxes; i++) 
		{
			pbbox[i].bone				= set->hitbox[i].bone;
			pbbox[i].group				= set->hitbox[i].group;
			VectorCopy( set->hitbox[i].bmin, pbbox[i].bbmin );
			VectorCopy( set->hitbox[i].bmax, pbbox[i].bbmax );
			pbbox[i].szhitboxnameindex = 0;
			AddToStringTable( &(pbbox[i]), &(pbbox[i].szhitboxnameindex), set->hitbox[i].hitboxname );	
		}

		pData += hitboxset->numhitboxes * sizeof( mstudiobbox_t );
		ALIGN4( pData );
	}
	byte *pBoneTable = pData;
	phdr->bonetablebynameindex = (pData - pStart);

	// make a table in bone order and sort it with qsort
	for ( i = 0; i < phdr->numbones; i++ )
	{
		pBoneTable[i] = i;
	}
	qsort( pBoneTable, phdr->numbones, sizeof(byte), BoneNameCompare );
	pData += phdr->numbones * sizeof( byte );
	ALIGN4( pData );
}


static void WriteSequenceInfo( studiohdr_t *phdr )
{
	int i, j, k;

	mstudioseqdesc_t	*pseqdesc;
	mstudioseqdesc_t	*pbaseseqdesc;
	mstudioevent_t		*pevent;
	byte				*ptransition;

	// write models to disk with this flag set false. This will force
	// the sequences to be indexed by activity whenever the g_model is loaded
	// from disk.
	phdr->activitylistversion = 0;
	phdr->eventsindexed = 0;

	// save g_sequence info
	pseqdesc = (mstudioseqdesc_t *)pData;
	pbaseseqdesc = pseqdesc;
	phdr->numlocalseq = IsShort( g_sequence.Count() );
	phdr->localseqindex = IsInt24( (pData - pStart) );
	pData += g_sequence.Count() * sizeof( mstudioseqdesc_t );

	bool bErrors = false;

	for (i = 0; i < g_sequence.Count(); i++, pseqdesc++) 
	{
		byte *pSequenceStart = (byte *)pseqdesc;

		AddToStringTable( pseqdesc, &pseqdesc->szlabelindex, g_sequence[i].name );
		AddToStringTable( pseqdesc, &pseqdesc->szactivitynameindex, g_sequence[i].activityname );

		pseqdesc->baseptr		= pStart - (byte *)pseqdesc;

		pseqdesc->flags			= g_sequence[i].flags;

		pseqdesc->numblends		= g_sequence[i].numblends;
		pseqdesc->groupsize[0]	= g_sequence[i].groupsize[0];
		pseqdesc->groupsize[1]	= g_sequence[i].groupsize[1];

		pseqdesc->paramindex[0]	= g_sequence[i].paramindex[0];
		pseqdesc->paramstart[0] = g_sequence[i].paramstart[0];
		pseqdesc->paramend[0]	= g_sequence[i].paramend[0];
		pseqdesc->paramindex[1]	= g_sequence[i].paramindex[1];
		pseqdesc->paramstart[1] = g_sequence[i].paramstart[1];
		pseqdesc->paramend[1]	= g_sequence[i].paramend[1];

		if (g_sequence[i].groupsize[0] > 1 || g_sequence[i].groupsize[1] > 1)
		{
			// save posekey values
			float *pposekey			= (float *)pData;
			pseqdesc->posekeyindex	= (pData - pSequenceStart);
			pData += (pseqdesc->groupsize[0] + pseqdesc->groupsize[1]) * sizeof( float );
			for (j = 0; j < pseqdesc->groupsize[0]; j++)
			{
				*(pposekey++) = g_sequence[i].param0[j];
				// printf("%.2f ", g_sequence[i].param0[j] );
			}
			for (j = 0; j < pseqdesc->groupsize[1]; j++)
			{
				*(pposekey++) = g_sequence[i].param1[j];
				// printf("%.2f ", g_sequence[i].param1[j] );
			}
			// printf("\n" );
		}

		// pseqdesc->motiontype	= g_sequence[i].motiontype;
		// pseqdesc->motionbone	= 0; // g_sequence[i].motionbone;
		// VectorCopy( g_sequence[i].linearmovement, pseqdesc->linearmovement );

		pseqdesc->activity		= g_sequence[i].activity;
		pseqdesc->actweight		= g_sequence[i].actweight;

		pseqdesc->bbmin			= g_sequence[i].bmin;
		pseqdesc->bbmax			= g_sequence[i].bmax;

		pseqdesc->fadeintime	= g_sequence[i].fadeintime;
		pseqdesc->fadeouttime	= g_sequence[i].fadeouttime;

		pseqdesc->localentrynode	= g_sequence[i].entrynode; 
		pseqdesc->localexitnode		= g_sequence[i].exitnode;
		//pseqdesc->entryphase	= g_sequence[i].entryphase;
		//pseqdesc->exitphase		= g_sequence[i].exitphase;
		pseqdesc->nodeflags		= g_sequence[i].nodeflags;

		// save events
		pevent					= (mstudioevent_t *)pData;
		pseqdesc->numevents		= IsByte( g_sequence[i].numevents );
		pseqdesc->eventindex	= IsInt24( (pData - pSequenceStart) );
		pData += pseqdesc->numevents * sizeof( mstudioevent_t );
		for (j = 0; j < g_sequence[i].numevents; j++)
		{
			k = g_sequence[i].panim[0][0]->numframes - 1;

			if (g_sequence[i].event[j].frame <= k)
				pevent[j].cycle		= g_sequence[i].event[j].frame / ((float)k);
			else if (k == 0 && g_sequence[i].event[j].frame == 0)
				pevent[j].cycle		= 0;
			else
			{
				MdlWarning("Event %d (frame %d) out of range in %s\n", g_sequence[i].event[j].event, g_sequence[i].event[j].frame, g_sequence[i].name );
				bErrors = true;
			}

			//Adrian - Remove me once we phase out the old event system.
			if ( isdigit( g_sequence[i].event[j].eventname[0] ) )
			{
				 pevent[j].event = atoi( g_sequence[i].event[j].eventname );
				 pevent[j].type = 0;
				 pevent[j].szeventindex = 0;
			}
			else
			{
				 AddToStringTable( &pevent[j], &pevent[j].szeventindex, g_sequence[i].event[j].eventname );
				 pevent[j].type = NEW_EVENT_STYLE;
			}
				 						
			
			// printf("%4d : %d %f\n", pevent[j].event, g_sequence[i].event[j].frame, pevent[j].cycle );
			// AddToStringTable( &pevent[j], &pevent[j].szoptionindex, g_sequence[i].event[j].options );
			strcpy( pevent[j].options, g_sequence[i].event[j].options );
		}
		ALIGN4( pData );

		// save ikrules
		pseqdesc->numikrules	= g_sequence[i].numikrules;

		// save autolayers
		mstudioautolayer_t *pautolayer			= (mstudioautolayer_t *)pData;
		pseqdesc->numautolayers	= IsChar( g_sequence[i].numautolayers );
		pseqdesc->autolayerindex = IsInt24( (pData - pSequenceStart) );
		pData += pseqdesc->numautolayers * sizeof( mstudioautolayer_t );
		for (j = 0; j < g_sequence[i].numautolayers; j++)
		{
			pautolayer[j].iSequence = g_sequence[i].autolayer[j].sequence;
			pautolayer[j].iPose		= g_sequence[i].autolayer[j].pose;
			pautolayer[j].flags		= g_sequence[i].autolayer[j].flags;
			if (!(pautolayer[j].flags & STUDIO_AL_POSE))
			{
				pautolayer[j].start		= g_sequence[i].autolayer[j].start / (g_sequence[i].panim[0][0]->numframes - 1);
				pautolayer[j].peak		= g_sequence[i].autolayer[j].peak / (g_sequence[i].panim[0][0]->numframes - 1);
				pautolayer[j].tail		= g_sequence[i].autolayer[j].tail / (g_sequence[i].panim[0][0]->numframes - 1);
				pautolayer[j].end		= g_sequence[i].autolayer[j].end / (g_sequence[i].panim[0][0]->numframes - 1);
			}
			else
			{
				pautolayer[j].start		= g_sequence[i].autolayer[j].start;
				pautolayer[j].peak		= g_sequence[i].autolayer[j].peak;
				pautolayer[j].tail		= g_sequence[i].autolayer[j].tail;
				pautolayer[j].end		= g_sequence[i].autolayer[j].end;
			}
		}


		// save boneweights
		float *pweight = 0;
		j = 0;
		// look up previous sequence weights and try to find a match
		for (k = 0; k < i; k++)
		{
			j = 0;
			// only check newer boneweights than the last one
			if (pseqdesc[k-i].pBoneweight( 0 ) > pweight)
			{
				pweight = pseqdesc[k-i].pBoneweight( 0 );
				for (j = 0; j < g_numbones; j++)
				{
					if (g_sequence[i].weight[j] != g_sequence[k].weight[j])
						break;
				}
				if (j == g_numbones)
					break;
			}
		}

		// check to see if all the bones matched
		if (j < g_numbones)
		{
			// allocate new block
			//printf("new %08x\n", pData );
			pweight						= (float *)pData;
			pseqdesc->weightlistindex = (pData - pSequenceStart);
			pData += g_numbones * sizeof( float );
			for (j = 0; j < g_numbones; j++)
			{
				pweight[j] = g_sequence[i].weight[j];
			}
		}
		else
		{
			// use previous boneweight
			//printf("prev %08x\n", pweight );
			pseqdesc->weightlistindex = ((byte *)pweight - pSequenceStart);
		}


		// save iklocks
		mstudioiklock_t *piklock	= (mstudioiklock_t *)pData;
		pseqdesc->numiklocks		= IsChar( g_sequence[i].numiklocks );
		pseqdesc->iklockindex		= IsInt24( (pData - pSequenceStart) );
		pData += pseqdesc->numiklocks * sizeof( mstudioiklock_t );
		ALIGN4( pData );

		for (j = 0; j < pseqdesc->numiklocks; j++)
		{
			piklock->chain			= g_sequence[i].iklock[j].chain;
			piklock->flPosWeight	= g_sequence[i].iklock[j].flPosWeight;
			piklock->flLocalQWeight	= g_sequence[i].iklock[j].flLocalQWeight;
			piklock++;
		}

		short *blends = ( short * )pData;
		pseqdesc->animindexindex = ( pData - pSequenceStart );
		pData += ( g_sequence[i].groupsize[0] * g_sequence[i].groupsize[1] ) * sizeof( short );
		ALIGN4( pData );

		for ( j = 0; j < g_sequence[i].groupsize[0] ; j++ )
		{
			for ( k = 0; k < g_sequence[i].groupsize[1]; k++ )
			{
				// height value * width of row + width value
				int offset = k * g_sequence[i].groupsize[0] + j;

				if ( g_sequence[i].panim[j][k] )
				{
					int animindex = g_sequence[i].panim[j][k]->index;

					Assert( animindex >= 0 && animindex < SHRT_MAX );

					blends[ offset ] = (short)animindex;
				}
				else
				{
					blends[ offset ] = 0;
				}
			}
		}

		WriteSeqKeyValues( pseqdesc, &g_sequence[i].KeyValue );
	}

	if (bErrors)
	{
		MdlError( "Exiting due to Errors\n");
	}

	// save transition graph
	int *pxnodename = (int *)pData;
	phdr->localnodenameindex = (pData - pStart);
	pData += g_numxnodes * sizeof( *pxnodename );
	ALIGN4( pData );
	for (i = 0; i < g_numxnodes; i++)
	{
		AddToStringTable( phdr, pxnodename, g_xnodename[i+1] );
		// printf("%d : %s\n", i, g_xnodename[i+1] );
		pxnodename++;
	}

	ptransition	= (byte *)pData;
	phdr->numlocalnodes = IsChar( g_numxnodes );
	phdr->localnodeindex = IsInt24( pData - pStart );
	pData += g_numxnodes * g_numxnodes * sizeof( byte );
	ALIGN4( pData );
	for (i = 0; i < g_numxnodes; i++)
	{
//		printf("%2d (%12s) : ", i + 1, g_xnodename[i+1] );
		for (j = 0; j < g_numxnodes; j++)
		{
			*ptransition++ = g_xnode[i][j];
//			printf(" %2d", g_xnode[i][j] );
		}
//		printf("\n" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Stub implementation
// Input  : *group - 
//-----------------------------------------------------------------------------

const studiohdr_t *studiohdr_t::FindModel( void **cache, char const *modelname ) const
{
	return NULL;
}

virtualmodel_t *studiohdr_t::GetVirtualModel( void ) const
{
	return NULL;
}

const studiohdr_t *virtualgroup_t::GetStudioHdr( void ) const
{
	return (studiohdr_t *)cache;
}

byte *studiohdr_t::GetAnimBlock( int i ) const
{
	return NULL;
}

int	studiohdr_t::GetAutoplayList( unsigned short **pOut ) const
{
	return 0;
}


int rawanimbytes = 0;
int animboneframes = 0;

int numAxis[4] = { 0, 0, 0, 0 };
int numPos[4] = { 0, 0, 0, 0 };
int useRaw = 0;

byte *WriteAnimationData( s_animation_t *srcanim, byte *pData )
{
	int j, k, n;

	mstudioanim_t	*destanim = (mstudioanim_t *)pData;

	pData += sizeof( *destanim );
	destanim->bone = 255;

	mstudioanim_t	*prevanim = NULL;

	// save animation value info
	for (j = 0; j < g_numbones; j++)
	{
		// destanim->weight = srcanim->weight[j];
		// printf( "%s %.1f\n", g_bonetable[j].name, destanim->weight );
		destanim->flags = 0;

		numPos[ (srcanim->numanim[j][0] != 0) + (srcanim->numanim[j][1] != 0) + (srcanim->numanim[j][2] != 0) ]++;
		numAxis[ (srcanim->numanim[j][3] != 0) + (srcanim->numanim[j][4] != 0) + (srcanim->numanim[j][5] != 0) ]++;

		if (srcanim->numanim[j][0] + srcanim->numanim[j][1] + srcanim->numanim[j][2] + srcanim->numanim[j][3] + srcanim->numanim[j][4] + srcanim->numanim[j][5] == 0)
		{
			// no animation, skip
			continue;
		}

		destanim->bone = j;

		// copy flags over if delta animation
		if (srcanim->flags & STUDIO_DELTA)
		{
			destanim->flags |= STUDIO_ANIM_DELTA;
		}

		if (srcanim->numframes == 1)
		{
			// single frame, if animation detected just store as raw
			if (srcanim->numanim[j][3] != 0 || srcanim->numanim[j][4] != 0 || srcanim->numanim[j][5] != 0)
			{
				Quaternion q;
				AngleQuaternion( srcanim->sanim[0][j].rot, q );
				*((Quaternion48 *)pData) = q;
				pData += sizeof( Quaternion48 );
				destanim->flags |= STUDIO_ANIM_RAWROT;
				rawanimbytes += sizeof( Quaternion48 );
			}

			if (srcanim->numanim[j][0] != 0 || srcanim->numanim[j][1] != 0 || srcanim->numanim[j][2] != 0)
			{
				*((Vector48 *)pData) = srcanim->sanim[0][j].pos;
				pData += sizeof( Vector48 );
				destanim->flags |= STUDIO_ANIM_RAWPOS;
				rawanimbytes += sizeof( Vector48 );
			}
		}
		else
		{
			// look to see if storing raw quat's would have taken less space
			if (srcanim->numanim[j][3] >= srcanim->numframes && srcanim->numanim[j][4] >= srcanim->numframes && srcanim->numanim[j][5] >= srcanim->numframes)
			{
				useRaw++;
			}

			mstudioanim_valueptr_t *posvptr	= NULL;
			mstudioanim_valueptr_t *rotvptr	= NULL;

			// allocate room for rotation ptrs
			rotvptr	= (mstudioanim_valueptr_t *)pData;
			pData += sizeof( *rotvptr );

			// skip all position info if there's no animation
			if (srcanim->numanim[j][0] != 0 || srcanim->numanim[j][1] != 0 || srcanim->numanim[j][2] != 0)
			{
				posvptr	= (mstudioanim_valueptr_t *)pData;
				pData += sizeof( *posvptr );
			}

			mstudioanimvalue_t	*destanimvalue = (mstudioanimvalue_t *)pData;

			if (rotvptr)
			{
				// store position animations
				for (k = 3; k < 6; k++)
				{
					if (srcanim->numanim[j][k] == 0)
					{
						rotvptr->offset[k-3] = 0;
					}
					else
					{
						rotvptr->offset[k-3] = ((byte *)destanimvalue - (byte *)rotvptr);
						for (n = 0; n < srcanim->numanim[j][k]; n++)
						{
							destanimvalue->value = srcanim->anim[j][k][n].value;
							destanimvalue++;
						}
					}
				}
				destanim->flags |= STUDIO_ANIM_ANIMROT;
			}

			if (posvptr)
			{
				// store rotation animations
				for (k = 0; k < 3; k++)
				{
					if (srcanim->numanim[j][k] == 0)
					{
						posvptr->offset[k] = 0;
					}
					else
					{
						posvptr->offset[k] = ((byte *)destanimvalue - (byte *)posvptr);
						for (n = 0; n < srcanim->numanim[j][k]; n++)
						{
							destanimvalue->value = srcanim->anim[j][k][n].value;
							destanimvalue++;
						}
					}
				}
				destanim->flags |= STUDIO_ANIM_ANIMPOS;
			}

			rawanimbytes += ((byte *)destanimvalue - pData);
			pData = (byte *)destanimvalue;
		}

		prevanim					= destanim;
		destanim->nextoffset		= pData - (byte *)destanim;
		destanim					= (mstudioanim_t *)pData;
		pData						+= sizeof( *destanim );
	}

	if (prevanim)
	{
		prevanim->nextoffset		= 0;
	}

	return pData;
}


byte *WriteIkErrors( s_animation_t *srcanim, byte *pData )
{
	int j, k;

	// write IK error keys
	mstudioikrule_t *pikruledata = (mstudioikrule_t *)pData;
	pData += srcanim->numikrules * sizeof( *pikruledata );
	ALIGN4( pData );

	for (j = 0; j < srcanim->numikrules; j++)
	{
		mstudioikrule_t *pikrule = pikruledata + j;

		pikrule->index	= srcanim->ikrule[j].index;

		pikrule->chain	= srcanim->ikrule[j].chain;
		pikrule->bone	= srcanim->ikrule[j].bone;
		pikrule->type	= srcanim->ikrule[j].type;
		pikrule->slot	= srcanim->ikrule[j].slot;
		pikrule->pos	= srcanim->ikrule[j].pos;
		pikrule->q		= srcanim->ikrule[j].q;
		pikrule->height	= srcanim->ikrule[j].height;
		pikrule->floor	= srcanim->ikrule[j].floor;
		pikrule->radius = srcanim->ikrule[j].radius;

		if (srcanim->numframes > 1.0)
		{
			pikrule->start	= srcanim->ikrule[j].start / (srcanim->numframes - 1.0f);
			pikrule->peak	= srcanim->ikrule[j].peak / (srcanim->numframes - 1.0f);
			pikrule->tail	= srcanim->ikrule[j].tail / (srcanim->numframes - 1.0f);
			pikrule->end	= srcanim->ikrule[j].end / (srcanim->numframes - 1.0f);
			pikrule->contact= srcanim->ikrule[j].contact / (srcanim->numframes - 1.0f);
		}
		else
		{
			pikrule->start	= 0.0f;
			pikrule->peak	= 0.0f;
			pikrule->tail	= 1.0f;
			pikrule->end	= 1.0f;
			pikrule->contact= 0.0f;
		}

		/*
		printf("%d %d %d %d : %.2f %.2f %.2f %.2f\n", 
			srcanim->ikrule[j].start, srcanim->ikrule[j].peak, srcanim->ikrule[j].tail, srcanim->ikrule[j].end, 
			pikrule->start, pikrule->peak, pikrule->tail, pikrule->end );
		*/

		pikrule->iStart = srcanim->ikrule[j].start;

#if 0
		// uncompressed
		pikrule->ikerrorindex = (pData - (byte*)pikrule);
		mstudioikerror_t *perror = (mstudioikerror_t *)pData;
		pData += srcanim->ikrule[j].numerror * sizeof( *perror );

		for (k = 0; k < srcanim->ikrule[j].numerror; k++)
		{
			perror[k].pos = srcanim->ikrule[j].pError[k].pos;
			perror[k].q = srcanim->ikrule[j].pError[k].q;
		}
#endif
#if 1
		// skip writting the header if there's no IK data
		for (k = 0; k < 6; k++)
		{
			if (srcanim->ikrule[j].numanim[k]) break;
		}

		if (k == 6)
			continue;

		// compressed
		pikrule->compressedikerrorindex = (pData - (byte*)pikrule);
		mstudiocompressedikerror_t *pCompressed = (mstudiocompressedikerror_t *)pData;
		pData += sizeof( *pCompressed );

		for (k = 0; k < 6; k++)
		{
			pCompressed->scale[k] = srcanim->ikrule[j].scale[k];
			pCompressed->offset[k] = (pData - (byte*)pCompressed);
			int size = srcanim->ikrule[j].numanim[k] * sizeof( mstudioanimvalue_t );
			memmove( pData, srcanim->ikrule[j].anim[k], size );
			pData += size;
		}

		if (strlen( srcanim->ikrule[j].attachment ) > 0)
		{
			// don't use string table, we're probably not in the same file.
			int size = strlen( srcanim->ikrule[j].attachment ) + 1;
			strcpy( (char *)pData, srcanim->ikrule[j].attachment );
			pikrule->szattachmentindex = pData - (byte *)pikrule;
			pData += size;
		}

		ALIGN4( pData );

#endif
		// AddToStringTable( pikrule, &pikrule->szattachmentindex, srcanim->ikrule[j].attachment );
	}

	return pData;
}



static byte *WriteAnimations( byte *pData, byte *pStart, int group, studiohdr_t *phdr, int *outcount )
{
	int i, j;

	mstudioanimdesc_t	*panimdesc;

	CUtlVector< s_animation_t * > anims;
	for (i = 0; i < g_numani; i++) 
	{
		if ( g_panimation[i]->animgroup != group )
			continue;

		anims.AddToTail( g_panimation[i] );
	}

	int animcount = anims.Count();

	// Assert( animcount > 0 );

	if ( outcount )
	{
		*outcount = animcount;
	}

	// save animations
	panimdesc = (mstudioanimdesc_t *)pData;
	if( phdr )
	{
		phdr->numlocalanim = animcount;
		phdr->localanimindex = (pData - pStart);
	}
	pData += animcount * sizeof( *panimdesc );
	ALIGN4( pData );

	//      ------------ ------- ------- : ------- (-------)
	if( g_verbose )
	{
		printf("   animation       x       y       ips    angle\n");
	}

	for (i = 0; i < animcount; i++) 
	{
		s_animation_t *srcanim = anims[ i ];
		Assert( srcanim );

		AddToStringTable( &panimdesc[i], &panimdesc[i].sznameindex, srcanim->name );

		panimdesc[i].baseptr	= pStart - (byte *)&panimdesc[i];
		panimdesc[i].fps		= srcanim->fps;
		panimdesc[i].flags		= srcanim->flags;

		totalframes				+= srcanim->numframes;
		totalseconds			+= srcanim->numframes / srcanim->fps;

		panimdesc[i].numframes	= srcanim->numframes;

		// panimdesc[i].motiontype = srcanim->motiontype;	
		// panimdesc[i].motionbone = srcanim->motionbone;
		// VectorCopy( srcanim->linearpos, panimdesc[i].linearpos );

		j = srcanim->numpiecewisekeys - 1;
		if (srcanim->piecewisemove[j].pos[0] != 0 || srcanim->piecewisemove[j].pos[1] != 0) 
		{
			float t = (srcanim->numframes - 1) / srcanim->fps;

			float r = 1 / t;
			
			float a = atan2( srcanim->piecewisemove[j].pos[1], srcanim->piecewisemove[j].pos[0] ) * (180 / M_PI);
			float d = sqrt( DotProduct( srcanim->piecewisemove[j].pos, srcanim->piecewisemove[j].pos ) );
			if( g_verbose )
			{
				printf("%12s %7.2f %7.2f : %7.2f (%7.2f) %.1f\n", srcanim->name, srcanim->piecewisemove[j].pos[0], srcanim->piecewisemove[j].pos[1], d * r, a, t );
			}
		}

		// VectorCopy( srcanim->linearrot, panimdesc[i].linearrot );
		// panimdesc[i].automoveposindex = srcanim->automoveposindex;
		// panimdesc[i].automoveangleindex = srcanim->automoveangleindex;

		if (!pBlockStart)
		{
			panimdesc[i].animindex	= IsInt24( pData - (byte *)(&panimdesc[i]) );
			pData = WriteAnimationData( srcanim, pData );
			if ( srcanim->numikrules )
			{
				panimdesc[i].ikruleindex = IsInt24( pData - (byte *)(&panimdesc[i]) );
				panimdesc[i].numikrules = IsChar( srcanim->numikrules );
				pData = WriteIkErrors( srcanim, pData );
			}
		}
		else
		{
			static int iCurAnim = 0;

			// align all animation data to cache line boundaries
			ALIGN16( pBlockData );

			byte *pIkData = WriteAnimationData( srcanim, pBlockData );
			byte *pBlockEnd = WriteIkErrors( srcanim, pIkData );

			if (g_numanimblocks == 0)
			{
				g_numanimblocks = 1;
				// XBox, align each anim block to 512 for fast io
				byte *pBlockData2 = pBlockData;
				ALIGN512( pBlockData2 );

				int size = pBlockEnd - pBlockData;
				int shift = pBlockData2 - pBlockData;

				memmove( pBlockData2, pBlockData, size );
				memset( pBlockData, 0, shift );

				pBlockData = pBlockData2;
				pIkData = pIkData + shift;
				pBlockEnd = pBlockEnd + shift;

				g_animblock[g_numanimblocks].start = pBlockData;
				g_numanimblocks++;
			}
			else if (pBlockEnd - g_animblock[g_numanimblocks-1].start > g_animblocksize)
			{
				// the data we just wrote went over the boundry
				// XBox, align each anim block to 512 for fast io
				byte *pBlockData2 = pBlockData;
				ALIGN512( pBlockData2 );

				int size = pBlockEnd - pBlockData;
				int shift = pBlockData2 - pBlockData;

				memmove( pBlockData2, pBlockData, size );
				memset( pBlockData, 0, shift );

				pBlockData = pBlockData2;
				pIkData = pIkData + shift;
				pBlockEnd = pBlockEnd + shift;

				g_animblock[g_numanimblocks-1].end = pBlockData;
				g_animblock[g_numanimblocks].start = pBlockData;
				g_animblock[g_numanimblocks].iStartAnim = i;

				g_numanimblocks++;
				if (g_numanimblocks > MAXSTUDIOANIMBLOCKS)
				{
					MdlError( "Too many animation blocks\n");
				}
			}

			if ( i == animcount - 1 )
			{
				// fixup size for last block
				// XBox, align each anim block to 512 for fast io 
				ALIGN512( pBlockEnd );
			}
			g_animblock[g_numanimblocks-1].iEndAnim = i;
			g_animblock[g_numanimblocks-1].end = pBlockEnd;

			panimdesc[i].animblock	= IsChar( g_numanimblocks-1 );
			panimdesc[i].animindex	= IsInt24( pBlockData - g_animblock[panimdesc[i].animblock].start );
			panimdesc[i].numikrules = IsChar( srcanim->numikrules );
			panimdesc[i].animblockikruleindex = IsInt24( pIkData - g_animblock[panimdesc[i].animblock].start );
			pBlockData = pBlockEnd;
		}

		// printf("raw bone data %d : %s\n", (byte *)destanimvalue - pData, srcanim->name);
	}

	if( !g_quiet )
	{
		/*
		for (i = 0; i < g_numanimblocks; i++)
		{
			printf("%2d (%3d:%3d): %d\n", i, g_animblock[i].iStartAnim, g_animblock[i].iEndAnim, g_animblock[i].end - g_animblock[i].start );
		}
		*/
	}

	if( !g_quiet )
	{
		/*
		printf("raw anim data %d : %d\n", rawanimbytes, animboneframes );
		printf("pos  %d %d %d %d\n", numPos[0], numPos[1], numPos[2], numPos[3] );
		printf("axis %d %d %d %d : %d\n", numAxis[0], numAxis[1], numAxis[2], numAxis[3], useRaw );
		*/
	}

	// write movement keys
	for (i = 0; i < animcount; i++) 
	{
		s_animation_t *anim = anims[ i ];

		// panimdesc[i].entrancevelocity = anim->entrancevelocity;
		panimdesc[i].nummovements = IsChar( anim->numpiecewisekeys );
		panimdesc[i].movementindex = IsInt24( pData - (byte*)&panimdesc[i] );

		mstudiomovement_t	*pmove = (mstudiomovement_t *)pData;
		pData += panimdesc[i].nummovements * sizeof( *pmove );
		ALIGN4( pData );

		for (j = 0; j < panimdesc[i].nummovements; j++)
		{
			pmove[j].endframe		= anim->piecewisemove[j].endframe;
			pmove[j].motionflags	= anim->piecewisemove[j].flags;
			pmove[j].v0				= anim->piecewisemove[j].v0;
			pmove[j].v1				= anim->piecewisemove[j].v1;
			pmove[j].angle			= RAD2DEG( anim->piecewisemove[j].rot[2] );
			VectorCopy( anim->piecewisemove[j].vector, pmove[j].vector );
			VectorCopy( anim->piecewisemove[j].pos, pmove[j].position );

		}
	}

	// only write zero frames if the animation data is demand loaded
	if (!pBlockStart)
		return pData;


	// calculate what bones should be have zero frame saved out
	if (g_bonesaveframe.Count() == 0)
	{
		for (j = 0; j < g_numbones; j++)
		{
			if ((g_bonetable[j].parent == -1) || (g_bonetable[j].posrange.Length() > 2.0))
			{
				g_bonetable[j].flags |= BONE_HAS_SAVEFRAME_POS;
			}
			g_bonetable[j].flags |= BONE_HAS_SAVEFRAME_ROT;

			if ((!g_quiet) && (g_bonetable[j].flags & (BONE_HAS_SAVEFRAME_POS | BONE_HAS_SAVEFRAME_ROT)))
			{
				printf("$BoneSaveFrame \"%s\"", g_bonetable[j].name );
				if (g_bonetable[j].flags & BONE_HAS_SAVEFRAME_POS)
					printf(" position" );
				if (g_bonetable[j].flags & BONE_HAS_SAVEFRAME_ROT)
					printf(" rotation" );
				printf("\n");
			}
		}
	}
	else
	{
		for (i = 0; i < g_bonesaveframe.Count(); i++)
		{
			j = findGlobalBone( g_bonesaveframe[i].name );

			if (j != -1)
			{
				if (g_bonesaveframe[i].bSavePos)
				{
					g_bonetable[j].flags |= BONE_HAS_SAVEFRAME_POS;
					phdr->pBone(j)->flags |= BONE_HAS_SAVEFRAME_POS;
				}
				if (g_bonesaveframe[i].bSaveRot)
				{
					g_bonetable[j].flags |= BONE_HAS_SAVEFRAME_ROT;
					phdr->pBone(j)->flags |= BONE_HAS_SAVEFRAME_ROT;
				}
			}
		}
	}

	// write zero frames
	int *pZeroframeindex = (int *)pData;
	phdr->zeroframecacheindex = (byte *)pZeroframeindex - (byte *)phdr;
	pData += animcount * sizeof( int );

	for (i = 0; i < animcount; i++) 
	{
		s_animation_t *anim = anims[ i ];

		pZeroframeindex[i] = pData - pStart;

		for (j = 0; j < g_numbones; j++)
		{
			if (g_bonetable[j].flags & BONE_HAS_SAVEFRAME_POS)
			{
				*(Vector48 *)pData = anim->sanim[0][j].pos;
				pData += sizeof( Vector48 );
			}
			if (g_bonetable[j].flags & BONE_HAS_SAVEFRAME_ROT)
			{
				if ((anim->flags & STUDIO_DELTA) == 0)
				{
					Quaternion q;
					AngleQuaternion( anim->sanim[0][j].rot, q );
					*((Quaternion32 *)pData) = q;
					pData += sizeof( Quaternion48 );
				}
			}
		}
	}

	return pData;
}

static void WriteTextures( studiohdr_t *phdr )
{
	int i, j;
	short	*pref;

	// save texture info
	mstudiotexture_t *ptexture = (mstudiotexture_t *)pData;
	phdr->numtextures = IsChar( g_nummaterials );
	phdr->textureindex = IsInt24( pData - pStart );
	pData += g_nummaterials * sizeof( mstudiotexture_t );
	for (i = 0; i < g_nummaterials; i++) 
	{
		j = g_material[i];
		AddToStringTable( &ptexture[i], &ptexture[i].sznameindex, g_texture[j].name );
	}
	ALIGN4( pData );

	int *cdtextureoffset = (int *)pData;
	phdr->numcdtextures = IsChar( numcdtextures );
	phdr->cdtextureindex = IsInt24( pData - pStart );
	pData += numcdtextures * sizeof( int );
	for (i = 0; i < numcdtextures; i++) 
	{
		AddToStringTable( phdr, &cdtextureoffset[i], cdtextures[i] );
	}
	ALIGN4( pData );

	// save texture directory info
	phdr->skinindex = (pData - pStart);
	phdr->numskinref = g_numskinref;
	phdr->numskinfamilies = g_numskinfamilies;
	pref = (short *)pData;

	for (i = 0; i < phdr->numskinfamilies; i++) 
	{
		for (j = 0; j < phdr->numskinref; j++) 
		{
			*pref = g_skinref[i][j];
			pref++;
		}
	}
	pData = (byte *)pref;
	ALIGN4( pData );
}

//-----------------------------------------------------------------------------
// Write the processed vertices
//-----------------------------------------------------------------------------
static void WriteVertices( studiohdr_t *phdr )
{
	char			fileName[260];
	s_loddata_t		*pLodDataSrc;
	byte			*pStart;
	byte			*pData;
	int				i;
	int				j;
	int				k;
	int				cur;

	if (!g_nummodelsbeforeLOD)
		return;

	strcpy( fileName, gamedir );
//	if( *g_pPlatformName )
//	{
//		strcat( fileName, "platform_" );
//		strcat( fileName, g_pPlatformName );
//		strcat( fileName, "/" );	
//	}
	strcat( fileName, "models/" );	
	strcat( fileName, outname );
	Q_StripExtension( fileName, fileName, sizeof( fileName ) );
	strcat( fileName, ".vvd" );

	if ( !g_quiet )
	{
		printf ("---------------------\n");
		printf ("writing %s:\n", fileName);
	}

	pStart = (byte *)kalloc( 1, FILEBUFFER );
	pData  = pStart;

	vertexFileHeader_t *fileHeader = (vertexFileHeader_t *)pData;
	pData += sizeof(vertexFileHeader_t);

	fileHeader->id        = MODEL_VERTEX_FILE_ID;
	fileHeader->version   = MODEL_VERTEX_FILE_VERSION;
	fileHeader->checksum  = phdr->checksum;

	// data has no fixes and requires no fixes
	fileHeader->numFixups       = 0;
	fileHeader->fixupTableStart = 0;

	// unfinalized during first pass, fixed during second pass
	// data can be considered as single lod at lod 0
	fileHeader->numLODs = 1;
	fileHeader->numLODVertexes[0] = 0;

	// store vertexes grouped by mesh order
	ALIGN16( pData );
	fileHeader->vertexDataStart  = pData-pStart;
	for (i = 0; i < g_nummodelsbeforeLOD; i++) 
	{
		pLodDataSrc = g_model[i]->source->pLodData;
		if (!pLodDataSrc)
		{
			// skip blank empty model
			continue;
		}

		// save vertices
		ALIGN16( pData );
		cur = (int)pData;
		mstudiovertex_t *pVert = (mstudiovertex_t *)pData;
		pData += pLodDataSrc->numvertices * sizeof( mstudiovertex_t );
		for (j = 0; j < pLodDataSrc->numvertices; j++)
		{
//			printf( "saving bone weight %d for model %d at 0x%p\n",
//				j, i, &pbone[j] );

			VectorCopy( pLodDataSrc->vertex[j].position, pVert[j].m_vecPosition );
			VectorCopy( pLodDataSrc->vertex[j].normal, pVert[j].m_vecNormal );
			Vector2DCopy( pLodDataSrc->vertex[j].texcoord, pVert[j].m_vecTexCoord );

			mstudioboneweight_t *pBoneWeight = &pVert[j].m_BoneWeights;
			memset( pBoneWeight, 0, sizeof( mstudioboneweight_t ) );
			pBoneWeight->numbones = pLodDataSrc->vertex[j].globalBoneweight.numbones;
			for (k = 0; k < pBoneWeight->numbones; k++)
			{
				pBoneWeight->bone[k]   = pLodDataSrc->vertex[j].globalBoneweight.bone[k];
				pBoneWeight->weight[k] = pLodDataSrc->vertex[j].globalBoneweight.weight[k];
			}
		}

		fileHeader->numLODVertexes[0] += pLodDataSrc->numvertices;

		if (!g_quiet)
		{
			printf( "vertices   %7d bytes (%d vertices)\n", pData - cur, pLodDataSrc->numvertices );
		}
	}

	// store tangents grouped by mesh order
	ALIGN4( pData );
	fileHeader->tangentDataStart = pData-pStart;
	for (i = 0; i < g_nummodelsbeforeLOD; i++) 
	{
		pLodDataSrc = g_model[i]->source->pLodData;
		if (!pLodDataSrc)
		{
			// skip blank empty model
			continue;
		}

		// save tangent space S
		ALIGN4( pData );
		cur = (int)pData;
		Vector4D *ptangents = (Vector4D *)pData;
		pData += pLodDataSrc->numvertices * sizeof( Vector4D );
		for (j = 0; j < pLodDataSrc->numvertices; j++)
		{
			Vector4DCopy( pLodDataSrc->vertex[j].tangentS, ptangents[j] );
#ifdef _DEBUG
			float w = ptangents[j].w;
			Assert( w == 1.0f || w == -1.0f );
#endif
		}

		if (!g_quiet)
		{
			printf( "tangents   %7d bytes (%d vertices)\n", pData - cur, pLodDataSrc->numvertices );
		}
	}

	if (!g_quiet)
	{
		printf( "total      %7d bytes\n", pData - pStart );
	}

	// fileHeader->length = pData - pStart;
	SaveFile( fileName, pStart, pData - pStart );
}

static void WriteModel( studiohdr_t *phdr )
{
	int i, j, k, m;
	mstudiobodyparts_t	*pbodypart;
	mstudiomodel_t		*pmodel;
	s_source_t			*psource;
	s_loddata_t			*pLodDataSrc;
	mstudiovertanim_t	*pvertanim;
	s_vertanim_t		*pvanim;

	int cur	= (int)pData;

	// vertex data is written to external file, offsets kept internal
	// track expected external base to store proper offsets
	byte *externalVertexIndex   = 0;
	byte *externalTangentsIndex = 0;

	// write bodypart info
	pbodypart = (mstudiobodyparts_t *)pData;
	phdr->numbodyparts = IsChar( g_numbodyparts );
	phdr->bodypartindex = IsInt24( pData - pStart );
	pData += g_numbodyparts * sizeof( mstudiobodyparts_t );

	pmodel = (mstudiomodel_t *)pData;
	pData += g_nummodelsbeforeLOD * sizeof( mstudiomodel_t );

	for (i = 0, j = 0; i < g_numbodyparts; i++)
	{
		AddToStringTable( &pbodypart[i], &pbodypart[i].sznameindex, g_bodypart[i].name );
		pbodypart[i].nummodels		= IsChar( g_bodypart[i].nummodels );
		pbodypart[i].base			= g_bodypart[i].base;
		pbodypart[i].modelindex		= IsInt24( ((byte *)&pmodel[j]) - (byte *)&pbodypart[i] );
		j += g_bodypart[i].nummodels;
	}
	ALIGN4( pData );

	// write global flex names
	mstudioflexdesc_t *pflexdesc = (mstudioflexdesc_t *)pData;
	phdr->numflexdesc			= IsInt24( g_numflexdesc );
	phdr->flexdescindex			= IsInt24( pData - pStart );
	pData += g_numflexdesc * sizeof( mstudioflexdesc_t );
	ALIGN4( pData );

	for (j = 0; j < g_numflexdesc; j++)
	{
		// printf("%d %s\n", j, g_flexdesc[j].FACS );
		AddToStringTable( pflexdesc, &pflexdesc->szFACSindex, g_flexdesc[j].FACS );
		pflexdesc++;
	}

	// write global flex controllers
	mstudioflexcontroller_t *pflexcontroller = (mstudioflexcontroller_t *)pData;
	phdr->numflexcontrollers	= IsChar( g_numflexcontrollers );
	phdr->flexcontrollerindex	= IsInt24( pData - pStart );
	pData += g_numflexcontrollers * sizeof( mstudioflexcontroller_t );
	ALIGN4( pData );

	for (j = 0; j < g_numflexcontrollers; j++)
	{
		AddToStringTable( pflexcontroller, &pflexcontroller->sznameindex, g_flexcontroller[j].name );
		AddToStringTable( pflexcontroller, &pflexcontroller->sztypeindex, g_flexcontroller[j].type );
		pflexcontroller->min = g_flexcontroller[j].min;
		pflexcontroller->max = g_flexcontroller[j].max;
		pflexcontroller->link = -1;
		pflexcontroller++;
	}

	// write flex rules
	mstudioflexrule_t *pflexrule = (mstudioflexrule_t *)pData;
	phdr->numflexrules			= IsInt24( g_numflexrules );
	phdr->flexruleindex			= IsInt24( pData - pStart );
	pData += g_numflexrules * sizeof( mstudioflexrule_t );
	ALIGN4( pData );

	for (j = 0; j < g_numflexrules; j++)
	{
		pflexrule->flex		= g_flexrule[j].flex;
		pflexrule->numops	= g_flexrule[j].numops;
		pflexrule->opindex	= (pData - (byte *)pflexrule);

		mstudioflexop_t *pflexop = (mstudioflexop_t *)pData;

		for (i = 0; i < pflexrule->numops; i++)
		{
			pflexop[i].op = g_flexrule[j].op[i].op;
			pflexop[i].d.index = g_flexrule[j].op[i].d.index;
		}

		pData += sizeof( mstudioflexop_t ) * pflexrule->numops;
		ALIGN4( pData );

		pflexrule++;
	}

	// write ik chains
	mstudioikchain_t *pikchain = (mstudioikchain_t *)pData;
	phdr->numikchains			= IsChar( g_numikchains );
	phdr->ikchainindex			= IsInt24( pData - pStart );
	pData += g_numikchains * sizeof( mstudioikchain_t );
	ALIGN4( pData );

	for (j = 0; j < g_numikchains; j++)
	{
		AddToStringTable( pikchain, &pikchain->sznameindex, g_ikchain[j].name );
		pikchain->numlinks		= g_ikchain[j].numlinks;

		mstudioiklink_t *piklink = (mstudioiklink_t *)pData;
		pikchain->linkindex		= (pData - (byte *)pikchain);
		pData += pikchain->numlinks * sizeof( mstudioiklink_t );

		for (i = 0; i < pikchain->numlinks; i++)
		{
			piklink[i].bone = g_ikchain[j].link[i].bone;
			piklink[i].kneeDir = g_ikchain[j].link[i].kneeDir;
		}

		pikchain++;
	}

	// save autoplay locks
	mstudioiklock_t *piklock = (mstudioiklock_t *)pData;
	phdr->numlocalikautoplaylocks	= IsChar( g_numikautoplaylocks );
	phdr->localikautoplaylockindex	= IsInt24( pData - pStart );
	pData += g_numikautoplaylocks * sizeof( mstudioiklock_t );
	ALIGN4( pData );

	for (j = 0; j < g_numikautoplaylocks; j++)
	{
		piklock->chain			= g_ikautoplaylock[j].chain;
		piklock->flPosWeight	= g_ikautoplaylock[j].flPosWeight;
		piklock->flLocalQWeight	= g_ikautoplaylock[j].flLocalQWeight;
		piklock++;
	}

	// save mouth info
	mstudiomouth_t *pmouth = (mstudiomouth_t *)pData;
	phdr->nummouths = IsChar( g_nummouths );
	phdr->mouthindex = IsInt24( pData - pStart );
	pData += g_nummouths * sizeof( mstudiomouth_t );
	ALIGN4( pData );

	for (i = 0; i < g_nummouths; i++) {
		pmouth[i].bone			= g_mouth[i].bone;
		VectorCopy( g_mouth[i].forward, pmouth[i].forward );
		pmouth[i].flexdesc = g_mouth[i].flexdesc;
	}

	// save pose parameters
	mstudioposeparamdesc_t *ppose = (mstudioposeparamdesc_t *)pData;
	phdr->numlocalposeparameters = IsChar( g_numposeparameters );
	phdr->localposeparamindex = IsInt24( pData - pStart );
	pData += g_numposeparameters * sizeof( mstudioposeparamdesc_t );
	ALIGN4( pData );

	for (i = 0; i < g_numposeparameters; i++)
	{
		AddToStringTable( &ppose[i], &ppose[i].sznameindex, g_pose[i].name );
		ppose[i].start	= g_pose[i].min;
		ppose[i].end	= g_pose[i].max;
		ppose[i].flags	= g_pose[i].flags;
		ppose[i].loop	= g_pose[i].loop;
	}

	if( !g_quiet )
	{
		printf("ik/pose    %7d bytes\n", pData - cur );
	}
	cur = (int)pData;

	// write model
	for (i = 0; i < g_nummodelsbeforeLOD; i++) 
	{
		int n = 0;

		byte *pModelStart = (byte *)(&pmodel[i]);
		
		strcpy( pmodel[i].name, g_model[i]->filename );
		// AddToStringTable( &pmodel[i], &pmodel[i].sznameindex, g_model[i]->filename );

		// pmodel[i].mrmbias = g_model[i]->mrmbias;
		// pmodel[i].minresolution = g_model[i]->minresolution;
		// pmodel[i].maxresolution = g_model[i]->maxresolution;

		// save bbox info
		
		psource = g_model[i]->source;
		pLodDataSrc = psource->pLodData;

		// save mesh info
		if (pLodDataSrc)
		{
			pmodel[i].numvertices = pLodDataSrc->numvertices;
		}
		else
		{
			// empty model
			pmodel[i].numvertices = 0;
		}

		if ( pmodel[i].numvertices >= MAXSTUDIOVERTS )
		{
			// We have to check this here so that we don't screw up decal
			// vert caching in the runtime.
			MdlError( "Too many verts in model. (%d verts, MAXSTUDIOVERTS==%d)\n", 
				pmodel[i].numvertices, ( int )MAXSTUDIOVERTS );
		}

		mstudiomesh_t *pmesh = (mstudiomesh_t *)pData;
		pmodel[i].meshindex = (pData - pModelStart);
		pData += psource->nummeshes * sizeof( mstudiomesh_t );
		ALIGN4( pData );

		pmodel[i].nummeshes = psource->nummeshes;
		for (m = 0; m < pmodel[i].nummeshes; m++)
		{
			n = psource->meshindex[m];

			pmesh[m].material     = n;
			pmesh[m].modelindex   = (byte *)&pmodel[i] - (byte *)&pmesh[m];
			pmesh[m].numvertices  = pLodDataSrc->mesh[n].numvertices;
			pmesh[m].vertexoffset = pLodDataSrc->mesh[n].vertexoffset;
		}

		// set expected base offsets to external data
		ALIGN16( externalVertexIndex );
		pmodel[i].vertexindex = (int)externalVertexIndex; 
		externalVertexIndex += pmodel[i].numvertices * sizeof(mstudiovertex_t);

		// set expected base offsets to external data
		ALIGN4( externalTangentsIndex );
		pmodel[i].tangentsindex = (int)externalTangentsIndex;
		externalTangentsIndex += pmodel[i].numvertices * sizeof( Vector4D );

		cur = (int)pData;

		// save eyeballs
		mstudioeyeball_t *peyeball;
		peyeball					= (mstudioeyeball_t *)pData;
		pmodel[i].numeyeballs		= IsChar( g_model[i]->numeyeballs );
		pmodel[i].eyeballindex		= IsInt24( pData - pModelStart );
		pData += g_model[i]->numeyeballs * sizeof( mstudioeyeball_t );
			
		ALIGN4( pData );
		for (j = 0; j < g_model[i]->numeyeballs; j++)
		{
			k = g_model[i]->eyeball[j].mesh;
			pmesh[k].materialtype		= 1;	// FIXME: tag custom material
			pmesh[k].materialparam		= j;	// FIXME: tag custom material

			peyeball[j].bone			= g_model[i]->eyeball[j].bone;
			VectorCopy( g_model[i]->eyeball[j].org, peyeball[j].org );
			peyeball[j].zoffset			= g_model[i]->eyeball[j].zoffset;
			peyeball[j].radius			= g_model[i]->eyeball[j].radius;
			VectorCopy( g_model[i]->eyeball[j].up, peyeball[j].up );
			VectorCopy( g_model[i]->eyeball[j].forward, peyeball[j].forward );
			peyeball[j].iris_material	= g_model[i]->eyeball[j].iris_material;
			peyeball[j].iris_scale		= g_model[i]->eyeball[j].iris_scale;
			peyeball[j].glint_material	= g_model[i]->eyeball[j].glint_material;

			//peyeball[j].upperflex			= g_model[i]->eyeball[j].upperflex;
			//peyeball[j].lowerflex			= g_model[i]->eyeball[j].lowerflex;
			for (k = 0; k < 3; k++)
			{
				peyeball[j].upperflexdesc[k]	= g_model[i]->eyeball[j].upperflexdesc[k];
				peyeball[j].lowerflexdesc[k]	= g_model[i]->eyeball[j].lowerflexdesc[k];
				peyeball[j].uppertarget[k]		= g_model[i]->eyeball[j].uppertarget[k];
				peyeball[j].lowertarget[k]		= g_model[i]->eyeball[j].lowertarget[k];
			}

			peyeball[j].upperlidflexdesc	= g_model[i]->eyeball[j].upperlidflexdesc;
			peyeball[j].lowerlidflexdesc	= g_model[i]->eyeball[j].lowerlidflexdesc;
		}	

		if ( !g_quiet )
		{
			printf("eyeballs   %7d bytes (%d eyeballs)\n", pData - cur, g_model[i]->numeyeballs );
		}

		// move flexes into individual meshes
		cur = (int)pData;
		for (m = 0; m < pmodel[i].nummeshes; m++)
		{
			int numflexkeys[MAXSTUDIOFLEXKEYS];
			pmesh[m].numflexes = 0;

			// initialize array
			for (j = 0; j < g_numflexkeys; j++)
			{
				numflexkeys[j] = 0;
			}

			// count flex instances per mesh
			for (j = 0; j < g_numflexkeys; j++)
			{
				if (g_flexkey[j].imodel == i)
				{
					for (k = 0; k < g_flexkey[j].numvanims; k++)
					{
						n = g_flexkey[j].vanim[k].vertex - pmesh[m].vertexoffset;
						if (n >= 0 && n < pmesh[m].numvertices)
						{
							if (numflexkeys[j]++ == 0)
							{
								pmesh[m].numflexes++;
							}
						}
					}
				}
			}

			if (pmesh[m].numflexes)
			{
				pmesh[m].flexindex	= IsInt24( pData - (byte *)&pmesh[m] );
				mstudioflex_t *pflex = (mstudioflex_t *)pData;
				pData += pmesh[m].numflexes * sizeof( mstudioflex_t );
				ALIGN4( pData );

				for (j = 0; j < g_numflexkeys; j++)
				{
					if (!numflexkeys[j])
						continue;

					pflex->flexdesc		= g_flexkey[j].flexdesc;
					pflex->target0		= g_flexkey[j].target0;
					pflex->target1		= g_flexkey[j].target1;
					pflex->target2		= g_flexkey[j].target2;
					pflex->target3		= g_flexkey[j].target3;
					pflex->numverts		= numflexkeys[j];
					pflex->vertindex	= (pData - (byte *)pflex);
					pflex->flexpair		= g_flexkey[j].flexpair;
					// printf("%d %d %s : %f %f %f %f\n", j, g_flexkey[j].flexdesc, g_flexdesc[g_flexkey[j].flexdesc].FACS, g_flexkey[j].target0, g_flexkey[j].target1, g_flexkey[j].target2, g_flexkey[j].target3 );
					// if (j < 9) printf("%d %d %s : %d (%d) %f\n", j, g_flexkey[j].flexdesc, g_flexdesc[g_flexkey[j].flexdesc].FACS, g_flexkey[j].numvanims, pflex->numverts, g_flexkey[j].target );

					// printf("%d %d : %d %f\n", j, g_flexkey[j].flexnum, g_flexkey[j].numvanims, g_flexkey[j].target );

					pvertanim = (mstudiovertanim_t *)pData;
					pData += pflex->numverts * sizeof( mstudiovertanim_t );
					ALIGN4( pData );

					pvanim = g_flexkey[j].vanim;
				
					for (k = 0; k < g_flexkey[j].numvanims; k++)
					{
						n = g_flexkey[j].vanim[k].vertex - pmesh[m].vertexoffset;
						if (n >= 0 && n < pmesh[m].numvertices)
						{
							pvertanim->index = n;
							pvertanim->speed = 255.0F*pvanim->speed;
							pvertanim->side  = 255.0F*pvanim->side;
							pvertanim->SetDeltaFloat( pvanim->pos );
							pvertanim->SetNDeltaFloat( pvanim->normal );
							// Vector tmp = pvertanim->delta;

							pvertanim++;

							/*
							if ((tmp - pvanim->pos).Length() > 0.1)
							{	
								pvertanim->delta.x = pvanim->pos.x;
								printf("%f %f %f  : %f %f %f\n", 
									pvanim->pos[0], pvanim->pos[1], pvanim->pos[2],
									tmp.x, tmp.y, tmp.z );
							}
							*/
							// if (j < 9) printf("%d %.2f %.2f %.2f\n", n, pvanim->pos[0], pvanim->pos[1], pvanim->pos[2] );
						}
						// printf("%d %.2f %.2f %.2f\n", pvanim->vertex, pvanim->pos[0], pvanim->pos[1], pvanim->pos[2] );
						pvanim++;
					}
					pflex++;
				}
			}
		}

		if( !g_quiet )
		{
			printf("flexes     %7d bytes (%d flexes)\n", pData - cur, g_numflexkeys );
		}
		cur = (int)pData;
	}

	ALIGN4( pData );

	mstudiomodelgroup_t *pincludemodel = (mstudiomodelgroup_t *)pData;
	phdr->numincludemodels = IsChar( g_numincludemodels );
	phdr->includemodelindex = IsInt24( pData - pStart );
	pData += g_numincludemodels * sizeof( mstudiomodelgroup_t );

	for (i = 0; i < g_numincludemodels; i++)
	{
		AddToStringTable( pincludemodel, &pincludemodel->sznameindex, g_includemodel[i].name );
		pincludemodel++;
	}

	// save animblock group info
	mstudioanimblock_t *panimblock = (mstudioanimblock_t *)pData;
	phdr->numanimblocks = IsChar( g_numanimblocks );
	phdr->animblockindex = IsInt24( pData - pStart );
	pData += phdr->numanimblocks * sizeof( mstudioanimblock_t );
	ALIGN4( pData );

	for (i = 0; i < g_numanimblocks; i++) 
	{
		panimblock[i].datastart = g_animblock[i].start - pBlockStart;
		panimblock[i].dataend = g_animblock[i].end - pBlockStart;
		// printf("block %d : %x %x (%x)\n", i, panimblock[i].datastart, panimblock[i].dataend, panimblock[i].dataend - panimblock[i].datastart );
	}
	AddToStringTable( phdr, &phdr->szanimblocknameindex, g_animblockname );
}

static void AssignMeshIDs( studiohdr_t *pStudioHdr )
{
	int					i;
	int					j;
	int					m;
	int					numMeshes;
	mstudiobodyparts_t	*pStudioBodyPart;
	mstudiomodel_t		*pStudioModel;
	mstudiomesh_t		*pStudioMesh;

	numMeshes = 0;
	for (i=0; i<pStudioHdr->numbodyparts; i++)
	{
		pStudioBodyPart = pStudioHdr->pBodypart(i);
		for (j=0; j<pStudioBodyPart->nummodels; j++)
		{
			pStudioModel = pStudioBodyPart->pModel(j);
			for (m=0; m<pStudioModel->nummeshes; m++)
			{				
				// get each mesh
				pStudioMesh = pStudioModel->pMesh(m);
				pStudioMesh->meshid = numMeshes + m;
			}
			numMeshes += pStudioModel->nummeshes;
		}
	}
}

	
void LoadMaterials( studiohdr_t *phdr )
{
	int					i, j;

	// get index of each material
	if( phdr->textureindex != 0 )
	{
		for( i = 0; i < phdr->numtextures; i++ )
		{
			char szPath[256];
			IMaterial *pMaterial = NULL;
			// search through all specified directories until a valid material is found
			for( j = 0; j < phdr->numcdtextures && IsErrorMaterial( pMaterial ); j++ )
			{
				strcpy( szPath, phdr->pCdtexture( j ) );
				strcat( szPath, phdr->pTexture( i )->pszName( ) );

				pMaterial = g_pMaterialSystem->FindMaterial( szPath, TEXTURE_GROUP_OTHER, false );
			}
			if( IsErrorMaterial( pMaterial ) && !g_quiet )
			{
				// hack - if it isn't found, go through the motions of looking for it again
				// so that the materialsystem will give an error.
				for( j = 0; j < phdr->numcdtextures; j++ )
				{
					strcpy( szPath, phdr->pCdtexture( j ) );
					strcat( szPath, phdr->pTexture( i )->pszName( ) );
					g_pMaterialSystem->FindMaterial( szPath, TEXTURE_GROUP_OTHER, true );
				}
			}

			phdr->pTexture( i )->material = pMaterial;

			// FIXME: hack, needs proper client side material system interface
			bool found = false;
			IMaterialVar *clientShaderVar = pMaterial->FindVar( "$clientShader", &found, false );
			if( found )
			{
				if (stricmp( clientShaderVar->GetStringValue(), "MouthShader") == 0)
				{
					phdr->pTexture( i )->flags = 1;
				}
				phdr->pTexture( i )->used = 1;
			}
		}
	}
}


void WriteKeyValues( studiohdr_t *phdr, CUtlVector< char > *pKeyValue )
{
	phdr->keyvalueindex = (pData - pStart);
	phdr->keyvaluesize = KeyValueTextSize( pKeyValue );
	if (phdr->keyvaluesize)
	{
		memcpy(	pData, KeyValueText( pKeyValue ), phdr->keyvaluesize );

		// Add space for a null terminator
		pData[phdr->keyvaluesize] = 0;
		++phdr->keyvaluesize;

		pData += phdr->keyvaluesize * sizeof( char );
	}
	ALIGN4( pData );
}


void WriteSeqKeyValues( mstudioseqdesc_t *pseqdesc, CUtlVector< char > *pKeyValue )
{
	pseqdesc->keyvalueindex = (pData - (byte *)pseqdesc);
	pseqdesc->keyvaluesize = KeyValueTextSize( pKeyValue );
	if (pseqdesc->keyvaluesize)
	{
		memcpy(	pData, KeyValueText( pKeyValue ), pseqdesc->keyvaluesize );

		// Add space for a null terminator
		pData[pseqdesc->keyvaluesize] = 0;
		++pseqdesc->keyvaluesize;

		pData += pseqdesc->keyvaluesize * sizeof( char );
	}
	ALIGN4( pData );
}


void EnsureFileDirectoryExists( const char *pFilename )
{
	char dirName[MAX_PATH];
	Q_strncpy( dirName, pFilename, sizeof( dirName ) );
	Q_FixSlashes( dirName );
	char *pLastSlash = strrchr( dirName, CORRECT_PATH_SEPARATOR );
	if ( pLastSlash )
	{
		*pLastSlash = 0;

		if ( _access( dirName, 0 ) != 0 )
		{
			char cmdLine[512];
			Q_snprintf( cmdLine, sizeof( cmdLine ), "md \"%s\"", dirName );
			system( cmdLine );
		}
	}
}


void WriteModelFiles(void)
{
	FileHandle_t modelouthandle = 0;
	FileHandle_t blockouthandle = 0;
	int			total = 0;
	int			i;
	char		filename[260];
	studiohdr_t *phdr;
	studiohdr_t *pblockhdr;

	pStart = (byte *)kalloc( 1, FILEBUFFER );

	pBlockData = NULL;
	pBlockStart = NULL;

	Q_StripExtension( outname, outname, sizeof( outname ) );

	if (g_bXbox && g_animblocksize == 0 && g_numani > 2)
	{
		g_animblocksize = 16 * 1024;
	}
		
	if (g_animblocksize != 0)
	{
		// write the non-default g_sequence group data to separate files
		sprintf( g_animblockname, "models/%s.ani", outname );

		strcpy( filename, gamedir );
		strcat( filename, g_animblockname );	

		EnsureFileDirectoryExists( filename );

		if (!g_bVerifyOnly)
			blockouthandle = SafeOpenWrite( filename );

		pBlockStart = (byte *)kalloc( 1, FILEBUFFER );
		pBlockData = pBlockStart;

		pblockhdr = (studiohdr_t *)pBlockData;
		pblockhdr->id = IDSTUDIOANIMGROUPHEADER;
		pblockhdr->version = STUDIO_VERSION;

		pBlockData += sizeof( *pblockhdr ); 
	}

//
// write the g_model output file
//
	phdr = (studiohdr_t *)pStart;

	phdr->id = IDSTUDIOHEADER;
	phdr->version = STUDIO_VERSION;

	strcat (outname, ".mdl");

	// strcpy( outname, ExpandPath( outname ) );

	strcpy( filename, gamedir );
//	if( *g_pPlatformName )
//	{
//		strcat( filename, "platform_" );
//		strcat( filename, g_pPlatformName );
//		strcat( filename, "/" );
//	}
	strcat( filename, "models/" );	
	strcat( filename, outname );	

	
	// Create the directory.
	EnsureFileDirectoryExists( filename );


	if( !g_quiet )
	{
		printf ("---------------------\n");
		printf ("writing %s:\n", filename);
	}

	if (!g_bVerifyOnly)
		modelouthandle = SafeOpenWrite (filename);

	phdr->eyeposition = eyeposition;
	phdr->illumposition = illumposition;

	if ( !g_wrotebbox && g_sequence.Count() > 0)
	{
		VectorCopy( g_sequence[0].bmin, bbox[0] );
		VectorCopy( g_sequence[0].bmax, bbox[1] );
		CollisionModel_ExpandBBox( bbox[0], bbox[1] );
		VectorCopy( bbox[0], g_sequence[0].bmin );
		VectorCopy( bbox[1], g_sequence[0].bmax );
	}
	if ( !g_wrotecbox )
	{
		// no default clipping box, just use per-sequence box
		VectorCopy( vec3_origin, cbox[0] );
		VectorCopy( vec3_origin, cbox[1] );
	}

	phdr->hull_min = bbox[0]; 
	phdr->hull_max = bbox[1]; 
	phdr->view_bbmin = cbox[0]; 
	phdr->view_bbmax = cbox[1]; 

	phdr->flags = gflags;
	phdr->mass = GetCollisionModelMass();	
	phdr->constdirectionallightdot = g_constdirectionalightdot;

	pData = (byte *)phdr + sizeof( studiohdr_t );

	BeginStringTable( );

	strcpy( phdr->name, outname );
	// AddToStringTable( phdr, &phdr->sznameindex, outname );

	WriteBoneInfo( phdr );
	if( !g_quiet )
	{
		printf("bones      %7d bytes (%d)\n", pData - pStart - total, g_numbones );
	}
	total = pData - pStart;

	pData = WriteAnimations( pData, pStart, 0, phdr, NULL );
	if( !g_quiet )
	{
		printf("animations %7d bytes (%d anims) (%d frames) [%d:%02d]\n", pData - pStart - total, g_numani, totalframes, (int)totalseconds / 60, (int)totalseconds % 60 );
	}
	total  = pData - pStart;

	WriteSequenceInfo( phdr );
	if( !g_quiet )
	{
		printf("sequences  %7d bytes (%d seq) \n", pData - pStart - total, g_sequence.Count() );
	}
	total  = pData - pStart;

	WriteModel( phdr );
	/*
	if( !g_quiet )
	{
		printf("models     %7d bytes\n", pData - pStart - total );
	}
	*/
	total  = pData - pStart;

	WriteTextures( phdr );
	if( !g_quiet )
	{
 		printf("textures   %7d bytes\n", pData - pStart - total );
	}
	total  = pData - pStart;

	WriteKeyValues( phdr, &g_KeyValueText );
	if( !g_quiet )
	{
		printf("keyvalues  %7d bytes\n", pData - pStart - total );
	}
	total  = pData - pStart;

	pData = WriteStringTable( pData );

	total  = pData - pStart;

	phdr->checksum = 0;
	for (i = 0; i < total; i += 4)
	{
		// TODO: does this need something more than a simple shift left and add checksum?
		phdr->checksum = (phdr->checksum << 1) + ((phdr->checksum & 0x8000000) ? 1 : 0) + *((long *)(pStart + i));
	}

	if (g_bVerifyOnly)
		return;

	CollisionModel_Write( phdr->checksum );

	if( !g_quiet )
	{
		printf("collision  %7d bytes\n", pData - pStart - total );
	}

	AssignMeshIDs( phdr );

	phdr->length = pData - pStart;
	if( !g_quiet )
	{
		printf("total      %7d\n", phdr->length );
	}

	// Load materials for this model via the material system so that the
	// optimizer can ask questions about the materials.
	char materialDir[256];
	strcpy( materialDir, gamedir );
	strcat( materialDir, "materials" );	
	InitMaterialSystem( materialDir );
	LoadMaterials( phdr );

	SafeWrite( modelouthandle, pStart, phdr->length );

	g_pFileSystem->Close(modelouthandle);

	if (pBlockStart)
	{
		pblockhdr->length = pBlockData - pBlockStart;

		SafeWrite( blockouthandle, pBlockStart, pblockhdr->length );
		g_pFileSystem->Close(blockouthandle);

		if ( !g_quiet )
		{
			printf ("---------------------\n");
			printf("writing %s:\n", g_animblockname);
			printf("blocks	   %7d\n", g_numanimblocks );
			printf("total      %7d\n", pblockhdr->length );
		}
	}

	if (phdr->numbodyparts == 0)
		return;

	// vertices have become an external peer data store
	// write now prior to impending vertex access from any further code
	// vertex accessors hide shifting vertex data
	WriteVertices( phdr );

#ifdef _DEBUG
	int bodyPartID;
	for( bodyPartID = 0; bodyPartID < phdr->numbodyparts; bodyPartID++ )
	{
		mstudiobodyparts_t *pBodyPart = phdr->pBodypart( bodyPartID );
		int modelID;
		for( modelID = 0; modelID < pBodyPart->nummodels; modelID++ )
		{
			mstudiomodel_t *pModel = pBodyPart->pModel( modelID );
			const mstudio_modelvertexdata_t *vertData = pModel->GetVertexData();
			int vertID;
			for( vertID = 0; vertID < pModel->numvertices; vertID++ )
			{
				Vector4D *pTangentS = vertData->TangentS( vertID );
				Assert( pTangentS->w == -1.0f || pTangentS->w == 1.0f );
			}
		}
	}
#endif

	OptimizedModel::WriteOptimizedFiles( phdr, g_bodypart );

	// now have external finalized vtx (windings) and vvd (vertexes)
	// re-open files, sort vertexes, perform fixups, and rewrite
	// purposely isolated as a post process for stability
	if (!FixupToSortedLODVertexes( phdr ))
	{
		MdlError("Aborted vertex sort fixup on '%s':\n", filename);
	}

	if (!Clamp_RootLOD( phdr ))
	{
		MdlError("Aborted root lod shift '%s':\n", filename);
	}

	if ( g_bPerf )
	{
		SpewPerfStats( phdr, filename );
	}
}

const mstudio_modelvertexdata_t *mstudiomodel_t::GetVertexData()
{
	static vertexFileHeader_t	*pVertexHdr;
	char						filename[260];

	if (pVertexHdr)
	{
		// studiomdl is a single model process, can simply persist data in static
		goto hasData;
	}

	// load and persist the vertex file
	strcpy( filename, gamedir );
//	if( *g_pPlatformName )
//	{
//		strcat( filename, "platform_" );
//		strcat( filename, g_pPlatformName );
//		strcat( filename, "/" );	
//	}
	strcat( filename, "models/" );	
	strcat( filename, outname );
	Q_StripExtension( filename, filename, sizeof( filename ) );
	strcat( filename, ".vvd" );

	LoadFile(filename, (void**)&pVertexHdr);

	// check id
	if (pVertexHdr->id != MODEL_VERTEX_FILE_ID)
	{
		MdlError("Error Vertex File: '%s' (id %d should be %d)\n", filename, pVertexHdr->id, MODEL_VERTEX_FILE_ID);
	}

	// check version
	if (pVertexHdr->version != MODEL_VERTEX_FILE_VERSION)
	{
		MdlError("Error Vertex File: '%s' (version %d should be %d)\n", filename, pVertexHdr->version, MODEL_VERTEX_FILE_VERSION);
	}

hasData:
	vertexdata.pVertexData  = (byte *)pVertexHdr + pVertexHdr->vertexDataStart;
	vertexdata.pTangentData = (byte *)pVertexHdr + pVertexHdr->tangentDataStart;

	return &vertexdata;
}

typedef struct
{
	int meshVertID;
	int	finalMeshVertID;
	int	vertexOffset;
	int	lodFlags;
} usedVertex_t;

typedef struct
{
	int	offsets[MAX_NUM_LODS];
	int	numVertexes[MAX_NUM_LODS];
} lodMeshInfo_t;

typedef struct
{
	usedVertex_t	*pVertexList;
	short			*pVertexMap;
	int				numVertexes;
	lodMeshInfo_t	lodMeshInfo;
} vertexPool_t;

#define ALIGN(b,s)		(((unsigned int)(b)+(s)-1)&~((s)-1))

//-----------------------------------------------------------------------------
// FindVertexOffsets
// 
// Iterate sorted vertex list to determine mesh starts and counts.
//-----------------------------------------------------------------------------
void FindVertexOffsets(int vertexOffset, int offsets[MAX_NUM_LODS], int counts[MAX_NUM_LODS], int numLods, const usedVertex_t *pVertexList, int numVertexes)
{
	int lodFlags;
	int	i;
	int	j;
	int	k;

	// vertexOffset uniquely identifies a single mesh's vertexes in lod vertex sorted list
	// lod vertex list is sorted from lod N..lod 0
	for (i=numLods-1; i>=0; i--)
	{
		offsets[i] = 0;
		counts[i]  = 0;

		lodFlags = (1<<(i+1))-1;
		for (j=0; j<numVertexes; j++)
		{
			// determine start of mesh at desired lod
			if (pVertexList[j].lodFlags > lodFlags)
				continue;
			if (pVertexList[j].vertexOffset != vertexOffset)
				continue;

			for (k=j; k<numVertexes; k++)
			{
				// determine end of mesh at desired lod
				if (pVertexList[k].vertexOffset != vertexOffset)
					break;
				if (!(pVertexList[k].lodFlags & (1<<i)))
					break;
			}

			offsets[i] = j;
			counts[i]  = k-j;
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// _CompareUsedVertexes
// 
// qsort callback
//-----------------------------------------------------------------------------
static int _CompareUsedVertexes(const void *a, const void *b)
{
	usedVertex_t	*pVertexA;
	usedVertex_t	*pVertexB;
	int				sort;
	int				lodA;
	int				lodB;

	pVertexA = (usedVertex_t*)a;
	pVertexB = (usedVertex_t*)b;

	// determine highest (lowest detail) lod
	// forces grouping into discrete MAX_NUM_LODS sections
	lodA = Q_log2(pVertexA->lodFlags);
	lodB = Q_log2(pVertexB->lodFlags);

	// descending sort (LodN..Lod0)
	sort = lodB-lodA;
	if (sort)
		return sort;

	// within same lod, sub sort (ascending) by mesh
	sort = pVertexA->vertexOffset - pVertexB->vertexOffset;
	if (sort)
		return sort;
	
	// within same mesh, sub sort (ascending) by vertex
	sort = pVertexA->meshVertID - pVertexB->meshVertID;
	return sort;
}

//-----------------------------------------------------------------------------
// BuildSortedVertexList
// 
// Generates the sorted vertex list. Routine is purposely serial to
// ensure vertex integrity.
//-----------------------------------------------------------------------------
bool BuildSortedVertexList(const studiohdr_t *pStudioHdr, const void *pVtxBuff, vertexPool_t **ppVertexPools, int *pNumVertexPools, usedVertex_t **ppVertexList, int *pNumVertexes)
{
	OptimizedModel::FileHeader_t		*pVtxHdr;
	OptimizedModel::BodyPartHeader_t	*pBodyPartHdr;
	OptimizedModel::ModelHeader_t		*pModelHdr;
	OptimizedModel::ModelLODHeader_t	*pModelLODHdr;
	OptimizedModel::MeshHeader_t		*pMeshHdr;
	OptimizedModel::StripGroupHeader_t	*pStripGroupHdr;
	OptimizedModel::Vertex_t			*pStripVertex;
	mstudiobodyparts_t					*pStudioBodyPart;
	mstudiomodel_t						*pStudioModel;
	mstudiomesh_t						*pStudioMesh;
	usedVertex_t						*usedVertexes;
	vertexPool_t						*pVertexPools;
	vertexPool_t						*pPool;
	usedVertex_t						*pVertexList;
	int									*pVertexes;
	short								*pVertexMap;
	int									index;
	int									currLod;
	int									vertexOffset;
	int									i,j,k,m,n,p;
	int									poolStart;
	int									numVertexPools;
	int									numVertexes;
	int									numMeshVertexes;
	int									offsets[MAX_NUM_LODS];
	int									counts[MAX_NUM_LODS];
	int									finalMeshVertID;
	int									baseMeshVertID;

	*ppVertexPools   = NULL;
	*pNumVertexPools = 0;
	*ppVertexList    = NULL;
	*pNumVertexes    = 0;

	pVtxHdr = (OptimizedModel::FileHeader_t*)pVtxBuff; 

	// determine number of vertex pools
	if (pStudioHdr->numbodyparts != pVtxHdr->numBodyParts)
		return false;
	numVertexPools = 0;
	for (i=0; i<pVtxHdr->numBodyParts; i++)
	{
		pBodyPartHdr    = pVtxHdr->pBodyPart(i);
		pStudioBodyPart = pStudioHdr->pBodypart(i);
		if (pStudioBodyPart->nummodels != pBodyPartHdr->numModels)
			return false;

		// the model's subordinate lods only reference from a single top level pool 
		// no new verts are created for sub lods
		// each model's subordinate mesh dictates its own vertex pool
		for (j=0; j<pBodyPartHdr->numModels; j++)
		{
			pStudioModel    = pStudioBodyPart->pModel(j);
			numVertexPools += pStudioModel->nummeshes;
		}
	}

	// allocate pools
	pVertexPools = (vertexPool_t*)malloc(numVertexPools*sizeof(vertexPool_t));
	memset(pVertexPools, 0, numVertexPools*sizeof(vertexPool_t));

	// iterate lods, mark referenced indexes
	numVertexPools = 0;
	for (i=0; i<pVtxHdr->numBodyParts; i++)
	{
		pBodyPartHdr    = pVtxHdr->pBodyPart(i);
		pStudioBodyPart = pStudioHdr->pBodypart(i);

		for (j=0; j<pBodyPartHdr->numModels; j++)
		{
			pModelHdr    = pBodyPartHdr->pModel(j);
			pStudioModel = pStudioBodyPart->pModel(j);

			// allocate each mesh's vertex list
			poolStart = numVertexPools;
			for (k=0; k<pStudioModel->nummeshes; k++)
			{
				// track the expected relative offset into a flattened vertex list
				vertexOffset = 0;
				for (m=0; m<poolStart+k; m++)
					vertexOffset += pVertexPools[m].numVertexes;

				pStudioMesh = pStudioModel->pMesh(k);
				numMeshVertexes = pStudioMesh->numvertices;
				if (numMeshVertexes)
				{
					usedVertexes = (usedVertex_t*)malloc(numMeshVertexes*sizeof(usedVertex_t));
					pVertexMap   = (short*)malloc(numMeshVertexes*sizeof(short));

					for (n=0; n<numMeshVertexes; n++)
					{
						// setup mapping
						// due to the hierarchial layout, the vertID's map per mesh's pool
						// a linear layout of the vertexes requires a unique signature to achieve a remap
						// the offset and index form a unique signature
						usedVertexes[n].meshVertID      = n;
						usedVertexes[n].finalMeshVertID = -1;
						usedVertexes[n].vertexOffset    = vertexOffset;
						usedVertexes[n].lodFlags        = 0;
						pVertexMap[n]                   = n;
					}
				
					pVertexPools[numVertexPools].pVertexList = usedVertexes;
					pVertexPools[numVertexPools].pVertexMap  = pVertexMap;
				}
				pVertexPools[numVertexPools].numVertexes = numMeshVertexes;
				numVertexPools++;
			}

			// iterate all lods
			for (currLod=0; currLod<pVtxHdr->numLODs; currLod++)
			{
				pModelLODHdr = pModelHdr->pLOD(currLod);

				if (pModelLODHdr->numMeshes != pStudioModel->nummeshes)
					return false;

				for (k=0; k<pModelLODHdr->numMeshes; k++)
				{
					pMeshHdr      = pModelLODHdr->pMesh(k);
					pStudioMesh   = pStudioModel->pMesh(k);
					for (m=0; m<pMeshHdr->numStripGroups; m++)
					{
						pStripGroupHdr = pMeshHdr->pStripGroup(m);
						
						// sanity check the indexes have 100% coverage of the vertexes
						pVertexes = (int*)malloc(pStripGroupHdr->numVerts*sizeof(int));
						memset(pVertexes, 0xFF, pStripGroupHdr->numVerts*sizeof(int));

						for (n=0; n<pStripGroupHdr->numIndices; n++)
						{
							index = *pStripGroupHdr->pIndex(n);
							if (index < 0 || index >= pStripGroupHdr->numVerts)
								return false;
							pVertexes[index] = index;
						}

						// sanity check for coverage
						for (n=0; n<pStripGroupHdr->numVerts; n++)
						{
							if (pVertexes[n] != n)
								return false;
						}

						free(pVertexes);

						// iterate vertexes
						pPool = &pVertexPools[poolStart + k];
						for (n=0; n<pStripGroupHdr->numVerts; n++)
						{
							pStripVertex = pStripGroupHdr->pVertex(n);
							if (pStripVertex->origMeshVertID < 0 || pStripVertex->origMeshVertID >= pPool->numVertexes)
								return false;

							// arrange binary flags for numerical sorting
							// the lowest detail lod's verts at the top, the root lod's verts at the bottom
							pPool->pVertexList[pStripVertex->origMeshVertID].lodFlags |= 1<<currLod;
						}
					}
				}
			}
		}
	}

	// flatten the vertex pool hierarchy into a linear sequence
	numVertexes = 0;
	for (i=0; i<numVertexPools; i++)
		numVertexes += pVertexPools[i].numVertexes;
	pVertexList = (usedVertex_t*)malloc(numVertexes*sizeof(usedVertex_t));
	numVertexes  = 0;
	for (i=0; i<numVertexPools; i++)
	{
		pPool = &pVertexPools[i];
		for (j=0; j<pPool->numVertexes; j++)
		{
			if (!pPool->pVertexList[j].lodFlags)
			{
				// found an orphaned vertex that is unreferenced at any lod strip winding
				// don't know how these occur or who references them
				// cannot cull the orphaned vertexes, otherwise vertex counts are wrong
				// every vertex must be remapped
				// force the vertex to belong to the lowest lod 
				// lod flags must be nonzero for proper sorted runs
				pPool->pVertexList[j].lodFlags = 1<<(pVtxHdr->numLODs-1);
			}
		}

		memcpy(&pVertexList[numVertexes], pPool->pVertexList, pPool->numVertexes*sizeof(usedVertex_t));
		numVertexes += pPool->numVertexes;
	}

	// sort the vertexes based on lod flags
	// the sort dictates the linear sequencing of the .vvd data file
	// the vtx file indexes get remapped to the new sort order
	qsort(pVertexList, numVertexes, sizeof(usedVertex_t), _CompareUsedVertexes);
	
	// build a mapping table from mesh relative indexes to the flat lod sorted array
	vertexOffset = 0;
	for (i=0; i<numVertexPools; i++)
	{
		pPool = &pVertexPools[i];
		for (j=0; j<pPool->numVertexes; j++)
		{
			// scan flattened sorted vertexes
			for (k=0; k<numVertexes; k++)
			{
				if (pVertexList[k].vertexOffset == vertexOffset && pVertexList[k].meshVertID == j)
					break;
			}
			pPool->pVertexMap[j] = k;
		}
		vertexOffset += pPool->numVertexes;
	}

	// build offsets and counts that identifies mesh's distribution across lods
	// calc final fixed vertex location if vertexes were gathered to mesh order from lod sorted list
	finalMeshVertID = 0;
	poolStart = 0; 
	for (i=0; i<pStudioHdr->numbodyparts; i++)
	{
		pStudioBodyPart = pStudioHdr->pBodypart(i);
		for (j=0; j<pStudioBodyPart->nummodels; j++)
		{
			pStudioModel = pStudioBodyPart->pModel(j);
			for (m=0; m<pStudioModel->nummeshes; m++)
			{
				// track the expected offset into linear vertexes
				vertexOffset = 0;
				for (n=0; n<poolStart+m; n++)
					vertexOffset += pVertexPools[n].numVertexes;

				// vertexOffset works as unique key to identify vertexes for a specific mesh
				// a mesh's verts are distributed, but guaranteed sequential in the lod sorted vertex list
				// determine base index and offset and run length for target mesh for all lod levels
				FindVertexOffsets(vertexOffset, offsets, counts, pVtxHdr->numLODs, pVertexList, numVertexes);

				for (n=0; n<pVtxHdr->numLODs; n++)
				{
					if (!counts[n])
						offsets[n] = 0;

					pVertexPools[poolStart+m].lodMeshInfo.offsets[n]     = offsets[n];
					pVertexPools[poolStart+m].lodMeshInfo.numVertexes[n] = counts[n];
				}

				// iterate using calced offsets to walk each mesh
				// set its expected final vertex id, which is its "gathered" index relative to mesh
				baseMeshVertID = finalMeshVertID;
				for (n=pVtxHdr->numLODs-1; n>=0; n--)
				{
					// iterate each vert in the mesh
					// vertex id is relative to
					for (p=0; p<counts[n]; p++)
					{
						pVertexList[offsets[n] + p].finalMeshVertID = finalMeshVertID - baseMeshVertID;
						finalMeshVertID++;
					}
				}
			}
			poolStart += pStudioModel->nummeshes;
		}
	}

	// safety check
	// every referenced vertex should have been remapped correctly
	// some models do have orphaned vertexes, ignore these
	for (i=0; i<numVertexes; i++)
	{
		if (pVertexList[i].lodFlags && pVertexList[i].finalMeshVertID == -1)
		{
			// should never happen, data occured in unknown manner
			// don't build corrupted data
			return false;
		}
	}

	// provide generated tables
	*ppVertexPools   = pVertexPools;
	*pNumVertexPools = numVertexPools;
	*ppVertexList    = pVertexList;
	*pNumVertexes    = numVertexes;

	// success
	return true;
}

//-----------------------------------------------------------------------------
// FixupVVDFile
// 
// VVD files get vertexes remapped to a flat lod sorted order.
//-----------------------------------------------------------------------------
bool FixupVVDFile(const char *fileName,  const studiohdr_t *pStudioHdr, const void *pVtxBuff, const vertexPool_t *pVertexPools, int numVertexPools, const usedVertex_t *pVertexList, int numVertexes)
{	
	OptimizedModel::FileHeader_t	*pVtxHdr;
	vertexFileHeader_t				*pFileHdr_old;
	vertexFileHeader_t				*pFileHdr_new;
	mstudiobodyparts_t				*pStudioBodyPart;
	mstudiomodel_t					*pStudioModel;
	mstudiomesh_t					*pStudioMesh;
	mstudiovertex_t					*pVertex_old;
	mstudiovertex_t					*pVertex_new;
	Vector4D						*pTangent_new;
	Vector4D						*pTangent_old;
	mstudiovertex_t					**pFlatVertexes;
	Vector4D						**pFlatTangents;
	vertexFileFixup_t				*pFixupTable;
	const lodMeshInfo_t				*pLodMeshInfo;
	byte							*pStart_new;
	byte							*pData_new;
	byte							*pStart_base;
	byte							*pVertexBase_old;
	byte							*pTangentBase_old;
	void							*pVvdBuff;
	int								i;
	int								j;
	int								k;
	int								n;
	int								p;
	int								numFixups;
	int								numFlat;
	int								oldIndex;
	int								mask;
	int								maxCount;
	int								numMeshes;
	int								numOutFixups;

	pVtxHdr = (OptimizedModel::FileHeader_t*)pVtxBuff; 

	LoadFile((char*)fileName, &pVvdBuff);

	pFileHdr_old = (vertexFileHeader_t*)pVvdBuff;
	if (pFileHdr_old->numLODs != 1)
	{
		// file has wrong expected state
		return false;
	}

	// meshes need relocation fixup from lod order back to mesh order
	numFixups = 0;
	numMeshes = 0;
	for (i=0; i<pStudioHdr->numbodyparts; i++)
	{
		pStudioBodyPart = pStudioHdr->pBodypart(i);
		for (j=0; j<pStudioBodyPart->nummodels; j++)
		{
			pStudioModel = pStudioBodyPart->pModel(j);
			for (k=0; k<pStudioModel->nummeshes; k++)
			{
				pStudioMesh = pStudioModel->pMesh(k);
				if (!pStudioMesh->numvertices)
				{
					// no vertexes for this mesh, skip it
					continue;
				}
				for (n=pVtxHdr->numLODs-1; n>=0; n--)
				{
					pLodMeshInfo = &pVertexPools[numMeshes+k].lodMeshInfo;
					if (!pLodMeshInfo->numVertexes[n])
					{
						// no vertexes for this portion of the mesh at this lod, skip it
						continue;
					}
					numFixups++;
				}
			}
			numMeshes += k;
		}
	}
	if (numMeshes == 1 || numFixups == 1 || pVtxHdr->numLODs == 1)
	{
		// no fixup required for a single mesh
		// no fixup required for single lod
		// no fixup required when mesh data is contiguous as expected
		numFixups = 0;
	}

	pStart_base = (byte*)malloc(FILEBUFFER);
	memset(pStart_base, 0, FILEBUFFER);
	pStart_new  = (byte*)ALIGN(pStart_base,16);
	pData_new   = pStart_new;

	// setup headers
	pFileHdr_new = (vertexFileHeader_t*)pData_new;
	pData_new += sizeof(vertexFileHeader_t);

	// clone and fixup new header
	*pFileHdr_new = *pFileHdr_old;
	pFileHdr_new->numLODs   = pVtxHdr->numLODs;
	pFileHdr_new->numFixups = numFixups;

	// skip new fixup table
	pData_new   = (byte*)ALIGN(pData_new, 4);
	pFixupTable = (vertexFileFixup_t*)pData_new;
	pFileHdr_new->fixupTableStart = pData_new - pStart_new;
	pData_new += numFixups*sizeof(vertexFileFixup_t);

	// skip new vertex data
	pData_new    = (byte*)ALIGN(pData_new, 16);
	pVertex_new  = (mstudiovertex_t*)pData_new;
	pFileHdr_new->vertexDataStart = pData_new - pStart_new;
	pData_new += numVertexes*sizeof(mstudiovertex_t);

	// skip new tangent data
	pData_new    = (byte*)ALIGN(pData_new, 16);
	pTangent_new = (Vector4D*)pData_new;
	pFileHdr_new->tangentDataStart = pData_new - pStart_new;
	pData_new += numVertexes*sizeof(Vector4D);

	pVertexBase_old  = (byte*)pFileHdr_old + pFileHdr_old->vertexDataStart;
	pTangentBase_old = (byte*)pFileHdr_old + pFileHdr_old->tangentDataStart;

	// determine number of aggregate verts towards root lod
	// loader can truncate read according to desired root lod
	maxCount = -1;
	for (n=pVtxHdr->numLODs-1; n>=0; n--)
	{
		mask = 1<<n;
		for (p=0; p<numVertexes; p++)
		{
			if (mask & pVertexList[p].lodFlags)
			{
				if (maxCount < p)
					maxCount = p;
			}
		}
		pFileHdr_new->numLODVertexes[n] = maxCount+1;
	}
	for (n=pVtxHdr->numLODs; n<MAX_NUM_LODS; n++)
	{
		// ripple the last valid lod entry all the way down
		pFileHdr_new->numLODVertexes[n] = pFileHdr_new->numLODVertexes[pVtxHdr->numLODs-1];
	}
	
	// build mesh relocation fixup table
	if (numFixups)
	{
		numMeshes    = 0;
		numOutFixups = 0;
		for (i=0; i<pStudioHdr->numbodyparts; i++)
		{
			pStudioBodyPart = pStudioHdr->pBodypart(i);
			for (j=0; j<pStudioBodyPart->nummodels; j++)
			{
				pStudioModel = pStudioBodyPart->pModel(j);
				for (k=0; k<pStudioModel->nummeshes; k++)
				{
					pStudioMesh = pStudioModel->pMesh(k);
					if (!pStudioMesh->numvertices)
					{
						// not vertexes for this mesh, skip it
						continue;
					}
					for (n=pVtxHdr->numLODs-1; n>=0; n--)
					{
						pLodMeshInfo = &pVertexPools[numMeshes+k].lodMeshInfo;
						if (!pLodMeshInfo->numVertexes[n])
						{
							// no vertexes for this portion of the mesh at this lod, skip it
							continue;
						}
						pFixupTable[numOutFixups].lod            = n;
						pFixupTable[numOutFixups].numVertexes    = pLodMeshInfo->numVertexes[n];
						pFixupTable[numOutFixups].sourceVertexID = pLodMeshInfo->offsets[n];
						numOutFixups++;
					}
				}
				numMeshes += pStudioModel->nummeshes;
			}
		}

		if (numOutFixups != numFixups)
		{
			// logic sync error, final calc should match precalc, otherwise memory corruption
			return false;
		}
	}

	// generate offsets to vertexes
	numFlat = 0;
	pFlatVertexes = (mstudiovertex_t**)malloc(numVertexes*sizeof(mstudiovertex_t*));
	pFlatTangents = (Vector4D**)malloc(numVertexes*sizeof(Vector4D*));
	for (i=0; i<pStudioHdr->numbodyparts; i++)
	{
		pStudioBodyPart = pStudioHdr->pBodypart(i);
		for (j=0; j<pStudioBodyPart->nummodels; j++)
		{
			pStudioModel = pStudioBodyPart->pModel(j);
			pVertex_old  = (mstudiovertex_t*)&pVertexBase_old[pStudioModel->vertexindex];
			pTangent_old = (Vector4D*)&pTangentBase_old[pStudioModel->tangentsindex];
			for (k=0; k<pStudioModel->nummeshes; k++)
			{
				// get each mesh's vertexes
				pStudioMesh = pStudioModel->pMesh(k);
				for (n=0; n<pStudioMesh->numvertices; n++)
				{
					// old vertex pools are per model, seperated per mesh by a start offset
					// vertexes are then isolated subpools per mesh
					// build the flat linear array of lookup pointers
					pFlatVertexes[numFlat] = &pVertex_old[pStudioMesh->vertexoffset + n];
					pFlatTangents[numFlat] = &pTangent_old[pStudioMesh->vertexoffset + n];
					numFlat++;
				}
			}
		}
	}

	// write in lod sorted order
	for (i=0; i<numVertexes; i++)
	{
		// iterate sorted order, remap old vert location to new vert location
		oldIndex = pVertexList[i].vertexOffset + pVertexList[i].meshVertID;
		
		memcpy(&pVertex_new[i], pFlatVertexes[oldIndex], sizeof(mstudiovertex_t));
		memcpy(&pTangent_new[i], pFlatTangents[oldIndex], sizeof(Vector4D));
	}

	// pFileHdr_new->length =  pData_new-pStart_new;
	SaveFile((char*)fileName, pStart_new, pData_new-pStart_new);

	free(pStart_base);
	free(pFlatVertexes);
	free(pFlatTangents);

	// success
	return true;
}

//-----------------------------------------------------------------------------
// FixupVTXFile
// 
// VTX files get their windings remapped.
//-----------------------------------------------------------------------------
bool FixupVTXFile(const char *fileName, const studiohdr_t *pStudioHdr, const vertexPool_t *pVertexPools, int numVertexPools, const usedVertex_t *pVertexList, int numVertexes)
{
	OptimizedModel::FileHeader_t		*pVtxHdr;
	OptimizedModel::BodyPartHeader_t	*pBodyPartHdr;
	OptimizedModel::ModelHeader_t		*pModelHdr;
	OptimizedModel::ModelLODHeader_t	*pModelLODHdr;
	OptimizedModel::MeshHeader_t		*pMeshHdr;
	OptimizedModel::StripGroupHeader_t	*pStripGroupHdr;
	OptimizedModel::Vertex_t			*pStripVertex;
	int									currLod;
	int									vertexOffset;
	mstudiobodyparts_t					*pStudioBodyPart;
	mstudiomodel_t						*pStudioModel;
	int									i,j,k,m,n;
	int									poolStart;
	int									VtxLen;
	int									newMeshVertID;
	void								*pVtxBuff;

	VtxLen  = LoadFile((char*)fileName, &pVtxBuff);
	pVtxHdr = (OptimizedModel::FileHeader_t*)pVtxBuff; 

	// iterate all lod's windings
	poolStart = 0;
	for (i=0; i<pVtxHdr->numBodyParts; i++)
	{
		pBodyPartHdr    = pVtxHdr->pBodyPart(i);
		pStudioBodyPart = pStudioHdr->pBodypart(i);

		for (j=0; j<pBodyPartHdr->numModels; j++)
		{
			pModelHdr    = pBodyPartHdr->pModel(j);
			pStudioModel = pStudioBodyPart->pModel(j);

			// iterate all lods
			for (currLod=0; currLod<pVtxHdr->numLODs; currLod++)
			{
				pModelLODHdr = pModelHdr->pLOD(currLod);

				if (pModelLODHdr->numMeshes != pStudioModel->nummeshes)
					return false;

				for (k=0; k<pModelLODHdr->numMeshes; k++)
				{
					// track the expected relative offset into the flat vertexes
					vertexOffset = 0;
					for (m=0; m<poolStart+k; m++)
						vertexOffset += pVertexPools[m].numVertexes;

					pMeshHdr = pModelLODHdr->pMesh(k);
					for (m=0; m<pMeshHdr->numStripGroups; m++)
					{
						pStripGroupHdr = pMeshHdr->pStripGroup(m);
						
						for (n=0; n<pStripGroupHdr->numVerts; n++)
						{
							pStripVertex = pStripGroupHdr->pVertex(n);

							// remap old mesh relative vertex index to absolute flat sorted list
							newMeshVertID = pVertexPools[poolStart+k].pVertexMap[pStripVertex->origMeshVertID];

							// map to expected final fixed vertex locations
							// final fixed vertex location is performed by runtime loading code
							newMeshVertID = pVertexList[newMeshVertID].finalMeshVertID;

							// fixup to expected 
							pStripVertex->origMeshVertID = newMeshVertID;
						}
					}
				}
			}
			poolStart += pStudioModel->nummeshes;
		}
	}

	// pVtxHdr->length = VtxLen;
	SaveFile((char*)fileName, pVtxBuff, VtxLen);

	free(pVtxBuff);

	return true;
}

//-----------------------------------------------------------------------------
// FixupMDLFile
// 
// MDL files get flexes/vertex/tangent data offsets fixed
//-----------------------------------------------------------------------------
bool FixupMDLFile(const char *fileName, studiohdr_t *pStudioHdr, const void *pVtxBuff, const vertexPool_t *pVertexPools, int numVertexPools, const usedVertex_t *pVertexList, int numVertexes)
{
	OptimizedModel::FileHeader_t	*pVtxHdr;
	const lodMeshInfo_t				*pLodMeshInfo;
	mstudiobodyparts_t				*pStudioBodyPart;
	mstudiomodel_t					*pStudioModel;
	mstudiomesh_t					*pStudioMesh;
	mstudioflex_t					*pStudioFlex;
	mstudiovertanim_t				*pStudioVertAnim;
	int								newMeshVertID;
	int								i;
	int								j;
	int								m;
	int								n;
	int								p;
	int								numLODs;
	int								numMeshes;
	int								total;

	pVtxHdr = (OptimizedModel::FileHeader_t*)pVtxBuff;

	numLODs = pVtxHdr->numLODs; 

	numMeshes = 0;
	for (i=0; i<pStudioHdr->numbodyparts; i++)
	{
		pStudioBodyPart = pStudioHdr->pBodypart(i);

		for (j=0; j<pStudioBodyPart->nummodels; j++)
		{
			pStudioModel = pStudioBodyPart->pModel(j);

			for (m=0; m<pStudioModel->nummeshes; m++)
			{				
				// get each mesh
				pStudioMesh  = pStudioModel->pMesh(m);
				pLodMeshInfo = &pVertexPools[numMeshes+m].lodMeshInfo;

				for (n=0; n<numLODs; n++)
				{
					// the root lod, contains all the lower detail lods verts
					// tally the verts that are at each lod
					total = 0;
					for (p=n; p<numLODs; p++)
						total += pLodMeshInfo->numVertexes[p];

					// embed the fixup for loader
					pStudioMesh->vertexdata.numLODVertexes[n] = total;
				}
				for (p=n; p<MAX_NUM_LODS; p++)
				{
					// duplicate last valid lod to end of list
					pStudioMesh->vertexdata.numLODVertexes[p] = pStudioMesh->vertexdata.numLODVertexes[numLODs-1];
				}

				// fix the flexes
				for (n=0; n<pStudioMesh->numflexes; n++)
				{
					pStudioFlex = pStudioMesh->pFlex(n);

					for (p=0; p<pStudioFlex->numverts; p++)
					{
						pStudioVertAnim = pStudioFlex->pVertanim(p);

						if (pStudioVertAnim->index < 0 || pStudioVertAnim->index >= pStudioMesh->numvertices)
							return false;

						// remap old mesh relative vertex index to absolute flat sorted list
						newMeshVertID = pVertexPools[numMeshes+m].pVertexMap[pStudioVertAnim->index];

						// map to expected final fixed vertex locations
						// final fixed vertex location is performed by runtime loading code
						newMeshVertID = pVertexList[newMeshVertID].finalMeshVertID;

						// fixup to expected 
						pStudioVertAnim->index = newMeshVertID;
					}
				}
			}
			numMeshes += pStudioModel->nummeshes;
		}
	}

	SaveFile((char*)fileName, (void*)pStudioHdr, pStudioHdr->length);

	// success
	return true;
}

//-----------------------------------------------------------------------------
// FixupToSortedLODVertexes
// 
// VVD files get vertexes fixed to a flat sorted order, ascending in lower detail lod usage
// VTX files get their windings remapped to the sort.
//-----------------------------------------------------------------------------
bool FixupToSortedLODVertexes(studiohdr_t *pStudioHdr)
{
	char							filename[260];
	char							tmpFileName[260];
	void							*pVtxBuff;
	usedVertex_t					*pVertexList;
	vertexPool_t					*pVertexPools;
	int								numVertexes;
	int								numVertexPools;
	int								VtxLen;
	int								i;
	const char						*vtxPrefixes[] = {".dx80.vtx", ".dx90.vtx", ".sw.vtx", ".xbox.vtx"};

	strcpy( filename, gamedir );
//	if( *g_pPlatformName )
//	{
//		strcat( filename, "platform_" );
//		strcat( filename, g_pPlatformName );
//		strcat( filename, "/" );	
//	}
	strcat( filename, "models/" );	
	strcat( filename, outname );
	Q_StripExtension( filename, filename, sizeof( filename ) );

	// determine lod usage per vertex
	// all vtx files enumerate model's lod verts, but differ in their mesh makeup
	// use xxx.dx80.vtx to establish which vertexes are used by each lod
	strcpy( tmpFileName, filename );
	if ( !g_bXbox )
	{
		strcat( tmpFileName, ".dx80.vtx" );
	}
	else
	{
		// must use the target we are building for
		strcat( tmpFileName, ".xbox.vtx" );
	}
	VtxLen = LoadFile( tmpFileName, &pVtxBuff );

	// build the sorted vertex tables
	if (!BuildSortedVertexList(pStudioHdr, pVtxBuff, &pVertexPools, &numVertexPools, &pVertexList, &numVertexes))
	{
		// data sync error
		return false;
	}

	// fixup ???.vvd
	strcpy( tmpFileName, filename );
	strcat( tmpFileName, ".vvd" );
	if (!FixupVVDFile(tmpFileName, pStudioHdr, pVtxBuff, pVertexPools, numVertexPools, pVertexList, numVertexes))
	{
		// data error
		return false;
	}

	for (i=0; i<ARRAYSIZE(vtxPrefixes); i++)
	{
		// fixup ???.vtx
		strcpy( tmpFileName, filename );
		strcat( tmpFileName, vtxPrefixes[i] );
		if (!FixupVTXFile(tmpFileName, pStudioHdr, pVertexPools, numVertexPools, pVertexList, numVertexes))
		{
			// data error
			return false;
		}
	}

	// fixup ???.mdl
	strcpy( tmpFileName, filename );
	strcat( tmpFileName, ".mdl" );
	if (!FixupMDLFile(tmpFileName, pStudioHdr, pVtxBuff, pVertexPools, numVertexPools, pVertexList, numVertexes))
	{
		// data error
		return false;
	}

	// free the tables
	for (i=0; i<numVertexPools; i++)
	{
		if (pVertexPools[i].pVertexList)
			free(pVertexPools[i].pVertexList);
		if (pVertexPools[i].pVertexMap)
			free(pVertexPools[i].pVertexMap);
	}
	if (numVertexPools)
		free(pVertexPools);
	free(pVtxBuff);

	// success
	return true;
}


byte IsByte( int val )
{
	if (val < 0 || val > 0xFF)
	{
		MdlError("byte conversion out of range %d\n", val );
	}
	return val;
}

char IsChar( int val )
{
	if (val < -0x80 || val > 0x7F)
	{
		MdlError("char conversion out of range %d\n", val );
	}
	return val;
}

int IsInt24( int val )
{
	if (val < -0x800000 || val > 0x7FFFFF)
	{
		MdlError("int24 conversion out of range %d\n", val );
	}
	return val;
}


short IsShort( int val )
{
	if (val < -0x8000 || val > 0x7FFF)
	{
		MdlError("short conversion out of range %d\n", val );
	}
	return val;
}

unsigned short IsUShort( int val )
{
	if (val < 0 || val > 0xFFFF)
	{
		MdlError("ushort conversion out of range %d\n", val );
	}
	return val;
}


bool Clamp_MDL_LODS( const char *fileName, int rootLOD )
{
	studiohdr_t *pStudioHdr;
	int			len;

	len  = LoadFile((char*)fileName, (void **)&pStudioHdr);

	Studio_SetRootLOD( pStudioHdr, rootLOD );

#if 0
	// shift down bone LOD masks
	int iBone;
	for ( iBone = 0; iBone < pStudioHdr->numbones; iBone++)
	{
		mstudiobone_t *pBone = pStudioHdr->pBone( iBone );

		int nLodID;
		for ( nLodID = 0; nLodID < rootLOD; nLodID++)
		{
			int iLodMask = BONE_USED_BY_VERTEX_LOD0 << nLodID;

			if (pBone->flags & (BONE_USED_BY_VERTEX_LOD0 << rootLOD))
			{
				pBone->flags = pBone->flags | iLodMask;
			}
			else
			{
				pBone->flags = pBone->flags & (~iLodMask);
			}
		}
	}
#endif

	SaveFile( (char *)fileName, pStudioHdr, len );

	return true;
}




bool Clamp_VVD_LODS( const char *fileName, int rootLOD )
{
	vertexFileHeader_t *pTempVvdHdr;
	int			len;

	len  = LoadFile((char*)fileName, (void **)&pTempVvdHdr);

	int newLength = Studio_VertexDataSize( pTempVvdHdr, rootLOD, true );

	// printf("was %d now %d\n", len, newLength );

	vertexFileHeader_t *pNewVvdHdr = (vertexFileHeader_t *)calloc( newLength, 1 );

	Studio_LoadVertexes( pTempVvdHdr, pNewVvdHdr, rootLOD, true );

	if (!g_quiet)
	{
		printf ("---------------------\n");
		printf ("writing %s:\n", fileName);
		printf( "vertices   (%d vertices)\n", pNewVvdHdr->numLODVertexes[ 0 ] );
	}

	// pNewVvdHdr->length = newLength;

	SaveFile( (char *)fileName, pNewVvdHdr, newLength );

	return true;
}


bool Clamp_VTX_LODS( const char *fileName, int rootLOD, studiohdr_t *pStudioHdr )
{
	int i, j, k, m, n;
	int nLodID;
	int size;

	OptimizedModel::FileHeader_t *pVtxHdr;
	int			len;

	len  = LoadFile((char*)fileName, (void **)&pVtxHdr);

	OptimizedModel::FileHeader_t *pNewVtxHdr = (OptimizedModel::FileHeader_t *)calloc( FILEBUFFER, 1 );

	byte *pData = (byte *)pNewVtxHdr;
	pData += sizeof( OptimizedModel::FileHeader_t );
	ALIGN4( pData );

	// header
	pNewVtxHdr->version = pVtxHdr->version;
	pNewVtxHdr->vertCacheSize = pVtxHdr->vertCacheSize;
	pNewVtxHdr->maxBonesPerStrip = pVtxHdr->maxBonesPerStrip;
	pNewVtxHdr->maxBonesPerTri = pVtxHdr->maxBonesPerTri;
	pNewVtxHdr->maxBonesPerVert = pVtxHdr->maxBonesPerVert;
	pNewVtxHdr->checkSum = pVtxHdr->checkSum;
	pNewVtxHdr->numLODs = pVtxHdr->numLODs;

	// material replacement list
	pNewVtxHdr->materialReplacementListOffset = (pData - (byte *)pNewVtxHdr);
	pData += pVtxHdr->numLODs * sizeof( OptimizedModel::MaterialReplacementListHeader_t );
	// ALIGN4( pData );

	BeginStringTable( );

	// allocate replacement list arrays
	for ( nLodID = rootLOD; nLodID < pVtxHdr->numLODs; nLodID++ )
	{
		OptimizedModel::MaterialReplacementListHeader_t *pReplacementList = pVtxHdr->pMaterialReplacementList( nLodID );
		OptimizedModel::MaterialReplacementListHeader_t *pNewReplacementList = pNewVtxHdr->pMaterialReplacementList( nLodID );

		pNewReplacementList->numReplacements = pReplacementList->numReplacements;
		pNewReplacementList->replacementOffset = (pData - (byte *)pNewReplacementList);
		pData += pNewReplacementList->numReplacements * sizeof( OptimizedModel::MaterialReplacementHeader_t );
		// ALIGN4( pData );

		for (i = 0; i < pReplacementList->numReplacements; i++)
		{
			OptimizedModel::MaterialReplacementHeader_t *pReplacement = pReplacementList->pMaterialReplacement( i );
			OptimizedModel::MaterialReplacementHeader_t *pNewReplacement = pNewReplacementList->pMaterialReplacement( i );

			pNewReplacement->materialID = pReplacement->materialID;
			AddToStringTable( pNewReplacement, &pNewReplacement->replacementMaterialNameOffset, pReplacement->pMaterialReplacementName() );
		}
	}
	pData = WriteStringTable( pData );

	// link previous LODs to higher LODs
	for ( nLodID = 0; nLodID < rootLOD; nLodID++ )
	{
		OptimizedModel::MaterialReplacementListHeader_t *pRootReplacementList = pNewVtxHdr->pMaterialReplacementList( rootLOD );
		OptimizedModel::MaterialReplacementListHeader_t *pNewReplacementList = pNewVtxHdr->pMaterialReplacementList( nLodID );

		int delta = (byte *)pRootReplacementList - (byte *)pNewReplacementList;

		pNewReplacementList->numReplacements = pRootReplacementList->numReplacements;
		pNewReplacementList->replacementOffset = pRootReplacementList->replacementOffset + delta;
	}

	// body parts
	pNewVtxHdr->numBodyParts = pStudioHdr->numbodyparts;
	pNewVtxHdr->bodyPartOffset = (pData - (byte *)pNewVtxHdr);
	pData += pNewVtxHdr->numBodyParts * sizeof( OptimizedModel::BodyPartHeader_t );
	// ALIGN4( pData );

	// Iterate over every body part...
	for ( i = 0; i < pStudioHdr->numbodyparts; i++ )
	{
		mstudiobodyparts_t* pBodyPart = pStudioHdr->pBodypart(i);
		OptimizedModel::BodyPartHeader_t* pVtxBodyPart = pVtxHdr->pBodyPart(i);
		OptimizedModel::BodyPartHeader_t* pNewVtxBodyPart = pNewVtxHdr->pBodyPart(i);

		pNewVtxBodyPart->numModels = pBodyPart->nummodels;
		pNewVtxBodyPart->modelOffset = (pData - (byte *)pNewVtxBodyPart);
		pData += pNewVtxBodyPart->numModels * sizeof( OptimizedModel::ModelHeader_t );
		// ALIGN4( pData );

		// Iterate over every submodel...
		for (j = 0; j < pBodyPart->nummodels; ++j)
		{
			mstudiomodel_t* pModel = pBodyPart->pModel(j);
			OptimizedModel::ModelHeader_t* pVtxModel = pVtxBodyPart->pModel(j);
			OptimizedModel::ModelHeader_t* pNewVtxModel = pNewVtxBodyPart->pModel(j);

			pNewVtxModel->numLODs = pVtxModel->numLODs;
			pNewVtxModel->lodOffset = (pData - (byte *)pNewVtxModel);
			pData += pNewVtxModel->numLODs * sizeof( OptimizedModel::ModelLODHeader_t );
			ALIGN4( pData );

			for ( nLodID = rootLOD; nLodID < pVtxModel->numLODs; nLodID++ )
			{
				OptimizedModel::ModelLODHeader_t *pVtxLOD = pVtxModel->pLOD( nLodID );
				OptimizedModel::ModelLODHeader_t *pNewVtxLOD = pNewVtxModel->pLOD( nLodID );

				pNewVtxLOD->numMeshes = pVtxLOD->numMeshes;
				pNewVtxLOD->switchPoint = pVtxLOD->switchPoint;
				pNewVtxLOD->meshOffset = (pData - (byte *)pNewVtxLOD);
				pData += pNewVtxLOD->numMeshes * sizeof( OptimizedModel::MeshHeader_t );
				ALIGN4( pData );

				// Iterate over all the meshes....
				for (k = 0; k < pModel->nummeshes; ++k)
				{
					Assert( pModel->nummeshes == pVtxLOD->numMeshes );
					mstudiomesh_t* pMesh = pModel->pMesh(k);
					OptimizedModel::MeshHeader_t* pVtxMesh = pVtxLOD->pMesh(k);
					OptimizedModel::MeshHeader_t* pNewVtxMesh = pNewVtxLOD->pMesh(k);

					pNewVtxMesh->numStripGroups = pVtxMesh->numStripGroups;
					pNewVtxMesh->flags = pVtxMesh->flags;
					pNewVtxMesh->stripGroupHeaderOffset = (pData - (byte *)pNewVtxMesh);
					pData += pNewVtxMesh->numStripGroups * sizeof( OptimizedModel::StripGroupHeader_t );

					// printf("part %d : model %d : lod %d : mesh %d : strips %d : offset %d\n", i, j, nLodID, k, pVtxMesh->numStripGroups, pVtxMesh->stripGroupHeaderOffset );

					for (m = 0; m < pVtxMesh->numStripGroups; m++)
					{
						OptimizedModel::StripGroupHeader_t *pStripGroup = pVtxMesh->pStripGroup( m );
						OptimizedModel::StripGroupHeader_t *pNewStripGroup = pNewVtxMesh->pStripGroup( m );

						// int delta = ((byte *)pStripGroup - (byte *)pVtxHdr) - ((byte *)pNewStripGroup - (byte *)pNewVtxHdr);

						pNewStripGroup->numVerts = pStripGroup->numVerts;
						pNewStripGroup->vertOffset = (pData - (byte *)pNewStripGroup);
						size = pNewStripGroup->numVerts * sizeof( OptimizedModel::Vertex_t );
						memcpy( pData, pStripGroup->pVertex(0), size );
						pData += size;

						pNewStripGroup->numIndices = pStripGroup->numIndices;
						pNewStripGroup->indexOffset = (pData - (byte *)pNewStripGroup);
						size = pNewStripGroup->numIndices * sizeof( unsigned short );
						memcpy( pData, pStripGroup->pIndex(0), size );
						pData += size;

						pNewStripGroup->numStrips = pStripGroup->numStrips;
						pNewStripGroup->stripOffset = (pData - (byte *)pNewStripGroup);
						size = pNewStripGroup->numStrips * sizeof( OptimizedModel::StripHeader_t );
						pData += size;

						pNewStripGroup->flags = pStripGroup->flags;

						/*
						printf("\tnumVerts %d %d :\n", pStripGroup->numVerts, pStripGroup->vertOffset );
						printf("\tnumIndices %d %d :\n", pStripGroup->numIndices, pStripGroup->indexOffset );
						printf("\tnumStrips %d %d :\n", pStripGroup->numStrips, pStripGroup->stripOffset );
						*/

						for (n = 0; n < pStripGroup->numStrips; n++)
						{
							OptimizedModel::StripHeader_t *pStrip = pStripGroup->pStrip( n );
							OptimizedModel::StripHeader_t *pNewStrip = pNewStripGroup->pStrip( n );

							pNewStrip->numIndices = pStrip->numIndices;
							pNewStrip->indexOffset = pStrip->indexOffset;

							pNewStrip->numVerts = pStrip->numVerts;
							pNewStrip->vertOffset = pStrip->vertOffset;

							pNewStrip->numBones = pStrip->numBones;
							pNewStrip->flags = pStrip->flags;

							pNewStrip->numBoneStateChanges = pStrip->numBoneStateChanges;
							pNewStrip->boneStateChangeOffset = (pData - (byte *)pNewStrip);
							size = pNewStrip->numBoneStateChanges * sizeof( OptimizedModel::BoneStateChangeHeader_t );
							memcpy( pData, pStrip->pBoneStateChange(0), size );
							pData += size;

							/*
							printf("\t\tnumIndices %d %d :\n", pNewStrip->numIndices, pNewStrip->indexOffset );
							printf("\t\tnumVerts %d %d :\n", pNewStrip->numVerts, pNewStrip->vertOffset );
							printf("\t\tnumBoneStateChanges %d %d :\n", pNewStrip->numBoneStateChanges, pNewStrip->boneStateChangeOffset );
							*/
							// printf("(%d)\n", delta );
						}
						// printf("(%d)\n", delta );
					}
				}
			}
		}
	}

	// Iterate over every body part...
	for ( i = 0; i < pStudioHdr->numbodyparts; i++ )
	{
		mstudiobodyparts_t* pBodyPart = pStudioHdr->pBodypart(i);

		// Iterate over every submodel...
		for (j = 0; j < pBodyPart->nummodels; ++j)
		{
			// link previous LODs to higher LODs
			for ( nLodID = 0; nLodID < rootLOD; nLodID++ )
			{
				OptimizedModel::ModelLODHeader_t *pVtxLOD = pVtxHdr->pBodyPart(i)->pModel(j)->pLOD(nLodID);
				OptimizedModel::ModelLODHeader_t *pRootVtxLOD = pNewVtxHdr->pBodyPart(i)->pModel(j)->pLOD(rootLOD);
				OptimizedModel::ModelLODHeader_t *pNewVtxLOD = pNewVtxHdr->pBodyPart(i)->pModel(j)->pLOD(nLodID);

				pNewVtxLOD->numMeshes = pRootVtxLOD->numMeshes;
				pNewVtxLOD->switchPoint = pVtxLOD->switchPoint;

				int delta = (byte *)pRootVtxLOD - (byte *)pNewVtxLOD;
				pNewVtxLOD->meshOffset = pRootVtxLOD->meshOffset + delta;
			}
		}
	}

	int newLen = pData - (byte *)pNewVtxHdr;
	// printf("len %d : %d\n", len, newLen );

	// pNewVtxHdr->length = newLen;

	if (!g_quiet)
	{
		printf ("writing %s:\n", fileName);
		printf( "everything (%d bytes)\n", newLen );
	}
	SaveFile( (char *)fileName, pNewVtxHdr, newLen );

	free( pNewVtxHdr );

	return true;
}




bool Clamp_RootLOD( studiohdr_t *phdr )
{
	char	filename[260];
	char	tmpFileName[260];
	int		i;
	const char						*vtxPrefixes[] = {".dx80.vtx", ".dx90.vtx", ".sw.vtx", ".xbox.vtx"};

	int rootLOD = g_minLod;

	if (rootLOD > g_ScriptLODs.Size() - 1)
	{
		rootLOD = g_ScriptLODs.Size() -1;
	}

	if (rootLOD == 0)
	{
		return true;
	}

	strcpy( filename, gamedir );
	strcat( filename, "models/" );	
	strcat( filename, outname );
	Q_StripExtension( filename, filename, sizeof( filename ) );

	// shift the files so that g_minLod is the root LOD
	strcpy( tmpFileName, filename );
	strcat( tmpFileName, ".mdl" );
	Clamp_MDL_LODS( tmpFileName, rootLOD );

	strcpy( tmpFileName, filename );
	strcat( tmpFileName, ".vvd" );
	Clamp_VVD_LODS( tmpFileName, rootLOD );

	for (i=0; i<ARRAYSIZE(vtxPrefixes); i++)
	{
		// fixup ???.vtx
		strcpy( tmpFileName, filename );
		strcat( tmpFileName, vtxPrefixes[i] );
		Clamp_VTX_LODS( tmpFileName, rootLOD, phdr );
	}

	return true;
}






