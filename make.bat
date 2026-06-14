@echo off
setlocal
set CFILES=p.c lex.c timer.c k.c main.c repl.c dict.c systime.c scope.c fn.c b.c v.c av.c ms.c h.c pnp.c fe.c lzw.c md5.c sha1.c sha2.c aes256.c io.c irecur.c la.c nt.c ipc.c tmr.c watch.c ffi.c
set COREFILES=k.core\k.c k.core\v.c k.core\av.c k.core\sort.c k.core\rand.c k.core\sym.c k.core\x.c
if "%1%"=="test" goto test
if "%1%"=="testp" goto testp
if not "%VSCMD_ARG_TGT_ARCH%"=="x64" ( echo error: not an x64 toolchain ^(VSCMD_ARG_TGT_ARCH=%VSCMD_ARG_TGT_ARCH%^), run vcvars64.bat first & exit /b 1 )
if not exist obj\core md obj\core
del /q obj\*.obj obj\core\*.obj 2>nul
cl /c /O2 /GL %CFILES% /Fo:obj\
if errorlevel 1 exit /b 1
cl /c /O2 /GL %COREFILES% /Fo:obj\core\
if errorlevel 1 exit /b 1
cl obj\*.obj obj\core\*.obj /Fe:gk /link /LTCG
exit /b 0
:test
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
