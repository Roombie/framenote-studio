# 🐰 Framenote Studio

> Flipnote's soul. Aseprite's precision.

A pixel art and frame-by-frame animation tool built with C++, SDL3, and Dear ImGui.

---

## Mascot

Meet **Nibbit** — the official Framenote Studio mascot.

---

## Features (v0.1)

**Drawing**
- Pencil, Eraser, Fill (flood fill), Eyedropper tools
- 32-color palette including Nibbit blue
- Canvas resize with pixel data preserved outside bounds
- Mouse coordinates shown in real time (including negative outside canvas)

**Animation**
- Frame-by-frame timeline — add, delete, duplicate frames
- Playback with loop support and adjustable FPS
- Onion skinning with adjustable opacity

**Multi-tab workflow**
- Multiple documents open at once
- Drag tabs to reorder
- Home tab with New Document and Open File
- Unsaved changes dialog on close and quit

**Zoom & navigation**
- Scroll wheel zoom toward cursor
- Middle mouse or Space+drag to pan
- +/- keys to zoom, Ctrl+0 to reset
- Arrow keys to navigate frames

**File operations**
- Save / Open `.framenote` files
- Export as GIF, PNG (single frame), PNG sequence
- Native Windows file dialogs

**UI**
- Dark and light theme
- Keyboard shortcuts: B, E, F, I (tools), C (canvas size), O (onion skin), Ctrl+N/O/S/Z/Y
- Per-tab independent undo/redo history (up to 50 steps)

---

## Known issues (v0.1)

- Tool buttons show `?` instead of icons — coming in v0.2
- Brush size is fixed at 1px — coming in v0.2
- Recent files list not yet implemented — coming in v0.2
- Windows only — macOS and Linux support coming in v0.3

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

## Installation

Download the latest release, extract the zip, and run `FramenoteStudio.exe`. No installer needed.

---

## Building (Windows)

### Prerequisites

- CMake 3.20+
- Visual Studio 2022
- SDL3 dev libraries from [github.com/libsdl-org/SDL/releases](https://github.com/libsdl-org/SDL/releases)

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

Coming in v0.3. The codebase is designed for cross-platform porting — `core/` has zero SDL/ImGui dependencies.

---

## Controls

| Action | Input |
|---|---|
| Draw | Left click / drag |
| Erase | E key, then left click |
| Fill | F key, then left click |
| Eyedropper | I key, then left click |
| Pencil | B key |
| Zoom in/out | Scroll wheel or +/- keys |
| Reset zoom | Ctrl+0 |
| Pan | Middle mouse drag or Space + left drag |
| Previous/next frame | Left/Right arrow |
| Toggle onion skin | O |
| Canvas size | C |
| Undo/Redo | Ctrl+Z / Ctrl+Y |
| New / Open / Save | Ctrl+N / Ctrl+O / Ctrl+S |

---

## Project Structure

```
framenote-studio/
├── src/
│   ├── main.cpp                  # Entry point, SDL3 + ImGui init, main loop
│   ├── core/                     # Pure logic -- no SDL/ImGui dependencies
│   │   ├── Document.cpp          # Root data structure
│   │   ├── Frame.cpp             # Pixel buffer per frame (ARGB8888)
│   │   ├── DocumentTab.cpp       # Owns Document, Timeline, History, Renderer per tab
│   │   ├── History.cpp           # Undo/redo with pixel snapshots
│   │   ├── Palette.cpp           # 32-color palette
│   │   └── Timeline.cpp          # Playback engine with delta time
│   ├── tools/                    # Drawing tools (strategy pattern)
│   │   ├── PencilTool.cpp        # Bresenham line algorithm
│   │   ├── EraserTool.cpp
│   │   ├── FillTool.cpp          # BFS flood fill
│   │   └── EyedropperTool.cpp    # Picks closest palette color
│   ├── renderer/
│   │   └── CanvasRenderer.cpp    # SDL textures, pre-rendered checkerboard
│   ├── ui/                       # Dear ImGui panels
│   │   ├── MainWindow.cpp        # Menu bar, dialogs, keyboard shortcuts
│   │   ├── TabManager.cpp        # Multi-tab system with drag reorder
│   │   ├── CanvasPanel.cpp       # Drawing surface, zoom/pan, coordinates
│   │   ├── TimelinePanel.cpp
│   │   ├── ToolsPanel.cpp
│   │   ├── PalettePanel.cpp
│   │   └── Theme.cpp             # Dark/light theme (Nibbit blue accent)
│   └── io/
│       ├── FileManager.cpp       # .framenote save/load (JSON + base64)
│       ├── FileDialog.cpp        # Native Win32 file dialogs
│       ├── GifExporter.cpp
│       └── PngExporter.cpp       # Single frame + sequence
├── include/                      # Headers mirroring src/ structure
├── assets/
│   └── nibbit.ico                # App icon (hand-drawn 16x16 + 256x256)
├── third_party/                  # Vendored deps (not committed)
│   ├── imgui/
│   ├── stb/
│   ├── nlohmann/
│   └── gif-h/
└── CMakeLists.txt
```

---

## Roadmap

**v0.2**
- [ ] Tool icons
- [ ] Brush size control
- [ ] Custom palettes
- [ ] Recent files list
- [ ] Layer system

**v0.3**
- [ ] Audio per frame (Flipnote style)
- [ ] macOS + Linux builds
- [ ] Web build via Emscripten

---

## License

MIT — see [LICENSE](LICENSE) for details.

---

*Built with SDL3 + Dear ImGui. Mascot drawn in Framenote itself.*