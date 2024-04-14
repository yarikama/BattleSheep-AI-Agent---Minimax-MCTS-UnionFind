@echo off
setlocal

set "iterations=5"
set "timeout=25"

for /l %%i in (1,1,%iterations%) do (
    echo Iteration %%i
    echo Iteration %%i >> result.txt
    
    powershell -Command "./AI_game.exe" >> result.txt 2>&1
    
    timeout /t %timeout% /nobreak > nul
    taskkill /F /IM "AI_game.exe" > nul 2>&1
    
    echo.
    echo. >> test.txt
)

endlocal