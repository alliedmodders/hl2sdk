//===== Copyright © 1996-2008, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//

#ifndef PACKEDSTORE_H
#define PACKEDSTORE_H
#ifdef _WIN32
#pragma once
#endif


#include <tier0/platform.h>
#include <tier0/threadtools.h>
#include <tier2/tier2.h>

#include "filesystem.h"
#include "tier1/utlintrusivelist.h"
#include "tier1/utlvector.h"
#include "tier1/utlmap.h"

class CPackedStore;




struct CPackedStoreFileHandle
{
public:
	int m_nFileNumber;
	int m_nFileOffset;
	int m_nFileSize;
	int m_nCurrentFileOffset;
	void const *m_pMetaData;
	uint16 m_nMetaDataSize;
	CPackedStore *m_pOwner;
	struct CFileHeaderFixedData *m_pHeaderData;
	uint8 *m_pDirFileNamePtr;								// pointer to basename in dir block

	FORCEINLINE operator bool( void ) const
	{
		return ( m_nFileNumber != -1 );
	}

	FORCEINLINE int Read( void *pOutData, int nNumBytes );

	CPackedStoreFileHandle( void )
	{
		m_nFileNumber = -1;
	}

	int Seek( int nOffset, int nWhence )
	{
		switch( nWhence )
		{
			case SEEK_CUR:
				nOffset = m_nFileOffset + nOffset ;
				break;

			case SEEK_END:
				nOffset = m_nFileSize - 1 + nOffset;
				break;
		}
		m_nCurrentFileOffset = MAX( 0, MIN( m_nFileSize - 1, nOffset ) );
		return m_nCurrentFileOffset;
	}

	int Tell( void ) const
	{
		return m_nCurrentFileOffset;
	}
};

#define MAX_ARCHIVE_FILES_TO_KEEP_OPEN_AT_ONCE 128

#define PACKEDFILE_EXT_HASH_SIZE 15


#ifdef _WIN32
typedef HANDLE PackDataFileHandle_t;
#else
typedef FileHandle_t PackDataFileHandle_t;
#endif

struct FileHandleTracker_t
{
	int m_nFileNumber;
	PackDataFileHandle_t m_hFileHandle;
	int m_nCurOfs;
	CThreadFastMutex m_Mutex;

	FileHandleTracker_t( void )
	{
		m_nFileNumber = -1;
	}
};

enum ePackedStoreAddResultCode
{
	EPADD_NEWFILE,											// the file was added and is new
	EPADD_ADDSAMEFILE,										// the file was already present, and the contents are the same as what you passed.
	EPADD_UPDATEFILE,										// the file was alreayd present and its contents have been updated
	EPADD_ERROR,											// some error has resulted
};


class CPackedStore
{
public:
	CPackedStore( char const *pFileBasename, IBaseFileSystem *pFS, bool bOpenForWrite = false );

	CPackedStoreFileHandle OpenFile( char const *pFile );

	ePackedStoreAddResultCode AddFile( char const *pFile, int nNumDataParts, uint16 nMetaDataSize, void *pFileData, uint32 nFullFileSize, bool bMultiChunk, uint32 const *pCrcToUse = NULL );

	// write out the file directory
	void Write( void );

	int ReadData( CPackedStoreFileHandle &handle, void *pOutData, int nNumBytes );

	~CPackedStore( void );

	FORCEINLINE void *DirectoryData( void )
	{
		return m_DirectoryData.Base();
	}

	// Get a list of all the files in the zip You are responsible for freeing the contents of
	// outFilenames (call outFilenames.PurgeAndDeleteElements).
	int GetFileList( CUtlStringList &outFilenames, bool bFormattedOutput, bool bSortedOutput );

	// Get a list of all files that match the given wildcard string
	int GetFileList( const char *pWildCard, CUtlStringList &outFilenames, bool bFormattedOutput, bool bSortedOutput );
	
	// Get a list of all directories of the given wildcard
	int GetFileAndDirLists( const char *pWildCard, CUtlStringList &outDirnames, CUtlStringList &outFilenames, bool bSortedOutput );
	int GetFileAndDirLists( CUtlStringList &outDirnames, CUtlStringList &outFilenames, bool bSortedOutput );

	bool IsEmpty( void ) const;


	char const *FullPathName( void )
	{
		return m_pszFullPathName;
	}

	void SetWriteChunkSize( int nWriteChunkSize )
	{
		m_nWriteChunkSize = nWriteChunkSize;
	}

private:
	char m_pszFileBaseName[MAX_PATH];
	char m_pszFullPathName[MAX_PATH];
	int m_nWriteOffset;
	int m_nChunkWriteIndex;
	int m_nWriteChunkFileIndex;
	int m_nDirectoryDataSize;
	int m_nWriteChunkSize;

	FileHandle_t m_ChunkWriteHandle;
	IBaseFileSystem *m_pFileSystem;
	CThreadFastMutex m_Mutex;
	

	CUtlIntrusiveList<class CFileExtensionData> m_pExtensionData[PACKEDFILE_EXT_HASH_SIZE];

	CUtlVector<uint8> m_DirectoryData;
	CUtlBlockVector<uint8> m_EmbeddedChunkData;

	int m_nHighestChunkFileIndex;

	FileHandleTracker_t m_FileHandles[MAX_ARCHIVE_FILES_TO_KEEP_OPEN_AT_ONCE];
	
	void Init( void );

	struct CFileHeaderFixedData *FindFileEntry( 
		char const *pDirname, char const *pBaseName, char const *pExtension,
		uint8 **pExtBaseOut = NULL, uint8 **pNameBaseOut = NULL );

	void BuildHashTables( void );
	void GetDataFileName( char *pszOutName, int nFileNumber );

	FileHandleTracker_t &GetFileHandle( int nFileNumber );

	FileHandle_t GetWriteHandle( int nChunkIdx );
	void CloseWriteHandle( void );

	// For cache-ing directory and contents data
	CUtlStringList m_directoryList; // The index of this list of directories...
	CUtlMap<int, CUtlStringList*> m_dirContents; // ...is the key to this map of filenames
	void BuildFindFirstCache();

};

FORCEINLINE int CPackedStoreFileHandle::Read( void *pOutData, int nNumBytes )
{
	return m_pOwner->ReadData( *this, pOutData, nNumBytes );
}


#endif // packedtsore_h
