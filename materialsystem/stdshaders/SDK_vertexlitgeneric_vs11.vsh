vs.1.1

# STATIC:  "HALF_LAMBERT"			"0..1"		[PC]
# STATIC:  "DETAIL"					"1..1"
# STATIC:  "ENVMAP"					"0..1"
# STATIC:  "ENVMAPCAMERASPACE"		"0..0"
# STATIC:  "ENVMAPSPHERE"			"0..1"
# DYNAMIC: "DOWATERFOG"				"0..1"
# DYNAMIC: "LIGHT_COMBO"			"0..21"
# DYNAMIC: "SKINNING"				"0..1"		[XBOX]
# DYNAMIC: "NUM_BONES"				"0..3"		[PC]

# can't have envmapshere or envmapcameraspace without envmap
# SKIP: !$ENVMAP && ( $ENVMAPSPHERE || $ENVMAPCAMERASPACE )

# can't have both envmapsphere and envmapcameraspace
# SKIP: $ENVMAPSPHERE && $ENVMAPCAMERASPACE

# decal is by itself
# SKIP: $DECAL && ( $DETAIL || $ENVMAP || $ENVMAPCAMERASPACE || $ENVMAPSPHERE )

#include "SDK_VertexLitGeneric_inc.vsh"

&VertexLitGeneric( $DETAIL, $ENVMAP, $ENVMAPCAMERASPACE, $ENVMAPSPHERE, 0 );
