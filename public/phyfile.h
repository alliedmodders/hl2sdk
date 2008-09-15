//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PHYFILE_H
#define PHYFILE_H
#pragma once

typedef struct phyheader_s
{
	int		size;
	int		id;
	int		solidCount;
	long	checkSum;	// checksum of source .mdl file
} phyheader_t;

#endif // PHYFILE_H
