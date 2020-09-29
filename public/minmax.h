//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#error use basetypes.h
#ifndef MINMAX_H
#define MINMAX_H

#ifndef V_min
#define V_min(a,b)  (((a) < (b)) ? (a) : (b))
#endif
#ifndef V_max
#define V_max(a,b)  (((a) > (b)) ? (a) : (b))
#endif

#endif // MINMAX_H
