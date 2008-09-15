//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PACIFIER_H
#define PACIFIER_H
#ifdef _WIN32
#pragma once
#endif


// Use these to display a pacifier like:
// ProcessBlock_Thread: 0...1...2...3...4...5...6...7...8...9... (0)
void StartPacifier( char const *pPrefix );
void UpdatePacifier( float flPercent );	// percent value between 0 and 1.
void EndPacifier( bool bCarriageReturn=true );


#endif // PACIFIER_H
