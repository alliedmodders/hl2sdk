//========= Copyright (c) 1996-2005, Valve Corporation, All rights reserved. ============//
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
	virtual int				Read( void* pOutput, int size, FileHandle_t file )							{ return m_pBaseFileSystemPassThru->Read( pOutput, size, file ); }
	virtual int				Write( void const* pInput, int size, FileHandle_t file )					{ return m_pBaseFileSystemPassThru->Write( pInput, size, file ); }
	virtual FileHandle_t	Open( const char *pFileName, const char *pOptions, const char *pathID )		{ return m_pBaseFileSystemPassThru->Open( pFileName, pOptions, pathID ); }
	virtual void			Close( FileHandle_t file )													{ m_pBaseFileSystemPassThru->Close( file ); }
	virtual void			Seek( FileHandle_t file, int pos, FileSystemSeek_t seekType )				{ m_pBaseFileSystemPassThru->Seek( file, pos, seekType ); }
	virtual unsigned int	Tell( FileHandle_t file )													{ return m_pBaseFileSystemPassThru->Tell( file ); }
	virtual unsigned int	Size( FileHandle_t file )													{ return m_pBaseFileSystemPassThru->Size( file ); }
	virtual unsigned int	Size( const char *pFileName, const char *pPathID )							{ return m_pBaseFileSystemPassThru->Size( pFileName, pPathID ); }
	virtual void			Flush( FileHandle_t file )													{ m_pBaseFileSystemPassThru->Flush( file ); }
	virtual bool			Precache( const char *pFileName, const char *pPathID )						{ return m_pBaseFileSystemPassThru->Precache( pFileName, pPathID ); }
	virtual bool			FileExists( const char *pFileName, const char *pPathID )					{ return m_pBaseFileSystemPassThru->FileExists( pFileName, pPathID ); }
	virtual bool			IsFileWritable( char const *pFileName, const char *pPathID )					{ return m_pBaseFileSystemPassThru->IsFileWritable( pFileName, pPathID ); }
	virtual bool			SetFileWritable( char const *pFileName, bool writable, const char *pPathID )	{ return m_pBaseFileSystemPassThru->SetFileWritable( pFileName, writable, pPathID ); }
	virtual long			GetFileTime( const char *pFileName, const char *pPathID )						{ return m_pBaseFileSystemPassThru->GetFileTime( pFileName, pPathID ); }
	virtual bool			ReadFile( const char *pFileName, const char *pPath, CUtlBuffer &buf, int nMaxBytes = 0, int nStartingByte = 0, FSAllocFunc_t pfnAlloc = NULL ) { return m_pBaseFileSystemPassThru->ReadFile( pFileName, pPath, buf, nMaxBytes, nStartingByte, pfnAlloc  ); }
	virtual bool			WriteFile( const char *pFileName, const char *pPath, CUtlBuffer &buf )		{  return m_pBaseFileSystemPassThru->WriteFile( pFileName, pPath, buf ); }
	virtual bool			UnzipFile( const char *pFileName, const char *pPath, const char *pDestination )		{  return m_pBaseFileSystemPassThru->UnzipFile( pFileName, pPath, pDestination ); }
	virtual bool			CopyAFile( const char *pFileName, const char *pPath, const char *pDestination, bool bDontOverwrite )
							{ return m_pBaseFileSystemPassThru->CopyAFile( pFileName, pPath, pDestination, bDontOverwrite ); }

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
	virtual bool Connect( CreateInterfaceFn factory )															{ return m_pFileSystemPassThru->Connect( factory ); }
	virtual void Disconnect()																					{ m_pFileSystemPassThru->Disconnect(); }
	virtual void *QueryInterface( const char *pInterfaceName )													{ return m_pFileSystemPassThru->QueryInterface( pInterfaceName ); }
	virtual InitReturnVal_t Init()																				{ return m_pFileSystemPassThru->Init(); }
	virtual void Shutdown()																						{ m_pFileSystemPassThru->Shutdown(); }
	virtual void PreShutdown()																					{ m_pFileSystemPassThru->PreShutdown(); }
	virtual const AppSystemInfo_t* GetDependencies()															{ return m_pFileSystemPassThru->GetDependencies(); }
	virtual AppSystemTier_t GetTier()																			{ return m_pFileSystemPassThru->GetTier(); }
	virtual void Reconnect( CreateInterfaceFn factory, const char *pInterfaceName )								{ m_pFileSystemPassThru->Reconnect( factory, pInterfaceName ); }
	virtual bool IsSingleton()																					{ return m_pFileSystemPassThru->IsSingleton(); }
	virtual BuildType_t GetBuildType()																			{ return m_pFileSystemPassThru->GetBuildType(); }

	virtual void			RemoveAllSearchPaths( void )														{ m_pFileSystemPassThru->RemoveAllSearchPaths(); }
	virtual void			AddSearchPath( const char *pPath, const char *pathID, SearchPathAdd_t addType, SearchPathPriority_t priority )
							{ m_pFileSystemPassThru->AddSearchPath( pPath, pathID, addType, priority ); }
	virtual bool			RemoveSearchPath( const char *pPath, const char *pathID )							{ return m_pFileSystemPassThru->RemoveSearchPath( pPath, pathID ); }
	virtual SearchPathStateHandle_t		*SaveSearchPathState( const char *pszName ) const						{ return m_pFileSystemPassThru->SaveSearchPathState( pszName ); }
	virtual void			RestoreSearchPathState( SearchPathStateHandle_t *pState )							{ m_pFileSystemPassThru->RestoreSearchPathState( pState ); }
	virtual void			DestroySearchPathState( SearchPathStateHandle_t *pState )							{ m_pFileSystemPassThru->DestroySearchPathState( pState ); }
	virtual void			RemoveFile( char const* pRelativePath, const char *pathID )							{ m_pFileSystemPassThru->RemoveFile( pRelativePath, pathID ); }
	virtual bool			RenameFile( char const *pOldPath, char const *pNewPath, const char *pathID )		{ return m_pFileSystemPassThru->RenameFile( pOldPath, pNewPath, pathID ); }
	virtual void			CreateDirHierarchy( const char *path, const char *pathID )							{ m_pFileSystemPassThru->CreateDirHierarchy( path, pathID ); }
	virtual bool			IsDirectory( const char *pFileName, const char *pathID )							{ return m_pFileSystemPassThru->IsDirectory( pFileName, pathID ); }
	virtual void			FileTimeToString( char* pStrip, int maxCharsIncludingTerminator, long fileTime )	{ m_pFileSystemPassThru->FileTimeToString( pStrip, maxCharsIncludingTerminator, fileTime ); }
	virtual void			SetBufferSize( FileHandle_t file, unsigned nBytes )									{ m_pFileSystemPassThru->SetBufferSize( file, nBytes  ); }
	virtual bool			IsOk( FileHandle_t file )															{ return m_pFileSystemPassThru->IsOk( file ); }
	virtual bool			EndOfFile( FileHandle_t file )														{ return m_pFileSystemPassThru->EndOfFile( file ); }
	virtual char			*ReadLine( char *pOutput, int maxChars, FileHandle_t file )							{ return m_pFileSystemPassThru->ReadLine( pOutput, maxChars, file ); }
	virtual int				FPrintf( FileHandle_t file, const char *pFormat, ... ) 
	{ 
		char str[8192];
		va_list marker;
		va_start( marker, pFormat );
		_vsnprintf( str, sizeof( str ), pFormat, marker );
		va_end( marker );
		return m_pFileSystemPassThru->FPrintf( file, "%s", str );
	}
	virtual HMODULE 		*LoadModule( const char *pFileName, const char *pPathID, bool bValidatedDllOnly )	{ return m_pFileSystemPassThru->LoadModule( pFileName, pPathID, bValidatedDllOnly ); }
	virtual void			UnloadModule( HMODULE *pModule )													{ m_pFileSystemPassThru->UnloadModule( pModule ); }
	virtual const char		*FindFirst( const char *pWildCard, FileFindHandle_t *pHandle )						{ return m_pFileSystemPassThru->FindFirst( pWildCard, pHandle ); }
	virtual const char		*FindNext( FileFindHandle_t handle )												{ return m_pFileSystemPassThru->FindNext( handle ); }
	virtual bool			FindIsDirectory( FileFindHandle_t handle )											{ return m_pFileSystemPassThru->FindIsDirectory( handle ); }
	virtual void			FindClose( FileFindHandle_t handle )												{ m_pFileSystemPassThru->FindClose( handle ); }
	virtual const char		*GetLocalPath( const char *pFileName, char *pLocalPath, int localPathBufferSize )	{ return m_pFileSystemPassThru->GetLocalPath( pFileName, pLocalPath, localPathBufferSize ); }
	virtual bool			FullPathToRelativePath( const char *pFullPath, const char *pPathId, char *pRelative, int nMaxLen ) { return m_pFileSystemPassThru->FullPathToRelativePath( pFullPath, pPathId, pRelative, nMaxLen ); }
	virtual bool			GetCurrentDirectory( char* pDirectory, int maxlen )									{ return m_pFileSystemPassThru->GetCurrentDirectory( pDirectory, maxlen ); }
	virtual void			PrintOpenedFiles( void )															{ m_pFileSystemPassThru->PrintOpenedFiles(); }
	virtual void			PrintSearchPaths( void )															{ m_pFileSystemPassThru->PrintSearchPaths(); }
	virtual void			SetWarningFunc( void (*pfnWarning)( const char *fmt, ... ) )						{ m_pFileSystemPassThru->SetWarningFunc( pfnWarning ); }
	virtual void			SetWarningLevel( FileWarningLevel_t level )											{ m_pFileSystemPassThru->SetWarningLevel( level ); } 
	virtual void			AddLoggingFunc( void (*pfnLogFunc)( const char *fileName, const char *accessType ) ){ m_pFileSystemPassThru->AddLoggingFunc( pfnLogFunc ); }
	virtual void			RemoveLoggingFunc( FileSystemLoggingFunc_t logFunc )								{ m_pFileSystemPassThru->RemoveLoggingFunc( logFunc ); }
	virtual FSAsyncStatus_t	AsyncReadMultiple( const FileAsyncRequest_t *pRequests, int nRequests, FSAsyncControl_t *pControls )			{ return m_pFileSystemPassThru->AsyncReadMultiple( pRequests, nRequests, pControls ); }
	virtual FSAsyncStatus_t AsyncWriteUGCFile( const char *pFileName, const CUtlBuffer *pSrc, int nSrcBytes, bool bFreeMemory, CUtlDelegate<void (bool, const char *)> callback, FSAsyncControl_t *phControl = NULL ) { return m_pFileSystemPassThru->AsyncWriteUGCFile( pFileName, pSrc, nSrcBytes, bFreeMemory, callback, phControl ); }
	virtual FSAsyncStatus_t	AsyncReadMultipleCreditAlloc( const FileAsyncRequest_t *pRequests, int nRequests, const char *pszFile, int line, FSAsyncControl_t *pControls ) { return m_pFileSystemPassThru->AsyncReadMultipleCreditAlloc( pRequests, nRequests, pszFile, line, pControls ); }
	virtual FSAsyncStatus_t AsyncDirectoryScan( const char* pSearchSpec, bool recurseFolders,  void* pContext, FSAsyncScanAddFunc_t pfnAdd,  FSAsyncScanCompleteFunc_t pfnDone, FSAsyncControl_t *pControl = NULL )  { return m_pFileSystemPassThru->AsyncDirectoryScan( pSearchSpec, recurseFolders, pContext, pfnAdd, pfnDone, pControl ); }
	virtual FSAsyncStatus_t	AsyncFinish(FSAsyncControl_t hControl, bool wait)									{ return m_pFileSystemPassThru->AsyncFinish( hControl, wait ); }
	virtual FSAsyncStatus_t	AsyncGetResult( FSAsyncControl_t hControl, void **ppData, int *pSize )				{ return m_pFileSystemPassThru->AsyncGetResult( hControl, ppData, pSize ); }
	virtual FSAsyncStatus_t	AsyncAbort(FSAsyncControl_t hControl)												{ return m_pFileSystemPassThru->AsyncAbort( hControl ); }
	virtual FSAsyncStatus_t	AsyncStatus(FSAsyncControl_t hControl)												{ return m_pFileSystemPassThru->AsyncStatus( hControl ); }
	virtual FSAsyncStatus_t	AsyncFlush()																		{ return m_pFileSystemPassThru->AsyncFlush(); }
	virtual void			AsyncAddRef( FSAsyncControl_t hControl )											{ m_pFileSystemPassThru->AsyncAddRef( hControl ); }
	virtual void			AsyncRelease( FSAsyncControl_t hControl )											{ m_pFileSystemPassThru->AsyncRelease( hControl ); }
	virtual FSAsyncStatus_t	AsyncBeginRead( const char *pszFile, FSAsyncFile_t *phFile )						{ return m_pFileSystemPassThru->AsyncBeginRead( pszFile, phFile ); }
	virtual FSAsyncStatus_t	AsyncEndRead( FSAsyncFile_t hFile )													{ return m_pFileSystemPassThru->AsyncEndRead( hFile ); }
	virtual const FileSystemStatistics *GetFilesystemStatistics()												{ return m_pFileSystemPassThru->GetFilesystemStatistics(); }
	virtual WaitForResourcesHandle_t WaitForResources( const char *resourcelist )								{ return m_pFileSystemPassThru->WaitForResources( resourcelist ); }
	virtual bool			GetWaitForResourcesProgress( WaitForResourcesHandle_t handle, 
								float *progress, bool *complete )												{ return m_pFileSystemPassThru->GetWaitForResourcesProgress( handle, progress, complete ); }
	virtual void			CancelWaitForResources( WaitForResourcesHandle_t handle )							{ m_pFileSystemPassThru->CancelWaitForResources( handle ); }
	virtual int				HintResourceNeed( const char *hintlist, int forgetEverything )						{ return m_pFileSystemPassThru->HintResourceNeed( hintlist, forgetEverything ); }
	virtual bool			IsFileImmediatelyAvailable(const char *pFileName)									{ return m_pFileSystemPassThru->IsFileImmediatelyAvailable( pFileName ); }
	virtual void			GetLocalCopy( const char *pFileName )												{ m_pFileSystemPassThru->GetLocalCopy( pFileName ); }
	virtual FileNameHandle_t	FindOrAddFileName( char const *pFileName )										{ return m_pFileSystemPassThru->FindOrAddFileName( pFileName ); }
	virtual FileNameHandle_t	FindFileName( char const *pFileName )											{ return m_pFileSystemPassThru->FindFileName( pFileName ); }
	virtual bool				String( const FileNameHandle_t& handle, char *buf, int buflen )					{ return m_pFileSystemPassThru->String( handle, buf, buflen ); }
	virtual bool			IsOk2( FileHandle_t file )															{ return IsOk(file); }
	virtual void			RemoveSearchPaths( const char *szPathID )											{ m_pFileSystemPassThru->RemoveSearchPaths( szPathID ); }
	
	virtual const char		*FindFirstEx( 
		const char *pWildCard, 
		const char *pPathID,
		FileFindHandle_t *pHandle
		)																										{ return m_pFileSystemPassThru->FindFirstEx( pWildCard, pPathID, pHandle ); }
	virtual void			FindFileAbsoluteList(
		CUtlVector<CUtlString> &output,
		const char *pWildCard,
		const char *pPathID
		)																										{ m_pFileSystemPassThru->FindFileAbsoluteList( output, pWildCard, pPathID ); }
	virtual void			MarkPathIDByRequestOnly( const char *pPathID, bool bRequestOnly )					{ m_pFileSystemPassThru->MarkPathIDByRequestOnly( pPathID, bRequestOnly ); }
	virtual void			Unknown1(const char *a1, void *a2) {};
	virtual void			Unknown2(void *a1, void *a2, void *a3, char *a4) {};
	virtual const char		*Unknown3(void *a1, void *a2, void *a3, int a4, char *a5, double a6) { return NULL; };
	virtual void			Unknown4() {};
	virtual void			Unknown5(void *a1, int a2, int *a3) {};
	virtual void			Unknown6() {};
	//virtual bool			IsFileInReadOnlySearchPath ( const char *pPathID, const char *pFileName )			{ return m_pFileSystemPassThru->IsFileInReadOnlySearchPath( pPathID, pFileName ); }
	//virtual void			SetSearchPathReadOnly( const char *pPathID, const char *pUnknown, bool bReadOnly )			{ m_pFileSystemPassThru->SetSearchPathReadOnly( pPathID, pUnknown, bReadOnly ); }
	virtual FSAsyncStatus_t	AsyncAppend(const char *pFileName, const void *pSrc, int nSrcBytes, bool bFreeMemory, FSAsyncControl_t *pControl ) { return m_pFileSystemPassThru->AsyncAppend( pFileName, pSrc, nSrcBytes, bFreeMemory, pControl); }
	virtual FSAsyncStatus_t	AsyncWrite(const char *pFileName, const void *pSrc, int nSrcBytes, bool bFreeMemory, bool bAppend, FSAsyncControl_t *pControl ) { return m_pFileSystemPassThru->AsyncWrite( pFileName, pSrc, nSrcBytes, bFreeMemory, bAppend, pControl); }
	virtual FSAsyncStatus_t	AsyncWriteFile(const char *pFileName, const CUtlBuffer *pSrc, int nSrcBytes, bool bFreeMemory, bool bAppend, FSAsyncControl_t *pControl ) { return m_pFileSystemPassThru->AsyncWriteFile( pFileName, pSrc, nSrcBytes, bFreeMemory, bAppend, pControl); }
	virtual FSAsyncStatus_t	AsyncAppendFile(const char *pDestFileName, const char *pSrcFileName, FSAsyncControl_t *pControl )			{ return m_pFileSystemPassThru->AsyncAppendFile(pDestFileName, pSrcFileName, pControl); }
	virtual void			AsyncFinishAll( int iToPriority )													{ m_pFileSystemPassThru->AsyncFinishAll(iToPriority); }
	virtual void			AsyncFinishAllWrites()																{ m_pFileSystemPassThru->AsyncFinishAllWrites(); }
	virtual FSAsyncStatus_t	AsyncSetPriority(FSAsyncControl_t hControl, int newPriority)						{ return m_pFileSystemPassThru->AsyncSetPriority(hControl, newPriority); }
	virtual bool			AsyncSuspend()																		{ return m_pFileSystemPassThru->AsyncSuspend(); }
	virtual bool			AsyncResume()																		{ return m_pFileSystemPassThru->AsyncResume(); }
	//virtual const char		*RelativePathToFullPath( const char *pFileName, const char *pPathID, char *pLocalPath, int localPathBufferSize, PathTypeFilter_t pathFilter = FILTER_NONE, PathTypeQuery_t *pPathType = NULL ) { return m_pFileSystemPassThru->RelativePathToFullPath( pFileName, pPathID, pLocalPath, localPathBufferSize, pathFilter, pPathType ); }
	//virtual int				GetSearchPath( const char *pathID, GetSearchPathTypes_t type, char *pPath, int nMaxLen	)	{ return m_pFileSystemPassThru->GetSearchPath( pathID, type, pPath, nMaxLen ); }

	virtual FileHandle_t	OpenEx( const char *pFileName, const char *pOptions, unsigned flags = 0, const char *pathID = 0 ) { return m_pFileSystemPassThru->OpenEx( pFileName, pOptions, flags, pathID );}
	virtual int				ReadEx( void* pOutput, int destSize, int size, FileHandle_t file )					{ return m_pFileSystemPassThru->ReadEx( pOutput, destSize, size, file ); }
	virtual int				ReadFileEx( const char *pFileName, const char *pPath, void **ppBuf, bool bNullTerminate, bool bOptimalAlloc, int nMaxBytes = 0, int nStartingByte = 0, FSAllocFunc_t pfnAlloc = NULL ) { return m_pFileSystemPassThru->ReadFileEx( pFileName, pPath, ppBuf, bNullTerminate, bOptimalAlloc, nMaxBytes, nStartingByte, pfnAlloc ); }

#if defined( TRACK_BLOCKING_IO )
	virtual void			EnableBlockingFileAccessTracking( bool state ) { m_pFileSystemPassThru->EnableBlockingFileAccessTracking( state ); }
	virtual bool			IsBlockingFileAccessEnabled() const { return m_pFileSystemPassThru->IsBlockingFileAccessEnabled(); }

	virtual IBlockingFileItemList *RetrieveBlockingFileAccessInfo() { return m_pFileSystemPassThru->RetrieveBlockingFileAccessInfo(); }
#endif

	// If the "PreloadedData" hasn't been purged, then this'll try and instance the KeyValues using the fast path of compiled keyvalues loaded during startup.
	// Otherwise, it'll just fall through to the regular KeyValues loading routines
	virtual KeyValues		*LoadKeyValues( KeyValuesPreloadType_t type, char const *filename, char const *pPathID = 0 ) { return m_pFileSystemPassThru->LoadKeyValues( type, filename, pPathID ); }
	virtual bool			LoadKeyValues( KeyValues& head, KeyValuesPreloadType_t type, char const *filename, char const *pPathID = 0 ) { return m_pFileSystemPassThru->LoadKeyValues( head, type, filename, pPathID ); }

	virtual bool			GetFileTypeForFullPath( char const *pFullPath, wchar_t *buf, size_t bufSizeInBytes ) { return m_pFileSystemPassThru->GetFileTypeForFullPath( pFullPath, buf, bufSizeInBytes ); }

	virtual bool			GetOptimalIOConstraints( FileHandle_t hFile, unsigned *pOffsetAlign, unsigned *pSizeAlign, unsigned *pBufferAlign ) { return m_pFileSystemPassThru->GetOptimalIOConstraints( hFile, pOffsetAlign, pSizeAlign, pBufferAlign ); }
	virtual void			*AllocOptimalReadBuffer( FileHandle_t hFile, unsigned nSize, unsigned nOffset  ) { return m_pFileSystemPassThru->AllocOptimalReadBuffer( hFile, nOffset, nSize ); }
	virtual void			FreeOptimalReadBuffer( void *p ) { m_pFileSystemPassThru->FreeOptimalReadBuffer( p ); }

	virtual bool			ReadToBuffer( FileHandle_t hFile, CUtlBuffer &buf, int nMaxBytes = 0, FSAllocFunc_t pfnAlloc = NULL ) { return m_pFileSystemPassThru->ReadToBuffer( hFile, buf, nMaxBytes, pfnAlloc ); }
	virtual int				GetPathIndex( const FileNameHandle_t &handle ) { return m_pFileSystemPassThru->GetPathIndex( handle ); }
	virtual long			GetPathTime( const char *pPath, const char *pPathID ) { return m_pFileSystemPassThru->GetPathTime( pPath, pPathID ); }

	virtual DVDMode_t		GetDVDMode() { return m_pFileSystemPassThru->GetDVDMode(); }

	virtual void EnableWhitelistFileTracking( bool bEnable )
		{ m_pFileSystemPassThru->EnableWhitelistFileTracking( bEnable ); }
	virtual void RegisterFileWhitelist( IFileList *pForceMatchList, IFileList *pAllowFromDiskList, IFileList **pFilesToReload )
		{ m_pFileSystemPassThru->RegisterFileWhitelist( pForceMatchList, pAllowFromDiskList, pFilesToReload ); }
	virtual void MarkAllCRCsUnverified()
		{ m_pFileSystemPassThru->MarkAllCRCsUnverified(); }
	virtual void CacheFileCRCs( const char *pPathname, ECacheCRCType eType, IFileList *pFilter )
		{ return m_pFileSystemPassThru->CacheFileCRCs( pPathname, eType, pFilter ); }
	virtual EFileCRCStatus CheckCachedFileCRC( const char *pPathID, const char *pRelativeFilename, CRC32_t *pCRC )
		{ return m_pFileSystemPassThru->CheckCachedFileCRC( pPathID, pRelativeFilename, pCRC ); }
	virtual int GetUnverifiedCRCFiles( CUnverifiedCRCFile *pFiles, int nMaxFiles )
		{ return m_pFileSystemPassThru->GetUnverifiedCRCFiles( pFiles, nMaxFiles ); }
	virtual int GetWhitelistSpewFlags()
		{ return m_pFileSystemPassThru->GetWhitelistSpewFlags(); }
	virtual void SetWhitelistSpewFlags( int spewFlags )
		{ m_pFileSystemPassThru->SetWhitelistSpewFlags( spewFlags ); }
	virtual void InstallDirtyDiskReportFunc( FSDirtyDiskReportFunc_t func ) { m_pFileSystemPassThru->InstallDirtyDiskReportFunc( func ); }

	virtual bool			IsLaunchedFromXboxHDD() { return m_pFileSystemPassThru->IsLaunchedFromXboxHDD(); }
	virtual bool			IsInstalledToXboxHDDCache() { return m_pFileSystemPassThru->IsInstalledToXboxHDDCache(); }
	virtual bool			IsDVDHosted() { return m_pFileSystemPassThru->IsDVDHosted(); }
	virtual bool			IsInstallAllowed() { return m_pFileSystemPassThru->IsInstallAllowed(); }
	virtual int				GetSearchPathID( char *pPath, int nMaxLen	) { return m_pFileSystemPassThru->GetSearchPathID( pPath, nMaxLen ); }
	virtual bool			FixupSearchPathsAfterInstall() { return m_pFileSystemPassThru->FixupSearchPathsAfterInstall(); }

	virtual FSDirtyDiskReportFunc_t	GetDirtyDiskReportFunc() { return m_pFileSystemPassThru->GetDirtyDiskReportFunc(); }

	virtual void AddVPKFile( char const *pPkName, const char *pUnknown, SearchPathAdd_t addType = PATH_ADD_TO_TAIL ) { m_pFileSystemPassThru->AddVPKFile( pPkName, pUnknown, addType ); }
	virtual void RemoveVPKFile( char const *pPkName, const char *pUnknown ) { m_pFileSystemPassThru->RemoveVPKFile( pPkName, pUnknown ); }
	virtual bool IsVPKFileLoaded( const char *pszName )															{ return m_pFileSystemPassThru->IsVPKFileLoaded( pszName ); }
	virtual void EnableAutoVPKFileLoading( bool unk )															{ m_pFileSystemPassThru->EnableAutoVPKFileLoading( unk ); }
	virtual void GetAutoVPKFileLoading( void )																	{ m_pFileSystemPassThru->GetAutoVPKFileLoading(); }
	virtual void AddUGCVPKFile( uint64 ugcId, const char *unk1, SearchPathAdd_t unk2 )							{ m_pFileSystemPassThru->AddUGCVPKFile( ugcId, unk1, unk2 ); }
	virtual void RemoveUGCVPKFile( uint64 ugcId, const char *unk1 )												{ m_pFileSystemPassThru->RemoveUGCVPKFile( ugcId, unk1 ); }
	virtual bool IsUGCVPKFileLoaded( uint64 ugcId )																{ return m_pFileSystemPassThru->IsUGCVPKFileLoaded( ugcId ); }
	virtual void ParseUGCHandleFromFilename( const char *unk1, char **unk2 ) const								{ m_pFileSystemPassThru->ParseUGCHandleFromFilename( unk1, unk2 ); }
	virtual void CreateFilenameForUGCFile( char *unk1, int unk2, uint64 ugcId, const char *unk3, char unk4 ) const	{ m_pFileSystemPassThru->CreateFilenameForUGCFile( unk1, unk2, ugcId, unk3, unk4 ); }
	virtual void GetUGCInfo( uint64 ugcId, char **unk1, int *unk2, CSteamID *unk3 )								{ m_pFileSystemPassThru->GetUGCInfo( ugcId, unk1, unk2, unk3 ); }
	virtual void OpenUGCFile( uint64 ugcId )																	{ m_pFileSystemPassThru->OpenUGCFile( ugcId ); }
	
	virtual void			SyncDvdDevCache( void ) { m_pFileSystemPassThru->SyncDvdDevCache(); }

	virtual bool			DiscoverDLC( int iController ) { return m_pFileSystemPassThru->DiscoverDLC( iController ); }
	virtual int				IsAnyDLCPresent( bool *pbDLCSearchPathMounted = NULL ) { return m_pFileSystemPassThru->IsAnyDLCPresent( pbDLCSearchPathMounted ); }
	virtual bool			GetAnyDLCInfo( int iDLC, unsigned int *pLicenseMask, wchar_t *pTitleBuff, int nOutTitleSize ) { return m_pFileSystemPassThru->GetAnyDLCInfo( iDLC, pLicenseMask, pTitleBuff, nOutTitleSize ); }
	virtual int				IsAnyCorruptDLC() { return m_pFileSystemPassThru->IsAnyCorruptDLC(); }
	virtual bool			GetAnyCorruptDLCInfo( int iCorruptDLC, wchar_t *pTitleBuff, int nOutTitleSize ) { return m_pFileSystemPassThru->GetAnyCorruptDLCInfo( iCorruptDLC, pTitleBuff, nOutTitleSize ); }
	virtual bool			AddDLCSearchPaths() { return m_pFileSystemPassThru->AddDLCSearchPaths(); }
	virtual bool			IsSpecificDLCPresent( unsigned int nDLCPackage ) { return m_pFileSystemPassThru->IsSpecificDLCPresent( nDLCPackage ); }
	virtual void            SetIODelayAlarm( float flThreshhold ) { m_pFileSystemPassThru->SetIODelayAlarm( flThreshhold ); }

	virtual ILowLevelFileIO *GetLowLevelFileIO( void )													{ return m_pFileSystemPassThru->GetLowLevelFileIO(); }
	virtual void			SetLowLevelFileIO( ILowLevelFileIO *pIO )									{ m_pFileSystemPassThru->SetLowLevelFileIO( pIO ); }
	virtual void			AddXLSPUpdateSearchPath( const void *unk1, int unk2 )						{ m_pFileSystemPassThru->AddXLSPUpdateSearchPath( unk1, unk2 ); }
	virtual void			DumpFileSystemStats( int unk1, const char *unk2 )							{ m_pFileSystemPassThru->DumpFileSystemStats( unk1, unk2 ); }
	virtual void			DeleteDirectory( const char *pFileName, const char *pathID = 0 )			{ m_pFileSystemPassThru->DeleteDirectory( pFileName, pathID ); }
	virtual bool			IsPathInvalidForFilesystem( const char *pFileName )							{ return m_pFileSystemPassThru->IsPathInvalidForFilesystem( pFileName ); }
	virtual void			GetAvailableDrives( CUtlVector<CUtlString> &drives )						{ m_pFileSystemPassThru->GetAvailableDrives( drives ); }
	virtual const char		*ReadLine( FileHandle_t file, bool bStripNewline = true )					{ return m_pFileSystemPassThru->ReadLine( file, bStripNewline ); }
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
