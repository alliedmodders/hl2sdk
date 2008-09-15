//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#if !defined(_STATIC_LINKED) || defined(_SHARED_LIB)

#undef PROTECTED_THINGS_ENABLE
#undef PROTECT_FILEIO_FUNCTIONS
#undef fopen

#ifdef _XBOX
#include "xbox/xbox_platform.h"
#include "xbox/xbox_win32stubs.h"
#include "xbox/xbox_core.h"
#endif
#if defined(_WIN32) && !defined(_XBOX)
#include <windows.h>
#include <direct.h>
#include <io.h> // _chmod
#include <process.h>
#elif _LINUX
#include <unistd.h>
#define _putenv putenv
#define _chdir chdir
#define _access access
#endif
#include <stdio.h>
#include <sys/stat.h>
#include "vstdlib/strtools.h"
#include "filesystem_init.h"
#include "vstdlib/ICommandLine.h"
#include "KeyValues.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

#define GAMEINFO_FILENAME			"gameinfo.txt"

static char g_FileSystemError[256];
static FSErrorMode_t g_FileSystemErrorMode = FS_ERRORMODE_VCONFIG;

// This class lets you modify environment variables, and it restores the original value
// when it goes out of scope.
#ifndef _XBOX
class CTempEnvVar
{
public:
	CTempEnvVar( const char *pVarName )
	{
		m_bRestoreOriginalValue = true;
		m_pVarName = pVarName;

		const char *pValue = NULL;

#ifdef _WIN32
		// Use GetEnvironmentVariable instead of getenv because getenv doesn't pick up changes
		// to the process environment after the DLL was loaded.
		char szBuf[ 4096 ];
		if ( GetEnvironmentVariable( m_pVarName, szBuf, sizeof( szBuf ) ) != 0)
		{
			pValue = szBuf;
		}
#else
		// LINUX BUG: see above
		pValue = getenv( pVarName );
#endif // _WIN32

		if ( pValue )
		{
			m_bExisted = true;
			m_OriginalValue.SetSize( strlen( pValue ) + 1 );
			memcpy( m_OriginalValue.Base(), pValue, m_OriginalValue.Count() );
		}
		else
		{
			m_bExisted = false;
		}
	}

	~CTempEnvVar()
	{
		if ( m_bRestoreOriginalValue )
		{
			// Restore the original value.
			if ( m_bExisted )
			{
				SetValue( "%s", m_OriginalValue.Base() );
			}
			else
			{
				ClearValue();
			}
		}
	}

	void SetRestoreOriginalValue( bool bRestore )
	{
		m_bRestoreOriginalValue = bRestore;
	}

	int GetValue(char *pszBuf, int nBufSize )
	{
		if ( !pszBuf || ( nBufSize <= 0 ) )
			return 0;
	
#ifdef _WIN32
		// Use GetEnvironmentVariable instead of getenv because getenv doesn't pick up changes
		// to the process environment after the DLL was loaded.
		return GetEnvironmentVariable( m_pVarName, pszBuf, nBufSize );
#else
		// LINUX BUG: see above
		const char *pszOut = getenv( m_pVarName );
		if ( !pszOut )
		{
			*pszBuf = '\0';
			return 0;
		}

		Q_strncpy( pszBuf, pszOut, nBufSize );		
		return Q_strlen( pszBuf );
#endif // _WIN32
	}

	void SetValue( const char *pValue, ... )
	{
		char valueString[4096];
		va_list marker;
		va_start( marker, pValue );
		Q_vsnprintf( valueString, sizeof( valueString ), pValue, marker );
		va_end( marker );

		char str[4096];
		Q_snprintf( str, sizeof( str ), "%s=%s", m_pVarName, valueString );
		_putenv( str );
	}

	void ClearValue()
	{
		char str[512];
		Q_snprintf( str, sizeof( str ), "%s=", m_pVarName );
		_putenv( str );
	}

private:
	bool m_bRestoreOriginalValue;
	const char *m_pVarName;
	bool m_bExisted;
	CUtlVector<char> m_OriginalValue;
};


class CSteamEnvVars
{
public:
	CSteamEnvVars() :
		m_SteamAppId( "SteamAppId" ),
		m_SteamUserPassphrase( "SteamUserPassphrase" ),
		m_SteamAppUser( "SteamAppUser" ),
		m_Path( "path" )
	{
	}
	
	void SetRestoreOriginalValue_ALL( bool bRestore )
	{
		m_SteamAppId.SetRestoreOriginalValue( bRestore );
		m_SteamUserPassphrase.SetRestoreOriginalValue( bRestore );
		m_SteamAppUser.SetRestoreOriginalValue( bRestore );
		m_Path.SetRestoreOriginalValue( bRestore );
	}

	CTempEnvVar m_SteamAppId;
	CTempEnvVar m_SteamUserPassphrase;
	CTempEnvVar m_SteamAppUser;
	CTempEnvVar m_Path;
};
#endif



// ---------------------------------------------------------------------------------------------------- //
// Helpers.
// ---------------------------------------------------------------------------------------------------- //
#ifndef _XBOX
void Q_getwd( char *out, int outSize )
{
#if defined( _WIN32 ) || defined( WIN32 )
	_getcwd( out, outSize );
	Q_strncat( out, "\\", outSize, COPY_ALL_CHARACTERS );
#else
	getcwd(out, outSize);
	strcat(out, "/");
#endif
	Q_FixSlashes( out );
}
#endif

// ---------------------------------------------------------------------------------------------------- //
// Module interface.
// ---------------------------------------------------------------------------------------------------- //

CFSSearchPathsInit::CFSSearchPathsInit()
{
	m_pDirectoryName = NULL;
	m_pLanguage = NULL;
	m_ModPath[0] = 0;
}


CFSSteamSetupInfo::CFSSteamSetupInfo()
{
	m_pDirectoryName = NULL;
	m_bOnlyUseDirectoryName = false;
	m_bSteam = false;
	m_bToolsMode = true;
	m_bNoGameInfo = false;
}


CFSLoadModuleInfo::CFSLoadModuleInfo()
{
	m_pFileSystemDLLName = NULL;
	m_pFileSystem = NULL;
	m_pModule = NULL;
}


CFSMountContentInfo::CFSMountContentInfo()
{
	m_bToolsMode = true;
	m_pDirectoryName = NULL;
	m_pFileSystem = NULL;
}


const char *FileSystem_GetLastErrorString()
{
	return g_FileSystemError;
}


void AddLanguageGameDir( IFileSystem *pFileSystem, const char *pLocation, const char *pLanguage )
{
#if !defined(SWDS) && !defined(_XBOX)
	char temp[MAX_PATH];
	Q_snprintf( temp, sizeof(temp), "%s_%s", pLocation, pLanguage );
	pFileSystem->AddSearchPath( temp, "GAME", PATH_ADD_TO_TAIL );

	if ( !pFileSystem->IsSteam() )
	{
		// also look in "..\localization\<folder>" if not running Steam
		char baseDir[MAX_PATH];
		char *tempPtr = NULL, *gameDir = NULL;

		Q_strncpy( baseDir, pLocation, sizeof(baseDir) );
		tempPtr = Q_strstr( baseDir, "\\game\\" );
		
		if ( tempPtr )
		{
			gameDir = tempPtr + Q_strlen( "\\game\\" );
			*tempPtr = 0;
			Q_snprintf( temp, sizeof(temp), "%s%clocalization%c%s_%s", baseDir, CORRECT_PATH_SEPARATOR, CORRECT_PATH_SEPARATOR, gameDir, pLanguage );
			pFileSystem->AddSearchPath( temp, "GAME", PATH_ADD_TO_TAIL );
		}
	}
#endif
}


void AddGameBinDir( IFileSystem *pFileSystem, const char *pLocation )
{
#ifndef _XBOX
	char temp[MAX_PATH];
	Q_snprintf( temp, sizeof(temp), "%s%cbin", pLocation, CORRECT_PATH_SEPARATOR );
	pFileSystem->AddSearchPath( temp, "GAMEBIN", PATH_ADD_TO_TAIL );
#endif
}

#ifndef _XBOX
KeyValues* ReadKeyValuesFile( const char *pFilename )
{
	// Read in the gameinfo.txt file and null-terminate it.
	FILE *fp = fopen( pFilename, "rb" );
	if ( !fp )
		return NULL;
	CUtlVector<char> buf;
	fseek( fp, 0, SEEK_END );
	buf.SetSize( ftell( fp ) + 1 );
	fseek( fp, 0, SEEK_SET );
	fread( buf.Base(), 1, buf.Count()-1, fp );
	fclose( fp );
	buf[buf.Count()-1] = 0;

	KeyValues *kv = new KeyValues( "" );
	if ( !kv->LoadFromBuffer( pFilename, buf.Base() ) )
	{
		kv->deleteThis();
		return NULL;
	}
	
	return kv;
}
#endif

static int Sys_GetExecutableName( char *out, int len )
{
#ifdef _WIN32
    if ( !::GetModuleFileName( ( HINSTANCE )GetModuleHandle( NULL ), out, len ) )
    {
		return 0;
    }
#else
	if ( CommandLine()->GetParm(0) )
	{
		Q_MakeAbsolutePath( out, len, CommandLine()->GetParm(0) );
	}
	else
	{
		return 0;
	}
#endif	
	return 1;
}

bool FileSystem_GetExecutableDir( char *exedir, int exeDirLen )
{
	exedir[ 0 ] = 0;
	if ( !Sys_GetExecutableName( exedir, exeDirLen ) )
		return false;

	Q_StripFilename( exedir );
	Q_FixSlashes( exedir );

	// Return the bin directory as the executable dir if it's not in there
	// because that's really where we're running from...
	char ext[MAX_PATH];
	Q_StrRight( exedir, 4, ext, sizeof( ext ) );
	if ( ext[0] != CORRECT_PATH_SEPARATOR || Q_stricmp( ext+1, "bin" ) != 0 )
	{
		Q_strncat( exedir, "\\bin", exeDirLen, COPY_ALL_CHARACTERS );
		Q_FixSlashes( exedir );
	}
	return true;
}

#ifndef _XBOX
static bool FileSystem_GetBaseDir( char *baseDir, int baseDirLen )
{
	if ( FileSystem_GetExecutableDir( baseDir, baseDirLen ) )
	{
		Q_StripFilename( baseDir );
		return true;
	}
	else
	{
		return false;
	}
}
#endif

#ifndef _XBOX
void LaunchVConfig()
{
#ifdef _WIN32
	char vconfigExe[MAX_PATH];
	FileSystem_GetExecutableDir( vconfigExe, sizeof( vconfigExe ) );
	Q_AppendSlash( vconfigExe, sizeof( vconfigExe ) );
	Q_strncat( vconfigExe, "vconfig.exe", sizeof( vconfigExe ), COPY_ALL_CHARACTERS );

	char *argv[] =
	{
		vconfigExe,
		"-allowdebug",
		NULL
	};

	_spawnv( _P_NOWAIT, vconfigExe, argv );
#endif
}
#endif

#ifndef _XBOX
const char* GetVProjectCmdLineValue()
{
	return CommandLine()->ParmValue( "-vproject", CommandLine()->ParmValue( "-game" ) );
}
#endif

FSReturnCode_t SetupFileSystemError( bool bRunVConfig, FSReturnCode_t retVal, const char *pMsg, ... )
{
	va_list marker;
	va_start( marker, pMsg );
	Q_vsnprintf( g_FileSystemError, sizeof( g_FileSystemError ), pMsg, marker );
	va_end( marker );

	Warning( "%s", g_FileSystemError );

#ifndef _XBOX
	// Run vconfig?
	// Don't do it if they specifically asked for it not to, or if they manually specified a vconfig with -game or -vproject.
	if ( bRunVConfig && g_FileSystemErrorMode == FS_ERRORMODE_VCONFIG && !CommandLine()->FindParm( CMDLINEOPTION_NOVCONFIG ) && !GetVProjectCmdLineValue() )
		LaunchVConfig();
#endif

	if ( g_FileSystemErrorMode == FS_ERRORMODE_AUTO || g_FileSystemErrorMode == FS_ERRORMODE_VCONFIG )
		Error( "%s\n", g_FileSystemError );
	
	return retVal;
}


#ifndef _XBOX
FSReturnCode_t LoadGameInfoFile( 
	const char *pDirectoryName, 
	KeyValues *&pMainFile, 
	KeyValues *&pFileSystemInfo, 
	KeyValues *&pSearchPaths )
{
	// If GameInfo.txt exists under pBaseDir, then this is their game directory.
	// All the filesystem mappings will be in this file.
	char gameinfoFilename[MAX_PATH];
	Q_strncpy( gameinfoFilename, pDirectoryName, sizeof( gameinfoFilename ) );
	Q_AppendSlash( gameinfoFilename, sizeof( gameinfoFilename ) );
	Q_strncat( gameinfoFilename, GAMEINFO_FILENAME, sizeof( gameinfoFilename ), COPY_ALL_CHARACTERS );
	Q_FixSlashes( gameinfoFilename );
	pMainFile = ReadKeyValuesFile( gameinfoFilename );
	if ( !pMainFile )
		return SetupFileSystemError( true, FS_MISSING_GAMEINFO_FILE, "%s is missing.", gameinfoFilename );

	pFileSystemInfo = pMainFile->FindKey( "FileSystem" );
	if ( !pFileSystemInfo )
	{
		pMainFile->deleteThis();
		return SetupFileSystemError( true, FS_INVALID_GAMEINFO_FILE, "%s is not a valid format.", gameinfoFilename );
	}

	// Now read in all the search paths.
	pSearchPaths = pFileSystemInfo->FindKey( "SearchPaths" );
	if ( !pSearchPaths )
	{
		pMainFile->deleteThis();
		return SetupFileSystemError( true, FS_INVALID_GAMEINFO_FILE, "%s is not a valid format.", gameinfoFilename );
	}
	return FS_OK;
}
#endif

// checks the registry for the low violence setting
// Check "HKEY_CURRENT_USER\Software\Valve\Source\Settings" and "User Token 2" or "User Token 3"
#ifndef _XBOX
bool IsLowViolenceBuild( void )
{
#if defined(_WIN32)
	HKEY hKey;
	char szValue[64];
	unsigned long len = sizeof(szValue) - 1;
	bool retVal = false;
	
	if ( RegOpenKeyEx( HKEY_CURRENT_USER, "Software\\Valve\\Source\\Settings", NULL, KEY_READ, &hKey) == ERROR_SUCCESS )
	{
		// User Token 2
		if ( RegQueryValueEx( hKey, "User Token 2", NULL, NULL, (unsigned char*)szValue, &len ) == ERROR_SUCCESS )
		{
			if ( Q_strlen( szValue ) > 0 )
			{
				retVal = true;
			}
		}

		if ( !retVal )
		{
			// reset "len" for the next check
			len = sizeof(szValue) - 1;

			// User Token 3
			if ( RegQueryValueEx( hKey, "User Token 3", NULL, NULL, (unsigned char*)szValue, &len ) == ERROR_SUCCESS )
			{
				if ( Q_strlen( szValue ) > 0 )
				{
					retVal = true;
				}
			}
		}

		RegCloseKey(hKey);
	}

	return retVal;
#elif _LINUX
	return false;
#elif
#error "Fix me"
#endif
}
#endif

FSReturnCode_t FileSystem_LoadSearchPaths( CFSSearchPathsInit &initInfo )
{
#ifndef _XBOX
	bool bLowViolence = IsLowViolenceBuild();

	if ( !initInfo.m_pFileSystem || !initInfo.m_pDirectoryName )
		return SetupFileSystemError( false, FS_INVALID_PARAMETERS, "FileSystem_LoadSearchPaths: Invalid parameters specified." );

	KeyValues *pMainFile, *pFileSystemInfo, *pSearchPaths;
	FSReturnCode_t retVal = LoadGameInfoFile( initInfo.m_pDirectoryName, pMainFile, pFileSystemInfo, pSearchPaths );
	if ( retVal != FS_OK )
		return retVal;
	
	// All paths except those marked with |gameinfo_path| are relative to the base dir.
	char baseDir[MAX_PATH];
	if ( !FileSystem_GetBaseDir( baseDir, sizeof( baseDir ) ) )
		return SetupFileSystemError( false, FS_INVALID_PARAMETERS, "FileSystem_GetBaseDir failed." );
	
	initInfo.m_ModPath[0] = 0;

	bool bFirstGamePath = true;
	for ( KeyValues *pCur=pSearchPaths->GetFirstValue(); pCur; pCur=pCur->GetNextValue() )
	{
		const char *pPathID = pCur->GetName();
		
		char fullLocationPath[MAX_PATH];
		const char *pLocation = pCur->GetString();
		if ( Q_stristr( pLocation, "|gameinfo_path|" ) == pLocation )
			Q_MakeAbsolutePath( fullLocationPath, sizeof( fullLocationPath ), pLocation + strlen( "|gameinfo_path|" ), initInfo.m_pDirectoryName );
		else
			Q_MakeAbsolutePath( fullLocationPath, sizeof( fullLocationPath ), pLocation, baseDir );
	
		// Add language, mod, and gamebin search paths automatically.
		if ( Q_stricmp( pPathID, "game" ) == 0 )
		{
			// add the low violence path
			if ( bLowViolence )
			{
				char szPath[MAX_PATH];
				Q_snprintf( szPath, sizeof(szPath), "%s_lv", fullLocationPath );
				initInfo.m_pFileSystem->AddSearchPath( szPath, pPathID, PATH_ADD_TO_TAIL );
			}
			
			// add the language path
			if ( initInfo.m_pLanguage )
			{
				AddLanguageGameDir( initInfo.m_pFileSystem, fullLocationPath, initInfo.m_pLanguage );
			}

#ifndef _XBOX
			if ( CommandLine()->FindParm( "-tempcontent" ) != 0 )
			{
				char szPath[MAX_PATH];
				Q_snprintf( szPath, sizeof(szPath), "%s_tempcontent", fullLocationPath );
				initInfo.m_pFileSystem->AddSearchPath( szPath, pPathID, PATH_ADD_TO_TAIL );
			}
#endif

			// mark the first "game" dir as the "MOD" dir
			if ( bFirstGamePath )
			{
				bFirstGamePath = false;
				initInfo.m_pFileSystem->AddSearchPath( fullLocationPath, "MOD", PATH_ADD_TO_TAIL );
				Q_strncpy( initInfo.m_ModPath, fullLocationPath, sizeof( initInfo.m_ModPath ) );
			}
		
			// add the game bin
			AddGameBinDir( initInfo.m_pFileSystem, fullLocationPath );
		}

		initInfo.m_pFileSystem->AddSearchPath( fullLocationPath, pPathID, PATH_ADD_TO_TAIL );
	}

	pMainFile->deleteThis();

	// Also, mark specific path IDs as "by request only". That way, we won't waste time searching in them
	// when people forget to specify a search path.
	initInfo.m_pFileSystem->MarkPathIDByRequestOnly( "executable_path", true );
	initInfo.m_pFileSystem->MarkPathIDByRequestOnly( "gamebin", true );
	initInfo.m_pFileSystem->MarkPathIDByRequestOnly( "mod", true );
	if ( initInfo.m_ModPath[0] != 0 )
	{
		// Add the write path last.
		initInfo.m_pFileSystem->AddSearchPath( initInfo.m_ModPath, "DEFAULT_WRITE_PATH", PATH_ADD_TO_TAIL );
	}

#ifdef _DEBUG	
	initInfo.m_pFileSystem->PrintSearchPaths();
#endif

#endif // _XBOX
	return FS_OK;
}

#ifndef _XBOX
bool DoesFileExistIn( const char *pDirectoryName, const char *pFilename )
{
	char filename[MAX_PATH];
	Q_strncpy( filename, pDirectoryName, sizeof( filename ) );
	Q_AppendSlash( filename, sizeof( filename ) );
	Q_strncat( filename, pFilename, sizeof( filename ), COPY_ALL_CHARACTERS );
	Q_FixSlashes( filename );
	return ( _access( filename, 0 ) == 0 );
}
#endif

#ifndef _XBOX
FSReturnCode_t LocateGameInfoFile( const CFSSteamSetupInfo &fsInfo, char *pOutDir, int outDirLen )
{
	// Engine and Hammer don't want to search around for it.
	if ( fsInfo.m_bOnlyUseDirectoryName )
	{
		if ( !fsInfo.m_pDirectoryName )
			return SetupFileSystemError( false, FS_MISSING_GAMEINFO_FILE, "bOnlyUseDirectoryName=1 and pDirectoryName=NULL." );

		if ( !DoesFileExistIn( fsInfo.m_pDirectoryName, GAMEINFO_FILENAME ) )
			return SetupFileSystemError( true, FS_MISSING_GAMEINFO_FILE, "%s doesn't exist in %s.", GAMEINFO_FILENAME, fsInfo.m_pDirectoryName );

		Q_strncpy( pOutDir, fsInfo.m_pDirectoryName, outDirLen );
		return FS_OK;
	}

	// First, check for overrides on the command line or environment variables.
	const char *pProject = GetVProjectCmdLineValue();
	if ( !pProject )
	{
		// Check their registry.
		pProject = getenv( GAMEDIR_TOKEN );
	}

	if ( pProject )
	{
		if ( DoesFileExistIn( pProject, GAMEINFO_FILENAME ) )
		{
			Q_MakeAbsolutePath( pOutDir, outDirLen, pProject );
			return FS_OK;
		}
		else if ( fsInfo.m_bNoGameInfo )
		{
			// fsInfo.m_bNoGameInfo is set by the Steam dedicated server, before it knows which mod to use.
			// Steam dedicated server doesn't need a gameinfo.txt, because we'll ask which mod to use, even if
			// -game is supplied on the command line.
			Q_strncpy( pOutDir, "", outDirLen );
			return FS_OK;
		}
		else
		{
			// They either specified vproject on the command line or it's in their registry. Either way,
			// we don't want to continue if they've specified it but it's not valid.
			goto ShowError;
		}
	}

	if ( fsInfo.m_bNoGameInfo )
	{
		Q_strncpy( pOutDir, "", outDirLen );
		return FS_OK;
	}

	Warning( "Warning: falling back to auto detection of vproject directory.\n" );
	
	// Now look for it in the directory they passed in.
	if ( fsInfo.m_pDirectoryName )
		Q_MakeAbsolutePath( pOutDir, outDirLen, fsInfo.m_pDirectoryName );
	else
		Q_MakeAbsolutePath( pOutDir, outDirLen, "." );

	do 
	{
		if ( DoesFileExistIn( pOutDir, GAMEINFO_FILENAME ) )
			return FS_OK;
	} while ( Q_StripLastDir( pOutDir, outDirLen ) );

	// use the cwd and hunt down the tree until we find something
	Q_getwd( pOutDir, outDirLen );
	do
	{
		if ( DoesFileExistIn( pOutDir, GAMEINFO_FILENAME ) )
			return FS_OK;
	} while ( Q_StripLastDir( pOutDir, outDirLen ) );

ShowError:
	return SetupFileSystemError( true, FS_MISSING_GAMEINFO_FILE, 
		"Unable to find %s. Solutions:\n\n"
		"1. Read http://www.valve-erc.com/srcsdk/faq.html#NoGameDir\n"
		"2. Run vconfig to specify which game you're working on.\n"
		"3. Add -game <path> on the command line where <path> is the directory that %s is in.\n",
		GAMEINFO_FILENAME, GAMEINFO_FILENAME );
}
#endif

#ifndef _XBOX
bool DoesPathExistAlready( const char *pPathEnvVar, const char *pTestPath )
{
	// Fix the slashes in the input arguments.
	char correctedPathEnvVar[8192], correctedTestPath[MAX_PATH];
	Q_strncpy( correctedPathEnvVar, pPathEnvVar, sizeof( correctedPathEnvVar ) );
	Q_FixSlashes( correctedPathEnvVar );
	pPathEnvVar = correctedPathEnvVar;

	Q_strncpy( correctedTestPath, pTestPath, sizeof( correctedTestPath ) );
	Q_FixSlashes( correctedTestPath );
	if ( strlen( correctedTestPath ) > 0 && PATHSEPARATOR( correctedTestPath[strlen(correctedTestPath)-1] ) )
		correctedTestPath[ strlen(correctedTestPath) - 1 ] = 0;

	pTestPath = correctedTestPath;

	const char *pCurPos = pPathEnvVar;
	while ( 1 )
	{
		const char *pTestPos = Q_stristr( pCurPos, pTestPath );
		if ( !pTestPos )
			return false;

		// Ok, we found pTestPath in the path, but it's only valid if it's followed by an optional slash and a semicolon.
		pTestPos += strlen( pTestPath );
		if ( pTestPos[0] == 0 || pTestPos[0] == ';' || (PATHSEPARATOR( pTestPos[0] ) && pTestPos[1] == ';') )
			return true;
	
		// Advance our marker..
		pCurPos = pTestPos;
	}
}
#endif

#ifndef _XBOX
FSReturnCode_t SetSteamInstallPath( char *steamInstallPath, int steamInstallPathLen, CSteamEnvVars &steamEnvVars, bool bErrorsAsWarnings )
{
	// Start at our bin directory and move up until we find a directory with steam.dll in it.
	char executablePath[MAX_PATH];
	if ( !FileSystem_GetExecutableDir( executablePath, sizeof( executablePath ) )	)
	{
		if ( bErrorsAsWarnings )
		{
			Warning( "SetSteamInstallPath: FileSystem_GetExecutableDir failed." );
			return FS_INVALID_PARAMETERS;
		}
		else
		{
			return SetupFileSystemError( false, FS_INVALID_PARAMETERS, "FileSystem_GetExecutableDir failed." );
		}
	}

	Q_strncpy( steamInstallPath, executablePath, steamInstallPathLen );
	while ( 1 )
	{
		// Ignore steamapp.cfg here in case they're debugging. We still need to know the real steam path so we can find their username.
		// find 
		if ( DoesFileExistIn( steamInstallPath, "steam.dll" ) && !DoesFileExistIn( steamInstallPath, "steamapp.cfg" ) )
			break;
	
		if ( !Q_StripLastDir( steamInstallPath, steamInstallPathLen ) )
		{
			if ( bErrorsAsWarnings )
			{
				Warning( "Can't find steam.dll relative to executable path: %s.", executablePath );
				return FS_MISSING_STEAM_DLL;
			}
			else
			{
				return SetupFileSystemError( false, FS_MISSING_STEAM_DLL, "Can't find steam.dll relative to executable path: %s.", executablePath );
			}
		}			
	}

	// Also, add the install path to their PATH environment variable, so filesystem_steam.dll can get to steam.dll.
	char szPath[ 8192 ];
	steamEnvVars.m_Path.GetValue( szPath, sizeof( szPath ) );
	if ( !DoesPathExistAlready( szPath, steamInstallPath ) )
	{
		steamEnvVars.m_Path.SetValue( "%s;%s", szPath, steamInstallPath );
	}
	return FS_OK;
}
#endif

#ifndef _XBOX
FSReturnCode_t GetSteamCfgPath( char *steamCfgPath, int steamCfgPathLen )
{
	steamCfgPath[0] = 0;
	char executablePath[MAX_PATH];
	if ( !FileSystem_GetExecutableDir( executablePath, sizeof( executablePath ) ) )
	{
		return SetupFileSystemError( false, FS_INVALID_PARAMETERS, "FileSystem_GetExecutableDir failed." );
	}
	Q_strncpy( steamCfgPath, executablePath, steamCfgPathLen );
	while ( 1 )
	{
		if ( DoesFileExistIn( steamCfgPath, "steam.cfg" ) )
			break;
	
		if ( !Q_StripLastDir( steamCfgPath, steamCfgPathLen) )
		{
			// the file isnt found, thats ok, its not mandatory
			return FS_OK;
		}			
	}
	Q_AppendSlash( steamCfgPath, steamCfgPathLen );
	Q_strncat( steamCfgPath, "steam.cfg", steamCfgPathLen, COPY_ALL_CHARACTERS );

	return FS_OK;
}
#endif

#ifndef _XBOX
void SetSteamAppUser( KeyValues *pSteamInfo, const char *steamInstallPath, CSteamEnvVars &steamEnvVars )
{
	// Always inherit the Steam user if it's already set, since it probably means we (or the
	// the app that launched us) were launched from Steam.
	char appUser[MAX_PATH];
	if ( steamEnvVars.m_SteamAppUser.GetValue( appUser, sizeof( appUser ) ) )
		return;

	const char *pTempAppUser = NULL;
	if ( pSteamInfo && (pTempAppUser = pSteamInfo->GetString( "SteamAppUser", NULL )) != NULL )
	{
		Q_strncpy( appUser, pTempAppUser, sizeof( appUser ) );
	}
	else
	{
		// They don't have SteamInfo.txt, or it's missing SteamAppUser. Try to figure out the user
		// by looking in <steam install path>\config\SteamAppData.vdf.
		char fullFilename[MAX_PATH];
		Q_strncpy( fullFilename, steamInstallPath, sizeof( fullFilename ) );
		Q_AppendSlash( fullFilename, sizeof( fullFilename ) );
		Q_strncat( fullFilename, "config\\SteamAppData.vdf", sizeof( fullFilename ), COPY_ALL_CHARACTERS );

		KeyValues *pSteamAppData = ReadKeyValuesFile( fullFilename );
		if ( !pSteamAppData || (pTempAppUser = pSteamAppData->GetString( "AutoLoginUser", NULL )) == NULL )
		{
			Error( "Can't find steam app user info." );
		}
		Q_strncpy( appUser, pTempAppUser, sizeof( appUser ) ); 
		
		pSteamAppData->deleteThis();
	}

	Q_strlower( appUser );
	steamEnvVars.m_SteamAppUser.SetValue( "%s", appUser );
}
#endif

#ifndef _XBOX
void SetSteamUserPassphrase( KeyValues *pSteamInfo, CSteamEnvVars &steamEnvVars )
{
	// Always inherit the passphrase if it's already set, since it probably means we (or the
	// the app that launched us) were launched from Steam.
	char szPassPhrase[ MAX_PATH ];
	if ( steamEnvVars.m_SteamUserPassphrase.GetValue( szPassPhrase, sizeof( szPassPhrase ) ) )
		return;

	// SteamUserPassphrase.
	const char *pStr;
	if ( pSteamInfo && (pStr = pSteamInfo->GetString( "SteamUserPassphrase", NULL )) != NULL )
	{
		steamEnvVars.m_SteamUserPassphrase.SetValue( "%s", pStr );
	}
}
#endif

#ifndef _XBOX
void SetSteamAppId( KeyValues *pFileSystemInfo, const char *pGameInfoDirectory, CSteamEnvVars &steamEnvVars )
{
	// SteamAppId is in gameinfo.txt->FileSystem->FileSystemInfo_Steam->SteamAppId.
	int iAppId = pFileSystemInfo->GetInt( "SteamAppId", -1 );
	if ( iAppId == -1 )
		Error( "Missing SteamAppId in %s\\%s.", pGameInfoDirectory, GAMEINFO_FILENAME );

	steamEnvVars.m_SteamAppId.SetValue( "%d", iAppId );
}
#endif

#ifndef _XBOX
FSReturnCode_t SetupSteamStartupEnvironment( KeyValues *pFileSystemInfo, const char *pGameInfoDirectory, CSteamEnvVars &steamEnvVars )
{
	// Ok, we're going to run Steam. See if they have SteamInfo.txt. If not, we'll try to deduce what we can.
	char steamInfoFile[MAX_PATH];
	Q_strncpy( steamInfoFile, pGameInfoDirectory, sizeof( steamInfoFile ) );
	Q_AppendSlash( steamInfoFile, sizeof( steamInfoFile ) );
	Q_strncat( steamInfoFile, "steaminfo.txt", sizeof( steamInfoFile ), COPY_ALL_CHARACTERS );
	KeyValues *pSteamInfo = ReadKeyValuesFile( steamInfoFile );

	char steamInstallPath[MAX_PATH];
	FSReturnCode_t ret = SetSteamInstallPath( steamInstallPath, sizeof( steamInstallPath ), steamEnvVars, false );
	if ( ret != FS_OK )
		return ret;

	SetSteamAppUser( pSteamInfo, steamInstallPath, steamEnvVars );
	SetSteamUserPassphrase( pSteamInfo, steamEnvVars );
	SetSteamAppId( pFileSystemInfo, pGameInfoDirectory, steamEnvVars );

	if ( pSteamInfo )
		pSteamInfo->deleteThis();

	return FS_OK;
}
#endif

#ifndef _XBOX
FSReturnCode_t GetSteamExtraAppId( const char *pDirectoryName, int *nExtraAppId )
{
	// Now, load gameinfo.txt (to make sure it's there)
	KeyValues *pMainFile, *pFileSystemInfo, *pSearchPaths;
	FSReturnCode_t ret = LoadGameInfoFile( pDirectoryName, pMainFile, pFileSystemInfo, pSearchPaths );
	if ( ret != FS_OK )
		return ret;

	*nExtraAppId = pFileSystemInfo->GetInt( "ToolsAppId", -1 );
	pMainFile->deleteThis();
	return FS_OK;
}
#endif

#ifndef _XBOX
FSReturnCode_t FileSystem_SetBasePaths( IFileSystem *pFileSystem )
{
	pFileSystem->RemoveSearchPaths( "EXECUTABLE_PATH" );

	char executablePath[MAX_PATH];
	if ( !FileSystem_GetExecutableDir( executablePath, sizeof( executablePath ) )	)
		return SetupFileSystemError( false, FS_INVALID_PARAMETERS, "FileSystem_GetExecutableDir failed." );

	pFileSystem->AddSearchPath( executablePath, "EXECUTABLE_PATH" );
	return FS_OK;
}
#endif

//-----------------------------------------------------------------------------
// Returns the name of the file system DLL to use
//-----------------------------------------------------------------------------
FSReturnCode_t FileSystem_GetFileSystemDLLName( char *pFileSystemDLL, int nMaxLen, bool &bSteam )
{
	bSteam = false;

	// Inside of here, we don't have a filesystem yet, so we have to assume that the filesystem_stdio or filesystem_steam
	// is in this same directory with us.
	char executablePath[MAX_PATH];
	if ( !FileSystem_GetExecutableDir( executablePath, sizeof( executablePath ) )	)
		return SetupFileSystemError( false, FS_INVALID_PARAMETERS, "FileSystem_GetExecutableDir failed." );
	
#ifdef _WIN32
	Q_snprintf( pFileSystemDLL, nMaxLen, "%s%cfilesystem_stdio.dll", executablePath, CORRECT_PATH_SEPARATOR );

	// If filesystem_stdio.dll is missing or -steam is specified, then load filesystem_steam.dll.
	// There are two command line parameters for Steam:
	//		1) -steam (runs Steam in remote filesystem mode; requires Steam backend)
	//		2) -steamlocal (runs Steam in local filesystem mode (all content off HDD)
#ifndef _XBOX
	if ( CommandLine()->FindParm( "-steam" ) || CommandLine()->FindParm( "-steamlocal" ) || _access( pFileSystemDLL, 0 ) != 0 )
	{
		Q_snprintf( pFileSystemDLL, nMaxLen, "%s%cfilesystem_steam.dll", executablePath, CORRECT_PATH_SEPARATOR );
		bSteam = true;
	}
#endif
#elif _LINUX
	Q_snprintf( pFileSystemDLL, nMaxLen, "%s%cfilesystem_i486.so", executablePath, CORRECT_PATH_SEPARATOR );
#else
	#error "define a filesystem dll name"
#endif

	return FS_OK;
}

//-----------------------------------------------------------------------------
// Sets up the steam.dll install path in our PATH env var (so you can then just 
// LoadLibrary() on filesystem_steam.dll without having to copy steam.dll anywhere special )
//-----------------------------------------------------------------------------
FSReturnCode_t FileSystem_SetupSteamInstallPath()
{
#ifndef _XBOX
	CSteamEnvVars steamEnvVars;
	char steamInstallPath[MAX_PATH];
	FSReturnCode_t ret = SetSteamInstallPath( steamInstallPath, sizeof( steamInstallPath ), steamEnvVars, true );
	steamEnvVars.m_Path.SetRestoreOriginalValue( false ); // We want to keep the change to the path going forward.
	return ret;
#else
	return FS_OK;
#endif
}

//-----------------------------------------------------------------------------
// Sets up the steam environment + gets back the gameinfo.txt path
//-----------------------------------------------------------------------------
FSReturnCode_t FileSystem_SetupSteamEnvironment( CFSSteamSetupInfo &fsInfo )
{
#ifndef _XBOX
	// First, locate the directory with gameinfo.txt.
	FSReturnCode_t ret = LocateGameInfoFile( fsInfo, fsInfo.m_GameInfoPath, sizeof( fsInfo.m_GameInfoPath ) );
	if ( ret != FS_OK )
		return ret;

	CSteamEnvVars steamEnvVars;
	if ( fsInfo.m_bSteam )
	{
		if ( fsInfo.m_bToolsMode )
		{
			// Now, load gameinfo.txt (to make sure it's there)
			KeyValues *pMainFile, *pFileSystemInfo, *pSearchPaths;
			ret = LoadGameInfoFile( fsInfo.m_GameInfoPath, pMainFile, pFileSystemInfo, pSearchPaths );
			if ( ret != FS_OK )
				return ret;

			// If filesystem_stdio.dll is missing or -steam is specified, then load filesystem_steam.dll.
			// There are two command line parameters for Steam:
			//		1) -steam (runs Steam in remote filesystem mode; requires Steam backend)
			//		2) -steamlocal (runs Steam in local filesystem mode (all content off HDD)

			// Setup all the environment variables related to Steam so filesystem_steam.dll knows how to initialize Steam.
			ret = SetupSteamStartupEnvironment( pFileSystemInfo, fsInfo.m_GameInfoPath, steamEnvVars );
			if ( ret != FS_OK )
				return ret;

			steamEnvVars.m_SteamAppId.SetRestoreOriginalValue( false ); // We want to keep the change to the path going forward.

			// We're done with main file
			pMainFile->deleteThis();
		}
		else if ( fsInfo.m_bSetSteamDLLPath )
		{
			// This is used by the engine to automatically set the path to their steam.dll when running the engine,
			// so they can debug it without having to copy steam.dll up into their hl2.exe folder.
			char steamInstallPath[MAX_PATH];
			ret = SetSteamInstallPath( steamInstallPath, sizeof( steamInstallPath ), steamEnvVars, true );
			steamEnvVars.m_Path.SetRestoreOriginalValue( false ); // We want to keep the change to the path going forward.
		}
	}
#endif

	return FS_OK;
}


//-----------------------------------------------------------------------------
// Loads the file system module
//-----------------------------------------------------------------------------
FSReturnCode_t FileSystem_LoadFileSystemModule( CFSLoadModuleInfo &fsInfo )
{
	// First, locate the directory with gameinfo.txt.
	FSReturnCode_t ret = FileSystem_SetupSteamEnvironment( fsInfo );
	if ( ret != FS_OK )
		return ret;

	// Now that the environment is setup, load the filesystem module.
	if ( !Sys_LoadInterface(
		fsInfo.m_pFileSystemDLLName,
		FILESYSTEM_INTERFACE_VERSION,
		&fsInfo.m_pModule,
		(void**)&fsInfo.m_pFileSystem ) )
	{
		return SetupFileSystemError( false, FS_UNABLE_TO_INIT, "Can't load %s.", fsInfo.m_pFileSystemDLLName );
	}

	if ( !fsInfo.m_pFileSystem->Connect( fsInfo.m_ConnectFactory ) )
		return SetupFileSystemError( false, FS_UNABLE_TO_INIT, "%s IFileSystem::Connect failed.", fsInfo.m_pFileSystemDLLName );

	if ( fsInfo.m_pFileSystem->Init() != INIT_OK )
		return SetupFileSystemError( false, FS_UNABLE_TO_INIT, "%s IFileSystem::Init failed.", fsInfo.m_pFileSystemDLLName );

	return FS_OK;
}


//-----------------------------------------------------------------------------
// Mounds a particular steam cache
//-----------------------------------------------------------------------------
#ifndef _XBOX
FSReturnCode_t FileSystem_MountContent( CFSMountContentInfo &mountContentInfo )
{
	// This part is Steam-only.
	if ( mountContentInfo.m_pFileSystem->IsSteam() )
	{
		// Find out the "extra app id". This is for tools, which want to mount a base app's filesystem
		// like HL2, then mount the SDK content (tools materials and models, etc) in addition.
		int nExtraAppId = -1;
		if ( mountContentInfo.m_bToolsMode )
		{
			FSReturnCode_t ret = GetSteamExtraAppId( mountContentInfo.m_pDirectoryName, &nExtraAppId );
			if ( ret != FS_OK )
				return ret;
		}

		// Set our working directory temporarily so Steam can remember it.
		// This is what Steam strips off absolute filenames like c:\program files\valve\steam\steamapps\username\sourcesdk
		// to get to the relative part of the path.
		char baseDir[MAX_PATH], oldWorkingDir[MAX_PATH];
		if ( !FileSystem_GetBaseDir( baseDir, sizeof( baseDir ) ) )
			return SetupFileSystemError( false, FS_INVALID_PARAMETERS, "FileSystem_GetBaseDir failed." );

		Q_getwd( oldWorkingDir, sizeof( oldWorkingDir ) );
		_chdir( baseDir );

		// Filesystem_tools needs to add dependencies in here beforehand.
		FilesystemMountRetval_t retVal = mountContentInfo.m_pFileSystem->MountSteamContent( nExtraAppId );
		
		_chdir( oldWorkingDir );

		if ( retVal != FILESYSTEM_MOUNT_OK )
			return SetupFileSystemError( true, FS_UNABLE_TO_INIT, "Unable to mount Steam content in the file system" );
	}

	return FileSystem_SetBasePaths( mountContentInfo.m_pFileSystem );
}
#endif

void FileSystem_SetErrorMode( FSErrorMode_t errorMode )
{
	g_FileSystemErrorMode = errorMode;
}

#ifndef _XBOX
void FileSystem_ClearSteamEnvVars()
{
	CSteamEnvVars envVars;

	// Change the values and don't restore the originals.
	envVars.m_SteamAppId.SetValue( "" );
	envVars.m_SteamUserPassphrase.SetValue( "" );
	envVars.m_SteamAppUser.SetValue( "" );
	
	envVars.SetRestoreOriginalValue_ALL( false );
}
#endif

#endif // !_STATIC_LINKED || _SHARED_LIB


//-----------------------------------------------------------------------------
// Adds the platform folder to the search path.
//-----------------------------------------------------------------------------
void FileSystem_AddSearchPath_Platform( IFileSystem *pFileSystem, const char *szGameInfoPath )
{
	char platform[MAX_PATH];
	if ( pFileSystem->IsSteam() )
	{
		// Steam doesn't support relative paths
		Q_strncpy( platform, "platform", MAX_PATH );
	}
	else
	{
		Q_strncpy( platform, szGameInfoPath, MAX_PATH );
		Q_StripTrailingSlash( platform );
		Q_strncat( platform, "/../platform", MAX_PATH, MAX_PATH );
	}

	pFileSystem->AddSearchPath( platform, "PLATFORM" );
}
