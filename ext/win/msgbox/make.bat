@echo off
rem Build the Win32 msgbox extension with MSVC. Run from a Visual Studio
rem "x64 Native Tools" command prompt so cl.exe and the SDK are on PATH.
rem   make           build msgbox.dll
rem   make test      build, then run test.k (pops the modal)
rem   make clean     remove build artifacts
setlocal
cd /d "%~dp0"
set "GKDIR=..\..\.."
set "GK=%GKDIR%\gk.exe"
set "TARGET=%~1"
if "%TARGET%"=="" set "TARGET=all"

if /I "%TARGET%"=="clean" (
  del /q msgbox.dll msgbox.obj msgbox.exp msgbox.lib 2>nul
  goto :eof
)

rem gk.lib is the import library emitted next to gk.exe when gk is built on
rem Windows; a client DLL must link it so the gk_* imports bind. user32.lib
rem provides MessageBoxA.
cl /nologo /O2 /LD /I"%GKDIR%" msgbox.c "%GKDIR%\gk.lib" user32.lib /Femsgbox.dll
if errorlevel 1 exit /b 1

if /I "%TARGET%"=="test" "%GK%" -q test.k
