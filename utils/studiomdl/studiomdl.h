//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include <stdio.h>
#include "basetypes.h"
#include "utlvector.h"
#include "utlsymbol.h"
#include "vector.h"
#include "studio.h"

struct LodScriptData_t;

#define IDSTUDIOHEADER			(('T'<<24)+('S'<<16)+('D'<<8)+'I')
														// little-endian "IDST"
#define IDSTUDIOANIMGROUPHEADER	(('G'<<24)+('A'<<16)+('D'<<8)+'I')
														// little-endian "IDAG"


#define STUDIO_QUADRATIC_MOTION 0x00002000

#define MAXSTUDIOANIMFRAMES		2000	// max frames per animation
#define MAXSTUDIOANIMS			2000	// total animations
#define MAXSTUDIOSEQUENCES		1524	// total sequences
#define MAXSTUDIOSRCBONES		512		// bones allowed at source movement
#define MAXSTUDIOMODELS			32		// sub-models per model
#define MAXSTUDIOBODYPARTS		32
#define MAXSTUDIOMESHES			256
#define MAXSTUDIOEVENTS			1024
#define MAXSTUDIOFLEXKEYS		512
#define MAXSTUDIOFLEXRULES		1024
#define MAXSTUDIOBONEWEIGHTS	3
#define MAXSTUDIOCMDS			64
#define MAXSTUDIOMOVEKEYS		64
#define MAXSTUDIOIKRULES		64
#define MAXSTUDIONAME			128

#ifndef EXTERN
#define EXTERN extern
#endif

EXTERN	char		outname[1024];
//EXTERN	char		g_pPlatformName[1024];
EXTERN  qboolean	cdset;
EXTERN  int			numdirs;
EXTERN	char		cddir[32][MAX_PATH];
EXTERN	int			numcdtextures;
EXTERN	char *		cdtextures[16];
EXTERN  char		fullpath[1024];

EXTERN	char		rootname[MAXSTUDIONAME];		// name of the root bone
EXTERN	float		g_defaultscale;
EXTERN  float		g_currentscale;
EXTERN  RadianEuler	g_defaultrotation;


EXTERN	char		defaulttexture[16][MAX_PATH];
EXTERN	char		sourcetexture[16][MAX_PATH];

EXTERN	int			numrep;

EXTERN	int			tag_reversed;
EXTERN	int			tag_normals;
EXTERN	int			flip_triangles;
EXTERN	float		normal_blend;
EXTERN	int			dump_hboxes;
EXTERN	int			ignore_warnings;

EXTERN	Vector		eyeposition;
EXTERN	Vector		illumposition;
EXTERN	int			illumpositionset;
EXTERN	int			gflags;
EXTERN	Vector		bbox[2];
EXTERN	Vector		cbox[2];
EXTERN	bool		g_wrotebbox;
EXTERN	bool		g_wrotecbox;

EXTERN	int			clip_texcoords;
EXTERN	bool		g_staticprop;
EXTERN	bool		g_centerstaticprop;

EXTERN	bool		g_realignbones;
EXTERN	bool		g_definebones;

EXTERN  byte		g_constdirectionalightdot;

// Methods associated with the key value text block
extern CUtlVector< char >	g_KeyValueText;
int		KeyValueTextSize( CUtlVector< char > *pKeyValue );
const char *KeyValueText( CUtlVector< char > *pKeyValue );

extern vec_t Q_rint (vec_t in);

extern void WriteModelFiles(void);
void *kalloc( int num, int size );

struct s_trianglevert_t
{
	int					vertindex;
	int					normindex;		// index into normal array
	int					s,t;
	float				u,v;
};

struct s_boneweight_t
{
	int		numbones;

	int		bone[MAXSTUDIOBONEWEIGHTS];
	float	weight[MAXSTUDIOBONEWEIGHTS];
};


struct s_tmpface_t
{
	int	material;
	unsigned long		a, b, c;
	unsigned long		ta, tb, tc;
	unsigned long		na, nb, nc;
};

struct s_face_t
{
	unsigned long		a, b, c;
};


struct s_vertexinfo_t
{
	int				material;
	int				mesh;
	s_boneweight_t	globalBoneweight;
	Vector			position;	
	Vector			normal;
	Vector4D		tangentS;
	Vector2D		texcoord;
	int				bLoD;
};

//============================================================================

// dstudiobone_t bone[MAXSTUDIOBONES];
struct s_bonefixup_t
{
	matrix3x4_t m;	
};

EXTERN int g_numbones;
struct s_bonetable_t
{
	char			name[MAXSTUDIONAME];	// bone name for symbolic links
	int		 		parent;		// parent bone
	bool			split;
	int				bonecontroller;	// -1 == 0
	Vector			pos;		// default pos
	Vector			posscale;	// pos values scale
	RadianEuler		rot;		// default pos
	Vector			rotscale;	// rotation values scale
	int				group;		// hitgroup
	Vector			bmin, bmax;	// bounding box
	bool			bPreDefined;
	matrix3x4_t		rawLocalOriginal; // original transform of preDefined bone
	matrix3x4_t		rawLocal;
	matrix3x4_t		srcRealign;
	bool			bPreAligned;
	matrix3x4_t		boneToPose;
	int				flags;
	int				proceduralindex;
	int				physicsBoneIndex;
	int				surfacePropIndex;
	Quaternion		qAlignment;
	bool			bDontCollapse;
	Vector			posrange;
};
EXTERN	s_bonetable_t g_bonetable[MAXSTUDIOSRCBONES];
extern int findGlobalBone( const char *name );	// finds a named bone in the global bone table

EXTERN int g_numrenamedbones;
struct s_renamebone_t
{
	char			from[MAXSTUDIONAME];
	char			to[MAXSTUDIONAME];
};
EXTERN s_renamebone_t g_renamedbone[MAXSTUDIOSRCBONES];
const char *RenameBone( const char *pName ); // returns new name if available, else return pName.

EXTERN int g_numimportbones;
struct s_importbone_t
{
	char			name[MAXSTUDIONAME];
	char			parent[MAXSTUDIONAME];
	matrix3x4_t		rawLocal;
	bool			bPreAligned;
	matrix3x4_t		srcRealign;
};
EXTERN s_importbone_t g_importbone[MAXSTUDIOSRCBONES];


EXTERN int g_numincludemodels;
struct s_includemodel_t
{
	char			name[MAXSTUDIONAME];
};
EXTERN s_includemodel_t g_includemodel[128];

struct s_bbox_t
{
	char			name[MAXSTUDIONAME];		// bone name
	char			hitboxname[MAXSTUDIONAME];	// hitbox name
	int				bone;
	int				group;		// hitgroup
	int				model;
	Vector			bmin, bmax;	// bounding box
};

#define MAXSTUDIOHITBOXSETNAME 64

struct s_hitboxset
{
	char		hitboxsetname[ MAXSTUDIOHITBOXSETNAME ];
	
	int			numhitboxes;

	s_bbox_t	hitbox[MAXSTUDIOSRCBONES];
};

extern CUtlVector< s_hitboxset > g_hitboxsets;

EXTERN int g_numhitgroups;
struct s_hitgroup_t
{
	int				models;
	int				group;
	char			name[MAXSTUDIONAME];	// bone name
};
EXTERN s_hitgroup_t g_hitgroup[MAXSTUDIOSRCBONES];


struct s_bonecontroller_t
{
	char	name[MAXSTUDIONAME];
	int		bone;
	int		type;
	int		inputfield;
	float	start;
	float	end;
};

EXTERN s_bonecontroller_t g_bonecontroller[MAXSTUDIOSRCBONES];
EXTERN int g_numbonecontrollers;

struct s_screenalignedbone_t
{
	char	name[MAXSTUDIONAME];
	int		flags;
};

EXTERN s_screenalignedbone_t g_screenalignedbone[MAXSTUDIOSRCBONES];
EXTERN int g_numscreenalignedbones;

struct s_attachment_t
{
	char	name[MAXSTUDIONAME];
	char	bonename[MAXSTUDIONAME];
	int		bone;
	int		type;
	int		flags;
	matrix3x4_t	local;
	int		found;	// a owning bone has been flagged
};


#define IS_ABSOLUTE		0x0001
#define IS_RIGID		0x0002

EXTERN s_attachment_t g_attachment[MAXSTUDIOSRCBONES];
EXTERN int g_numattachments;

struct s_bonemerge_t
{
	char	bonename[MAXSTUDIONAME];
};

EXTERN CUtlVector< s_bonemerge_t > g_BoneMerge;

struct s_mouth_t
{
	char	bonename[MAXSTUDIONAME];
	int		bone;
	Vector	forward;
	int		flexdesc;
};

EXTERN s_mouth_t g_mouth[MAXSTUDIOSRCBONES]; // ?? skins?
EXTERN int g_nummouths;

struct s_node_t
{
	char			name[MAXSTUDIONAME];
	int				parent;
};

struct s_bone_t
{
	Vector			pos;
	RadianEuler		rot;
};

struct s_linearmove_t
{
	int				endframe;	// frame when pos, rot is valid.
	int				flags;		// type of motion.  Only linear, linear accel, and linear decel is allowed
	float			v0;
	float			v1;
	Vector			vector;		// movement vector
	Vector			pos;	// final position
	RadianEuler		rot;		// final rotation
};


#define CMD_WEIGHTS	1
#define CMD_SUBTRACT 2
#define CMD_AO		3
#define CMD_MATCH	4
#define CMD_FIXUP	5
#define CMD_ANGLE	6
#define CMD_IKFIXUP	7
#define CMD_IKRULE	8
#define CMD_MOTION	9
#define CMD_REFMOTION	10
#define CMD_DERIVATIVE 11
#define	CMD_NOANIMATION 12
#define CMD_LINEARDELTA 13
#define CMD_SPLINEDELTA 14
#define CMD_COMPRESS 15
#define CMD_NUMFRAMES 16
#define CMD_COUNTERROTATE 17
#define CMD_SETBONE 18
#define CMD_WORLDSPACEBLEND 19
#define CMD_MATCHBLEND 20

struct s_animation_t;
struct s_ikrule_t;


struct s_motion_t
{
	int				motiontype;
	int				iStartFrame;// starting frame to apply motion over
	int				iEndFrame;	// end frame to apply motion over
	int				iSrcFrame;	// frame that matches the "reference" animation
	s_animation_t	*pRefAnim;	// animation to match
	int				iRefFrame;	// reference animation's frame to match
};


struct s_animcmd_t
{
	int cmd;
	union 
	{
		struct
		{
			int				index;	
		} weightlist;

		struct
		{
			s_animation_t	*ref;
			int				frame;
			int				flags;
		} subtract;

		struct
		{
			s_animation_t	*ref;
			int				motiontype;
			int				srcframe;
			int				destframe;
			char			*pBonename;
		} ao;

		struct
		{
			s_animation_t	*ref;
			int				srcframe;
			int				destframe;
			int				destpre;
			int				destpost;
		} match;

		struct
		{
			s_animation_t	*ref;
			int				startframe;
			int				loops;
		} world;

		struct 
		{
			int				start;
			int				end;
		} fixuploop;

		struct 
		{
			float			angle;
		} angle;

		struct
		{
			s_ikrule_t		*pRule;
		} ikfixup;

		struct
		{
			s_ikrule_t		*pRule;
		} ikrule;

		struct
		{
			float			scale;
		} derivative;

		struct
		{
			int				flags;
		} linear;

		struct
		{
			int				frames;
		} compress;

		struct
		{
			int				frames;
		} numframes;

		struct
		{
			char			*pBonename;
			bool			bHasTarget;
			float 			targetAngle[3];
		} counterrotate;

		struct s_motion_t	motion;
	} u;
};

struct s_ikerror_t
{
	Vector pos;
	Quaternion q;
};

struct s_ikrule_t
{
	int		chain;

	int		index;
	int		type;
	int		slot;
	char	bonename[MAXSTUDIONAME];
	char	attachment[MAXSTUDIONAME];
	int		bone;
	Vector	pos;
	Quaternion q;
	float	height;
	float	floor;
	float	radius;

	int		start;
	int		peak;
	int		tail;
	int		end;

	int		contact;
	int		numerror;
	s_ikerror_t	*pError;

	bool	usesequence;
	bool	usesource;

	int		flags;

	float	scale[6];
	int		numanim[6];
	mstudioanimvalue_t *anim[6];
};


struct s_source_t;
EXTERN	int g_numani;
struct s_animation_t
{
	bool			isimplied;
	bool			isOverride;
	bool			doesOverride;
	int				index;
	char			name[MAXSTUDIONAME];
	char			filename[MAX_PATH];

	int				animgroup;
	/*
	int				animsubindex;

	// For sharing outside of current .mdl file
	bool			shared_group_checkvalidity;
	bool			shared_group_valid;
	char			shared_animgroup_file[ MAX_PATH ]; // share file name
	char			shared_animgroup_name[ MAXSTUDIONAME ]; // group name in share file
	int				shared_group_subindex;
	studioanimhdr_t *shared_group_header;
	*/

	float			fps;
	int				startframe;
	int				endframe;
	int				flags;
	// animations processed (time shifted, linearized, and bone adjusted ) from source animations
	s_bone_t		*sanim[MAXSTUDIOANIMFRAMES]; // [frame][bones];

	int				motiontype;

	int				fudgeloop;
	int				looprestart; // new starting frame for looping animations

	// piecewise linear motion
	int				numpiecewisekeys;
	s_linearmove_t	piecewisemove[MAXSTUDIOMOVEKEYS];

	// default adjustments
	Vector			adjust;
	float			scale; // ????
	RadianEuler		rotation; 

	s_source_t *source;

	Vector 			bmin;
	Vector			bmax;

	int				numframes;

	// compressed animation data
	int				numanim[MAXSTUDIOSRCBONES][6];
	mstudioanimvalue_t *anim[MAXSTUDIOSRCBONES][6];

	// int				weightlist;
	float			weight[MAXSTUDIOSRCBONES];
	float			posweight[MAXSTUDIOSRCBONES];

	int				numcmds;
	s_animcmd_t		cmds[MAXSTUDIOCMDS];

	int				numikrules;
	s_ikrule_t		ikrule[MAXSTUDIOIKRULES];
	bool			noAutoIK;
};
EXTERN	s_animation_t *g_panimation[MAXSTUDIOANIMS];


EXTERN  int	g_numcmdlists;
struct s_cmdlist_t
{
	char			name[MAXSTUDIONAME];
	int				numcmds;
	s_animcmd_t		cmds[MAXSTUDIOCMDS];
};
EXTERN	s_cmdlist_t g_cmdlist[MAXSTUDIOANIMS];


struct s_iklock_t
{
	char			name[MAXSTUDIONAME];
	int				chain;
	float			flPosWeight;
	float			flLocalQWeight;
};

EXTERN	int g_numikautoplaylocks;
EXTERN	s_iklock_t g_ikautoplaylock[16];


struct s_event_t
{
	int				event;
	int				frame;
	char			options[64];
	char			eventname[MAXSTUDIONAME];
};

struct s_autolayer_t
{
	char			name[MAXSTUDIONAME];
	int				sequence;
	int				flags;
	int				pose;
	float			start;
	float			peak;
	float			tail;
	float			end;
};


struct s_sequence_t
{
	char			name[MAXSTUDIONAME];
	char			activityname[MAXSTUDIONAME];	// index into the string table, the name of this activity.

	int				flags;
	// float			fps;
	// int				numframes;

	int				activity;
	int				actweight;

	int				numevents;
	s_event_t		event[MAXSTUDIOEVENTS];

	int				numblends;
	int				groupsize[2];
	s_animation_t	*panim[MAXSTUDIOBLENDS][MAXSTUDIOBLENDS];

	int				paramindex[2];
	float			paramstart[2];
	float			paramend[2];
	int				paramattachment[2];
	int				paramcontrol[2];
	float			param0[MAXSTUDIOBLENDS]; // [MAXSTUDIOBLENDS];
	float			param1[MAXSTUDIOBLENDS]; // [MAXSTUDIOBLENDS];
	s_animation_t	*paramanim;
	s_animation_t	*paramcompanim;
	s_animation_t	*paramcenter;

	// Vector			automovepos[MAXSTUDIOANIMATIONS];
	// Vector			automoveangle[MAXSTUDIOANIMATIONS];

	int				animindex;

	Vector 			bmin;
	Vector			bmax;

	float			fadeintime;
	float			fadeouttime;

	int				entrynode;
	int				exitnode;
	int				nodeflags;
	float			entryphase;
	float			exitphase;

	int				numikrules;

	int				numautolayers;
	s_autolayer_t	autolayer[64];

	float			weight[MAXSTUDIOSRCBONES];

	s_iklock_t		iklock[64];
	int				numiklocks;

	CUtlVector< char > KeyValue;
};
EXTERN	CUtlVector< s_sequence_t > g_sequence;
//EXTERN	int g_numseq;


EXTERN int g_numanimblocks;
struct s_animblock_t
{
	int		iStartAnim;
	int		iEndAnim;
	byte	*start;
	byte	*end;
};
EXTERN s_animblock_t g_animblock[MAXSTUDIOANIMBLOCKS];
EXTERN int g_animblocksize;
EXTERN char g_animblockname[260];



EXTERN int g_numposeparameters;
struct s_poseparameter_t
{
	char	name[MAXSTUDIONAME];
	float	min;
	float	max;
	int		flags;
	float	loop;
};
EXTERN s_poseparameter_t g_pose[32]; // FIXME: this shouldn't be hard coded


EXTERN int g_numxnodes;
EXTERN char *g_xnodename[100];
EXTERN int g_xnode[100][100];
EXTERN int g_numxnodeskips;
EXTERN int g_xnodeskip[10000][2];

struct rgb_t
{
	byte r, g, b;
};
struct rgb2_t
{
	float r, g, b, a;
};

// FIXME: what about texture overrides inline with loading models

struct s_texture_t
{
	char	name[MAX_PATH];
	int		flags;
	int		parent;
	int		material;
	float	width;
	float	height;
	float	dPdu;
	float	dPdv;
};
EXTERN	s_texture_t g_texture[MAXSTUDIOSKINS];
EXTERN	int g_numtextures;
EXTERN	int	g_material[MAXSTUDIOSKINS]; // link into texture array
EXTERN  int g_nummaterials;

EXTERN  float g_gamma;
EXTERN	int g_numskinref;
EXTERN  int g_numskinfamilies;
EXTERN  int g_skinref[256][MAXSTUDIOSKINS]; // [skin][skinref], returns texture index
EXTERN	int g_numtexturegroups;
EXTERN	int g_numtexturelayers[32];
EXTERN	int g_numtexturereps[32];
EXTERN  int g_texturegroup[32][32][32];

struct s_mesh_t
{
	int numvertices;
	int	vertexoffset;

	int numfaces;
	int	faceoffset;
};


struct s_vertanim_t
{
	int		vertex;
	float	speed;
	float	side;
	Vector	pos;
	Vector	normal;
};

// processed aggregate lod pools
struct s_loddata_t
{
	int				numvertices;
	s_vertexinfo_t	*vertex;

	int				numfaces;
	s_face_t		*face;

	s_mesh_t		mesh[MAXSTUDIOSKINS];

	// remaps verts from an lod's source mesh to this all-lod processed aggregate pool
	int				*pMeshVertIndexMaps[MAX_NUM_LODS];
};

// raw off-disk source files.  Raw data should be not processed.
struct s_source_t
{
	char	filename[MAX_PATH];
	int 	time;	// time stamp

	bool	isActiveModel;

	// local skeleton hierarchy
	int numbones;
	s_node_t localBone[MAXSTUDIOSRCBONES];
	matrix3x4_t boneToPose[MAXSTUDIOSRCBONES];	// converts bone local data into initial pose data

	// bone remapping
	int boneflags[MAXSTUDIOSRCBONES];	// attachment, vertex, etc flags for this bone
	int boneref[MAXSTUDIOSRCBONES];		// flags for this and child bones
	int	boneLocalToGlobal[MAXSTUDIOSRCBONES]; // bonemap : local bone to world bone mapping
	int	boneGlobalToLocal[MAXSTUDIOSRCBONES]; // boneimap : world bone to local bone mapping

	int	texmap[MAXSTUDIOSKINS*4];		// map local MAX materials to unique textures

	// per material mesh
	int				nummeshes;
	int				meshindex[MAXSTUDIOSKINS];	// mesh to skin index
	s_mesh_t		mesh[MAXSTUDIOSKINS];

	// vertex info about local bone weighting
	s_boneweight_t	*localBoneweight;

	// model global copy of vertices
	int				numvertices;
	s_vertexinfo_t	*vertex;

	int numfaces;
	s_face_t *face;						// vertex indexs per face

	// raw skeletal animation	
	int numframes;
	int startframe;
	int endframe;
	s_bone_t		*rawanim[MAXSTUDIOANIMFRAMES]; // [frame][bones];

	// vertex animation
	int				*vanim_mapcount;	// local verts map to N target verts
	int				**vanim_map;		// local vertices to target vertices mapping list
	int				*vanim_flag;		// local vert does animate

	int				numvanims[MAXSTUDIOANIMFRAMES];
	s_vertanim_t	*vanim[MAXSTUDIOANIMFRAMES];	// [frame][vertex]

	// processed aggregate lod data
	s_loddata_t		*pLodData;
};


EXTERN int g_numsources;
EXTERN s_source_t *g_source[MAXSTUDIOSEQUENCES];

struct s_eyeballvert_t
{
	int		vertex;		// vertex index
	int		texcoord;	// texture coordinate index
	int		normal;		// normal index
};

struct s_eyeball_t
{
	char	name[MAXSTUDIONAME];
	int		index;
	int		bone;
	Vector	org;
	float	zoffset;
	float	radius;
	Vector	up;
	Vector	forward;

	int		mesh;
	int		iris_material;
	float	iris_scale;
	int		glint_material;	// !!!

	int		upperflexdesc[3];
	int		lowerflexdesc[3];
	float	uppertarget[3];
	float	lowertarget[3];
	int		upperflex;
	int		lowerflex;

	int		upperlidflexdesc;
	int		lowerlidflexdesc;
};

struct s_model_t
{
	char name[MAXSTUDIONAME];
	char filename[MAX_PATH];

	// needs local scaling and rotation paramaters

	s_source_t	 *source; // index into source table

	float scale;	// UNUSED

	float boundingradius;

	Vector boundingbox[MAXSTUDIOSRCBONES][2];

	int	numattachments;
	s_attachment_t	attachment[32];

	int numeyeballs;
	s_eyeball_t		eyeball[4];

	int	numflexes;
	int flexoffset;
};
EXTERN	int g_nummodels;
EXTERN	int g_nummodelsbeforeLOD;
EXTERN	s_model_t *g_model[MAXSTUDIOMODELS];


struct s_flexdesc_t
{
	char FACS[MAXSTUDIONAME];	// FACS identifier
};
EXTERN int g_numflexdesc;
EXTERN s_flexdesc_t g_flexdesc[MAXSTUDIOFLEXDESC];


struct s_flexcontroller_t
{
	char name[MAXSTUDIONAME];
	char type[MAXSTUDIONAME];
	float min;
	float max;
};
EXTERN int g_numflexcontrollers;
EXTERN s_flexcontroller_t g_flexcontroller[MAXSTUDIOFLEXCTRL];


struct s_flexkey_t
{
	int	 flexdesc;
	int	 flexpair;
	
	s_source_t	 *source; // index into source table

	int	imodel;
	int	frame;

	float	target0;
	float	target1;
	float	target2;
	float	target3;

	int		original;
	float	split;

	float	decay;

	// extracted and remapped vertex animations
	int				numvanims;
	s_vertanim_t	*vanim;

	int	weighttable;
};
EXTERN int g_numflexkeys;
EXTERN s_flexkey_t g_flexkey[MAXSTUDIOFLEXKEYS];
EXTERN s_flexkey_t *g_defaultflexkey;

#define MAX_OPS 512

struct s_flexop_t
{
	int		op;
	union {
		int		index;
		float	value;
	} d;
};

struct s_flexrule_t
{
	int		flex;
	int		numops;
	s_flexop_t op[MAX_OPS];
};
EXTERN int g_numflexrules;
EXTERN s_flexrule_t g_flexrule[MAXSTUDIOFLEXRULES];

EXTERN	Vector g_defaultadjust;

struct s_bodypart_t
{
	char				name[MAXSTUDIONAME];
	int					nummodels;
	int					base;
	s_model_t			*pmodel[MAXSTUDIOMODELS];
};

EXTERN	int g_numbodyparts;
EXTERN	s_bodypart_t g_bodypart[MAXSTUDIOBODYPARTS];


#define MAXWEIGHTLISTS	32
#define MAXWEIGHTSPERLIST	16

struct s_weightlist_t
{
	// weights, indexed by numbones per weightlist
	char			name[MAXSTUDIONAME];
	int				numbones;
	char			bonename[MAXWEIGHTSPERLIST][MAXSTUDIONAME];
	float			boneweight[MAXWEIGHTSPERLIST];
	float			boneposweight[MAXWEIGHTSPERLIST];

	// weights, indexed by global bone index
	float			weight[MAXSTUDIOBONES];
	float			posweight[MAXSTUDIOBONES];
};

EXTERN	int	g_numweightlist;
EXTERN	s_weightlist_t g_weightlist[MAXWEIGHTLISTS];

struct s_iklink_t
{
	int		bone;
	Vector	kneeDir;
};

struct s_ikchain_t
{
	char			name[MAXSTUDIONAME];
	char			bonename[MAXSTUDIONAME];
	int				axis;
	float			value;
	int				numlinks;
	s_iklink_t		link[10]; // hip, knee, ankle, toes...
	float			height;
	float			radius;
	float			floor;
	Vector			center;
};

EXTERN	int g_numikchains;
EXTERN	s_ikchain_t g_ikchain[16];


struct s_axisinterpbone_t
{
	int				flags;
	char			bonename[MAXSTUDIONAME];
	int				bone;
	char			controlname[MAXSTUDIONAME];
	int				control;
	int				axis;
	Vector			pos[6];
	Quaternion		quat[6];
};

EXTERN int g_numaxisinterpbones;
EXTERN s_axisinterpbone_t g_axisinterpbones[MAXSTUDIOBONES];
EXTERN int g_axisinterpbonemap[MAXSTUDIOBONES]; // map used axisinterpbone's to source axisinterpbone's

struct s_quatinterpbone_t
{
	int				flags;
	char			bonename[MAXSTUDIONAME];
	int				bone;
	char			parentname[MAXSTUDIONAME];
	// int				parent;
	char			controlparentname[MAXSTUDIONAME];
	// int				controlparent;
	char			controlname[MAXSTUDIONAME];
	int				control;
	int				numtriggers;
	Vector			size;
	Vector			basepos;
	float			percentage;
	float			tolerance[32];
	Quaternion		trigger[32];
	Vector			pos[32];
	Quaternion		quat[32];
};

EXTERN int g_numquatinterpbones;
EXTERN s_quatinterpbone_t g_quatinterpbones[MAXSTUDIOBONES];
EXTERN int g_quatinterpbonemap[MAXSTUDIOBONES]; // map used quatinterpbone's to source axisinterpbone's


struct s_aimatbone_t
{
	char			bonename[MAXSTUDIONAME];
	int				bone;
	char			parentname[MAXSTUDIONAME];
	int				parent;
	char			aimname[MAXSTUDIONAME];
	int				aimAttach;
	int				aimBone;
	Vector			aimvector;
	Vector			upvector;
	Vector			basepos;
};

EXTERN int g_numaimatbones;
EXTERN s_aimatbone_t g_aimatbones[MAXSTUDIOBONES];
EXTERN int g_aimatbonemap[MAXSTUDIOBONES]; // map used aimatpbone's to source aimatpbone's (may be optimized out)


struct s_forcedhierarchy_t
{
	char			parentname[MAXSTUDIONAME];
	char			childname[MAXSTUDIONAME];
	char			subparentname[MAXSTUDIONAME];
};

EXTERN int g_numforcedhierarchy;
EXTERN s_forcedhierarchy_t g_forcedhierarchy[MAXSTUDIOBONES];

struct s_forcedrealign_t
{
	char			name[MAXSTUDIONAME];
	RadianEuler		rot;
};
EXTERN int g_numforcedrealign;
EXTERN s_forcedrealign_t g_forcedrealign[MAXSTUDIOBONES];

struct s_limitrotation_t
{
	char			name[MAXSTUDIONAME];
	int				numseq;
	char			*sequencename[64];
};

EXTERN int g_numlimitrotation;
EXTERN s_limitrotation_t g_limitrotation[MAXSTUDIOBONES];

extern int BuildTris (s_trianglevert_t (*x)[3], s_mesh_t *y, byte **ppdata );


struct s_bonesaveframe_t
{
	char		name[ MAXSTUDIOHITBOXSETNAME ];
	bool		bSavePos;
	bool		bSaveRot;
};

EXTERN CUtlVector< s_bonesaveframe_t > g_bonesaveframe;

EXTERN	int is_v1support;

int OpenGlobalFile( char *src );
s_source_t *Load_Source( char const *filename, const char *ext, bool reverse = false, bool isActiveModel = false );
extern int Load_VRM( s_source_t *psource );
extern int Load_SMD( s_source_t *psource );
extern int Load_VTA( s_source_t *psource );
extern int Load_OBJ( s_source_t *psource );
extern int AppendVTAtoOBJ( s_source_t *psource, char *filename, int frame );
extern void Build_Reference( s_source_t *psource);
extern int Grab_Nodes( s_node_t *pnodes );
extern void Grab_Animation( s_source_t *psource );

extern int lookup_texture( char *texturename, int maxlen );
extern int use_texture_as_material( int textureindex );
extern int material_to_texture( int material );

extern int LookupAttachment( char *name );

extern void ClearModel (void);
extern void SimplifyModel (void);
extern void CollapseBones (void);

extern void adjust_vertex( float *org );
extern void scale_vertex( Vector &org );
extern void clip_rotations( RadianEuler& rot );
extern void clip_rotations( Vector& rot );

extern void *kalloc( int num, int size );
extern void kmemset( void *ptr, int value, int size );
extern char *stristr( const char *string, const char *string2 );
#define strcpyn( a, b ) strncpy( a, b, sizeof( a ) )

void CalcBoneTransforms( s_animation_t *panimation, int frame, matrix3x4_t* pBoneToWorld );
void CalcBoneTransforms( s_animation_t *panimation, s_animation_t *pbaseanimation, int frame, matrix3x4_t* pBoneToWorld );
void CalcBoneTransformsCycle( s_animation_t *panimation, s_animation_t *pbaseanimation, float flCycle, matrix3x4_t* pBoneToWorld );

// Returns surface property for a given joint
char* GetSurfaceProp ( char const* pJointName );
int GetContents ( char const* pJointName );
char* GetDefaultSurfaceProp ( );
int GetDefaultContents( );

// Did we read 'end'
bool IsEnd( char const* pLine );

// Parses an LOD command
void Cmd_LOD( char const *cmdname );
void Cmd_ShadowLOD( void );

// Fixes up the LOD source files
void FixupLODSources();

// Get model LOD source
s_source_t* GetModelLODSource( const char *pModelName, 
						const LodScriptData_t& scriptLOD, bool* pFound );


void LoadLODSources( void );
void ConvertBoneTreeCollapsesToReplaceBones( void );
void FixupReplacedBones( void );
void UnifyLODs( void );
void SpewBoneUsageStats( void );
void MarkParentBoneLODs( void );
//void CheckAutoShareAnimationGroup( char const *animation_name );

/*
=================
=================
*/

extern char	g_szFilename[1024];
extern FILE	*g_fpInput;
extern char	g_szLine[4096];
extern int	g_iLinecount;

extern int g_min_faces, g_max_faces;
extern float g_min_resolution, g_max_resolution;

EXTERN	int g_numverts;
EXTERN	Vector g_vertex[MAXSTUDIOVERTS];
EXTERN	s_boneweight_t g_bone[MAXSTUDIOVERTS];

EXTERN	int g_numnormals;
EXTERN	Vector g_normal[MAXSTUDIOVERTS];

EXTERN	int g_numtexcoords;
EXTERN	Vector2D g_texcoord[MAXSTUDIOVERTS];

EXTERN	int g_numfaces;
EXTERN	s_tmpface_t g_face[MAXSTUDIOTRIANGLES];
EXTERN	s_face_t g_src_uface[MAXSTUDIOTRIANGLES];	// max res unified faces

struct v_unify_t
{
	int	refcount;
	int	lastref;
	int	firstref;
	int	v;
	int m;
	int n;
	int t;
	v_unify_t *next;
};

EXTERN	v_unify_t *v_list[MAXSTUDIOVERTS];
EXTERN	v_unify_t v_listdata[MAXSTUDIOVERTS];
EXTERN	int numvlist;

int SortAndBalanceBones( int iCount, int iMaxCount, int bones[], float weights[] );
void Grab_Vertexanimation( s_source_t *psource );
extern void BuildIndividualMeshes( s_source_t *psource );

//-----------------------------------------------------------------------------
// A little class used to deal with replacement commands
//-----------------------------------------------------------------------------

class CLodScriptReplacement_t
{
public:
	void SetSrcName( const char *pSrcName )
	{
		if( m_pSrcName )
		{
			delete [] m_pSrcName;
		}
		m_pSrcName = new char[strlen( pSrcName ) + 1];
		strcpy( m_pSrcName, pSrcName );
	}
	void SetDstName( const char *pDstName )
	{
		if( m_pDstName )
		{
			delete [] m_pDstName;
		}
		m_pDstName = new char[strlen( pDstName ) + 1];
		strcpy( m_pDstName, pDstName );
	}

	const char *GetSrcName( void ) const 
	{
		return m_pSrcName;
	}
	const char *GetDstName( void ) const
	{
		return m_pDstName;
	}
	CLodScriptReplacement_t()
	{
		m_pSrcName = NULL;
		m_pDstName = NULL;
		m_pSource = 0;
	}
	~CLodScriptReplacement_t()
	{
		delete [] m_pSrcName;
		delete [] m_pDstName;
	}

	s_source_t*	m_pSource;

private:
	char *m_pSrcName;
	char *m_pDstName;
	bool m_bReverse;
};


struct LodScriptData_t
{
public:
	float switchValue;
	CUtlVector<CLodScriptReplacement_t> modelReplacements;
	CUtlVector<CLodScriptReplacement_t> boneReplacements;
	CUtlVector<CLodScriptReplacement_t> boneTreeCollapses;
	CUtlVector<CLodScriptReplacement_t> materialReplacements;
	CUtlVector<CLodScriptReplacement_t> meshRemovals;


	void EnableFacialAnimation( bool val )
	{
		m_bFacialAnimation = val;
	}
	bool GetFacialAnimationEnabled() const 
	{
		return m_bFacialAnimation;
	}
	LodScriptData_t()
	{
		m_bFacialAnimation = true;
	}
private:
	bool m_bFacialAnimation;
};

EXTERN CUtlVector<LodScriptData_t> g_ScriptLODs;

extern bool g_collapse_bones;
extern bool g_quiet;
extern bool g_verbose;
extern bool g_bCheckLengths;
extern bool g_bPrintBones;
extern bool g_bPerf;
extern bool g_bFast;
extern bool g_bDumpGraph;
extern bool g_bMultistageGraph;
extern bool g_bCreateMakefile;
extern bool g_bZBrush;
extern bool g_bVerifyOnly;
extern bool g_bUseBoneInBBox;
extern bool g_bLockBoneLengths;
extern bool g_bOverridePreDefinedBones;
extern bool g_bXbox;
extern int g_minLod;

EXTERN int g_numcollapse;
EXTERN char *g_collapse[MAXSTUDIOSRCBONES];

extern float GetCollisionModelMass();


// the first time these are called, the name of the model/QC file is printed so that when 
// running in batch mode, no echo, when dumping to a file, it can be determined which file is broke.
extern void MdlError( char const *pMsg, ... );
extern void MdlWarning( char const *pMsg, ... );

extern void CreateMakefile_AddDependency( const char *pFileName );

extern bool ComparePath( const char *a, const char *b );

extern byte IsByte( int val );
extern char IsChar( int val );
extern int IsInt24( int val );
extern short IsShort( int val );
extern unsigned short IsUShort( int val );



