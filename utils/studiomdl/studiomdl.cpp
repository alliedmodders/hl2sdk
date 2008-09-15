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

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <math.h>
#include "istudiorender.h"
#include "filesystem_tools.h"

#include "cmdlib.h"
#include "scriplib.h"
#include "mathlib.h"
#define EXTERN
#include "studio.h"
#include "studiomdl.h"
#include "collisionmodel.h"
#include "optimize.h"
#include "vstdlib/strtools.h"
#include "bspflags.h"
#include "vstdlib/icommandline.h"
#include "utldict.h"


bool g_collapse_bones = false;
bool g_quiet = false;
bool g_badCollide = false;
bool g_IHVTest = false;
bool g_bCheckLengths = false;
bool g_bPrintBones = false;
bool g_bPerf = false;
bool g_bDumpGraph = false;
bool g_bMultistageGraph = false;
bool g_verbose = false;
bool g_bCreateMakefile = false;
bool g_bHasModelName = false;
bool g_bZBrush = false;
bool g_bVerifyOnly = false;
bool g_bUseBoneInBBox = true;
bool g_bLockBoneLengths = false;
bool g_bOverridePreDefinedBones = true;
bool g_bXbox = false;
int g_minLod = 0;
bool g_bNoWarnings = false;

char g_path[1024];


CUtlVector< s_hitboxset > g_hitboxsets;
CUtlVector< char >	g_KeyValueText;


//-----------------------------------------------------------------------------
//  Stuff for writing a makefile to build models incrementally.
//-----------------------------------------------------------------------------
CUtlVector<CUtlSymbol> m_CreateMakefileDependencies;

void CreateMakefile_AddDependency( const char *pFileName )
{
	if( !g_bCreateMakefile )
	{
		return;
	}
	CUtlSymbol sym( pFileName );
	int i;
	for( i = 0; i < m_CreateMakefileDependencies.Count(); i++ )
	{
		if( m_CreateMakefileDependencies[i] == sym )
		{
			return;
		}
	}
	m_CreateMakefileDependencies.AddToTail( sym );
}

void CreateMakefile_OutputMakefile( void )
{
	if( !g_bHasModelName )
	{
		MdlError( "Can't write makefile since a target mdl hasn't been specified!" );
	}
	FILE *fp = fopen( "makefile.tmp", "a" );
	if( !fp )
	{
		MdlError( "can't open makefile.tmp!\n" );
	}
	char mdlname[MAX_PATH];
	strcpy( mdlname, gamedir );
//	if( *g_pPlatformName )
//	{
//		strcat( mdlname, "platform_" );
//		strcat( mdlname, g_pPlatformName );
//		strcat( mdlname, "/" );	
//	}
	strcat( mdlname, "models/" );	
	strcat( mdlname, outname );
	Q_StripExtension( mdlname, mdlname, sizeof( mdlname ) );
	strcat( mdlname, ".mdl" );
	Q_FixSlashes( mdlname );

	fprintf( fp, "%s:", mdlname );
	int i;
	for( i = 0; i < m_CreateMakefileDependencies.Count(); i++ )
	{
		fprintf( fp, " %s", m_CreateMakefileDependencies[i].String() );
	}
	fprintf( fp, "\n" );
	char mkdirpath[MAX_PATH];
	strcpy( mkdirpath, mdlname );
	Q_StripFilename( mkdirpath );
	fprintf( fp, "\tmkdir \"%s\"\n", mkdirpath );
	fprintf( fp, "\t%s -quiet %s\n\n", CommandLine()->GetParm( 0 ), fullpath );
	fclose( fp );
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------

static bool g_bFirstWarning = true;

void TokenError( char const *fmt, ... )
{
	static char output[1024];
	va_list		args;

	char *pFilename;
	int iLineNumber;

	if (GetTokenizerStatus( &pFilename, &iLineNumber ))
	{
		va_start( args, fmt );
		vsprintf( output, fmt, args );

		MdlError( "%s(%d): - %s", pFilename, iLineNumber, output );
	}
	else
	{
		va_start( args, fmt );
		vsprintf( output, fmt, args );
		MdlError( "%s", output );
	}
}

void MdlError( char const *fmt, ... )
{
	static char output[1024];
	static char *knownExtensions[] = {".mdl", ".ani", ".phy", ".sw.vtx", ".dx80.vtx", ".dx90.vtx", ".xbox.vtx", ".vvd"};
	char		fileName[MAX_PATH];
	char		baseName[MAX_PATH];
	va_list		args;

	Assert( 0 );
	if (g_quiet)
	{
		if (g_bFirstWarning)
		{
			printf("%s :\n", fullpath );
			g_bFirstWarning = false;
		}
		printf("\t");
	}

	printf("ERROR: ");
	va_start( args, fmt );
	vprintf( fmt, args );

	// delete premature files
	// unforunately, content is built without verification
	// ensuring that targets are not available, prevents check-in
	if (g_bHasModelName)
	{
		// undescriptive errors in batch processes could be anonymous
		printf("ERROR: Aborted Processing on '%s'\n", outname);

		strcpy( fileName, gamedir );
		strcat( fileName, "models/" );	
		strcat( fileName, outname );
		Q_FixSlashes( fileName );
		Q_StripExtension( fileName, baseName, sizeof( baseName ) );

		for (int i=0; i<ARRAYSIZE(knownExtensions); i++)
		{
			strcpy( fileName, baseName);
			strcat( fileName, knownExtensions[i] );

			// really need filesystem concept here
//			g_pFileSystem->RemoveFile( fileName );
			unlink( fileName );
		}
	}

	exit( -1 );
}


void MdlWarning( const char *fmt, ... )
{
	va_list args;
	static char output[1024];

	if (g_bNoWarnings)
		return;

	if (g_quiet)
	{
		if (g_bFirstWarning)
		{
			printf("%s :\n", fullpath );
			g_bFirstWarning = false;
		}
		printf("\t");
	}

	Assert( 0 );

	printf("WARNING: ");
	va_start( args, fmt );
	vprintf( fmt, args );
}

#ifndef _DEBUG

void MdlHandleCrash( const char *pMessage, bool bAssert )
{
	static LONG crashHandlerCount = 0;
	if ( InterlockedIncrement( &crashHandlerCount ) == 1 )
	{
		MdlError( "'%s' (assert: %d)\n", pMessage, bAssert );
	}

	InterlockedDecrement( &crashHandlerCount );
}


// This is called if we crash inside our crash handler. It just terminates the process immediately.
LONG __stdcall MdlSecondExceptionFilter( struct _EXCEPTION_POINTERS *ExceptionInfo )
{
	TerminateProcess( GetCurrentProcess(), 2 );
	return EXCEPTION_EXECUTE_HANDLER; // (never gets here anyway)
}


void MdlExceptionFilter( unsigned long code )
{
	// This is called if we crash inside our crash handler. It just terminates the process immediately.
	SetUnhandledExceptionFilter( MdlSecondExceptionFilter );

	//DWORD code = ExceptionInfo->ExceptionRecord->ExceptionCode;

	#define ERR_RECORD( name ) { name, #name }
	struct
	{
		int code;
		char *pReason;
	} errors[] =
	{
		ERR_RECORD( EXCEPTION_ACCESS_VIOLATION ),
		ERR_RECORD( EXCEPTION_ARRAY_BOUNDS_EXCEEDED ),
		ERR_RECORD( EXCEPTION_BREAKPOINT ),
		ERR_RECORD( EXCEPTION_DATATYPE_MISALIGNMENT ),
		ERR_RECORD( EXCEPTION_FLT_DENORMAL_OPERAND ),
		ERR_RECORD( EXCEPTION_FLT_DIVIDE_BY_ZERO ),
		ERR_RECORD( EXCEPTION_FLT_INEXACT_RESULT ),
		ERR_RECORD( EXCEPTION_FLT_INVALID_OPERATION ),
		ERR_RECORD( EXCEPTION_FLT_OVERFLOW ),
		ERR_RECORD( EXCEPTION_FLT_STACK_CHECK ),
		ERR_RECORD( EXCEPTION_FLT_UNDERFLOW ),
		ERR_RECORD( EXCEPTION_ILLEGAL_INSTRUCTION ),
		ERR_RECORD( EXCEPTION_IN_PAGE_ERROR ),
		ERR_RECORD( EXCEPTION_INT_DIVIDE_BY_ZERO ),
		ERR_RECORD( EXCEPTION_INT_OVERFLOW ),
		ERR_RECORD( EXCEPTION_INVALID_DISPOSITION ),
		ERR_RECORD( EXCEPTION_NONCONTINUABLE_EXCEPTION ),
		ERR_RECORD( EXCEPTION_PRIV_INSTRUCTION ),
		ERR_RECORD( EXCEPTION_SINGLE_STEP ),
		ERR_RECORD( EXCEPTION_STACK_OVERFLOW ),
		ERR_RECORD( EXCEPTION_ACCESS_VIOLATION ),
	};

	int nErrors = sizeof( errors ) / sizeof( errors[0] );
	int i = 0;
	for ( i=0; i < nErrors; i++ )
	{
		if ( errors[i].code == code )
			MdlHandleCrash( errors[i].pReason, true );
	}

	if ( i == nErrors )
	{
		MdlHandleCrash( "Unknown reason", true );
	}

	TerminateProcess( GetCurrentProcess(), 1 );
}

#endif

/*
=================
=================
*/

int k_memtotal;
void *kalloc( int num, int size )
{
	// printf( "calloc( %d, %d )\n", num, size );
	// printf( "%d ", num * size );
	k_memtotal += num * size;
	// ensure memory alignment on maximum of ALIGN
	void *ptr = calloc( num, size + 511 );
	ptr = (byte *)((int)((byte *)ptr + 511) & ~ 511);
	return ptr;
}

void kmemset( void *ptr, int value, int size )
{
	// printf( "kmemset( %x, %d, %d )\n", ptr, value, size );
	memset( ptr, value, size );
	return;
}


int verify_atoi( const char *token )
{
	if (token[0] != '-' && (token[0] < '0' || token[0] > '9'))
	{
		TokenError( "expecting number, got \"%s\"\n", token );
	}
	return atoi( token );
}

float verify_atof( const char *token )
{
	if (token[0] != '-' && token[0] != '.' && (token[0] < '0' || token[0] > '9'))
	{
		TokenError( "expecting number, got \"%s\"\n", token );
	}
	return atof( token );
}
//-----------------------------------------------------------------------------
// Key value block
//-----------------------------------------------------------------------------
static void AppendKeyValueText( CUtlVector< char > *pKeyValue, const char *pString )
{
	int nLen = strlen(pString);
	int nFirst = pKeyValue->AddMultipleToTail( nLen );
	memcpy( pKeyValue->Base() + nFirst, pString, nLen );
}

int	KeyValueTextSize( CUtlVector< char > *pKeyValue )
{
	return pKeyValue->Count();
}

const char *KeyValueText( CUtlVector< char > *pKeyValue )
{
	return pKeyValue->Base();
}

void Option_KeyValues( CUtlVector< char > *pKeyValue );

/*
=================
=================
*/


/*
=================
=================
*/




int lookupControl( char *string )
{
	if (stricmp(string,"X")==0) return STUDIO_X;
	if (stricmp(string,"Y")==0) return STUDIO_Y;
	if (stricmp(string,"Z")==0) return STUDIO_Z;
	if (stricmp(string,"XR")==0) return STUDIO_XR;
	if (stricmp(string,"YR")==0) return STUDIO_YR;
	if (stricmp(string,"ZR")==0) return STUDIO_ZR;

	if (stricmp(string,"LX")==0) return STUDIO_LX;
	if (stricmp(string,"LY")==0) return STUDIO_LY;
	if (stricmp(string,"LZ")==0) return STUDIO_LZ;
	if (stricmp(string,"LXR")==0) return STUDIO_LXR;
	if (stricmp(string,"LYR")==0) return STUDIO_LYR;
	if (stricmp(string,"LZR")==0) return STUDIO_LZR;

	if (stricmp(string,"LM")==0) return STUDIO_LINEAR;
	if (stricmp(string,"LQ")==0) return STUDIO_QUADRATIC_MOTION;

	return -1;
}



/*
=================
=================
*/

int LookupPoseParameter( char *name )
{
	int i;
	for ( i = 0; i < g_numposeparameters; i++)
	{
		if (!stricmp( name, g_pose[i].name))
		{
			return i;
		}
	}
	strcpyn( g_pose[i].name, name );
	g_numposeparameters = i + 1;

	if (g_numposeparameters > MAXSTUDIOPOSEPARAM)
	{
		TokenError( "too many pose parameters (max %d)\n", MAXSTUDIOPOSEPARAM );
	}

	return i;
}

void Cmd_PoseParameter( )
{
	if (g_numposeparameters >= MAXSTUDIOPOSEPARAM)
	{
		TokenError( "too many pose parameters (max %d)\n", MAXSTUDIOPOSEPARAM );
	}

	int i = LookupPoseParameter( token );

	// name
	GetToken (false);
	strcpyn( g_pose[i].name, token );

	if (TokenAvailable())
	{
		// min
		GetToken (false);
		g_pose[i].min = verify_atof (token);
	}

	if (TokenAvailable())
	{
		// max
		GetToken (false);
		g_pose[i].max = verify_atof (token);
	}

	while (TokenAvailable())
	{
		GetToken (false);

		if (!stricmp( token, "wrap" ))
		{
			g_pose[i].flags |= STUDIO_LOOPING;
			g_pose[i].loop = g_pose[i].max - g_pose[i].min;
		}
		else if (!stricmp( token, "loop" ))
		{
			g_pose[i].flags |= STUDIO_LOOPING;
			GetToken (false);
			g_pose[i].loop = verify_atof( token );
		}
	}
}


/*
=================
=================
*/

// search case-insensitive for string2 in string
char *stristr( const char *string, const char *string2 )
{
	int c, len;
	c = tolower( *string2 );
	len = strlen( string2 );

	while (string) {
		for (; *string && tolower( *string ) != c; string++);
		if (*string) {
			if (strnicmp( string, string2, len ) == 0) {
				break;
			}
			string++;
		}
		else {
			return NULL;
		}
	}
	return (char *)string;
}

/*
=================
=================
*/


int lookup_texture( char *texturename, int maxlen )
{
	int i;

	Q_StripExtension( texturename, texturename, maxlen );

	for (i = 0; i < g_numtextures; i++) 
	{
		if (stricmp( g_texture[i].name, texturename ) == 0) 
		{
			return i;
		}
	}

	if (i >= MAXSTUDIOSKINS)
		MdlError("Too many materials used, max %d\n", ( int )MAXSTUDIOSKINS );

//	printf( "texture %d = %s\n", i, texturename );
	strcpyn( g_texture[i].name, texturename );

	g_texture[i].material = -1;
	/*
	if (stristr( texturename, "chrome" ) != NULL) {
		texture[i].flags = STUDIO_NF_FLATSHADE | STUDIO_NF_CHROME;
	}
	else {
		texture[i].flags = 0;
	}
	*/
	g_numtextures++;
	return i;
}


void Cmd_RenameMaterial( void )
{
	char from[256];
	char to[256];

	GetToken( false );
	strcpy( from, token );

	GetToken( false );
	strcpy( to, token );

	int i;
	for (i = 0; i < g_numtextures; i++) 
	{
		if (stricmp( g_texture[i].name, from ) == 0) 
		{
			strcpy( g_texture[i].name, to );
			return;
		}
	}
	MdlError( "unknown material \"%s\" in rename\n", from );
}


int use_texture_as_material( int textureindex )
{
	if (g_texture[textureindex].material == -1)
	{
		// printf("%d %d %s\n", textureindex, g_nummaterials, g_texture[textureindex].name );
		g_material[g_nummaterials] = textureindex;
		g_texture[textureindex].material = g_nummaterials++;
	}

	return g_texture[textureindex].material;
}

int material_to_texture( int material )
{
	int i;
	for (i = 0; i < g_numtextures; i++)
	{
		if (g_texture[i].material == material)
		{
			return i;
		}
	}
	return -1;
}

//Wrong name for the use of it.
void scale_vertex( Vector &org )
{
	org[0] = org[0] * g_currentscale;
	org[1] = org[1] * g_currentscale;
	org[2] = org[2] * g_currentscale;
}



void SetSkinValues( )
{
	int			i, j;
	int			index;

	if (numcdtextures == 0)
	{
		char szName[256];

		// strip down till it finds "models"
		strcpyn( szName, fullpath );
		while (szName[0] != '\0' && strnicmp( "models", szName, 6 ) != 0)
		{
			strcpy( &szName[0], &szName[1] );
		}
		if (szName[0] != '\0')
		{
			Q_StripFilename( szName );
			strcat( szName, "/" );
		}
		else
		{
//			if( *g_pPlatformName )
//			{
//				strcat( szName, "platform_" );
//				strcat( szName, g_pPlatformName );
//				strcat( szName, "/" );	
//			}
			strcpy( szName, "models/" );	
			strcat( szName, outname );
			Q_StripExtension( szName, szName, sizeof( szName ) );
			strcat( szName, "/" );
		}
		cdtextures[0] = strdup( szName );
		numcdtextures = 1;
	}

	for (i = 0; i < g_numtextures; i++)
	{
		char szName[256];
		Q_StripExtension( g_texture[i].name, szName, sizeof( szName ) );
		Q_strncpy( g_texture[i].name, szName, sizeof( g_texture[i].name ) );
	}

	// build texture groups
	for (i = 0; i < MAXSTUDIOSKINS; i++)
	{
		for (j = 0; j < MAXSTUDIOSKINS; j++)
		{
			g_skinref[i][j] = j;
		}
	}
	index = 0;
	for (i = 0; i < g_numtexturelayers[0]; i++)
	{
		for (j = 0; j < g_numtexturereps[0]; j++)
		{
			g_skinref[i][g_texturegroup[0][0][j]] = g_texturegroup[0][i][j];
		}
	}

	if (i != 0)
	{
		g_numskinfamilies = i;
	}
	else
	{
		g_numskinfamilies = 1;
	}
	g_numskinref = g_numtextures;

	// printf ("width: %i  height: %i\n",width, height);
	/*
	printf ("adjusted width: %i height: %i  top : %i  left: %i\n",
			pmesh->skinwidth, pmesh->skinheight, pmesh->skintop, pmesh->skinleft );
	*/
}

/*
=================
=================
*/


int LookupXNode( char *name )
{
	int i;
	for ( i = 1; i <= g_numxnodes; i++)
	{
		if (stricmp( name, g_xnodename[i] ) == 0)
		{
			return i;
		}
	}
	g_xnodename[i] = strdup( name );
	g_numxnodes = i;
	return i;
}


/*
=================
=================
*/

char	g_szFilename[1024];
FILE	*g_fpInput;
char	g_szLine[4096];
int		g_iLinecount;


void Build_Reference( s_source_t *psource)
{
	int		i, parent;
	Vector	angle;

	for (i = 0; i < psource->numbones; i++)
	{
		matrix3x4_t m;
		AngleMatrix( psource->rawanim[0][i].rot, m );
		m[0][3] = psource->rawanim[0][i].pos[0];
		m[1][3] = psource->rawanim[0][i].pos[1];
		m[2][3] = psource->rawanim[0][i].pos[2];

		parent = psource->localBone[i].parent;
		if (parent == -1) 
		{
			// scale the done pos.
			// calc rotational matrices
			MatrixCopy( m, psource->boneToPose[i] );
		}
		else 
		{
			// calc compound rotational matrices
			// FIXME : Hey, it's orthogical so inv(A) == transpose(A)
			ConcatTransforms( psource->boneToPose[parent], m, psource->boneToPose[i] );
		}
		// printf("%3d %f %f %f\n", i, psource->bonefixup[i].worldorg[0], psource->bonefixup[i].worldorg[1], psource->bonefixup[i].worldorg[2] );
		/*
		AngleMatrix( angle, m );
		printf("%8.4f %8.4f %8.4f\n", m[0][0], m[1][0], m[2][0] );
		printf("%8.4f %8.4f %8.4f\n", m[0][1], m[1][1], m[2][1] );
		printf("%8.4f %8.4f %8.4f\n", m[0][2], m[1][2], m[2][2] );
		*/
	}
}




int Grab_Nodes( s_node_t *pnodes )
{
	int index;
	char name[1024];
	int parent;
	int numbones = 0;

	for (index = 0; index < MAXSTUDIOSRCBONES; index++)
	{
		pnodes[index].parent = -1;
	}

	while (fgets( g_szLine, sizeof( g_szLine ), g_fpInput ) != NULL) 
	{
		g_iLinecount++;
		if (sscanf( g_szLine, "%d \"%[^\"]\" %d", &index, name, &parent ) == 3)
		{
			// check for duplicated bones
			/*
			if (strlen(pnodes[index].name) != 0)
			{
				MdlError( "bone \"%s\" exists more than once\n", name );
			}
			*/
			
			strcpyn( pnodes[index].name, name );
			pnodes[index].parent = parent;
			if (index > numbones)
			{
				numbones = index;
			}
		}
		else 
		{
			return numbones + 1;
		}
	}
	MdlError( "Unexpected EOF at line %d\n", g_iLinecount );
	return 0;
}




void clip_rotations( RadianEuler& rot )
{
	int j;
	// clip everything to : -M_PI <= x < M_PI

	for (j = 0; j < 3; j++) {
		while (rot[j] >= M_PI) 
			rot[j] -= M_PI*2;
		while (rot[j] < -M_PI) 
			rot[j] += M_PI*2;
	}
}


void clip_rotations( Vector& rot )
{
	int j;
	// clip everything to : -180 <= x < 180

	for (j = 0; j < 3; j++) {
		while (rot[j] >= 180) 
			rot[j] -= 180*2;
		while (rot[j] < -180) 
			rot[j] += 180*2;
	}
}



/*
=================
Cmd_Eyeposition
=================
*/
void Cmd_Eyeposition (void)
{
// rotate points into frame of reference so g_model points down the positive x
// axis
	//	FIXME: these coords are bogus
	GetToken (false);
	eyeposition[1] = verify_atof (token);

	GetToken (false);
	eyeposition[0] = -verify_atof (token);

	GetToken (false);
	eyeposition[2] = verify_atof (token);
}



/*
=================
Cmd_Eyeposition
=================
*/
void Cmd_Illumposition (void)
{
// rotate points into frame of reference so g_model points down the positive x
// axis
	//	FIXME: these coords are bogus
	GetToken (false);
	illumposition[1] = verify_atof (token);

	GetToken (false);
	illumposition[0] = -verify_atof (token);

	GetToken (false);
	illumposition[2] = verify_atof (token);

	illumpositionset = true;
}


/*
=================
Cmd_Modelname
=================
*/
void Cmd_Modelname (void)
{
	g_bHasModelName = true;
	GetToken (false);
	
	if ( token[0] == '/' || token[0] == '\\' )
	{
		MdlWarning( "$modelname key has slash as first character. Removing.\n" );
		Q_strncpy( outname, &token[1], sizeof( outname ) );
	}
	else
	{
		Q_strncpy( outname, token, sizeof( outname ) );
	}
}

void Cmd_Autocenter()
{
	g_centerstaticprop = true;
}

/*
===============
===============
*/


void Option_Studio( s_model_t *pmodel )
{
	if (!GetToken (false)) return;
	strcpyn( pmodel->filename, token );

	// pmodel = (s_model_t *)kalloc( 1, sizeof( s_model_t ) );
	// g_bodypart[g_numbodyparts].pmodel[g_bodypart[g_numbodyparts].nummodels] = pmodel;


	flip_triangles = 1;

	pmodel->scale = g_currentscale = g_defaultscale;

	while (TokenAvailable())
	{
		GetToken(false);
		if (stricmp( "reverse", token ) == 0)
		{
			flip_triangles = 0;
		}
		else if (stricmp( "scale", token ) == 0)
		{
			GetToken(false);
			pmodel->scale = g_currentscale = verify_atof( token );
		}
		else if (stricmp( "faces", token ) == 0)
		{
			GetToken( false );
			GetToken( false );
		}
		else if (stricmp( "bias", token ) == 0)
		{
			GetToken( false );
		}
		else if (stricmp("{", token ) == 0)
		{
			UnGetToken( );
			break;
		}
		else
		{
			MdlError("unknown command \"%s\"\n", token );
		}
	}

	// load source
	pmodel->source = Load_Source( pmodel->filename, "", false, true );

	//Reset currentscale to whatever global we currently have set
	//g_defaultscale gets set in Cmd_ScaleUp everytime the $scale command is used.
	g_currentscale = g_defaultscale;
}


int Option_Blank( )
{
	g_model[g_nummodels] = (s_model_t *)kalloc( 1, sizeof( s_model_t ) );

	g_source[g_numsources] = (s_source_t *)kalloc( 1, sizeof( s_source_t ) );
	g_model[g_nummodels]->source = g_source[g_numsources];
	g_numsources++;

	g_bodypart[g_numbodyparts].pmodel[g_bodypart[g_numbodyparts].nummodels] = g_model[g_nummodels];

	strcpyn( g_model[g_nummodels]->name, "blank" );

	g_bodypart[g_numbodyparts].nummodels++;
	g_nummodels++;
	return 0;
}


void Cmd_Bodygroup( )
{
	int is_started = 0;

	if (!GetToken(false)) return;

	if (g_numbodyparts == 0) {
		g_bodypart[g_numbodyparts].base = 1;
	}
	else {
		g_bodypart[g_numbodyparts].base = g_bodypart[g_numbodyparts-1].base * g_bodypart[g_numbodyparts-1].nummodels;
	}
	strcpyn( g_bodypart[g_numbodyparts].name, token );

	do
	{
		GetToken (true);
		if (endofscript)
			return;
		else if (token[0] == '{')
			is_started = 1;
		else if (token[0] == '}')
			break;
		else if (stricmp("studio", token ) == 0)
		{
			g_model[g_nummodels] = (s_model_t *)kalloc( 1, sizeof( s_model_t ) );
			g_bodypart[g_numbodyparts].pmodel[g_bodypart[g_numbodyparts].nummodels] = g_model[g_nummodels];
			g_bodypart[g_numbodyparts].nummodels++;
		
			Option_Studio( g_model[g_nummodels] );
			g_nummodels++;
		}
		else if (stricmp("blank", token ) == 0)
			Option_Blank( );
		else
		{
			MdlError("unknown bodygroup option: \"%s\"\n", token );
		}
	} while (1);

	g_numbodyparts++;
	return;
}


void Cmd_Body( )
{
	int is_started = 0;

	if (!GetToken(false)) return;

	if (g_numbodyparts == 0) {
		g_bodypart[g_numbodyparts].base = 1;
	}
	else {
		g_bodypart[g_numbodyparts].base = g_bodypart[g_numbodyparts-1].base * g_bodypart[g_numbodyparts-1].nummodels;
	}
	strcpyn(g_bodypart[g_numbodyparts].name, token );

	g_model[g_nummodels] = (s_model_t *)kalloc( 1, sizeof( s_model_t ) );
	g_bodypart[g_numbodyparts].pmodel[g_bodypart[g_numbodyparts].nummodels] = g_model[g_nummodels];
	g_bodypart[g_numbodyparts].nummodels = 1;

	Option_Studio( g_model[g_nummodels] );

	g_nummodels++;
	g_numbodyparts++;
}



/*
===============
===============
*/

void Grab_Animation( s_source_t *psource )
{
	Vector pos;
	RadianEuler rot;
	char cmd[1024];
	int index;
	int	t = -99999999;
	int size;

	psource->startframe = -1;

	size = psource->numbones * sizeof( s_bone_t );

	while (fgets( g_szLine, sizeof( g_szLine ), g_fpInput ) != NULL) 
	{
		g_iLinecount++;
		if (sscanf( g_szLine, "%d %f %f %f %f %f %f", &index, &pos[0], &pos[1], &pos[2], &rot[0], &rot[1], &rot[2] ) == 7)
		{
			if (psource->startframe < 0)
			{
				MdlError( "Missing frame start(%d) : %s", g_iLinecount, g_szLine );
			}

			scale_vertex( pos );
			VectorCopy( pos, psource->rawanim[t][index].pos );
			VectorCopy( rot, psource->rawanim[t][index].rot );

			clip_rotations( rot ); // !!!
		}
		else if (sscanf( g_szLine, "%1023s %d", cmd, &index ))
		{
			if (stricmp( cmd, "time" ) == 0) 
			{
				t = index;
				if (psource->startframe == -1)
				{
					psource->startframe = t;
				}
				if (t < psource->startframe)
				{
					MdlError( "Frame MdlError(%d) : %s", g_iLinecount, g_szLine );
				}
				if (t > psource->endframe)
				{
					psource->endframe = t;
				}
				t -= psource->startframe;

				if (psource->rawanim[t] == NULL)
				{
					psource->rawanim[t] = (s_bone_t *)kalloc( 1, size );

					// duplicate previous frames keys
					if (t > 0 && psource->rawanim[t-1])
					{
						for (int j = 0; j < psource->numbones; j++)
						{
							VectorCopy( psource->rawanim[t-1][j].pos, psource->rawanim[t][j].pos );
							VectorCopy( psource->rawanim[t-1][j].rot, psource->rawanim[t][j].rot );
						}
					}
				}
				else
				{
					// MdlError( "%s has duplicated frame %d\n", psource->filename, t );
				}
			}
			else if (stricmp( cmd, "end") == 0) 
			{
				psource->numframes = psource->endframe - psource->startframe + 1;

				for (t = 0; t < psource->numframes; t++)
				{
					if (psource->rawanim[t] == NULL)
					{
						MdlError( "%s is missing frame %d\n", psource->filename, t + psource->startframe );
					}
				}

				Build_Reference( psource );
				return;
			}
			else
			{
				MdlError( "MdlError(%d) : %s", g_iLinecount, g_szLine );
			}
		}
		else
		{
			MdlError( "MdlError(%d) : %s", g_iLinecount, g_szLine );
		}
	}

	MdlError( "unexpected EOF: %s\n", psource->filename );
}





int Option_Activity( s_sequence_t *psequence )
{
	qboolean found;

	found = false;

	GetToken(false);
	strcpy( psequence->activityname, token );

	GetToken(false);
	psequence->actweight = verify_atoi(token);

	return 0;
}



/*
===============
===============
*/


int Option_Event ( s_sequence_t *psequence )
{
	if (psequence->numevents + 1 >= MAXSTUDIOEVENTS)
	{
		TokenError("too many events\n");
	}

	GetToken (false);
	
	strcpy( psequence->event[psequence->numevents].eventname, token );

	GetToken( false );
	psequence->event[psequence->numevents].frame = verify_atoi( token );

	psequence->numevents++;

	// option token
	if (TokenAvailable())
	{
		GetToken( false );
		if (token[0] == '}') // opps, hit the end
			return 1;
		// found an option
		strcpyn( psequence->event[psequence->numevents-1].options, token );
	}

	return 0;
}



void Option_IKRule( s_ikrule_t *pRule )
{
	// chain
	GetToken( false );

	int i;
	for ( i = 0; i < g_numikchains; i++)
	{
		if (stricmp( token, g_ikchain[i].name ) == 0)
		{
			break;
		}
	}
	if (i >= g_numikchains)
	{
		TokenError( "unknown chain \"%s\" in ikrule\n", token );
	}
	pRule->chain = i;
	// default slot
	pRule->slot = i;

	// type
	GetToken( false );
	if (stricmp( token, "touch" ) == 0)
	{
		pRule->type = IK_SELF;

		// bone
		GetToken( false );
		strcpyn( pRule->bonename, token );
	}
	else if (stricmp( token, "footstep" ) == 0)
	{
		pRule->type = IK_GROUND;

		pRule->height = g_ikchain[pRule->chain].height;
		pRule->floor = g_ikchain[pRule->chain].floor;
		pRule->radius = g_ikchain[pRule->chain].radius;
	}
	else if (stricmp( token, "attachment" ) == 0)
	{
		pRule->type = IK_ATTACHMENT;

		// name of attachment
		GetToken( false );
		strcpyn( pRule->attachment, token );
	}
	else if (stricmp( token, "release" ) == 0)
	{
		pRule->type = IK_RELEASE;
	}
	else if (stricmp( token, "unlatch" ) == 0)
	{
		pRule->type = IK_UNLATCH;
	}

	pRule->contact = -1;

	while (TokenAvailable())
	{
		GetToken( false );
		if (stricmp( token, "height" ) == 0)
		{
			GetToken( false );
			pRule->height = verify_atof( token );
		}
		else if (stricmp( token, "target" ) == 0)
		{
			// slot
			GetToken( false );
			pRule->slot = verify_atoi( token );
		}
		else if (stricmp( token, "range" ) == 0)
		{
			// ramp
			GetToken( false );
			if (token[0] == '.')
				pRule->start = -1;
			else
				pRule->start = verify_atoi( token );

			GetToken( false );
			if (token[0] == '.')
				pRule->peak = -1;
			else
				pRule->peak = verify_atoi( token );
	
			GetToken( false );
			if (token[0] == '.')
				pRule->tail = -1;
			else
				pRule->tail = verify_atoi( token );

			GetToken( false );
			if (token[0] == '.')
				pRule->end = -1;
			else
				pRule->end = verify_atoi( token );
		}
		else if (stricmp( token, "floor" ) == 0)
		{
			GetToken( false );
			pRule->floor = verify_atof( token );
		}
		else if (stricmp( token, "pad" ) == 0)
		{
			GetToken( false );
			pRule->radius = verify_atof( token ) / 2.0f;
		}
		else if (stricmp( token, "radius" ) == 0)
		{
			GetToken( false );
			pRule->radius = verify_atof( token );
		}
		else if (stricmp( token, "contact" ) == 0)
		{
			GetToken( false );
			pRule->contact = verify_atoi( token );
		}
		else if (stricmp( token, "usesequence" ) == 0)
		{
			pRule->usesequence = true;
			pRule->usesource = false;
		}
		else if (stricmp( token, "usesource" ) == 0)
		{
			pRule->usesequence = false;
			pRule->usesource = true;
		}
		else if (stricmp( token, "fakeorigin" ) == 0)
		{
			GetToken( false );
			pRule->pos.x = verify_atof( token );
			GetToken( false );
			pRule->pos.y = verify_atof( token );
			GetToken( false );
			pRule->pos.z = verify_atof( token );

			pRule->bone = -1;
		}
		else if (stricmp( token, "fakerotate" ) == 0)
		{
			QAngle ang;

			GetToken( false );
			ang.x = verify_atof( token );
			GetToken( false );
			ang.y = verify_atof( token );
			GetToken( false );
			ang.z = verify_atof( token );

			AngleQuaternion( ang, pRule->q );

			pRule->bone = -1;
		}
		else if (stricmp( token, "bone" ) == 0)
		{
			strcpy( pRule->bonename, token );
		}
		else
		{
			UnGetToken();
			return;
		}
	}
}


/*
=================
Cmd_Origin
=================
*/
void Cmd_Origin (void)
{
	GetToken (false);
	g_defaultadjust.x = verify_atof (token);

	GetToken (false);
	g_defaultadjust.y = verify_atof (token);

	GetToken (false);
	g_defaultadjust.z = verify_atof (token);

	if (TokenAvailable()) {
		GetToken (false);
		g_defaultrotation.z = DEG2RAD( verify_atof( token ) + 90);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Set the default root rotation so that the Y axis is up instead of the Z axis (for Maya)
//-----------------------------------------------------------------------------
void Cmd_UpAxis( void )
{
	// We want to create a rotation that rotates from the art space
	// (specified by the up direction) to a z up space
	// Note: x, -x, -y are untested
	GetToken (false);
	if (!stricmp( token, "x" ))
	{
		// rotate 90 degrees around y to move x into z
		g_defaultrotation.x = 0.0f;
		g_defaultrotation.y = M_PI / 2.0f;
	}
	else if (!stricmp( token, "-x" ))
	{
		// untested
		g_defaultrotation.x = 0.0f;
		g_defaultrotation.y = -M_PI / 2.0f;
	}
	else if (!stricmp( token, "y" ))
	{
		// rotate 90 degrees around x to move y into z
		g_defaultrotation.x = M_PI / 2.0f;
		g_defaultrotation.y = 0.0f;
	}
	else if (!stricmp( token, "-y" ))
	{
		// untested
		g_defaultrotation.x = -M_PI / 2.0f;
		g_defaultrotation.y = 0.0f;
	}
	else if (!stricmp( token, "z" ))
	{
		// there's still a built in 90 degree Z rotation :(
		g_defaultrotation.x = 0.0f;
		g_defaultrotation.y = 0.0f;
	}
	else if (!stricmp( token, "-z" ))
	{
		// there's still a built in 90 degree Z rotation :(
		g_defaultrotation.x = 0.0f;
		g_defaultrotation.y = 0.0f;
	}
	else
	{
		TokenError( "unknown $upaxis option: \"%s\"\n", token );
	}
}


/*
=================
=================
*/
void Cmd_ScaleUp (void)
{

	GetToken (false);
	g_defaultscale = verify_atof (token);

	g_currentscale = g_defaultscale;
}

//-----------------------------------------------------------------------------
// Purpose: Sets how what size chunks to cut the animations into
//-----------------------------------------------------------------------------

void Cmd_AnimBlockSize( void )
{
	GetToken( false );
	g_animblocksize = verify_atoi( token );
	if (g_animblocksize < 1024)
	{
		g_animblocksize *= 1024;
	}
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------


static void FlipFacing( s_source_t *pSrc )
{
	unsigned short tmp;

	int i, j;
	for( i = 0; i < pSrc->nummeshes; i++ )
	{
		s_mesh_t *pMesh = &pSrc->mesh[i];
		for( j = 0; j < pMesh->numfaces; j++ )
		{
			s_face_t &f = pSrc->face[pMesh->faceoffset + j];
			tmp = f.b;  f.b  = f.c;  f.c  = tmp;
		}
	}
}


//-----------------------------------------------------------------------------
// Checks to see if the model source was already loaded
//-----------------------------------------------------------------------------
static s_source_t *FindCachedSource( char const* name, char const* xext )
{
	int i;

	if( xext[0] )
	{
		// we know what extension is necessary. . look for it.
		sprintf (g_szFilename, "%s%s.%s", cddir[numdirs], name, xext );
		for (i = 0; i < g_numsources; i++)
		{
			if (stricmp( g_szFilename, g_source[i]->filename ) == 0)
				return g_source[i];
		}
	}
	else
	{
		// we don't know what extension to use, so look for all of 'em.
		sprintf (g_szFilename, "%s%s.vrm", cddir[numdirs], name );
		for (i = 0; i < g_numsources; i++)
		{
			if (stricmp( g_szFilename, g_source[i]->filename ) == 0)
				return g_source[i];
		}
		sprintf (g_szFilename, "%s%s.smd", cddir[numdirs], name );
		for (i = 0; i < g_numsources; i++)
		{
			if (stricmp( g_szFilename, g_source[i]->filename ) == 0)
				return g_source[i];
		}
		/*
		sprintf (g_szFilename, "%s%s.vta", cddir[numdirs], name );
		for (i = 0; i < g_numsources; i++)
		{
			if (stricmp( g_szFilename, g_source[i]->filename ) == 0)
				return g_source[i];
		}
		*/
	}

	// Not found
	return 0;
}


//-----------------------------------------------------------------------------
// Loads an animation/model source
//-----------------------------------------------------------------------------

s_source_t *Load_Source( char const *name, const char *ext, bool reverse, bool isActiveModel )
{
	if ( g_numsources >= MAXSTUDIOSEQUENCES )
		TokenError( "Load_Source( %s ) - overflowed g_numsources.", name );

	Assert(name);
	int namelen = strlen(name) + 1;
	char* pTempName = (char*)_alloca( namelen );
	char xext[32];
	int result = false;

	strcpy( pTempName, name );
	Q_ExtractFileExtension( pTempName, xext, sizeof( xext ) );

	if (xext[0] == '\0')
	{
		strcpyn( xext, ext );
	}
	else
	{
		Q_StripExtension( pTempName, pTempName, namelen );
	}

	s_source_t* pSource = FindCachedSource( pTempName, xext );
	if (pSource)
	{
		if (isActiveModel)
			pSource->isActiveModel = true;
		return pSource;
	}

	g_source[g_numsources] = (s_source_t *)kalloc( 1, sizeof( s_source_t ) );
	strcpyn( g_source[g_numsources]->filename, g_szFilename );


	if (isActiveModel)
	{
		g_source[g_numsources]->isActiveModel = true;
	}

	if ( xext[0] == '\0' || stricmp( xext, "vrm" ) == 0)
	{
		sprintf (g_szFilename, "%s%s.vrm", cddir[numdirs], pTempName );
		strcpyn( g_source[g_numsources]->filename, g_szFilename );
		result = Load_VRM( g_source[g_numsources] );
	}
	if ( ( !result && xext[0] == '\0' ) || stricmp( xext, "smd" ) == 0)
	{
		sprintf (g_szFilename, "%s%s.smd", cddir[numdirs], pTempName );
		strcpyn( g_source[g_numsources]->filename, g_szFilename );
		result = Load_SMD( g_source[g_numsources] );
	}
	if ( ( !result && xext[0] == '\0' ) || stricmp( xext, "sma" ) == 0)
	{
		sprintf (g_szFilename, "%s%s.sma", cddir[numdirs], pTempName );
		strcpyn( g_source[g_numsources]->filename, g_szFilename );
		result = Load_SMD( g_source[g_numsources] );
	}
	if ( ( !result && xext[0] == '\0' ) || stricmp( xext, "phys" ) == 0)
	{
		sprintf (g_szFilename, "%s%s.phys", cddir[numdirs], pTempName );
		strcpyn( g_source[g_numsources]->filename, g_szFilename );
		result = Load_SMD( g_source[g_numsources] );
	}
	if (( !result && xext[0] == '\0' ) || stricmp( xext, "vta" ) == 0)
	{
		sprintf (g_szFilename, "%s%s.vta", cddir[numdirs], pTempName );
		strcpyn( g_source[g_numsources]->filename, g_szFilename );
		result = Load_VTA( g_source[g_numsources] );
	}
	if (( !result && xext[0] == '\0' ) || stricmp( xext, "obj" ) == 0)
	{
		sprintf (g_szFilename, "%s%s.obj", cddir[numdirs], pTempName );
		strcpyn( g_source[g_numsources]->filename, g_szFilename );
		result = Load_OBJ( g_source[g_numsources] );
	}

	if (!g_bCreateMakefile && !result)
	{
		TokenError( "could not load file '%s'\n", g_source[g_numsources]->filename );
	}

	if ( g_source[g_numsources]->numbones == 0 )
	{
		TokenError( "missing all bones in file '%s'\n", g_source[g_numsources]->filename );
	}

	g_numsources++;
	if( reverse )
	{
		FlipFacing( g_source[g_numsources-1] );
	}

	return g_source[g_numsources-1];
}


s_sequence_t *LookupSequence( char *name )
{
	int i;
	for (i = 0; i < g_sequence.Count(); i++)
	{
		if (stricmp( g_sequence[i].name, token ) == 0)
		{
			return &g_sequence[i];
		}
	}
	return NULL;
}


s_animation_t *LookupAnimation( char *name )
{
	int i;
	for ( i = 0; i < g_numani; i++)
	{
		if (stricmp( g_panimation[i]->name, token ) == 0)
		{
			return g_panimation[i];
		}
	}

	s_sequence_t *pseq = LookupSequence( name );
	if (pseq)
	{
		return pseq->panim[0][0];
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: parse order dependant s_animcmd_t token for $animations
//-----------------------------------------------------------------------------

int ParseCmdlistToken( int &numcmds, s_animcmd_t *cmds )
{
	if (numcmds >= MAXSTUDIOCMDS)
	{
		return false;
	}
	s_animcmd_t *pcmd = &cmds[numcmds];
	if (stricmp("fixuploop", token ) == 0)
	{
		pcmd->cmd = CMD_FIXUP;

		GetToken( false );
		pcmd->u.fixuploop.start = verify_atoi( token );
		GetToken( false );
		pcmd->u.fixuploop.end = verify_atoi( token );
	}
	else if (strnicmp("weightlist", token, 6 ) == 0)
	{
		GetToken( false );

		int i;
		for ( i = 1; i < g_numweightlist; i++)
		{
			if (stricmp( g_weightlist[i].name, token ) == 0)
			{
				break;
			}
		}
		if (i == g_numweightlist)
		{
			TokenError( "unknown weightlist '%s\'\n", token );
		}
		pcmd->cmd = CMD_WEIGHTS;
		pcmd->u.weightlist.index = i;
	}
	else if (stricmp("subtract", token ) == 0)
	{
		pcmd->cmd = CMD_SUBTRACT;

		GetToken( false );

		s_animation_t *extanim = LookupAnimation( token );
		if (extanim == NULL)
		{
			TokenError( "unknown subtract animation '%s\'\n", token );
		}

		pcmd->u.subtract.ref = extanim;

		GetToken( false );
		pcmd->u.subtract.frame = verify_atoi( token );

		pcmd->u.subtract.flags |= STUDIO_POST;
	}
	else if (stricmp("presubtract", token ) == 0) // FIXME: rename this to something better
	{
		pcmd->cmd = CMD_SUBTRACT;

		GetToken( false );

		s_animation_t *extanim = LookupAnimation( token );
		if (extanim == NULL)
		{
			TokenError( "unknown presubtract animation '%s\'\n", token );
		}

		pcmd->u.subtract.ref = extanim;

		GetToken( false );
		pcmd->u.subtract.frame = verify_atoi( token );
	}
	else if (stricmp( "alignto", token ) == 0)
	{
		pcmd->cmd = CMD_AO;

		pcmd->u.ao.pBonename = NULL;

		GetToken( false );
		s_animation_t *extanim = LookupAnimation( token );
		if (extanim == NULL)
		{
			TokenError( "unknown alignto animation '%s\'\n", token );
		}

		pcmd->u.ao.ref = extanim;
		pcmd->u.ao.motiontype = STUDIO_X | STUDIO_Y;
		pcmd->u.ao.srcframe = 0;
		pcmd->u.ao.destframe = 0;
	}
	else if (stricmp( "align", token ) == 0)
	{
		pcmd->cmd = CMD_AO;

		pcmd->u.ao.pBonename = NULL;

		GetToken( false );
		s_animation_t *extanim = LookupAnimation( token );
		if (extanim == NULL)
		{
			TokenError( "unknown align animation '%s\'\n", token );
		}

		pcmd->u.ao.ref = extanim;

		// motion type to match
		pcmd->u.ao.motiontype = 0;
		GetToken( false );
		int ctrl;
		while ((ctrl = lookupControl( token )) != -1)
		{
			pcmd->u.ao.motiontype |= ctrl;
			GetToken( false );
		}
		if (pcmd->u.ao.motiontype == 0)
		{
			TokenError( "missing controls on align\n" );
		}

		// frame of reference animation to match
		pcmd->u.ao.srcframe = verify_atoi( token );

		// against what frame of the current animation
		GetToken( false );
		pcmd->u.ao.destframe = verify_atoi( token );
	}
	else if (stricmp( "alignboneto", token ) == 0)
	{
		pcmd->cmd = CMD_AO;

		GetToken( false );
		pcmd->u.ao.pBonename = strdup( token );
		
		GetToken( false );
		s_animation_t *extanim = LookupAnimation( token );
		if (extanim == NULL)
		{
			TokenError( "unknown alignboneto animation '%s\'\n", token );
		}

		pcmd->u.ao.ref = extanim;
		pcmd->u.ao.motiontype = STUDIO_X | STUDIO_Y;
		pcmd->u.ao.srcframe = 0;
		pcmd->u.ao.destframe = 0;
	}
	else if (stricmp( "match", token ) == 0)
	{
		pcmd->cmd = CMD_MATCH;

		GetToken( false );

		s_animation_t *extanim = LookupAnimation( token );
		if (extanim == NULL)
		{
			TokenError( "unknown match animation '%s\'\n", token );
		}

		pcmd->u.match.ref = extanim;
	}
	else if (stricmp( "matchblend", token ) == 0)
	{
		pcmd->cmd = CMD_MATCHBLEND;

		GetToken( false );

		s_animation_t *extanim = LookupAnimation( token );
		if (extanim == NULL)
		{
			MdlError( "unknown match animation '%s\'\n", token );
		}

		pcmd->u.match.ref = extanim;

		// frame of reference animation to match
		GetToken( false );
		pcmd->u.match.srcframe = verify_atoi( token );

		// against what frame of the current animation
		GetToken( false );
		pcmd->u.match.destframe = verify_atoi( token );

		// backup and starting match in here
		GetToken( false );
		pcmd->u.match.destpre = verify_atoi( token );

		// continue blending match till here
		GetToken( false );
		pcmd->u.match.destpost = verify_atoi( token );

	}
	else if (stricmp( "worldspaceblend", token ) == 0)
	{
		pcmd->cmd = CMD_WORLDSPACEBLEND;

		GetToken( false );

		s_animation_t *extanim = LookupAnimation( token );
		if (extanim == NULL)
		{
			TokenError( "unknown worldspaceblend animation '%s\'\n", token );
		}

		pcmd->u.world.ref = extanim;
		pcmd->u.world.startframe = 0;
		pcmd->u.world.loops = false;
	}
	else if (stricmp( "worldspaceblendloop", token ) == 0)
	{
		pcmd->cmd = CMD_WORLDSPACEBLEND;

		GetToken( false );

		s_animation_t *extanim = LookupAnimation( token );
		if (extanim == NULL)
		{
			TokenError( "unknown worldspaceblend animation '%s\'\n", token );
		}

		pcmd->u.world.ref = extanim;

		GetToken( false );
		pcmd->u.world.startframe = atoi( token );

		pcmd->u.world.loops = true;
	}
	else if (stricmp( "rotateto", token ) == 0)
	{
		pcmd->cmd = CMD_ANGLE;

		GetToken( false );
		pcmd->u.angle.angle = verify_atof( token );
	}
	else if (stricmp( "ikrule", token ) == 0)
	{
		pcmd->cmd = CMD_IKRULE;

		pcmd->u.ikrule.pRule = (s_ikrule_t *)kalloc( 1, sizeof( s_ikrule_t ) );

		Option_IKRule( pcmd->u.ikrule.pRule );
	}
	else if (stricmp( "ikfixup", token ) == 0)
	{
		pcmd->cmd = CMD_IKFIXUP;

		pcmd->u.ikfixup.pRule = (s_ikrule_t *)kalloc( 1, sizeof( s_ikrule_t ) );

		Option_IKRule( pcmd->u.ikrule.pRule );
	}
	else if (stricmp( "walkframe", token ) == 0)
	{
		pcmd->cmd = CMD_MOTION;

		// frame
		GetToken( false );
		pcmd->u.motion.iEndFrame = verify_atoi( token );

		// motion type to match
		pcmd->u.motion.motiontype = 0;
		while (TokenAvailable())
		{
			GetToken( false );
			int ctrl = lookupControl( token );
			if (ctrl != -1)
			{
				pcmd->u.motion.motiontype |= ctrl;
			}
			else
			{
				UnGetToken();
				break;
			}
		}

		/*
		GetToken( false ); // X
		pcmd->u.motion.x = verify_atof( token );

		GetToken( false ); // Y
		pcmd->u.motion.y = verify_atof( token );

		GetToken( false ); // A
		pcmd->u.motion.zr = verify_atof( token );
		*/
	}
	else if (stricmp( "walkalignto", token ) == 0)
	{
		pcmd->cmd = CMD_REFMOTION;

		GetToken( false );
		pcmd->u.motion.iEndFrame = verify_atoi( token );

		pcmd->u.motion.iSrcFrame = pcmd->u.motion.iEndFrame;

		GetToken( false ); // reference animation
		s_animation_t *extanim = LookupAnimation( token );
		if (extanim == NULL)
		{
			TokenError( "unknown alignto animation '%s\'\n", token );
		}
		pcmd->u.motion.pRefAnim = extanim;

		pcmd->u.motion.iRefFrame = 0;

		// motion type to match
		pcmd->u.motion.motiontype = 0;
		while (TokenAvailable())
		{
			GetToken( false );
			int ctrl = lookupControl( token );
			if (ctrl != -1)
			{
				pcmd->u.motion.motiontype |= ctrl;
			}
			else
			{
				UnGetToken();
				break;
			}
		}


		/*
		GetToken( false ); // X
		pcmd->u.motion.x = verify_atof( token );

		GetToken( false ); // Y
		pcmd->u.motion.y = verify_atof( token );

		GetToken( false ); // A
		pcmd->u.motion.zr = verify_atof( token );
		*/
	}
	else if (stricmp( "walkalign", token ) == 0)
	{
		pcmd->cmd = CMD_REFMOTION;

		// end frame to apply motion over
		GetToken( false );
		pcmd->u.motion.iEndFrame = verify_atoi( token );

		// reference animation
		GetToken( false );
		s_animation_t *extanim = LookupAnimation( token );
		if (extanim == NULL)
		{
			TokenError( "unknown alignto animation '%s\'\n", token );
		}
		pcmd->u.motion.pRefAnim = extanim;

		// motion type to match
		pcmd->u.motion.motiontype = 0;
		while (TokenAvailable())
		{
			GetToken( false );
			int ctrl = lookupControl( token );
			if (ctrl != -1)
			{
				pcmd->u.motion.motiontype |= ctrl;
			}
			else
			{
				break;
			}
		}
		if (pcmd->u.motion.motiontype == 0)
		{
			TokenError( "missing controls on walkalign\n" );
		}

		// frame of reference animation to match
		pcmd->u.motion.iRefFrame = verify_atoi( token );

		// against what frame of the current animation
		GetToken( false );
		pcmd->u.motion.iSrcFrame = verify_atoi( token );
	}
	else if (stricmp("derivative", token ) == 0)
	{
		pcmd->cmd = CMD_DERIVATIVE;

		// get scale
		GetToken( false );
		pcmd->u.derivative.scale = verify_atof( token );
	}
	else if (stricmp("noanimation", token ) == 0)
	{
		pcmd->cmd = CMD_NOANIMATION;
	}
	else if (stricmp("lineardelta", token ) == 0)
	{
		pcmd->cmd = CMD_LINEARDELTA;
		pcmd->u.linear.flags |= STUDIO_AL_POST;
	}
	else if (stricmp("splinedelta", token ) == 0)
	{
		pcmd->cmd = CMD_LINEARDELTA;
		pcmd->u.linear.flags |= STUDIO_AL_POST;
		pcmd->u.linear.flags |= STUDIO_AL_SPLINE;
	}
	else if (stricmp("compress", token ) == 0)
	{
		pcmd->cmd = CMD_COMPRESS;

		// get frames to skip
		GetToken( false );
		pcmd->u.compress.frames = verify_atoi( token );
	}
	else if (stricmp("numframes", token ) == 0)
	{
		pcmd->cmd = CMD_NUMFRAMES;

		// get frames to force
		GetToken( false );
		pcmd->u.compress.frames = verify_atoi( token );
	}
	else if (stricmp("counterrotate", token ) == 0)
	{
		pcmd->cmd = CMD_COUNTERROTATE;

		// get bone name
		GetToken( false );
		pcmd->u.counterrotate.pBonename = strdup( token );
	}
	else if (stricmp("counterrotateto", token ) == 0)
	{
		pcmd->cmd = CMD_COUNTERROTATE;

		pcmd->u.counterrotate.bHasTarget = true;

		// get pitch
		GetToken( false );
		pcmd->u.counterrotate.targetAngle[0] = verify_atof( token );

		// get yaw
		GetToken( false );
		pcmd->u.counterrotate.targetAngle[1] = verify_atof( token );

		// get roll
		GetToken( false );
		pcmd->u.counterrotate.targetAngle[2] = verify_atof( token );

		// get bone name
		GetToken( false );
		pcmd->u.counterrotate.pBonename = strdup( token );


	}
	else
	{
		return false;
	}
	numcmds++;
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: parse order independant s_animation_t token for $animations
//-----------------------------------------------------------------------------

int ParseAnimationToken( s_animation_t *panim )
{
	if (stricmp("if", token ) == 0)
	{
		// fixme: add expression evaluation
		GetToken( false );
		if (atoi( token ) == 0 && stricmp( token, "true" ) != 0)
		{
			GetToken(true);
			if (token[0] == '{')
			{
				int depth = 1;
				while (TokenAvailable() && depth > 0)
				{
					GetToken( true );
					if (stricmp("{", token ) == 0)
					{
						depth++;
					}
					else if (stricmp("}", token ) == 0)
					{
						depth--;
					}
				}
			}
		}
	}
	else if (stricmp("fps", token ) == 0)
	{
		GetToken( false );
		panim->fps = verify_atof( token );
		if ( panim->fps <= 0.0f )
		{
			TokenError( "ParseAnimationToken:  fps (%f from '%s') <= 0.0\n", panim->fps, token );
		}
	}
	else if (stricmp("origin", token ) == 0)
	{
		GetToken (false);
		panim->adjust.x = verify_atof (token);

		GetToken (false);
		panim->adjust.y = verify_atof (token);

		GetToken (false);
		panim->adjust.z = verify_atof (token);
	}
	else if (stricmp("rotate", token ) == 0)
	{
		GetToken( false );
		// FIXME: broken for Maya
		panim->rotation.z = DEG2RAD( verify_atof( token ) + 90 );
	}
	else if (stricmp("angles", token ) == 0)
	{
		GetToken( false );
		panim->rotation.x = DEG2RAD( verify_atof( token ) );
		GetToken( false );
		panim->rotation.y = DEG2RAD( verify_atof( token ) );
		GetToken( false );
		panim->rotation.z = DEG2RAD( verify_atof( token ) + 90.0);
	}
	else if (stricmp("scale", token ) == 0)
	{
		GetToken( false );
		panim->scale = verify_atof( token );
	}
	else if (strnicmp("loop", token, 4 ) == 0)
	{
		panim->flags |= STUDIO_LOOPING;
	}
	else if (strnicmp("startloop", token, 5 ) == 0)
	{
		GetToken( false );
		panim->looprestart = verify_atoi( token );
		panim->flags |= STUDIO_LOOPING;
	}
	else if (stricmp("fudgeloop", token ) == 0)
	{
		panim->fudgeloop = true;
		panim->flags |= STUDIO_LOOPING;
	}
	else if (strnicmp("snap", token, 4 ) == 0)
	{
		panim->flags |= STUDIO_SNAP;
	}
	else if (strnicmp("frame", token, 5 ) == 0)
	{
		GetToken( false );
		panim->startframe = verify_atoi( token );
		GetToken( false );
		panim->endframe = verify_atoi( token );

		if (panim->startframe < panim->source->startframe)
			panim->startframe = panim->source->startframe;

		if (panim->endframe > panim->source->endframe)
			panim->endframe = panim->source->endframe;

		if (!g_bCreateMakefile && panim->endframe < panim->startframe)
			TokenError( "end frame before start frame in %s", panim->name );

		panim->numframes = panim->endframe - panim->startframe + 1;
	}
	else if (stricmp("post", token) == 0)
	{
		panim->flags |= STUDIO_POST;
	}
	else if (stricmp("noautoik", token) == 0)
	{
		panim->noAutoIK = true;
	}
	else if (stricmp("autoik", token) == 0)
	{
		panim->noAutoIK = false;
	}
	else if (ParseCmdlistToken( panim->numcmds, panim->cmds ))
	{

	}
	else if (stricmp("cmdlist", token) == 0)
	{
		GetToken( false ); // A

		int i;
		for ( i = 0; i < g_numcmdlists; i++)
		{
			if (stricmp( g_cmdlist[i].name, token) == 0)
			{
				break;
			}
		}
		if (i == g_numcmdlists)
			TokenError( "unknown cmdlist %s\n", token );

		for (int j = 0; j < g_cmdlist[i].numcmds; j++)
		{
			if (panim->numcmds >= MAXSTUDIOCMDS)
			{
				TokenError("Too many cmds in %s\n", panim->name );
			}
			panim->cmds[panim->numcmds++] = g_cmdlist[i].cmds[j];
		}
	}
	else if (lookupControl( token ) != -1)
	{
		panim->motiontype |= lookupControl( token );
	}
	else
	{
		return false;
	}
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: create named order dependant s_animcmd_t blocks, used as replicated token list for $animations
//-----------------------------------------------------------------------------

void Cmd_Cmdlist( )
{
	int depth = 0;

	// name
	GetToken(false);
	strcpyn( g_cmdlist[g_numcmdlists].name, token );

	while (1)
	{
		if (depth > 0)
		{
			if(!GetToken(true)) 
			{
				break;
			}
		}
		else
		{
			if (!TokenAvailable()) 
			{
				break;
			}
			else 
			{
				GetToken (false);
			}
		}

		if (endofscript)
		{
			if (depth != 0)
			{
				TokenError("missing }\n" );
			}
			return;
		}
		if (stricmp("{", token ) == 0)
		{
			depth++;
		}
		else if (stricmp("}", token ) == 0)
		{
			depth--;
		}
		else if (ParseCmdlistToken( g_cmdlist[g_numcmdlists].numcmds, g_cmdlist[g_numcmdlists].cmds ))
		{

		}
		else
		{
			TokenError( "unknown command: %s\n", token );
		}

		if (depth < 0)
		{
			TokenError("missing {\n");
		}
	};

	g_numcmdlists++;
}

int ParseAnimation( s_animation_t *panim, bool isAppend );
int ParseEmpty( void );

//-----------------------------------------------------------------------------
// Purpose: allocate an entry for $animation
//-----------------------------------------------------------------------------

void Cmd_Animation( )
{
	// name
	GetToken(false);

	s_animation_t *panim = LookupAnimation( token );

	if (panim != NULL)
	{
		if (!panim->isOverride)
		{
			TokenError( "Duplicate animation name \"%s\"\n", token );
		}
		else
		{
			panim->doesOverride = true;
			ParseEmpty();
			return;
		}
	}

	// allocate animation entry
	g_panimation[g_numani] = (s_animation_t *)kalloc( 1, sizeof( s_animation_t ) );
	g_panimation[g_numani]->index = g_numani;
	panim = g_panimation[g_numani];
	strcpyn( panim->name, token );
	g_numani++;

	// filename
	GetToken(false);
	strcpyn( panim->filename, token );

	// panim->animgroup = g_currentanimgroup;

	//panim->source = Load_Source( panim->filename, "smd" );
	panim->source = Load_Source( panim->filename, "" );
	panim->startframe = panim->source->startframe;
	panim->endframe = panim->source->endframe;

	VectorCopy( g_defaultadjust, panim->adjust );
	panim->rotation = g_defaultrotation;
	panim->scale = 1.0f;
	panim->fps = 30.0;

	ParseAnimation( panim, false );

	panim->numframes = panim->endframe - panim->startframe + 1;

	//CheckAutoShareAnimationGroup( panim->name );
}

//-----------------------------------------------------------------------------
// Purpose: wrapper for parsing $animation tokens
//-----------------------------------------------------------------------------

int ParseAnimation( s_animation_t *panim, bool isAppend )
{
	int depth = 0;

	while (1)
	{
		if (depth > 0)
		{
			if(!GetToken(true)) 
			{
				break;
			}
		}
		else
		{
			if (!TokenAvailable()) 
			{
				break;
			}
			else 
			{
				GetToken (false);
			}
		}

		if (endofscript)
		{
			if (depth != 0)
			{
				TokenError("missing }\n" );
			}
			return 1;
		}
		if (stricmp("{", token ) == 0)
		{
			depth++;
		}
		else if (stricmp("}", token ) == 0)
		{
			depth--;
		}
		else if (ParseAnimationToken( panim ))
		{

		}
		else
		{
			TokenError( "Unknown animation option\'%s\'\n", token );
		}

		if (depth < 0)
		{
			TokenError("missing {\n");
		}
	};

	return 0;
}


//-----------------------------------------------------------------------------
// Purpose: create a virtual $animation command from a $sequence reference
//-----------------------------------------------------------------------------

s_animation_t *Cmd_ImpliedAnimation( s_sequence_t *psequence, char *filename )
{
	// allocate animation entry
	g_panimation[g_numani] = (s_animation_t *)kalloc( 1, sizeof( s_animation_t ) );
	g_panimation[g_numani]->index = g_numani;
	s_animation_t *panim = g_panimation[g_numani];
	g_numani++;

	panim->isimplied = true;

	// panim->animgroup = g_currentanimgroup;

	panim->startframe = 0;
	panim->endframe = MAXSTUDIOANIMFRAMES - 1;

	strcpy( panim->name, "@" );
	strcat( panim->name, psequence->name );
	strcpyn( panim->filename, filename );

	VectorCopy( g_defaultadjust, panim->adjust );
	panim->scale = 1.0f;
	panim->rotation = g_defaultrotation;
	panim->fps = 30;

	//panim->source = Load_Source( panim->filename, "smd" );
	panim->source = Load_Source( panim->filename, "" );

	if (panim->startframe < panim->source->startframe)
		panim->startframe = panim->source->startframe;
	
	if (panim->endframe > panim->source->endframe)
		panim->endframe = panim->source->endframe;

	if (!g_bCreateMakefile && panim->endframe < panim->startframe)
		TokenError( "end frame before start frame in %s", panim->name );

	panim->numframes = panim->endframe - panim->startframe + 1;

	//CheckAutoShareAnimationGroup( panim->name );

	return panim;
}


//-----------------------------------------------------------------------------
// Purpose: copy globally reavent $animation options from one $animation to another
//-----------------------------------------------------------------------------

void CopyAnimationSettings( s_animation_t *pdest, s_animation_t *psrc )
{
	pdest->fps = psrc->fps;

	VectorCopy( psrc->adjust, pdest->adjust );
	pdest->scale = psrc->scale;
	pdest->rotation = psrc->rotation;

	pdest->motiontype = psrc->motiontype;

	//Adrian - Hey! Revisit me later.
	/*if (pdest->startframe < psrc->startframe)
		pdest->startframe = psrc->startframe;
	
	if (pdest->endframe > psrc->endframe)
		pdest->endframe = psrc->endframe;
	
	if (pdest->endframe < pdest->startframe)
		TokenError( "fixedup end frame before start frame in %s", pdest->name );

	pdest->numframes = pdest->endframe - pdest->startframe + 1;*/

	for (int i = 0; i < psrc->numcmds; i++)
	{
		if (pdest->numcmds >= MAXSTUDIOCMDS)
		{
			TokenError("Too many cmds in %s\n", pdest->name );
		}
		pdest->cmds[pdest->numcmds++] = psrc->cmds[i];
	}
}

int ParseSequence( s_sequence_t *pseq, bool isAppend );


//-----------------------------------------------------------------------------
// Purpose: allocate an entry for $sequence
//-----------------------------------------------------------------------------

void Cmd_Sequence( )
{
	int depth = 0;
	int numblends = 0;

	if (!GetToken(false)) 
		return;

	s_animation_t *panim = LookupAnimation( token );

	// allocate sequence
	if (panim != NULL)
	{
		if (!panim->isOverride)
		{
			TokenError( "Duplicate sequence name \"%s\"\n", token );
		}
		else
		{
			panim->doesOverride = true;
			ParseEmpty( );
			return;
		}
	}

	if (g_sequence.Count() >= MAXSTUDIOSEQUENCES)
	{
		TokenError("Too many sequences (%d max)\n", MAXSTUDIOSEQUENCES );
	}

	s_sequence_t *pseq = &g_sequence[ g_sequence.AddToTail() ];
	memset( pseq, 0, sizeof( s_sequence_t ) );

	// initialize sequence
	strcpyn( pseq->name, token );

	pseq->actweight = 0;
	pseq->activityname[0] = '\0';
	pseq->activity = -1; // -1 is the default for 'no activity'

	pseq->paramindex[0] = -1;
	pseq->paramindex[1] = -1;

	pseq->groupsize[0] = 0;
	pseq->groupsize[1] = 0;

	pseq->fadeintime = 0.2;
	pseq->fadeouttime = 0.2;

	ParseSequence( pseq, false );
}


//-----------------------------------------------------------------------------
// Purpose: parse options unique to $sequence
//-----------------------------------------------------------------------------

int ParseSequence( s_sequence_t *pseq, bool isAppend )
{
	int depth = 0;
	s_animation_t *animations[64];
	int i, j, k, n;
	int numblends = 0;

	if (isAppend)
	{
		animations[0] = pseq->panim[0][0];
	}

	while (1)
	{
		if (depth > 0)
		{
			if(!GetToken(true)) 
			{
				break;
			}
		}
		else
		{
			if (!TokenAvailable()) 
			{
				break;
			}
			else 
			{
				GetToken (false);
			}
		}

		if (endofscript)
		{
			if (depth != 0)
			{
				TokenError("missing }\n" );
			}
			return 1;
		}
		if (stricmp("{", token ) == 0)
		{
			depth++;
		}
		else if (stricmp("}", token ) == 0)
		{
			depth--;
		}
		/*
		else if (stricmp("deform", token ) == 0)
		{
			Option_Deform( pseq );
		}
		*/

		else if (stricmp("event", token ) == 0)
		{
			depth -= Option_Event( pseq );
		}
		else if (stricmp("activity", token ) == 0)
		{
			Option_Activity( pseq );
		}
		else if (strnicmp( token, "ACT_", 4 ) == 0)
		{
			UnGetToken( );
			Option_Activity( pseq );
		}

		else if (stricmp("snap", token ) == 0)
		{
			pseq->flags |= STUDIO_SNAP;
		}

		else if (stricmp("blendwidth", token ) == 0)
		{
			GetToken( false );
			pseq->groupsize[0] = verify_atoi( token );
		}

		else if (stricmp("blend", token ) == 0)
		{
			i = 0;
			if (pseq->paramindex[0] != -1)
			{
				i = 1;
			}

			GetToken( false );
			j = LookupPoseParameter( token );
			pseq->paramindex[i] = j;
			pseq->paramattachment[i] = -1;
			GetToken( false );
			pseq->paramstart[i] = verify_atof( token );
			GetToken( false );
			pseq->paramend[i] = verify_atof( token );

			g_pose[j].min  = min( g_pose[j].min, pseq->paramstart[i] );
			g_pose[j].min  = min( g_pose[j].min, pseq->paramend[i] );
			g_pose[j].max  = max( g_pose[j].max, pseq->paramstart[i] );
			g_pose[j].max  = max( g_pose[j].max, pseq->paramend[i] );
		}
		else if (stricmp("calcblend", token ) == 0)
		{
			i = 0;
			if (pseq->paramindex[0] != -1)
			{
				i = 1;
			}

			GetToken( false );
			j = LookupPoseParameter( token );
			pseq->paramindex[i] = j;

			GetToken( false );
			pseq->paramattachment[i] = LookupAttachment( token );
			if (pseq->paramattachment[i] == -1)
			{
				TokenError( "Unknown calcblend attachment \"%s\"\n", token );
			}

			GetToken( false );
			pseq->paramcontrol[i] = lookupControl( token );
		}
		else if (stricmp("blendref", token ) == 0)
		{
			GetToken( false );
			pseq->paramanim = LookupAnimation( token );
			if (pseq->paramanim == NULL)
			{
				TokenError( "Unknown blendref animation \"%s\"\n", token );
			}
		}
		else if (stricmp("blendcomp", token ) == 0)
		{
			GetToken( false );
			pseq->paramcompanim = LookupAnimation( token );
			if (pseq->paramcompanim == NULL)
			{
				TokenError( "Unknown blendcomp animation \"%s\"\n", token );
			}
		}
		else if (stricmp("blendcenter", token ) == 0)
		{
			GetToken( false );
			pseq->paramcenter = LookupAnimation( token );
			if (pseq->paramcenter == NULL)
			{
				TokenError( "Unknown blendcenter animation \"%s\"\n", token );
			}
		}
		else if (stricmp("node", token ) == 0)
		{
			GetToken( false );
			pseq->entrynode = pseq->exitnode = LookupXNode( token );
		}
		else if (stricmp("transition", token ) == 0)
		{
			GetToken( false );
			pseq->entrynode = LookupXNode( token );
			GetToken( false );
			pseq->exitnode = LookupXNode( token );
		}
		else if (stricmp("rtransition", token ) == 0)
		{
			GetToken( false );
			pseq->entrynode = LookupXNode( token );
			GetToken( false );
			pseq->exitnode = LookupXNode( token );
			pseq->nodeflags |= 1;
		}
		else if (stricmp("exitphase", token ) == 0)
		{
			GetToken( false );
			pseq->exitphase = verify_atof( token );
		}
		else if (stricmp("delta", token) == 0)
		{
			pseq->flags |= STUDIO_DELTA;
			pseq->flags |= STUDIO_POST;
		}
		else if (stricmp("worldspace", token) == 0)
		{
			pseq->flags |= STUDIO_WORLD;
			pseq->flags |= STUDIO_POST;
		}
		else if (stricmp("post", token) == 0) // remove
		{
			pseq->flags |= STUDIO_POST; 
		}
		else if (stricmp("predelta", token) == 0)
		{
			pseq->flags |= STUDIO_DELTA;
		}
		else if (stricmp("autoplay", token) == 0)
		{
			pseq->flags |= STUDIO_AUTOPLAY;
		}
		else if (stricmp( "fadein", token ) == 0)
		{
			GetToken( false );
			pseq->fadeintime = verify_atof( token );
		}
		else if (stricmp( "fadeout", token ) == 0)
		{
			GetToken( false );
			pseq->fadeouttime = verify_atof( token );
		}
		else if (stricmp( "realtime", token ) == 0)
		{
			pseq->flags |= STUDIO_REALTIME;
		}
		else if (stricmp( "hidden", token ) == 0)
		{
			pseq->flags |= STUDIO_HIDDEN;
		}
		else if (stricmp( "addlayer", token ) == 0)
		{
			GetToken( false );
			strcpyn( pseq->autolayer[pseq->numautolayers].name, token );
			pseq->numautolayers++;
		}
		else if (stricmp( "iklock", token ) == 0)
		{
			GetToken(false);
			strcpyn( pseq->iklock[pseq->numiklocks].name, token );

			GetToken(false);
			pseq->iklock[pseq->numiklocks].flPosWeight = verify_atof( token );

			GetToken(false);
			pseq->iklock[pseq->numiklocks].flLocalQWeight = verify_atof( token );
		
			pseq->numiklocks++;
		}
		else if (stricmp( "keyvalues", token ) == 0)
		{
			Option_KeyValues( &pseq->KeyValue );
		}
		else if (stricmp( "blendlayer", token ) == 0)
		{
			pseq->autolayer[pseq->numautolayers].flags = 0;

			GetToken( false );
			strcpyn( pseq->autolayer[pseq->numautolayers].name, token );

			GetToken( false );
			pseq->autolayer[pseq->numautolayers].start = verify_atoi( token );

			GetToken( false );
			pseq->autolayer[pseq->numautolayers].peak = verify_atoi( token );

			GetToken( false );
			pseq->autolayer[pseq->numautolayers].tail = verify_atoi( token );

			GetToken( false );
			pseq->autolayer[pseq->numautolayers].end = verify_atoi( token );

			while (TokenAvailable( ))
			{
				GetToken( false );
				if (stricmp( "xfade", token ) == 0)
				{
					pseq->autolayer[pseq->numautolayers].flags |= STUDIO_AL_XFADE;
				}
				else if (stricmp( "spline", token ) == 0)
				{
					pseq->autolayer[pseq->numautolayers].flags |= STUDIO_AL_SPLINE;
				}
				else if (stricmp( "noblend", token ) == 0)
				{
					pseq->autolayer[pseq->numautolayers].flags |= STUDIO_AL_NOBLEND;
				}
				else if (stricmp( "poseparameter", token ) == 0)
				{
					pseq->autolayer[pseq->numautolayers].flags |= STUDIO_AL_POSE;
					GetToken( false );
					pseq->autolayer[pseq->numautolayers].pose = LookupPoseParameter( token );
				}
				else if (stricmp( "local", token ) == 0)
				{
					pseq->autolayer[pseq->numautolayers].flags |= STUDIO_AL_LOCAL;
					pseq->flags |= STUDIO_LOCAL;
				}
				else
				{
					UnGetToken();
					break;
				}
			}

			pseq->numautolayers++;
		}
		else if ((numblends || isAppend) && ParseAnimationToken( animations[0] ))
		{

		}
		else if (!isAppend)
		{
			// assume it's an animation reference
			// first look up an existing animation
			for (n = 0; n < g_numani; n++)
			{
				if (stricmp( token, g_panimation[n]->name ) == 0)
				{
					animations[numblends++] = g_panimation[n];
					break;
				}
			}

			if (n >= g_numani)
			{
				// assume it's an implied animation
				animations[numblends++] = Cmd_ImpliedAnimation( pseq, token );
			}
			// hack to allow animation commands to refer to same sequence
			if (numblends == 1)
			{
				pseq->panim[0][0] = animations[0];
			}

		}
		else
		{
			TokenError( "unknown command \"%s\"\n", token );
		}

		if (depth < 0)
		{
			TokenError("missing {\n");
		}
	};

	if (isAppend)
	{
		return 0;
	}

	if (numblends == 0)
	{
		TokenError("no animations found\n");
	}

	if (pseq->groupsize[0] == 0)
	{
		if (numblends < 4)
		{
			pseq->groupsize[0] = numblends;
			pseq->groupsize[1] = 1;
		}
		else
		{
			i = sqrt( (float) numblends );
			if (i * i == numblends)
			{
				pseq->groupsize[0] = i;
				pseq->groupsize[1] = i;
			}
			else
			{
				TokenError( "non-square (%d) number of blends without \"blendwidth\" set\n", numblends );
			}
		}
	}
	else
	{
		pseq->groupsize[1] = numblends / pseq->groupsize[0];

		if (pseq->groupsize[0] * pseq->groupsize[1] != numblends)
		{
			TokenError( "missing animation blends. Expected %d, found %d\n", 
				pseq->groupsize[0] * pseq->groupsize[1], numblends );
		}
	}

	for (i = 0; i < numblends; i++)
	{
		j = i % pseq->groupsize[0];
		k = i / pseq->groupsize[0];
		
		pseq->panim[j][k] = animations[i];

		if (i > 0 && animations[i]->isimplied)
		{
			CopyAnimationSettings( animations[i], animations[0] );
		}
		pseq->flags |= animations[i]->flags;
	}

	pseq->numblends = numblends;

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: throw away all the options for a specific sequence or animation
//-----------------------------------------------------------------------------

int ParseEmpty( )
{
	int depth = 0;

	while (1)
	{
		if (depth > 0)
		{
			if(!GetToken(true)) 
			{
				break;
			}
		}
		else
		{
			if (!TokenAvailable()) 
			{
				break;
			}
			else 
			{
				GetToken (false);
			}
		}

		if (endofscript)
		{
			if (depth != 0)
			{
				TokenError("missing }\n" );
			}
			return 1;
		}
		if (stricmp("{", token ) == 0)
		{
			depth++;
		}
		else if (stricmp("}", token ) == 0)
		{
			depth--;
		}

		if (depth < 0)
		{
			TokenError("missing {\n");
		}
	};

	return 0;
}


//-----------------------------------------------------------------------------
// Purpose: append commands to either a sequence or an animation
//-----------------------------------------------------------------------------
void Cmd_Append( )
{
	GetToken(false);


	s_sequence_t *pseq = LookupSequence( token );

	if (pseq)
	{
		ParseSequence( pseq, true );
		return;
	}
	else
	{
		s_animation_t *panim = LookupAnimation( token );

		if (panim)
		{
			ParseAnimation( panim, true );
			return;
		}
	}
	TokenError( "unknown append animation %s\n", token );
}



void Cmd_Prepend( )
{
	GetToken(false);

	s_sequence_t *pseq = LookupSequence( token );
	int count = 0;
	s_animation_t *panim = NULL;
	int iRet =  false;

	if (pseq)
	{
		panim = pseq->panim[0][0];
		count = panim->numcmds;
		iRet = ParseSequence( pseq, true );
	}
	else
	{
		panim = LookupAnimation( token );
		if (panim)
		{
			count = panim->numcmds;
			iRet = ParseAnimation( panim, true );
		}
	}
	if (panim && count != panim->numcmds)
	{
		s_animcmd_t tmp;
		tmp = panim->cmds[panim->numcmds - 1];
		int i;
		for (i = panim->numcmds - 1; i > 0; i--)
		{
			panim->cmds[i] = panim->cmds[i-1];
		}
		panim->cmds[0] = tmp;
		return;
	}
	TokenError( "unknown prepend animation \"%s\"\n", token );
}

void Cmd_Continue( )
{
	GetToken(false);

	s_sequence_t *pseq = LookupSequence( token );

	if (pseq)
	{
		GetToken(true);
		UnGetToken();
		if (token[0] != '$')
			ParseSequence( pseq, true );
		return;
	}
	else
	{
		s_animation_t *panim = LookupAnimation( token );

		if (panim)
		{
			GetToken(true);
			UnGetToken();
			if (token[0] != '$')
				ParseAnimation( panim, true );
			return;
		}
	}
	TokenError( "unknown continue animation %s\n", token );
}

//-----------------------------------------------------------------------------
// Purpose: foward declare an empty sequence
//-----------------------------------------------------------------------------

void Cmd_DeclareSequence( void )
{
	if (g_sequence.Count() >= MAXSTUDIOSEQUENCES)
	{
		TokenError("Too many sequences (%d max)\n", MAXSTUDIOSEQUENCES );
	}

	s_sequence_t *pseq = &g_sequence[ g_sequence.AddToTail() ];
	memset( pseq, 0, sizeof( s_sequence_t ) );
	pseq->flags = STUDIO_OVERRIDE;

	// initialize sequence
	GetToken( false );
	strcpyn( pseq->name, token );
}


//-----------------------------------------------------------------------------
// Purpose: foward declare an empty sequence
//-----------------------------------------------------------------------------

void Cmd_DeclareAnimation( void )
{
	if (g_numani >= MAXSTUDIOANIMS)
	{
		TokenError("Too many animations (%d max)\n", MAXSTUDIOANIMS );
	}

	// allocate animation entry
	s_animation_t *panim = (s_animation_t *)kalloc( 1, sizeof( s_animation_t ) );
	g_panimation[g_numani] = panim;
	panim->index = g_numani;
	panim->flags = STUDIO_OVERRIDE;
	g_numani++;
	
	// initialize animation
	GetToken( false );
	strcpyn( panim->name, token );
}


//-----------------------------------------------------------------------------
// Purpose: create named list of boneweights
//-----------------------------------------------------------------------------
void Option_Weightlist( s_weightlist_t *pweightlist )
{
	int depth = 0;
	int i;

	pweightlist->numbones = 0;

	while (1)
	{
		if (depth > 0)
		{
			if(!GetToken(true)) 
			{
				break;
			}
		}
		else
		{
			if (!TokenAvailable()) 
			{
				break;
			}
			else 
			{
				GetToken (false);
			}
		}

		if (endofscript)
		{
			if (depth != 0)
			{
				TokenError("missing }\n" );
			}
			return;
		}
		if (stricmp("{", token ) == 0)
		{
			depth++;
		}
		else if (stricmp("}", token ) == 0)
		{
			depth--;
		}
		else if (stricmp("posweight", token ) == 0)
		{
			i = pweightlist->numbones - 1;
			if (i < 0)
			{
				MdlError( "Error with specifing bone Position weight \'%s:%s\'\n", pweightlist->name, pweightlist->bonename[i] );
			}
			GetToken( false );
			pweightlist->boneposweight[i] = verify_atof( token );
			if (pweightlist->boneweight[i] == 0 && pweightlist->boneposweight[i] > 0)
			{
				MdlError( "Non-zero Position weight with zero Rotation weight not allowed \'%s:%s %f %f\'\n", 
					pweightlist->name, pweightlist->bonename[i], pweightlist->boneweight[i], pweightlist->boneposweight[i] );
			}
		}
		else
		{
			i = pweightlist->numbones++;
			if (i >= MAXWEIGHTSPERLIST)
			{
				TokenError("Too many bones (%d) in weightlist '%s'\n", i, pweightlist->name );
			}
			strcpyn( pweightlist->bonename[i], token );
			GetToken( false );
			pweightlist->boneweight[i] = verify_atof( token );
			pweightlist->boneposweight[i] = pweightlist->boneweight[i];
		}

		if (depth < 0)
		{
			TokenError("missing {\n");
		}
	};
}


void Cmd_Weightlist( )
{
	int i;

	if (!GetToken(false)) 
		return;

	if (g_numweightlist >= MAXWEIGHTLISTS)
	{
		TokenError( "Too many weightlist commands (%d)\n", MAXWEIGHTLISTS );
	}

	for (i = 1; i < g_numweightlist; i++)
	{
		if (stricmp( g_weightlist[i].name, token ) == 0)
		{
			TokenError( "Duplicate weightlist '%s'\n", token );
		}
	}

	strcpyn( g_weightlist[i].name, token );

	Option_Weightlist( &g_weightlist[g_numweightlist] );

	g_numweightlist++;
}

void Cmd_DefaultWeightlist( )
{
	Option_Weightlist( &g_weightlist[0] );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

void Option_Eyeball( s_model_t *pmodel )
{
	Vector	tmp;
	int i, j;
	int mesh_material;
	char szMeshMaterial[256];

	s_eyeball_t *eyeball = &(pmodel->eyeball[pmodel->numeyeballs++]);

	// name
	GetToken (false);
	strcpyn( eyeball->name, token );

	// bone name
	GetToken (false);
	for (i = 0; i < pmodel->source->numbones; i++)
	{
		if (stricmp( pmodel->source->localBone[i].name, token ) == 0)
		{
			eyeball->bone = i;
			break;
		}
	}
	if (!g_bCreateMakefile && i >= pmodel->source->numbones)
	{
		TokenError( "unknown eyeball bone \"%s\"\n", token );
	}

	// X
	GetToken (false);
	tmp[0] = verify_atof (token);

	// Y
	GetToken (false);
	tmp[1] = verify_atof (token);

	// Z
	GetToken (false);
	tmp[2] = verify_atof (token);

	// mesh material 
	GetToken (false);
	strcpyn( szMeshMaterial, token );
	mesh_material = use_texture_as_material( lookup_texture( token, sizeof( token ) ) );

	// diameter
	GetToken (false);
	eyeball->radius = verify_atof (token) / 2.0;

	// Z angle offset
	GetToken (false);
	eyeball->zoffset = tan( DEG2RAD( verify_atof (token) ) );

	// iris material
	GetToken (false);
	eyeball->iris_material = use_texture_as_material( lookup_texture( token, sizeof( token ) ) );

	// pupil scale
	GetToken (false);
	eyeball->iris_scale = 1.0 / verify_atof( token );

	eyeball->glint_material = use_texture_as_material( lookup_texture( "glint", Q_strlen( "glint" ) + 1 ) );
	
	VectorCopy( tmp, eyeball->org );

	for (i = 0; i < pmodel->source->nummeshes; i++)
	{
		j = pmodel->source->meshindex[i]; // meshes are internally stored by material index

		if (j == mesh_material)
		{
			eyeball->mesh = i; // FIXME: should this be pre-adjusted?
			break;
		}
	}

	if (!g_bCreateMakefile && i >= pmodel->source->nummeshes)
	{
		TokenError("can't find eyeball texture \"%s\" on model\n", szMeshMaterial );
	}
	
	// translate eyeball into bone space
	VectorITransform( tmp, pmodel->source->boneToPose[eyeball->bone], eyeball->org );

	tmp[0] = 0; tmp[1] = 0; tmp[2] = 1;
	VectorIRotate( tmp, pmodel->source->boneToPose[eyeball->bone], eyeball->up );

	tmp[0] = 0; tmp[1] = 1; tmp[2] = 0; // FIXME: this is backwards
	VectorIRotate( tmp, pmodel->source->boneToPose[eyeball->bone], eyeball->forward );

	// these get overwritten by "eyelid" data
	eyeball->upperlidflexdesc		= 0;
	eyeball->lowerlidflexdesc		= 0;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

void Option_Spherenormals( s_source_t *psource )
{
	Vector	pos;
	int i, j;
	int mesh_material;
	char szMeshMaterial[256];

	// mesh material 
	GetToken (false);
	strcpyn( szMeshMaterial, token );
	mesh_material = use_texture_as_material( lookup_texture( token, sizeof( token ) ) );

	// X
	GetToken (false);
	pos[0] = verify_atof (token);

	// Y
	GetToken (false);
	pos[1] = verify_atof (token);

	// Z
	GetToken (false);
	pos[2] = verify_atof (token);

	for (i = 0; i < psource->nummeshes; i++)
	{
		j = psource->meshindex[i]; // meshes are internally stored by material index

		if (j == mesh_material)
		{
			s_vertexinfo_t *vertex = &psource->vertex[psource->mesh[i].vertexoffset];

			for (int k = 0; k < psource->mesh[i].numvertices; k++)
			{
				Vector n = vertex[k].position - pos;
				VectorNormalize( n );
				if (DotProduct( n, vertex[k].normal ) < 0.0)
				{
					vertex[k].normal = -1 * n;
				}
				else
				{
					vertex[k].normal = n;
				}
#if 0
				vertex[k].normal[0] += 0.5f * ( 2.0f * ( ( float )rand() ) / ( float )RAND_MAX ) - 1.0f;
				vertex[k].normal[1] += 0.5f * ( 2.0f * ( ( float )rand() ) / ( float )RAND_MAX ) - 1.0f;
				vertex[k].normal[2] += 0.5f * ( 2.0f * ( ( float )rand() ) / ( float )RAND_MAX ) - 1.0f;
				VectorNormalize( vertex[k].normal );
#endif
			}
			break;
		}
	}

	if (i >= psource->nummeshes)
	{
		TokenError("can't find spherenormal texture \"%s\" on model\n", szMeshMaterial );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int Add_Flexdesc( const char *name )
{
	int flexdesc;
	for ( flexdesc = 0; flexdesc < g_numflexdesc; flexdesc++)
	{
		if (stricmp( name, g_flexdesc[flexdesc].FACS ) == 0)
		{
			break;
		}
	}

	if (flexdesc >= MAXSTUDIOFLEXDESC)
	{
		TokenError( "Too many flex types, max %d\n", MAXSTUDIOFLEXDESC );
	}

	if (flexdesc == g_numflexdesc)
	{
		strcpyn( g_flexdesc[flexdesc].FACS, name );

		g_numflexdesc++;
	}
	return flexdesc;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

void Option_Flex( char *name, char *vtafile, int imodel, float pairsplit )
{
	if (g_numflexkeys >= MAXSTUDIOFLEXKEYS)
	{
		TokenError( "Too many flexes, max %d\n", MAXSTUDIOFLEXKEYS );
	}

	int flexdesc, flexpair;
	
	if (pairsplit != 0)
	{
		char mod[256];
		sprintf( mod, "%sR", name );
		flexdesc = Add_Flexdesc( mod );
		sprintf( mod, "%sL", name );
		flexpair = Add_Flexdesc( mod );
	}
	else
	{
		flexdesc = Add_Flexdesc( name );
		flexpair = 0;
	}

	// initialize
	g_flexkey[g_numflexkeys].imodel = imodel;
	g_flexkey[g_numflexkeys].flexdesc = flexdesc;
	g_flexkey[g_numflexkeys].target0 = 0.0;
	g_flexkey[g_numflexkeys].target1 = 1.0;
	g_flexkey[g_numflexkeys].target2 = 10;
	g_flexkey[g_numflexkeys].target3 = 11;
	g_flexkey[g_numflexkeys].split = pairsplit;
	g_flexkey[g_numflexkeys].flexpair = flexpair;
	g_flexkey[g_numflexkeys].decay = 1.0;

	while (TokenAvailable())
	{
		GetToken(false);

		if (stricmp( token, "frame") == 0)
		{
			GetToken (false);

			g_flexkey[g_numflexkeys].frame = verify_atoi( token );
		}
		else if (stricmp( token, "position") == 0)
		{
			GetToken (false);
			g_flexkey[g_numflexkeys].target1 = verify_atof( token );
		}
		else if (stricmp( token, "split") == 0)
		{
			GetToken (false);
			g_flexkey[g_numflexkeys].split = verify_atof( token );
		}
		else if (stricmp( token, "decay") == 0)
		{
			GetToken (false);
			g_flexkey[g_numflexkeys].decay = verify_atof( token );
		}
		else
		{
			TokenError( "unknown option: %s", token );
		}

	}

	if (g_numflexkeys > 1)
	{
		if (g_flexkey[g_numflexkeys-1].flexdesc == g_flexkey[g_numflexkeys].flexdesc)
		{
			g_flexkey[g_numflexkeys-1].target2 = g_flexkey[g_numflexkeys-1].target1;
			g_flexkey[g_numflexkeys-1].target3 = g_flexkey[g_numflexkeys].target1;
			g_flexkey[g_numflexkeys].target0 = g_flexkey[g_numflexkeys-1].target1;
		}
	}

	// link to source
	g_flexkey[g_numflexkeys].source = Load_Source( vtafile, "vta" );
	g_numflexkeys++;
	// this needs to be per model.
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

void Option_Eyelid( int imodel )
{
	char type[256];
	char vtafile[256];

	// type
	GetToken (false);
	strcpyn( type, token );

	// source
	GetToken (false);
	strcpyn( vtafile, token );

	int lowererframe, neutralframe, raiserframe;
	float lowerertarget, neutraltarget, raisertarget;
	int lowererdesc, neutraldesc, raiserdesc;
	int basedesc;
	float split = 0;
	char szEyeball[64] = {""};

	basedesc = g_numflexdesc;
	strcpyn( g_flexdesc[g_numflexdesc++].FACS, type );

	while (TokenAvailable())
	{
		GetToken(false);

		char localdesc[256];
		strcpy( localdesc, type );
		strcat( localdesc, "_" );
		strcat( localdesc, token );

		if (stricmp( token, "lowerer") == 0)
		{
			GetToken (false);
			lowererframe = verify_atoi( token );
			GetToken (false);
			lowerertarget = verify_atof( token );
			lowererdesc = g_numflexdesc;
			strcpyn( g_flexdesc[g_numflexdesc++].FACS, localdesc );
		}
		else if (stricmp( token, "neutral") == 0)
		{
			GetToken (false);
			neutralframe = verify_atoi( token );
			GetToken (false);
			neutraltarget = verify_atof( token );
			neutraldesc = g_numflexdesc;
			strcpyn( g_flexdesc[g_numflexdesc++].FACS, localdesc );
		}
		else if (stricmp( token, "raiser") == 0)
		{
			GetToken (false);
			raiserframe = verify_atoi( token );
			GetToken (false);
			raisertarget = verify_atof( token );
			raiserdesc = g_numflexdesc;
			strcpyn( g_flexdesc[g_numflexdesc++].FACS, localdesc );
		}
		else if (stricmp( token, "split") == 0)
		{
			GetToken (false);
			split = verify_atof( token );
		}
		else if (stricmp( token, "eyeball") == 0)
		{
			GetToken (false);
			strcpy( szEyeball, token );
		}


		else
		{
			TokenError( "unknown option: %s", token );
		}
	}

	g_flexkey[g_numflexkeys+0].source = Load_Source( vtafile, "vta" );
	g_flexkey[g_numflexkeys+0].frame = lowererframe;
	g_flexkey[g_numflexkeys+0].flexdesc = basedesc;
	g_flexkey[g_numflexkeys+0].imodel = imodel;
	g_flexkey[g_numflexkeys+0].split = split;
	g_flexkey[g_numflexkeys+0].target0 = -11;
	g_flexkey[g_numflexkeys+0].target1 = -10;
	g_flexkey[g_numflexkeys+0].target2 = lowerertarget;
	g_flexkey[g_numflexkeys+0].target3 = neutraltarget;
	g_flexkey[g_numflexkeys+0].decay = 0.0;

	g_flexkey[g_numflexkeys+1].source = g_flexkey[g_numflexkeys+0].source;
	g_flexkey[g_numflexkeys+1].frame = neutralframe;
	g_flexkey[g_numflexkeys+1].flexdesc = basedesc;
	g_flexkey[g_numflexkeys+1].imodel = imodel;
	g_flexkey[g_numflexkeys+1].split = split;
	g_flexkey[g_numflexkeys+1].target0 = lowerertarget;
	g_flexkey[g_numflexkeys+1].target1 = neutraltarget;
	g_flexkey[g_numflexkeys+1].target2 = neutraltarget;
	g_flexkey[g_numflexkeys+1].target3 = raisertarget;
	g_flexkey[g_numflexkeys+1].decay = 0.0;

	g_flexkey[g_numflexkeys+2].source = g_flexkey[g_numflexkeys+0].source;
	g_flexkey[g_numflexkeys+2].frame = raiserframe;
	g_flexkey[g_numflexkeys+2].flexdesc = basedesc;
	g_flexkey[g_numflexkeys+2].imodel = imodel;
	g_flexkey[g_numflexkeys+2].split = split;
	g_flexkey[g_numflexkeys+2].target0 = neutraltarget;
	g_flexkey[g_numflexkeys+2].target1 = raisertarget;
	g_flexkey[g_numflexkeys+2].target2 = 10;
	g_flexkey[g_numflexkeys+2].target3 = 11;
	g_flexkey[g_numflexkeys+2].decay = 0.0;
	g_numflexkeys += 3;

	s_model_t *pmodel = g_model[imodel];
	for (int i = 0; i < pmodel->numeyeballs; i++)
	{
		s_eyeball_t *peyeball = &(pmodel->eyeball[i]);

		if (szEyeball[0] != '\0')
		{
			if (stricmp( peyeball->name, szEyeball ) != 0)
				continue;
		}

		if (fabs( lowerertarget ) > peyeball->radius)
		{
			TokenError( "Eyelid \"%s\" lowerer out of range (+-%.1f)\n", type, peyeball->radius );
		}
		if (fabs( neutraltarget ) > peyeball->radius)
		{
			TokenError( "Eyelid \"%s\" neutral out of range (+-%.1f)\n", type, peyeball->radius );
		}
		if (fabs( raisertarget ) > peyeball->radius)
		{
			TokenError( "Eyelid \"%s\" raiser  out of range (+-%.1f)\n", type, peyeball->radius );
		}

		switch( type[0] )
		{
		case 'u':
			peyeball->upperlidflexdesc	= basedesc;
			peyeball->upperflexdesc[0]	= lowererdesc; 
			peyeball->uppertarget[0]	= lowerertarget;
			peyeball->upperflexdesc[1]	= neutraldesc; 
			peyeball->uppertarget[1]	= neutraltarget;
			peyeball->upperflexdesc[2]	= raiserdesc; 
			peyeball->uppertarget[2]	= raisertarget;
			break;
		case 'l':
			peyeball->lowerlidflexdesc	= basedesc;
			peyeball->lowerflexdesc[0]	= lowererdesc; 
			peyeball->lowertarget[0]	= lowerertarget;
			peyeball->lowerflexdesc[1]	= neutraldesc; 
			peyeball->lowertarget[1]	= neutraltarget;
			peyeball->lowerflexdesc[2]	= raiserdesc; 
			peyeball->lowertarget[2]	= raisertarget;
			break;
		}
	}
}

/*
=================
=================
*/
int Option_Mouth( s_model_t *pmodel )
{
	// index
	GetToken (false);
	int index = verify_atoi( token );
	if (index >= g_nummouths)
		g_nummouths = index + 1;

	// flex controller name
	GetToken (false);
	g_mouth[index].flexdesc = Add_Flexdesc( token );

	// bone name
	GetToken (false);
	strcpyn( g_mouth[index].bonename, token );

	// vector
	GetToken (false);
	g_mouth[index].forward[0] = verify_atof( token );
	GetToken (false);
	g_mouth[index].forward[1] = verify_atof( token );
	GetToken (false);
	g_mouth[index].forward[2] = verify_atof( token );
	return 0;
}



void Option_Flexcontroller( s_model_t *pmodel )
{
	char type[256];
	float range_min = 0.0;
	float range_max = 1.0;

	// g_flex
	GetToken (false);
	strcpy( type, token );

	while (TokenAvailable())
	{
		GetToken(false);

		if (stricmp( token, "range") == 0)
		{
			GetToken(false);
			range_min = verify_atof( token );

			GetToken(false);
			range_max = verify_atof( token );
		}
		else
		{
			if (g_numflexcontrollers >= MAXSTUDIOFLEXCTRL)
			{
				TokenError( "Too many flex controllers, max %d\n", MAXSTUDIOFLEXCTRL );
			}


			strcpyn( g_flexcontroller[g_numflexcontrollers].name, token );
			strcpyn( g_flexcontroller[g_numflexcontrollers].type, type );
			g_flexcontroller[g_numflexcontrollers].min = range_min;
			g_flexcontroller[g_numflexcontrollers].max = range_max;
			g_numflexcontrollers++;
		}
	}

	// this needs to be per model.
}

void Option_Flexrule( s_model_t *pmodel, char *name )
{
	int precedence[32];
	precedence[ STUDIO_CONST ] = 	0;
	precedence[ STUDIO_FETCH1 ] =	0;
	precedence[ STUDIO_FETCH2 ] =	0;
	precedence[ STUDIO_ADD ] =		1;
	precedence[ STUDIO_SUB ] =		1;
	precedence[ STUDIO_MUL ] =		2;
	precedence[ STUDIO_DIV ] =		2;
	precedence[ STUDIO_NEG ] =		4;
	precedence[ STUDIO_EXP ] =		3;
	precedence[ STUDIO_OPEN ] =		0;	// only used in token parsing
	precedence[ STUDIO_CLOSE ] =	0;
	precedence[ STUDIO_COMMA ] =	0;
	precedence[ STUDIO_MAX ] =		5;
	precedence[ STUDIO_MIN ] =		5;

	s_flexop_t stream[MAX_OPS];
	int i = 0;
	s_flexop_t stack[MAX_OPS];
	int j = 0;
	int k = 0;

	s_flexrule_t *pRule = &g_flexrule[g_numflexrules++];

	if (g_numflexrules > MAXSTUDIOFLEXRULES)
	{
		TokenError( "Too many flex rules (max %d)\n", MAXSTUDIOFLEXRULES );
	}

	int flexdesc;
	for ( flexdesc = 0; flexdesc < g_numflexdesc; flexdesc++)
	{
		if (stricmp( name, g_flexdesc[flexdesc].FACS ) == 0)
		{
			break;
		}
	}

	if (flexdesc >= g_numflexdesc)
	{
		TokenError( "Rule for unknown flex %s\n", g_flexdesc[flexdesc].FACS );
	}

	pRule->flex = flexdesc;
	pRule->numops = 0;

	// = 
	GetToken(false);

	// parse all the tokens
	bool linecontinue = false;
	while ( linecontinue || TokenAvailable())
	{
		GetExprToken(linecontinue);

		linecontinue = false;

		if ( token[0] == '\\' )
		{
			if (!GetToken(false) || token[0] != '\\')
			{
				TokenError( "unknown expression token '\\%s\n", token );
			}
			linecontinue = true;
		}
		else if ( token[0] == '(' )
		{
			stream[i++].op = STUDIO_OPEN;
		}
		else if ( token[0] == ')' )
		{
			stream[i++].op = STUDIO_CLOSE;
		}
		else if ( token[0] == '+' )
		{
			stream[i++].op = STUDIO_ADD;
		}
		else if ( token[0] == '-' )
		{
			stream[i].op = STUDIO_SUB;
			if (i > 0)
			{
				switch( stream[i-1].op )
				{
				case STUDIO_OPEN:
				case STUDIO_ADD:
				case STUDIO_SUB:
				case STUDIO_MUL:
				case STUDIO_DIV:
				case STUDIO_COMMA:
					// it's a unary if it's preceded by a "(+-*/,"?
					stream[i].op = STUDIO_NEG;
					break;
				}
			}
			i++;
		}
		else if ( token[0] == '*' )
		{
			stream[i++].op = STUDIO_MUL;
		}
		else if ( token[0] == '/' )
		{
			stream[i++].op = STUDIO_DIV;
		}
		else if ( isdigit( token[0] ))
		{
			stream[i].op = STUDIO_CONST;
			stream[i++].d.value = verify_atof( token );
		}
		else if ( token[0] == ',' )
		{
			stream[i++].op = STUDIO_COMMA;
		}
		else if ( stricmp( token, "max" ) == 0)
		{
			stream[i++].op = STUDIO_MAX;
		}
		else if ( stricmp( token, "min" ) == 0)
		{
			stream[i++].op = STUDIO_MIN;
		}
		else 
		{
			if (token[0] == '%')
			{
				GetExprToken(false);

				for (k = 0; k < g_numflexdesc; k++)
				{
					if (stricmp( token, g_flexdesc[k].FACS ) == 0)
					{
						stream[i].op = STUDIO_FETCH2;
						stream[i++].d.index = k;
						break;
					}
				}
				if (k >= g_numflexdesc)
				{
					TokenError( "unknown flex %s\n", token );
				}
			}
			else
			{
				for (k = 0; k < g_numflexcontrollers; k++)
				{
					if (stricmp( token, g_flexcontroller[k].name ) == 0)
					{
						stream[i].op = STUDIO_FETCH1;
						stream[i++].d.index = k;
						break;
					}
				}
				if (k >= g_numflexcontrollers)
				{
					TokenError( "unknown controller %s\n", token );
				}
			}
		}
	}

	if (i > MAX_OPS)
	{
		TokenError("expression %s too complicated\n", g_flexdesc[pRule->flex].FACS );
	}

	if (0)
	{
		printf("%s = ", g_flexdesc[pRule->flex].FACS );
		for ( k = 0; k < i; k++)
		{
			switch( stream[k].op )
			{
			case STUDIO_CONST: printf("%f ", stream[k].d.value ); break;
			case STUDIO_FETCH1: printf("%s ", g_flexcontroller[stream[k].d.index].name ); break;
			case STUDIO_FETCH2: printf("[%d] ", stream[k].d.index ); break;
			case STUDIO_ADD: printf("+ "); break;
			case STUDIO_SUB: printf("- "); break;
			case STUDIO_MUL: printf("* "); break;
			case STUDIO_DIV: printf("/ "); break;
			case STUDIO_NEG: printf("neg "); break;
			case STUDIO_MAX: printf("max "); break;
			case STUDIO_MIN: printf("min "); break;
			case STUDIO_COMMA: 	printf(", "); break; // error
			case STUDIO_OPEN: 	printf("( " ); break; // error
			case STUDIO_CLOSE: 	printf(") " ); break; // error
			default:
				printf("err%d ", stream[k].op ); break;
			}
		}
		printf("\n");
		// exit(1);
	}

	j = 0;
	for (k = 0; k < i; k++)
	{
		if (j >= MAX_OPS)
		{
			TokenError("expression %s too complicated\n", g_flexdesc[pRule->flex].FACS );
		}
		switch( stream[k].op )
		{
		case STUDIO_CONST:
		case STUDIO_FETCH1:
		case STUDIO_FETCH2:
			pRule->op[pRule->numops++] = stream[k];
			break;
		case STUDIO_OPEN:
			stack[j++] = stream[k];
			break;
		case STUDIO_CLOSE:
			// pop all operators off of the stack until an open paren
			while (j > 0 && stack[j-1].op != STUDIO_OPEN)
			{
				pRule->op[pRule->numops++] = stack[j-1];
				j--;
			}
			if (j == 0)
			{
				TokenError( "unmatched closed parentheses\n" );
			}
			if (j > 0) 
				j--;
			break;
		case STUDIO_COMMA:
			// pop all operators off of the stack until an open paren
			while (j > 0 && stack[j-1].op != STUDIO_OPEN)
			{
				pRule->op[pRule->numops++] = stack[j-1];
				j--;
			}
			// push operator onto the stack
			stack[j++] = stream[k];
			break;
		case STUDIO_ADD:
		case STUDIO_SUB:
		case STUDIO_MUL:
		case STUDIO_DIV:
			// pop all operators off of the stack that have equal or higher precedence
			while (j > 0 && precedence[stream[k].op] <= precedence[stack[j-1].op])
			{
				pRule->op[pRule->numops++] = stack[j-1];
				j--;
			}
			// push operator onto the stack
			stack[j++] = stream[k];
			break;
		case STUDIO_NEG:
			if (stream[k+1].op == STUDIO_CONST)
			{
				// change sign of constant, skip op
				stream[k+1].d.value = -stream[k+1].d.value;
			}
			else
			{
				// push operator onto the stack
				stack[j++] = stream[k];
			}
			break;
		case STUDIO_MAX:
		case STUDIO_MIN:
			// push operator onto the stack
			stack[j++] = stream[k];
			break;
		}
		if (pRule->numops >= MAX_OPS)
			TokenError("expression for \"%s\" too complicated\n", g_flexdesc[pRule->flex].FACS );
	}
	// pop all operators off of the stack
	while (j > 0)
	{
		pRule->op[pRule->numops++] = stack[j-1];
		j--;
		if (pRule->numops >= MAX_OPS)
			TokenError("expression for \"%s\" too complicated\n", g_flexdesc[pRule->flex].FACS );
	}

	// reprocess the operands, eating commas for all functions
	int numCommas = 0;
	j = 0;
	for (k = 0; k < pRule->numops; k++)
	{
		switch( pRule->op[k].op )
		{
		case STUDIO_MAX:
		case STUDIO_MIN:
			if (pRule->op[j-1].op != STUDIO_COMMA)
			{
				TokenError( "missing comma\n");
			}
			// eat the comma operator
			numCommas--;
			pRule->op[j-1] = pRule->op[k];
			break;
		case STUDIO_COMMA:
			numCommas++;
			pRule->op[j++] = pRule->op[k];
			break;
		default:
			pRule->op[j++] = pRule->op[k];
			break;
		}
	}
	pRule->numops = j;
	if (numCommas != 0)
	{
		TokenError( "too many comma's\n" );
	}

	if (pRule->numops > MAX_OPS)
	{
		TokenError("expression %s too complicated\n", g_flexdesc[pRule->flex].FACS );
	}

	if (0)
	{
		printf("%s = ", g_flexdesc[pRule->flex].FACS );
		for ( i = 0; i < pRule->numops; i++)
		{
			switch( pRule->op[i].op )
			{
			case STUDIO_CONST: printf("%f ", pRule->op[i].d.value ); break;
			case STUDIO_FETCH1: printf("%s ", g_flexcontroller[pRule->op[i].d.index].name ); break;
			case STUDIO_FETCH2: printf("[%d] ", pRule->op[i].d.index ); break;
			case STUDIO_ADD: printf("+ "); break;
			case STUDIO_SUB: printf("- "); break;
			case STUDIO_MUL: printf("* "); break;
			case STUDIO_DIV: printf("/ "); break;
			case STUDIO_NEG: printf("neg "); break;
			case STUDIO_MAX: printf("max "); break;
			case STUDIO_MIN: printf("min "); break;
			case STUDIO_COMMA: 	printf(", "); break; // error
			case STUDIO_OPEN: 	printf("( " ); break; // error
			case STUDIO_CLOSE: 	printf(") " ); break; // error
			default:
				printf("err%d ", pRule->op[i].op ); break;
			}
		}
		printf("\n");
		// exit(1);
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------


void Cmd_Model( )
{
	g_model[g_nummodels] = (s_model_t *)kalloc( 1, sizeof( s_model_t ) );
	
	// name
	if (!GetToken(false)) 
		return;
	strcpyn( g_model[g_nummodels]->name, token );

	// fake g_bodypart stuff
	if (g_numbodyparts == 0) {
		g_bodypart[g_numbodyparts].base = 1;
	}
	else {
		g_bodypart[g_numbodyparts].base = g_bodypart[g_numbodyparts-1].base * g_bodypart[g_numbodyparts-1].nummodels;
	}
	strcpyn( g_bodypart[g_numbodyparts].name, token );

	g_bodypart[g_numbodyparts].pmodel[g_bodypart[g_numbodyparts].nummodels] = g_model[g_nummodels];
	g_bodypart[g_numbodyparts].nummodels = 1;
	g_numbodyparts++;

	Option_Studio( g_model[g_nummodels] );
	
	int depth = 0;
	while (1)
	{
		char FAC[256], vtafile[256];
		if (depth > 0)
		{
			if(!GetToken(true)) 
			{
				break;
			}
		}
		else
		{
			if (!TokenAvailable()) 
			{
				break;
			}
			else 
			{
				GetToken (false);
			}
		}

		if (endofscript)
		{
			if (depth != 0)
			{
				TokenError("missing }\n" );
			}
			return;
		}
		if (stricmp("{", token ) == 0)
		{
			depth++;
		}
		else if (stricmp("}", token ) == 0)
		{
			depth--;
		}
		else if (stricmp("eyeball", token ) == 0)
		{
			Option_Eyeball( g_model[g_nummodels] );
		}
		else if (stricmp("eyelid", token ) == 0)
		{
			Option_Eyelid( g_nummodels );
		}
		else if (stricmp("flex", token ) == 0)
		{
			// g_flex
			GetToken (false);
			strcpy( FAC, token );
			if (depth == 0)
			{
				// file
				GetToken (false);
				strcpy( vtafile, token );
			}
			Option_Flex( FAC, vtafile, g_nummodels, 0.0 ); // FIXME: this needs to point to a model used, not loaded!!!
		}
		else if (stricmp("flexpair", token ) == 0)
		{
			// g_flex
			GetToken (false);
			strcpy( FAC, token );

			GetToken( false );
			float split = atof( token );

			if (depth == 0)
			{
				// file
				GetToken (false);
				strcpy( vtafile, token );
			}
			Option_Flex( FAC, vtafile, g_nummodels, split ); // FIXME: this needs to point to a model used, not loaded!!!
		}
		else if (stricmp("defaultflex", token ) == 0)
		{
			if (depth == 0)
			{
				// file
				GetToken (false);
				strcpy( vtafile, token );
			}

			// g_flex
			Option_Flex( "default", vtafile, g_nummodels, 0.0 ); // FIXME: this needs to point to a model used, not loaded!!!
			g_defaultflexkey = &g_flexkey[g_numflexkeys-1];
		}
		else if (stricmp("flexfile", token ) == 0)
		{
			// file
			GetToken (false);
			strcpy( vtafile, token );
		}
		else if (stricmp("localvar", token ) == 0)
		{
			while (TokenAvailable())
			{
				GetToken( false );
				Add_Flexdesc( token );
			}
		}
		else if (stricmp("mouth", token ) == 0)
		{
			Option_Mouth( g_model[g_nummodels] );
		}
		else if (stricmp("flexcontroller", token ) == 0)
		{
			Option_Flexcontroller( g_model[g_nummodels] );
		}
		else if (token[0] == '%' )
		{
			Option_Flexrule( g_model[g_nummodels], &token[1] );
		}
		else if (stricmp("attachment", token ) == 0)
		{
		// 	Option_Attachment( g_model[g_nummodels] );
		}
		else if (stricmp( token, "spherenormals") == 0)
		{
			Option_Spherenormals( g_model[g_nummodels]->source );
		}
		else
		{
			TokenError( "unknown model option \"%s\"\n", token );
		}

		if (depth < 0)
		{
			TokenError("missing {\n");
		}
	};

	g_nummodels++;
}


void Cmd_FakeVTA( void )
{
	int depth = 0;

	GetToken( false );

	s_source_t *psource = (s_source_t *)kalloc( 1, sizeof( s_source_t ) );
	g_source[g_numsources] = psource;
	strcpyn( g_source[g_numsources]->filename, token );
	g_numsources++;

	while (1)
	{
		if (depth > 0)
		{
			if(!GetToken(true)) 
			{
				break;
			}
		}
		else
		{
			if (!TokenAvailable()) 
			{
				break;
			}
			else 
			{
				GetToken (false);
			}
		}

		if (endofscript)
		{
			if (depth != 0)
			{
				TokenError("missing }\n" );
			}
			return;
		}
		if (stricmp("{", token ) == 0)
		{
			depth++;
		}
		else if (stricmp("}", token ) == 0)
		{
			depth--;
		}
		else if (stricmp("appendvta", token ) == 0)
		{
			char filename[256];
			// file
			GetToken (false);
			strcpy( filename, token );
			
			GetToken( false );
			int frame = verify_atoi( token );

			AppendVTAtoOBJ( psource, filename, frame );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

void Cmd_IKChain( )
{
	if (!GetToken(false)) 
		return;

	int i;
	for ( i = 0; i < g_numikchains; i++)
	{
		if (stricmp( token, g_ikchain[i].name ) == 0)
		{
			break;
		}
	}
	if (i < g_numikchains)
	{
		if (!g_quiet)
		{
			printf("duplicate ikchain \"%s\" ignored\n", token );
		}
		while (TokenAvailable())
		{
			GetToken(false);
		}
		return;
	}

	strcpyn( g_ikchain[g_numikchains].name, token );

	GetToken(false);
	strcpyn( g_ikchain[g_numikchains].bonename, token );

	g_ikchain[g_numikchains].axis = STUDIO_Z;
	g_ikchain[g_numikchains].value = 0.0;
	g_ikchain[g_numikchains].height = 18.0;
	g_ikchain[g_numikchains].floor = 0.0;
	g_ikchain[g_numikchains].radius = 0.0;

	while (TokenAvailable())
	{
		GetToken(false);

		if (lookupControl( token ) != -1)
		{
			g_ikchain[g_numikchains].axis = lookupControl( token );
			GetToken(false);
			g_ikchain[g_numikchains].value = verify_atof( token );
		}
		else if (stricmp( "height", token ) == 0)
		{
			GetToken(false);
			g_ikchain[g_numikchains].height = verify_atof( token );
		}
		else if (stricmp( "pad", token ) == 0)
		{
			GetToken(false);
			g_ikchain[g_numikchains].radius = verify_atof( token ) / 2.0;
		}
		else if (stricmp( "floor", token ) == 0)
		{
			GetToken(false);
			g_ikchain[g_numikchains].floor = verify_atof( token );
		}
		else if (stricmp( "knee", token ) == 0)
		{
			GetToken(false);
			g_ikchain[g_numikchains].link[0].kneeDir.x = verify_atof( token );
			GetToken(false);
			g_ikchain[g_numikchains].link[0].kneeDir.y = verify_atof( token );
			GetToken(false);
			g_ikchain[g_numikchains].link[0].kneeDir.z = verify_atof( token );
		}
		else if (stricmp( "center", token ) == 0)
		{
			GetToken(false);
			g_ikchain[g_numikchains].center.x = verify_atof( token );
			GetToken(false);
			g_ikchain[g_numikchains].center.y = verify_atof( token );
			GetToken(false);
			g_ikchain[g_numikchains].center.z = verify_atof( token );
		}
	}
	g_numikchains++;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------


void Cmd_IKAutoplayLock( )
{
	GetToken(false);
	strcpyn( g_ikautoplaylock[g_numikautoplaylocks].name, token );

	GetToken(false);
	g_ikautoplaylock[g_numikautoplaylocks].flPosWeight = verify_atof( token );

	GetToken(false);
	g_ikautoplaylock[g_numikautoplaylocks].flLocalQWeight = verify_atof( token );
	
	g_numikautoplaylocks++;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

void Cmd_Root ()
{
	if (GetToken (false))
	{
		strcpyn( rootname, token );
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

void Cmd_Controller (void)
{
	if (GetToken (false))
	{
		if (!stricmp("mouth",token))
		{
			g_bonecontroller[g_numbonecontrollers].inputfield = 4;
		}
		else
		{
			g_bonecontroller[g_numbonecontrollers].inputfield = verify_atoi(token);
		}
		if (GetToken(false))
		{
			strcpyn( g_bonecontroller[g_numbonecontrollers].name, token );
			GetToken(false);
			if ((g_bonecontroller[g_numbonecontrollers].type = lookupControl(token)) == -1) 
			{
				MdlWarning("unknown g_bonecontroller type '%s'\n", token );
				return;
			}
			GetToken(false);
			g_bonecontroller[g_numbonecontrollers].start = verify_atof( token );
			GetToken(false);
			g_bonecontroller[g_numbonecontrollers].end = verify_atof( token );

			if (g_bonecontroller[g_numbonecontrollers].type & (STUDIO_XR | STUDIO_YR | STUDIO_ZR))
			{
				if (((int)(g_bonecontroller[g_numbonecontrollers].start + 360) % 360) == ((int)(g_bonecontroller[g_numbonecontrollers].end + 360) % 360))
				{
					g_bonecontroller[g_numbonecontrollers].type |= STUDIO_RLOOP;
				}
			}
			g_numbonecontrollers++;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

// Debugging function that enumerate all a models bones to stdout.
static void SpewBones()
{
	MdlWarning("g_numbones %i\n",g_numbones);

	for ( int i = g_numbones; --i >= 0; )
	{
		printf("%s\n",g_bonetable[i].name);
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

void Cmd_ScreenAlign ( void )
{
	if (GetToken (false))
	{
		
		Assert( g_numscreenalignedbones < MAXSTUDIOSRCBONES );

		strcpyn( g_screenalignedbone[g_numscreenalignedbones].name, token );
		g_screenalignedbone[g_numscreenalignedbones].flags = BONE_SCREEN_ALIGN_SPHERE;

		if( GetToken( false ) )
		{
			if( !stricmp( "sphere", token )  )
			{
				g_screenalignedbone[g_numscreenalignedbones].flags = BONE_SCREEN_ALIGN_SPHERE;				
			}
			else if( !stricmp( "cylinder", token ) )
			{
				g_screenalignedbone[g_numscreenalignedbones].flags = BONE_SCREEN_ALIGN_CYLINDER;				
			}
		}

		g_numscreenalignedbones++;

	} else
	{
		TokenError( "$screenalign: expected bone name\n" );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

void Cmd_BBox (void)
{
	GetToken (false);
	bbox[0][0] = verify_atof( token );

	GetToken (false);
	bbox[0][1] = verify_atof( token );

	GetToken (false);
	bbox[0][2] = verify_atof( token );

	GetToken (false);
	bbox[1][0] = verify_atof( token );

	GetToken (false);
	bbox[1][1] = verify_atof( token );

	GetToken (false);
	bbox[1][2] = verify_atof( token );

	g_wrotebbox = true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

void Cmd_CBox (void)
{
	GetToken (false);
	cbox[0][0] = verify_atof( token );

	GetToken (false);
	cbox[0][1] = verify_atof( token );

	GetToken (false);
	cbox[0][2] = verify_atof( token );

	GetToken (false);
	cbox[1][0] = verify_atof( token );

	GetToken (false);
	cbox[1][1] = verify_atof( token );

	GetToken (false);
	cbox[1][2] = verify_atof( token );

	g_wrotecbox = true;
}



//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

void Cmd_Gamma (void)
{
	GetToken (false);
	g_gamma = verify_atof( token );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

void Cmd_TextureGroup( )
{
	if( g_bCreateMakefile )
	{
		return;
	}
	int i;
	int depth = 0;
	int index = 0;
	int group = 0;

	if (g_numtextures == 0)
		TokenError( "texturegroups must follow model loading\n");

	if (!GetToken(false)) 
		return;

	if (g_numskinref == 0)
		g_numskinref = g_numtextures;

	while (1)
	{
		if(!GetToken(true)) 
		{
			break;
		}

		if (endofscript)
		{
			if (depth != 0)
			{
				TokenError("missing }\n" );
			}
			return;
		}
		if (token[0] == '{')
		{
			depth++;
		}
		else if (token[0] == '}')
		{
			depth--;
			if (depth == 0)
				break;
			group++;
			index = 0;
		}
		else if (depth == 2)
		{
			i = use_texture_as_material( lookup_texture( token, sizeof( token ) ) );
			g_texturegroup[g_numtexturegroups][group][index] = i;
			if (group != 0)
				g_texture[i].parent = g_texturegroup[g_numtexturegroups][0][index];
			index++;
			g_numtexturereps[g_numtexturegroups] = index;
			g_numtexturelayers[g_numtexturegroups] = group + 1;
		}
	}

	g_numtexturegroups++;
}



//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

void Cmd_Hitgroup( )
{
	GetToken (false);
	g_hitgroup[g_numhitgroups].group = verify_atoi( token );
	GetToken (false);
	strcpyn( g_hitgroup[g_numhitgroups].name, token );
	g_numhitgroups++;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

void Cmd_Hitbox( )
{
	bool autogenerated = false;
	if ( g_hitboxsets.Size() == 0 )
	{
		g_hitboxsets.AddToTail();
		autogenerated = true;
	}

	// Last one
	s_hitboxset *set = &g_hitboxsets[ g_hitboxsets.Size() - 1 ];
	if ( autogenerated )
	{
		memset( set, 0, sizeof( *set ) );

		// fill in name if it wasn't specified in the .qc
		strcpy( set->hitboxsetname, "default" );
	}

	GetToken (false);
	set->hitbox[set->numhitboxes].group = verify_atoi( token );
	
	// Grab the bone name:
	GetToken (false);
	strcpyn( set->hitbox[set->numhitboxes].name, token );

	GetToken (false);
	set->hitbox[set->numhitboxes].bmin[0] = verify_atof( token );
	GetToken (false);
	set->hitbox[set->numhitboxes].bmin[1] = verify_atof( token );
	GetToken (false);
	set->hitbox[set->numhitboxes].bmin[2] = verify_atof( token );
	GetToken (false);
	set->hitbox[set->numhitboxes].bmax[0] = verify_atof( token );
	GetToken (false);
	set->hitbox[set->numhitboxes].bmax[1] = verify_atof( token );
	GetToken (false);
	set->hitbox[set->numhitboxes].bmax[2] = verify_atof( token );

	//Scale hitboxes
	scale_vertex( set->hitbox[set->numhitboxes].bmin );
	scale_vertex( set->hitbox[set->numhitboxes].bmax );
	// clear out the hitboxname:
	memset( set->hitbox[set->numhitboxes].hitboxname, 0, sizeof( set->hitbox[set->numhitboxes].hitboxname ) );

	// Grab the hit box name if present:
	if( TokenAvailable() )
	{
		GetToken (false);
		strcpyn( set->hitbox[set->numhitboxes].hitboxname, token );
	}


	set->numhitboxes++;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

void Cmd_HitboxSet( void )
{
	// Add a new hitboxset
	s_hitboxset *set = &g_hitboxsets[ g_hitboxsets.AddToTail() ];
	GetToken( false );
	memset( set, 0, sizeof( *set ) );
	strcpy( set->hitboxsetname, token );
}


//-----------------------------------------------------------------------------
// Assigns a default surface property to the entire model
//-----------------------------------------------------------------------------
struct SurfacePropName_t
{
	char m_pJointName[128];
	char m_pSurfaceProp[128];
};

static char								s_pDefaultSurfaceProp[128] = {"default"};
static CUtlVector<SurfacePropName_t>	s_JointSurfaceProp;

//-----------------------------------------------------------------------------
// Assigns a default surface property to the entire model
//-----------------------------------------------------------------------------
void Cmd_SurfaceProp ()
{
	GetToken (false);
	strcpyn( s_pDefaultSurfaceProp, token );
}	


//-----------------------------------------------------------------------------
// Assigns a surface property to a particular joint
//-----------------------------------------------------------------------------
void Cmd_JointSurfaceProp ()
{
	// Get joint name...
	GetToken (false);

	// Search for the name in our list
	int i;
	for ( i = s_JointSurfaceProp.Count(); --i >= 0; )
	{
		if (!stricmp(s_JointSurfaceProp[i].m_pJointName, token))
		{
			break;
		}
	}

	// Add new entry if we haven't seen this name before
	if (i < 0)
	{
		i = s_JointSurfaceProp.AddToTail();
		strcpyn( s_JointSurfaceProp[i].m_pJointName, token );
	}

	// surface property name
	GetToken(false);
	strcpyn( s_JointSurfaceProp[i].m_pSurfaceProp, token );
}


//-----------------------------------------------------------------------------
// Returns the default surface prop name
//-----------------------------------------------------------------------------
char* GetDefaultSurfaceProp ( )
{
	return s_pDefaultSurfaceProp;
}


//-----------------------------------------------------------------------------
// Returns surface property for a given joint
//-----------------------------------------------------------------------------
static char* FindSurfaceProp ( char const* pJointName )
{
	for ( int i = s_JointSurfaceProp.Count(); --i >= 0; )
	{
		if (!stricmp(s_JointSurfaceProp[i].m_pJointName, pJointName))
		{
			return s_JointSurfaceProp[i].m_pSurfaceProp;
		}
	}

	return 0;
}


//-----------------------------------------------------------------------------
// Returns surface property for a given joint
//-----------------------------------------------------------------------------
char* GetSurfaceProp ( char const* pJointName )
{
	while( pJointName )
	{
		// First try to find this joint
		char* pSurfaceProp = FindSurfaceProp( pJointName );
		if (pSurfaceProp)
			return pSurfaceProp;

		// If we can't find the joint, then find it's parent...
		if (!g_numbones)
			return s_pDefaultSurfaceProp;

		int i = findGlobalBone( pJointName );

		if ((i >= 0) && (g_bonetable[i].parent >= 0))
		{
			pJointName = g_bonetable[g_bonetable[i].parent].name;
		}
		else
		{
			pJointName = 0;
		}
	}

	// No match, return the default one
	return s_pDefaultSurfaceProp;
}


//-----------------------------------------------------------------------------
// Returns surface property for a given joint
//-----------------------------------------------------------------------------
void ConsistencyCheckSurfaceProp ( )
{
	for ( int i = s_JointSurfaceProp.Count(); --i >= 0; )
	{
		int j = findGlobalBone( s_JointSurfaceProp[i].m_pJointName );

		if (j < 0)
		{
			MdlWarning("You specified a joint surface property for joint\n"
				"    \"%s\" which either doesn't exist or was optimized out.\n", s_JointSurfaceProp[i].m_pJointName );
		}
	}
}


//-----------------------------------------------------------------------------
// Assigns a default contents to the entire model
//-----------------------------------------------------------------------------
struct ContentsName_t
{
	char m_pJointName[128];
	int m_nContents;
};

static int s_nDefaultContents = CONTENTS_SOLID;
static CUtlVector<ContentsName_t>	s_JointContents;


//-----------------------------------------------------------------------------
// Parse contents flags
//-----------------------------------------------------------------------------
static void ParseContents( int *pAddFlags, int *pRemoveFlags )
{
	*pAddFlags = 0;
	*pRemoveFlags = 0;
	do 
	{
		GetToken (false);

		if ( !stricmp( token, "grate" ) )
		{
			*pAddFlags |= CONTENTS_GRATE;
			*pRemoveFlags |= CONTENTS_SOLID;
		}
		else if ( !stricmp( token, "ladder" ) )
		{
			*pAddFlags |= CONTENTS_LADDER;
		}
		else if ( !stricmp( token, "solid" ) )
		{
			*pAddFlags |= CONTENTS_SOLID;
		}
		else if ( !stricmp( token, "monster" ) )
		{
			*pAddFlags |= CONTENTS_MONSTER;
		}
		else if ( !stricmp( token, "notsolid" ) )
		{
			*pRemoveFlags |= CONTENTS_SOLID;
		}
	} while (TokenAvailable());
}


//-----------------------------------------------------------------------------
// Assigns a default contents to the entire model
//-----------------------------------------------------------------------------
void Cmd_Contents()
{
	int nAddFlags, nRemoveFlags;
	ParseContents( &nAddFlags, &nRemoveFlags );
	s_nDefaultContents |= nAddFlags;
	s_nDefaultContents &= ~nRemoveFlags;
}


//-----------------------------------------------------------------------------
// Assigns contents to a particular joint
//-----------------------------------------------------------------------------
void Cmd_JointContents ()
{
	// Get joint name...
	GetToken (false);

	// Search for the name in our list
	int i;
	for ( i = s_JointContents.Count(); --i >= 0; )
	{
		if (!stricmp(s_JointContents[i].m_pJointName, token))
		{
			break;
		}
	}

	// Add new entry if we haven't seen this name before
	if (i < 0)
	{
		i = s_JointContents.AddToTail();
		strcpyn( s_JointContents[i].m_pJointName, token );
	}

	int nAddFlags, nRemoveFlags;
	ParseContents( &nAddFlags, &nRemoveFlags );
	s_JointContents[i].m_nContents = CONTENTS_SOLID;
	s_JointContents[i].m_nContents |= nAddFlags;
	s_JointContents[i].m_nContents &= ~nRemoveFlags;
}


//-----------------------------------------------------------------------------
// Returns the default contents
//-----------------------------------------------------------------------------
int GetDefaultContents( )
{
	return s_nDefaultContents;
}


//-----------------------------------------------------------------------------
// Returns contents for a given joint
//-----------------------------------------------------------------------------
static int FindContents( char const* pJointName )
{
	for ( int i = s_JointContents.Count(); --i >= 0; )
	{
		if (!stricmp(s_JointContents[i].m_pJointName, pJointName))
		{
			return s_JointContents[i].m_nContents;
		}
	}

	return -1;
}


//-----------------------------------------------------------------------------
// Returns contents for a given joint
//-----------------------------------------------------------------------------
int GetContents( char const* pJointName )
{
	while( pJointName )
	{
		// First try to find this joint
		int nContents = FindContents( pJointName );
		if (nContents != -1)
			return nContents;

		// If we can't find the joint, then find it's parent...
		if (!g_numbones)
			return s_nDefaultContents;

		int i = findGlobalBone( pJointName );

		if ((i >= 0) && (g_bonetable[i].parent >= 0))
		{
			pJointName = g_bonetable[g_bonetable[i].parent].name;
		}
		else
		{
			pJointName = 0;
		}
	}

	// No match, return the default one
	return s_nDefaultContents;
}


//-----------------------------------------------------------------------------
// Checks specified contents
//-----------------------------------------------------------------------------
void ConsistencyCheckContents( )
{
	for ( int i = s_JointContents.Count(); --i >= 0; )
	{
		int j = findGlobalBone( s_JointContents[i].m_pJointName );

		if (j < 0)
		{
			MdlWarning("You specified a joint contents for joint\n"
				"    \"%s\" which either doesn't exist or was optimized out.\n", s_JointSurfaceProp[i].m_pJointName );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void Cmd_BoneMerge( )
{
	if( g_bCreateMakefile )
		return;

	int nIndex = g_BoneMerge.AddToTail();

	// bone name
	GetToken (false);
	strcpyn( g_BoneMerge[nIndex].bonename, token );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void Cmd_Attachment( )
{
	if( g_bCreateMakefile )
		return;

	// name
	GetToken (false);
	strcpyn( g_attachment[g_numattachments].name, token );

	// bone name
	GetToken (false);
	strcpyn( g_attachment[g_numattachments].bonename, token );

	Vector tmp;

	// position
	GetToken (false);
	tmp.x = verify_atof( token );
	GetToken (false);
	tmp.y = verify_atof( token );
	GetToken (false);
	tmp.z = verify_atof( token );

	scale_vertex( tmp );
	// identity matrix
	AngleMatrix( QAngle( 0, 0, 0 ), g_attachment[g_numattachments].local );

	while (TokenAvailable())
	{
		GetToken (false);

		if (stricmp(token,"absolute") == 0)
		{
			g_attachment[g_numattachments].type |= IS_ABSOLUTE;
			AngleIMatrix( g_defaultrotation, g_attachment[g_numattachments].local );
			// AngleIMatrix( Vector( 0, 0, 0 ), g_attachment[g_numattachments].local );
		}
		else if (stricmp(token,"rigid") == 0)
		{
			g_attachment[g_numattachments].type |= IS_RIGID;
		}
		else if (stricmp(token,"world_align") == 0)
		{
			g_attachment[g_numattachments].flags |= ATTACHMENT_FLAG_WORLD_ALIGN;
		}
		else if (stricmp(token,"rotate") == 0)
		{
			QAngle angles;
			for (int i = 0; i < 3; ++i)
			{
				if (!TokenAvailable())
					break;

				GetToken(false);
				angles[i] = verify_atof( token );
			}
			AngleMatrix( angles, g_attachment[g_numattachments].local );
		}
		else if (stricmp(token,"x_and_z_axes") == 0)
		{
			int i;
			Vector xaxis, yaxis, zaxis;
			for (i = 0; i < 3; ++i)
			{
				if (!TokenAvailable())
					break;

				GetToken(false);
				xaxis[i] = verify_atof( token );
			}
			for (i = 0; i < 3; ++i)
			{
				if (!TokenAvailable())
					break;

				GetToken(false);
				zaxis[i] = verify_atof( token );
			}
			VectorNormalize( xaxis );
			VectorMA( zaxis, -DotProduct( zaxis, xaxis ), xaxis, zaxis );
			VectorNormalize( zaxis );
			CrossProduct( zaxis, xaxis, yaxis );
			MatrixSetColumn( xaxis, 0, g_attachment[g_numattachments].local );
			MatrixSetColumn( yaxis, 1, g_attachment[g_numattachments].local );
			MatrixSetColumn( zaxis, 2, g_attachment[g_numattachments].local );
			MatrixSetColumn( vec3_origin, 3, g_attachment[g_numattachments].local );
		}
		else
		{
			TokenError("unknown attachment (%s) option: ", g_attachment[g_numattachments].name, token );
		}
	}

	g_attachment[g_numattachments].local[0][3] = tmp.x;
	g_attachment[g_numattachments].local[1][3] = tmp.y;
	g_attachment[g_numattachments].local[2][3] = tmp.z;

	g_numattachments++;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int LookupAttachment( char *name )
{
	int i;
	for (i = 0; i < g_numattachments; i++)
	{
		if (stricmp( g_attachment[i].name, name ) == 0)
		{
			return i;
		}
	}
	return -1;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void Cmd_Renamebone( )
{
	// from
	GetToken (false);
	strcpyn( g_renamedbone[g_numrenamedbones].from, token );

	// to
	GetToken (false);
	strcpyn( g_renamedbone[g_numrenamedbones].to, token );

	g_numrenamedbones++;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

void Cmd_Skiptransition( )
{
	int nskips = 0;
	int list[10];

	while (TokenAvailable())
	{
		GetToken (false);
		list[nskips++] = LookupXNode( token );
	}

	for (int i = 0; i < nskips; i++)
	{
		for (int j = 0; j < nskips; j++)
		{
			if (list[i] != list[j])
			{
				g_xnodeskip[g_numxnodeskips][0] = list[i];
				g_xnodeskip[g_numxnodeskips][1] = list[j];
				g_numxnodeskips++;
			}
		}
	}
}


//-----------------------------------------------------------------------------
//
// The following code is all related to LODs
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Parse replacemodel command, causes an LOD to use a new model
//-----------------------------------------------------------------------------

static void Cmd_ReplaceModel( LodScriptData_t& lodData )
{
	int i = lodData.modelReplacements.AddToTail();
	CLodScriptReplacement_t& newReplacement = lodData.modelReplacements[i];

	// from
	GetToken( false );

	// Strip off extensions for the source...
	char* pDot = strrchr( token, '.' );
	if (pDot)
		*pDot = 0;

	if (!FindCachedSource( token, "" ))
	{
		// must have prior knowledge of the from
		TokenError( "Unknown replace model '%s'\n", token );
	}

	newReplacement.SetSrcName( token );

	// to
	GetToken( false );
	newReplacement.SetDstName( token );

	// check for "reverse"
	bool reverse =  false;
	if( TokenAvailable() && GetToken( false ) )
	{
		if( stricmp( "reverse", token ) == 0 )
		{
			reverse = true;
		}
		else
		{
			TokenError( "\"%s\" unexpected\n", token );
		}
	}

	// If the LOD system tells us to replace "blank", let's forget
	// we ever read this. Have to do it here so parsing works
	if( !stricmp( newReplacement.GetSrcName(), "blank" ) )
	{
		lodData.modelReplacements.FastRemove( i );
		return;
	}
	
	// Load the source right here baby! That way its bones will get converted
	newReplacement.m_pSource = Load_Source( newReplacement.GetDstName(), 
		"smd", reverse, false ); 
}

//-----------------------------------------------------------------------------
// Parse removemodel command, causes an LOD to stop using a model
//-----------------------------------------------------------------------------

static void Cmd_RemoveModel( LodScriptData_t& lodData )
{
	int i = lodData.modelReplacements.AddToTail();
	CLodScriptReplacement_t& newReplacement = lodData.modelReplacements[i];

	// from
	GetToken( false );

	// Strip off extensions...
	char* pDot = strrchr( token, '.' );
	if (pDot)
		*pDot = 0;

	newReplacement.SetSrcName( token );

	// to
	newReplacement.SetDstName( "" );

	// If the LOD system tells us to replace "blank", let's forget
	// we ever read this. Have to do it here so parsing works
	if( !stricmp( newReplacement.GetSrcName(), "blank" ) )
	{
		lodData.modelReplacements.FastRemove( i );
	}
}

//-----------------------------------------------------------------------------
// Parse replacebone command, causes a part of an LOD model to use a different bone
//-----------------------------------------------------------------------------

static void Cmd_ReplaceBone( LodScriptData_t& lodData )
{
	int i = lodData.boneReplacements.AddToTail();
	CLodScriptReplacement_t& newReplacement = lodData.boneReplacements[i];

	// from
	GetToken( false );
	newReplacement.SetSrcName( token );

	// to
	GetToken( false );
	newReplacement.SetDstName( token );
}

//-----------------------------------------------------------------------------
// Parse bonetreecollapse command, causes the entire subtree to use the same bone as the node
//-----------------------------------------------------------------------------

static void Cmd_BoneTreeCollapse( LodScriptData_t& lodData )
{
	int i = lodData.boneTreeCollapses.AddToTail();
	CLodScriptReplacement_t& newCollapse = lodData.boneTreeCollapses[i];

	// from
	GetToken( false );
	newCollapse.SetSrcName( token );
}

//-----------------------------------------------------------------------------
// Parse replacematerial command, causes a material to be used in place of another
//-----------------------------------------------------------------------------

static void Cmd_ReplaceMaterial( LodScriptData_t& lodData )
{
	int i = lodData.materialReplacements.AddToTail();
	CLodScriptReplacement_t& newReplacement = lodData.materialReplacements[i];

	// from
	GetToken( false );
	newReplacement.SetSrcName( token );

	// to
	GetToken( false );
	newReplacement.SetDstName( token );

	// make sure it goes into the master list
	use_texture_as_material( lookup_texture( token, sizeof( token ) ) );
}

//-----------------------------------------------------------------------------
// Parse removemesh command, causes a mesh to not be used anymore
//-----------------------------------------------------------------------------

static void Cmd_RemoveMesh( LodScriptData_t& lodData )
{
	int i = lodData.meshRemovals.AddToTail();
	CLodScriptReplacement_t& newReplacement = lodData.meshRemovals[i];

	// from
	GetToken( false );
	Q_FixSlashes( token );
	newReplacement.SetSrcName( token );
}

static void Cmd_LOD( char const *cmdname )
{
	if ( gflags & STUDIOHDR_FLAGS_HASSHADOWLOD )
	{
		MdlError( "Model can only have one $shadowlod and it must be the last lod in the .qc (%d) : %s\n", g_iLinecount, g_szLine );
	}

	int i = g_ScriptLODs.AddToTail();
	LodScriptData_t& newLOD = g_ScriptLODs[i];

	if( g_ScriptLODs.Count() > MAX_NUM_LODS )
	{
		MdlError( "Too many LODs (MAX_NUM_LODS==%d)\n", ( int )MAX_NUM_LODS );
	}

	// Shadow lod reserves -1 as switch value
	// which uniquely identifies a shadow lod
	newLOD.switchValue = -1.0f;

	bool isShadowCall = ( !stricmp( cmdname, "$shadowlod" ) ) ? true : false;

	if ( isShadowCall )
	{
		if ( TokenAvailable() )
		{
			GetToken( false );
			MdlWarning( "(%d) : %s:  Ignoring switch value on %s command line\n", cmdname, g_iLinecount, g_szLine );
		}

		// Disable facial animation by default
		newLOD.EnableFacialAnimation( false );
	}
	else
	{
		if ( TokenAvailable() )
		{
			GetToken( false );
			newLOD.switchValue = verify_atof( token );
			if ( newLOD.switchValue < 0.0f )
			{
				MdlError( "Negative switch value reserved for $shadowlod (%d) : %s\n", g_iLinecount, g_szLine );
			}
		}
		else
		{
			MdlError( "Expected LOD switch value (%d) : %s\n", g_iLinecount, g_szLine );
		}
	}

	GetToken( true );
	if( stricmp( "{", token ) != 0 )
	{
		MdlError( "\"{\" expected while processing %s (%d) : %s", cmdname, g_iLinecount, g_szLine );
	}

	while( 1 )
	{
		GetToken( true );
		if( stricmp( "replacemodel", token ) == 0 )
		{
			Cmd_ReplaceModel(newLOD);
		}
		else if( stricmp( "removemodel", token ) == 0 )
		{
			Cmd_RemoveModel(newLOD);
		}
		else if( stricmp( "replacebone", token ) == 0 )
		{
			Cmd_ReplaceBone( newLOD );
		}
		else if( stricmp( "bonetreecollapse", token ) == 0 )
		{
			Cmd_BoneTreeCollapse( newLOD );
		}
		else if( stricmp( "replacematerial", token ) == 0 )
		{
			Cmd_ReplaceMaterial( newLOD );
		}
		else if( stricmp( "removemesh", token ) == 0 )
		{
			Cmd_RemoveMesh( newLOD );
		}
		else if( stricmp( "nofacial", token ) == 0 )
		{
			newLOD.EnableFacialAnimation( false );
		}
		else if( stricmp( "facial", token ) == 0 )
		{
			if (isShadowCall)
			{
				// facial animation has no reasonable purpose on a shadow lod
				TokenError( "Facial animation is not allowed for $shadowlod\n" );
			}

			newLOD.EnableFacialAnimation( true );
		}
		else if ( stricmp( "use_shadowlod_materials", token ) == 0 )
		{
			if (isShadowCall)
			{
				gflags |= STUDIOHDR_FLAGS_USE_SHADOWLOD_MATERIALS;
			}
		}
		else if( stricmp( "}", token ) == 0 )
		{
			break;
		}
		else
		{
			MdlError( "invalid input while processing %s (%d) : %s", cmdname, g_iLinecount, g_szLine );
		}
	}
}

void Cmd_ShadowLOD( void )
{
	if (!g_quiet)
	{
		printf( "Processing $shadowlod\n" );
	}

	// Act like it's a regular lod entry
	Cmd_LOD( "$shadowlod" );

	// Mark .mdl as having shadow lod (we also check above that we have only one of these
	// and that it's the last entered lod )
	gflags |= STUDIOHDR_FLAGS_HASSHADOWLOD;
}


//-----------------------------------------------------------------------------
// A couple commands related to translucency sorting
//-----------------------------------------------------------------------------
void Cmd_Opaque( )
{
	// Force Opaque has precedence
	gflags |= STUDIOHDR_FLAGS_FORCE_OPAQUE;
	gflags &= ~STUDIOHDR_FLAGS_TRANSLUCENT_TWOPASS;
}

void Cmd_TranslucentTwoPass( )
{
	// Force Opaque has precedence
	if ((gflags & STUDIOHDR_FLAGS_FORCE_OPAQUE) == 0)
	{
		gflags |= STUDIOHDR_FLAGS_TRANSLUCENT_TWOPASS;
	}
}

//-----------------------------------------------------------------------------
// Indicates the model be rendered with ambient boost heuristic (first used on Alyx in Episode 1)
//-----------------------------------------------------------------------------
void Cmd_AmbientBoost()
{
	gflags |= STUDIOHDR_FLAGS_AMBIENT_BOOST;
}

//-----------------------------------------------------------------------------
// Indicates the model should not fade out even if the level or fallback settings say to
//-----------------------------------------------------------------------------
void Cmd_NoForcedFade()
{
	gflags |= STUDIOHDR_FLAGS_NO_FORCED_FADE;
}


//-----------------------------------------------------------------------------
// Indicates the model should not use the bone origin when calculating bboxes, sequence boxes, etc.
//-----------------------------------------------------------------------------
void Cmd_SkipBoneInBBox()
{
	g_bUseBoneInBBox = false;
}


//-----------------------------------------------------------------------------
// Indicates the model will lengthen the viseme check to always include two phonemes
//-----------------------------------------------------------------------------
void Cmd_ForcePhonemeCrossfade()
{
	gflags |= STUDIOHDR_FLAGS_FORCE_PHONEME_CROSSFADE;
}

//-----------------------------------------------------------------------------
// Indicates the model should keep pre-defined bone lengths regardless of animation changes
//-----------------------------------------------------------------------------
void Cmd_LockBoneLengths()
{
	g_bLockBoneLengths = true;
}

//-----------------------------------------------------------------------------
// Indicates the model should keep pre-defined bone lengths regardless of animation changes
//-----------------------------------------------------------------------------
void Cmd_LockDefineBones()
{
	g_bOverridePreDefinedBones = false;
}

//-----------------------------------------------------------------------------
// Mark this model as obsolete so that it'll show the obsolete material in game.
//-----------------------------------------------------------------------------
void Cmd_Obsolete( )
{
	// Force Opaque has precedence
	gflags |= STUDIOHDR_FLAGS_OBSOLETE;
}



//-----------------------------------------------------------------------------
// Key value block!
//-----------------------------------------------------------------------------
void Option_KeyValues( CUtlVector< char > *pKeyValue )
{
	// Simply read in the block between { }s as text 
	// and plop it out unchanged into the .mdl file. 
	// Make sure to respect the fact that we may have nested {}s
	int nLevel = 1;

	if ( !GetToken( true ) )
		return;

	if ( token[0] != '{' )
		return;

	AppendKeyValueText( pKeyValue, "mdlkeyvalue\n{\n" );

	while ( GetToken(true) )
	{
		if ( !stricmp( token, "}" ) )
		{
			nLevel--;
			if ( nLevel <= 0 )
				break;
			AppendKeyValueText( pKeyValue, " }\n" );
		}
		else if ( !stricmp( token, "{" ) )
		{
			AppendKeyValueText( pKeyValue, "{\n" );
			nLevel++;
		}
		else
		{
			// tokens inside braces are quoted
			if ( nLevel > 1 )
			{
				AppendKeyValueText( pKeyValue, "\"" );
				AppendKeyValueText( pKeyValue, token );
				AppendKeyValueText( pKeyValue, "\" " );
			}
			else
			{
				AppendKeyValueText( pKeyValue, token );
				AppendKeyValueText( pKeyValue, " " );
			}
		}
	}

	if ( nLevel >= 1 )
	{
		TokenError( "Keyvalue block missing matching braces.\n" );
	}

	AppendKeyValueText( pKeyValue, "}\n" );
}



//-----------------------------------------------------------------------------
// Purpose: force a specific parent child relationship
//-----------------------------------------------------------------------------

void Cmd_ForcedHierarchy( )
{
	// child name
	GetToken (false);
	strcpyn( g_forcedhierarchy[g_numforcedhierarchy].childname, token );

	// parent name
	GetToken (false);
	strcpyn( g_forcedhierarchy[g_numforcedhierarchy].parentname, token );

	g_numforcedhierarchy++;
}


//-----------------------------------------------------------------------------
// Purpose: insert a virtual bone between a child and parent (currently unsupported)
//-----------------------------------------------------------------------------

void Cmd_InsertHierarchy( )
{
	// child name
	GetToken (false);
	strcpyn( g_forcedhierarchy[g_numforcedhierarchy].childname, token );

	// subparent name
	GetToken (false);
	strcpyn( g_forcedhierarchy[g_numforcedhierarchy].subparentname, token );

	// parent name
	GetToken (false);
	strcpyn( g_forcedhierarchy[g_numforcedhierarchy].parentname, token );

	g_numforcedhierarchy++;
}


//-----------------------------------------------------------------------------
// Purpose: rotate a specific bone
//-----------------------------------------------------------------------------

void Cmd_ForceRealign( )
{
	// bone name
	GetToken (false);
	strcpyn( g_forcedrealign[g_numforcedrealign].name, token );

	// skip
	GetToken (false);

	// X axis
	GetToken (false);
	g_forcedrealign[g_numforcedrealign].rot.x = DEG2RAD( verify_atof( token ) );

	// Y axis
	GetToken (false);
	g_forcedrealign[g_numforcedrealign].rot.y = DEG2RAD( verify_atof( token ) );

	// Z axis
	GetToken (false);
	g_forcedrealign[g_numforcedrealign].rot.z = DEG2RAD( verify_atof( token ) );

	g_numforcedrealign++;
}


//-----------------------------------------------------------------------------
// Purpose: specify a bone to allow > 180 but < 360 rotation (forces a calculated "mid point" to rotation)
//-----------------------------------------------------------------------------

void Cmd_LimitRotation( )
{
	// bone name
	GetToken (false);
	strcpyn( g_limitrotation[g_numlimitrotation].name, token );

	while (TokenAvailable())
	{
		// sequence name
		GetToken (false);
		strcpyn( g_limitrotation[g_numlimitrotation].sequencename[g_limitrotation[g_numlimitrotation].numseq++], token );
	}

	g_numlimitrotation++;
}


//-----------------------------------------------------------------------------
// Purpose: specify bones to store, even if nothing references them
//-----------------------------------------------------------------------------

void Cmd_DefineBone( )
{
	// bone name
	GetToken (false);
	strcpyn( g_importbone[g_numimportbones].name, token );

	// parent name
	GetToken (false);
	strcpyn( g_importbone[g_numimportbones].parent, token );

	Vector pos;
	QAngle angles;

	// default pos
	GetToken (false);
	pos.x = verify_atof( token );
	GetToken (false);
	pos.y = verify_atof( token );
	GetToken (false);
	pos.z = verify_atof( token );
	GetToken (false);
	angles.x = verify_atof( token );
	GetToken (false);
	angles.y = verify_atof( token );
	GetToken (false);
	angles.z = verify_atof( token );
	AngleMatrix( angles, pos, g_importbone[g_numimportbones].rawLocal );

	if (TokenAvailable())
	{
		g_importbone[g_numimportbones].bPreAligned = true;
		// realign pos
		GetToken (false);
		pos.x = verify_atof( token );
		GetToken (false);
		pos.y = verify_atof( token );
		GetToken (false);
		pos.z = verify_atof( token );
		GetToken (false);
		angles.x = verify_atof( token );
		GetToken (false);
		angles.y = verify_atof( token );
		GetToken (false);
		angles.z = verify_atof( token );

		AngleMatrix( angles, pos, g_importbone[g_numimportbones].srcRealign );
	}
	else
	{
		SetIdentityMatrix( g_importbone[g_numimportbones].srcRealign );
	}

	g_numimportbones++;
}



//-----------------------------------------------------------------------------
// Purpose: specify bones to store, even if nothing references them
//-----------------------------------------------------------------------------

void Cmd_IncludeModel( )
{
	GetToken( false );
	strcpyn( g_includemodel[g_numincludemodels].name, "models/" );
	strcat( g_includemodel[g_numincludemodels].name, token );
	g_numincludemodels++;
}


/*
=================
=================
*/

void Grab_Vertexanimation( s_source_t *psource )
{
	char	cmd[1024];
	int		index;
	Vector	pos;
	Vector	normal;
	int		t = -1;
	int		count = 0;
	static s_vertanim_t	tmpvanim[MAXSTUDIOVERTS*4];

	while (fgets( g_szLine, sizeof( g_szLine ), g_fpInput ) != NULL) 
	{
		g_iLinecount++;
		if (sscanf( g_szLine, "%d %f %f %f %f %f %f", &index, &pos[0], &pos[1], &pos[2], &normal[0], &normal[1], &normal[2] ) == 7)
		{
			if (psource->startframe < 0)
			{
				MdlError( "Missing frame start(%d) : %s", g_iLinecount, g_szLine );
			}

			if (t < 0)
			{
				MdlError( "VTA Frame Sync (%d) : %s", g_iLinecount, g_szLine );
			}

			tmpvanim[count].vertex = index;
			VectorCopy( pos, tmpvanim[count].pos );
			VectorCopy( normal, tmpvanim[count].normal );
			count++;

			if (index >= psource->numvertices)
				psource->numvertices = index + 1;
		}
		else
		{
			// flush data

			if (count)
			{
				psource->numvanims[t] = count;

				psource->vanim[t] = (s_vertanim_t *)kalloc( count, sizeof( s_vertanim_t ) );

				memcpy( psource->vanim[t], tmpvanim, count * sizeof( s_vertanim_t ) );
			}
			else if (t > 0)
			{
				psource->numvanims[t] = 0;
			}

			// next command
			if (sscanf( g_szLine, "%1023s %d", cmd, &index ))
			{
				if (stricmp( cmd, "time" ) == 0) 
				{
					t = index;
					count = 0;

					if (t < psource->startframe)
					{
						MdlError( "Frame MdlError(%d) : %s", g_iLinecount, g_szLine );
					}
					if (t > psource->endframe)
					{
						MdlError( "Frame MdlError(%d) : %s", g_iLinecount, g_szLine );
					}

					t -= psource->startframe;
				}
				else if (stricmp( cmd, "end") == 0) 
				{
					psource->numframes = psource->endframe - psource->startframe + 1;
					return;
				}
				else
				{
					MdlError( "MdlError(%d) : %s", g_iLinecount, g_szLine );
				}

			}
			else
			{
				MdlError( "MdlError(%d) : %s", g_iLinecount, g_szLine );
			}
		}
	}
	MdlError( "unexpected EOF: %s\n", psource->filename );
}

int OpenGlobalFile( char *src )
{
	int		time1;
	char	filename[1024];

	strcpy( filename, ExpandPath( src ) );

	int pathLength;
	int numBasePaths = CmdLib_GetNumBasePaths();
	// This is kinda gross. . . doing the same work in cmdlib on SafeOpenRead.
	if( CmdLib_HasBasePath( filename, pathLength ) )
	{
		char tmp[1024];
		int i;
		for( i = 0; i < numBasePaths; i++ )
		{
			strcpy( tmp, CmdLib_GetBasePath( i ) );
			strcat( tmp, filename + pathLength );
			if( g_bCreateMakefile )
			{
				CreateMakefile_AddDependency( tmp );
				return 0;
			}
			
			time1 = FileTime( tmp );
			if( time1 != -1 )
			{
				if ((g_fpInput = fopen(tmp, "r")) == 0) 
				{
					MdlWarning( "reader: could not open file '%s'\n", src );
					return 0;
				}
				else
				{
					return 1;
				}
			}
		}
		return 0;
	}
	else
	{
		time1 = FileTime (filename);
		if (time1 == -1)
			return 0;

		if( g_bCreateMakefile )
		{
			CreateMakefile_AddDependency( filename );
			return 0;
		}
		if ((g_fpInput = fopen(filename, "r")) == 0) 
		{
			MdlWarning( "reader: could not open file '%s'\n", src );
			return 0;
		}

		return 1;
	}
}



int Load_VTA( s_source_t *psource )
{
	char	cmd[1024];
	int		option;

	if (!OpenGlobalFile( psource->filename ))
		return 0;

	if (!g_quiet)
		printf ("VTA MODEL %s\n", psource->filename);

	g_iLinecount = 0;
	while (fgets( g_szLine, sizeof( g_szLine ), g_fpInput ) != NULL) 
	{
		g_iLinecount++;
		sscanf( g_szLine, "%s %d", cmd, &option );
		if (stricmp( cmd, "version" ) == 0) 
		{
			if (option != 1) 
			{
				MdlError("bad version\n");
			}
		}
		else if (stricmp( cmd, "nodes" ) == 0) 
		{
			psource->numbones = Grab_Nodes( psource->localBone );
		}
		else if (stricmp( cmd, "skeleton" ) == 0) 
		{
			Grab_Animation( psource );
		}
		else if (stricmp( cmd, "vertexanimation" ) == 0) 
		{
			Grab_Vertexanimation( psource );
		}
		else 
		{
			MdlWarning("unknown studio command \"%s\"\n", cmd );
		}
	}
	fclose( g_fpInput );

	is_v1support = true;

	return 1;
}


void Grab_AxisInterpBones( )
{
	char	cmd[1024], tmp[1025];
	Vector	basepos;
	s_axisinterpbone_t *pAxis = NULL;
	s_axisinterpbone_t *pBone = &g_axisinterpbones[g_numaxisinterpbones];

	while (fgets( g_szLine, sizeof( g_szLine ), g_fpInput ) != NULL) 
	{
		g_iLinecount++;
		if (IsEnd( g_szLine )) 
		{
			return;
		}
		int i = sscanf( g_szLine, "%1023s \"%[^\"]\" \"%[^\"]\" \"%[^\"]\" \"%[^\"]\" %d", cmd, pBone->bonename, tmp, pBone->controlname, tmp, &pBone->axis );
		if (i == 6 && stricmp( cmd, "bone") == 0)
		{
			// printf( "\"%s\" \"%s\" \"%s\" \"%s\"\n", cmd, pBone->bonename, tmp, pBone->controlname );
			pAxis = pBone;
			pBone->axis = pBone->axis - 1;	// MAX uses 1..3, engine 0..2
			g_numaxisinterpbones++;
			pBone = &g_axisinterpbones[g_numaxisinterpbones];
		}
		else if (stricmp( cmd, "display" ) == 0)
		{
			// skip all display info
		}
		else if (stricmp( cmd, "type" ) == 0)
		{
			// skip all type info
		}
		else if (stricmp( cmd, "basepos" ) == 0)
		{
			i = sscanf( g_szLine, "basepos %f %f %f", &basepos.x, &basepos.y, &basepos.z );
			// skip all type info
		}
		else if (stricmp( cmd, "axis" ) == 0)
		{
			Vector pos;
			QAngle rot;
			int j;
			i = sscanf( g_szLine, "axis %d %f %f %f %f %f %f", &j, &pos[0], &pos[1], &pos[2], &rot[2], &rot[0], &rot[1] );
			if (i == 7)
			{
				VectorAdd( basepos, pos, pAxis->pos[j] );
				AngleQuaternion( rot, pAxis->quat[j] );
			}
		}
	}
}


bool Grab_AimAtBones( )
{
	s_aimatbone_t *pAimAtBone( &g_aimatbones[g_numaimatbones] );

	// Already know it's <aimconstraint> in the first string, otherwise wouldn't be here
	if ( sscanf( g_szLine, "%*s %127s %127s %127s", pAimAtBone->bonename, pAimAtBone->parentname, pAimAtBone->aimname ) == 3 )
	{
		g_numaimatbones++;

		char	cmd[1024];
		Vector	vector;

		while ( fgets( g_szLine, sizeof( g_szLine ), g_fpInput ) != NULL) 
		{
			g_iLinecount++;

			if (IsEnd( g_szLine )) 
			{
				return false;
			}

			if ( sscanf( g_szLine, "%1024s %f %f %f", cmd, &vector[0], &vector[1], &vector[2] ) != 4 )
			{
				// Allow blank lines to be skipped without error
				bool allSpace( true );
				for ( const char *pC( g_szLine ); *pC != '\0' && pC < ( g_szLine + 4096 ); ++pC )
				{
					if ( !isspace( *pC ) )
					{
						allSpace = false;
						break;
					}
				}

				if ( allSpace )
				{
					continue;
				}

				return true;
			}

			if ( stricmp( cmd, "<aimvector>" ) == 0)
			{
				// Make sure these are unit length on read
				VectorNormalize( vector );
				pAimAtBone->aimvector = vector;
			}
			else if ( stricmp( cmd, "<upvector>" ) == 0)
			{
				// Make sure these are unit length on read
				VectorNormalize( vector );
				pAimAtBone->upvector = vector;
			}
			else if ( stricmp( cmd, "<basepos>" ) == 0)
			{
				pAimAtBone->basepos = vector;
			}
			else
			{
				return true;
			}
		}
	}

	// If we get here, we're at EOF
	return false;
}


void Grab_QuatInterpBones( )
{
	char	cmd[1024];
	Vector	basepos;
	s_quatinterpbone_t *pAxis = NULL;
	s_quatinterpbone_t *pBone = &g_quatinterpbones[g_numquatinterpbones];

	while (fgets( g_szLine, sizeof( g_szLine ), g_fpInput ) != NULL) 
	{
		g_iLinecount++;
		if (IsEnd( g_szLine )) 
		{
			return;
		}

		int i = sscanf( g_szLine, "%s %s %s %s %s", cmd, pBone->bonename, pBone->parentname, pBone->controlparentname, pBone->controlname );

		while ( i == 4 && stricmp( cmd, "<aimconstraint>" ) == 0 )
		{
			// If Grab_AimAtBones() returns false, there file is at EOF
			if ( !Grab_AimAtBones() )
			{
				return;
			}

			// Grab_AimAtBones will read input into g_szLine same as here until it gets a line it doesn't understand, at which point
			// it will exit leaving that line in g_szLine, so check for the end and scan the current buffer again and continue on with 
			// the normal QuatInterpBones process

			i = sscanf( g_szLine, "%s %s %s %s %s", cmd, pBone->bonename, pBone->parentname, pBone->controlparentname, pBone->controlname );
		}

		if (i == 5 && stricmp( cmd, "<helper>") == 0)
		{
			// printf( "\"%s\" \"%s\" \"%s\" \"%s\"\n", cmd, pBone->bonename, tmp, pBone->controlname );
			pAxis = pBone;
			g_numquatinterpbones++;
			pBone = &g_quatinterpbones[g_numquatinterpbones];
		}
		else if ( i > 0 )
		{
			// There was a bug before which could cause the same command to be parsed twice
			// because if the sscanf above completely fails, it will return 0 and not 
			// change the contents of cmd, so i should be greater than 0 in order for
			// any of these checks to be valid... Still kind of buggy as these checks
			// do case insensitive stricmp but then the sscanf does case sensitive
			// matching afterwards... Should probably change those to
			// sscanf( g_szLine, "%*s %f ... ) etc...

			if ( stricmp( cmd, "<display>" ) == 0)
			{
				// skip all display info
				Vector size;
				float distance;

				i = sscanf( g_szLine, "<display> %f %f %f %f", 
					&size[0], &size[1], &size[2],
					&distance );

				if (i == 4)
				{
					pAxis->percentage = distance / 100.0;
					pAxis->size = size;
				}
				else
				{
					MdlError( "Line %d: Unable to parse procedual <display> bone: %s", g_iLinecount, g_szLine );
				}
			}
			else if ( stricmp( cmd, "<basepos>" ) == 0)
			{
				i = sscanf( g_szLine, "<basepos> %f %f %f", &basepos.x, &basepos.y, &basepos.z );
				// skip all type info
			}
			else if ( stricmp( cmd, "<trigger>" ) == 0)
			{
				float tolerance;
				RadianEuler trigger;
				Vector pos;
				RadianEuler ang;

				QAngle rot;
				int j;
				i = sscanf( g_szLine, "<trigger> %f %f %f %f %f %f %f %f %f %f", 
					&tolerance,
					&trigger.x, &trigger.y, &trigger.z,
					&ang.x, &ang.y, &ang.z,
					&pos.x, &pos.y, &pos.z );

				if (i == 10)
				{
					trigger.x = DEG2RAD( trigger.x );
					trigger.y = DEG2RAD( trigger.y );
					trigger.z = DEG2RAD( trigger.z );
					ang.x = DEG2RAD( ang.x );
					ang.y = DEG2RAD( ang.y );
					ang.z = DEG2RAD( ang.z );

					j = pAxis->numtriggers++;
					pAxis->tolerance[j] = DEG2RAD( tolerance );
					AngleQuaternion( trigger, pAxis->trigger[j] );
					VectorAdd( basepos, pos, pAxis->pos[j] );
					AngleQuaternion( ang, pAxis->quat[j] );
				}
				else
				{
					MdlError( "Line %d: Unable to parse procedual <trigger> bone: %s", g_iLinecount, g_szLine );
				}
			}
			else
			{
				MdlError( "Line %d: Unable to parse procedual bone data: %s", g_iLinecount, g_szLine );
			}
		}
		else
		{
			// Allow blank lines to be skipped without error
			bool allSpace( true );
			for ( const char *pC( g_szLine ); *pC != '\0' && pC < ( g_szLine + 4096 ); ++pC )
			{
				if ( !isspace( *pC ) )
				{
					allSpace = false;
					break;
				}
			}

			if ( !allSpace )
			{
				MdlError( "Line %d: Unable to parse procedual bone data: %s", g_iLinecount, g_szLine );
			}
		}
	}
}


void Load_ProceduralBones( )
{
	char	filename[256];
	char	cmd[1024];
	int		option;

	GetToken( false );
	strcpy( filename, token );

	if (!OpenGlobalFile( filename ))
		return;

	g_iLinecount = 0;

	char ext[32];
	Q_ExtractFileExtension( filename, ext, sizeof( ext ) );

	if (stricmp( ext, "vrd") == 0)
	{
		Grab_QuatInterpBones( );
	}
	else
	{
		while (fgets( g_szLine, sizeof( g_szLine ), g_fpInput ) != NULL) 
		{
			g_iLinecount++;
			sscanf( g_szLine, "%s", cmd, &option );
			if (stricmp( cmd, "version" ) == 0) 
			{
				if (option != 1) 
				{
					MdlError("bad version\n");
				}
			}
			else if (stricmp( cmd, "proceduralbones" ) == 0) 
			{
				Grab_AxisInterpBones( );
			}
		}
	}
	fclose( g_fpInput );
}


void Cmd_CD()
{
	if (cdset)
		MdlError ("Two $cd in one model");
	cdset = true;
	GetToken (false);
	strcpy (cddir[0], token);
	strcat (cddir[0], "/" );
	numdirs = 0;
}


void Cmd_CDMaterials()
{
	while (TokenAvailable())
	{
		GetToken (false);
		
		char szPath[512];
		Q_strncpy( szPath, token, sizeof( szPath ) );

		int len = strlen( szPath );
		if ( len > 0 && szPath[len-1] != '/' && szPath[len-1] != '\\' )
		{
			Q_strncat( szPath, "/", sizeof( szPath ), COPY_ALL_CHARACTERS );
		}

		Q_FixSlashes( szPath );
		cdtextures[numcdtextures] = strdup( szPath );
		numcdtextures++;
	}
}


void Cmd_Pushd()
{
	GetToken(false);

	strcpy( cddir[numdirs+1], cddir[numdirs] );
	strcat( cddir[numdirs+1], token );
	strcat( cddir[numdirs+1], "/" );
	numdirs++;
}

void Cmd_Popd()
{
	if (numdirs > 0)
		numdirs--;
}

void Cmd_CollisionModel()
{
	DoCollisionModel( false );
}

void Cmd_CollisionJoints()
{
	DoCollisionModel( true );
}

void Cmd_ExternalTextures()
{
	MdlWarning( "ignoring $externaltextures, obsolete..." );
}

void Cmd_ClipToTextures()
{
	clip_texcoords = 1;
}

void Cmd_CollapseBones()
{
	g_collapse_bones = true;
}

void Cmd_AlwaysCollapse()
{
	g_collapse_bones = true;
	GetToken(false);
	g_collapse[g_numcollapse++] = strdup( token );
}

void Cmd_CalcTransitions()
{
	g_bMultistageGraph = true;
}

void Cmd_StaticProp()
{
	g_staticprop = true;
	gflags |= STUDIOHDR_FLAGS_STATIC_PROP;
}

void Cmd_ZBrush()
{
	g_bZBrush = true;
}

void Cmd_RealignBones()
{
	g_realignbones = true;
}

void Cmd_BaseLOD()
{
	Cmd_LOD( "$lod" );
}

void Cmd_KeyValues()
{
	Option_KeyValues( &g_KeyValueText );
}

void Cmd_ConstDirectionalLight()
{
	gflags |= STUDIOHDR_FLAGS_CONSTANT_DIRECTIONAL_LIGHT_DOT;

	GetToken (false);
	g_constdirectionalightdot = (byte)( verify_atof(token) * 255.0f );
}

void Cmd_MinLOD()
{
	GetToken( false );
	g_minLod = atoi( token );
}


void Cmd_BoneSaveFrame( )
{
	s_bonesaveframe_t tmp;

	// bone name
	GetToken( false );
	strcpyn( tmp.name, token );

	tmp.bSavePos = false;
	tmp.bSaveRot = false;
	while (TokenAvailable(  ))
	{
		GetToken( false );
		if (stricmp( "position", token ) == 0)
		{
			tmp.bSavePos = true;
		}
		else if (stricmp( "rotation", token ) == 0)
		{
			tmp.bSaveRot = true;
		}
		else
		{
			MdlError( "unknown option \"%s\" on $bonesaveframe : %s\n", token, tmp.name );
		}
	}

	g_bonesaveframe.AddToTail( tmp );
}


//
// This is the master list of the commands a QC file supports.
// To add a new command to the QC files, add it here.
//
struct
{
	char *m_pName;
	void (*m_pCmd)();
} g_Commands[] =
{
	{ "$cd", Cmd_CD },
	{ "$modelname", Cmd_Modelname },
	{ "$cdmaterials", Cmd_CDMaterials },
	{ "$pushd", Cmd_Pushd },
	{ "$popd", Cmd_Popd },
	{ "$scale", Cmd_ScaleUp },
	{ "$root", Cmd_Root },
	{ "$controller", Cmd_Controller },
	{ "$screenalign", Cmd_ScreenAlign },
	{ "$model", Cmd_Model },
	{ "$collisionmodel", Cmd_CollisionModel },
	{ "$collisionjoints", Cmd_CollisionJoints },
	{ "$collisiontext", Cmd_CollisionText },
	{ "$body", Cmd_Body },
	{ "$bodygroup", Cmd_Bodygroup },
	{ "$animation", Cmd_Animation },
	{ "$autocenter", Cmd_Autocenter },
	{ "$sequence", Cmd_Sequence },
	{ "$append", Cmd_Append },
	{ "$prepend", Cmd_Prepend  },
	{ "$continue", Cmd_Continue  },
	{ "$declaresequence", Cmd_DeclareSequence  },
	{ "$declareanimation", Cmd_DeclareAnimation },
	{ "$cmdlist", Cmd_Cmdlist },
	{ "$animblocksize", Cmd_AnimBlockSize },
	{ "$weightlist", Cmd_Weightlist },
	{ "$defaultweightlist", Cmd_DefaultWeightlist },
	{ "$ikchain", Cmd_IKChain },
	{ "$ikautoplaylock", Cmd_IKAutoplayLock },
	{ "$eyeposition", Cmd_Eyeposition },
	{ "$illumposition", Cmd_Illumposition },
	{ "$origin", Cmd_Origin },
	{ "$upaxis", Cmd_UpAxis },
	{ "$bbox", Cmd_BBox },
	{ "$cbox", Cmd_CBox },
	{ "$gamma", Cmd_Gamma },
	{ "$texturegroup", Cmd_TextureGroup },
	{ "$hgroup", Cmd_Hitgroup },
	{ "$hbox", Cmd_Hitbox },
	{ "$hboxset", Cmd_HitboxSet },
	{ "$surfaceprop", Cmd_SurfaceProp },
	{ "$jointsurfaceprop", Cmd_JointSurfaceProp },
	{ "$contents", Cmd_Contents },
	{ "$jointcontents", Cmd_JointContents },
	{ "$attachment", Cmd_Attachment },
	{ "$bonemerge", Cmd_BoneMerge },
	{ "$externaltextures", Cmd_ExternalTextures },
	{ "$cliptotextures", Cmd_ClipToTextures },
	{ "$renamebone", Cmd_Renamebone },
	{ "$collapsebones", Cmd_CollapseBones },
	{ "$alwayscollapse", Cmd_AlwaysCollapse },
	{ "$proceduralbones", Load_ProceduralBones },
	{ "$skiptransition", Cmd_Skiptransition },
	{ "$calctransitions", Cmd_CalcTransitions },
	{ "$staticprop", Cmd_StaticProp },
	{ "$zbrush", Cmd_ZBrush },
	{ "$realignbones", Cmd_RealignBones },
	{ "$forcerealign", Cmd_ForceRealign },
	{ "$lod", Cmd_BaseLOD },
	{ "$shadowlod", Cmd_ShadowLOD },
	{ "$poseparameter", Cmd_PoseParameter },
	{ "$heirarchy", Cmd_ForcedHierarchy },
	{ "$hierarchy", Cmd_ForcedHierarchy },
	{ "$insertbone", Cmd_InsertHierarchy },
	{ "$limitrotation", Cmd_LimitRotation },
	{ "$definebone", Cmd_DefineBone },
	{ "$includemodel", Cmd_IncludeModel },
	{ "$opaque", Cmd_Opaque },
	{ "$mostlyopaque", Cmd_TranslucentTwoPass },
//	{ "$platform", Cmd_Platform },
	{ "$keyvalues", Cmd_KeyValues },
	{ "$obsolete", Cmd_Obsolete },
	{ "$renamematerial", Cmd_RenameMaterial },
	{ "$fakevta", Cmd_FakeVTA },
	{ "$noforcedfade", Cmd_NoForcedFade },
	{ "$skipboneinbbox", Cmd_SkipBoneInBBox },
	{ "$forcephonemecrossfade", Cmd_ForcePhonemeCrossfade },
	{ "$lockbonelengths", Cmd_LockBoneLengths },
	{ "$lockdefinebones", Cmd_LockDefineBones },
	{ "$constantdirectionallight", Cmd_ConstDirectionalLight },
	{ "$minlod", Cmd_MinLOD },
	{ "$bonesaveframe", Cmd_BoneSaveFrame },
	{ "$ambientboost", Cmd_AmbientBoost }
};
	

/*
===============
ParseScript
===============
*/
void ParseScript (void)
{
	while (1)
	{
		GetToken (true);
		if (endofscript)
			return;

		// Check all the commands we know about.
		int i;
		for ( i=0; i < ARRAYSIZE( g_Commands ); i++ )
		{
			if ( !stricmp( g_Commands[i].m_pName, token ) )
			{
				g_Commands[i].m_pCmd();
				break;
			}
		}
		if ( i == ARRAYSIZE( g_Commands ) )
		{
			if( !g_bCreateMakefile )
			{
				TokenError("bad command %s\n", token);
			}
		}
	}
}


// Used by the CheckSurfaceProps.py script.
// They specify the .mdl file and it prints out all the surface props that the model uses.
bool HandlePrintSurfaceProps( int &returnValue )
{
	const char *pFilename = CommandLine()->ParmValue( "-PrintSurfaceProps", (const char*)NULL );
	if ( pFilename )
	{
		CUtlVector<char> buf;

		FILE *fp = fopen( pFilename, "rb" );
		if ( fp )
		{
			fseek( fp, 0, SEEK_END );
			buf.SetSize( ftell( fp ) );
			fseek( fp, 0, SEEK_SET );
			fread( buf.Base(), 1, buf.Count(), fp );

			fclose( fp );

			studiohdr_t *pHdr = (studiohdr_t*)buf.Base();

			Studio_ConvertStudioHdrToNewVersion( pHdr );

			if ( pHdr->version == STUDIO_VERSION )
			{
				for ( int i=0; i < pHdr->numbones; i++ )
				{
					mstudiobone_t *pBone = pHdr->pBone( i );
					printf( "%s\n", pBone->pszSurfaceProp() );
				}

				returnValue = 0;
			}
			else
			{
				printf( "-PrintSurfaceProps: '%s' is wrong version (%d should be %d).\n", 
					pFilename, pHdr->version, STUDIO_VERSION );
				returnValue = 1;
			}
		}
		else
		{
			printf( "-PrintSurfaceProps: can't open '%s'\n", pFilename );
			returnValue = 1;
		}

		return true;
	}
	else
	{
		return false;
	}
}

void UsageAndExit()
{
	MdlError( "Bad or missing options\n"
		"usage: studiomdl [options] <file.qc>\n"
		"options:\n"
		"[-a <normal_blend_angle>]\n"
		"[-checklengths]\n"
		"[-d] - dump glview files\n"
		"[-definebones]\n"
		"[-f] - flip all triangles\n"
		"[-fullcollide] - don't truncate really big collisionmodels\n"
		"[-game <gamedir>]\n"
		"[-h] - dump hboxes\n"
		"[-i] - ignore warnings\n"
		"[-minlod <lod>] - truncate to highest detail <lod>\n"
		"[-n] - tag bad normals\n"
		"[-perf]\n"
		"[-printbones]\n"
		"[-printgraph]\n"
		"[-quiet] - operate silently\n"
		"[-r] - tag reversed\n"
		"[-t <texture>]\n"
		"[-xbox] - enable xbox processing(default)\n"
		"[-notxbox] - disable xbox processing\n"
		"[-nowarnings] - disable warnings\n"
		);
}

#ifndef _DEBUG

LONG __stdcall VExceptionFilter( struct _EXCEPTION_POINTERS *ExceptionInfo )
{
	MdlExceptionFilter( ExceptionInfo->ExceptionRecord->ExceptionCode );
	return EXCEPTION_EXECUTE_HANDLER; // (never gets here anyway)
}

#endif
/*
==============
main
==============
*/

int main (int argc, char **argv)
{
	int		i;

#ifndef _DEBUG
	LPTOP_LEVEL_EXCEPTION_FILTER pOldFilter = SetUnhandledExceptionFilter( VExceptionFilter );
#endif

	CommandLine()->CreateCmdLine( argc, argv );

	InstallSpewFunction();
	MathLib_Init( 2.2f, 2.2f, 0.0f, 2.0f, false, false, false, false );
	

	int returnValue;	
	if ( HandlePrintSurfaceProps( returnValue ) )
		return returnValue;

	
	g_currentscale = g_defaultscale = 1.0;
	g_defaultrotation = RadianEuler( 0, 0, M_PI / 2 );

	// skip weightlist 0
	g_numweightlist = 1;

	eyeposition = Vector( 0, 0, 0 );
	gflags = 0;
	numrep = 0;
	tag_reversed = 0;
	tag_normals = 0;
	flip_triangles = 1;

	normal_blend = cos( DEG2RAD( 2.0 ));

	g_gamma = 2.2;

	g_staticprop = false;
	g_centerstaticprop = false;

	g_realignbones = false;
	g_constdirectionalightdot = 0;

	if (argc == 1)
	{
		UsageAndExit();
	}
	
	g_bDumpGLViewFiles = false;
	g_quiet = false;		  
	for (i = 1; i < argc - 1; i++) 
	{
		if (argv[i][0] == '-') 
		{
			if (!stricmp(argv[i], "-allowdebug"))
			{
				// Ignore, used by interface system to catch debug builds checked into release tree
				continue;
			}

			if (!stricmp(argv[i], "-ihvtest"))
			{
				++i;
				g_IHVTest = true;
				continue;
			}

			if (!stricmp(argv[i], "-quiet"))
			{
				g_quiet = true;
				g_verbose = false;
				continue;
			}

			if (!stricmp(argv[i], "-verbose"))
			{
				g_quiet = false;
				g_verbose = true;
				continue;
			}

			if (!stricmp(argv[i], "-fullcollide"))
			{
				g_badCollide = true;
				continue;
			}

			if (!stricmp(argv[i], "-checklengths"))
			{
				g_bCheckLengths = true;
				continue;
			}

			if (!stricmp(argv[i], "-printbones"))
			{
				g_bPrintBones = true;
				continue;
			}

			if (!stricmp(argv[i], "-perf"))
			{
				g_bPerf = true;
				continue;
			}

			if (!stricmp(argv[i], "-printgraph"))
			{
				g_bDumpGraph = true;
				continue;
			}

			if (!stricmp(argv[i], "-definebones"))
			{
				g_definebones = true;
				continue;
			}

			if (!stricmp(argv[i], "-makefile"))
			{
				g_bCreateMakefile = true;
				g_quiet = true;
				continue;
			}

			if (!stricmp(argv[i], "-verify"))
			{
				g_bVerifyOnly = true;
				continue;
			}

			if (!stricmp(argv[i], "-minlod"))
			{
				g_minLod = atoi( argv[++i] );
				continue;
			}

			if (!stricmp(argv[i], "-xbox"))
			{
				g_bXbox  = true;
				g_minLod = 2;
				continue;
			}

			if (!stricmp(argv[i], "-notxbox"))
			{
				g_bXbox  = false;
				g_minLod = 0;
				continue;
			}

			if (!stricmp(argv[i], "-nowarnings"))
			{
				g_bNoWarnings = true;
				continue;
			}

			if (argv[i][1] && argv[i][2] == '\0')
			{
				switch( argv[i][1] )
				{
				case 't':
					i++;
					strcpy ( defaulttexture[numrep], argv[i]);
					if (i < argc - 2 && argv[i + 1][0] != '-') {
						i++;
						strcpy ( sourcetexture[numrep], argv[i]);
						printf ("Replacing %s with %s\n", sourcetexture[numrep], defaulttexture[numrep] );
					}
					printf ("Using default texture: %s\n", defaulttexture);
					numrep++;
					break;
				case 'r':
					tag_reversed = 1;
					break;
				case 'n':
					tag_normals = 1;
					break;
				case 'f':
					flip_triangles = 0;
					break;
				case 'a':
					i++;
					normal_blend = cos( DEG2RAD( verify_atof( argv[i] ) ) );
					break;
				case 'h':
					dump_hboxes = 1;
					break;
				case 'i':
					ignore_warnings = 1;
					break;
				case 'd':
					g_bDumpGLViewFiles = true;
					break;
//				case 'p':
//					i++;
//					strcpy( qproject, argv[i] );
//					break;
				}
			}
		}
	}	

	if (i >= argc)
	{
		// misformed arguments
		// otherwise generating unintended results
		UsageAndExit();
	}
	
	strcpy( g_path, argv[i] );

	CmdLib_InitFileSystem( g_path );

	Q_FileBase( g_path, g_path, sizeof( g_path ) );

	// look for the "content\hl2x" string in the qdir and add what should be the correct path as an alternate
	// FIXME: add these to an envvar if folks are using complicated directory mappings instead of defaults
	char *match = "content\\hl2x\\";
	char *sp = strstr( qdir, match );
	if (sp)
	{
		char temp[1024];
		strncpy( temp, qdir, sp - qdir + strlen( match ) );
		temp[sp - qdir + strlen( match )] = '\0';
		CmdLib_AddBasePath( temp );
		strcat( temp, "..\\..\\..\\..\\main\\content\\hl2\\" );
		CmdLib_AddBasePath( temp );
	}

	if (!g_quiet)
	{
		printf("qdir:    \"%s\"\n", qdir );
		printf("gamedir: \"%s\"\n", gamedir );
		printf("g_path:  \"%s\"\n", g_path );
	}

	// load the script
	Q_DefaultExtension(g_path, ".qc", sizeof( g_path ) );
	if (!g_quiet)
	{
		printf("Working on \"%s\"\n", g_path);
	}
	LoadScriptFile(g_path);

	strcpy( fullpath, g_path );
	strcpy( fullpath, ExpandPath( fullpath ) );
	strcpy( fullpath, ExpandArg( fullpath ) );
	CreateMakefile_AddDependency( fullpath );
	
	// default to having one entry in the LOD list that doesn't do anything so
	// that we don't have to do any special cases for the first LOD.
	g_ScriptLODs.Purge();
	g_ScriptLODs.AddToTail(); // add an empty one
	g_ScriptLODs[0].switchValue = 0.0f;
	
	//
	// parse it
	//
	ClearModel();
	Q_StripExtension( argv[i], outname, sizeof( outname ) );

//	strcpy( g_pPlatformName, "" );
	
	ParseScript();
	
	if ( !g_bCreateMakefile )
	{
		SetSkinValues();

		SimplifyModel();

		ConsistencyCheckSurfaceProp();
		ConsistencyCheckContents();

		CollisionModel_Build();

		// ValidateSharedAnimationGroups();

		WriteModelFiles();
	}

	if ( g_bCreateMakefile )
	{
		CreateMakefile_OutputMakefile();
	}

	if (!g_quiet)
	{
		printf("\nCompleted \"%s\"\n", g_path);
	}

	return 0;
}




