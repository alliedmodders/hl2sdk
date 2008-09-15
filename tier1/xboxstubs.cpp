//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Stubs Xbox functions when compiling for other platforms
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#ifndef _XBOX

#ifdef _WIN32
#include <windows.h>
#elif _LINUX
#define ERROR_GEN_FAILURE 1
#define GetTickCount() Plat_FloatTime()
#else
#error "implement me"
#endif
#include "xbox/xboxstubs.h"
#include "tier0/memdbgon.h"

void XBX_DebugString(xverbose_e verbose, COLORREF color, const char* format, ...)
{
}

void XBX_ProcessEvents( void )
{
}

void XInitDevices( DWORD dwPreallocTypeCount, PXDEVICE_PREALLOC_TYPE PreallocTypes )
{
}

DWORD XGetDevices(PXPP_DEVICE_TYPE DeviceType)
{
	return 0;
}

bool XGetDeviceChanges(PXPP_DEVICE_TYPE DeviceType, DWORD *pdwInsertions, DWORD *pdwRemovals)
{
	return false;
}

HANDLE XInputOpen(PXPP_DEVICE_TYPE DeviceType, DWORD dwPort, DWORD dwSlot, PXINPUT_POLLING_PARAMETERS pPollingParameters)
{
	return 0;
}

void XInputClose(HANDLE hDevice)
{
}

DWORD XInputSetState(HANDLE hDevice, PXINPUT_FEEDBACK pFeedback)
{
	return ERROR_GEN_FAILURE;
}

DWORD XInputGetState(HANDLE hDevice, PXINPUT_STATE  pState)
{
	return ERROR_GEN_FAILURE;
}

DWORD XInputPoll(HANDLE hDevice)
{
	return 0;
}

unsigned int XBX_GetSystemTime( void )
{
	return (unsigned int)GetTickCount();
}

#endif // _XBOX
