@echo off
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo Requesting administrative privileges...
    powershell -Command "Start-Process '%~f0' -Verb RunAs"
    exit /b
)

setlocal enabledelayedexpansion

set "found="

for %%c in (python python3 python2 py) do (
    echo Checking python executable: '%%c'
    where %%c >nul 2>&1 && %%c --version >nul 2>&1 && (
        set "found=%%c"
        goto :found
    )
)

echo Valid python executable found. Runnnig setup...

:found
if defined found (
    %found% setup.py -s --vulkan-sdk --cmake --visual-studio
) else (
    echo Python is required to run the setup.
    exit /b 1
)

pause
