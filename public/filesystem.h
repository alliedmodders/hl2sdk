//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include <limits.h>

#include "tier0/threadtools.h"
#include "tier0/memalloc.h"
#include "tier1/interface.h"
#include "tier1/utlsymbol.h"
#include "appframework/IAppSystem.h"

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------

class CUtlBuffer;
class KeyValues;

typedef void * FileHandle_t;
typedef int FileFindHandle_t;
typedef void (*FileSystemLoggingFunc_t)( const char *fileName, const char *accessType );
typedef int WaitForResourcesHandle_t;

// The handle is a CUtlSymbol for the dirname and the same for the filename, the accessor
//  copies them into a static char buffer for return.
typedef void* FileNameHandle_t;


#ifdef _XBOX
typedef void* HANDLE;
#endif
//-----------------------------------------------------------------------------
// Enums used by the interface
//-----------------------------------------------------------------------------

#ifndef _XBOX
#define FILESYSTEM_MAX_SEARCH_PATHS 128
#else
#define FILESYSTEM_MAX_SEARCH_PATHS 16
#endif

enum FileSystemSeek_t
{
	FILESYSTEM_SEEK_HEAD	= SEEK_SET,
	FILESYSTEM_SEEK_CURRENT = SEEK_CUR,
	FILESYSTEM_SEEK_TAIL	= SEEK_END,
};

enum
{
	FILESYSTEM_INVALID_FIND_HANDLE = -1
};

enum FileWarningLevel_t
{
	// A problem!
	FILESYSTEM_WARNING = -1,

	// Don't print anything
	FILESYSTEM_WARNING_QUIET = 0,

	// On shutdown, report names of files left unclosed
	FILESYSTEM_WARNING_REPORTUNCLOSED,

	// Report number of times a file was opened, closed
	FILESYSTEM_WARNING_REPORTUSAGE,

	// Report all open/close events to console ( !slow! )
	FILESYSTEM_WARNING_REPORTALLACCESSES,

	// Report all open/close/read events to the console ( !slower! )
	FILESYSTEM_WARNING_REPORTALLACCESSES_READ,

	// Report all open/close/read/write events to the console ( !slower! )
	FILESYSTEM_WARNING_REPORTALLACCESSES_READWRITE,

	// Report all open/close/read/write events and all async I/O file events to the console ( !slower(est)! )
	FILESYSTEM_WARNING_REPORTALLACCESSES_ASYNC,

};

enum FilesystemMountRetval_t
{
	FILESYSTEM_MOUNT_OK = 0,
	FILESYSTEM_MOUNT_FAILED,
};

enum SearchPathAdd_t
{
	PATH_ADD_TO_HEAD,	// First path searched
	PATH_ADD_TO_TAIL,	// Last path searched
};

#define FILESYSTEM_INVALID_HANDLE	( FileHandle_t )0

//-----------------------------------------------------------------------------
// Structures used by the interface
//-----------------------------------------------------------------------------

struct FileSystemStatistics
{
	CInterlockedUInt	nReads,		
						nWrites,		
						nBytesRead,
						nBytesWritten,
						nSeeks;
};

//-----------------------------------------------------------------------------
// File system allocation functions. Client must free on failure
//-----------------------------------------------------------------------------
typedef void *(*FSAllocFunc_t)( const char *pszFilename, unsigned nBytes );

//-----------------------------------------------------------------------------
// Asynchronous support types
//-----------------------------------------------------------------------------

DECLARE_POINTER_HANDLE(FSAsyncControl_t);
DECLARE_POINTER_HANDLE(FSAsyncFile_t);
const FSAsyncFile_t FS_INVALID_ASYNC_FILE = (FSAsyncFile_t)(0x0000ffff);

//---------------------------------------------------------
// Async file status
//---------------------------------------------------------
enum FSAsyncStatus_t
{
	FSASYNC_ERR_ALIGNMENT    = -6,	// read parameters invalid for unbuffered IO
	FSASYNC_ERR_FAILURE      = -5,	// hard subsystem failure
	FSASYNC_ERR_READING      = -4,	// read error on file
	FSASYNC_ERR_NOMEMORY     = -3,	// out of memory for file read
	FSASYNC_ERR_UNKNOWNID    = -2,	// caller's provided id is not recognized
	FSASYNC_ERR_FILEOPEN     = -1,	// filename could not be opened (bad path, not exist, etc)
	FSASYNC_OK               = 0,	// operation is successful
	FSASYNC_STATUS_PENDING,			// file is properly queued, waiting for service
	FSASYNC_STATUS_INPROGRESS,		// file is being accessed
	FSASYNC_STATUS_ABORTED,			// file was aborted by caller
	FSASYNC_STATUS_UNSERVICED,		// file is not yet queued
};

//---------------------------------------------------------
// Async request flags
//---------------------------------------------------------
enum FSAsyncFlags_t
{
	FSASYNC_FLAGS_ALLOCNOFREE		= ( 1 << 0 ),	// do the allocation for dataPtr, but don't free
	FSASYNC_FLAGS_NOSEARCHPATHS		= ( 1 << 1 ),	// filename is absolute
	FSASYNC_FLAGS_FREEDATAPTR		= ( 1 << 2 ),	// free the memory for the dataPtr post callback
	FSASYNC_FLAGS_RAWIO				= ( 1 << 3 ),	// request an unbuffered IO. On Win32 & XBox implies alignment restrictions.
	FSASYNC_FLAGS_SYNC				= ( 1 << 4 ),	// Actually perform the operation synchronously. Used to simplify client code paths
	FSASYNC_FLAGS_NULLTERMINATE		= ( 1 << 5 ),	// allocate an extra byte and null terminate the buffer read in
};

//---------------------------------------------------------
// Optional completion callback for each async file serviced (or failed)
// call is not reentrant, async i/o guaranteed suspended until return
// Note: If you change the signature of the callback, you will have to account for it in FileSystemV12 (toml [4/18/2005] )
//---------------------------------------------------------
struct FileAsyncRequest_t;
typedef void (*FSAsyncCallbackFunc_t)(const FileAsyncRequest_t &request, int nBytesRead, FSAsyncStatus_t err);

//---------------------------------------------------------
// Description of an async request
//---------------------------------------------------------
struct FileAsyncRequest_t
{
	FileAsyncRequest_t()	{ memset( this, 0, sizeof(*this) ); hSpecificAsyncFile = FS_INVALID_ASYNC_FILE;	}
	const char *			pszFilename;		// file system name
	void *					pData;				// optional, system will alloc/free if NULL
	int						nOffset;			// optional initial seek_set, 0=beginning
	int						nBytes;				// optional read clamp, -1=exist test, 0=full read
	FSAsyncCallbackFunc_t	pfnCallback;		// optional completion callback
	void *					pContext;			// caller's unique file identifier
	int						priority;			// inter list priority, 0=lowest
	unsigned				flags;				// behavior modifier
	const char *			pszPathID;			// path ID (NOTE: this field is here to remain binary compatible with release HL2 filesystem interface)
	FSAsyncFile_t			hSpecificAsyncFile; // Optional hint obtained using AsyncBeginRead()
	FSAllocFunc_t			pfnAlloc;			// custom allocator. can be null. not compatible with FSASYNC_FLAGS_FREEDATAPTR
};


//-----------------------------------------------------------------------------
// Base file system interface
//-----------------------------------------------------------------------------

// This is the minimal interface that can be implemented to provide access to
// a named set of files.
#define BASEFILESYSTEM_INTERFACE_VERSION		"VBaseFileSystem011"

abstract_class IBaseFileSystem
{
public:
	virtual int				Read( void* pOutput, int size, FileHandle_t file ) = 0;
	virtual int				Write( void const* pInput, int size, FileHandle_t file ) = 0;

	// if pathID is NULL, all paths will be searched for the file
	virtual FileHandle_t	Open( const char *pFileName, const char *pOptions, const char *pathID = 0 ) = 0;
	virtual void			Close( FileHandle_t file ) = 0;


	virtual void			Seek( FileHandle_t file, int pos, FileSystemSeek_t seekType ) = 0;
	virtual unsigned int	Tell( FileHandle_t file ) = 0;
	virtual unsigned int	Size( FileHandle_t file ) = 0;
	virtual unsigned int	Size( const char *pFileName, const char *pPathID = 0 ) = 0;

	virtual void			Flush( FileHandle_t file ) = 0;
	virtual bool			Precache( const char *pFileName, const char *pPathID = 0 ) = 0;

	virtual bool			FileExists( const char *pFileName, const char *pPathID = 0 ) = 0;
	virtual bool			IsFileWritable( char const *pFileName, const char *pPathID = 0 ) = 0;
	virtual bool			SetFileWritable( char const *pFileName, bool writable, const char *pPathID = 0 ) = 0;

	virtual long			GetFileTime( const char *pFileName, const char *pPathID = 0 ) = 0;

	//--------------------------------------------------------
	// Reads/writes files to utlbuffers. Use this for optimal read performance when doing open/read/close
	//--------------------------------------------------------
	virtual bool			ReadFile( const char *pFileName, const char *pPath, CUtlBuffer &buf, int nMaxBytes = 0, int nStartingByte = 0, FSAllocFunc_t pfnAlloc = NULL ) = 0;
	virtual bool			WriteFile( const char *pFileName, const char *pPath, CUtlBuffer &buf ) = 0;

#ifdef _XBOX
	virtual bool			IsFileOnLocalHDD( const char *pFileName, const char *pPath, int &pathID, unsigned int &offset, unsigned int &length ) = 0; 
	virtual bool			GetSearchPathFromId( int pathID, char *pPath, int nPathLen, bool &bIsPackFile ) = 0;
#endif
};


//-----------------------------------------------------------------------------
// Main file system interface
//-----------------------------------------------------------------------------

#define FILESYSTEM_INTERFACE_VERSION			"VFileSystem017"

abstract_class IFileSystem : public IAppSystem, public IBaseFileSystem
{
public:
	//--------------------------------------------------------
	// Optional hooks to notify file system that application is initializing,
	// needed if want to use convars in filesystem
	//--------------------------------------------------------
	virtual bool			ConnectApp( CreateInterfaceFn factory ) = 0;
	virtual InitReturnVal_t	InitApp() = 0;
	virtual void			ShutdownApp() = 0;
	virtual void			DisconnectApp() = 0;

	//--------------------------------------------------------
	// Steam operations
	//--------------------------------------------------------

	virtual bool			IsSteam() const = 0;

	// Supplying an extra app id will mount this app in addition 
	// to the one specified in the environment variable "steamappid"
	// 
	// If nExtraAppId is < -1, then it will mount that app ID only.
	// (Was needed by the dedicated server b/c the "SteamAppId" env var only gets passed to steam.dll
	// at load time, so the dedicated couldn't pass it in that way).
	virtual	FilesystemMountRetval_t MountSteamContent( int nExtraAppId = -1 ) = 0;

	//--------------------------------------------------------
	// Search path manipulation
	//--------------------------------------------------------

	// Add paths in priority order (mod dir, game dir, ....)
	// If one or more .pak files are in the specified directory, then they are
	//  added after the file system path
	// If the path is the relative path to a .bsp file, then any previous .bsp file 
	//  override is cleared and the current .bsp is searched for an embedded PAK file
	//  and this file becomes the highest priority search path ( i.e., it's looked at first
	//   even before the mod's file system path ).
	virtual void			AddSearchPath( const char *pPath, const char *pathID, SearchPathAdd_t addType = PATH_ADD_TO_TAIL ) = 0;
	virtual bool			RemoveSearchPath( const char *pPath, const char *pathID = 0 ) = 0;

	// Remove all search paths (including write path?)
	virtual void			RemoveAllSearchPaths( void ) = 0;

	// Remove search paths associated with a given pathID
	virtual void			RemoveSearchPaths( const char *szPathID ) = 0;

	// This is for optimization. If you mark a path ID as "by request only", then files inside it
	// will only be accessed if the path ID is specifically requested. Otherwise, it will be ignored.
	// If there are currently no search paths with the specified path ID, then it will still
	// remember it in case you add search paths with this path ID.
	virtual void			MarkPathIDByRequestOnly( const char *pPathID, bool bRequestOnly ) = 0;

	// converts a partial path into a full path
	virtual const char		*RelativePathToFullPath( const char *pFileName, const char *pPathID, char *pLocalPath, int localPathBufferSize ) = 0;

	// Returns the search path, each path is separated by ;s. Returns the length of the string returned
	virtual int				GetSearchPath( const char *pathID, bool bGetPackFiles, char *pPath, int nMaxLen ) = 0;

	// interface for custom pack files > 4Gb
	virtual bool			AddPackFile( const char *fullpath, const char *pathID ) = 0;

	//--------------------------------------------------------
	// File manipulation operations
	//--------------------------------------------------------

	// Deletes a file (on the WritePath)
	virtual void			RemoveFile( char const* pRelativePath, const char *pathID = 0 ) = 0;

	// Renames a file (on the WritePath)
	virtual void			RenameFile( char const *pOldPath, char const *pNewPath, const char *pathID = 0 ) = 0;

	// create a local directory structure
	virtual void			CreateDirHierarchy( const char *path, const char *pathID = 0 ) = 0;
	
	virtual bool			UnknownFunc1( const char *path, const char *pathID = 0 ) = 0;

	// File I/O and info
	virtual bool			IsDirectory( const char *pFileName, const char *pathID = 0 ) = 0;

	virtual void			FileTimeToString( char* pStrip, int maxCharsIncludingTerminator, long fileTime ) = 0;

	//--------------------------------------------------------
	// Open file operations
	//--------------------------------------------------------

	virtual void			SetBufferSize( FileHandle_t file, unsigned nBytes ) = 0;

	virtual bool			IsOk( FileHandle_t file ) = 0;

	virtual bool			EndOfFile( FileHandle_t file ) = 0;

	virtual char			*ReadLine( char *pOutput, int maxChars, FileHandle_t file ) = 0;
	virtual int				FPrintf( FileHandle_t file, char *pFormat, ... ) = 0;

	//--------------------------------------------------------
	// Dynamic library operations
	//--------------------------------------------------------

	// load/unload modules
	virtual CSysModule 		*LoadModule( const char *pFileName, const char *pPathID = 0, bool bValidatedDllOnly = true ) = 0;
	virtual void			UnloadModule( CSysModule *pModule ) = 0;

	//--------------------------------------------------------
	// File searching operations
	//--------------------------------------------------------

	// FindFirst/FindNext. Also see FindFirstEx.
	virtual const char		*FindFirst( const char *pWildCard, FileFindHandle_t *pHandle ) = 0;
	virtual const char		*FindNext( FileFindHandle_t handle ) = 0;
	virtual bool			FindIsDirectory( FileFindHandle_t handle ) = 0;
	virtual void			FindClose( FileFindHandle_t handle ) = 0;

	// Same as FindFirst, but you can filter by path ID, which can make it faster.
	virtual const char		*FindFirstEx( 
		const char *pWildCard, 
		const char *pPathID,
		FileFindHandle_t *pHandle
		) = 0;

	//--------------------------------------------------------
	// File name and directory operations
	//--------------------------------------------------------

	// FIXME: This method is obsolete! Use RelativePathToFullPath instead!
	// converts a partial path into a full path
	virtual const char		*GetLocalPath( const char *pFileName, char *pLocalPath, int localPathBufferSize ) = 0;

	// Returns true on success ( based on current list of search paths, otherwise false if 
	//  it can't be resolved )
	virtual bool			FullPathToRelativePath( const char *pFullpath, char *pRelative, int maxlen ) = 0;

	// Gets the current working directory
	virtual bool			GetCurrentDirectory( char* pDirectory, int maxlen ) = 0;

	//--------------------------------------------------------
	// Filename dictionary operations
	//--------------------------------------------------------

	virtual FileNameHandle_t	FindOrAddFileName( char const *pFileName ) = 0;
	virtual bool				String( const FileNameHandle_t& handle, char *buf, int buflen ) = 0;

	//--------------------------------------------------------
	// Asynchronous file operations
	//--------------------------------------------------------

	//------------------------------------
	// Global operations
	//------------------------------------
			FSAsyncStatus_t	AsyncRead( const FileAsyncRequest_t &request, FSAsyncControl_t *phControl = NULL )	{ return AsyncReadMultiple( &request, 1, phControl ); 	}
	virtual FSAsyncStatus_t	AsyncReadMultiple( const FileAsyncRequest_t *pRequests, int nRequests,  FSAsyncControl_t *phControls = NULL ) = 0;
	virtual FSAsyncStatus_t	AsyncAppend(const char *pFileName, const void *pSrc, int nSrcBytes, bool bFreeMemory, FSAsyncControl_t *pControl = NULL ) = 0;
	virtual FSAsyncStatus_t	AsyncAppendFile(const char *pAppendToFileName, const char *pAppendFromFileName, FSAsyncControl_t *pControl = NULL ) = 0;
	virtual void			AsyncFinishAll( int iToPriority = INT_MIN ) = 0;
	virtual void			AsyncFinishAllWrites() = 0;
	virtual FSAsyncStatus_t	AsyncFlush() = 0;
	virtual bool			AsyncSuspend() = 0;
	virtual bool			AsyncResume() = 0;

	//------------------------------------
	// Functions to hold a file open if planning on doing mutiple reads. Use is optional,
	// and is taken only as a hint
	//------------------------------------
	virtual FSAsyncStatus_t	AsyncBeginRead( const char *pszFile, FSAsyncFile_t *phFile ) = 0;
	virtual FSAsyncStatus_t	AsyncEndRead( FSAsyncFile_t hFile ) = 0;

	//------------------------------------
	// Request management
	//------------------------------------
	virtual FSAsyncStatus_t	AsyncFinish( FSAsyncControl_t hControl, bool wait = true ) = 0;
	virtual FSAsyncStatus_t	AsyncGetResult( FSAsyncControl_t hControl, void **ppData, int *pSize ) = 0;
	virtual FSAsyncStatus_t	AsyncAbort( FSAsyncControl_t hControl ) = 0;
	virtual FSAsyncStatus_t	AsyncStatus( FSAsyncControl_t hControl ) = 0;
	// set a new priority for a file already in the queue
	virtual FSAsyncStatus_t	AsyncSetPriority(FSAsyncControl_t hControl, int newPriority) = 0;
	virtual void			AsyncAddRef( FSAsyncControl_t hControl ) = 0;
	virtual void			AsyncRelease( FSAsyncControl_t hControl ) = 0;

	//--------------------------------------------------------
	// Remote resource management
	//--------------------------------------------------------

	// starts waiting for resources to be available
	// returns FILESYSTEM_INVALID_HANDLE if there is nothing to wait on
	virtual WaitForResourcesHandle_t WaitForResources( const char *resourcelist ) = 0;
	// get progress on waiting for resources; progress is a float [0, 1], complete is true on the waiting being done
	// returns false if no progress is available
	// any calls after complete is true or on an invalid handle will return false, 0.0f, true
	virtual bool			GetWaitForResourcesProgress( WaitForResourcesHandle_t handle, float *progress /* out */ , bool *complete /* out */ ) = 0;
	// cancels a progress call
	virtual void			CancelWaitForResources( WaitForResourcesHandle_t handle ) = 0;

	// hints that a set of files will be loaded in near future
	// HintResourceNeed() is not to be confused with resource precaching.
	virtual int				HintResourceNeed( const char *hintlist, int forgetEverything ) = 0;
	// returns true if a file is on disk
	virtual bool			IsFileImmediatelyAvailable(const char *pFileName) = 0;
	
	// copies file out of pak/bsp/steam cache onto disk (to be accessible by third-party code)
	virtual void			GetLocalCopy( const char *pFileName ) = 0;

	//--------------------------------------------------------
	// Debugging operations
	//--------------------------------------------------------

	// Dump to printf/OutputDebugString the list of files that have not been closed
	virtual void			PrintOpenedFiles( void ) = 0;
	virtual void			PrintSearchPaths( void ) = 0;

	// output
	virtual void			SetWarningFunc( void (*pfnWarning)( const char *fmt, ... ) ) = 0;
	virtual void			SetWarningLevel( FileWarningLevel_t level ) = 0;
	virtual void			AddLoggingFunc( void (*pfnLogFunc)( const char *fileName, const char *accessType ) ) = 0;
	virtual void			RemoveLoggingFunc( FileSystemLoggingFunc_t logFunc ) = 0;

	// Returns the file system statistics retreived by the implementation.  Returns NULL if not supported.
	virtual const FileSystemStatistics *GetFilesystemStatistics() = 0;
};

//-----------------------------------------------------------------------------

#if defined(_XBOX) && !defined(_RETAIL)
extern char g_szXBOXProfileLastFileOpened[1024];
#define SetLastProfileFileRead( s ) Q_strcpy( g_szXBOXProfileLastFileOpened, pFileName )
#define GetLastProfileFileRead() (&g_szXBOXProfileLastFileOpened[0])
#else
#define SetLastProfileFileRead( s ) ((void)0)
#define GetLastProfileFileRead() NULL
#endif

#if defined(_XBOX) && defined(_BASETSD_H_)
class CXboxDiskCacheSetter
{
public:
	CXboxDiskCacheSetter( SIZE_T newSize )
	{
		m_oldSize = XGetFileCacheSize();
		XSetFileCacheSize( newSize );
	}

	~CXboxDiskCacheSetter()
	{
		XSetFileCacheSize( m_oldSize );
	}
private:
	SIZE_T m_oldSize;
};
#define DISK_INTENSIVE() CXboxDiskCacheSetter cacheSetter( 1024*1024 )
#else
#define DISK_INTENSIVE() ((void)0)
#endif

//-----------------------------------------------------------------------------

// We include this here so it'll catch compile errors in VMPI early.
#include "filesystem_passthru.h"

//-----------------------------------------------------------------------------
// Async memory tracking
//-----------------------------------------------------------------------------

#if (defined(_DEBUG) || defined(USE_MEM_DEBUG))
#define AsyncRead( a, b ) AsyncReadCreditAlloc( a, __FILE__, __LINE__, b )
#define AsyncReadMutiple( a, b, c ) AsyncReadMultipleCreditAlloc( a, b, __FILE__, __LINE__, c )
#endif

#endif // FILESYSTEM_H
