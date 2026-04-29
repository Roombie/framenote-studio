# Framenote Studio

**Flipnote’s soul. Aseprite’s precision.**

Framenote Studio is a lightweight pixel animation editor inspired by Flipnote Studio and Aseprite. It focuses on quick frame-by-frame drawing, simple animation playback, onion skinning, palette-based pixel art, and export tools for small animations.

Current version: **v0.2.0**

---

## Features

### Drawing

- Pencil tool
- Eraser tool
- Fill tool
- Eyedropper tool
- Adjustable brush size
- Palette-based color selection
- Transparent canvas support
- Pixel-perfect drawing workflow

### Animation

- Frame-by-frame timeline
- Add, duplicate, and delete frames
- Previous/next frame navigation
- Playback preview
- Adjustable FPS
- Onion skinning

### Documents

- Multiple document tabs
- Custom canvas size when creating a document
- Canvas resize support
- Per-document FPS
- Native `.framenote` save/load format

### Export

- Export current frame as PNG
- Export animation as PNG sequence
- Export animation as GIF
- Correct color conversion for exported images
- Export respects visible canvas size after resizing

### UI

- Dark and light theme
- Tool icons
- Timeline controls
- Palette panel
- Canvas panning and zooming
- Keyboard shortcuts
- Per-tab undo/redo history

---

## Known issues / limitations

- Recent files list is not yet implemented.
- Native file dialogs are currently implemented for Windows.
- macOS and Linux builds are planned, but cross-platform file dialog support still needs to be added.
- Layer system is not yet implemented.
- The native `.framenote` format is still early and may change before v1.0.

---

## Requirements

### Windows

- Windows 10 or newer
- Visual Studio 2022
- CMake 3.20 or newer
- SDL3 development package

### macOS / Linux

The app is built with SDL3 and Dear ImGui, so the core code is designed to be portable. However, native file dialogs are currently Windows-only.

You will need:

- CMake 3.20 or newer
- A C++17 compiler
- SDL3 development files
- Git, if using the dependency setup scripts

---

## Getting started

### 1. Clone the repository

```bash
git clone <your-repo-url>
cd framenote-studio
```

---

### 2. Get third-party dependencies

On Windows:

```powershell
.\setup-deps.bat
```

On macOS / Linux:

```bash
./setup-deps.sh
```

These scripts download the header/source dependencies used by the project, such as Dear ImGui, stb, nlohmann/json, and gif-h.

SDL3 must still be installed separately or provided to CMake through `SDL3_DIR`.

---

## Building on Windows

If CMake can already find SDL3:

```powershell
cmake -B .\build -S .
cmake --build .\build --config Debug
```

If CMake cannot find SDL3, provide the SDL3 CMake path manually:

```powershell
cmake -B .\build -S . -DSDL3_DIR="C:\SDL3\cmake"
cmake --build .\build --config Debug
```

Run the app:

```powershell
.\build\bin\Debug\FramenoteStudio.exe
```

For Release:

```powershell
cmake --build .\build --config Release
.\build\bin\Release\FramenoteStudio.exe
```

---

## Building on macOS / Linux

Install SDL3 development files through your package manager, or provide `SDL3_DIR` manually when configuring CMake.

```bash
./setup-deps.sh

cmake -B build -S .
cmake --build build
```

Run the app:

```bash
./build/bin/FramenoteStudio
```

Note: native file dialogs are currently Windows-only, so some open/save dialog behavior may need a cross-platform backend before macOS/Linux builds are fully comfortable to use.

---

## Controls

| Action | Shortcut |
|---|---|
| Pencil tool | B |
| Eraser tool | E |
| Fill tool | F |
| Eyedropper tool | I |
| New document | Ctrl + N |
| Open document | Ctrl + O |
| Save document | Ctrl + S |
| Undo | Ctrl + Z |
| Redo | Ctrl + Y |
| Canvas size | C |
| Toggle onion skin | O |
| Previous frame | Left Arrow |
| Next frame | Right Arrow |
| Play / Pause | Enter |
| Zoom in / out | Mouse wheel or +/- |
| Reset zoom | Ctrl + 0 |
| Pan | Middle mouse drag or Space + left drag |

---

## Project structure

```text
framenote-studio/
├── assets/
│   ├── icons/
│   └── nibbit.ico
│
├── include/
│   ├── core/
│   ├── io/
│   ├── renderer/
│   ├── tools/
│   └── ui/
│
├── src/
│   ├── core/
│   ├── io/
│   ├── renderer/
│   ├── tools/
│   └── ui/
│
├── third_party/
│   ├── gif-h/
│   ├── imgui/
│   ├── nlohmann/
│   └── stb/
│
├── CMakeLists.txt
├── setup-deps.bat
├── setup-deps.sh
└── README.md
```

---

## Core architecture

Framenote Studio is split into independent systems:

### `core/`

Contains the document model, frames, canvas data, timeline state, palette data, and undo/redo history.

Important classes include:

- `Document`
- `Frame`
- `Timeline`
- `History`
- `Palette`
- `DocumentTab`

### `tools/`

Contains drawing tools that operate on frame data.

Current tools:

- `PencilTool`
- `EraserTool`
- `FillTool`
- `EyedropperTool`

### `renderer/`

Handles canvas rendering through SDL3 and ImGui integration.

### `ui/`

Contains the application interface, including the main window, canvas panel, timeline panel, tools panel, palette panel, tab manager, theme handling, and icon loading.

### `io/`

Handles saving, loading, native file dialogs, PNG export, and GIF export.

---

## File format

Framenote Studio uses a custom `.framenote` format.

The current format stores:

- Canvas width and height
- FPS
- Palette colors
- Frame pixel data encoded as PNG data inside the document structure

The format is still considered early and may change before v1.0.

---

## Development notes

### Pixel format

Internally, frame memory uses SDL-style ARGB8888 byte layout on little-endian systems, which appears in memory as BGRA.

Exporters convert this internal format to RGBA before writing PNG or GIF output.

This prevents red/blue channel swapping during export and save/load round trips.

### Canvas resizing

Frames may keep a larger backing buffer after the visible canvas is resized smaller. Export and save logic should always respect the visible canvas size while using the backing buffer width as the row stride.

### Playback shortcut

Space is reserved for canvas panning with `Space + left drag`.

Playback uses `Enter`.

---

## Roadmap

### v0.2.0

- [x] Tool icons
- [x] Brush size control
- [x] Basic palette editing
- [ ] Recent files list
- [ ] More drawing tools
- [ ] Improved save/load validation
- [ ] Better export options

### Future versions

- [ ] Layer system
- [ ] Selection tools
- [ ] Move / transform tools
- [ ] Line, rectangle, and circle tools
- [ ] Cross-platform native file dialogs
- [ ] macOS and Linux packaging
- [ ] Timeline improvements
- [ ] Animation preview improvements
- [ ] Custom workspace/layout settings
- [ ] More robust `.framenote` format versioning

---

## Clean build

If the build folder becomes corrupted or Visual Studio reports PDB/linker issues, delete the build folder and configure again.

```powershell
Remove-Item -Recurse -Force .\build
cmake -B .\build -S . -DSDL3_DIR="C:\SDL3\cmake"
cmake --build .\build --config Debug
```

If the application compiles but does not open, make sure `SDL3.dll` exists beside the executable:

```text
build/bin/Debug/SDL3.dll
```

The CMake configuration copies `SDL3.dll` automatically on Windows after a successful build.

---

## License

License information has not been finalized yet.

---

## Credits

Framenote Studio uses:

- SDL3
- Dear ImGui
- stb
- nlohmann/json
- gif-h

Mascot and visual identity: **Nibbit**