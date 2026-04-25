# 🐰 Framenote Studio v0.1

> Pixel art animation — approachable like Flipnote, powerful like Aseprite.

Mascot: **Nibbit** 🐰

---

## Tech Stack

| Layer       | Library                  |
|-------------|--------------------------|
| Window/Input| SDL2                     |
| UI          | Dear ImGui               |
| Images      | stb_image / stb_image_write |
| GIF Export  | gif-h                    |
| File Format | nlohmann/json            |
| Build       | CMake 3.20+              |

---

## Project Structure

```
framenote-studio/
├── src/
│   ├── main.cpp              # Entry point, SDL2 + ImGui init, main loop
│   ├── core/                 # Pure logic — NO SDL/ImGui dependencies
│   │   ├── Document.cpp      # Root data structure (.framenote in memory)
│   │   ├── Frame.cpp         # Single animation frame (RGBA pixel buffer)
│   │   ├── Palette.cpp       # 32-color palette
│   │   └── Timeline.cpp      # Playback engine (FPS, onion skin)
│   ├── tools/                # Drawing tools (strategy pattern)
│   │   ├── ToolManager.cpp
│   │   ├── PencilTool.cpp    # Bresenham pixel drawing
│   │   ├── EraserTool.cpp
│   │   └── FillTool.cpp      # BFS flood fill
│   ├── renderer/
│   │   └── CanvasRenderer.cpp # SDL2 texture upload + draw
│   ├── ui/                   # Dear ImGui panels
│   │   ├── MainWindow.cpp
│   │   ├── CanvasPanel.cpp
│   │   ├── TimelinePanel.cpp
│   │   ├── ToolsPanel.cpp
│   │   └── PalettePanel.cpp
│   └── io/
│       ├── FileManager.cpp   # .framenote save/load
│       └── GifExporter.cpp   # GIF export
├── include/                  # Headers mirroring src/
├── third_party/
│   ├── imgui/                # Dear ImGui (clone from github)
│   ├── stb/                  # stb_image.h + stb_image_write.h
│   ├── nlohmann/             # json.hpp
│   └── gif-h/                # gif.h
└── CMakeLists.txt
```

---

## Building (Windows)

### Prerequisites
- CMake 3.20+
- Visual Studio 2022 (or MinGW)
- SDL3 development libraries

### Steps

```bash
# 1. Clone third-party dependencies
cd third_party
git clone https://github.com/ocornut/imgui.git
# Download stb_image.h and stb_image_write.h into third_party/stb/
# Download json.hpp into third_party/nlohmann/
# Download gif.h into third_party/gif-h/

# 2. Configure
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug

# 3. Build
cmake --build build

# 4. Run
./build/bin/FramenoteStudio
```

---

## v0.1 Roadmap

- [x] Project scaffold
- [ ] Blank canvas renders
- [ ] Pencil tool draws pixels
- [ ] Eraser tool
- [ ] Fill (bucket) tool
- [ ] 32-color palette panel
- [ ] Add / delete frames
- [ ] Frame list UI
- [ ] Playback (play/pause/loop)
- [ ] Onion skinning
- [ ] Save .framenote file
- [ ] Load .framenote file
- [ ] Export GIF

---

## Architecture Rule

> **`core/` has zero SDL2 or ImGui includes.**
> This keeps the door open for mobile (Flutter FFI) and web (Emscripten) later.
