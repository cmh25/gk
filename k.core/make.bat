@echo off
if "%1%"=="gka" goto gka
cl /c k.c v.c av.c sort.c rand.c sym.c x.c /Fe:gk /O2 /GL
lib /nologo /out:k.lib k.obj v.obj av.obj sort.obj rand.obj sym.obj x.obj
exit /b
:gka
clang-cl /c -fsanitize=address,undefined /Od /Zi /MT k.c v.c av.c sort.c rand.c sym.c x.c
lib /nologo /out:k.lib k.obj v.obj av.obj sort.obj rand.obj sym.obj x.obj
exit /b
