//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PHONEMECONVERTER_H
#define PHONEMECONVERTER_H
#ifdef _WIN32
#pragma once
#endif

const char *ConvertPhoneme( int code );
int TextToPhoneme( const char *text );
float WeightForPhonemeCode( int code );
float WeightForPhoneme( char *text );

#endif // PHONEMECONVERTER_H
