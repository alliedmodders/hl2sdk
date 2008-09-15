$dynamic_compile = defined $ENV{"dynamic_shaders"} && $ENV{"dynamic_shaders"} != 0;

$cwd = Win32::GetFullPathName(".");

$changeToDir = "";
$noMPI = "";
$xboxFlag = "";
$gameFlag = "";
$shaderoutputdir = "";

while( 1 )
{
	$argname = shift;

	if( $argname =~ m/-xbox/i )
	{
		# parse it out, to be used later
		$xboxFlag = "-xbox";
	}
	elsif( $argname =~ m/-changetodir/i )
	{
		$changeToDir = shift;
	}
	elsif( $argname =~ m/-game/i )
	{
		$gameFlag = "-game \"" . (shift) . "\"";
	}
	elsif( $argname =~ m/-nompi/i )
	{
		$noMPI = "-nompi";
	}
	elsif( $argname =~ m/-shaderoutputdir/i )
	{
		$shaderoutputdir = shift;
	}
	else
	{
		last;
	}
}

if( !stat "filelist.txt" || !stat "uniquefilestocopy.txt" )
{
	exit;
}

$shaderpath = $cwd;
$shaderpath =~ s,/,\\,g;
chdir $changeToDir;
print $cwd;

# pc
#$cmdToRun = "calc";
$cmdToRun = "\"$ENV{\"sourcesdk\"}\\bin\\shadercompile.exe\" -game -nompi -shaderpath $shaderpath -allowdebug $gameFlag";
if( !$dynamic_compile )
{
	system $cmdToRun;
}
