//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef ZIP_UNCOMPRESSED_H
#define ZIP_UNCOMPRESSED_H
#ifdef _WIN32
#pragma once
#endif

#pragma pack(1)
struct ZIP_EndOfCentralDirRecord
{
	unsigned int signature; // 4 bytes (0x06054b50)
	unsigned short numberOfThisDisk;  // 2 bytes
	unsigned short numberOfTheDiskWithStartOfCentralDirectory; // 2 bytes
	unsigned short nCentralDirectoryEntries_ThisDisk;	// 2 bytes
	unsigned short nCentralDirectoryEntries_Total;	// 2 bytes
	unsigned int centralDirectorySize; // 4 bytes
	unsigned int startOfCentralDirOffset; // 4 bytes
	unsigned short commentLength; // 2 bytes
	// zip file comment follows
};

struct ZIP_FileHeader
{
	unsigned int signature; //  4 bytes (0x02014b50) 
	unsigned short versionMadeBy; // version made by 2 bytes 
	unsigned short versionNeededToExtract; // version needed to extract 2 bytes 
	unsigned short flags; // general purpose bit flag 2 bytes 
	unsigned short compressionMethod; // compression method 2 bytes 
	unsigned short lastModifiedTime; // last mod file time 2 bytes 
	unsigned short lastModifiedDate; // last mod file date 2 bytes 
	unsigned int crc32; // crc-32 4 bytes 
	unsigned int compressedSize; // compressed size 4 bytes 
	unsigned int uncompressedSize; // uncompressed size 4 bytes 
	unsigned short fileNameLength; // file name length 2 bytes 
	unsigned short extraFieldLength; // extra field length 2 bytes 
	unsigned short fileCommentLength; // file comment length 2 bytes 
	unsigned short diskNumberStart; // disk number start 2 bytes 
	unsigned short internalFileAttribs; // internal file attributes 2 bytes 
	unsigned int externalFileAttribs; // external file attributes 4 bytes 
	unsigned int relativeOffsetOfLocalHeader; // relative offset of local header 4 bytes 
	// file name (variable size) 
	// extra field (variable size) 
	// file comment (variable size) 
};

struct ZIP_LocalFileHeader
{
	unsigned int signature; //local file header signature 4 bytes (0x04034b50) 
	unsigned short versionNeededToExtract; // version needed to extract 2 bytes 
	unsigned short flags; // general purpose bit flag 2 bytes 
	unsigned short compressionMethod; // compression method 2 bytes 
	unsigned short lastModifiedTime; // last mod file time 2 bytes 
	unsigned short lastModifiedDate; // last mod file date 2 bytes 
	unsigned int crc32; // crc-32 4 bytes 
	unsigned int compressedSize; // compressed size 4 bytes 
	unsigned int uncompressedSize; // uncompressed size 4 bytes 
	unsigned short fileNameLength; // file name length 2 bytes 
	unsigned short extraFieldLength; // extra field length 2 bytes 
	// file name (variable size) 
	// extra field (variable size) 
};
#pragma pack()

#endif // ZIP_UNCOMPRESSED_H
