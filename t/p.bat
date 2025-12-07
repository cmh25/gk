@echo off

for %%i in (p001 p002 p003 p004 p005) do (
    echo %%i =========================
    ..\gk %%i
    echo.
)

exit /b 0