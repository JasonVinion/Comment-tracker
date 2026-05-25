@echo off
set VS_PATH=C:\Program Files\Microsoft Visual Studio\18\Community
set BUILD_CONFIG=Release
if /i "%1"=="debug" set BUILD_CONFIG=Debug
set PROJECT_DIR=%~dp0
set BUILD_DIR=%PROJECT_DIR%out\build\x64-%BUILD_CONFIG%
echo Building %BUILD_CONFIG%...
powershell -NoProfile -Command "& '%VS_PATH%\Common7\Tools\Launch-VsDevShell.ps1' -Arch amd64; cd '%PROJECT_DIR%'; cmake -B '%BUILD_DIR%' -S . -G Ninja -DCMAKE_BUILD_TYPE=%BUILD_CONFIG%; cmake --build '%BUILD_DIR%'"
if exist "%BUILD_DIR%\CommentExplorer.exe" (echo SUCCESS) else (echo FAILED & exit /b 1)
