@echo off
setlocal enabledelayedexpansion
set ec=0
for /f "usebackq tokens=*" %%t in ("tests") do (
    set line=%%t
    if not "!line!"=="" if not "!line:~0,1!"=="#" call :run !line!
)
goto end

:run
if "%1"=="" exit /b
..\gk t%1 > %1 2>NUL
comp /a /l /m r%1 %1 > d%1
if "%errorlevel%"=="0" (
    echo t%1: pass
    del %1 d%1
) else (
    set /a ec+=1
    echo t%1: fail *****
)
exit /b

:end
echo failed: %ec%
