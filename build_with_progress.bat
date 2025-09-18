@echo off
setlocal enabledelayedexpansion

:: Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

@set SOURCE_DIR_WITH_BACKSLASH=%~dp0
@set SOURCE_DIR=%SOURCE_DIR_WITH_BACKSLASH:~0,-1%

:: Initialize the MSVC environment.
call %SOURCE_DIR%\build_utils\msvc\call_vcvars64.bat || @goto :exit

:: Make the build dir at the same level as the parent dir of this script, suffixed with "-build".
@set BUILD_DIR=%SOURCE_DIR%-build

@if exist "%BUILD_DIR%\CMakeCache.txt" goto :skip_generation
    @mkdir "%BUILD_DIR%" 2>NUL

    :: Ensure that only the currently supplied CMake arguments are in effect.
    del "%BUILD_DIR%\CMakeCache.txt" 2>NUL

    :: Use the Ninja generator to avoid using CMake compiler search heuristics for MSBuild. Also,
    :: specify the compiler explicitly to avoid potential clashes with gcc if ran from a
    :: Cygwin/MinGW shell.
    GENERATOR_OPTIONS=-GNinja -DCMAKE_C_COMPILER=cl.exe -DCMAKE_CXX_COMPILER=cl.exe -DCMAKE_CXX_FLAGS="/W1 /wd4535 /wd4996 /MDd" -DCMAKE_C_FLAGS="/W1 /MDd" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDebugDLL
    echo [CMAKE] Generating build files...
    cmake "%SOURCE_DIR%" %GENERATOR_OPTIONS% -B "%BUILD_DIR%" %* || @goto :exit
:skip_generation

:: Get total number of targets for progress calculation
echo [PROGRESS] Calculating build targets...
for /f %%i in ('cmake --build "%BUILD_DIR%" --target help ^| find /c "..."') do set TOTAL_TARGETS=%%i
if "%TOTAL_TARGETS%"=="" set TOTAL_TARGETS=100

echo [BUILD] Starting build of %TOTAL_TARGETS% targets...
echo [BUILD] ================================================

:: Build with progress tracking
cmake --build "%BUILD_DIR%" --config Debug --parallel 4 -- /maxcpucount:4 /verbosity:normal /p:WarningLevel=1 /flp:logfile=build.log;verbosity=diagnostic /consoleloggerparameters:PerformanceSummary;Summary;ShowTimestamp

echo.
echo [BUILD] ================================================
echo [BUILD] Build completed!

:exit
    @exit /b %ERRORLEVEL% %= Needed for a proper cmd.exe exit status. =%
