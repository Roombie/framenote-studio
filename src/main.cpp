#include <SDL3/SDL.h>
#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>

#include <memory>
#include <string>

#include "ui/Theme.h"
#include "ui/MainWindow.h"
#include "ui/TabManager.h"
#include "tools/ToolManager.h"

static constexpr int   WINDOW_W   = 1280;
static constexpr int   WINDOW_H   = 800;

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Framenote Studio v0.1",
        WINDOW_W, WINDOW_H,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY
    );
    if (!window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_SetRenderVSync(renderer, 1);

    // â”€â”€ Dear ImGui â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = "imgui.ini";
    (void)io;

    // DPI fix
    float dpiScale = SDL_GetWindowDisplayScale(window);
    ImGui::GetStyle().ScaleAllSizes(dpiScale);
    ImGui::GetIO().FontGlobalScale = dpiScale;

    // Apply Framenote dark theme
    Framenote::Theme::applyDark();

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    // â”€â”€ App state â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    Framenote::ToolManager toolManager;
    Framenote::TabManager  tabManager(renderer);
    Framenote::MainWindow  mainWindow(&tabManager, &toolManager);

    // â”€â”€ Main loop â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    bool running = true;
    bool pendingQuit = false;
    bool showQuitDialog = false;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                pendingQuit = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                event.window.windowID == SDL_GetWindowID(window))
                pendingQuit = true;
        }

        // Quit confirmation if any unsaved tabs
        if (pendingQuit) {
            pendingQuit = false;
            if (tabManager.hasUnsavedTabs())
                showQuitDialog = true;
            else
                running = false;
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        mainWindow.render();

        // Quit confirmation dialog
        if (showQuitDialog) ImGui::OpenPopup("Quit##confirm");
        if (ImGui::BeginPopupModal("Quit##confirm", nullptr,
                ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("You have unsaved changes. Quit anyway?");
            ImGui::Separator();
            if (ImGui::Button("Quit without saving", {180, 0})) {
                running = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", {80, 0})) {
                showQuitDialog = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::Render();

        // SDL clear color matches theme
        if (Framenote::Theme::current() == Framenote::ThemeMode::Dark)
            SDL_SetRenderDrawColor(renderer, 20, 18, 16, 255);
        else
            SDL_SetRenderDrawColor(renderer, 248, 246, 240, 255);

        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
