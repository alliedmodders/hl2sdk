vs.1.1

# DYNAMIC: "DOWATERFOG"				"0..1"
# DYNAMIC: "DOFOG"					"0..1"

;------------------------------------------------------------------------------
; Constants specified by the app
;    c0      = (0, 1, 2, 0.5)
;	 c1		 = (1/2.2, 0, 0, 0)
;    c2      = camera position *in world space*
;    c4-c7   = modelViewProj matrix	(transpose)
;    c8-c11  = ViewProj matrix (transpose)
;    c12-c15 = model->view matrix (transpose)
;	 c16	 = [fogStart, fogEnd, fogRange, undefined]
;
; Vertex components (as specified in the vertex DECL)
;    $vPos    = Position
;	 $vTexCoord0.xy = TexCoord0
;------------------------------------------------------------------------------

#include "SDK_macros.vsh"

; Vertex components
;    $vPos		= Position
;	 $vNormal		= normal
;	 $vTexCoord0.xy	= TexCoord0
;	 $vTangentS		= S axis of Texture space
;	 $vTangentT	= T axis of Texture space


;------------------------------------------------------------------------------
; Transform the position from world to view space
;------------------------------------------------------------------------------

alloc $projPos

; Transform position from object to projection space
dp4 $projPos.x, $vPos, $cModelViewProj0
dp4 $projPos.y, $vPos, $cModelViewProj1
dp4 $projPos.z, $vPos, $cModelViewProj2
dp4 $projPos.w, $vPos, $cModelViewProj3
mov oPos, $projPos

alloc $worldPos
alloc $fog

; Transform position from object to world space
dp4 $worldPos.x, $vPos, $cModel0
dp4 $worldPos.y, $vPos, $cModel1
dp4 $worldPos.z, $vPos, $cModel2

if( $DOFOG == 0 )
{
	mov oT1, $cZero
}
else
{
	&CalcFog( $worldPos, $projPos, $fog );
	sub oT1, $cOne, $fog.x
}

mov oT1.xyz, $SHADER_SPECIFIC_CONST_3	; monochrome refracttint

; dudv map
alloc $bumpTexCoord

dp4 $bumpTexCoord.x, $vTexCoord0, $SHADER_SPECIFIC_CONST_1
dp4 $bumpTexCoord.y, $vTexCoord0, $SHADER_SPECIFIC_CONST_2

mov oT0.xy, $bumpTexCoord

free $fog
free $bumpTexCoord
free $projPos
free $worldPos
