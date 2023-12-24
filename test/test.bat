@echo off
set ec=0
for /f "tokens=*" %%t in (tests) do call :run %%t
:run
if "%1"=="" goto end
..\gk test%1 > %1 2>NUL
comp /a /l /m res%1 %1 > diff%1
if "%errorlevel%"=="0" (echo %1: pass & del %1 diff%1) else set /a ec+=1 & echo %1: fail *****
exit /b
:end
echo failed: %ec%