#!/bin/bash
set -e

echo "============================================"
echo " Framenote Studio - Dependency Setup"
echo " Mascot: Nibbit the Pixel Bunny 🐰"
echo "============================================"
echo

# Dear ImGui
echo "[1/4] Cloning Dear ImGui..."
if [ ! -d "third_party/imgui" ]; then
    git clone --depth=1 https://github.com/ocornut/imgui.git third_party/imgui
else
    echo "      Already exists, skipping."
fi

# stb
echo ""
echo "[2/4] Downloading stb headers..."
mkdir -p third_party/stb
curl -sL https://raw.githubusercontent.com/nothings/stb/master/stb_image.h       -o third_party/stb/stb_image.h
curl -sL https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h -o third_party/stb/stb_image_write.h
echo "      Done."

# nlohmann/json
echo ""
echo "[3/4] Downloading nlohmann/json..."
mkdir -p third_party/nlohmann
curl -sL https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp \
     -o third_party/nlohmann/json.hpp
echo "      Done."

# gif-h
echo ""
echo "[4/4] Downloading gif-h..."
mkdir -p third_party/gif-h
curl -sL https://raw.githubusercontent.com/charlietangora/gif-h/master/gif.h \
     -o third_party/gif-h/gif.h
echo "      Done."

echo ""
echo "============================================"
echo " All dependencies ready!"
echo ""
echo " Next steps (Linux):"
echo "   sudo apt install libsdl2-dev"
echo " Next steps (macOS):"
echo "   brew install sdl2"
echo ""
echo "   cmake -B build -S ."
echo "   cmake --build build"
echo "   ./build/bin/FramenoteStudio"
echo "============================================"
