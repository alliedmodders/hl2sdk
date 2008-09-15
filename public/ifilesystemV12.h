//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef IFILESYSTEM_V12_H
#define IFILESYSTEM_V12_H
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"
#include "appframework/IAppSystem.h"
#ifdef _LINUX
#include <limits.h> // PATH_MAX define
#endif

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CUtlBuffer;

namespace FileSystemV12
{

typedef void * FileHandle_t;
typedef int FileFindHandle_t;
typedef void (*FileSystemLoggingFunc_t)( const char *fileName, const char *accessType );
typedef int WaitForResourcesHandle_t;

// The handle is a CUtlSymbol for the dirname and the same for the filename, the accessor
//  copies them into a static char buffer for return.
typedef void* FileNameHandle_t;


//-----------------------------------------------------------------------------
// Enums used by the interface
//-----------------------------------------------------------------------------
enum FileSystemSeek_t
{
	FILESYSTEM_SEEK_HEAD = 0,
	FILESYSTEM_SEEK_CURRENT,
	FILESYSTEM_SEEK_TAIL,
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
	size_t nReads,		
		   nWrites,		
		   nBytesRead,
		   nBytesWritten,
		   nSeeks;
};

//-----------------------------------------------------------------------------
// Main file system interface
//-----------------------------------------------------------------------------

// This is the minimal interface that can be implemented to provide access to
// a named set of files.
#define BASEFILESYSTEM_INTERFACE_VERSION_7		"VBaseFileSystem007"

// async file status
enum fsasync_t
{
	FSASYNC_ERR_FAILURE      = -5,	// hard subsystem failure
	FSASYNC_ERR_READING      = -4,	// read error on file
	FSASYNC_ERR_NOMEMORY     = -3,	// out of memory for file read
	FSASYNC_ERR_UNKNOWNID    = -2,	// caller's provided id is not recognized
	FSASYNC_ERR_FILEOPEN     = -1,	// filename could not be opened (bad path, not exist, etc)
	FSASYNC_OK               = 0,	// operation is successful
	FSASYNC_STATUS_PENDING,			// file is properly queued, waiting for service
	FSASYNC_STATUS_READING,			// file is being accessed
	FSASYNC_STATUS_ABORTED,			// file was aborted by caller
};

// modifier flags
enum
{
	FSASYNC_FLAGS_ALLOCNOFREE		= 0x01,	// do the allocation for dataPtr, but don't free
	FSASYNC_FLAGS_NOSEARCHPATHS		= 0x02,	// filename is absolute
};

struct asyncFileList_s;
typedef struct asyncFileList_s	asyncFileList_t;

// optional completion callback for each async file serviced (or failed)
// call is not reentrant, async i/o guaranteed suspended until return
typedef void (*asyncFileCallbackPtr_t)(const asyncFileList_t* asyncFilePtr, int numReadBytes, fsasync_t err);

// caller provided params
typedef struct asyncFileList_s
{
	char					*filename;			// file system name
	void					*dataPtr;			// optional, system will alloc/free if NULL
	int						fileStartOffset;	// optional initial seek_set, 0=beginning
	int						maxDataBytes;		// optional read clamp, -1=exist test, 0=full read
	asyncFileCallbackPtr_t	callbackPtr;		// optional completion callback
	int						asyncID;			// caller's unique file identifier
	int						priority;			// inter list priority, 0=lowest
	int						flags;				// behavior modifier
} asyncFileList_t;

typedef struct asyncFileNode_s
{
	asyncFileList_t*	fileListPtr;
	asyncFileNode_s*	nextPtr;
	fsasync_t			status;
} asyncFileNode_t;

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
};



#define FILESYSTEM_INTERFACE_VERSION_12			"VFileSystem012"
abstract_class IFileSystem : public IBaseFileSystem, public IAppSystemV0
{
public:
	// Remove all search paths (including write path?)
	virtual void			RemoveAllSearchPaths( void ) = 0;

	// Add paths in priority order (mod dir, game dir, ....)
	// If one or more .pak files are in the specified directory, then they are
	//  added after the file system path
	// If the path is the relative path to a .bsp file, then any previous .bsp file 
	//  override is cleared and the current .bsp is searched for an embedded PAK file
	//  and this file becomes the highest priority search path ( i.e., it's looked at first
	//   even before the mod's file system path ).
	virtual void			AddSearchPath( const char *pPath, const char *pathID, SearchPathAdd_t addType = PATH_ADD_TO_TAIL ) = 0;
	virtual bool			RemoveSearchPath( const char *pPath, const char *pathID = 0 ) = 0;

	// Deletes a file (on the WritePath)
	virtual void			RemoveFile( char const* pRelativePath, const char *pathID = 0 ) = 0;

	// Renames a file (on the WritePath)
	virtual void			RenameFile( char const *pOldPath, char const *pNewPath, const char *pathID = 0 ) = 0;

	// create a local directory structure
	virtual void			CreateDirHierarchy( const char *path, const char *pathID = 0 ) = 0;

	// File I/O and info
	virtual bool			IsDirectory( const char *pFileName, const char *pathID = 0 ) = 0;

	virtual void			FileTimeToString( char* pStrip, int maxCharsIncludingTerminator, long fileTime ) = 0;

	virtual bool			IsOk( FileHandle_t file ) = 0;

	virtual bool			EndOfFile( FileHandle_t file ) = 0;

	virtual char			*ReadLine( char *pOutput, int maxChars, FileHandle_t file ) = 0;
	virtual int				FPrintf( FileHandle_t file, char *pFormat, ... ) = 0;

	// load/unload modules
	virtual CSysModule 		*LoadModule( const char *pFileName, const char *pPathID = 0, bool bValidatedDllOnly = true ) = 0;
	virtual void			UnloadModule( CSysModule *pModule ) = 0;

	// FindFirst/FindNext
	virtual const char		*FindFirst( const char *pWildCard, FileFindHandle_t *pHandle ) = 0;
	virtual const char		*FindNext( FileFindHandle_t handle ) = 0;
	virtual bool			FindIsDirectory( FileFindHandle_t handle ) = 0;
	virtual void			FindClose( FileFindHandle_t handle ) = 0;

	// converts a partial path into a full path
	virtual const char		*GetLocalPath( const char *pFileName, char *pLocalPath, int localPathBufferSize ) = 0;

	// Returns true on success ( based on current list of search paths, otherwise false if 
	//  it can't be resolved )
	virtual bool			FullPathToRelativePath( const char *pFullpath, char *pRelative, int maxlen ) = 0;

	// Gets the current working directory
	virtual bool			GetCurrentDirectory( char* pDirectory, int maxlen ) = 0;

	// Dump to printf/OutputDebugString the list of files that have not been closed
	virtual void			PrintOpenedFiles( void ) = 0;
	virtual void			PrintSearchPaths( void ) = 0;

	// output
	virtual void			SetWarningFunc( void (*pfnWarning)( const char *fmt, ... ) ) = 0;
	virtual void			SetWarningLevel( FileWarningLevel_t level ) = 0;
	virtual void			AddLoggingFunc( void (*pfnLogFunc)( const char *fileName, const char *accessType ) ) = 0;
	virtual void			RemoveLoggingFunc( FileSystemLoggingFunc_t logFunc ) = 0;

	// asynchronous file loading
	virtual fsasync_t			AsyncFilePrefetch(int numFiles, const asyncFileList_t *fileListPtr) = 0;
	virtual fsasync_t			AsyncFileFinish(int asyncID, bool wait) = 0;
	virtual fsasync_t			AsyncFileAbort(int asyncID) = 0;
	virtual fsasync_t			AsyncFileStatus(int asyncID) = 0;
	virtual fsasync_t			AsyncFlush() = 0;

	// Returns the file system statistics retreived by the implementation.  Returns NULL if not supported.
	virtual const FileSystemStatistics *GetFilesystemStatistics() = 0;

	// remote resource management
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

	virtual FileNameHandle_t	FindOrAddFileName( char const *pFileName ) = 0;
	virtual bool				String( const FileNameHandle_t& handle, char *buf, int buflen ) = 0;

	//
	// new functions appended after "VFileSystem012" first release (but without a version roll!)
	//
	//
	virtual bool			IsOk2( FileHandle_t file ) = 0;
	virtual void			RemoveSearchPaths( const char *szPathID ) = 0;

	virtual bool			IsSteam() const = 0;

	// Supplying an extra app id will mount this app in addition 
	// to the one specified in the environment variable "steamappid"
	virtual	FilesystemMountRetval_t MountSteamContent( int nExtraAppId = -1 ) = 0;

	// Same as FindFirst, but you can filter by path ID, which can make it faster.
	virtual const char		*FindFirstEx( 
		const char *pWildCard, 
		const char *pPathID,
		FileFindHandle_t *pHandle
		) = 0;

	// This is for optimization. If you mark a path ID as "by request only", then files inside it
	// will only be accessed if the path ID is specifically requested. Otherwise, it will be ignored.
	//
	// If there are currently no search paths with the specified path ID, then it will still
	// remember it in case you add search paths with this path ID.
	virtual void			MarkPathIDByRequestOnly( const char *pPathID, bool bRequestOnly ) = 0;

	// set a new priority for a file already in the queue
	virtual fsasync_t		AsyncFileSetPriority(int asyncID, int newPriority) = 0;

	// interface for custom pack files > 4Gb
	virtual bool			AddPackFile( const char *fullpath, const char *pathID ) = 0;

	// async write (should be moved up next to the other async calls next time the interface is rev'd)
	virtual fsasync_t		AsyncFileWrite(const char *pFileName, const void *pSrc, int nSrcBytes, bool bFreeMemory) = 0;
	// appends the latter file to the former
	virtual fsasync_t		AsyncFileAppendFile(const char *pDestFileName, const char *pSrcFileName) = 0;

	// blocks until all async file writes are completed
	virtual void			AsyncFileFinishAllWrites() = 0;
};

} // end namespace


#endif // IFILESYSTEM_V12_H
