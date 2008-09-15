$dynamic_compile = defined $ENV{"dynamic_shaders"} && $ENV{"dynamic_shaders"} != 0;

sub ReadInputFile
{
	local( $filename ) = shift;
	local( *INPUT );
	local( @output );
	open INPUT, "<$filename" || die;

	local( $line );
	local( $linenum ) = 1;
	while( $line = <INPUT> )
	{
#		print "LINE: $line";
#		$line =~ s/\n//g;
#		local( $postfix ) = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
#		$postfix .= "; LINEINFO($filename)($linenum)\n";
		if( $line =~ m/\#include\s+\"(.*)\"/i )
		{
			push @output, &ReadInputFile( $1 );
		}
		else
		{
#			push @output, $line . $postfix;
			push @output, $line;
		}
		$linenum++;
	}

	close INPUT;
#	print "-----------------\n";
#	print @output;
#	print "-----------------\n";
	return @output;
}

$generateListingFile = 0;
$spewCombos = 0;

@startTimes = times;
$startTime = time;

$g_dx9 = 1;

while( 1 )
{
	$fxc_filename = shift;
#	if( $fxc_filename =~ m/-dx9/i )
#	{
#		$g_dx9 = 1;
#	}
	if( $fxc_filename =~ m/-source/ )
	{
		shift;
	}
	elsif( $fxc_filename =~ m/-nv3x/i )
	{
		$nvidia = 1;
	}
	elsif( $fxc_filename =~ m/-ps20a/i )
	{
		$ps2a = 1;
	}
	elsif( $fxc_filename =~ m/-xbox/i )
	{
		# enable xbox
		$g_xbox = 1;
	}
	elsif( $fxc_filename =~ m/-shaderoutputdir/i )
	{
		$shaderoutputdir = shift;
	}
	else
	{
		last;
	}
}


$debug = 0;
$forcehalf = 0;

sub ToUpper
{
	local( $in ) = shift;
	$in =~ tr/a-z/A-Z/;
	return $in;
}

sub CreateCCodeToSpewDynamicCombo
{
	local( $out ) = "";

	$out .= "\t\tOutputDebugString( \"$fxc_filename dynamic index\" );\n";
	$out .= "\t\tchar tmp[128];\n";
	$out .= "\t\tint shaderID = ";
	local( $scale ) = 1;
	for( $i = 0; $i < scalar( @dynamicDefineNames ); $i++ )
	{
		local( $name ) = @dynamicDefineNames[$i];
		local( $varname ) = "m_n" . $name;
		$out .= "( $scale * $varname ) + ";
		$scale *= $dynamicDefineMax[$i] - $dynamicDefineMin[$i] + 1;
	}
	$out .= "0;\n";
	if( scalar( @dynamicDefineNames ) + scalar( @staticDefineNames ) > 0 )
	{
		$out .= "\t\tint nCombo = shaderID;\n";
	}
	
	my $type = GetShaderType( $fxc_filename );
	for( $i = 0; $i < scalar( @dynamicDefineNames ); $i++ )
	{
		$out .= "\t\tint n$dynamicDefineNames[$i] = nCombo % ";
		$out .= ( $dynamicDefineMax[$i] - $dynamicDefineMin[$i] + 1 ) + $dynamicDefineMin[$i];
		$out .= ";\n";

		$out .= "\t\tsprintf( tmp, \"\%d\", n$dynamicDefineNames[$i] );\n";
		$out .= "\t\tOutputDebugString( \" $dynamicDefineNames[$i]";
		$out .= "=\" );\n";
		$out .= "\t\tOutputDebugString( tmp );\n";

		$out .= "\t\tnCombo = nCombo / " . ( $dynamicDefineMax[$i] - $dynamicDefineMin[$i] + 1 ) . ";\n";
		$out .= "\n";
	}
	$out .= "\t\tOutputDebugString( \"\\n\" );\n";
	return $out;
}

sub CreateCCodeToSpewStaticCombo
{
	local( $out ) = "";

	$out .= "\t\tOutputDebugString( \"$fxc_filename static index\" );\n";
	$out .= "\t\tchar tmp[128];\n";
	$out .= "\t\tint shaderID = ";

	local( $scale ) = 1;
	for( $i = 0; $i < scalar( @dynamicDefineNames ); $i++ )
	{
		$scale *= $dynamicDefineMax[$i] - $dynamicDefineMin[$i] + 1;
	}
	for( $i = 0; $i < scalar( @staticDefineNames ); $i++ )
	{
		local( $name ) = @staticDefineNames[$i];
		local( $varname ) = "m_n" . $name;
		$out .= "( $scale * $varname ) + ";
		$scale *= $staticDefineMax[$i] - $staticDefineMin[$i] + 1;
	}
	$out .= "0;\n";

#	$out .= "\t\tsprintf( tmp, \"\%d\\n\", shaderID );\n";
#	$out .= "\t\tOutputDebugString( tmp );\n\n";
	if( scalar( @staticDefineNames ) + scalar( @staticDefineNames ) > 0 )
	{
		$out .= "\t\tint nCombo = shaderID;\n";
	}
	
	my $type = GetShaderType( $fxc_filename );
	for( $i = 0; $i < scalar( @dynamicDefineNames ); $i++ )
	{
		$out .= "\t\tnCombo = nCombo / " . ( $dynamicDefineMax[$i] - $dynamicDefineMin[$i] + 1 ) . ";\n";
	}
	for( $i = 0; $i < scalar( @staticDefineNames ); $i++ )
	{
		$out .= "\t\tint n$staticDefineNames[$i] = nCombo % ";
		$out .= ( $staticDefineMax[$i] - $staticDefineMin[$i] + 1 ) + $staticDefineMin[$i];
		$out .= ";\n";

		$out .= "\t\tsprintf( tmp, \"\%d\", n$staticDefineNames[$i] );\n";
		$out .= "\t\tOutputDebugString( \" $staticDefineNames[$i]";
		$out .= "=\" );\n";
		$out .= "\t\tOutputDebugString( tmp );\n";

		$out .= "\t\tnCombo = nCombo / " . ( $staticDefineMax[$i] - $staticDefineMin[$i] + 1 ) . ";\n";
		$out .= "\n";
	}
	$out .= "\t\tOutputDebugString( \"\\n\" );\n";
	return $out;
}

sub WriteHelperVar
{
	local( $name ) = shift;
	local( $min ) = shift;
	local( $max ) = shift;
	local( $varname ) = "m_n" . $name;
	local( $boolname ) = "m_b" . $name;
	push @outputHeader, "private:\n";
	push @outputHeader, "\tint $varname;\n";
	push @outputHeader, "#ifdef _DEBUG\n";
	push @outputHeader, "\tbool $boolname;\n";
	push @outputHeader, "#endif\n";
	push @outputHeader, "public:\n";
	# int version of set function
	push @outputHeader, "\tvoid Set" . $name . "( int i )\n";
	push @outputHeader, "\t{\n";
	push @outputHeader, "\t\tAssert( i >= $min && i <= $max );\n";
	push @outputHeader, "\t\t$varname = i;\n";
	push @outputHeader, "#ifdef _DEBUG\n";
	push @outputHeader, "\t\t$boolname = true;\n";
	push @outputHeader, "#endif\n";
	push @outputHeader, "\t}\n";
	# bool version of set function
	push @outputHeader, "\tvoid Set" . $name . "( bool i )\n";
	push @outputHeader, "\t{\n";
#		push @outputHeader, "\t\tAssert( i >= $min && i <= $max );\n";
	push @outputHeader, "\t\t$varname = i ? 1 : 0;\n";
	push @outputHeader, "#ifdef _DEBUG\n";
	push @outputHeader, "\t\t$boolname = true;\n";
	push @outputHeader, "#endif\n";
	push @outputHeader, "\t}\n";
}

sub WriteStaticBoolExpression
{
	local( $prefix ) = shift;
	local( $operator ) = shift;
	for( $i = 0; $i < scalar( @staticDefineNames ); $i++ )
	{
		if( $i )
		{
			push @outputHeader, " $operator ";
		}
		local( $name ) = @staticDefineNames[$i];
		local( $boolname ) = "m_b" . $name;
		push @outputHeader, "$prefix$boolname";
	}
	push @outputHeader, ";\n";
}

sub WriteDynamicBoolExpression
{
	local( $prefix ) = shift;
	local( $operator ) = shift;
	for( $i = 0; $i < scalar( @dynamicDefineNames ); $i++ )
	{
		if( $i )
		{
			push @outputHeader, " $operator ";
		}
		local( $name ) = @dynamicDefineNames[$i];
		local( $boolname ) = "m_b" . $name;
		push @outputHeader, "$prefix$boolname";
	}
	push @outputHeader, ";\n";
}

sub WriteDynamicHelperClasses
{
	local( $basename ) = $fxc_filename;
	$basename =~ s/\.fxc//i;
	$basename =~ tr/A-Z/a-z/;
	local( $classname ) = $basename . "_Dynamic_Index";
	push @outputHeader, "class $classname\n";
	push @outputHeader, "{\n";
	for( $i = 0; $i < scalar( @dynamicDefineNames ); $i++ )
	{
		$name = $dynamicDefineNames[$i];
		$min = $dynamicDefineMin[$i];
		$max = $dynamicDefineMax[$i];
		&WriteHelperVar( $name, $min, $max );
	}
	push @outputHeader, "public:\n";
#	push @outputHeader, "void SetPixelShaderIndex( IShaderAPI *pShaderAPI ) { pShaderAPI->SetPixelShaderIndex( GetIndex() ); }\n";
	push @outputHeader, "\t$classname()\n";
	push @outputHeader, "\t{\n";
	for( $i = 0; $i < scalar( @dynamicDefineNames ); $i++ )
	{
		local( $name ) = @dynamicDefineNames[$i];
		local( $boolname ) = "m_b" . $name;
		local( $varname ) = "m_n" . $name;
		push @outputHeader, "#ifdef _DEBUG\n";
		push @outputHeader, "\t\t$boolname = false;\n";
		push @outputHeader, "#endif // _DEBUG\n";
		push @outputHeader, "\t\t$varname = 0;\n";
	}
	push @outputHeader, "\t}\n";
	push @outputHeader, "\tint GetIndex()\n";
	push @outputHeader, "\t{\n";
	push @outputHeader, "\t\t// Asserts to make sure that we aren't using any skipped combinations.\n";
	foreach $skip (@perlskipcodeindividual)
	{
		# can't do this static and dynamic can see each other.
#		$skip =~ s/\$/m_n/g;
#		$skip =~ s/defined//g;
#		push @outputHeader, "\t\tAssert( !( $skip ) );\n";
#		print "\t\tAssert( !( $skip ) );\n";
	}
	push @outputHeader, "\t\t// Asserts to make sure that we are setting all of the combination vars.\n";

	push @outputHeader, "#ifdef _DEBUG\n";
	if( scalar( @dynamicDefineNames ) > 0 )
	{
		push @outputHeader, "\t\tbool bAllDynamicVarsDefined = ";
		WriteDynamicBoolExpression( "", "&&" );
	}
	if( scalar( @dynamicDefineNames ) > 0 )
	{
		push @outputHeader, "\t\tAssert( bAllDynamicVarsDefined );\n";
	}
	push @outputHeader, "#endif // _DEBUG\n";

	if( $spewCombos && scalar( @dynamicDefineNames ) )
	{
		push @outputHeader, &CreateCCodeToSpewDynamicCombo();
	}
	push @outputHeader, "\t\treturn ";
	local( $scale ) = 1;
	for( $i = 0; $i < scalar( @dynamicDefineNames ); $i++ )
	{
		local( $name ) = @dynamicDefineNames[$i];
		local( $varname ) = "m_n" . $name;
		push @outputHeader, "( $scale * $varname ) + ";
		$scale *= $dynamicDefineMax[$i] - $dynamicDefineMin[$i] + 1;
	}
	push @outputHeader, "0;\n";
	push @outputHeader, "\t}\n";
	push @outputHeader, "};\n";
	push @outputHeader, "\#define shaderDynamicTest_" . $basename . " ";
	my $prefix;
	my $shaderType = &GetShaderType( $fxc_filename );
	if( $shaderType =~ m/^vs/i )
	{
		$prefix = "vsh_";
	}
	else
	{
		$prefix = "psh_";
	}
	for( $i = 0; $i < scalar( @dynamicDefineNames ); $i++ )
	{
		local( $name ) = @dynamicDefineNames[$i];
		push @outputHeader, $prefix . "forgot_to_set_dynamic_" . $name . " + ";
	}
	push @outputHeader, "0\n";
}

sub WriteStaticHelperClasses
{
	local( $basename ) = $fxc_filename;
	$basename =~ s/\.fxc//i;
	$basename =~ tr/A-Z/a-z/;
	local( $classname ) = $basename . "_Static_Index";
	push @outputHeader, "class $classname\n";
	push @outputHeader, "{\n";
	for( $i = 0; $i < scalar( @staticDefineNames ); $i++ )
	{
		$name = $staticDefineNames[$i];
		$min = $staticDefineMin[$i];
		$max = $staticDefineMax[$i];
		&WriteHelperVar( $name, $min, $max );
	}
	push @outputHeader, "public:\n";
#	push @outputHeader, "void SetShaderIndex( IShaderShadow *pShaderShadow ) { pShaderShadow->SetPixelShaderIndex( GetIndex() ); }\n";
	push @outputHeader, "\t$classname()\n";
	push @outputHeader, "\t{\n";
	for( $i = 0; $i < scalar( @staticDefineNames ); $i++ )
	{
		local( $name ) = @staticDefineNames[$i];
		local( $boolname ) = "m_b" . $name;
		local( $varname ) = "m_n" . $name;
		push @outputHeader, "#ifdef _DEBUG\n";
		push @outputHeader, "\t\t$boolname = false;\n";
		push @outputHeader, "#endif // _DEBUG\n";
		push @outputHeader, "\t\t$varname = 0;\n";
	}
	push @outputHeader, "\t}\n";
	push @outputHeader, "\tint GetIndex()\n";
	push @outputHeader, "\t{\n";
	push @outputHeader, "\t\t// Asserts to make sure that we aren't using any skipped combinations.\n";
	foreach $skip (@perlskipcodeindividual)
	{
		$skip =~ s/\$/m_n/g;
#		push @outputHeader, "\t\tAssert( !( $skip ) );\n";
	}
	push @outputHeader, "\t\t// Asserts to make sure that we are setting all of the combination vars.\n";

	push @outputHeader, "#ifdef _DEBUG\n";
	if( scalar( @staticDefineNames ) > 0 )
	{
		push @outputHeader, "\t\tbool bAllStaticVarsDefined = ";
		WriteStaticBoolExpression( "", "&&" );

	}
	if( scalar( @staticDefineNames ) > 0 )
	{
		push @outputHeader, "\t\tAssert( bAllStaticVarsDefined );\n";
	}
	push @outputHeader, "#endif // _DEBUG\n";

	if( $spewCombos && scalar( @staticDefineNames ) )
	{
		push @outputHeader, &CreateCCodeToSpewStaticCombo();
	}
	push @outputHeader, "\t\treturn ";
	local( $scale ) = 1;
	for( $i = 0; $i < scalar( @dynamicDefineNames ); $i++ )
	{
		$scale *= $dynamicDefineMax[$i] - $dynamicDefineMin[$i] + 1;
	}
	for( $i = 0; $i < scalar( @staticDefineNames ); $i++ )
	{
		local( $name ) = @staticDefineNames[$i];
		local( $varname ) = "m_n" . $name;
		push @outputHeader, "( $scale * $varname ) + ";
		$scale *= $staticDefineMax[$i] - $staticDefineMin[$i] + 1;
	}
	push @outputHeader, "0;\n";
	push @outputHeader, "\t}\n";
	push @outputHeader, "};\n";
	push @outputHeader, "\#define shaderStaticTest_" . $basename . " ";
	my $prefix;
	my $shaderType = &GetShaderType( $fxc_filename );
	if( $shaderType =~ m/^vs/i )
	{
		$prefix = "vsh_";
	}
	else
	{
		$prefix = "psh_";
	}
	for( $i = 0; $i < scalar( @staticDefineNames ); $i++ )
	{
		local( $name ) = @staticDefineNames[$i];
		push @outputHeader, $prefix . "forgot_to_set_static_" . $name . " + ";
	}
	push @outputHeader, "0\n";
}

sub BuildDefineOptions
{
	local( $output );
	local( $combo ) = shift;
	local( $i );
	for( $i = 0; $i < scalar( @dynamicDefineNames ); $i++ )
	{
		local( $val ) = ( $combo % ( $dynamicDefineMax[$i] - $dynamicDefineMin[$i] + 1 ) ) + $dynamicDefineMin[$i];
		$output .= "/D$dynamicDefineNames[$i]=$val ";
		$combo = $combo / ( $dynamicDefineMax[$i] - $dynamicDefineMin[$i] + 1 );
	}
	for( $i = 0; $i < scalar( @staticDefineNames ); $i++ )
	{
		local( $val ) = ( $combo % ( $staticDefineMax[$i] - $staticDefineMin[$i] + 1 ) ) + $staticDefineMin[$i];
		$output .= "/D$staticDefineNames[$i]=$val ";
		$combo = $combo / ( $staticDefineMax[$i] - $staticDefineMin[$i] + 1 );
	}
	return $output;
}

sub CreateFuncToSetPerlVars
{
	local( $out ) = "";

	$out .= "sub SetPerlVarsFunc\n";
	$out .= "{\n";
	$out .= "	local( \$combo ) = shift;\n";
	$out .= "	local( \$i );\n";
	local( $i );
	for( $i = 0; $i < scalar( @dynamicDefineNames ); \$i++ )
	{
		$out .= "	\$$dynamicDefineNames[$i] = \$combo % ";
		$out .= ( $dynamicDefineMax[$i] - $dynamicDefineMin[$i] + 1 ) + $dynamicDefineMin[$i];
		$out .= ";\n";
		$out .= "	\$combo = \$combo / " . ( $dynamicDefineMax[$i] - $dynamicDefineMin[$i] + 1 ) . ";\n";
	}
	for( $i = 0; $i < scalar( @staticDefineNames ); \$i++ )
	{
		$out .= "	\$$staticDefineNames[$i] = \$combo % ";
		$out .= ( $staticDefineMax[$i] - $staticDefineMin[$i] + 1 ) + $staticDefineMin[$i];
		$out .= ";\n";
		$out .= "	\$combo = \$combo / " . ( $staticDefineMax[$i] - $staticDefineMin[$i] + 1 ) . ";\n";
	}
	$out .= "}\n";

#	print $out;
	eval $out;
}

sub GetNewMainName
{
	local( $shadername ) = shift;
	local( $combo ) = shift;
	local( $i );
	$shadername =~ s/\./_/g;
	local( $name ) = $shadername;
	for( $i = 0; $i < scalar( @defineNames ); $i++ )
	{
		local( $val ) = ( $combo % ( $defineMax[$i] - $defineMin[$i] + 1 ) ) + $defineMin[$i];
		$name .= "_" . $defineNames[$i] . "_" . $val;
		$combo = $combo / ( $defineMax[$i] - $defineMin[$i] + 1 );
	}
#	return $name;
	return "main";
}

sub RenameMain
{
	local( $shadername ) = shift;
	local( $combo ) = shift;
	local( $name ) = &GetNewMainName( $shadername, $combo );
	return "/Dmain=$name /E$name ";
}

sub GetShaderType
{
	local( $shadername ) = shift;
	if( $shadername =~ m/ps30/i )
	{
		if( $debug )
		{
			return "ps_3_sw";
		}
		else
		{
			return "ps_3_0";
		}
	}
	elsif( $shadername =~ m/ps20b/i )
	{
		return "ps_2_b";
	}
	elsif( $shadername =~ m/ps20/i )
	{
		if( $debug )
		{
			return "ps_2_sw";
		}
		else
		{
			if( $ps2a )
			{
				return "ps_2_a";
			}
			else
			{
				return "ps_2_0";
			}
		}
	}
	elsif( $shadername =~ m/ps14/i )
	{
		return "ps_1_4";
	}
	elsif( $shadername =~ m/ps11/i )
	{
		return "ps_1_1";
	}
	elsif( $shadername =~ m/vs30/i )
	{
		if( $debug )
		{
			return "vs_3_sw";
		}
		else
		{
			return "vs_3_0";
		}
	}
	elsif( $shadername =~ m/vs20/i )
	{
		if( $debug )
		{
			return "vs_2_sw";
		}
		else
		{
			return "vs_2_0";
		}
	}
	elsif( $shadername =~ m/vs14/i )
	{
		return "vs_1_1";
	}
	elsif( $shadername =~ m/vs11/i )
	{
		return "vs_1_1";
	}
	else
	{
		die "\n\nSHADERNAME = $shadername\n\n";
	}
}

sub CalcNumCombos
{
	local( $i, $numCombos );
	$numCombos = 1;
	for( $i = 0; $i < scalar( @dynamicDefineNames ); $i++ )
	{
		$numCombos *= $dynamicDefineMax[$i] - $dynamicDefineMin[$i] + 1;
	}
	for( $i = 0; $i < scalar( @staticDefineNames ); $i++ )
	{
		$numCombos *= $staticDefineMax[$i] - $staticDefineMin[$i] + 1;
	}
	return $numCombos;
}

sub CalcNumDynamicCombos
{
	local( $i, $numCombos );
	$numCombos = 1;
	for( $i = 0; $i < scalar( @dynamicDefineNames ); $i++ )
	{
		$numCombos *= $dynamicDefineMax[$i] - $dynamicDefineMin[$i] + 1;
	}
	return $numCombos;
}

sub CreateCFuncToCreateCompileCommandLine
{
	local( $out ) = "";

	$out .= "\t\tOutputDebugString( \"compiling $fxc_filename \" );\n";
	$out .= "\t\tchar tmp[128];\n";
	$out .= "\t\tsprintf( tmp, \"\%d\\n\", shaderID );\n";
	$out .= "\t\tOutputDebugString( tmp );\n";
	$out .= "\t\tstatic PrecompiledShaderByteCode_t byteCode;\n";
	if( scalar( @dynamicDefineNames ) + scalar( @staticDefineNames ) > 0 )
	{
		$out .= "\t\tint nCombo = shaderID;\n";
	}
	
#	$out .= "\tvoid BuildCompileCommandLine( int nCombo, char *pResult, int maxLength )\n";
#	$out .= "\t{\n";
	$out .= "\t\tD3DXMACRO ";
	$out .= "defineMacros";
	$out .= "[";
	$out .= scalar( @dynamicDefineNames ) + scalar( @staticDefineNames ) + 1; # add 1 for null termination
	$out .= "];\n";
	if( scalar( @dynamicDefineNames ) + scalar( @staticDefineNames ) > 0 )
	{
		$out .= "\t\tchar tmpStringBuf[1024];\n";
		$out .= "\t\tchar *pTmpString = tmpStringBuf;\n\n";
	}

	local( $i );
	my $type = GetShaderType( $fxc_filename );
	for( $i = 0; $i < scalar( @dynamicDefineNames ); $i++ )
	{
		$out .= "\t\tsprintf( pTmpString, \"%d\", nCombo % ";
		$out .= ( $dynamicDefineMax[$i] - $dynamicDefineMin[$i] + 1 ) + $dynamicDefineMin[$i];
		$out .= " );\n";
		$out .= "\t\tdefineMacros";
		$out .= "[";
		$out .= $i;
		$out .= "]";
		$out .= "\.Name = ";
		$out .= "\"$dynamicDefineNames[$i]\";\n";

		$out .= "\t\tint n$dynamicDefineNames[$i] = nCombo % ";
		$out .= ( $dynamicDefineMax[$i] - $dynamicDefineMin[$i] + 1 ) + $dynamicDefineMin[$i];
		$out .= ";\n";
		$out .= "\t\tUNUSED( n$dynamicDefineNames[$i] );\n";

		$out .= "\t\tdefineMacros";
		$out .= "[";
		$out .= $i;
		$out .= "]";
		$out .= "\.Definition = ";
		$out .= "pTmpString;\n";
		$out .= "\t\tpTmpString += strlen( pTmpString ) + 1;\n";

		$out .= "\t\tsprintf( tmp, \"\%d\", n$dynamicDefineNames[$i] );\n";
		$out .= "\t\tOutputDebugString( \" $dynamicDefineNames[$i]";
		$out .= "=\" );\n";
		$out .= "\t\tOutputDebugString( tmp );\n";

		$out .= "\t\tnCombo = nCombo / " . ( $dynamicDefineMax[$i] - $dynamicDefineMin[$i] + 1 ) . ";\n";
		$out .= "\n";
	}
	for( $i = 0; $i < scalar( @staticDefineNames ); $i++ )
	{
		$out .= "\t\tsprintf( pTmpString, \"%d\", nCombo % ";
		$out .= ( $staticDefineMax[$i] - $staticDefineMin[$i] + 1 ) + $staticDefineMin[$i];
		$out .= " );\n";
		$out .= "\t\tdefineMacros";
		$out .= "[";
		$out .= $i + scalar( @dynamicDefineNames );
		$out .= "]";
		$out .= "\.Name = ";
		$out .= "\"$staticDefineNames[$i]\";\n";

		$out .= "\t\tint n$staticDefineNames[$i] = nCombo % ";
		$out .= ( $staticDefineMax[$i] - $staticDefineMin[$i] + 1 ) + $staticDefineMin[$i];
		$out .= ";\n";
		$out .= "\t\tUNUSED( n$staticDefineNames[$i] );\n";

		$out .= "\t\tdefineMacros";
		$out .= "[";
		$out .= $i + scalar( @dynamicDefineNames );
		$out .= "]";
		$out .= "\.Definition = ";
		$out .= "pTmpString;\n";
		$out .= "\t\tpTmpString += strlen( pTmpString ) + 1;\n";

		$out .= "\t\tsprintf( tmp, \"\%d\", n$staticDefineNames[$i] );\n";
		$out .= "\t\tOutputDebugString( \" $staticDefineNames[$i]";
		$out .= "=\" );\n";
		$out .= "\t\tOutputDebugString( tmp );\n";

		$out .= "\t\tnCombo = nCombo / " . ( $staticDefineMax[$i] - $staticDefineMin[$i] + 1 ) . ";\n";
		$out .= "\n";
	}

	$out .= "\t\tOutputDebugString( \"\\n\" );\n";

	$cskipcode = $perlskipcode;
	$cskipcode =~ s/\$/n/g;
	$out .= "\t\tif( $cskipcode )\n\t\t{\n";
	$out .= "\t\t\tstatic char blah[4] = { 0, 0, 0, 0 };\n";
	$out .= "\t\t\tbyteCode.m_pRawData = blah;\n";
	$out .= "\t\t\tbyteCode.m_nSizeInBytes = 4;\n";
	$out .= "\t\t\treturn byteCode;\n";
	$out .= "\t\t}\n";

	

	$out .= "\t\t// Must null terminate macros.\n";
	$out .= "\t\tdefineMacros[";
	$out .= scalar( @dynamicDefineNames ) + scalar( @staticDefineNames );
	$out .= "]";
	$out .= ".Name = NULL;\n";
	$out .= "\t\tdefineMacros[";
	$out .= scalar( @dynamicDefineNames ) + scalar( @staticDefineNames );
	$out .= "]";
	$out .= ".Definition = NULL;\n\n";


	$out .= "\t\tLPD3DXBUFFER pShader; // NOTE: THESE LEAK!!!\n";
	$out .= "\t\tLPD3DXBUFFER pErrorMessages; // NOTE: THESE LEAK!!!\n";
	$out .= "\t\tHRESULT hr = D3DXCompileShaderFromFile( \"u:\\\\hl2_e3_2004\\\\src_e3_2004\\\\materialsystem\\\\stdshaders\\\\$fxc_filename\",\n\t\t\tdefineMacros,\n\t\t\tNULL, // LPD3DXINCLUDE \n\t\t\t\"main\",\n\t\t\t\"$type\",\n\t\t\t0, // DWORD Flags\n\t\t\t&pShader,\n\t\t\t&pErrorMessages,\n\t\t\tNULL // LPD3DXCONSTANTTABLE *ppConstantTable\n\t\t\t );\n";
	$out .= "\t\tif( hr != D3D_OK )\n";
	$out .= "\t\t{\n";
	$out .= "\t\t\tconst char *pErrorMessageString = ( const char * )pErrorMessages->GetBufferPointer();\n";
	$out .= "\t\t\tOutputDebugString( pErrorMessageString );\n";
	$out .= "\t\t\tOutputDebugString( \"\\n\" );\n";
	$out .= "\t\t\tAssert( 0 );\n";
	$out .= "\t\t\tstatic char blah[4] = { 0, 0, 0, 0 };\n";
	$out .= "\t\t\tbyteCode.m_pRawData = blah;\n";
	$out .= "\t\t\tbyteCode.m_nSizeInBytes = 4;\n";
	$out .= "\t\t}\n";
	$out .= "\t\telse\n";
	$out .= "\t\t{\n";
	$out .= "\t\t\tbyteCode.m_pRawData = pShader->GetBufferPointer();\n";
	$out .= "\t\t\tbyteCode.m_nSizeInBytes = pShader->GetBufferSize();\n";
	$out .= "\t\t}\n";
	$out .= "\t\treturn byteCode;\n";
	return $out;
}

#print "--------\n";

if ( $g_xbox )
{
	$fxctmp = "fxctmp_xbox";
}
elsif( $g_dx9 )
{
	if( $nvidia )
	{
		if( $ps2a )
		{
			$fxctmp = "fxctmp9_nv3x";
		}
		else
		{
			$fxctmp = "fxctmp9_nv3x_ps20";
		}
	}
	else
	{
		$fxctmp = "fxctmp9";
	}
}
else
{
	$fxctmp = "fxctmp8";
}

if( !stat $fxctmp )
{
	mkdir $fxctmp, 0777 || die $!;
}

# suck in an input file (using includes)
print "$fxc_filename...";
@fxc = ReadInputFile( $fxc_filename );

# READ THE TOP OF THE FILE TO FIND SHADER COMBOS
foreach $line ( @fxc )
{
	next if( $line =~ m/^\s*$/ );
	if( $line =~ m/^\s*\/\/\s*STATIC\s*\:\s*\"(.*)\"\s+\"(\d+)\.\.(\d+)\"/ )
	{
		local( $name, $min, $max );
		$name = $1;
		$min = $2;
		$max = $3;
#		print "\"$name\" \"$min..$max\"\n";
		push @staticDefineNames, $name;
		push @staticDefineMin, $min;
		push @staticDefineMax, $max;
	}
	elsif( $line =~ m/^\s*\/\/\s*DYNAMIC\s*\:\s*\"(.*)\"\s+\"(\d+)\.\.(\d+)\"/ )
	{
		local( $name, $min, $max );
		$name = $1;
		$min = $2;
		$max = $3;
#		print "\"$name\" \"$min..$max\"\n";
		push @dynamicDefineNames, $name;
		push @dynamicDefineMin, $min;
		push @dynamicDefineMax, $max;
	}
}
# READ THE WHOLE FILE AND FIND SKIP STATEMENTS
foreach $line ( @fxc )
{
	if( $line =~ m/^\s*\/\/\s*SKIP\s*\:\s*(.*)$/ )
	{
#		print $1 . "\n";
		$perlskipcode .= "(" . $1 . ")||";
		push @perlskipcodeindividual, $1;
	}
}

if( defined $perlskipcode )
{
	$perlskipcode .= "0";
	$perlskipcode =~ s/\n//g;
}
else
{
	$perlskipcode = "0";
}

# READ THE WHOLE FILE AND FIND CENTROID STATEMENTS
foreach $line ( @fxc )
{
	if( $line =~ m/^\s*\/\/\s*CENTROID\s*\:\s*TEXCOORD(\d+)\s*$/ )
	{
		$centroidEnable{$1} = 1;
#		print "CENTROID: $1\n";
	}
}

if( $spewCombos )
{
	push @outputHeader, "#include \"windows.h\"\n";
}

#push @outputHeader, "\#include \"shaderlib\\baseshader.h\"\n";
#push @outputHeader, "IShaderDynamicAPI *CBaseShader::s_pShaderAPI;\n";

# Go ahead an compute the mask of samplers that need to be centroid sampled
$centroidMask = 0;
foreach $centroidRegNum ( keys( %centroidEnable ) )
{
#	print "THING: $samplerName $centroidRegNum\n";
	$centroidMask += 1 << $centroidRegNum;
}

#printf "0x%x\n", $centroidMask;

$numCombos = &CalcNumCombos();
#print "$numCombos combos\n";

# Write out the C++ helper class for picking shader combos
&WriteStaticHelperClasses();
&WriteDynamicHelperClasses();

# Create a subroutine out of $perlskipcode
$perlskipfunc = "sub SkipCombo { return $perlskipcode; }\n";
#print $perlskipfunc;

eval $perlskipfunc;
&CreateFuncToSetPerlVars();

if( !$dynamic_compile )
{
	open OUTFILELIST, ">>filelist.txt" || die "can't open filelist.txt";
	for( $i = 0; $i < $numCombos; $i++ )
	{
		&SetPerlVarsFunc( $i );

		local( $compileFailed );
	#	$ret = eval $perlskipcode;
		$ret = &SkipCombo;
		if( !defined $ret )
		{
			die "$@\n";
		}
		if( $ret )
		{
			# skip this combo!
	#			print OUTFILELIST "$i/$numCombos: SKIP\n";
			$compileFailed = 1;
			$numSkipped++;
		}
		else
		{
			local( $cmd );
			if( $g_xbox )
			{
				$cmd = "perl fxc_xbox.pl ";
			}
			else
			{
				$cmd = "fxc.exe ";
			}
			$cmd .= "/DSHADERCOMBO=$i ";
			$cmd .= "/DTOTALSHADERCOMBOS=$numCombos ";
			$cmd .= "/DCENTROIDMASK=$centroidMask ";
			$cmd .= "/DNUMDYNAMICCOMBOS=" . &CalcNumDynamicCombos() . " ";
			$cmd .= "/DFLAGS=0x0 "; # Nothing here for now.
			if( $g_xbox )
			{
				$cmd .= "/D_XBOX=1 ";
			}
			$cmd .= &BuildDefineOptions( $i );
			$cmd .= &RenameMain( $fxc_filename, $i );
			$cmd .= "/T" . &GetShaderType( $fxc_filename ) . " ";
			$cmd .= "/DSHADER_MODEL_" . &ToUpper( &GetShaderType( $fxc_filename ) ) . "=1 ";
			if( $nvidia )
			{
				$cmd .= "/DNV3X=1 "; # enable NV3X codepath
			}
			if( $debug )
			{
				$cmd .= "/Od "; # disable optimizations
				$cmd .= "/Zi "; # enable debug info
			}
	#			$cmd .= "/Zi "; # enable debug info
			$cmd .= "/nologo ";
			if( $fxc_filename =~ /ps20/i && $forcehalf )
			{
				$cmd .= "/Gpp "; # use half everywhere
			}
			else
			{
				if( $g_xbox )
				{
					$cmd .= "/Fcshader.asm ";
				}
				else
				{
	#					$cmd .= "/Fhtmpshader.h ";
					$cmd .= "/Foshader.o ";
				}
			}
			$cmd .= "$fxc_filename";
			$cmd .= ">output.txt 2>&1";
	#			print $i . "/" . $numCombos . " " . $cmd . "\n";
			print OUTFILELIST $cmd . "\n";
		}
	}
	close OUTFILELIST;
}

#print "$numSkipped/$numCombos skipped\n";

local( $name ) = $fxc_filename;
$name =~ s/\.fxc//gi;

#printf "writing $fxctmp/$name" . ".inc\n";

# open the existing inc file so that we can see if anything changed
local( *OLDINCFILE );
my $incfilechanged = 1;
# if( open OLDINCFILE, "<$fxctmp/$name" . ".inc" )
# {
# #	print "opened old inc file $fxctmp/$name" . ".inc\n";

# 	# The inc file already exists. . let's see if it's the same as before.
# 	my @oldincfile;
# 	@oldincfile = <OLDINCFILE>;
# 	close OLDINCFILE;
	
# 	if( join( "", @oldincfile ) eq join( "", @outputHeader ) )
# 	{
# #		print "They are the same!\n";
# 		$incfilechanged = 0;
# 	}
# }

if( $incfilechanged )
{
	print "writing inc\n";
	local( *FILE );
	if( !open FILE, ">$fxctmp/$name" . ".inc" )
	{
		die;
	}
	print FILE @outputHeader;
	close FILE;
	undef @outputHeader;
}
else
{
	print "no inc diffs\n";
}

if( $generateListingFile )
{
	my $listFileName = "$fxctmp/$name" . ".lst";
	print "writing $listFileName\n";
	if( !open FILE, ">$listFileName" )
	{
		die;
	}
	print FILE @listingOutput;
	close FILE;
}


@endTimes = times;

$endTime = time;

#printf "Elapsed user time: %.2f seconds!\n", $endTimes[0] - $startTimes[0];
#printf "Elapsed system time: %.2f seconds!\n", $endTimes[1] - $startTimes[1];
#printf "Elapsed child user time: %.2f seconds!\n", $endTimes[2] - $startTimes[2];
#printf "Elapsed child system time: %.2f seconds!\n", $endTimes[3] - $startTimes[3];

#printf "Elapsed total time: %.2f seconds!\n", $endTime - $startTime;

