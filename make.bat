@echo off
if "%1%"=="test" goto test
cl main.c p.c timer.c node.c x.c ops.c sym.c sort.c k.c dict.c av.c sys.c systime.c scope.c fn.c avopt.c io.c adcd.c interp.c rand.c os.c md5.c sha1.c sha2.c aes256.c /Fe:gk /O2
exit /b 0
:test
cd test
cmd /c test.bat
cd ..
