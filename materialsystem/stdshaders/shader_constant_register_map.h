//========= Copyright © 1996-2006, Valve LLC, All rights reserved. ============
//
// Purpose: Provide convenient mapping for shader constants
//
// $NoKeywords: $
//=============================================================================

#ifndef C_CODE_HACK
#include "common_vertexlitgeneric_dx9.h"
#endif

#define PSREG_SELFILLUMTINT					c0
#define PSREG_DIFFUSE_MODULATION			c1
#define PSREG_ENVMAP_CONTRAST				c2
#define PSREG_ENVMAP_SATURATION				c3
#define PSREG_AMBIENT_CUBE					c4
//		PSREG_AMBIENT_CUBE					c5
//		PSREG_AMBIENT_CUBE					c6
//		PSREG_AMBIENT_CUBE					c7
//		PSREG_AMBIENT_CUBE					c8
//		PSREG_AMBIENT_CUBE					c9
#define PSREG_WATER_FOG_COLOR				c10
#define PSREG_EYEPOS_SPEC_EXPONENT			c11
#define PSREG_FOG_PARAMS					c12
#define PSREG_FLASHLIGHT_ATTENUATION		c13
#define PSREG_FLASHLIGHT_POSITION			c14
#define PSREG_FLASHLIGHT_TO_WORLD_TEXTURE	c15
//		PSREG_FLASHLIGHT_TO_WORLD_TEXTURE	c16
//		PSREG_FLASHLIGHT_TO_WORLD_TEXTURE	c17
//		PSREG_FLASHLIGHT_TO_WORLD_TEXTURE	c18
#define PSREG_FRESNEL_SPEC_PARAMS			c19
#define PSREG_LIGHT_INFO_ARRAY				c20
//		PSREG_LIGHT_INFO_ARRAY				c21
//		PSREG_LIGHT_INFO_ARRAY				c22
//		PSREG_LIGHT_INFO_ARRAY				c23
//		PSREG_LIGHT_INFO_ARRAY				c24
//		PSREG_LIGHT_INFO_ARRAY				c25
//		** free **							c26
//		** free **							c27
#define	PSREG_FLASHLIGHT_COLOR				c28
#define	PSREG_GAMMA_FOG_COLOR				c29
#define	PSREG_GAMMA_LIGHT_SCALE				c30
#define	PSREG_LINEAR_LIGHT_SCALE			c31
//  --- End of ps_2_0 and ps_2_b constants ---

