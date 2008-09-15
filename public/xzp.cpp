#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#ifdef _WIN32
#include <process.h>
#include <io.h>
#endif
#include <stddef.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "../public/vstdlib/strtools.h"

#ifdef _WIN32
#ifdef _XBOX
	#include "xbox/xbox_platform.h"
	#include "xbox/xbox_win32stubs.h"
	#include "xbox/xbox_core.h"
	#include "xbox/xbox_memory.h"
#else
	#include <windows.h>
#endif
#endif

#ifdef MAKE_XZIP_TOOL
	#include "../public/materialsystem/shader_vcs_version.h"
	#include "../public/materialsystem/imaterial.h"
	#include "../public/materialsystem/hardwareverts.h"
	#include "../public/vtf/vtf.h"
	#include "../public/riff.h"
#else
	#include "materialsystem/shader_vcs_version.h"
	#include "materialsystem/imaterial.h"
	#include "materialsystem/hardwareverts.h"
#endif


#include "xwvfile.h"
#include "xzp.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef Assert
void MyDebuggerBreak()
{
	__asm int 3
}
#define Assert( XXX ) XXX ? 0 : MyDebuggerBreak()
#endif

CXZip::CXZip()
{
	// Ensure that the header doesn't contain a valid magic yet.
	m_Header.Magic = 0;	
	m_pPreloadedData = NULL;
	m_nPreloadStart = 0;
	m_pDirectory = NULL;
	m_pPreloadDirectory = NULL;
	m_nRegular2PreloadEntryMapping = NULL;

	m_pFilenames = NULL;
	m_hZip = NULL;
	
	m_pRead = NULL;
	m_hUser = 0;
	m_nMonitorLevel = 0;

}

CXZip::CXZip( char* filename )
{
	// Ensure that the header doesn't contain a valid magic yet.
	m_Header.Magic = 0;	
	m_nPreloadStart = 0;
	m_pPreloadedData = NULL;
	m_pDirectory = NULL;
	m_pPreloadDirectory = NULL;
	m_nRegular2PreloadEntryMapping = NULL;
	m_pFilenames = NULL;
	m_hZip = NULL;

	m_pRead = NULL;
	m_hUser = 0;
	m_nMonitorLevel = 0;

	Load( filename );
}

CXZip::CXZip( FILE* handle, int offset, int size )	// file handle and offset of the zip file
{
	m_pRead = NULL;
	m_hUser = 0;
	m_nPreloadStart = 0;
	m_pDirectory = NULL;
	m_pPreloadDirectory = NULL;
	m_nRegular2PreloadEntryMapping = NULL;

	m_pFilenames = NULL;
	m_pPreloadedData = NULL;
	m_nMonitorLevel = 0;

	Load( handle, offset, size );
}

CXZip::~CXZip()
{
	Unload();
}

bool CXZip::InstallAlternateIO( int (*read)( void* buffer, int offset, int length, int nDestLength, int hUser), int hUser )
{
	m_pRead = read;
	m_hUser = hUser;
	return true;
}


// Loads an xZip file into memory:
bool CXZip::Load( char* filename, bool bPreload )
{
	FILE* hZip = fopen( filename, "rb" );
	fseek(hZip,0,SEEK_END);
	int nSize = ftell( hZip );
	return Load( hZip, 0, nSize, bPreload );
}

bool CXZip::Load( FILE* handle, int nOffset, int nSize, bool bPreload )	// Load a pack file into this instance.  Returns true on success.
{
	Unload();

	m_hZip = handle;
	m_nOffset = nOffset;
	m_nSize = nSize;

	// Hacky, clean up:
	if( m_hZip && !m_pRead )
	{
		InstallAlternateIO( defaultRead, (int)m_hZip );
	}

	if( m_hZip == NULL && m_pRead == NULL )
	{
		return false;
	}

	// Read the header:
	m_pRead( &m_Header, 0, -1, sizeof(m_Header), m_hUser );

	// Validate the magic number:
	if( m_Header.Magic != xZipHeader_t::MAGIC )
	{
		printf("Invalid xZip file\n");

		if( m_hZip )
		{
			fclose( m_hZip );
			m_hZip = NULL;
		}
		return false;
	}

	// Validate the archive version:
	if( m_Header.Version != xZipHeader_t::VERSION )
	{
		// Backward compatable support for version 1
		Msg("Incorrect xZip version found %u - expected %u\n", m_Header.Version, xZipHeader_t::VERSION );
		if( m_hZip )
		{
			fclose( m_hZip );
			m_hZip = NULL;
		}

		m_Header.Magic = xZipHeader_t::FREE;
		return false;
	}

	// Read the directory:
	{
	MEM_ALLOC_CREDIT();

	m_pDirectory = (xZipDirectoryEntry_t*)malloc( sizeof(xZipDirectoryEntry_t) * m_Header.DirectoryEntries );
	m_pRead( m_pDirectory, m_Header.HeaderLength, -1, sizeof( xZipDirectoryEntry_t ) * m_Header.DirectoryEntries, m_hUser );

	m_nPreloadStart = m_Header.HeaderLength + ( sizeof( xZipDirectoryEntry_t ) * m_Header.DirectoryEntries );
	}

	// Preload the preload chunk if desired:
	if( bPreload )
	{
		PreloadData();
	}

	return true;

}

void CXZip::Unload()
{
	DiscardPreloadedData();

	// Dump the directory:
	if( m_pDirectory )
	{
		free( m_pDirectory );
		m_pDirectory = NULL;
	}

	if( m_pFilenames )
	{
		free( m_pFilenames );
		m_pFilenames = NULL;
	}

	// Invalidate the header:
	m_Header.Magic = 0;

	if( m_hZip )
	{
		fclose( m_hZip );
		m_hZip = NULL;
	}

}

//-----------------------------------------------------------------------------
// CXZip::PreloadData
//
// Loads the preloaded data if it isn't already.
//-----------------------------------------------------------------------------

void CXZip::PreloadData()
{
	Assert( IsValid() );

	// Ensure it isn't already preloaded
	if( m_pPreloadedData )
		return;

	// If I don't have a preloaded section, ignore the request.
	if( !m_Header.PreloadBytes || !m_Header.PreloadDirectoryEntries )
		return;

	// Allocate and read the data block in:
#ifndef _XBOX
	MEM_ALLOC_CREDIT_("xZip");
	m_pPreloadedData = malloc( m_Header.PreloadBytes );

	// Just drop out if allocation fails;
	if( !m_pPreloadedData )
		return;

	m_pRead( m_pPreloadedData, m_nPreloadStart, -1, m_Header.PreloadBytes, m_hUser );
#else
	int nAlignedStart = AlignValue( ( m_nPreloadStart - 512 ) + 1, 512 );
	int nBytesToRead = AlignValue( ( m_nPreloadStart - nAlignedStart ) + m_Header.PreloadBytes, 512 );
	int nBytesBuffer = AlignValue( nBytesToRead, 512 );
	byte *pReadData = (byte *)XBX_AllocPreloadMemory( nBytesBuffer );
	
	// Just drop out if allocation fails;
	if( !pReadData )
		return; 
	
	MEM_ALLOC_CREDIT_("xZip");
	m_pRead( pReadData, nAlignedStart, nBytesBuffer,nBytesToRead, m_hUser );
	m_pPreloadedData = pReadData + ( m_nPreloadStart - nAlignedStart );
#endif

	// Set up the preload directory:
	m_pPreloadDirectory = (xZipDirectoryEntry_t*)m_pPreloadedData;

	// Set up the regular 2 preload mapping section:
	m_nRegular2PreloadEntryMapping = (unsigned short*)(((unsigned char*)m_pPreloadDirectory) + ( sizeof(xZipDirectoryEntry_t) * m_Header.PreloadDirectoryEntries ));
}

//-----------------------------------------------------------------------------
// CXZip::DiscardPreloadedData
//
// frees the preloaded data cache if it's present.
//-----------------------------------------------------------------------------

void CXZip::DiscardPreloadedData()
{
	if( m_pPreloadedData )
	{
#ifndef _XBOX
		free( m_pPreloadedData );
#else
		int nAlignedStart = AlignValue( ( m_nPreloadStart - 512 ) + 1, 512 );
		byte *pReadData = (byte *)m_pPreloadedData - ( m_nPreloadStart - nAlignedStart );
		XBX_FreePreloadMemory( pReadData );
#endif
		m_pPreloadedData = NULL;
		m_pPreloadDirectory = NULL;
		m_nRegular2PreloadEntryMapping = NULL;
	}
}

int CXZip::defaultRead( void* buffer, int offset, int destLength, int length, int hUser)
{
	fseek( (FILE*)hUser, offset, SEEK_SET );
	return fread( buffer, 1, length, (FILE*)hUser );
}

char* CXZip::GetEntryFileName( unsigned CRC, char* pDefault )
{
	Assert( IsValid() );

	if( IsRetail() )
	{
		return pDefault;
	}
	else
	{

		// Make sure I have a filename section:
		if( m_Header.FilenameStringsOffset == 0 || m_Header.FilenameEntries == 0 || CRC == 0 )
		{
			return pDefault;
		}

		// If the filename chunk isn't here, load it up:
		if( !m_pFilenames )
		{
			MEM_ALLOC_CREDIT_("xZip");
			m_pFilenames = (xZipFilenameEntry_t*)malloc( m_Header.FilenameStringsLength );
			m_pRead( m_pFilenames, m_Header.FilenameStringsOffset, -1, m_Header.FilenameStringsLength, m_hUser );
		}

		// Find this entry in the preload directory
		xZipFilenameEntry_t entry;
		entry.FilenameCRC = CRC;

		xZipFilenameEntry_t* found = (xZipFilenameEntry_t*)bsearch( &entry, m_pFilenames, m_Header.FilenameEntries, sizeof(xZipFilenameEntry_t), xZipFilenameEntry_t::xZipFilenameEntryCompare );

		if( !found ) 
			return pDefault;

		return (((char*)m_pFilenames) + found->FilenameOffset) - m_Header.FilenameStringsOffset; 	
	}
}

// Sanity checks that the zip file is ready and readable:
bool CXZip::IsValid()
{
	if( m_Header.Magic != xZipHeader_t::MAGIC )
		return false;

	if( m_Header.Version > xZipHeader_t::VERSION )
		return false;

	if( !m_pDirectory )
		return false;

	return true;
}

void CXZip::WarningDir()
{
	Assert( IsValid());
	
	for( unsigned i = 0; i< m_Header.DirectoryEntries; i++ )
	{
		Msg( GetEntryFileName( m_pDirectory[i].FilenameCRC ) );
	}
}


int CXZip::ReadIndex( int nEntryIndex, int nFileOffset, int nDestBytes, int nLength, void* pBuffer )
{
	Assert( IsValid() );

	if( nLength <=0 || nEntryIndex < 0 ) 
		return 0;

	// HACK HACK HACK - convert the pack file index to a local file index (ie, assuming the full file index is being passed in)
	nFileOffset -= m_pDirectory[nEntryIndex].StoredOffset;
	// HACK HACK HACK

	// If I've got my preload section loaded, first check there:
	xZipDirectoryEntry_t* pPreloadEntry = GetPreloadEntry(nEntryIndex);

	if( pPreloadEntry )
	{
		Assert( pPreloadEntry->FilenameCRC == m_pDirectory[nEntryIndex].FilenameCRC );

		if( nFileOffset + nLength <= (int)pPreloadEntry->Length )
		{
			if( m_nMonitorLevel >= 2 )
			{
				char* filename = GetEntryFileName( m_pDirectory[nEntryIndex].FilenameCRC, "(!!! unknown !!!)" );

				Msg("PACK(preload) %s: length:%i offset:%i",filename,nLength, nFileOffset);

			}

			memcpy( pBuffer, (char*)m_pPreloadedData + pPreloadEntry->StoredOffset + nFileOffset - m_nPreloadStart, nLength );
			return nLength;
		}
	}

	// Offset int the zip to start the read:
	int ZipOffset = m_pDirectory[nEntryIndex].StoredOffset + nFileOffset;
	int nBytesRead = m_pRead( pBuffer, ZipOffset, nDestBytes, nLength, m_hUser);

	if( m_nMonitorLevel )
	{
		char* filename = GetEntryFileName( m_pDirectory[nEntryIndex].FilenameCRC, "(!!! unknown !!!)" );

		unsigned preload = 0;
		if( m_pPreloadedData && m_nRegular2PreloadEntryMapping[nEntryIndex] != 0xFFFF )
		{
			// Find this entry in the preload directory
			xZipDirectoryEntry_t* entry = &(m_pPreloadDirectory[m_nRegular2PreloadEntryMapping[nEntryIndex]]);
			Assert(entry->FilenameCRC == m_pDirectory[nEntryIndex].FilenameCRC);

			preload = entry->Length;
		}

		Msg("PACK %s: length:%i offset:%i (preload bytes:%i)",filename,nLength, nFileOffset, preload);
	}
	
	return nBytesRead;
}

bool CXZip::GetSimpleFileOffsetLength( const char* FileName, int& nBaseIndex, int &nFileOffset, int &nLength )
{
	Assert( IsValid() );

	xZipDirectoryEntry_t entry;
	entry.FilenameCRC = xZipCRCFilename( (char *)FileName );

	xZipDirectoryEntry_t* found = (xZipDirectoryEntry_t*)bsearch( &entry, m_pDirectory, m_Header.DirectoryEntries, sizeof(xZipDirectoryEntry_t), xZipDirectoryEntry_t::xZipDirectoryEntryFindCompare );

	if( found == NULL )
		return false;

	nFileOffset = found[0].StoredOffset;
	nLength = found[0].Length;
	nBaseIndex = (((int)((char*)found - (char*)m_pDirectory))/sizeof(xZipDirectoryEntry_t));

	return true;
}

bool CXZip::ExtractFile( const char* FileName )
{
	return false;
}

// Compares to xZipDirectoryEntries.
//
// Sorts in the following order:  
//		FilenameCRC
//		FileOffset
//		Length
//		StoredOffset
//
// The sort function may look overly complex, but it is actually useful for locating different pieces of 
// the same file in a meaningful order.
//
int __cdecl xZipDirectoryEntry_t::xZipDirectoryEntrySortCompare( const void* left, const void* right )
{
	xZipDirectoryEntry_t *l = (xZipDirectoryEntry_t*)left,
					     *r = (xZipDirectoryEntry_t*)right;

	if( l->FilenameCRC < r->FilenameCRC )
	{
		return -1;
	}

	else if( l->FilenameCRC > r->FilenameCRC )
	{
		return 1;
	}

	// else l->FileOffset == r->FileOffset
	if( l->Length < r->Length )
	{
		return -1;
	}
	else if( l->Length > r->Length )
	{
		return 1;
	}

	// else l->Length == r->Length
	if( l->StoredOffset < r->StoredOffset )
	{
		return -1;
	}
	else if( l->StoredOffset > r->StoredOffset )
	{
		return 1;
	}

	// else everything is identical:
	return 0;

}

// Find an entry with matching CRC only
int __cdecl xZipDirectoryEntry_t::xZipDirectoryEntryFindCompare( const void* left, const void* right )
{
	xZipDirectoryEntry_t *l = (xZipDirectoryEntry_t*)left,
					     *r = (xZipDirectoryEntry_t*)right;

	if( l->FilenameCRC < r->FilenameCRC )
	{
		return -1;
	}

	else if( l->FilenameCRC > r->FilenameCRC )
	{
		return 1;
	}

	return 0;

}

int __cdecl xZipFilenameEntry_t::xZipFilenameEntryCompare( const void* left, const void* right )
{
	xZipFilenameEntry_t *l = (xZipFilenameEntry_t*)left,
					    *r = (xZipFilenameEntry_t*)right;

	if( l->FilenameCRC < r->FilenameCRC )
	{
		return -1;
	}

	else if( l->FilenameCRC > r->FilenameCRC )
	{
		return 1;
	}

	return 0;

}


// CRC's an individual xZip filename:
unsigned xZipCRCFilename( char* filename )
{
	unsigned hash = 0xAAAAAAAA;	// Alternating 1's and 0's

	for( ; *filename ; filename++ )
	{
		char c = *filename;
	
		// Fix slashes
		if( c == '/' )
			c = '\\';
		else
			c = (char)tolower(c);

		hash = hash * 33 + c;
	}

	return hash;
}

#ifdef MAKE_XZIP_TOOL

// ------------
xZipHeader_t Header;
xZipDirectoryEntry_t	 *pDirectoryEntries = NULL;
xZipDirectoryEntry_t	 *pPreloadDirectoryEntries = NULL;
xZipFilenameEntry_t		 *pFilenameEntries = NULL;
char					 *pFilenameData = NULL;
unsigned				  nFilenameDataLength = 0;

unsigned			     InputFileBytes = 0;

char* CleanFilename( char* filename )
{
	// Trim leading white space:
	while( isspace(*filename) )
		filename++;

	// Trim trailing white space:
	while( isspace( filename[strlen(filename)-1] ) )
	{
		filename[strlen(filename)-1] = '\0';
	}

	return filename;
}


bool CopyFileBytes( FILE* hDestination, FILE* hSource, unsigned nBytes )
{
	char buffer[16384];

	while( nBytes > 0 )
	{
		int nBytesRead = fread( buffer, 1, nBytes > sizeof(buffer) ? sizeof(buffer) : nBytes,  hSource );
		fwrite(buffer, 1, nBytesRead,  hDestination );
		nBytes -= nBytesRead;
	}

	return true;
}

void PadFileBytes(FILE* hFile, int nPreloadPadding )
{
	if( nPreloadPadding < 0 || nPreloadPadding >= 512)
	{
		puts("Invalid padding");
		return;
	}
	
	char padding[512];
	memset(padding,0,nPreloadPadding);
	fwrite(padding,1,nPreloadPadding,hFile);
}

void AddFilename( char* filename )
{
	unsigned CRCfilename = xZipCRCFilename( filename );

	// If we already have this filename don't add it again:
	for( int i = 0; i < (int)Header.FilenameEntries; i++ )
	{
		if( pFilenameEntries[i].FilenameCRC == CRCfilename )
		{
			return;
		}
	}

	Header.FilenameEntries++;
	
	// Add the file to the file string table:
	pFilenameEntries = (xZipFilenameEntry_t*)realloc( pFilenameEntries, sizeof(xZipFilenameEntry_t) * Header.FilenameEntries );
	
	int filenameLength = (int)strlen(filename) + 1;
	pFilenameEntries[Header.FilenameEntries-1].FilenameCRC = CRCfilename;
	pFilenameEntries[Header.FilenameEntries-1].FilenameOffset = nFilenameDataLength;

	// Grab the timestamp for the file:
	struct stat buf;
	if( stat( filename, &buf ) != -1 )
	{
		pFilenameEntries[Header.FilenameEntries - 1].TimeStamp = buf.st_mtime;
	}
	else
	{
		pFilenameEntries[Header.FilenameEntries - 1].TimeStamp = 0;
	}

	nFilenameDataLength += filenameLength;
	pFilenameData = (char*)realloc(pFilenameData, nFilenameDataLength);
	memcpy(pFilenameData + nFilenameDataLength - filenameLength, filename, filenameLength);
}

FILE* hTempFilePreload;
FILE* hTempFileData;
FILE* hOutputFile;

bool xZipAddFile( char* filename, bool bPrecacheEntireFile, bool bProcessPrecacheHeader, bool bProcessPrecacheHeaderOnly )
{
	// Clean up the filename:
	char buffer[MAX_PATH];
	strcpy(buffer, filename);

	// Fix slashes and convert it to lower case:
	for( filename = buffer; *filename; filename++ )
	{
		if( *filename == '/' )
			*filename = '\\';
		else
		{
			*filename = (char)tolower(*filename);
		}
	}

	// Skip leading white space:
	for( filename = buffer; isspace(*filename); filename++ )
		;

	// Obliterate trailing white space:
	for(;;)
	{
		int len = (int)strlen( filename );
		if( len <= 0 )
		{
			printf("!!!! BAD FILENAME: \"%s\"\n", filename );
			return false;
		}
		
		if( isspace( filename[len-1] ) )
			filename[len-1]='\0';
		else
			break;
	}

	// Ensure we don't already have this file:
	unsigned CRC = xZipCRCFilename( filename );

	for( unsigned i=0; i < Header.DirectoryEntries; i++ )
	{
		if( pDirectoryEntries[i].FilenameCRC == CRC )
		{
			printf("!!!! NOT ADDING DUPLICATE FILENAME: \"%s\"\n", filename );
			return false;
		}
	}

	// Attempt to open the file:
	FILE* hFile = fopen( filename, "rb" );
	if( !hFile )
	{
		printf("!!!! FAILED TO OPEN FILE: \"%s\"\n", filename );
		return false;
	}

	// Get the length of the file:
	fseek(hFile,0,SEEK_END);
	unsigned fileSize = ftell(hFile);
	fseek(hFile,0,SEEK_SET);

	// Track total input bytes for stats reasons
	InputFileBytes += fileSize;

	unsigned customPreloadSize = 0;
	
	if( bPrecacheEntireFile  )
	{
		customPreloadSize = fileSize;
	} 
	else if( bProcessPrecacheHeader )
	{
		customPreloadSize = xZipComputeCustomPreloads( filename );
	}
	else if( bProcessPrecacheHeaderOnly )
	{
		customPreloadSize = xZipComputeCustomPreloads( filename );
		fileSize = min( fileSize, customPreloadSize );
	}

	// Does this file have a split header?
	if( customPreloadSize > 0  ) 
	{
		// Initialize the entry header:
		xZipDirectoryEntry_t entry;
		memset( &entry, 0, sizeof( entry ) );
		
		entry.FilenameCRC = CRC;
		entry.Length = customPreloadSize;
		entry.StoredOffset = ftell(hTempFilePreload);

		// Add the directory entry to the preload table:
		Header.PreloadDirectoryEntries++;
		pPreloadDirectoryEntries = (xZipDirectoryEntry_t*)realloc( pPreloadDirectoryEntries, sizeof( xZipDirectoryEntry_t ) * Header.PreloadDirectoryEntries );
		memcpy( pPreloadDirectoryEntries + Header.PreloadDirectoryEntries - 1, &entry, sizeof( entry ) );

		// Concatenate the data in the preload file:
		fseek(hFile,0,SEEK_SET);
		CopyFileBytes( hTempFilePreload, hFile, entry.Length );
		fseek(hFile,0,SEEK_SET);
		
		// Add the filename entry:
		AddFilename( filename );

		// Spew it:
		printf("+Preload: \"%s\": Length:%u\n", filename, entry.Length );
	}

	// Copy the file to the regular data region:
	xZipDirectoryEntry_t entry;
	memset(&entry,0,sizeof(entry));
	entry.FilenameCRC = CRC;
	entry.Length = fileSize;
	entry.StoredOffset = ftell(hTempFileData);		

	// Add the directory entry to the table:
	Header.DirectoryEntries++;
	pDirectoryEntries = (xZipDirectoryEntry_t*)realloc( pDirectoryEntries, sizeof( xZipDirectoryEntry_t ) * Header.DirectoryEntries );
	memcpy( pDirectoryEntries + Header.DirectoryEntries - 1, &entry, sizeof( entry ) );

	CopyFileBytes( hTempFileData, hFile, entry.Length );

	
	// Align the data region to a 512 byte boundry:  (has to be on last entry as well to ensure enough space to perform the final read,
	// and initial alignment is taken careof by assembexzip)
	int nPadding = ( 512 - ( ftell( hTempFileData ) % 512) ) % 512;

	PadFileBytes( hTempFileData, nPadding );

	// Add the file to the file string table:
	AddFilename( filename );

	// Print a summary
	printf("+File: \"%s\": Length:%u Padding:%i\n", filename,  entry.Length, nPadding );
	fclose(hFile);

	return true;
}

int xZipBegin( char* fileNameXzip )
{
	// Create and initialize the header:
	memset( &Header, 0, sizeof(Header) );	// Zero out the header:
	Header.Magic = xZipHeader_t::MAGIC;
	Header.Version = xZipHeader_t::VERSION;
	Header.HeaderLength = sizeof(Header);

	// Open the output file:
	hOutputFile = fopen(fileNameXzip,"wb+");
	if( !hOutputFile )
	{
		printf("Failed to open \"%s\" for writing.\n", fileNameXzip);
		exit( EXIT_FAILURE);
	}

	
	// Create a temporary file for storing the preloaded data:
	hTempFilePreload = tmpfile();
	if( !hTempFilePreload )
	{
		printf( "Error: failed to create temporary file\n" );
		return EXIT_FAILURE;
	}

	// Create a temporary file for storing the non preloaded data
	hTempFileData = tmpfile();
	if( !hTempFileData )
	{
		printf( "Error: failed to create temporary file\n");
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;

}

bool xZipEnd()
{
	int nPreloadDirectorySize = sizeof(xZipDirectoryEntry_t)*Header.PreloadDirectoryEntries;
	int nRegular2PreloadSize = sizeof(unsigned short) * Header.DirectoryEntries;

	// Compute the size of the preloaded section:
	if( Header.PreloadDirectoryEntries )
	{
		fseek( hTempFilePreload, 0, SEEK_END );
		Header.PreloadBytes = ftell(hTempFilePreload) + nPreloadDirectorySize + nRegular2PreloadSize;	// Raw# of bytes to preload
		fseek( hTempFilePreload, 0, SEEK_SET );
	}
	else
	{
		Header.PreloadBytes = 0;
	}

	// Number of bytes preceeding the preloaded section:
	int nPreloadOffset = sizeof( Header ) + ( sizeof( xZipDirectoryEntry_t ) * Header.DirectoryEntries );

	// Number of bytes to pad between the end of the preload section and the start of the data section:
	int nPadding = ( 512 - ( ( nPreloadOffset + Header.PreloadBytes ) % 512) ) %512;				// Number of alignment bytes after the preload section


	// Offset past the preload section:
	int nDataOffset = nPreloadOffset + Header.PreloadBytes + nPadding;

	// Write out the header: (will need to be rewritten at the end as well)
	fwrite(&Header,sizeof(Header),1,hOutputFile);


	// Fixup each of the directory entries to make them relative to the beginning of the file.
	for( unsigned i=0; i< Header.DirectoryEntries;i++ )
	{
		xZipDirectoryEntry_t* pDir = &(pDirectoryEntries[i]);

		// Adjust files in the regular data area:
		pDir->StoredOffset = nDataOffset + pDir->StoredOffset;
	}

	// Sort and write the directory:
	printf("Sorting and writing %i directory entries...\n",Header.DirectoryEntries);
	qsort(pDirectoryEntries,Header.DirectoryEntries,sizeof(xZipDirectoryEntry_t),&xZipDirectoryEntry_t::xZipDirectoryEntrySortCompare); 
	fwrite(pDirectoryEntries,Header.DirectoryEntries*sizeof(xZipDirectoryEntry_t),1, hOutputFile);

	// Copy the preload section:
	if( Header.PreloadBytes > 0 )
	{
		printf("Generating the preload section...(%u)\n", Header.PreloadBytes);


		// Fixup each of the directory entries to make them relative to the beginning of the file.
		for( unsigned i=0; i< Header.PreloadDirectoryEntries;i++ )
		{
			xZipDirectoryEntry_t* pDir = &(pPreloadDirectoryEntries[i]);

			// Shift preload data down by preload bytes (and skipping over the directory):
			pDir->StoredOffset += nPreloadOffset + nPreloadDirectorySize + nRegular2PreloadSize;
		}

		printf("Sorting %u preload directory entries...\n",Header.PreloadDirectoryEntries);
		qsort(pPreloadDirectoryEntries,Header.PreloadDirectoryEntries,sizeof(xZipDirectoryEntry_t),&xZipDirectoryEntry_t::xZipDirectoryEntrySortCompare); 


		printf("Building regular to preload mapping table for %u entries...\n", Header.DirectoryEntries );
		unsigned short* Regular2Preload = (unsigned short*)malloc( nRegular2PreloadSize );
		for( unsigned i = 0; i < Header.DirectoryEntries; i++ )
		{
			unsigned short j;
			for( j = 0; j < Header.PreloadDirectoryEntries; j++ )
			{
				if( pDirectoryEntries[i].FilenameCRC == pPreloadDirectoryEntries[j].FilenameCRC )
					break;
			}

			// If I couldn't find it mark it as non-existant:
			if( j == Header.PreloadDirectoryEntries )
				j = 0xFFFF;

			Regular2Preload[i] = j;
		}

		printf("Writing preloaded directory entreis...\n" );
		fwrite( pPreloadDirectoryEntries, Header.PreloadDirectoryEntries*sizeof(xZipDirectoryEntry_t),1, hOutputFile );

		printf("Writing regular to preload mapping (%u bytes)...\n", sizeof(unsigned short)*Header.DirectoryEntries );
		fwrite( Regular2Preload, nRegular2PreloadSize,1,hOutputFile );

		printf("Copying %u Preloadable Bytes...\n", Header.PreloadBytes - nPreloadDirectorySize - nRegular2PreloadSize );
		fseek(hTempFilePreload,0,SEEK_SET);
		CopyFileBytes(hOutputFile, hTempFilePreload, Header.PreloadBytes - nPreloadDirectorySize - nRegular2PreloadSize  );
	}

	// Align the data section following the preload section:
	if( nPadding )
	{
		printf("Aligning Data Section Start by %u bytes...\n", nPadding );
		PadFileBytes(hOutputFile, nPadding );
	}

	// Copy the data section:
	fseek(hTempFileData, 0, SEEK_END );
	unsigned length = ftell( hTempFileData );
	fseek(hTempFileData, 0, SEEK_SET );
	printf("Copying %u Bytes...\n",length);

	CopyFileBytes(hOutputFile, hTempFileData, length);

	// Write out the filename data if present:
	if( nFilenameDataLength && Header.FilenameEntries )
	{
		Header.FilenameStringsOffset = ftell(hOutputFile);
		Header.FilenameStringsLength = (Header.FilenameEntries*sizeof(xZipFilenameEntry_t)) + nFilenameDataLength;
		
		// Adjust the offset in each of the filename offsets to absolute position in the file.
		for( unsigned i=0;i<Header.FilenameEntries;i++ )
		{
			pFilenameEntries[i].FilenameOffset += ( Header.FilenameStringsOffset + (Header.DirectoryEntries*sizeof(xZipFilenameEntry_t)));
		}

		printf("Sorting and writing %u filename directory entries...\n",Header.FilenameEntries);

		// Sort the data:
		qsort(pFilenameEntries,Header.FilenameEntries,sizeof(xZipFilenameEntry_t),&xZipFilenameEntry_t::xZipFilenameEntryCompare); 

		// Write the data out:
		fwrite(pFilenameEntries,1,Header.FilenameEntries*sizeof(xZipFilenameEntry_t),hOutputFile);

		printf("Writing %u bytes of filename data...\n",nFilenameDataLength);
		fwrite(pFilenameData,1,nFilenameDataLength,hOutputFile);
	}



	// Compute the total file size, including the size of the footer:
	unsigned OutputFileBytes = ftell(hOutputFile) + sizeof(xZipFooter_t);

	// Write the footer:
	xZipFooter_t footer;
	footer.Magic = xZipFooter_t::MAGIC;
	footer.Size = OutputFileBytes ;
	fwrite( &footer, 1, sizeof(footer), hOutputFile );

	// Seek back and rewrite the header (filename data changes it for example)
	fseek(hOutputFile,0,SEEK_SET);
	fwrite(&Header,1,sizeof(Header),hOutputFile);

	// Shut down
	fclose(hOutputFile);

	// Print the summary
	printf("\n\nSummary: Input:%u, XZip:%u, Directory Entries:%u (%u preloaded), Preloaded Bytes:%u\n\n",InputFileBytes,OutputFileBytes,Header.DirectoryEntries, Header.PreloadDirectoryEntries, Header.PreloadBytes);

	// Shut down:
	fclose(hTempFileData);
	fclose(hTempFilePreload);

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Implements Audio IO on the engine's COMMON filesystem
//-----------------------------------------------------------------------------
class COM_IOReadBinary : public IFileReadBinary
{
public:
	int open( const char *pFileName )
	{
		return _open( pFileName,_O_RDONLY|_O_BINARY );
	}
	int read( void *pOutput, int size, int file )
	{
		return _read( file, pOutput, size );
	}
	void seek( int file, int pos )
	{
		lseek( file, pos, SEEK_SET );
	}
	unsigned int tell( int file )
	{
		return lseek( file, 0, SEEK_CUR );
	}
	unsigned int size( int file )
	{
		long	pos;
		long	length;
		pos = lseek( file, 0, SEEK_CUR );
		length = lseek( file, 0, SEEK_END );
		lseek( file, pos, SEEK_SET );
		return (length);
	}
	void close( int file )
	{
		_close( file );
	}
};
static COM_IOReadBinary io;
IFileReadBinary *g_pSndIO = &io;

#define PADD_ID	MAKEID('P','A','D','D')

//-----------------------------------------------------------------------------
// xZipComputeWAVPreload
//
// Returns the number of bytes from a xbox compliant WAV file that should go into 
// the preload section:
//-----------------------------------------------------------------------------
unsigned xZipComputeWAVPreload( char *pFileName )
{
	InFileRIFF riff( pFileName, *g_pSndIO );
	if ( riff.RIFFName() != RIFF_WAVE )
	{
		return 0;
	}

	IterateRIFF walk( riff, riff.RIFFSize() );

	while ( walk.ChunkAvailable() )
	{
		// xbox compliant wavs have a single PADD chunk
		if ( walk.ChunkName() == PADD_ID )
		{
			// want to preload data up through PADD chunk header
			// and not the actual pad bytes
			return walk.ChunkFilePosition() + 2*sizeof( int );
		}
		walk.ChunkNext();
	}

	return 0;
}


//-----------------------------------------------------------------------------
// xZipComputeXWVPreload
//
// Returns the number of bytes from a XWV file that should go into the preload
// section:
//-----------------------------------------------------------------------------
unsigned xZipComputeXWVPreload( char* filename )
{
	FILE* hFile = fopen( filename, "rb" );
	if ( !hFile )
	{
		printf( "Failed to open xwv file: %s\n", filename );
		return 0;
	}

	// Read and validate the XWV header:
	xwvheader_t header;
	memset( &header, 0, sizeof(header) );
	fread( &header, 1, sizeof(header), hFile );
	fclose( hFile );
	
	if ( header.id != XWV_ID || header.headerSize != sizeof(header) )
		return 0;

	return header.GetPreloadSize();
}

unsigned xZipComputeXTFPreload( char* filename )
{
	FILE* hFile = fopen( filename, "rb" );
	if( !hFile )
	{
		printf("Failed to open file: %s\n", filename);
		return 0;
	}

	XTFFileHeader_t header;
	memset( &header,0, sizeof( header ) );
	fread( &header,1,sizeof(header),hFile);

	fclose(hFile);

	if( !strncmp(header.fileTypeString,"XTF",4) )
		return header.preloadDataSize;

	return 0;
}

// TODO: ONLY store them in the preload section:
unsigned xZipComputeVMTPreload( char* filename )
{
	// Store VMT's entirely
	if( !strstr(filename,".vmt") )
		return 0;

	FILE* hFile = fopen( filename, "rb" );
	if( !hFile )
	{
		printf("Failed to open file: %s\n", filename);
		return 0;
	}

	fseek(hFile,0,SEEK_END);
	unsigned offset = ftell(hFile);
	fclose(hFile);
	return offset;
}

// TODO: ONLY store them in the preload section:
unsigned xZipComputeVHVPreload( char* filename )
{
	// Store VMT's entirely
	if( !strstr(filename,".vhv") )
		return 0;

	FILE* hFile = fopen( filename, "rb" );
	if( !hFile )
	{
		printf("Failed to open file: %s\n", filename);
		return 0;
	}

	fclose(hFile);

	// Just load the header:
	return sizeof(HardwareVerts::FileHeader_t);
}

unsigned xZipComputeXCSPreload( char* filename )
{
	if( !strstr(filename,".vcs") )
		return 0;

	FILE* hFile = fopen( filename, "rb" );
	if( !hFile )
	{
		printf("Failed to open file: %s\n", filename);
		return 0;
	}

	XShaderHeader_t header;
	fread(&header,1,sizeof(XShaderHeader_t), hFile);
	fseek(hFile,0,SEEK_END);
	fclose(hFile);
	
	if(!header.IsValid())
		return 0;

	return header.BytesToPreload();
}

unsigned xZipComputeCustomPreloads( char* filename )
{
	strlwr(filename);

	unsigned offset = xZipComputeXWVPreload( filename );
	if( offset )
		return offset;

	offset = xZipComputeVMTPreload( filename );
	if( offset )
		return offset;

	offset = xZipComputeXCSPreload( filename );
	if( offset )
		return offset;

	offset = xZipComputeVHVPreload( filename );
	if( offset )
		return offset;

	return xZipComputeXTFPreload( filename );
}

#endif // MAKE_XZIP_TOOL
