#include <SDL3/SDL.h>
#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>

#include <memory>
#include <string>

#include "ui/Theme.h"
#include "ui/MainWindow.h"
#include "ui/TabManager.h"
#include "ui/IconLoader.h"
#include "tools/ToolManager.h"

static constexpr int WINDOW_W = 1280;
static constexpr int WINDOW_H = 800;

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Framenote Studio v0.2.0",
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

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = "imgui.ini";
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;  // Enable docking
    (void)io;

    float dpiScale = SDL_GetWindowDisplayScale(window);
    ImGui::GetStyle().ScaleAllSizes(dpiScale);
    ImGui::GetIO().FontGlobalScale = dpiScale;

    Framenote::Theme::applyDark();

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    // Load icons before TabManager so they can be passed in
    Framenote::ToolIcons icons;
    if (!icons.load(renderer))
        SDL_Log("Warning: some tool icons failed to load");

    Framenote::ToolManager toolManager;
    Framenote::TabManager  tabManager(renderer, &icons);
    Framenote::MainWindow  mainWindow(&tabManager, &toolManager);

    bool running      = true;
    bool pendingQuit  = false;
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

        // Tick the active tab's timeline for animation playback
        {
            static Uint64 lastTime = SDL_GetPerformanceCounter();
            Uint64 now = SDL_GetPerformanceCounter();
            float deltaTime = (float)(now - lastTime) / (float)SDL_GetPerformanceFrequency();
            lastTime = now;
            if (deltaTime > 0.1f) deltaTime = 0.1f;
            auto* tab = tabManager.activeTab();
            if (tab) tab->timeline->tick(deltaTime);
        }

        mainWindow.render();

        if (showQuitDialog) ImGui::OpenPopup("Quit##confirm");
        if (ImGui::BeginPopupModal("Quit##confirm", nullptr,
                ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("You have unsaved changes.\nDo you want to save before quitting?");
            ImGui::Separator();
            if (ImGui::Button("Don't Save", {120, 0})) {
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
    icons.destroy();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}