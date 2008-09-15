@echo off
setlocal


if /i "%1" == "-game" goto CleanGameDir


rem Clean out hl2
if exist ..\..\..\game\hl2\shaders rd /s /q ..\..\..\game\hl2\shaders
goto CleanOtherStuff


:CleanGameDir
set __GameDir=%~2
if not exist "%__GameDir%\gameinfo.txt" goto MissingGameInfo
if exist "%__GameDir%\shaders" rd /s /q "%2\shaders"
goto CleanOtherStuff


:CleanOtherStuff
if exist debug_dx9 rd /s /q debug_dx9
if exist fxctmp9 rd /s /q fxctmp9
if exist pshtmp9 rd /s /q pshtmp9
if exist shaders rd /s /q shaders
if exist vshtmp9 rd /s /q vshtmp9
goto end


:MissingGameInfo
echo Invalid -game parameter specified (no "%__GameDir%\gameinfo.txt" exists).
goto end


:end