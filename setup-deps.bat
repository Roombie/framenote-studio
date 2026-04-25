@echo off
echo ============================================
echo  Framenote Studio - Dependency Setup
echo  Mascot: Nibbit the Pixel Bunny
echo ============================================
echo.

REM Check for git
where git >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] git not found. Install from https://git-scm.com
    pause & exit /b 1
)

REM Check for curl
where curl >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] curl not found. Install from https://curl.se
    pause & exit /b 1
)

echo [1/4] Cloning Dear ImGui...
if not exist "third_party\imgui" (
    git clone --depth=1 https://github.com/ocornut/imgui.git third_party/imgui
) else (
    echo       Already exists, skipping.
)

echo.
echo [2/4] Downloading stb headers...
if not exist "third_party\stb" mkdir third_party\stb
curl -sL https://raw.githubusercontent.com/nothings/stb/master/stb_image.h       -o third_party/stb/stb_image.h
curl -sL https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h -o third_party/stb/stb_image_write.h
echo       Done.

echo.
echo [3/4] Downloading nlohmann/json...
if not exist "third_party\nlohmann" mkdir third_party\nlohmann
curl -sL https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp -o third_party/nlohmann/json.hpp
echo       Done.

echo.
echo [4/4] Downloading gif-h...
if not exist "third_party\gif-h" mkdir third_party\gif-h
curl -sL https://raw.githubusercontent.com/charlietangora/gif-h/master/gif.h -o third_party/gif-h/gif.h
echo       Done.

echo.
echo ============================================
echo  All dependencies ready!
echo.
echo  Next steps:
echo  1. Make sure SDL3 is extracted (e.g. C:\SDL3-3.4.4\)
echo  2. Run:  cmake -B build -S . -DSDL3_DIR="C:/SDL3-3.4.4/cmake"
echo  3. Run:  cmake --build build
echo  4. Run:  build\bin\Debug\FramenoteStudio.exe
echo ============================================
pause
