//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#ifndef SCRIPLIB_H
#define SCRIPLIB_H

#ifdef _WIN32
#pragma once
#endif


#ifdef __cplusplus
extern "C" {
#endif


enum ScriptPathMode_t
{
	SCRIPT_USE_ABSOLUTE_PATH,
	SCRIPT_USE_RELATIVE_PATH
};


// scriplib.h

#define	MAXTOKEN	1024

extern	char	token[MAXTOKEN];
extern	char	*scriptbuffer,*script_p,*scriptend_p;
extern	int		grabbed;
extern	int		scriptline;
extern	qboolean	endofscript;


// If pathMode is SCRIPT_USE_ABSOLUTE_PATH, then it uses ExpandPath() on the filename before
// trying to open it. Otherwise, it passes the filename straight into the filesystem
// (so you can leave it as a relative path).
void LoadScriptFile (char *filename, ScriptPathMode_t pathMode=SCRIPT_USE_ABSOLUTE_PATH);
void ParseFromMemory (char *buffer, int size);

qboolean GetToken (qboolean crossline);
qboolean GetExprToken (qboolean crossline);
void UnGetToken (void);
qboolean TokenAvailable (void);
qboolean GetTokenizerStatus( char **pFilename, int *pLine );

#ifdef __cplusplus
}
#endif

#endif // SCRIPLIB_H
