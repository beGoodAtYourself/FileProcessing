@echo off
setlocal
if "%~1"=="" (
    echo Usage: run-svg-tests.cmd path-to-Qt-msvc2013_64
    exit /b 2
)
set "SVG_QT_DIR=%~f1"
if not exist "%SVG_QT_DIR%\bin\qmake.exe" exit /b 2
if not defined VS120COMNTOOLS (
    echo Visual Studio 2013 environment was not found.
    exit /b 2
)
call "%VS120COMNTOOLS%..\..\VC\vcvarsall.bat" amd64
if errorlevel 1 exit /b 1
set "PATH=%SVG_QT_DIR%\bin;%~dp0..\Output;%PATH%"
set "QT_QPA_PLATFORM=offscreen"
set "QT_QPA_FONTDIR=%WINDIR%\Fonts"
if not exist "%~dp0.build" mkdir "%~dp0.build"
pushd "%~dp0.build"
"%SVG_QT_DIR%\bin\qmake.exe" "%~dp0svg-export-tests.pro"
if errorlevel 1 goto failed
nmake /nologo
if errorlevel 1 goto failed
svg-export-tests.exe -o test-results.txt,txt
if errorlevel 1 goto failed
type test-results.txt
popd
exit /b 0
:failed
if exist test-results.txt type test-results.txt
popd
exit /b 1
