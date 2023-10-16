//========= Copyright � 1996-2008, Valve Corporation, All rights reserved. ====
//
// Entity definitions needed by hammer, compile tools, and the game.
//
//=============================================================================

#ifndef ENTITYDEFS_H
#define ENTITYDEFS_H
#ifdef _WIN32
#pragma once
#endif


#define MAX_ENTITY_NAME_LEN			   256
#define MAX_IO_NAME_LEN				      256

#define VMF_IOPARAM_STRING_DELIMITER   0x1b // Use ESC as a delimiter so we can pass commas etc. in I/O parameters


#endif // ENTITYDEFS_H

