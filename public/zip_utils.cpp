//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "utlbuffer.h"
#include "zip_utils.h"
#include "zip_uncompressed.h"
#include "checksum_crc.h"

//-----------------------------------------------------------------------------
// Purpose: Interface to allow abstraction of zip file output methods, and
// avoid duplication of code. Files may be written to a CUtlBuffer or a filestream
//-----------------------------------------------------------------------------
abstract_class IWriteStream
{
public:
		virtual void	Put( const void* pMem, int size ) = 0;
		virtual int		Tell( void ) = 0;
};

//-----------------------------------------------------------------------------
// Purpose: Wrapper for CUtlBuffer methods
//-----------------------------------------------------------------------------
class CBufferStream : public IWriteStream
{
public:
	CBufferStream( CUtlBuffer& buff ) : IWriteStream(), m_buff( &buff ) {}

	// Implementing IWriteStream method
	virtual void	Put( const void* pMem, int size )	{ m_buff->Put( pMem, size ); }

	// Implementing IWriteStream method
	virtual int		Tell( void )						{ return m_buff->TellPut(); }

private:
	CUtlBuffer *m_buff;
};

//-----------------------------------------------------------------------------
// Purpose: Wrapper for file I/O methods
//-----------------------------------------------------------------------------
class CFileStream : public IWriteStream
{
public:
	CFileStream( FILE *fout ) : IWriteStream(), m_file( fout )	{}

	// Implementing IWriteStream method
	virtual void	Put( const void* pMem, int size )	{ fwrite( pMem, size, 1, m_file ); }

	// Implementing IWriteStream method
	virtual int		Tell( void )						{ return ftell( m_file ); }

private:
	FILE *m_file;
};

//-----------------------------------------------------------------------------
// Purpose: Container for modifiable pak file which is embedded inside the .bsp file
//  itself.  It's used to allow one-off files to be stored local to the map and it is
//  hooked into the file system as an override for searching for named files.
//-----------------------------------------------------------------------------
class CZipFile
{
public:
	// Construction
				CZipFile( void );
				~CZipFile( void );

	// Public API
	// Clear all existing data
	void		Reset( void );

	// Add file to zip under relative name
	void		AddFileToZip( const char *relativename, const char *fullpath );

	// Delete file from zip
	void		RemoveFileFromZip( const char *relativename );

	// Add buffer to zip as a file with given name
	void		AddBufferToZip( const char *relativename, void *data, int length, bool bTextMode );

	// Check if a file already exists in the zip.
	bool		FileExistsInZip( const char *relativename );

	// Reads a file from a zip file
	bool		ReadFileFromZip( const char *relativename, bool bTextMode, CUtlBuffer &buf );

	// Initialize the zip file from a buffer
	void		ParseFromBuffer( byte *buffer, int bufferlength );

	// Estimate the size of the zip file (including header, padding, etc.)
	int			EstimateSize();

	// Print out a directory of files in the zip.
	void		PrintDirectory( void );

	// Use to iterate directory, pass 0 for first element
	// returns nonzero element id with filled buffer, or -1 at list conclusion
	int			GetNextFilename( int id, char *pBuffer, int bufferSize, int &fileSize );

	// Write the zip to a buffer
	void		SaveToBuffer( CUtlBuffer& buffer );

	// Write the zip to a filestream
	void		SaveToDisk( FILE *fout );

	int			CalculateSize( void );

	void		ForceAlignment( bool aligned, unsigned int sectorSize );

	unsigned int	GetAlignment();

private:
	// Hopefully this is enough
	enum
	{
		MAX_FILES_IN_ZIP = 32768,
	};

	typedef struct
	{
		CUtlSymbol			m_Name;
		int					filepos;
		int					filelen;
	} TmpFileInfo_t;

	unsigned int	m_SectorSize;
	bool			m_ForceAlignment;

	unsigned short	CalculatePadding( const unsigned int filenameLen, const int pos );
	void			SaveDirectory( IWriteStream& stream );

	// Internal entry for faster searching, etc.
	class CZipEntry
	{
	public:
					CZipEntry( void );
					~CZipEntry( void );

					CZipEntry( const CZipEntry& src );

		// RB tree compare function
		static bool ZipFileLessFunc( CZipEntry const& src1, CZipEntry const& src2 );

		// Name of entry
		CUtlSymbol	m_Name;
		// Offset ( set during write only )
		int			offset;
		// Lenth of data element
		int			length;
		// Raw data
		void		*data;
	};

	// For fast name lookup and sorting
	CUtlRBTree< CZipEntry, int > m_Files;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CZipFile::CZipEntry::CZipEntry( void )
{
	m_Name = "";
	offset = 0;
	length = 0;
	data = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : src - 
//-----------------------------------------------------------------------------
CZipFile::CZipEntry::CZipEntry( const CZipFile::CZipEntry& src )
{
	m_Name = src.m_Name;
	offset = src.offset;
	length = src.length;
	if ( length > 0 )
	{
		data = malloc( length );
		memcpy( data, src.data, length );
	}
	else
	{
		data = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Clear any leftover data
//-----------------------------------------------------------------------------
CZipFile::CZipEntry::~CZipEntry( void )
{
	free( data );
}

//-----------------------------------------------------------------------------
// Purpose: Construction
//-----------------------------------------------------------------------------
CZipFile::CZipFile( void )
: m_Files( 0, 32, CZipEntry::ZipFileLessFunc )
{
	m_SectorSize	 = 0;
	m_ForceAlignment = false;
}

//-----------------------------------------------------------------------------
// Purpose: Destroy zip data
//-----------------------------------------------------------------------------
CZipFile::~CZipFile( void )
{
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: Comparison for sorting entries
// Input  : src1 - 
//			src2 - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CZipFile::CZipEntry::ZipFileLessFunc( CZipEntry const& src1, CZipEntry const& src2 )
{
	return ( src1.m_Name < src2.m_Name );
}

void CZipFile::ForceAlignment( bool aligned, unsigned int sectorSize )
{
	m_ForceAlignment = aligned;
	m_SectorSize	 = sectorSize;

	if( !aligned )
		m_SectorSize = 0;
}

unsigned int CZipFile::GetAlignment()
{
	if ( !m_ForceAlignment || !m_SectorSize )
		return 0;

	return m_SectorSize;
}

//-----------------------------------------------------------------------------
// Purpose: Load pak file from raw buffer
// Input  : *buffer - 
//			bufferlength - 
//-----------------------------------------------------------------------------
void CZipFile::ParseFromBuffer( byte *buffer, int bufferlength )
{
	// Through away old data
	Reset();

	// Initialize a buffer
	CUtlBuffer buf( 0, bufferlength );
	buf.Put( buffer, bufferlength );

	buf.SeekGet( CUtlBuffer::SEEK_TAIL, 0 );
	int fileLen = buf.TellGet();

	// Start from beginning
	buf.SeekGet( CUtlBuffer::SEEK_HEAD, 0 );

	int offset;
	ZIP_EndOfCentralDirRecord rec;
	rec.startOfCentralDirOffset = 0;
	rec.nCentralDirectoryEntries_Total = 0;

	bool foundEndOfCentralDirRecord = false;
	for( offset = fileLen - sizeof( ZIP_EndOfCentralDirRecord ); offset >= 0; offset-- )
	{
		buf.SeekGet( CUtlBuffer::SEEK_HEAD, offset );
		buf.Get( &rec, sizeof( ZIP_EndOfCentralDirRecord ) );
		if( rec.signature == 0x06054b50 )
		{
			foundEndOfCentralDirRecord = true;

			// Grab the file's sector alignment size
			if( !m_ForceAlignment && rec.commentLength == sizeof( unsigned int ) )
				buf.Get( &m_SectorSize, sizeof( unsigned int ) );

			break;
		}
	}
	Assert( foundEndOfCentralDirRecord );
	
	buf.SeekGet( CUtlBuffer::SEEK_HEAD, rec.startOfCentralDirOffset );

	// Make sure there are some files to parse
	int numzipfiles = rec.nCentralDirectoryEntries_Total;
	if (  numzipfiles <= 0 )
	{
		// No files, sigh...
		return;
	}

	// Allocate space for directory
	TmpFileInfo_t *newfiles = new TmpFileInfo_t[ numzipfiles ];
	Assert( newfiles );

	int i;
	for( i = 0; i < rec.nCentralDirectoryEntries_Total; i++ )
	{
		ZIP_FileHeader fileHeader;
		buf.Get( &fileHeader, sizeof( ZIP_FileHeader ) );
		Assert( fileHeader.signature == 0x02014b50 );
		Assert( fileHeader.compressionMethod == 0 );
		
		// bogus. . .do we have to allocate this here?  should make a symbol instead.
		char tmpString[1024];
		buf.Get( tmpString, fileHeader.fileNameLength );
		tmpString[fileHeader.fileNameLength] = '\0';
		strlwr( tmpString );
		newfiles[i].m_Name = tmpString;
		newfiles[i].filepos = fileHeader.relativeOffsetOfLocalHeader;
		newfiles[i].filelen = fileHeader.compressedSize;
		buf.SeekGet( CUtlBuffer::SEEK_CURRENT, fileHeader.extraFieldLength + fileHeader.fileCommentLength );
	}

	for( i = 0; i < rec.nCentralDirectoryEntries_Total; i++ )
	{
		buf.SeekGet( CUtlBuffer::SEEK_HEAD, newfiles[i].filepos );
		ZIP_LocalFileHeader localFileHeader;
		buf.Get( &localFileHeader, sizeof( ZIP_LocalFileHeader ) );
		Assert( localFileHeader.signature == 0x04034b50 );
		buf.SeekGet( CUtlBuffer::SEEK_CURRENT, localFileHeader.fileNameLength + localFileHeader.extraFieldLength );
		newfiles[i].filepos = buf.TellGet();
	}

	// Insert current data into rb tree
	for ( i=0 ; i<numzipfiles ; i++ )
	{
		CZipEntry e;
		e.m_Name = newfiles[ i ].m_Name;
		e.length = newfiles[ i ].filelen;
		e.offset = 0;
		
		// Make sure length is reasonable
		if ( e.length > 0 )
		{
			e.data = malloc( e.length );

			// Copy in data
			buf.SeekGet( CUtlBuffer::SEEK_HEAD, newfiles[ i ].filepos );
			buf.Get( e.data, e.length );
		}
		else
		{
			e.data = NULL;
		}

		// Add to tree
		m_Files.Insert( e );
	}

	// Through away directory
	delete[] newfiles;
}

static int GetLengthOfBinStringAsText( const char *pSrc, int srcSize )
{
	const char *pSrcScan = pSrc;
	const char *pSrcEnd = pSrc + srcSize;
	int numChars = 0;
	for( ; pSrcScan < pSrcEnd; pSrcScan++ )
	{
		if( *pSrcScan == '\n' )
		{
			numChars += 2;
		}
		else
		{
			numChars++;
		}
	}
	return numChars;
}


//-----------------------------------------------------------------------------
// Copies text data from a form appropriate for disk to a normal string
//-----------------------------------------------------------------------------
static void ReadTextData( const char *pSrc, int nSrcSize, CUtlBuffer &buf )
{
	buf.EnsureCapacity( nSrcSize + 1 );
	const char *pSrcEnd = pSrc + nSrcSize;
	for ( const char *pSrcScan = pSrc; pSrcScan < pSrcEnd; ++pSrcScan )
	{
		if( *pSrcScan == '\r' )
		{
			if ( pSrcScan[1] == '\n' )
			{
				buf.PutChar( '\n' );
				++pSrcScan;
				continue;
			}
		}

		buf.PutChar( *pSrcScan );
	}
	
	// Null terminate
	buf.PutChar( '\0' );
}


//-----------------------------------------------------------------------------
// Copies text data into a form appropriate for disk
//-----------------------------------------------------------------------------
static void CopyTextData( char *pDst, const char *pSrc, int dstSize, int srcSize )
{
	const char *pSrcScan = pSrc;
	const char *pSrcEnd = pSrc + srcSize;
	char *pDstScan = pDst;

#ifdef _DEBUG
	char *pDstEnd = pDst + dstSize;
#endif

	for( ; pSrcScan < pSrcEnd; pSrcScan++ )
	{
		if( *pSrcScan == '\n' )
		{
			*pDstScan = '\r';
			pDstScan++;
			*pDstScan = '\n';
			pDstScan++;
		}
		else
		{
			*pDstScan = *pSrcScan;
			pDstScan++;
		}
	}
	Assert( pSrcScan == pSrcEnd );
	Assert( pDstScan == pDstEnd );
}

//-----------------------------------------------------------------------------
// Purpose: Adds a new lump, or overwrites existing one
// Input  : *relativename - 
//			*data - 
//			length - 
//-----------------------------------------------------------------------------
void CZipFile::AddBufferToZip( const char *relativename, void *data, int length, bool bTextMode )
{
	// Lower case only
	char name[ 512 ];
	strcpy( name, relativename );
	strlwr( name );

	int dstLength = length;
	if( bTextMode )
	{
		dstLength = GetLengthOfBinStringAsText( ( const char * )data, length );
	}
	
	// See if entry is in list already
	CZipEntry e;
	e.m_Name = name;
	int index = m_Files.Find( e );

	// If already existing, throw away old data and update data and length
	if ( index != m_Files.InvalidIndex() )
	{
		CZipEntry *update = &m_Files[ index ];
		free( update->data );
		if( bTextMode )
		{
			update->data = malloc( dstLength );
			CopyTextData( ( char * )update->data, ( char * )data, dstLength, length );
			update->length = dstLength;
		}
		else
		{
			update->data = malloc( length );
			memcpy( update->data, data, length );
			update->length = length;
		}
		update->offset = 0;
	}
	else
	{
		// Create a new entry
		e.length	= dstLength;
		if ( dstLength > 0 )
		{
			if( bTextMode )
			{
				e.data = malloc( dstLength );
				CopyTextData( ( char * )e.data, ( char * )data, dstLength, length );
			}
			else
			{
				e.data = malloc( length );
				memcpy(e.data, data, length );
			}
		}
		else
		{
			e.data = NULL;
		}
		e.offset	= 0;

		m_Files.Insert( e );
	}
}


//-----------------------------------------------------------------------------
// Reads a file from the zip
//-----------------------------------------------------------------------------
bool CZipFile::ReadFileFromZip( const char *pRelativeName, bool bTextMode, CUtlBuffer &buf )
{
	// Lower case only
	char pName[ 512 ];
	Q_strncpy( pName, pRelativeName, 512 );
	Q_strlower( pName );

	// See if entry is in list already
	CZipEntry e;
	e.m_Name = pName;
	int nIndex = m_Files.Find( e );

	// Didn't find it? We're done!
	if ( nIndex == m_Files.InvalidIndex() )
		return false;

	CZipEntry *pEntry = &m_Files[ nIndex ];
	if ( bTextMode )
	{
		buf.SetBufferType( true, false );
		ReadTextData( (char*)pEntry->data, pEntry->length, buf );
	}
	else
	{
		buf.SetBufferType( false, false );
		buf.Put( pEntry->data, pEntry->length );
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Check if a file already exists in the zip.
// Input  : *relativename - 
//-----------------------------------------------------------------------------
bool CZipFile::FileExistsInZip( const char *pRelativeName )
{
	// Lower case only
	char pName[ 512 ];
	Q_strncpy( pName, pRelativeName, 512 );
	Q_strlower( pName );

	// See if entry is in list already
	CZipEntry e;
	e.m_Name = pName;
	int nIndex = m_Files.Find( e );

	// If it is, then it exists in the pack!
	return nIndex != m_Files.InvalidIndex();
}


//-----------------------------------------------------------------------------
// Purpose: Adds a new file to the zip.
//-----------------------------------------------------------------------------
void CZipFile::AddFileToZip( const char *relativename, const char *fullpath )
{
	FILE *temp = fopen( fullpath, "rb" );
	if ( !temp )
		return;

	// Determine length
	fseek( temp, 0, SEEK_END );
	int size = ftell( temp );
	fseek( temp, 0, SEEK_SET );
	byte *buf = (byte *)malloc( size + 1 );

	// Read data
	fread( buf, size, 1, temp );
	fclose( temp );

	// Now add as a buffer
	AddBufferToZip( relativename, buf, size, false );
	
	free( buf );
}

//-----------------------------------------------------------------------------
// Purpose: Removes a file from the zip.
//-----------------------------------------------------------------------------
void CZipFile::RemoveFileFromZip( const char *relativename )
{
	CZipEntry e;
	e.m_Name = relativename;
	int index = m_Files.Find( e );

	if ( index != m_Files.InvalidIndex() )
	{
		CZipEntry update = m_Files[index];
		m_Files.Remove( update );
	}
}

//---------------------------------------------------------------
//	Purpose: Calculates how many bytes should be added to the extra field
//  to push the start of the file data to the next sector boundary
//  Output: Required padding size
//---------------------------------------------------------------
unsigned short CZipFile::CalculatePadding( const unsigned int filenameLen, const int pos )
{
	if( m_SectorSize == 0 )
		return 0;

	const long headerSize = sizeof(ZIP_LocalFileHeader) + filenameLen;
	return (unsigned short)( m_SectorSize - ( ( pos + headerSize ) % m_SectorSize ) );
}

//-----------------------------------------------------------------------------
// Purpose: Calculate the exact size of zip file, with headers and padding
// Output : int
//-----------------------------------------------------------------------------
int CZipFile::CalculateSize( void )
{
	if( m_SectorSize & (m_SectorSize - 1) )
	{
		m_SectorSize = 0;
	}

	unsigned int size = 0;
	unsigned int dirHeaders = 0;
	for ( int i = m_Files.FirstInorder(); i != m_Files.InvalidIndex(); i = m_Files.NextInorder( i ) )
	{
		CZipEntry *e = &m_Files[ i ];
		
		if( e->length == 0 )
			continue;

		// local file header
		size += sizeof( ZIP_LocalFileHeader );
		size += strlen( e->m_Name.String() );

		// every file has a directory header that duplicates the filename 
		dirHeaders += sizeof( ZIP_FileHeader ) + strlen( e->m_Name.String() );

		// calculate padding
		if( m_SectorSize != 0 )
		{
			// round up to next boundary
			unsigned int nextBoundary = ( size + m_SectorSize ) & ~( m_SectorSize - 1 );
			
			// the directory header also duplicates the padding
			dirHeaders += nextBoundary - size;

			size = nextBoundary;
		}

		// data size
		size += e->length;
	}

	size += dirHeaders;

	// All processed zip files will have a four-byte comment field tacked on
	size += sizeof( ZIP_EndOfCentralDirRecord ) + sizeof( unsigned int );
	return size;
}

//-----------------------------------------------------------------------------
// Purpose: Print a directory of files in the zip
//-----------------------------------------------------------------------------
void CZipFile::PrintDirectory( void )
{
	for ( int i = m_Files.FirstInorder(); i != m_Files.InvalidIndex(); i = m_Files.NextInorder( i ) )
	{
		CZipEntry *e = &m_Files[ i ];

		Msg( "%s\n", e->m_Name.String() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Iterate through directory
//-----------------------------------------------------------------------------
int CZipFile::GetNextFilename( int id, char *pBuffer, int bufferSize, int &fileSize )
{
	if ( id == -1 )
	{
		id = m_Files.FirstInorder();
	}
	else
	{
		id = m_Files.NextInorder( id );
	}
	if ( id == m_Files.InvalidIndex() )
	{
		// list is empty
		return -1;
	}

	CZipEntry *e = &m_Files[id];

	Q_strncpy( pBuffer, e->m_Name.String(), bufferSize );
	fileSize = e->length;

	return id;
}

//-----------------------------------------------------------------------------
// Purpose: Sets up alignment buffer
//-----------------------------------------------------------------------------
void SetAlignment( void *&buffer, unsigned int& sectorSize )
{
	if( sectorSize & (sectorSize - 1) )
	{
		sectorSize = 0;
		return;
	}

	buffer = malloc( sectorSize );
	memset( buffer, 0xFF, sectorSize );
}

//-----------------------------------------------------------------------------
// Purpose: Store data out to disk
//-----------------------------------------------------------------------------
void CZipFile::SaveToDisk( FILE *fout )
{
	CFileStream stream( fout );
	SaveDirectory( stream );
}

//-----------------------------------------------------------------------------
// Purpose: Store data out to a CUtlBuffer
//-----------------------------------------------------------------------------
void CZipFile::SaveToBuffer( CUtlBuffer& buf )
{
	CBufferStream stream( buf );
	SaveDirectory( stream );
}

//-----------------------------------------------------------------------------
// Purpose: Store data back out to a stream (could be CUtlBuffer or filestream)
//-----------------------------------------------------------------------------
void CZipFile::SaveDirectory( IWriteStream& stream )
{
	void *paddingBuffer = 0;
	if( m_SectorSize > 1 )
	{
		// SetAlignment validates sectorSize and allocates the paddingBuffer
		SetAlignment( paddingBuffer, m_SectorSize );
	}

	int i;
	for( i = m_Files.FirstInorder(); i != m_Files.InvalidIndex(); i = m_Files.NextInorder( i ) )
	{
		CZipEntry *e = &m_Files[ i ];
		Assert( e );

		// Fix up the offset
		e->offset = stream.Tell();

		if ( e->length > 0 && e->data != NULL )
		{
			ZIP_LocalFileHeader hdr;
			hdr.signature = 0x04034b50;
			hdr.versionNeededToExtract = 10;  // This is the version that the winzip that I have writes.
			hdr.flags = 0;
			hdr.compressionMethod = 0; // NO COMPRESSION!
			hdr.lastModifiedTime = 0;
			hdr.lastModifiedDate = 0;

			CRC32_t crc;
			CRC32_Init( &crc );
			CRC32_ProcessBuffer( &crc, e->data, e->length );
			CRC32_Final( &crc );
			hdr.crc32 = crc;
			
			hdr.compressedSize = e->length;
			hdr.uncompressedSize = e->length;
			hdr.fileNameLength = strlen( e->m_Name.String() );
			hdr.extraFieldLength = CalculatePadding( hdr.fileNameLength, e->offset );

			stream.Put( &hdr, sizeof( hdr ) );
			stream.Put( e->m_Name.String(), strlen( e->m_Name.String() ) );
			stream.Put( paddingBuffer, hdr.extraFieldLength );
			stream.Put( e->data, e->length );
		}
	}
	int centralDirStart = stream.Tell();
	int realNumFiles = 0;
	for( i = m_Files.FirstInorder(); i != m_Files.InvalidIndex(); i = m_Files.NextInorder( i ) )
	{
		CZipEntry *e = &m_Files[ i ];
		Assert( e );
		
		if ( e->length > 0 && e->data != NULL )
		{
			ZIP_FileHeader hdr;
			hdr.signature = 0x02014b50;
			hdr.versionMadeBy = 20; // This is the version that the winzip that I have writes.
			hdr.versionNeededToExtract = 10; // This is the version that the winzip that I have writes.
			hdr.flags = 0;
			hdr.compressionMethod = 0;
			hdr.lastModifiedTime = 0;
			hdr.lastModifiedDate = 0;

			// hack - processing the crc twice.
			CRC32_t crc;
			CRC32_Init( &crc );
			CRC32_ProcessBuffer( &crc, e->data, e->length );
			CRC32_Final( &crc );
			hdr.crc32 = crc;

			hdr.compressedSize = e->length;
			hdr.uncompressedSize = e->length;
			hdr.fileNameLength = strlen( e->m_Name.String() );
			hdr.extraFieldLength = CalculatePadding( hdr.fileNameLength, e->offset );
			hdr.fileCommentLength = 0;
			hdr.diskNumberStart = 0;
			hdr.internalFileAttribs = 0;
			hdr.externalFileAttribs = 0; // This is usually something, but zero is OK as if the input came from stdin
			hdr.relativeOffsetOfLocalHeader = e->offset;

			stream.Put( &hdr, sizeof( hdr ) );
			stream.Put( e->m_Name.String(), strlen( e->m_Name.String() ) );
			stream.Put( paddingBuffer, hdr.extraFieldLength );
			realNumFiles++;
		}
	}
	int centralDirEnd = stream.Tell();

	ZIP_EndOfCentralDirRecord rec;
	rec.signature = 0x06054b50;
	rec.numberOfThisDisk = 0;
	rec.numberOfTheDiskWithStartOfCentralDirectory = 0;
	rec.nCentralDirectoryEntries_ThisDisk = realNumFiles;
	rec.nCentralDirectoryEntries_Total = realNumFiles;
	rec.centralDirectorySize = centralDirEnd - centralDirStart;
	rec.startOfCentralDirOffset = centralDirStart;
	rec.commentLength = sizeof( unsigned int );

	unsigned int comment = m_SectorSize;
	stream.Put( &rec, sizeof( rec ) );
	stream.Put( &comment, sizeof( comment ) );

	if( paddingBuffer )
		free( paddingBuffer );
}

//-----------------------------------------------------------------------------
// Purpose: Delete all current data
//-----------------------------------------------------------------------------
void CZipFile::Reset( void )
{
	m_Files.RemoveAll();
}

class CZip : public IZip
{
public:
	CZip();
	virtual ~CZip();

	virtual void		Reset();

	// Add a single file to a zip - maintains the zip's previous alignment state
	virtual void		AddFileToZip		( const char *relativename, const char *fullpath );

	// Whether a file is contained in a zip - maintains alignment
	virtual bool		FileExistsInZip		( const char *pRelativeName );

	// Reads a file from the zip - maintains alignement
	virtual bool		ReadFileFromZip		( const char *pRelativeName, bool bTextMode, CUtlBuffer &buf );

	// Removes a single file from the zip - maintains alignment
	virtual void		RemoveFileFromZip	( const char *relativename );

	// Gets next filename in zip, for walking the directory - maintains alignment
	virtual int			GetNextFilename		( int id, char *pBuffer, int bufferSize, int &fileSize );

	// Prints the zip's contents - maintains alignment
	virtual void		PrintDirectory		( void );

	// Estimate the size of the Zip (including header, padding, etc.)
	virtual int			EstimateSize		( void );

	// Add buffer to zip as a file with given name - uses current alignment size, default 0 (no alignment)
	virtual void		AddBufferToZip		( const char *relativename, void *data, int length, bool bTextMode );

	// Writes out zip file to a buffer - uses current alignment size 
	// (set by file's previous alignment, or a call to ForceAlignment)
	virtual void		SaveToBuffer		( CUtlBuffer& outbuf );

	// Writes out zip file to a filestream - uses current alignment size 
	// (set by file's previous alignment, or a call to ForceAlignment)
	virtual void		SaveToDisk			( FILE *fout );

	// Reads a zip file from a buffer into memory - sets current alignment size to 
	// the file's alignment size, unless overridden by a ForceAlignment call)
	virtual void		ParseFromBuffer		( unsigned char *buffer, int bufferlength );

	// Forces a specific alignment size for all subsequent file operations, overriding files' previous alignment size.
	// Return to using files' individual alignment sizes by passing FALSE.
	virtual void		ForceAlignment		( bool aligned, unsigned int sectorSize );

	virtual unsigned int GetAlignment();

private:
	CZipFile			m_ZipFile;
};

static CZip g_ZipUtils;
IZip *zip_utils = &g_ZipUtils;

CZip::CZip( void )
{
	m_ZipFile.Reset();
}

CZip::~CZip()
{
	m_ZipFile.Reset();
}

void CZip::AddFileToZip( const char *relativename, const char *fullpath )
{
	m_ZipFile.AddFileToZip( relativename, fullpath );
}

bool CZip::FileExistsInZip( const char *pRelativeName )
{
	return m_ZipFile.FileExistsInZip( pRelativeName );
}

bool CZip::ReadFileFromZip( const char *pRelativeName, bool bTextMode, CUtlBuffer &buf )
{
	return m_ZipFile.ReadFileFromZip( pRelativeName, bTextMode, buf );
}

void CZip::RemoveFileFromZip( const char *relativename )
{
	m_ZipFile.RemoveFileFromZip( relativename );
}

int	CZip::GetNextFilename( int id, char *pBuffer, int bufferSize, int &fileSize )
{
	return m_ZipFile.GetNextFilename( id, pBuffer, bufferSize, fileSize );
}

void CZip::PrintDirectory( void )
{
	m_ZipFile.PrintDirectory();
}

void CZip::Reset()
{
	m_ZipFile.Reset();
}

int CZip::EstimateSize( void )
{
	return m_ZipFile.CalculateSize();
}

// Add buffer to zip as a file with given name
void CZip::AddBufferToZip( const char *relativename, void *data, int length, bool bTextMode )
{
	m_ZipFile.AddBufferToZip( relativename, data, length, bTextMode );
}

void CZip::SaveToBuffer( CUtlBuffer& outbuf )
{
	m_ZipFile.SaveToBuffer( outbuf );
}

void CZip::SaveToDisk( FILE *fout )
{
	m_ZipFile.SaveToDisk( fout );
}

void CZip::ParseFromBuffer( unsigned char *buffer, int bufferlength )
{
	m_ZipFile.Reset();
	m_ZipFile.ParseFromBuffer( buffer, bufferlength );
}

void CZip::ForceAlignment( bool aligned, unsigned int sectorSize=0 )
{
	m_ZipFile.ForceAlignment( aligned, sectorSize );
}

unsigned int CZip::GetAlignment()
{
	return m_ZipFile.GetAlignment();
}
