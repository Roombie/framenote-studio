# 🐰 Framenote Studio

> Flipnote's soul. Aseprite's precision.

A pixel art and frame-by-frame animation tool built with C++, SDL3, and Dear ImGui.

---

## Mascot

Meet **Nibbit** — the official Framenote Studio mascot. Drawn in Framenote itself. 🐰

---

## Features (v0.1)

- **Pixel art canvas** — draw with precise per-pixel control
- **Frame-by-frame animation** — add, delete, duplicate and reorder frames
- **Onion skinning** — see the previous frame as a ghost while drawing
- **Timeline** — play, pause, loop animations at any FPS
- **32-color palette** — pixel-art friendly default palette including Nibbit blue
- **Tools** — Pencil, Eraser, Fill (bucket), Eyedropper
- **Canvas resize** — resize with pixel data preserved outside bounds
- **Zoom & pan** — scroll to zoom, middle mouse or Space+drag to pan
- **GIF export** — export animations as GIF
- **Save/load** — `.framenote` file format (JSON + base64 PNG frames)

---

## Tech Stack

| Layer | Library |
|---|---|
| Window / Input | SDL3 |
| UI | Dear ImGui |
| Images | stb_image + stb_image_write |
| GIF Export | gif-h |
| File Format | nlohmann/json |
| Build | CMake 3.20+ |

---

## Building (Windows)

### Prerequisites

- CMake 3.20+
- Visual Studio 2022
- SDL3 dev libraries (`SDL3-devel-x.x.x-VC.zip` from [libsdl.org](https://github.com/libsdl-org/SDL/releases))

### Steps

```bash
# 1. Clone the repo
git clone https://github.com/YOURUSERNAME/framenote-studio.git
cd framenote-studio

# 2. Get third-party dependencies
setup-deps.bat

# 3. Configure (point to your SDL3 location)
cmake -B build -S . -DSDL3_DIR="C:/SDL3-3.x.x/cmake"

# 4. Build
cmake --build build

# 5. Copy SDL3.dll next to the exe
copy "C:\SDL3-3.x.x\lib\x64\SDL3.dll" "build\bin\Debug\"

# 6. Run
build\bin\Debug\FramenoteStudio.exe
```

### macOS / Linux

Coming in a future release. The codebase is designed for cross-platform porting.

---

## Controls

| Action | Input |
|---|---|
| Draw | Left click / drag |
| Erase | Select eraser tool + left click |
| Fill | Select fill tool + left click |
| Zoom in/out | Scroll wheel |
| Pan | Middle mouse drag or Space + left drag |
| Select color | Click palette swatch |

---

## Project Structure

```
framenote-studio/
├── src/
│   ├── main.cpp              # Entry point, SDL3 + ImGui init
│   ├── core/                 # Pure logic — no SDL/ImGui dependencies
│   │   ├── Document.cpp      # Root data structure
│   │   ├── Frame.cpp         # Pixel buffer per frame
│   │   ├── Palette.cpp       # 32-color palette
│   │   └── Timeline.cpp      # Playback engine
│   ├── tools/                # Drawing tools
│   │   ├── PencilTool.cpp    # Bresenham line algorithm
│   │   ├── EraserTool.cpp
│   │   └── FillTool.cpp      # BFS flood fill
│   ├── renderer/
│   │   └── CanvasRenderer.cpp
│   ├── ui/                   # Dear ImGui panels
│   │   ├── MainWindow.cpp
│   │   ├── CanvasPanel.cpp
│   │   ├── TimelinePanel.cpp
│   │   ├── ToolsPanel.cpp
│   │   └── PalettePanel.cpp
│   └── io/
│       ├── FileManager.cpp   # .framenote save/load
│       └── GifExporter.cpp
├── include/                  # Headers
├── assets/
│   └── nibbit.ico            # App icon
├── third_party/              # vendored deps (not committed)
│   ├── imgui/
│   ├── stb/
│   ├── nlohmann/
│   └── gif-h/
└── CMakeLists.txt
```

---

## Roadmap

- [ ] Undo / redo (`Ctrl+Z` / `Ctrl+Y`)
- [ ] Keyboard shortcuts (B, E, F, I for tools)
- [ ] New document dialog
- [ ] PNG export
- [ ] Multiple documents (tab system)
- [ ] Custom palettes
- [ ] Layer system
- [ ] Audio per frame (Flipnote style)
- [ ] Web build via Emscripten
- [ ] macOS + Linux builds

---

## License

MIT — see [LICENSE](LICENSE) for details.

---

*Built with SDL3 + Dear ImGui. Mascot drawn in Framenote itself.*