@echo off
rem Build all Windows ext packages with MSVC. Run from a Visual Studio
rem "x64 Native Tools" command prompt.
rem   make           build every package
rem   make test      build + test every package
rem   make clean     clean every package
setlocal
cd /d "%~dp0"
set "TARGET=%~1"
set "RC=0"
for /d %%p in (*) do (
  if exist "%%p\make.bat" (
    echo === %%p ===
    call "%%p\make.bat" %TARGET%
    if errorlevel 1 set "RC=1"
  )
)
exit /b %RC%
