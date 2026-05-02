cmake_minimum_required(VERSION 3.20)
project(FramenoteStudio VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(SDL3 REQUIRED CONFIG)

set(SOURCES
    src/main.cpp

    # Core
    src/core/Document.cpp
    src/core/Frame.cpp
    src/core/Layer.cpp
    src/core/Canvas.cpp
    src/core/Timeline.cpp
    src/core/History.cpp
    src/core/DocumentTab.cpp
    src/core/Palette.cpp

    # Tools
    src/tools/ToolManager.cpp
    src/tools/PencilTool.cpp
    src/tools/EraserTool.cpp
    src/tools/FillTool.cpp
    src/tools/EyedropperTool.cpp
    src/tools/LineTool.cpp
    src/tools/RectangleTool.cpp
    src/tools/EllipseTool.cpp
    src/tools/SelectionTool.cpp
    src/tools/MoveTool.cpp
    src/core/Selection.cpp

    # Renderer
    src/renderer/CanvasRenderer.cpp

    # UI
    src/ui/MainWindow.cpp
    src/ui/TimelinePanel.cpp
    src/ui/ToolsPanel.cpp
    src/ui/Theme.cpp
    src/ui/TabManager.cpp
    src/ui/PalettePanel.cpp
    src/ui/CanvasPanel.cpp
    src/ui/CanvasPanelActions.cpp
    src/ui/CanvasPanelInput.cpp
    src/ui/CanvasPanelRender.cpp
    src/ui/IconLoader.cpp

    # I/O
    src/io/FileManager.cpp
    src/io/GifExporter.cpp
    src/io/PngExporter.cpp
    src/io/FileDialog.cpp
    src/io/RecentFiles.cpp
    src/io/RecoveryManager.cpp

    # Dear ImGui (vendored)
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/backends/imgui_impl_sdl3.cpp
    third_party/imgui/backends/imgui_impl_sdlrenderer3.cpp
)

# Windows app icon — directly embed the .rc file
if(WIN32)
    file(WRITE ${CMAKE_BINARY_DIR}/app_icon.rc
         "IDI_ICON1 ICON \"${CMAKE_SOURCE_DIR}/assets/nibbit.ico\"\n")
    list(APPEND SOURCES ${CMAKE_BINARY_DIR}/app_icon.rc)
endif()

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    include
    third_party/imgui
    third_party/imgui/backends
    third_party/stb
    third_party/nlohmann
    third_party/gif-h
    third_party
)

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)

if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3::SDL3>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif()

if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE $<CONFIG:Release>
    )
endif()

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

message(STATUS "Framenote Studio v${PROJECT_VERSION} configured")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")