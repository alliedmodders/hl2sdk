$dynamic_compile = defined $ENV{"dynamic_shaders"} && $ENV{"dynamic_shaders"} != 0;

$depnum = 0;

$shaderoutputdir = "shaders";
$baseSourceDir = ".";

sub GetAsmShaderDependencies_R
{
	local( $shadername ) = shift;
	local( *SHADER );
	
	open SHADER, "<$shadername";
	while( <SHADER> )
	{
		if( m/^\s*\#\s*include\s+\"(.*)\"/ )
		{
#			print "$shadername depends on $1\n";
#			if( !defined( $dep{$shadername} ) )
			{
				if ( -e "$g_SourceDir\\materialsystem\\stdshaders\\$1" )
				{
					$dep{$shadername} = "$g_SourceDir\\materialsystem\\stdshaders\\$1";
				}
				else
				{
					$dep{$shadername} = ".\\$1";
				}
				GetAsmShaderDependencies_R( $dep{$shadername} );
			}
#			else
#			{
#				print "circular dependency in $shadername!\n";
#			}
		}
	}
	close SHADER;
}

sub GetAsmShaderDependencies
{
	local( $shadername ) = shift;
	undef %dep;
	GetAsmShaderDependencies_R( $shadername );
	local( $i );
	foreach $i ( keys( %dep ) )
	{
#		print "$i depends on $dep{$i}\n";
	}
	return values( %dep );
}

sub GetShaderType
{
	my $shadername = shift;
	my $shadertype;
	if( $shadername =~ m/\.vsh/i )
	{
		$shadertype = "vsh";
	}
	elsif( $shadername =~ m/\.psh/i )
	{
		$shadertype = "psh";
	}
	elsif( $shadername =~ m/\.fxc/i )
	{
		$shadertype = "fxc";
	}
	else
	{
		die;
	}
	return $shadertype;
}

sub GetShaderBase
{
	my $shadername = shift;
	my $shadertype = &GetShaderType( $shadername );
	$shadername =~ s/\.$shadertype//i;
	return $shadername;
}

sub DoAsmShader
{
	local( $shadername ) = shift;
	local( $shadertype );
	$shadertype = &GetShaderType( $shadername );
	@dep = &GetAsmShaderDependencies( $shadername );
	my $shaderbase = &GetShaderBase( $shadername );
	my $incfile = "";
	if( $shadertype eq "fxc" || $shadertype eq "vsh" )
	{
		if( $g_xbox )
		{
			$incfile = $shadertype . "tmp_xbox\\$shaderbase.inc ";
		}
		else
		{
			$incfile = $shadertype . "tmp9\\$shaderbase.inc ";
		}
	}
	if( $dynamic_compile && $shadertype eq "fxc" )
	{
		print MAKEFILE $incfile . ": ..\\..\\devtools\\bin\\updateshaders.pl ..\\..\\devtools\\bin\\" . $shadertype . "_prep.pl $shadername @dep\n";
	}
	else
	{
		print MAKEFILE $incfile . "$shaderoutputdir\\$shadertype\\$shaderbase.vcs: ..\\..\\devtools\\bin\\updateshaders.pl ..\\..\\devtools\\bin\\" . $shadertype . "_prep.pl $shadername @dep\n";
	}
	

	my $xboxswitch = "";
	if( $g_xbox )
	{
		$xboxswitch = "-xbox ";
	}
	print MAKEFILE "\tperl $g_SourceDir\\devtools\\bin\\" . $shadertype . "_prep.pl $xboxswitch -shaderoutputdir $shaderoutputdir -source \"$g_SourceDir\" $shadername\n";
	my $filename;
	if( $shadertype eq "fxc" && !$dynamic_compile )
	{
		print MAKEFILE "\techo $shadername>> filestocopy.txt\n";
		my $dep;
		foreach $dep( @dep )
		{
			print MAKEFILE "\techo $dep>> filestocopy.txt\n";
		}
	}
	print MAKEFILE "\n";
}

sub MakeSureFileExists
{
	local( $filename ) = shift;
	local( $testexists ) = shift;
	local( $testwrite ) = shift;

	local( @statresult ) = stat $filename;
	if( !@statresult && $testexists )
	{
		die "$filename doesn't exist!\n";
	}
	local( $mode, $iswritable );
	$mode = oct( $statresult[2] );
	$iswritable = ( $mode & 2 ) != 0;
	if( !$iswritable && $testwrite )
	{
		die "$filename isn't writable!\n";
	}
}

if( scalar( @ARGV ) == 0 )
{
	die "Usage updateshaders.pl shaderprojectbasename\n\tie: updateshaders.pl stdshaders_dx6\n";
}

while( 1 )
{
	$inputbase = shift;

	if( $inputbase =~ m/-source/ )
	{
		$g_SourceDir = shift;
	}
	elsif( $inputbase =~ m/-xbox/ )
	{
		$g_xbox = 1;
	}
	elsif( $inputbase =~ m/-nv3x/ )
	{
		$nv3x = 1;
	}
	elsif( $inputbase =~ m/-shaderoutputdir/i )
	{
		$shaderoutputdir = shift;
	}
	else
	{
		last;
	}
}

&MakeSureFileExists( "$inputbase.txt", 1, 0 );

open SHADERLISTFILE, "<$inputbase.txt" || die;
while( $line = <SHADERLISTFILE> )
{
	$line =~ s/\/\/.*$//;
	$line =~ s/^\s*//;
	$line =~ s/\s*$//;
	next if( $line =~ m/^\s*$/ );
	if( $line =~ m/\.fxc/ || $line =~ m/\.vsh/ || $line =~ m/\.psh/ )
	{
		push @srcfiles, $line;
	}
}
close SHADERLISTFILE;

open MAKEFILE, ">makefile\.$inputbase";

# make a default dependency that depends on all of the shaders.
print MAKEFILE "default: ";
foreach $shader ( @srcfiles )
{
	my $shadertype = &GetShaderType( $shader );
	my $shaderbase = &GetShaderBase( $shader );
	if( $shadertype eq "fxc" || $shadertype eq "vsh" )
	{
		# We only generate inc files for fxc and vsh files.
		if( $g_xbox )
		{
			print MAKEFILE " $shadertype" . "tmp_xbox\\" . $shaderbase . "\.inc";
		}
		else
		{
			print MAKEFILE " $shadertype" . "tmp9\\" . $shaderbase . "\.inc";
		}
	}
	if( !$dynamic_compile || $shadertype ne "fxc" )
	{
		print MAKEFILE " $shaderoutputdir\\$shadertype\\$shaderbase\.vcs";
	}
}
print MAKEFILE "\n\n";

# make a "clean" rule
print MAKEFILE "clean:\n";
foreach $shader ( @srcfiles )
{
	my $shadertype = &GetShaderType( $shader );
	my $shaderbase = &GetShaderBase( $shader );
	if( $shadertype eq "fxc" || $shadertype eq "vsh" )
	{
		# We only generate inc files for fxc and vsh files.
		if( $g_xbox )
		{
			print MAKEFILE "\tdel /f /q $shadertype" . "tmp_xbox\\" . $shaderbase . "\.inc\n";
		}
		else
		{
			print MAKEFILE "\tdel /f /q $shadertype" . "tmp9\\" . $shaderbase . "\.inc\n";
		}
	}
	print MAKEFILE "\tdel /f /q \"$shaderoutputdir\\$shadertype\\$shaderbase\.vcs\"\n";
}
print MAKEFILE "\n";


# Insert all of our vertex shaders and depencencies
foreach $shader ( @srcfiles )
{
	&DoAsmShader( $shader );
}
close MAKEFILE;
