#include "SDK_macros.vsh"

;------------------------------------------------------------------------------
;	 $SHADER_SPECIFIC_CONST_0-$SHADER_SPECIFIC_CONST_1 = Base texture transform
;    $SHADER_SPECIFIC_CONST_2-$SHADER_SPECIFIC_CONST_3 = Mask texture transform
;    $SHADER_SPECIFIC_CONST_4-$SHADER_SPECIFIC_CONST_5 = Detail texture transform
;------------------------------------------------------------------------------

sub UnlitGeneric
{
	local( $detail ) = shift;
	local( $envmap ) = shift;
	local( $envmapcameraspace ) = shift;
	local( $envmapsphere ) = shift;
	local( $vertexcolor ) = shift;

	local( $worldPos, $worldNormal, $projPos, $reflectionVector );

	;------------------------------------------------------------------------------
	; Vertex blending
	;------------------------------------------------------------------------------
	&AllocateRegister( \$worldPos );
	if( $envmap )
	{
		&AllocateRegister( \$worldNormal );
		&SkinPositionAndNormal( $worldPos, $worldNormal );
	}
	else
	{
		&SkinPosition( $worldPos );
	}

	;------------------------------------------------------------------------------
	; Transform the position from world to proj space
	;------------------------------------------------------------------------------

	&AllocateRegister( \$projPos );

	dp4 $projPos.x, $worldPos, $cViewProj0
	dp4 $projPos.y, $worldPos, $cViewProj1
	dp4 $projPos.z, $worldPos, $cViewProj2
	dp4 $projPos.w, $worldPos, $cViewProj3
	mov oPos, $projPos

	;------------------------------------------------------------------------------
	; Fog
	;------------------------------------------------------------------------------
	&CalcFog( $worldPos, $projPos );
	&FreeRegister( \$projPos );

	if( !$envmap )
	{
		&FreeRegister( \$worldPos );
	}


	
	;------------------------------------------------------------------------------
	; Texture coordinates (use world-space normal for envmap, tex transform for mask)
	;------------------------------------------------------------------------------
	dp4 oT0.x, $vTexCoord0, $SHADER_SPECIFIC_CONST_0
	dp4 oT0.y, $vTexCoord0, $SHADER_SPECIFIC_CONST_1

	if( $envmap )
	{
		if( $envmapcameraspace )
		{
			&AllocateRegister( \$reflectionVector );
			&ComputeReflectionVector( $worldPos, $worldNormal, $reflectionVector );

			; transform reflection vector into view space
			dp3 oT1.x, $reflectionVector, $cViewModel0
			dp3 oT1.y, $reflectionVector, $cViewModel1
			dp3 oT1.z, $reflectionVector, $cViewModel2

			&FreeRegister( \$reflectionVector );
		}
		elsif( $envmapsphere )
		{
			&AllocateRegister( \$reflectionVector );
			&ComputeReflectionVector( $worldPos, $worldNormal, $reflectionVector );
			&ComputeSphereMapTexCoords( $reflectionVector, "oT1" );

			&FreeRegister( \$reflectionVector );
		}
		else
		{
			&ComputeReflectionVector( $worldPos, $worldNormal, "oT1" );
		}

		; envmap mask
		dp4 oT2.x, $vTexCoord0, $SHADER_SPECIFIC_CONST_2
		dp4 oT2.y, $vTexCoord0, $SHADER_SPECIFIC_CONST_3

		&FreeRegister( \$worldPos );
		&FreeRegister( \$worldNormal );
	}

	if( $detail )
	{
		dp4 oT3.x, $vTexCoord0, $SHADER_SPECIFIC_CONST_4
		dp4 oT3.y, $vTexCoord0, $SHADER_SPECIFIC_CONST_5
	}

	if( $vertexcolor )
	{
		; Modulation color
		mul oD0, $vColor, $cModulationColor
	}
	else
	{
		; Modulation color
		mov oD0, $cModulationColor
	}
}
