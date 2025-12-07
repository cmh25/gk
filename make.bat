@echo off
setlocal
set CFILES=p.c lex.c timer.c k.c main.c repl.c dict.c systime.c scope.c fn.c b.c v.c av.c ms.c h.c pnp.c fe.c lzw.c md5.c sha1.c sha2.c aes256.c io.c irecur.c la.c
if "%1%"=="test" goto test
if "%1%"=="testp" goto testp
cd k.core
call make.bat
cd ..
cl %CFILES% k.core/k.lib /Fe:gk /O2 /GL /link /LTCG
exit /b 0
:test
cd t
call t.bat
cd ..
exit /b 0
:testp
cd t
call p.bat
cd ..
exit /b 0
