//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======
//
// .xzp creation and manipulation.
//
// XZip file format:
// [xZipHeader]
//
//
// Regular directory:
// [xZipDirectoryEntry 1]
// [xZipDirectoryEntry 2]
// [xZipDirectoryEntry 3]
// [...]
//
// Preload offset points here:
// Preload directory:
// [xZipDirectoryEntry 1]
// [xZipDirectoryEntry 2]
// [xZipDirectoryEntry 3]
// [...]
// [unsigned short directory to preload mapping 1]
// [unsigned short directory to preload mapping 2]
// [unsigned short directory to preload mapping 3]
// [...]
// [regular preload data]
//
// [consolidated file data]
//
// (v2+ only):
// The filename string table.  This is stored at the end of the file because in most cases it will rarely ever be read:
// [xZipFilenameEntry 1]	
// [xZipFilenameEntry 2]
// [xZipFilenameEntry 3]
// [...]
// [filename string data]		    
//
// The filename strings are currently only used for debugging purposes, though
// code could be written to use them for findfirst / findnext functionality.
//
//===========================================================================
//
// .xzp format version history:
// 
//    1: Initial version
//    2: Filename strings added, still reads version 1.
//	  3: File dates added to the filename strings structure.
//	  4: No more filename length - filenames are now offsets
//
//===========================================================================

#ifndef __XZIP_H__
#define __XZIP_H__

#pragma pack(1)

//-----------------------------------------------------------------------------
// xZipHeader
//
// Found at logical offset 0 in an .xzip file.
//-----------------------------------------------------------------------------
struct xZipHeader_t 
{
	enum 
	{ 
		MAGIC = 'xZip',
		FREE  = 'Free',
		VERSION = 6
	};

	unsigned Magic;						// MAGIC
	unsigned Version;					// VERSION
	unsigned PreloadDirectoryEntries;	// Number of preloaded directory entries.
    unsigned DirectoryEntries;			// Total # of directory entries (may not correspond to # of files as one file may have any number of directory entries)
    unsigned PreloadBytes;				// Number of bytes following the directory to preload.
	unsigned HeaderLength;				// Length of this header.
	unsigned FilenameEntries;			// The count of filename entries.
	unsigned FilenameStringsOffset;		// Offset to the filename strings offset table (array of dwords) (v2 only)
	unsigned FilenameStringsLength;		// Length of the filename string data. ( including the offsets table )
};
 
 
//-----------------------------------------------------------------------------
// xZipDirectoryEntry
//
// Multiple entries may be stored for a single file, representing pieces of the
// same file at different locations in the archive (for optimizing seek times)
// in the preload section, or to facilitate streaming.
//
// Note that a preloaded directory entry is the exact same structure, only
// stored in a different directory table.
//-----------------------------------------------------------------------------
struct xZipDirectoryEntry_t
{
    unsigned FilenameCRC;      // CRC of the filename
    unsigned Length;           // Length of this chunk of the file. In bytes. TODO: use sectors instead:
    int 	 StoredOffset;     // Offset the file data in the .xzip.  When building the zip, a negative means standard data, positive means preloaded data.

	static int __cdecl xZipDirectoryEntrySortCompare( const void* left, const void* right );
	static int __cdecl xZipDirectoryEntryFindCompare( const void* left, const void* right );
};

//-----------------------------------------------------------------------------
// xZipFilenameEntry
//
// An entry describing where to find an individual filename
//-----------------------------------------------------------------------------
struct xZipFilenameEntry_t
{
	unsigned FilenameCRC;			// CRC of the filename associated with this filename.
	unsigned FilenameOffset;		// Absolute offset to the filename on disk.
	unsigned TimeStamp;				// Time stamp of the file at the time it was added.

	static int __cdecl xZipFilenameEntryCompare( const void* left, const void* right );
};

//-----------------------------------------------------------------------------
// xZipFooter
//
// This footer is tacked onto the end of every xzip.  It allows you to find the 
// beginning of an .xzip file when it is appended to something else.
//-----------------------------------------------------------------------------
struct xZipFooter_t
{
	enum
	{
		MAGIC = 'XzFt'
	};

	unsigned Size;					// Total size of this .xzip including this footer.
	unsigned Magic;					// MAGIC  (last dword in the file)
};

#pragma pack()


//-----------------------------------------------------------------------------
// xZip
//
// Class for manipulating xZips.
//-----------------------------------------------------------------------------
class CXZip
{
public:
	// Construction and destruction:
	CXZip( char* filename );						// xZip filename to register
	CXZip( FILE* handle, int offset, int size );	// file handle and offset of the zip file  - used for constructing files inside of files.	
	CXZip();


	virtual ~CXZip();

	// Loading and unloading xZips:
	bool Load( char* filename, bool bPreload = true );				// Load a pack file into this instance.  Returns true on success.
	bool Load( FILE* handle, int nOffset, int nSize, bool bPreload = true );	// Load a pack file into this instance.  Returns true on success.
	void Unload();													// Unload this packfile
	
	void PreloadData();												// Loads the preloaded data if it isn't already.
	void DiscardPreloadedData();									// Free the preloaded data from memory.

	// Header Accessors:
	int GetVersion()	 { return m_Header.Version; }
	int EntryCount()	 { return m_Header.DirectoryEntries; }
	int GetPreloadSize() { return m_Header.PreloadBytes; }
	
	// Hack, interfaces to main thing:
	bool GetSimpleFileOffsetLength( const char* FileName, int& nBaseIndex, int &nFileOffset, int &nLength );
	bool ExtractFile( const char* FileName );

	// If installed, these routines will be called instead of fread, etc
	bool InstallAlternateIO( int (*read)( void* buffer, int offset, int destLength, int length, int hUser), int hUser );


	// Retrieves the file name of a given entry, returns true on success. NOTE: Slow and memory consuming
	char* GetEntryFileName( unsigned CRC, char* pDefault = NULL );

	xZipDirectoryEntry_t* GetEntry( int nEntryIndex )			{ return &m_pDirectory[nEntryIndex]; }
	xZipDirectoryEntry_t* GetPreloadEntry( int nEntryIndex )  
	{
		// If the preload chunk isn't loaded, fail.
		if( !m_pPreloadedData )
			return NULL;
	
		// If this entry doesn't have a corresponding preload entry, fail.
		if( m_nRegular2PreloadEntryMapping[nEntryIndex] == 0xFFFF )
			return NULL;
		
		// Return the entry:
		return &m_pPreloadDirectory[m_nRegular2PreloadEntryMapping[nEntryIndex]];
	}
	// Reads a certain amount of data from a certain file.
	// Determines and reads from the optimal entry index.  (Precached, closest seek, etc)
	// If -1 based from the beginning of the pack file, if -1, from the beginning of the entry
	int ReadIndex( int nEntryIndex, int nFileOffset, int nDestBytes, int nLength, void* pBuffer );

	// Sanity checks that the zip file is ready and readable:
	bool IsValid();

	void WarningDir();
	void SetMonitorLevel( int nLevel ) { m_nMonitorLevel = nLevel; }


protected:
	// Returns the size of the given entry: Returns -1 on error.
	int GetEntrySize( int nEntryIndex );

	xZipHeader_t				m_Header;			// The xZip header structure.
	xZipDirectoryEntry_t*		m_pPreloadDirectory;// Pointer to the preload directory entries.
	xZipDirectoryEntry_t*		m_pDirectory;		// Pointer to the directory entries
	xZipFilenameEntry_t*		m_pFilenames;		// Pointer to the filename table, or NULL
	void*						m_pPreloadedData;	// Pointer to the preloaded data, or NULL
	unsigned					m_nPreloadStart;
	unsigned short*				m_nRegular2PreloadEntryMapping;

	FILE*						m_hZip;				// Handle to the zip file.
	int							m_nSize;			// Size of the zip file.
	int							m_nOffset;			// Offset of the start of the zip within the above file
	int							m_hUser;
	int							m_nMonitorLevel;    // Default monitoring level
	
	// Alternate IO functions
	int (*m_pRead)( void* buffer, int offset, int destLength, int length, int hUser);
	static int defaultRead( void* buffer, int offset, int destLength, int length, int hUser);
};

//-----------------------------------------------------------------------------
// xZipCRCFilename
//
// CRC's and returns the string for the given filename.
//-----------------------------------------------------------------------------
unsigned xZipCRCFilename( char* filename );

#ifdef MAKE_XZIP_TOOL
//-----------------------------------------------------------------------------
// xZipBegin
// xZipAddFile
// xZipEnd
//
// Functions for constructing an xZip.
//-----------------------------------------------------------------------------
int  xZipBegin( char* fileNameXzip );
bool xZipAddFile( char* filename, bool bPrecacheEntireFile = false, bool bProcessPrecacheHeader = false, bool bProcessPrecacheHeaderOnly = false );
bool xZipEnd();

//-----------------------------------------------------------------------------
// xZipAttachPreloadFooter
//
// Attaches a preload footer to the given binary file.  Returns true on success.
//-----------------------------------------------------------------------------
bool xZipAttachPreloadFooter( char* filename, unsigned nLength, unsigned nOffset = 0 );

//-----------------------------------------------------------------------------
// xZipReadPreloadFooter
//
// Reads the preload footer off a file, or returns false if none exists:
//-----------------------------------------------------------------------------
bool xZipReadPreloadFooter( char* filename, unsigned &nLength, unsigned &nOffset );

//-----------------------------------------------------------------------------
// xZipComputeCustomPreloads
//
// Computes custom preload lengths for propriatary formats.
// ex, computes the offset of everything up until the data chunk in a .wav file.
//-----------------------------------------------------------------------------
unsigned xZipComputeCustomPreloads( char* filename );


// Hacky non encapsulated functions:
char* CleanFilename( char* filename );
#endif // MAKE_XZIP_TOOL

#endif // __XZIP_H__
