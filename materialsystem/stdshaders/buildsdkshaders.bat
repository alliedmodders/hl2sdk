@echo off
REM ****************
REM usage: buildshaders <shaderProjectName> 
REM ****************

setlocal
set targetdir=shaders
set SrcDirBase=..\..
set ChangeToDir=../../../game/bin
set shaderDir=shaders
set SDKArgs=

if "%1" == "" goto usage
set inputbase=%1

if /i "%2" == "-game" goto set_mod_args
goto build_shaders

REM ****************
REM USAGE
REM ****************
:usage
echo.
echo "usage: buildsdkshaders <shaderProjectName> [-game] [gameDir if -game was specified] [-source sourceDir]"
echo "       gameDir is where gameinfo.txt is (where it will store the compiled shaders)."
echo "       sourceDir is where the source code is (where it will find scripts and compilers)."
echo "ex   : buildshaders myshaders"
echo "ex   : buildshaders myshaders -game c:\steam\steamapps\sourcemods\mymod -source c:\mymod\src"
goto :end

REM ****************
REM MOD ARGS - look for -game or the vproject environment variable
REM ****************
:set_mod_args

if not exist "%sourcesdk%\bin\shadercompile.exe" goto NoShaderCompile
set ChangeToDir=%sourcesdk%\bin

if /i "%4" NEQ "-source" goto NoSourceDirSpecified
set SrcDirBase=%~5

REM ** use the -game parameter to tell us where to put the files
set targetdir=%~3\shaders
set SDKArgs=-nompi -game "%~3"

if not exist "%~3\gameinfo.txt" goto InvalidGameDirectory
goto build_shaders

REM ****************
REM ERRORS
REM ****************
:InvalidGameDirectory
echo -
echo Error: "%~3" is not a valid game directory.
echo (The -game directory must have a gameinfo.txt file)
echo -
goto end

:NoSourceDirSpecified
echo ERROR: If you specify -game on the command line, you must specify -source.
goto usage
goto end

:NoShaderCompile
echo -
echo - ERROR: shadercompile.exe doesn't exist in %sourcesdk%\bin
echo -
goto end


REM ****************
REM BUILD SHADERS
REM ****************
:build_shaders

echo --------------------------------
echo %inputbase%
echo --------------------------------
REM make sure that target dirs exist
REM files will be built in these targets and copied to their final destination
if not exist %shaderDir% mkdir %shaderDir%
if not exist %shaderDir%\fxc mkdir %shaderDir%\fxc
if not exist %shaderDir%\vsh mkdir %shaderDir%\vsh
if not exist %shaderDir%\psh mkdir %shaderDir%\psh
REM Nuke some files that we will add to later.
if exist filelist.txt del /f /q filelist.txt
if exist filestocopy.txt del /f /q filestocopy.txt

REM ****************
REM Generate a makefile for the shader project
REM ****************
perl ..\..\devtools\bin\updateshaders.pl -source ..\.. %inputbase% 

REM ****************
REM Run the makefile, generating minimal work/build list for fxc files, go ahead and compile vsh and psh files.
REM ****************
nmake /S /C -f makefile.%inputbase%

REM ****************
REM Cull duplicate entries in work/build list
REM ****************
if exist filestocopy.txt type filestocopy.txt | sort | "perl" "..\..\devtools\bin\uniq.pl" > uniquefilestocopy.txt
if exist filelist.txt type filelist.txt | sort | "perl" "..\..\devtools\bin\uniq.pl" > uniquefilelist.txt
if exist uniquefilelist.txt move uniquefilelist.txt filelist.txt

REM ****************
REM Execute distributed process on work/build list
REM ****************
perl "%SrcDirBase%\materialsystem\stdshaders\runvmpi.pl" %SDKArgs%

REM ****************
REM Copy the generated files to the output dir using XCOPY
REM ****************
:DoXCopy
if not exist "%targetdir%" md "%targetdir%"
if not "%targetdir%"=="%shaderDir%" xcopy %shaderDir%\*.* "%targetdir%" /e
goto end

REM ****************
REM END
REM ****************
:end

