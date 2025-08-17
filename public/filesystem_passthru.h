//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef FILESYSTEM_PASSTHRU_H
#define FILESYSTEM_PASSTHRU_H
#ifdef _WIN32
#pragma once
#endif


#include "filesystem.h"
#include <stdio.h>
#include <stdarg.h>

#ifdef AsyncRead
#undef AsyncRead
#undef AsyncReadMutiple
#endif

//
// These classes pass all filesystem interface calls through to another filesystem
// interface. They can be used anytime you want to override a couple things in 
// a filesystem. VMPI uses this to override the base filesystem calls while 
// allowing the rest of the filesystem functionality to work on the master.
//

template<class Base>
class CInternalFileSystemPassThru : public Base
{
public:
	CInternalFileSystemPassThru()
	{
		m_pBaseFileSystemPassThru = NULL;
	}
	virtual void InitPassThru( IBaseFileSystem *pBaseFileSystemPassThru )
	{
		m_pBaseFileSystemPassThru = pBaseFileSystemPassThru;
	}
	int				Read( void* pOutput, int size, FileHandle_t file ) override							{ return m_pBaseFileSystemPassThru->Read( pOutput, size, file ); }
	int				Write( void const* pInput, int size, FileHandle_t file ) override					{ return m_pBaseFileSystemPassThru->Write( pInput, size, file ); }
	FileHandle_t	Open( const char *pFileName, const char *pOptions, const char *pathID ) override		{ return m_pBaseFileSystemPassThru->Open( pFileName, pOptions, pathID ); }
	void			Close( FileHandle_t file ) override													{ m_pBaseFileSystemPassThru->Close( file ); }
	void			Seek( FileHandle_t file, int pos, FileSystemSeek_t seekType ) override				{ m_pBaseFileSystemPassThru->Seek( file, pos, seekType ); }
	unsigned int	Tell( FileHandle_t file ) override													{ return m_pBaseFileSystemPassThru->Tell( file ); }
	unsigned int	Size( FileHandle_t file ) override													{ return m_pBaseFileSystemPassThru->Size( file ); }
	unsigned int	Size( const char *pFileName, const char *pPathID ) override							{ return m_pBaseFileSystemPassThru->Size( pFileName, pPathID ); }
	void			Flush( FileHandle_t file ) override													{ m_pBaseFileSystemPassThru->Flush( file ); }
	bool			Precache( const char *pFileName, const char *pPathID ) override						{ return m_pBaseFileSystemPassThru->Precache( pFileName, pPathID ); }
	bool			FileExists( const char *pFileName, const char *pPathID ) override					{ return m_pBaseFileSystemPassThru->FileExists( pFileName, pPathID ); }
	bool			IsFileWritable( char const *pFileName, const char *pPathID ) override					{ return m_pBaseFileSystemPassThru->IsFileWritable( pFileName, pPathID ); }
	bool			SetFileWritable( char const *pFileName, bool writable, const char *pPathID ) override	{ return m_pBaseFileSystemPassThru->SetFileWritable( pFileName, writable, pPathID ); }
	long			GetFileTime( const char *pFileName, const char *pPathID ) override						{ return m_pBaseFileSystemPassThru->GetFileTime( pFileName, pPathID ); }
	bool			ReadFile( const char *pFileName, const char *pPath, CUtlBuffer &buf, int nMaxBytes = 0, int nStartingByte = 0, FSAllocFunc_t pfnAlloc = NULL ) override { return m_pBaseFileSystemPassThru->ReadFile( pFileName, pPath, buf, nMaxBytes, nStartingByte, pfnAlloc  ); }
	bool			WriteFile( const char *pFileName, const char *pPath, CUtlBuffer &buf ) override		{  return m_pBaseFileSystemPassThru->WriteFile( pFileName, pPath, buf ); }
	bool			UnzipFile( const char *pFileName, const char *pPath, const char *pDestination ) override		{  return m_pBaseFileSystemPassThru->UnzipFile( pFileName, pPath, pDestination ); }

protected:
	IBaseFileSystem *m_pBaseFileSystemPassThru;
};


class CBaseFileSystemPassThru : public CInternalFileSystemPassThru<IBaseFileSystem>
{
public:
};


class CFileSystemPassThru : public CInternalFileSystemPassThru<IFileSystem>
{
public:
	typedef CInternalFileSystemPassThru<IFileSystem> BaseClass;

	CFileSystemPassThru()
	{
		m_pFileSystemPassThru = NULL;
	}
	virtual void InitPassThru( IFileSystem *pFileSystemPassThru, bool bBaseOnly )
	{
		if ( !bBaseOnly )
			m_pFileSystemPassThru = pFileSystemPassThru;
		
		BaseClass::InitPassThru( pFileSystemPassThru );
	}

	// IAppSystem stuff.
	// Here's where the app systems get to learn about each other 
	bool Connect( CreateInterfaceFn factory ) override															{ return m_pFileSystemPassThru->Connect( factory ); }
	void Disconnect() override																					{ m_pFileSystemPassThru->Disconnect(); }
	void *QueryInterface( const char *pInterfaceName ) override													{ return m_pFileSystemPassThru->QueryInterface( pInterfaceName ); }
	InitReturnVal_t Init() override																				{ return m_pFileSystemPassThru->Init(); }
	void Shutdown() override																						{ m_pFileSystemPassThru->Shutdown(); }

	void			RemoveAllSearchPaths( void ) override														{ m_pFileSystemPassThru->RemoveAllSearchPaths(); }
	void			AddSearchPath( const char *pPath, const char *pathID, SearchPathAdd_t addType ) override		{ m_pFileSystemPassThru->AddSearchPath( pPath, pathID, addType ); }
	bool			RemoveSearchPath( const char *pPath, const char *pathID ) override							{ return m_pFileSystemPassThru->RemoveSearchPath( pPath, pathID ); }
	void			RemoveFile( char const* pRelativePath, const char *pathID ) override							{ m_pFileSystemPassThru->RemoveFile( pRelativePath, pathID ); }
	bool			RenameFile( char const *pOldPath, char const *pNewPath, const char *pathID ) override		{ return m_pFileSystemPassThru->RenameFile( pOldPath, pNewPath, pathID ); }
	void			CreateDirHierarchy( const char *path, const char *pathID ) override							{ m_pFileSystemPassThru->CreateDirHierarchy( path, pathID ); }
	bool			IsDirectory( const char *pFileName, const char *pathID ) override							{ return m_pFileSystemPassThru->IsDirectory( pFileName, pathID ); }
	void			FileTimeToString( char* pStrip, int maxCharsIncludingTerminator, long fileTime ) override	{ m_pFileSystemPassThru->FileTimeToString( pStrip, maxCharsIncludingTerminator, fileTime ); }
	void			SetBufferSize( FileHandle_t file, unsigned nBytes ) override									{ m_pFileSystemPassThru->SetBufferSize( file, nBytes  ); }
	bool			IsOk( FileHandle_t file ) override															{ return m_pFileSystemPassThru->IsOk( file ); }
	bool			EndOfFile( FileHandle_t file ) override														{ return m_pFileSystemPassThru->EndOfFile( file ); }
	char			*ReadLine( char *pOutput, int maxChars, FileHandle_t file ) override							{ return m_pFileSystemPassThru->ReadLine( pOutput, maxChars, file ); }
	int				FPrintf( FileHandle_t file, PRINTF_FORMAT_STRING const char *pFormat, ... ) override 
	{ 
		char str[8192];
		va_list marker;
		va_start( marker, pFormat );
		_vsnprintf( str, sizeof( str ), pFormat, marker );
		va_end( marker );
		return m_pFileSystemPassThru->FPrintf( file, "%s", str );
	}
	CSysModule 		*LoadModule( const char *pFileName, const char *pPathID, bool bValidatedDllOnly ) override	{ return m_pFileSystemPassThru->LoadModule( pFileName, pPathID, bValidatedDllOnly ); }
	void			UnloadModule( CSysModule *pModule ) override													{ m_pFileSystemPassThru->UnloadModule( pModule ); }
	const char		*FindFirst( const char *pWildCard, FileFindHandle_t *pHandle ) override						{ return m_pFileSystemPassThru->FindFirst( pWildCard, pHandle ); }
	const char		*FindNext( FileFindHandle_t handle ) override												{ return m_pFileSystemPassThru->FindNext( handle ); }
	bool			FindIsDirectory( FileFindHandle_t handle ) override											{ return m_pFileSystemPassThru->FindIsDirectory( handle ); }
	void			FindClose( FileFindHandle_t handle ) override												{ m_pFileSystemPassThru->FindClose( handle ); }
	const char		*GetLocalPath( const char *pFileName, OUT_Z_CAP(maxLenInChars) char *pDest, int maxLenInChars ) override	{ return m_pFileSystemPassThru->GetLocalPath( pFileName, pDest, maxLenInChars ); }
	bool			FullPathToRelativePath( const char *pFullpath, OUT_Z_CAP(maxLenInChars) char *pDest, int maxLenInChars ) override		{ return m_pFileSystemPassThru->FullPathToRelativePath( pFullpath, pDest, maxLenInChars ); }
	bool			GetCaseCorrectFullPath_Ptr( const char *pFullPath, OUT_Z_CAP(maxLenInChars) char *pDest, int maxLenInChars ) override { return m_pFileSystemPassThru->GetCaseCorrectFullPath_Ptr( pFullPath, pDest, maxLenInChars ); }
	bool			GetCurrentDirectory( char* pDirectory, int maxlen ) override									{ return m_pFileSystemPassThru->GetCurrentDirectory( pDirectory, maxlen ); }
	void			PrintOpenedFiles( void ) override															{ m_pFileSystemPassThru->PrintOpenedFiles(); }
	void			PrintSearchPaths( void ) override															{ m_pFileSystemPassThru->PrintSearchPaths(); }
	void			SetWarningFunc( void (*pfnWarning)( PRINTF_FORMAT_STRING const char *fmt, ... ) ) override						{ m_pFileSystemPassThru->SetWarningFunc( pfnWarning ); }
	void			SetWarningLevel( FileWarningLevel_t level ) override											{ m_pFileSystemPassThru->SetWarningLevel( level ); } 
	void			AddLoggingFunc( void (*pfnLogFunc)( const char *fileName, const char *accessType ) ) override{ m_pFileSystemPassThru->AddLoggingFunc( pfnLogFunc ); }
	void			RemoveLoggingFunc( FileSystemLoggingFunc_t logFunc ) override								{ m_pFileSystemPassThru->RemoveLoggingFunc( logFunc ); }
	FSAsyncStatus_t	AsyncReadMultiple( const FileAsyncRequest_t *pRequests, int nRequests, FSAsyncControl_t *pControls ) override			{ return m_pFileSystemPassThru->AsyncReadMultiple( pRequests, nRequests, pControls ); }
	FSAsyncStatus_t	AsyncReadMultipleCreditAlloc( const FileAsyncRequest_t *pRequests, int nRequests, const char *pszFile, int line, FSAsyncControl_t *pControls ) override { return m_pFileSystemPassThru->AsyncReadMultipleCreditAlloc( pRequests, nRequests, pszFile, line, pControls ); }
	FSAsyncStatus_t	AsyncFinish(FSAsyncControl_t hControl, bool wait) override									{ return m_pFileSystemPassThru->AsyncFinish( hControl, wait ); }
	FSAsyncStatus_t	AsyncGetResult( FSAsyncControl_t hControl, void **ppData, int *pSize ) override				{ return m_pFileSystemPassThru->AsyncGetResult( hControl, ppData, pSize ); }
	FSAsyncStatus_t	AsyncAbort(FSAsyncControl_t hControl) override												{ return m_pFileSystemPassThru->AsyncAbort( hControl ); }
	FSAsyncStatus_t	AsyncStatus(FSAsyncControl_t hControl) override												{ return m_pFileSystemPassThru->AsyncStatus( hControl ); }
	FSAsyncStatus_t	AsyncFlush() override																		{ return m_pFileSystemPassThru->AsyncFlush(); }
	void			AsyncAddRef( FSAsyncControl_t hControl ) override											{ m_pFileSystemPassThru->AsyncAddRef( hControl ); }
	void			AsyncRelease( FSAsyncControl_t hControl ) override											{ m_pFileSystemPassThru->AsyncRelease( hControl ); }
	FSAsyncStatus_t	AsyncBeginRead( const char *pszFile, FSAsyncFile_t *phFile ) override						{ return m_pFileSystemPassThru->AsyncBeginRead( pszFile, phFile ); }
	FSAsyncStatus_t	AsyncEndRead( FSAsyncFile_t hFile ) override													{ return m_pFileSystemPassThru->AsyncEndRead( hFile ); }
	void			AsyncAddFetcher( IAsyncFileFetch *pFetcher ) override										{ m_pFileSystemPassThru->AsyncAddFetcher( pFetcher ); }
	void			AsyncRemoveFetcher( IAsyncFileFetch *pFetcher ) override										{ m_pFileSystemPassThru->AsyncRemoveFetcher( pFetcher ); }
	const FileSystemStatistics *GetFilesystemStatistics() override												{ return m_pFileSystemPassThru->GetFilesystemStatistics(); }
	WaitForResourcesHandle_t WaitForResources( const char *resourcelist ) override								{ return m_pFileSystemPassThru->WaitForResources( resourcelist ); }
	bool			GetWaitForResourcesProgress( WaitForResourcesHandle_t handle, 
								float *progress, bool *complete ) override												{ return m_pFileSystemPassThru->GetWaitForResourcesProgress( handle, progress, complete ); }
	void			CancelWaitForResources( WaitForResourcesHandle_t handle ) override							{ m_pFileSystemPassThru->CancelWaitForResources( handle ); }
	int				HintResourceNeed( const char *hintlist, int forgetEverything ) override						{ return m_pFileSystemPassThru->HintResourceNeed( hintlist, forgetEverything ); }
	bool			IsFileImmediatelyAvailable(const char *pFileName) override									{ return m_pFileSystemPassThru->IsFileImmediatelyAvailable( pFileName ); }
	void			GetLocalCopy( const char *pFileName ) override												{ m_pFileSystemPassThru->GetLocalCopy( pFileName ); }
	FileNameHandle_t	FindOrAddFileName( char const *pFileName ) override										{ return m_pFileSystemPassThru->FindOrAddFileName( pFileName ); }
	FileNameHandle_t	FindFileName( char const *pFileName ) override											{ return m_pFileSystemPassThru->FindFileName( pFileName ); }
	bool				String( const FileNameHandle_t& handle, char *buf, int buflen ) override					{ return m_pFileSystemPassThru->String( handle, buf, buflen ); }
	virtual bool			IsOk2( FileHandle_t file )															{ return IsOk(file); }
	void			RemoveSearchPaths( const char *szPathID ) override											{ m_pFileSystemPassThru->RemoveSearchPaths( szPathID ); }
	bool			IsSteam() const override																		{ return m_pFileSystemPassThru->IsSteam(); }
	FilesystemMountRetval_t MountSteamContent( int nExtraAppId = -1 ) override									{ return m_pFileSystemPassThru->MountSteamContent( nExtraAppId ); }
	
	const char		*FindFirstEx( 
		const char *pWildCard, 
		const char *pPathID,
		FileFindHandle_t *pHandle
		) override																										{ return m_pFileSystemPassThru->FindFirstEx( pWildCard, pPathID, pHandle ); }
	void			MarkPathIDByRequestOnly( const char *pPathID, bool bRequestOnly ) override					{ m_pFileSystemPassThru->MarkPathIDByRequestOnly( pPathID, bRequestOnly ); }
	bool			AddPackFile( const char *fullpath, const char *pathID ) override								{ return m_pFileSystemPassThru->AddPackFile( fullpath, pathID ); }
	FSAsyncStatus_t	AsyncAppend(const char *pFileName, const void *pSrc, int nSrcBytes, bool bFreeMemory, FSAsyncControl_t *pControl ) override { return m_pFileSystemPassThru->AsyncAppend( pFileName, pSrc, nSrcBytes, bFreeMemory, pControl); }
	FSAsyncStatus_t	AsyncWrite(const char *pFileName, const void *pSrc, int nSrcBytes, bool bFreeMemory, bool bAppend, FSAsyncControl_t *pControl ) override { return m_pFileSystemPassThru->AsyncWrite( pFileName, pSrc, nSrcBytes, bFreeMemory, bAppend, pControl); }
	FSAsyncStatus_t	AsyncWriteFile(const char *pFileName, const CUtlBuffer *pSrc, int nSrcBytes, bool bFreeMemory, bool bAppend, FSAsyncControl_t *pControl ) override { return m_pFileSystemPassThru->AsyncWriteFile( pFileName, pSrc, nSrcBytes, bFreeMemory, bAppend, pControl); }
	FSAsyncStatus_t	AsyncAppendFile(const char *pDestFileName, const char *pSrcFileName, FSAsyncControl_t *pControl ) override			{ return m_pFileSystemPassThru->AsyncAppendFile(pDestFileName, pSrcFileName, pControl); }
	void			AsyncFinishAll( int iToPriority ) override													{ m_pFileSystemPassThru->AsyncFinishAll(iToPriority); }
	void			AsyncFinishAllWrites() override																{ m_pFileSystemPassThru->AsyncFinishAllWrites(); }
	FSAsyncStatus_t	AsyncSetPriority(FSAsyncControl_t hControl, int newPriority) override						{ return m_pFileSystemPassThru->AsyncSetPriority(hControl, newPriority); }
	bool			AsyncSuspend() override																		{ return m_pFileSystemPassThru->AsyncSuspend(); }
	bool			AsyncResume() override																		{ return m_pFileSystemPassThru->AsyncResume(); }
	const char		*RelativePathToFullPath( const char *pFileName, const char *pPathID, OUT_Z_CAP(maxLenInChars) char *pDest, int maxLenInChars, PathTypeFilter_t pathFilter = FILTER_NONE, PathTypeQuery_t *pPathType = NULL ) override { return m_pFileSystemPassThru->RelativePathToFullPath( pFileName, pPathID, pDest, maxLenInChars, pathFilter, pPathType ); }
	int				GetSearchPath( const char *pathID, bool bGetPackFiles, OUT_Z_CAP(maxLenInChars) char *pDest, int maxLenInChars ) override	{ return m_pFileSystemPassThru->GetSearchPath( pathID, bGetPackFiles, pDest, maxLenInChars ); }

	FileHandle_t	OpenEx( const char *pFileName, const char *pOptions, unsigned flags = 0, const char *pathID = 0, char **ppszResolvedFilename = NULL ) override { return m_pFileSystemPassThru->OpenEx( pFileName, pOptions, flags, pathID, ppszResolvedFilename );}
	int				ReadEx( void* pOutput, int destSize, int size, FileHandle_t file ) override					{ return m_pFileSystemPassThru->ReadEx( pOutput, destSize, size, file ); }
	int				ReadFileEx( const char *pFileName, const char *pPath, void **ppBuf, bool bNullTerminate, bool bOptimalAlloc, int nMaxBytes = 0, int nStartingByte = 0, FSAllocFunc_t pfnAlloc = NULL ) override { return m_pFileSystemPassThru->ReadFileEx( pFileName, pPath, ppBuf, bNullTerminate, bOptimalAlloc, nMaxBytes, nStartingByte, pfnAlloc ); }

#if defined( TRACK_BLOCKING_IO )
	virtual void			EnableBlockingFileAccessTracking( bool state ) { m_pFileSystemPassThru->EnableBlockingFileAccessTracking( state ); }
	virtual bool			IsBlockingFileAccessEnabled() const { return m_pFileSystemPassThru->IsBlockingFileAccessEnabled(); }

	virtual IBlockingFileItemList *RetrieveBlockingFileAccessInfo() { return m_pFileSystemPassThru->RetrieveBlockingFileAccessInfo(); }
#endif
	void			SetupPreloadData() override {}
	void			DiscardPreloadData() override {}

	void			LoadCompiledKeyValues( KeyValuesPreloadType_t type, char const *archiveFile ) override { m_pFileSystemPassThru->LoadCompiledKeyValues( type, archiveFile ); }

	// If the "PreloadedData" hasn't been purged, then this'll try and instance the KeyValues using the fast path of compiled keyvalues loaded during startup.
	// Otherwise, it'll just fall through to the regular KeyValues loading routines
	KeyValues		*LoadKeyValues( KeyValuesPreloadType_t type, char const *filename, char const *pPathID = 0 ) override { return m_pFileSystemPassThru->LoadKeyValues( type, filename, pPathID ); }
	bool			LoadKeyValues( KeyValues& head, KeyValuesPreloadType_t type, char const *filename, char const *pPathID = 0 ) override { return m_pFileSystemPassThru->LoadKeyValues( head, type, filename, pPathID ); }
	bool			ExtractRootKeyName( KeyValuesPreloadType_t type, char *outbuf, size_t bufsize, char const *filename, char const *pPathID = 0 ) override { return m_pFileSystemPassThru->ExtractRootKeyName( type, outbuf, bufsize, filename, pPathID ); }

	bool			GetFileTypeForFullPath( char const *pFullPath, wchar_t *buf, size_t bufSizeInBytes ) override { return m_pFileSystemPassThru->GetFileTypeForFullPath( pFullPath, buf, bufSizeInBytes ); }

	bool			GetOptimalIOConstraints( FileHandle_t hFile, unsigned *pOffsetAlign, unsigned *pSizeAlign, unsigned *pBufferAlign ) override { return m_pFileSystemPassThru->GetOptimalIOConstraints( hFile, pOffsetAlign, pSizeAlign, pBufferAlign ); }
	void			*AllocOptimalReadBuffer( FileHandle_t hFile, unsigned nSize, unsigned nOffset  ) override { return m_pFileSystemPassThru->AllocOptimalReadBuffer( hFile, nOffset, nSize ); }
	void			FreeOptimalReadBuffer( void *p ) override { m_pFileSystemPassThru->FreeOptimalReadBuffer( p ); }

	void			BeginMapAccess() override { m_pFileSystemPassThru->BeginMapAccess(); }
	void			EndMapAccess() override { m_pFileSystemPassThru->EndMapAccess(); }

	bool			ReadToBuffer( FileHandle_t hFile, CUtlBuffer &buf, int nMaxBytes = 0, FSAllocFunc_t pfnAlloc = NULL ) override { return m_pFileSystemPassThru->ReadToBuffer( hFile, buf, nMaxBytes, pfnAlloc ); }
	bool			FullPathToRelativePathEx( const char *pFullPath, const char *pPathId, OUT_Z_CAP(maxLenInChars) char *pDest, int maxLenInChars ) override { return m_pFileSystemPassThru->FullPathToRelativePathEx( pFullPath, pPathId, pDest, maxLenInChars ); }
	int				GetPathIndex( const FileNameHandle_t &handle ) override { return m_pFileSystemPassThru->GetPathIndex( handle ); }
	long			GetPathTime( const char *pPath, const char *pPathID ) override { return m_pFileSystemPassThru->GetPathTime( pPath, pPathID ); }

	DVDMode_t		GetDVDMode() override { return m_pFileSystemPassThru->GetDVDMode(); }

	void EnableWhitelistFileTracking( bool bEnable, bool bCacheAllVPKHashes, bool bRecalculateAndCheckHashes ) override
		{ m_pFileSystemPassThru->EnableWhitelistFileTracking( bEnable, bCacheAllVPKHashes, bRecalculateAndCheckHashes ); }
	void RegisterFileWhitelist( IPureServerWhitelist *pWhiteList, IFileList **ppFilesToReload ) override
		{ m_pFileSystemPassThru->RegisterFileWhitelist( pWhiteList, ppFilesToReload ); }
	void MarkAllCRCsUnverified() override
		{ m_pFileSystemPassThru->MarkAllCRCsUnverified(); }
	void CacheFileCRCs( const char *pPathname, ECacheCRCType eType, IFileList *pFilter ) override
		{ return m_pFileSystemPassThru->CacheFileCRCs( pPathname, eType, pFilter ); }
	EFileCRCStatus CheckCachedFileHash( const char *pPathID, const char *pRelativeFilename, int nFileFraction, FileHash_t *pFileHash ) override
		{ return m_pFileSystemPassThru->CheckCachedFileHash( pPathID, pRelativeFilename, nFileFraction, pFileHash ); }
	int GetUnverifiedFileHashes( CUnverifiedFileHash *pFiles, int nMaxFiles ) override
		{ return m_pFileSystemPassThru->GetUnverifiedFileHashes( pFiles, nMaxFiles ); }
	int GetWhitelistSpewFlags() override
		{ return m_pFileSystemPassThru->GetWhitelistSpewFlags(); }
	void SetWhitelistSpewFlags( int spewFlags ) override
		{ m_pFileSystemPassThru->SetWhitelistSpewFlags( spewFlags ); }
	void InstallDirtyDiskReportFunc( FSDirtyDiskReportFunc_t func ) override { m_pFileSystemPassThru->InstallDirtyDiskReportFunc( func ); }

	FileCacheHandle_t CreateFileCache() override { return m_pFileSystemPassThru->CreateFileCache(); }
	void AddFilesToFileCache( FileCacheHandle_t cacheId, const char **ppFileNames, int nFileNames, const char *pPathID ) override { m_pFileSystemPassThru->AddFilesToFileCache( cacheId, ppFileNames, nFileNames, pPathID ); }
	bool IsFileCacheFileLoaded( FileCacheHandle_t cacheId, const char *pFileName ) override { return m_pFileSystemPassThru->IsFileCacheFileLoaded( cacheId, pFileName ); }
	bool IsFileCacheLoaded( FileCacheHandle_t cacheId ) override { return m_pFileSystemPassThru->IsFileCacheLoaded( cacheId ); }
	void DestroyFileCache( FileCacheHandle_t cacheId ) override { m_pFileSystemPassThru->DestroyFileCache( cacheId ); }

	bool RegisterMemoryFile( CMemoryFileBacking *pFile, CMemoryFileBacking **ppExistingFileWithRef ) override { return m_pFileSystemPassThru->RegisterMemoryFile( pFile, ppExistingFileWithRef ); }
	void UnregisterMemoryFile( CMemoryFileBacking *pFile ) override { m_pFileSystemPassThru->UnregisterMemoryFile( pFile ); }
	void			CacheAllVPKFileHashes( bool bCacheAllVPKHashes, bool bRecalculateAndCheckHashes ) override
		{ return m_pFileSystemPassThru->CacheAllVPKFileHashes( bCacheAllVPKHashes, bRecalculateAndCheckHashes ); }
	bool			CheckVPKFileHash( int PackFileID, int nPackFileNumber, int nFileFraction, MD5Value_t &md5Value ) override
		{ return m_pFileSystemPassThru->CheckVPKFileHash( PackFileID, nPackFileNumber, nFileFraction, md5Value ); }
	void			NotifyFileUnloaded( const char *pszFilename, const char *pPathId ) override
		{ m_pFileSystemPassThru->NotifyFileUnloaded( pszFilename, pPathId ); }

	void			SetWriteProtectionEnable( bool bEnable ) override { m_pFileSystemPassThru->SetWriteProtectionEnable( bEnable ); }
	bool			GetWriteProtectionEnable() const override { return m_pFileSystemPassThru->GetWriteProtectionEnable(); }


protected:
	IFileSystem *m_pFileSystemPassThru;
};


// This is so people who change the filesystem interface are forced to add the passthru wrapper into CFileSystemPassThru,
// so they don't break VMPI.
inline void GiveMeACompileError()
{
	CFileSystemPassThru asdf;
}


#endif // FILESYSTEM_PASSTHRU_H
