# Framenote Studio

**Flipnote’s soul. Aseprite’s precision.**

Framenote Studio is a lightweight pixel animation editor inspired by Flipnote Studio and Aseprite. It focuses on quick frame-by-frame drawing, simple animation playback, onion skinning, palette-based pixel art, selection-aware editing, and export tools for small animations.

Current version: **v0.2.0**

---

## Features

### Drawing

- Pencil tool
- Eraser tool
- Fill tool
- Eyedropper tool
- Line tool
- Rectangle tool
- Ellipse tool
- Selection tool
- Move tool
- Adjustable brush size
- Filled and outline shape drawing
- Palette-based color selection
- Primary and secondary color support
- Transparent canvas support
- Pixel-perfect drawing workflow
- Selection-aware drawing, erasing, fill, line, rectangle, and ellipse behavior
- Shape previews that match the final committed result
- Delete / Backspace support for selected pixels or full-frame clearing

### Selection and movement

- Rectangular selection tool
- Marching ants selection outline
- Floating selected-pixel movement
- Full-canvas move tool behavior
- Selection-aware pixel editing
- Delete selected pixels while keeping the selection outline
- Commit floating selection/canvas movement with Enter
- Deselect/cancel selection with Escape
- Photoshop/Aseprite-like separation between selected-pixel movement and full-canvas movement

### Animation

- Frame-by-frame timeline
- Add, duplicate, and delete frames
- Previous/next frame navigation
- Playback preview
- Adjustable FPS
- Onion skinning
- Per-document animation state

### Documents

- Multiple document tabs
- Reorderable tabs, including the Home tab
- Custom canvas size when creating a document
- Canvas resize support
- Per-document FPS
- Native `.framenote` save/load format
- Recent projects/home screen support
- Recovery files UI
- Drag-and-drop opening for compatible files

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
- Modal dialog interaction blocking where needed
- Theme-aware recent/recovery thumbnails

---

## Known issues / limitations

- Native file dialogs are currently implemented for Windows.
- macOS and Linux builds are planned, but cross-platform file dialog support still needs to be added.
- Layer system is not yet implemented.
- Advanced transform tools are not yet implemented.
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
| Line tool | L |
| Rectangle tool | R |
| Ellipse tool | O |
| Selection tool | M |
| Move tool | V |
| New document | Ctrl + N |
| Open document | Ctrl + O |
| Save document | Ctrl + S |
| Undo | Ctrl + Z |
| Redo | Ctrl + Y |
| Canvas size | C |
| Previous frame | Left Arrow |
| Next frame | Right Arrow |
| First frame | Shift + Left Arrow |
| Last frame | Shift + Right Arrow |
| Play / Pause | Enter |
| Commit floating selection/canvas movement | Enter |
| Deselect / cancel selection | Escape |
| Delete selected pixels / clear current frame | Delete / Backspace |
| Increase brush size | ] |
| Decrease brush size | [ |
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

Contains the document model, frames, canvas data, timeline state, palette data, tab state, and undo/redo history.

Important classes include:

- `Document`
- `Frame`
- `Timeline`
- `History`
- `Palette`
- `DocumentTab`
- `Selection`

### `tools/`

Contains drawing and editing tools that operate on frame data.

Current tools:

- `PencilTool`
- `EraserTool`
- `FillTool`
- `EyedropperTool`
- `LineTool`
- `RectangleTool`
- `EllipseTool`
- `SelectionTool`
- `MoveTool`

Shared tool helpers include:

- `ShapeRasterizer`
- `SelectionPixelClip`

`SelectionPixelClip` centralizes selection-aware pixel editing so tools such as Pencil, Eraser, Fill, Line, Rectangle, and Ellipse can all respect the active selection consistently.

### `renderer/`

Handles canvas rendering through SDL3 and ImGui integration.

### `ui/`

Contains the application interface, including the main window, canvas panel, timeline panel, tools panel, palette panel, tab manager, theme handling, icon loading, home screen, recent projects UI, and recovery UI.

### `io/`

Handles saving, loading, native file dialogs, PNG export, GIF export, and file/document operations.

---

## Canvas panel architecture

The canvas UI is split by responsibility to keep the editor maintainable as it grows:

- `CanvasPanel.cpp` handles the high-level canvas render/input pipeline.
- `CanvasPanelActions.cpp` handles editor actions such as floating commits, delete behavior, snapshots, and tool event creation.
- `CanvasPanelInput.cpp` handles keyboard shortcuts, zoom, pan, cursor behavior, and tool input routing.
- `CanvasPanelRender.cpp` handles canvas rendering helpers, floating pixels, marching ants, rubber-band selection, and shape previews.

This keeps `CanvasPanel::render()` focused on the main flow:

```text
upload frame
draw canvas base
draw floating pixels
draw selection overlays
draw shape previews
draw selection rubber band
prepare canvas input region
handle cursor
handle shortcuts
handle zoom/pan
handle tool input
draw zoom label
```

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

### Selection-aware editing

Pixel-modifying tools use shared selection clipping logic so drawing, erasing, filling, and shape tools only modify selected pixels when a selection is active.

Affected tools include:

- Pencil
- Eraser
- Fill
- Line
- Rectangle
- Ellipse

Selection, Move, and Eyedropper are handled differently because they do not use normal clipped pixel drawing:

- Selection creates and manipulates selected regions.
- Move moves floating selection/canvas data.
- Eyedropper reads pixel color without modifying pixels.

### Shape previews

Line, rectangle, and ellipse previews are rendered using the same selection-aware clipping rules as the final committed result.

This prevents previews from showing pixels outside the active selection when the committed drawing would be clipped.

### Playback shortcut

Space is reserved for canvas panning with `Space + left drag`.

Playback uses `Enter`.

---

## Roadmap

### v0.2.0

- [x] Tool icons
- [x] Brush size control
- [x] Basic palette editing
- [x] More drawing tools
- [x] Selection tool
- [x] Move tool
- [x] Line, rectangle, and ellipse tools
- [x] Recent projects/home screen support
- [x] Recovery files UI
- [x] Drag-and-drop file opening
- [x] Selection-aware drawing and shape previews
- [x] CanvasPanel refactor by responsibility
- [x] Improved save/load validation
- [x] Better export options

### Future versions

- [ ] Layer system
- [ ] Advanced transform tools
- [ ] Cross-platform native file dialogs
- [ ] macOS and Linux packaging
- [ ] Timeline improvements
- [ ] Animation preview improvements
- [ ] Custom workspace/layout settings
- [ ] More robust `.framenote` format versioning
- [ ] More advanced selection modes
- [ ] More advanced palette workflows

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