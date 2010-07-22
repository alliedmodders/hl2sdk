//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#include "closedcaptions.h"
#include "filesystem.h"
#include "tier1/utlbuffer.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Assumed to be set up by calling code
bool AsyncCaption_t::LoadFromFile( const char *pRelativePath )
{
	char pRelativePathFixed[MAX_PATH];
	Q_strncpy( pRelativePathFixed, pRelativePath, sizeof(pRelativePathFixed) );
	Q_FixSlashes( pRelativePathFixed );
	Q_strlower( pRelativePathFixed );
	pRelativePath = pRelativePathFixed;

	if ( Q_IsAbsolutePath( pRelativePath ) )
	{
		Warning( "AsyncCaption_t::LoadFromFile: Fullpath encountered! %s\n", pRelativePath );
	}

	FileHandle_t fh = g_pFullFileSystem->Open( pRelativePath, "rb", "GAME" );
	if ( FILESYSTEM_INVALID_HANDLE == fh )
		return false;

	MEM_ALLOC_CREDIT();

	CUtlBuffer dirbuffer;

	// Read the header
	g_pFullFileSystem->Read( &m_Header, sizeof( m_Header ), fh );
	if ( m_Header.magic != COMPILED_CAPTION_FILEID )
		Error( "Invalid file id for %s\n", pRelativePath );
	if ( m_Header.version != COMPILED_CAPTION_VERSION )
		Error( "Invalid file version for %s\n", pRelativePath );
	if ( m_Header.directorysize < 0 || m_Header.directorysize > 64 * 1024 )
		Error( "Invalid directory size %d for %s\n", m_Header.directorysize, pRelativePath );
	//if ( m_Header.blocksize != MAX_BLOCK_SIZE )
	//	Error( "Invalid block size %d, expecting %d for %s\n", m_Header.blocksize, MAX_BLOCK_SIZE, pchFullPath );

	int directoryBytes = m_Header.directorysize * sizeof( CaptionLookup_t );
	m_CaptionDirectory.EnsureCapacity( m_Header.directorysize );
	dirbuffer.EnsureCapacity( directoryBytes );

	g_pFullFileSystem->Read( dirbuffer.Base(), directoryBytes, fh );
	g_pFullFileSystem->Close( fh );

	m_CaptionDirectory.CopyArray( (const CaptionLookup_t *)dirbuffer.PeekGet(), m_Header.directorysize );
	m_CaptionDirectory.RedoSort( true );

	m_DataBaseFile = pRelativePath;
	return true;
}
