@echo off
setlocal
set CFILES=src\p.c src\lex.c src\timer.c src\k.c src\main.c src\repl.c src\dict.c src\systime.c src\scope.c src\fn.c src\b.c src\v.c src\av.c src\ms.c src\h.c src\pnp.c src\fe.c src\lzw.c src\md5.c src\sha1.c src\sha2.c src\aes256.c src\io.c src\irecur.c src\la.c src\nt.c src\ipc.c src\tmr.c src\watch.c src\ffi.c
set COREFILES=src\k.core\k.c src\k.core\v.c src\k.core\av.c src\k.core\sort.c src\k.core\rand.c src\k.core\sym.c src\k.core\x.c

if "%1%"=="test" goto test
if "%1%"=="testp" goto testp
if "%1%"=="clean" goto clean

if not "%VSCMD_ARG_TGT_ARCH%"=="x64" ( echo error: not an x64 toolchain ^(VSCMD_ARG_TGT_ARCH=%VSCMD_ARG_TGT_ARCH%^), run vcvars64.bat first & exit /b 1 )
if not exist obj\core md obj\core
del /q obj\*.obj obj\core\*.obj 2>nul
cl /c /O2 /GL /I. %CFILES% /Fo:obj\
if errorlevel 1 exit /b 1
cl /c /O2 /GL %COREFILES% /Fo:obj\core\
if errorlevel 1 exit /b 1
cl obj\*.obj obj\core\*.obj /Fe:gk /link /LTCG /STACK:8388608
exit /b 0

:test
if not exist ndiff.exe cl /nologo /O2 src\ndiff.c /Fe:ndiff.exe
if exist ndiff.obj del ndiff.obj
cd t
call t.bat
if errorlevel 1 ( cd .. & exit /b 1 )
cd ..
exit /b 0

:testp
cd t
call p.bat
cd ..
exit /b 0

:clean
rd /s /q obj 2>nul
del /q gk.exe gk.exp gk.lib gk.ilk ndiff.exe *.obj *.pdb 2>nul
for %%d in (0 1 2 3 4 5 6 7 8 9) do (del /q t\%%d* t\d%%d* 2>nul)
exit /b 0
