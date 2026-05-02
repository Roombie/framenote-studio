# Changelog

All notable changes to Framenote Studio are documented here.

---

## [v0.2.0] — 2026-05-02

### Added

- Tool icons for all drawing and editing tools
- Brush size control with `[` and `]` shortcuts
- Basic palette editing
- Selection tool with marching ants outline
- Move tool with full-canvas and floating-pixel behavior
- Line, rectangle, and ellipse tools with filled/outline modes
- Shape previews that match the final committed result
- Selection-aware drawing, erasing, fill, line, rectangle, and ellipse behavior
- Recent projects home screen with thumbnails, pinning, and path display
- Recovery files UI for crash recovery
- Drag-and-drop file opening for `.framenote` files
- Canvas resize support
- Onion skinning with configurable opacity
- Per-document FPS and animation state
- Reorderable tabs including the Home tab
- Home screen interaction blocking while any modal is open
- Modal centering and viewport-clamping so dialogs stay visible on window resize
- Dark and light theme support with theme-aware recent project thumbnails
- Export current frame as PNG, animation as PNG sequence, or animation as GIF
- Per-tab undo/redo history
- Keyboard shortcut for canvas size dialog (`C`)

### Fixed

- Recent project card hover state incorrectly active while a modal was open
- Modal dialogs drifting off-screen after the application window was resized
- Recent project card backgrounds and borders not reflecting the active theme
- Framenote project file paths not being normalized on save
- Red/blue channel swap during PNG and GIF export
- Canvas export and save correctly respecting visible canvas size when the backing buffer is larger

### Changed

- CanvasPanel split into `CanvasPanel`, `CanvasPanelActions`, `CanvasPanelInput`, and `CanvasPanelRender` for maintainability
- Selection-aware pixel clipping centralized into `SelectionPixelClip` so all drawing tools behave consistently
- All modal dialogs share positioning helpers from `ModalUtils`
- New Document modal moved into the home tab window context so popup state is always consistent
- Improved save/load validation and error reporting
- CMake exporter sources cleaned up

---

## [v0.1.0] — Initial release

- Basic pixel canvas with pencil and eraser tools
- Frame-by-frame timeline
- Animation playback
- `.framenote` save/load format
- PNG export
- GIF export
