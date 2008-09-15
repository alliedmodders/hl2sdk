//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include <stdio.h>
#include <string.h>
#include "tier0/dbg.h"

typedef struct
{
	const char		*string;
	int				code;
	float			weight;
} PHONEMEMAP;

static PHONEMEMAP g_Phonemes[] =
{
	{ "b", 'b', 0.8f },
	{ "m", 'm', 1.0f },
	{ "p", 'p', 0.8f },
	{ "w", 'w', 1.0f },
	{ "f", 'f', 0.8f },
	{ "v", 'v', 0.8f },
	{ "r", 0x0279, 1.0f },
	{ "r2", 'r', 1.0f },
	{ "r3", 0x027b, 1.0f },
	{ "er", 0x025a, 1.2f },
	{ "er2", 0x025d, 1.2f },
	{ "dh", 0x00f0, 1.0f },
	{ "th", 0x03b8, 1.0f },
	{ "sh", 0x0283, 1.0f },
	{ "jh", 0x02a4, 1.0f },
	{ "ch", 0x02a7, 1.0f },
	{ "s", 's', 0.8f },
	{ "z", 'z', 0.8f },
	{ "d", 'd', 0.8f },
	{ "d2", 0x027e, 0.8f },
	{ "l", 'l', 0.8f },
	{ "l2", 0x026b, 0.8f },
	{ "n", 'n', 0.8f },
	{ "t", 't', 0.8f },
	{ "ow", 'o', 1.2f },
	{ "uw", 'u', 1.2f },
	{ "ey", 'e', 1.0f },
	{ "ae", 0x00e6, 1.0f },
	{ "aa", 0x0251, 1.0f },
	{ "aa2", 'a', 1.0f },
	{ "iy", 'i', 1.0f },
	{ "y", 'j', 0.7f },
	{ "ah", 0x028c, 1.0f },
	{ "ao", 0x0254, 1.2f },
	{ "ax", 0x0259, 1.0f },
	{ "ax2", 0x025c, 1.0f },
	{ "eh", 0x025b, 1.0f },
	{ "ih", 0x026a, 1.0f },
	{ "ih2", 0x0268, 1.0f },
	{ "uh", 0x028a, 1.0f },
	{ "g", 'g', 0.8f },
	{ "g2", 0x0261, 1.0f },
	{ "hh", 'h', 0.8f },
	{ "hh2", 0x0266, 0.8f },
	{ "c", 'k', 0.6f },
	{ "nx", 0x014b, 1.0f },
	{ "zh", 0x0292, 1.0f },

	// Added
	{ "h", 'h', 0.8f },
	{ "k", 'k', 0.6f },
	{ "ay", 0x0251, 1.0f }, // or possibly +0x026a (ih)
	{ "ng", 0x014b, 1.0f }, // nx
	{ "aw", 0x0251, 1.2f }, // // vOWel,   // aa + uh???
	{ "oy", 'u', 1.2f },

	// Silence
	{ "<sil>", '_', 1.0f },

	// End of list
	{ NULL, NULL, 0.0f },
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : code - 
// Output : const char
//-----------------------------------------------------------------------------
const char *ConvertPhoneme( int code )
{
	int i = 0;
	while ( 1 )
	{
		PHONEMEMAP *test = &g_Phonemes[ i ];
		if ( !test->string )
			break;

		if ( test->code == code )
			return test->string;

		i++;
	}

	Warning( "Unrecognized phoneme code %i\n", code );
	return "<sil>";
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *text - 
// Output : int
//-----------------------------------------------------------------------------
int TextToPhoneme( const char *text )
{
	int i = 0;
	while ( 1 )
	{
		PHONEMEMAP *test = &g_Phonemes[ i ];
		if ( !test->string )
			break;

		if ( !stricmp( test->string, text ) )
			return test->code;

		i++;
	}

	Warning( "Unrecognized phoneme %s\n", text );
	return '_';
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : code - 
// Output : float
//-----------------------------------------------------------------------------
float WeightForPhonemeCode( int code )
{
	int i = 0;
	while ( 1 )
	{
		PHONEMEMAP *test = &g_Phonemes[ i ];
		if ( !test->string )
			break;

		if ( test->code == code )
			return test->weight;

		i++;
	}

	Warning( "Unrecognized phoneme code %i\n", code );
	return 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *text - 
// Output : float
//-----------------------------------------------------------------------------
float WeightForPhoneme( char *text )
{
	int i = 0;
	while ( 1 )
	{
		PHONEMEMAP *test = &g_Phonemes[ i ];
		if ( !test->string )
			break;

		if ( !stricmp( test->string, text ) )
			return test->weight;

		i++;
	}

	Warning( "WeightForPhoneme:: Unrecognized phoneme %s\n", text );
	return 1.0f;
}
