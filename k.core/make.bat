@echo off
cl /c k.c v.c av.c sort.c rand.c sym.c x.c /Fe:gk /O2 /GL
lib /nologo /out:k.lib k.obj v.obj av.obj sort.obj rand.obj sym.obj x.obj
exit /b
